/**
 * @file lldrawpoolsimple.cpp
 * @brief LLDrawPoolSimple class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolsimple.h"

#include "llviewercamera.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llviewershadermgr.h"
#include "llrenderbackend.h"
#include "llrender.h"
#include "llworldrendercommand.h"
#include "gltfscenemanager.h"

//MK
#include "llagent.h"
#include "llvovolume.h"
#include "llrenderstate.h"
//mk

static LLTrace::BlockTimerStatHandle FTM_RENDER_SIMPLE_DEFERRED("Deferred Simple");
static LLTrace::BlockTimerStatHandle FTM_RENDER_GRASS_DEFERRED("Deferred Grass");

namespace
{
U32 with_weight4_attribute(U32 mask)
{
    return mask | static_cast<U32>(LLVertexBuffer::MAP_WEIGHT4);
}

U32 get_glow_vertex_data_mask(bool rigged)
{
    U32 mask =
        LLVertexBuffer::MAP_VERTEX |
        LLVertexBuffer::MAP_TEXCOORD0 |
        LLVertexBuffer::MAP_TEXTURE_INDEX |
        LLVertexBuffer::MAP_EMISSIVE;

    return rigged ? with_weight4_attribute(mask) : mask;
}
}

bool LLDrawPoolGlow::emitPostDeferredCommands(LLWorldRenderCommandBuffer& commands, S32 pass)
{
    if (gAgent.mRRInterface.mVisionRestricted)
    {
        return false;
    }

    commands.appendRenderMap(
        LLRenderPass::PASS_GLOW,
        LLWorldRenderMaterialClass::Glow,
        true,
        true,
        get_glow_vertex_data_mask(false));
    commands.appendRenderMap(
        LLRenderPass::PASS_GLOW_RIGGED,
        LLWorldRenderMaterialClass::Glow,
        true,
        true,
        get_glow_vertex_data_mask(true));
    return true;
}

void LLDrawPoolGlow::renderPostDeferred(S32 pass)
{
//MK
    // If the vision is restricted, don't show any glow in-world (attachments work)
    if (gAgent.mRRInterface.mVisionRestricted)
    {
        return;
    }
//mk
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    if (use_vulkan_world_command_path() && !LLPipeline::sRenderingHUDs)
    {
        LLWorldRenderCommandBuffer commands;
        emitPostDeferredCommands(commands, pass);
        submit_vulkan_world_commands(commands);
        return;
    }

    LLGLSLShader* shader = &gDeferredEmissiveProgram;

    LLGLEnable blend(LLRenderCapability::Blend);
    gGL.flush();
    /// Get rid of z-fighting with non-glow pass.
    LLGLEnable polyOffset(LLRenderCapability::PolygonOffsetFill);
    getRenderBackend().setPolygonOffset(-1.0f, -1.0f);
    gGL.setSceneBlendType(LLRender::BT_ADD);

    LLGLDepthTest depth(true, false);
    gGL.setColorMask(false, true);

    //first pass -- static objects
    shader->bind();
    pushBatches(LLRenderPass::PASS_GLOW, true, true);

    // second pass -- rigged objects
    shader = shader->mRiggedVariant;
    shader->bind();
    pushRiggedBatches(LLRenderPass::PASS_GLOW_RIGGED, true, true);

    gGL.setColorMask(true, false);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

LLDrawPoolSimple::LLDrawPoolSimple() :
    LLRenderPass(POOL_SIMPLE)
{
}

static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_MASK("Alpha Mask");

LLDrawPoolAlphaMask::LLDrawPoolAlphaMask() :
    LLRenderPass(POOL_ALPHA_MASK)
{
}

LLDrawPoolFullbrightAlphaMask::LLDrawPoolFullbrightAlphaMask() :
    LLRenderPass(POOL_FULLBRIGHT_ALPHA_MASK)
{
}

//===============================
//DEFERRED IMPLEMENTATION
//===============================

S32 LLDrawPoolSimple::getNumDeferredPasses()
{
    return 1;
}

bool LLDrawPoolSimple::emitDeferredCommands(LLWorldRenderCommandBuffer& commands, S32 pass)
{
    commands.appendRenderMap(
        LLRenderPass::PASS_SIMPLE,
        LLWorldRenderMaterialClass::SimpleOpaque,
        true,
        true,
        VERTEX_DATA_MASK);
    commands.appendRenderMap(
        LLRenderPass::PASS_SIMPLE_RIGGED,
        LLWorldRenderMaterialClass::SimpleOpaque,
        true,
        true,
        with_weight4_attribute(VERTEX_DATA_MASK));
    return true;
}

void LLDrawPoolSimple::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_SIMPLE_DEFERRED);
    LLGLDisable blend(LLRenderCapability::Blend);

    if (use_vulkan_world_command_path() && !LLPipeline::sRenderingHUDs)
    {
        static bool logged = false;
        if (!logged)
        {
            LL_INFOS("RenderBackend")
                << "Vulkan simple path is emitting static simple commands."
                << LL_ENDL;
            logged = true;
        }

        LLWorldRenderCommandBuffer commands;
        emitDeferredCommands(commands, pass);
        submit_vulkan_world_commands(commands);
        return;
    }

    //render static
    gDeferredDiffuseProgram.bind();
    pushBatches(LLRenderPass::PASS_SIMPLE, true, true);

    //render rigged
    gDeferredDiffuseProgram.bind(true);
    pushRiggedBatches(LLRenderPass::PASS_SIMPLE_RIGGED, true, true);
}

static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_MASK_DEFERRED("Deferred Alpha Mask");

bool LLDrawPoolAlphaMask::emitDeferredCommands(LLWorldRenderCommandBuffer& commands, S32 pass)
{
    commands.appendRenderMap(
        LLRenderPass::PASS_ALPHA_MASK,
        LLWorldRenderMaterialClass::AlphaMask,
        true,
        true,
        VERTEX_DATA_MASK);
    commands.appendRenderMap(
        LLRenderPass::PASS_ALPHA_MASK_RIGGED,
        LLWorldRenderMaterialClass::AlphaMask,
        true,
        true,
        with_weight4_attribute(VERTEX_DATA_MASK));
    return true;
}

void LLDrawPoolAlphaMask::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_MASK_DEFERRED);

    if (use_vulkan_world_command_path() && !LLPipeline::sRenderingHUDs)
    {
        static bool logged = false;
        if (!logged)
        {
            LL_INFOS("RenderBackend")
                << "Vulkan simple path is emitting static alpha-mask commands."
                << LL_ENDL;
            logged = true;
        }

        LLWorldRenderCommandBuffer commands;
        emitDeferredCommands(commands, pass);
        submit_vulkan_world_commands(commands);
        return;
    }

    LLGLSLShader* shader = &gDeferredDiffuseAlphaMaskProgram;

    //render static
    shader->bind();
    pushMaskBatches(LLRenderPass::PASS_ALPHA_MASK, true, true);

    //render rigged
    shader->bind(true);
    pushRiggedMaskBatches(LLRenderPass::PASS_ALPHA_MASK_RIGGED, true, true);
}

// grass drawpool
LLDrawPoolGrass::LLDrawPoolGrass() :
 LLRenderPass(POOL_GRASS)
{

}

bool LLDrawPoolGrass::emitDeferredCommands(LLWorldRenderCommandBuffer& commands, S32 pass)
{
    commands.appendRenderMap(
        LLRenderPass::PASS_GRASS,
        LLWorldRenderMaterialClass::Grass,
        true,
        true,
        VERTEX_DATA_MASK);
    return true;
}

void LLDrawPoolGrass::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    if (use_vulkan_world_command_path() && !LLPipeline::sRenderingHUDs)
    {
        static bool logged = false;
        if (!logged)
        {
            LL_INFOS("RenderBackend")
                << "Vulkan simple path is emitting grass commands."
                << LL_ENDL;
            logged = true;
        }

        LLWorldRenderCommandBuffer commands;
        emitDeferredCommands(commands, pass);
        submit_vulkan_world_commands(commands);
        return;
    }

    {
        gDeferredNonIndexedDiffuseAlphaMaskProgram.bind();
        gDeferredNonIndexedDiffuseAlphaMaskProgram.setMinimumAlpha(0.5f);

        //render grass
        LLRenderPass::pushBatches(LLRenderPass::PASS_GRASS, getVertexDataMask());
    }
}

// Fullbright drawpool
LLDrawPoolFullbright::LLDrawPoolFullbright() :
    LLRenderPass(POOL_FULLBRIGHT)
{
}

bool LLDrawPoolFullbright::emitPostDeferredCommands(LLWorldRenderCommandBuffer& commands, S32 pass)
{
    commands.appendRenderMap(
        LLRenderPass::PASS_FULLBRIGHT,
        LLWorldRenderMaterialClass::Fullbright,
        true,
        true,
        VERTEX_DATA_MASK);

    if (!LLPipeline::sRenderingHUDs)
    {
        commands.appendRenderMap(
            LLRenderPass::PASS_FULLBRIGHT_RIGGED,
            LLWorldRenderMaterialClass::Fullbright,
            true,
            true,
            with_weight4_attribute(VERTEX_DATA_MASK));
    }
    return true;
}

void LLDrawPoolFullbright::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_FULLBRIGHT);

    if (use_vulkan_world_command_path() && !LLPipeline::sRenderingHUDs)
    {
        LLWorldRenderCommandBuffer commands;
        emitPostDeferredCommands(commands, pass);
        submit_vulkan_world_commands(commands);
        return;
    }

    LLGLSLShader* shader = nullptr;
    if (LLPipeline::sRenderingHUDs)
    {
        shader = &gHUDFullbrightProgram;
    }
    else
    {
        shader = &gDeferredFullbrightProgram;
    }

    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    // render static
    shader->bind();
    pushBatches(LLRenderPass::PASS_FULLBRIGHT, true, true);

    if (!LLPipeline::sRenderingHUDs)
    {
        // render rigged
        shader->bind(true);
        pushRiggedBatches(LLRenderPass::PASS_FULLBRIGHT_RIGGED, true, true);
    }
}

bool LLDrawPoolFullbrightAlphaMask::emitPostDeferredCommands(LLWorldRenderCommandBuffer& commands, S32 pass)
{
    commands.appendRenderMap(
        LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK,
        LLWorldRenderMaterialClass::FullbrightAlphaMask,
        true,
        true,
        VERTEX_DATA_MASK);

    if (!LLPipeline::sRenderingHUDs)
    {
        commands.appendRenderMap(
            LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED,
            LLWorldRenderMaterialClass::FullbrightAlphaMask,
            true,
            true,
            with_weight4_attribute(VERTEX_DATA_MASK));
    }
    return true;
}

void LLDrawPoolFullbrightAlphaMask::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_FULLBRIGHT);

    if (use_vulkan_world_command_path() && !LLPipeline::sRenderingHUDs)
    {
        LLWorldRenderCommandBuffer commands;
        emitPostDeferredCommands(commands, pass);
        submit_vulkan_world_commands(commands);
        return;
    }

    // render unrigged unlit GLTF
    LL::GLTFSceneManager::instance().render(true, false, true);
    LL::GLTFSceneManager::instance().render(true, true, true);

    LLGLSLShader* shader = nullptr;
    if (LLPipeline::sRenderingHUDs)
    {
        shader = &gHUDFullbrightAlphaMaskProgram;
    }
    else
    {
        shader = &gDeferredFullbrightAlphaMaskProgram;
    }

    LLGLDisable blend(LLRenderCapability::Blend);

    // render static
    shader->bind();
    pushMaskBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, true, true);

    if (!LLPipeline::sRenderingHUDs)
    {
        // render rigged
        shader->bind(true);
        pushRiggedMaskBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED, true, true);
    }
}
