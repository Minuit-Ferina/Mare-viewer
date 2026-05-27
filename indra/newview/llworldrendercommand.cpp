/**
 * @file llworldrendercommand.cpp
 * @brief Backend-neutral world draw command buffer.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
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
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llworldrendercommand.h"

#include "lldrawable.h"
#include "lldrawpool.h"
#include "llface.h"
#include "m4math.h"
#include "llrenderbackend.h"
#include "llrenderstate.h"
#include "llspatialpartition.h"
#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "pipeline.h"

namespace
{
LLWorldRenderPassClass get_world_render_pass_class(LLWorldRenderMaterialClass material_class)
{
    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Fullbright:
        case LLWorldRenderMaterialClass::FullbrightAlphaMask:
        case LLWorldRenderMaterialClass::Alpha:
        case LLWorldRenderMaterialClass::Glow:
        case LLWorldRenderMaterialClass::Water:
        case LLWorldRenderMaterialClass::FullbrightShiny:
        case LLWorldRenderMaterialClass::PostBump:
            return LLWorldRenderPassClass::PostDeferred;
        default:
            return LLWorldRenderPassClass::Deferred;
    }
}

LLWorldRenderBlendMode get_world_render_blend_mode(LLWorldRenderMaterialClass material_class)
{
    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Alpha:
        case LLWorldRenderMaterialClass::Water:
            return LLWorldRenderBlendMode::Alpha;
        case LLWorldRenderMaterialClass::Glow:
            return LLWorldRenderBlendMode::Add;
        default:
            return LLWorldRenderBlendMode::None;
    }
}

LLWorldRenderDepthMode get_world_render_depth_mode(LLWorldRenderMaterialClass material_class)
{
    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Alpha:
        case LLWorldRenderMaterialClass::Glow:
        case LLWorldRenderMaterialClass::Water:
            return LLWorldRenderDepthMode::ReadOnly;
        default:
            return LLWorldRenderDepthMode::ReadWrite;
    }
}

LLWorldRenderCullMode get_world_render_cull_mode(LLWorldRenderMaterialClass material_class)
{
    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Alpha:
        case LLWorldRenderMaterialClass::Water:
            return LLWorldRenderCullMode::Disabled;
        default:
            return LLWorldRenderCullMode::Back;
    }
}

void classify_world_render_command(LLWorldRenderCommand& command)
{
    command.mPassClass = get_world_render_pass_class(command.mMaterialClass);
    command.mBlendMode = get_world_render_blend_mode(command.mMaterialClass);
    command.mDepthMode = get_world_render_depth_mode(command.mMaterialClass);
    command.mCullMode = get_world_render_cull_mode(command.mMaterialClass);
}

bool is_material_alpha_mask_source_pass(U32 source_pass)
{
    switch (source_pass)
    {
        case LLRenderPass::PASS_MATERIAL_ALPHA_MASK:
        case LLRenderPass::PASS_MATERIAL_ALPHA_MASK_RIGGED:
        case LLRenderPass::PASS_SPECMAP_MASK:
        case LLRenderPass::PASS_SPECMAP_MASK_RIGGED:
        case LLRenderPass::PASS_NORMMAP_MASK:
        case LLRenderPass::PASS_NORMMAP_MASK_RIGGED:
        case LLRenderPass::PASS_NORMSPEC_MASK:
        case LLRenderPass::PASS_NORMSPEC_MASK_RIGGED:
            return true;
        default:
            return false;
    }
}

F32 get_world_render_alpha_mask_cutoff(const LLWorldRenderCommand& command)
{
    switch (command.mMaterialClass)
    {
        case LLWorldRenderMaterialClass::AlphaMask:
        case LLWorldRenderMaterialClass::Grass:
        case LLWorldRenderMaterialClass::Tree:
        case LLWorldRenderMaterialClass::FullbrightAlphaMask:
        case LLWorldRenderMaterialClass::GLTFPBRAlphaMask:
            return command.mAlphaMaskCutoff;
        case LLWorldRenderMaterialClass::LegacyMaterial:
            return is_material_alpha_mask_source_pass(command.mSourcePass) ?
                command.mAlphaMaskCutoff :
                -1.f;
        default:
            return -1.f;
    }
}

LLRenderWorldTextureTransform get_world_texture_transform(const LLMatrix4* matrix)
{
    LLRenderWorldTextureTransform transform;
    if (!matrix)
    {
        return transform;
    }

    transform.mS[0] = matrix->mMatrix[0][0];
    transform.mS[1] = matrix->mMatrix[1][0];
    transform.mS[2] = matrix->mMatrix[2][0];
    transform.mS[3] = matrix->mMatrix[3][0];

    transform.mT[0] = matrix->mMatrix[0][1];
    transform.mT[1] = matrix->mMatrix[1][1];
    transform.mT[2] = matrix->mMatrix[2][1];
    transform.mT[3] = matrix->mMatrix[3][1];
    return transform;
}

LLRender::eBlendType get_scene_blend_type(LLWorldRenderBlendMode mode)
{
    switch (mode)
    {
        case LLWorldRenderBlendMode::Alpha:
            return LLRender::BT_ALPHA;
        case LLWorldRenderBlendMode::Add:
            return LLRender::BT_ADD_WITH_ALPHA;
        case LLWorldRenderBlendMode::None:
        default:
            return LLRender::BT_REPLACE;
    }
}

void apply_world_render_command_state(const LLWorldRenderCommand& command)
{
    getRenderBackend().setCapability(
        LLRenderCapability::Blend,
        command.mBlendMode != LLWorldRenderBlendMode::None);
    gGL.setSceneBlendType(get_scene_blend_type(command.mBlendMode));

    const bool depth_enabled = command.mDepthMode != LLWorldRenderDepthMode::Disabled;
    getRenderBackend().setCapability(LLRenderCapability::DepthTest, depth_enabled);
    getRenderBackend().setDepthFunction(LLRenderDepthFunction::LessEqual);
    getRenderBackend().setDepthWriteEnabled(command.mDepthMode == LLWorldRenderDepthMode::ReadWrite);

    getRenderBackend().setCapability(
        LLRenderCapability::CullFace,
        command.mCullMode == LLWorldRenderCullMode::Back);
    if (command.mCullMode == LLWorldRenderCullMode::Back)
    {
        getRenderBackend().setCullFace(LLRenderCullFace::Back);
    }
}
}

bool use_vulkan_world_command_path()
{
    return getRenderBackend().getType() == LLRenderBackendType::Vulkan &&
        getRenderBackend().isReady();
}

void LLWorldRenderCommandBuffer::clear()
{
    mCommands.clear();
}

void LLWorldRenderCommandBuffer::appendDrawInfo(
    const LLDrawInfo& params,
    LLWorldRenderMaterialClass material_class,
    U32 source_pass,
    bool texture,
    bool batch_textures,
    U32 attribute_mask)
{
    if (!params.mCount || params.mVertexBuffer.isNull())
    {
        return;
    }

    appendDrawRange(
        params.mVertexBuffer,
        params.mTexture,
        material_class,
        source_pass,
        params.mModelMatrix,
        params.mStart,
        params.mEnd,
        params.mCount,
        params.mOffset,
        texture,
        batch_textures,
        attribute_mask);

    LLWorldRenderCommand& command = mCommands.back();
    command.mTextureList = params.mTextureList;
    command.mTextureMatrix = params.mTextureMatrix;
    command.mAlphaMaskCutoff = params.mAlphaMaskCutoff;
    if (batch_textures && command.mTextureList.size() > 1)
    {
        command.mAttributeMask |= LLVertexBuffer::MAP_TEXTURE_INDEX;
    }
}

void LLWorldRenderCommandBuffer::appendFace(
    const LLFace& face,
    LLWorldRenderMaterialClass material_class,
    U32 source_pass)
{
    LLVertexBuffer* vertex_buffer = face.getVertexBuffer();
    if (!vertex_buffer || !face.getIndicesCount() || !face.getGeomCount())
    {
        return;
    }

    const LLDrawable* drawable = face.getDrawable();
    const LLViewerRegion* region = drawable ? drawable->getRegion() : nullptr;

    LLWorldRenderCommand command;
    command.mMaterialClass = material_class;
    classify_world_render_command(command);
    command.mSourcePass = source_pass;
    command.mAttributeMask =
        LLVertexBuffer::MAP_VERTEX |
        LLVertexBuffer::MAP_TEXCOORD0;
    command.mVertexBuffer = vertex_buffer;
    command.mModelMatrix = region ? &region->mRenderMatrix : nullptr;
    command.mStart = face.getGeomIndex();
    command.mEnd = face.getGeomIndex() + face.getGeomCount() - 1;
    command.mCount = face.getIndicesCount();
    command.mOffset = face.getIndicesStart();
    command.mUseTexture = false;
    command.mBatchTextures = false;
    mCommands.push_back(command);
}

void LLWorldRenderCommandBuffer::appendTerrainFace(
    const LLFace& face,
    LLViewerTexture* detail_texture0,
    LLViewerTexture* detail_texture1,
    LLViewerTexture* detail_texture2,
    LLViewerTexture* detail_texture3,
    LLViewerTexture* alpha_ramp,
    F32 detail_scale,
    F32 offset_x,
    F32 offset_y)
{
    const size_t command_count = mCommands.size();
    appendFace(face, LLWorldRenderMaterialClass::Terrain, LLDrawPool::POOL_TERRAIN);
    if (mCommands.size() == command_count)
    {
        return;
    }

    LLWorldRenderCommand& command = mCommands.back();
    command.mAttributeMask |= LLVertexBuffer::MAP_TEXCOORD1;
    command.mTexture = detail_texture0;
    command.mTextureList.clear();
    command.mTextureList.reserve(5);
    command.mTextureList.push_back(detail_texture0);
    command.mTextureList.push_back(detail_texture1);
    command.mTextureList.push_back(detail_texture2);
    command.mTextureList.push_back(detail_texture3);
    command.mTextureList.push_back(alpha_ramp);
    command.mUseTexture = true;
    command.mBatchTextures = true;
    command.mTerrainDetailScale = detail_scale;
    command.mTerrainOffsetX = offset_x;
    command.mTerrainOffsetY = offset_y;
}

void LLWorldRenderCommandBuffer::appendDrawRange(
    LLVertexBuffer* vertex_buffer,
    LLViewerTexture* texture,
    LLWorldRenderMaterialClass material_class,
    U32 source_pass,
    const LLMatrix4* model_matrix,
    U32 start,
    U32 end,
    U32 count,
    U32 offset,
    bool use_texture,
    bool batch_textures,
    U32 attribute_mask)
{
    if (!vertex_buffer || !count)
    {
        return;
    }

    LLWorldRenderCommand command;
    command.mMaterialClass = material_class;
    classify_world_render_command(command);
    command.mSourcePass = source_pass;
    command.mAttributeMask = attribute_mask;
    command.mVertexBuffer = vertex_buffer;
    command.mTexture = texture;
    command.mModelMatrix = model_matrix;
    command.mStart = start;
    command.mEnd = end;
    command.mCount = count;
    command.mOffset = offset;
    command.mUseTexture = use_texture;
    command.mBatchTextures = batch_textures;
    mCommands.push_back(command);
}

void LLWorldRenderCommandBuffer::appendRenderMap(
    U32 source_pass,
    LLWorldRenderMaterialClass material_class,
    bool texture,
    bool batch_textures,
    U32 attribute_mask)
{
    auto* begin = gPipeline.beginRenderMap(source_pass);
    auto* end = gPipeline.endRenderMap(source_pass);
    for (LLCullResult::drawinfo_iterator iter = begin; iter != end; )
    {
        LLDrawInfo* params = *iter;
        LLCullResult::increment_iterator(iter, end);
        if (params)
        {
            appendDrawInfo(
                *params,
                material_class,
                source_pass,
                texture,
                batch_textures,
                attribute_mask);
        }
    }
}

void submit_vulkan_world_commands(const LLWorldRenderCommandBuffer& command_buffer)
{
    if (command_buffer.empty())
    {
        return;
    }

    struct LLScopedWorldDraw
    {
        LLScopedWorldDraw()
        {
            getRenderBackend().setWorldDrawEnabled(true);
        }

        ~LLScopedWorldDraw()
        {
            getRenderBackend().setWorldDrawEnabled(false);
        }
    } scoped_world_draw;

    const U32 saved_attribute_mask = gUIProgram.mAttributeMask;
    U32 active_attribute_mask = 0;
    bool program_bound = false;
    LLWorldRenderBlendMode active_blend_mode = LLWorldRenderBlendMode::None;
    LLWorldRenderDepthMode active_depth_mode = LLWorldRenderDepthMode::ReadWrite;
    LLWorldRenderCullMode active_cull_mode = LLWorldRenderCullMode::Back;
    bool state_bound = false;

    for (const LLWorldRenderCommand& command : command_buffer.commands())
    {
        LLVertexBuffer* vertex_buffer = command.mVertexBuffer.get();
        if (!vertex_buffer || !command.mCount)
        {
            continue;
        }

        if (!state_bound ||
            active_blend_mode != command.mBlendMode ||
            active_depth_mode != command.mDepthMode ||
            active_cull_mode != command.mCullMode)
        {
            apply_world_render_command_state(command);
            active_blend_mode = command.mBlendMode;
            active_depth_mode = command.mDepthMode;
            active_cull_mode = command.mCullMode;
            state_bound = true;
        }

        getRenderBackend().setAlphaMaskCutoff(get_world_render_alpha_mask_cutoff(command));
        const bool uses_texture_matrix =
            command.mUseTexture &&
            (!command.mBatchTextures || command.mTextureList.size() <= 1);
        getRenderBackend().setWorldTextureTransform(
            get_world_texture_transform(
                uses_texture_matrix ?
                    command.mTextureMatrix :
                    nullptr));
        if (command.mMaterialClass == LLWorldRenderMaterialClass::Terrain)
        {
            getRenderBackend().setWorldShaderClass(LLRenderWorldShaderClass::Terrain);
            getRenderBackend().setWorldTerrainParameters(
                {
                    command.mTerrainDetailScale,
                    command.mTerrainOffsetX,
                    command.mTerrainOffsetY,
                });
        }
        else
        {
            getRenderBackend().setWorldShaderClass(LLRenderWorldShaderClass::Textured);
        }

        if (!program_bound || active_attribute_mask != command.mAttributeMask)
        {
            if (program_bound)
            {
                gUIProgram.unbind();
            }

            active_attribute_mask = command.mAttributeMask;
            gUIProgram.mAttributeMask = active_attribute_mask;
            gUIProgram.bind();
            program_bound = true;
        }

        LLRenderPass::applyModelMatrix(command.mModelMatrix);

        bool tex_setup = false;
        if (command.mUseTexture)
        {
            if (command.mBatchTextures && command.mTextureList.size() > 1)
            {
                for (U32 i = 0; i < command.mTextureList.size(); ++i)
                {
                    if (command.mTextureList[i].notNull())
                    {
                        gGL.getTexUnit(i)->bindFast(command.mTextureList[i]);
                    }
                    else
                    {
                        gGL.getTexUnit(i)->unbindFast(LLTexUnit::TT_TEXTURE);
                    }
                }
            }
            else if (command.mTexture.notNull())
            {
                gGL.getTexUnit(0)->bindFast(command.mTexture);
                if (command.mTextureMatrix)
                {
                    tex_setup = true;
                    gGL.getTexUnit(0)->activate();
                    gGL.matrixMode(LLRender::MM_TEXTURE);
                    gGL.loadMatrix((F32*) command.mTextureMatrix->mMatrix);
                    gPipeline.mTextureMatrixOps++;
                }
            }
            else
            {
                gGL.getTexUnit(0)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
        }
        else
        {
            gGL.getTexUnit(0)->unbindFast(LLTexUnit::TT_TEXTURE);
        }

        vertex_buffer->setBuffer();
        vertex_buffer->drawRange(
            LLRender::TRIANGLES,
            command.mStart,
            command.mEnd,
            command.mCount,
            command.mOffset);

        if (tex_setup)
        {
            gGL.matrixMode(LLRender::MM_TEXTURE0);
            gGL.loadIdentity();
            gGL.matrixMode(LLRender::MM_MODELVIEW);
        }
    }

    if (program_bound)
    {
        gUIProgram.unbind();
    }
    getRenderBackend().setAlphaMaskCutoff(-1.f);
    getRenderBackend().setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    getRenderBackend().setWorldTerrainParameters({});
    gUIProgram.mAttributeMask = saved_attribute_mask;
}
