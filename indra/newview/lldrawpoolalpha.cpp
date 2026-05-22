/**
 * @file lldrawpoolalpha.cpp
 * @brief LLDrawPoolAlpha class implementation
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

#include "lldrawpoolalpha.h"

#include "llglheaders.h"
#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "llfasttimer.h"
#include "llrender.h"

#include "llcubemap.h"
#include "llsky.h"
#include "lldrawable.h"
#include "llface.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"    // For debugging
#include "llviewerobjectlist.h" // For debugging
#include "llviewerwindow.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llviewerregion.h"
#include "lldrawpoolwater.h"
#include "llspatialpartition.h"
#include "llglcommonfunc.h"
#include "llvoavatar.h"
#include "gltfscenemanager.h"

#include "llenvironment.h"

//MK
#include "llagent.h"
#include "llvovolume.h"
//mk

bool LLDrawPoolAlpha::sShowDebugAlpha = false;

#define current_shader (LLGLSLShader::sCurBoundShaderPtr)

LLVector4 LLDrawPoolAlpha::sWaterPlane;

// minimum alpha before discarding a fragment
static const F32 MINIMUM_ALPHA = 0.004f; // ~ 1/255

// minimum alpha before discarding a fragment when rendering impostors
static const F32 MINIMUM_IMPOSTOR_ALPHA = 0.1f;

static bool is_particle_or_hud_particle_group(LLSpatialGroup* group)
{
    const U32 partition_type = group->getSpatialPartition()->mPartitionType;
    return partition_type == LLViewerRegion::PARTITION_PARTICLE ||
           partition_type == LLViewerRegion::PARTITION_HUD_PARTICLE;
}

static bool is_renderable_alpha_group(LLSpatialGroup* group)
{
    return group->getSpatialPartition()->mRenderByGroup &&
           !group->isDead();
}

static bool is_alpha_group_on_rendered_side_of_water(LLSpatialGroup* group,
                                                     bool above_water,
                                                     F32 water_height)
{
    if (LLPipeline::sRenderingHUDs)
    {
        return true;
    }

    LLSpatialBridge* bridge = group->getSpatialPartition()->asBridge();
    const LLVector4a* ext = bridge ? bridge->getSpatialExtents() : group->getExtents();

    if (above_water)
    { // reject any spatial groups that have no part above water
        return ext[1].getF32ptr()[2] >= water_height;
    }

    // reject any spatial groups that have no part below water
    return ext[0].getF32ptr()[2] <= water_height;
}

static LLSpatialGroup::drawmap_elem_t& get_alpha_draw_info(LLSpatialGroup* group, bool rigged)
{
    return rigged ? group->mDrawMap[LLRenderPass::PASS_ALPHA_RIGGED] :
                    group->mDrawMap[LLRenderPass::PASS_ALPHA];
}

static LLCullResult::sg_iterator begin_alpha_groups(bool rigged)
{
    return rigged ? gPipeline.beginRiggedAlphaGroups() :
                    gPipeline.beginAlphaGroups();
}

static LLCullResult::sg_iterator end_alpha_groups(bool rigged)
{
    return rigged ? gPipeline.endRiggedAlphaGroups() :
                    gPipeline.endAlphaGroups();
}

static bool is_alpha_draw_info_for_pass(const LLDrawInfo& params, bool rigged)
{
    return (bool)params.mAvatar == rigged;
}

static bool is_above_water_alpha_pool(U32 pool_type)
{
    const bool above_water = pool_type == LLDrawPool::POOL_ALPHA_POST_WATER;
    return LLPipeline::sUnderWaterRender ? !above_water : above_water;
}

static void finish_alpha_render(bool light_enabled)
{
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    LLVertexBuffer::unbind();

    if (!light_enabled)
    {
        gPipeline.enableLightsDynamic();
    }
}

static bool should_queue_alpha_emissive(U32 pool_type, const LLDrawInfo& params)
{
    return pool_type != LLDrawPool::POOL_ALPHA_PRE_WATER &&
           params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE);
}

static bool is_alpha_highlight_rigged_pass(S32 pass)
{
    return pass != 0;
}

static LLCullResult::sg_iterator begin_alpha_highlight_groups(bool rigged)
{
    return rigged ? gPipeline.beginRiggedAlphaGroups() :
                    gPipeline.beginAlphaGroups();
}

static LLCullResult::sg_iterator end_alpha_highlight_groups(bool rigged)
{
    return rigged ? gPipeline.endRiggedAlphaGroups() :
                    gPipeline.endAlphaGroups();
}

static LLSpatialGroup::drawmap_elem_t& get_alpha_highlight_draw_info(LLSpatialGroup* group,
                                                                     S32 pass)
{
    // Preserve the existing +pass mapping to use PASS_ALPHA_RIGGED on the second pass.
    return group->mDrawMap[LLRenderPass::PASS_ALPHA + pass];
}

static void push_static_alpha_highlight_mask_batches(LLRenderPass& render_pass)
{
    const U32 passes[] =
    {
        LLRenderPass::PASS_ALPHA_MASK,
        LLRenderPass::PASS_ALPHA_INVISIBLE
    };

    for (U32 pass : passes)
    {
        render_pass.pushUntexturedBatches(pass);
    }
}

static void push_static_material_alpha_highlight_batches(LLRenderPass& render_pass)
{
    const U32 passes[] =
    {
        LLRenderPass::PASS_MATERIAL_ALPHA_MASK,
        LLRenderPass::PASS_NORMMAP_MASK,
        LLRenderPass::PASS_SPECMAP_MASK,
        LLRenderPass::PASS_NORMSPEC_MASK,
        LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK,
        LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK
    };

    for (U32 pass : passes)
    {
        render_pass.pushUntexturedBatches(pass);
    }
}

static void push_rigged_alpha_highlight_mask_batches(LLRenderPass& render_pass)
{
    const U32 passes[] =
    {
        LLRenderPass::PASS_ALPHA_MASK_RIGGED,
        LLRenderPass::PASS_ALPHA_INVISIBLE_RIGGED
    };

    for (U32 pass : passes)
    {
        render_pass.pushRiggedBatches(pass, false);
    }
}

static void push_rigged_material_alpha_highlight_batches(LLRenderPass& render_pass)
{
    const U32 passes[] =
    {
        LLRenderPass::PASS_MATERIAL_ALPHA_MASK_RIGGED,
        LLRenderPass::PASS_NORMMAP_MASK_RIGGED,
        LLRenderPass::PASS_SPECMAP_MASK_RIGGED,
        LLRenderPass::PASS_NORMSPEC_MASK_RIGGED,
        LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED,
        LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK_RIGGED
    };

    for (U32 pass : passes)
    {
        render_pass.pushRiggedBatches(pass, false);
    }
}

static bool draw_alpha_highlight_info(LLDrawInfo& params,
                                      const LLVOAvatar*& last_avatar,
                                      U64& last_mesh_id,
                                      bool& skip_last_skin)
{
    const bool rigged = params.mAvatar != nullptr;
    gHighlightProgram.bind(rigged);

    if (rigged &&
        !LLRenderPass::uploadMatrixPalette(params.mAvatar, params.mSkinInfo, last_avatar, last_mesh_id, skip_last_skin))
    { // failed to upload matrix palette, skip rendering
        return false;
    }

    gGL.diffuseColor4f(1, 0, 0, 1);
    LLRenderPass::applyModelMatrix(params);
    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
    return true;
}

LLDrawPoolAlpha::LLDrawPoolAlpha(U32 type) :
        LLRenderPass(type), target_shader(NULL),
        mColorSFactor(LLRender::BF_UNDEF), mColorDFactor(LLRender::BF_UNDEF),
        mAlphaSFactor(LLRender::BF_UNDEF), mAlphaDFactor(LLRender::BF_UNDEF)
{

}

LLDrawPoolAlpha::~LLDrawPoolAlpha()
{
}

void LLDrawPoolAlpha::AlphaEmissiveQueues::clear()
{
    emissives.resize(0);
    rigged_emissives.resize(0);
    pbr_emissives.resize(0);
    pbr_rigged_emissives.resize(0);
}

void LLDrawPoolAlpha::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

S32 LLDrawPoolAlpha::getNumPostDeferredPasses()
{
    return 1;
}

// set some common parameters on the given shader to prepare for alpha rendering
static void prepare_alpha_shader(LLGLSLShader* shader, bool deferredEnvironment, F32 water_sign)
{
    static LLCachedControl<F32> displayGamma(gSavedSettings, "RenderDeferredDisplayGamma");
    F32 gamma = displayGamma;

    static LLStaticHashedString waterSign("waterSign");

    // Does this deferred shader need environment uniforms set such as sun_dir, etc. ?
    // NOTE: We don't actually need a gbuffer since we are doing forward rendering (for transparency) post deferred rendering
    // TODO: bindDeferredShader() probably should have the updating of the environment uniforms factored out into updateShaderEnvironmentUniforms()
    // i.e. shaders\class1\deferred\alphaF.glsl
    if (deferredEnvironment)
    {
        shader->mCanBindFast = false;
    }

    shader->bind();
    shader->uniform1f(LLShaderMgr::DISPLAY_GAMMA, (gamma > 0.1f) ? 1.0f / gamma : (1.0f / 2.2f));

    if (LLPipeline::sRenderingHUDs)
    { // for HUD attachments, only the pre-water pass is executed and we never want to clip anything
        LLVector4 near_clip(0, 0, -1, 0);
        shader->uniform1f(waterSign, 1.f);
        shader->uniform4fv(LLShaderMgr::WATER_WATERPLANE, 1, near_clip.mV);
    }
    else
    {
        shader->uniform1f(waterSign, water_sign);
        shader->uniform4fv(LLShaderMgr::WATER_WATERPLANE, 1, LLDrawPoolAlpha::sWaterPlane.mV);
    }

    if (LLPipeline::sImpostorRender)
    {
        shader->setMinimumAlpha(MINIMUM_IMPOSTOR_ALPHA);
    }
    else
    {
        shader->setMinimumAlpha(MINIMUM_ALPHA);
    }

    //also prepare rigged variant
    if (shader->mRiggedVariant && shader->mRiggedVariant != shader)
    {
        prepare_alpha_shader(shader->mRiggedVariant, deferredEnvironment, water_sign);
    }
}

extern bool gCubeSnapshot;

static F32 get_alpha_water_sign(U32 pool_type)
{
    F32 water_sign = 1.f;

    if (pool_type == LLDrawPool::POOL_ALPHA_PRE_WATER)
    {
        water_sign = -1.f;
    }

    if (LLPipeline::sUnderWaterRender)
    {
        water_sign *= -1.f;
    }

    return water_sign;
}

static bool should_render_alpha_depth_of_field_pass(U32 pool_type)
{
    return !LLPipeline::sImpostorRender &&
           LLPipeline::RenderDepthOfField &&
           !gCubeSnapshot &&
           !LLPipeline::sRenderingHUDs &&
           pool_type == LLDrawPool::POOL_ALPHA_POST_WATER;
}

static bool should_write_alpha_depth(bool rigged, U32 pool_type)
{
    // Depth is needed so rendered alpha can contribute to the impostor alpha mask.
    const bool needs_depth_for_alpha_mask =
        LLDrawPoolWater::sSkipScreenCopy ||
        LLPipeline::sImpostorRenderAlphaDepthPass;

    const bool needs_depth_for_water_fog =
        pool_type == LLDrawPoolAlpha::POOL_ALPHA_PRE_WATER;

    return rigged || needs_depth_for_alpha_mask || needs_depth_for_water_fog;
}

static void render_gltf_scene_depth_for_rigged_alpha()
{
    LL::GLTFSceneManager::instance().render(false, false);
    LL::GLTFSceneManager::instance().render(false, true);
    LL::GLTFSceneManager::instance().render(false, false, true);
    LL::GLTFSceneManager::instance().render(false, true, true);
}

static LLGLSLShader* get_gltf_alpha_shader(LLGLSLShader* pbr_shader, const LLDrawInfo& params)
{
    LLGLSLShader* shader = pbr_shader;
    if (params.mAvatar != nullptr)
    {
        shader = shader->mRiggedVariant;
    }
    return shader;
}

struct NonGltfAlphaUniforms
{
    LLVector4 spec_color = LLVector4(1, 1, 1, 1);
    F32 env_intensity = 0.0f;
    F32 brightness = 1.0f;
};

static LLMaterial* get_non_gltf_alpha_material(LLDrawInfo& params)
{
    return LLPipeline::sRenderingHUDs ? nullptr : params.mMaterial.get();
}

static void update_non_gltf_alpha_lighting_state(
    const LLDrawInfo& params,
    bool& initialized_lighting,
    bool& light_enabled)
{
    if (params.mFullbright)
    {
        // Turn off lighting if it hasn't already been so.
        if (light_enabled || !initialized_lighting)
        {
            initialized_lighting = true;
            light_enabled = false;
        }
    }
    // Turn on lighting if it isn't already.
    else if (!light_enabled || !initialized_lighting)
    {
        initialized_lighting = true;
        light_enabled = true;
    }
}

static LLGLSLShader* get_non_gltf_alpha_base_shader(
    const LLDrawInfo& params,
    LLMaterial* mat,
    LLGLSLShader* simple_shader,
    LLGLSLShader* fullbright_shader)
{
    if (LLPipeline::sRenderingHUDs)
    {
        return fullbright_shader;
    }

    if (mat)
    {
        U32 mask = params.mShaderMask;

        llassert(mask < LLMaterial::SHADER_COUNT);
        return &(gDeferredMaterialProgram[mask]);
    }

    if (!params.mFullbright)
    {
        return simple_shader;
    }

    return fullbright_shader;
}

static LLGLSLShader* get_rigged_alpha_shader(LLGLSLShader* shader, const LLDrawInfo& params)
{
    if (params.mAvatar != nullptr)
    {
        llassert(shader->mRiggedVariant != nullptr);
        shader = shader->mRiggedVariant;
    }
    return shader;
}

static LLGLSLShader* get_non_gltf_alpha_shader(
    const LLDrawInfo& params,
    LLMaterial* mat,
    LLGLSLShader* simple_shader,
    LLGLSLShader* fullbright_shader)
{
    LLGLSLShader* shader = get_non_gltf_alpha_base_shader(
        params,
        mat,
        simple_shader,
        fullbright_shader);
    return get_rigged_alpha_shader(shader, params);
}

static void bind_alpha_shader_if_needed(LLGLSLShader* target_shader, bool bind_exposure_map)
{
    if (current_shader == target_shader)
    {
        return;
    }

    gPipeline.bindDeferredShaderFast(*target_shader);

    if (bind_exposure_map)
    { // make sure the bind the exposure map for fullbright shaders so they can cancel out exposure
        S32 channel = target_shader->enableTexture(LLShaderMgr::EXPOSURE_MAP);
        if (channel > -1)
        {
            gGL.getTexUnit(channel)->bind(&gPipeline.mExposureMap);
        }
    }
}

static NonGltfAlphaUniforms get_non_gltf_alpha_uniforms(const LLDrawInfo& params, LLMaterial* mat)
{
    NonGltfAlphaUniforms uniforms;

    // We have a material. Supply the appropriate data here.
    if (mat)
    {
        uniforms.spec_color = params.mSpecColor;
        uniforms.env_intensity = params.mEnvIntensity;
        uniforms.brightness = params.mFullbright ? 1.f : 0.f;
    }

    return uniforms;
}

static void set_non_gltf_alpha_uniforms(const NonGltfAlphaUniforms& uniforms)
{
    if (current_shader)
    {
        current_shader->uniform4f(
            LLShaderMgr::SPECULAR_COLOR,
            uniforms.spec_color.mV[VRED],
            uniforms.spec_color.mV[VGREEN],
            uniforms.spec_color.mV[VBLUE],
            uniforms.spec_color.mV[VALPHA]);
        current_shader->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, uniforms.env_intensity);
        current_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, uniforms.brightness);
    }
}

static void apply_alpha_draw_blend_state(
    const LLDrawInfo& params,
    LLRender::eBlendFactor alpha_s_factor,
    LLRender::eBlendFactor alpha_d_factor)
{
    gGL.blendFunc(
        (LLRender::eBlendFactor) params.mBlendFuncSrc,
        (LLRender::eBlendFactor) params.mBlendFuncDst,
        alpha_s_factor,
        alpha_d_factor);
}

static bool should_lower_minimum_alpha_for_draw(const LLDrawInfo& params)
{
    // Custom blend modes may require rendering fragments below the normal alpha cutoff.
    return !LLPipeline::sImpostorRender &&
           params.mBlendFuncDst != LLRender::BF_SOURCE_ALPHA &&
           params.mBlendFuncSrc != LLRender::BF_SOURCE_ALPHA;
}

static void lower_minimum_alpha_for_draw()
{
    current_shader->setMinimumAlpha(0.f);
}

static void restore_minimum_alpha_after_draw()
{
    current_shader->setMinimumAlpha(MINIMUM_ALPHA);
}

static void draw_alpha_batch(LLDrawInfo& params)
{
    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(
        LLRender::TRIANGLES,
        params.mStart,
        params.mEnd,
        params.mCount,
        params.mOffset);
    stop_glerror();
}

static void render_alpha_batch(
    LLDrawInfo& params,
    LLRender::eBlendFactor alpha_s_factor,
    LLRender::eBlendFactor alpha_d_factor)
{
    apply_alpha_draw_blend_state(params, alpha_s_factor, alpha_d_factor);

    const bool reset_minimum_alpha = should_lower_minimum_alpha_for_draw(params);
    if (reset_minimum_alpha)
    {
        lower_minimum_alpha_for_draw();
    }

    draw_alpha_batch(params);

    if (reset_minimum_alpha)
    {
        restore_minimum_alpha_after_draw();
    }
}

static U32 get_alpha_vertex_data_mask()
{
    return (U32)LLDrawPoolAlpha::VERTEX_DATA_MASK |
           LLVertexBuffer::MAP_TEXTURE_INDEX |
           LLVertexBuffer::MAP_TANGENT |
           LLVertexBuffer::MAP_TEXCOORD1 |
           LLVertexBuffer::MAP_TEXCOORD2;
}

void LLDrawPoolAlpha::prepareDeferredAlphaShaders(F32 water_sign)
{
    emissive_shader = &gDeferredEmissiveProgram;
    prepare_alpha_shader(emissive_shader, false, water_sign);

    pbr_emissive_shader = &gPBRGlowProgram;
    prepare_alpha_shader(pbr_emissive_shader, false, water_sign);

    fullbright_shader   =
        (LLPipeline::sImpostorRender) ? &gDeferredFullbrightAlphaMaskProgram :
        (LLPipeline::sRenderingHUDs) ? &gHUDFullbrightAlphaMaskAlphaProgram :
        &gDeferredFullbrightAlphaMaskAlphaProgram;
    prepare_alpha_shader(fullbright_shader, true, water_sign);

    simple_shader   =
        (LLPipeline::sImpostorRender) ? &gDeferredAlphaImpostorProgram :
        (LLPipeline::sRenderingHUDs) ? &gHUDAlphaProgram :
        &gDeferredAlphaProgram;

    prepare_alpha_shader(simple_shader, true, water_sign); //prime simple shader (loads shadow relevant uniforms)

    LLGLSLShader* materialShader = gDeferredMaterialProgram;
    for (int i = 0; i < LLMaterial::SHADER_COUNT*2; ++i)
    {
        prepare_alpha_shader(&materialShader[i], true, water_sign);
    }

    pbr_shader =
        (LLPipeline::sRenderingHUDs) ? &gHUDPBRAlphaProgram :
        &gDeferredPBRAlphaProgram;

    prepare_alpha_shader(pbr_shader, true, water_sign);
}

void LLDrawPoolAlpha::renderDepthOfFieldAlphaPass()
{
    //update depth buffer sampler
    simple_shader = fullbright_shader = &gDeferredFullbrightAlphaMaskProgram;

    simple_shader->bind();
    simple_shader->setMinimumAlpha(0.33f);

    // mask off color buffer writes as we're only writing to depth buffer
    gGL.setColorMask(false, false);

    // If the face is more than 90% transparent, then don't update the Depth buffer for Dof
    // We don't want the nearly invisible objects to cause of DoF effects
    renderAlpha(get_alpha_vertex_data_mask(), true); // <--- discard mostly transparent faces

    gGL.setColorMask(true, false);
}

void LLDrawPoolAlpha::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    if (LLPipeline::isWaterClip() && getType() == LLDrawPool::POOL_ALPHA_PRE_WATER)
    { // don't render alpha objects on the other side of the water plane if water is opaque
        return;
    }

    F32 water_sign = get_alpha_water_sign(getType());

    // prepare shaders
    llassert(LLPipeline::sRenderDeferred);

    prepareDeferredAlphaShaders(water_sign);

    // explicitly unbind here so render loop doesn't make assumptions about the last shader
    // already being setup for rendering
    LLGLSLShader::unbind();

    if (!LLPipeline::sRenderingHUDs)
    {
        // first pass, render rigged objects only and render to depth buffer
        forwardRender(true);
    }

    // second pass, regular forward alpha rendering
    forwardRender();

    // final pass, render to depth for depth of field effects
    if (should_render_alpha_depth_of_field_pass(getType()))
    {
        renderDepthOfFieldAlphaPass();
    }
}

void LLDrawPoolAlpha::forwardRender(bool rigged)
{
    gPipeline.enableLightsDynamic();

    LLGLSPipelineAlpha gls_pipeline_alpha;

    //enable writing to alpha for emissive effects
    gGL.setColorMask(true, true);

    bool write_depth = should_write_alpha_depth(rigged, getType());
    LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE);

    mColorSFactor = LLRender::BF_SOURCE_ALPHA;           // } regular alpha blend
    mColorDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA; // }
    mAlphaSFactor = LLRender::BF_ZERO;                         // } glow suppression
    mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;       // }
    gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

    if (rigged && mType == LLDrawPool::POOL_ALPHA_POST_WATER)
    { // draw GLTF scene to depth buffer before rigged alpha
        render_gltf_scene_depth_for_rigged_alpha();
    }

    // If the face is more than 90% transparent, then don't update the Depth buffer for Dof
    // We don't want the nearly invisible objects to cause of DoF effects
    renderAlpha(get_alpha_vertex_data_mask(), false, rigged);

    gGL.setColorMask(true, false);

    if (!rigged)
    { //render "highlight alpha" on final non-rigged pass
        // NOTE -- hacky call here protected by !rigged instead of alongside "forwardRender"
        // so renderDebugAlpha is executed while gls_pipeline_alpha and depth GL state
        // variables above are still in scope
        renderDebugAlpha();
    }
}

void LLDrawPoolAlpha::renderDebugAlpha()
{
    if (sShowDebugAlpha && !gCubeSnapshot)
    {
        gHighlightProgram.bind();
        gGL.diffuseColor4f(1, 0, 0, 1);
        gGL.getTexUnit(0)->bindFast(LLViewerFetchedTexture::getSmokeImage());


        renderAlphaHighlight();

        push_static_alpha_highlight_mask_batches(*this);

        // Material alpha mask
        gGL.diffuseColor4f(0, 0, 1, 1);
        push_static_material_alpha_highlight_batches(*this);

        gGL.diffuseColor4f(0, 1, 0, 1);
        pushUntexturedBatches(LLRenderPass::PASS_INVISIBLE);

        gHighlightProgram.mRiggedVariant->bind();
        gGL.diffuseColor4f(1, 0, 0, 1);

        push_rigged_alpha_highlight_mask_batches(*this);

        // Material alpha mask
        gGL.diffuseColor4f(0, 0, 1, 1);
        push_rigged_material_alpha_highlight_batches(*this);

        gGL.diffuseColor4f(0, 1, 0, 1);
        pushRiggedBatches(LLRenderPass::PASS_INVISIBLE_RIGGED, false);
        LLGLSLShader::sCurBoundShaderPtr->unbind();
    }
}

void LLDrawPoolAlpha::renderAlphaHighlight()
{
    for (int pass = 0; pass < 2; ++pass)
    { //two passes, one rigged and one not
        const LLVOAvatar* lastAvatar = nullptr;
        U64 lastMeshId = 0;
        bool skipLastSkin = false;

        const bool rigged_pass = is_alpha_highlight_rigged_pass(pass);
        LLCullResult::sg_iterator begin = begin_alpha_highlight_groups(rigged_pass);
        LLCullResult::sg_iterator end = end_alpha_highlight_groups(rigged_pass);

        for (LLCullResult::sg_iterator i = begin; i != end; ++i)
        {
            LLSpatialGroup* group = *i;
            if (!is_renderable_alpha_group(group))
            {
                continue;
            }

            LLSpatialGroup::drawmap_elem_t& draw_info = get_alpha_highlight_draw_info(group, pass);

            for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
            {
                LLDrawInfo& params = **k;
                if (!draw_alpha_highlight_info(params, lastAvatar, lastMeshId, skipLastSkin))
                {
                    continue;
                }
            }
        }
    }

    // make sure static version of highlight shader is bound before returning
    gHighlightProgram.bind();
}

bool LLDrawPoolAlpha::SetupTextureMatrix(LLDrawInfo* draw)
{
    if (!draw->mTextureMatrix)
    {
        return false;
    }

    gGL.getTexUnit(0)->activate();
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadMatrix((GLfloat*)draw->mTextureMatrix->mMatrix);
    gPipeline.mTextureMatrixOps++;

    return true;
}

bool LLDrawPoolAlpha::SetupGltfTextures(LLDrawInfo* draw)
{
    return SetupTextureMatrix(draw);
}

void LLDrawPoolAlpha::BindLegacyMaterialAuxMaps(LLDrawInfo* draw, bool use_material)
{
    if (!LLPipeline::sRenderingHUDs && use_material && current_shader)
    {
        if (draw->mNormalMap)
        {
            current_shader->bindTexture(LLShaderMgr::BUMP_MAP, draw->mNormalMap);
        }

        if (draw->mSpecularMap)
        {
            current_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, draw->mSpecularMap);
        }
    }
    else if (current_shader == simple_shader || current_shader == simple_shader->mRiggedVariant)
    {
        current_shader->bindTexture(LLShaderMgr::BUMP_MAP, LLViewerFetchedTexture::sFlatNormalImagep);
        current_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, LLViewerFetchedTexture::sWhiteImagep);
    }
}

void LLDrawPoolAlpha::BindLegacyTextureList(LLDrawInfo* draw)
{
    for (U32 i = 0; i < draw->mTextureList.size(); ++i)
    {
        if (draw->mTextureList[i].notNull())
        {
            gGL.getTexUnit(i)->bindFast(draw->mTextureList[i]);
        }
    }
}

bool LLDrawPoolAlpha::BindLegacySingleTexture(LLDrawInfo* draw, bool use_material)
{
    if (draw->mTexture.notNull())
    {
        if (use_material)
        {
            current_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, draw->mTexture);
        }
        else
        {
            gGL.getTexUnit(0)->bindFast(draw->mTexture);
        }

        return SetupTextureMatrix(draw);
    }

    gGL.getTexUnit(0)->unbindFast(LLTexUnit::TT_TEXTURE);
    return false;
}

bool LLDrawPoolAlpha::SetupLegacyTextures(LLDrawInfo* draw, bool use_material)
{
    BindLegacyMaterialAuxMaps(draw, use_material);

    if (draw->mTextureList.size() > 1)
    {
        BindLegacyTextureList(draw);
        return false;
    }

    // Not batching textures or batch has only 1 texture: might need a texture matrix.
    return BindLegacySingleTexture(draw, use_material);
}

bool LLDrawPoolAlpha::TexSetup(LLDrawInfo* draw, bool use_material)
{
    if (draw->mGLTFMaterial)
    {
        return SetupGltfTextures(draw);
    }

    return SetupLegacyTextures(draw, use_material);
}

void LLDrawPoolAlpha::RestoreTexSetup(bool tex_setup)
{
    if (tex_setup)
    {
        gGL.getTexUnit(0)->activate();
        gGL.matrixMode(LLRender::MM_TEXTURE);
        gGL.loadIdentity();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
    }
}

void LLDrawPoolAlpha::drawEmissive(LLDrawInfo* draw)
{
    LLGLSLShader::sCurBoundShaderPtr->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);
    draw->mVertexBuffer->setBuffer();
    draw->mVertexBuffer->drawRange(LLRender::TRIANGLES, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
}

void LLDrawPoolAlpha::renderLegacyEmissiveDraw(LLDrawInfo* draw)
{
    bool tex_setup = TexSetup(draw, false);
    drawEmissive(draw);
    RestoreTexSetup(tex_setup);
}

void LLDrawPoolAlpha::renderPbrEmissiveDraw(LLDrawInfo* draw)
{
    llassert(draw->mGLTFMaterial);
    LLGLDisable cull_face(draw->mGLTFMaterial->mDoubleSided ? GL_CULL_FACE : 0);
    draw->mGLTFMaterial->bind(draw->mTexture);
    draw->mVertexBuffer->setBuffer();
    draw->mVertexBuffer->drawRange(LLRender::TRIANGLES, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
}

void LLDrawPoolAlpha::renderEmissives(std::vector<LLDrawInfo*>& emissives)
{
    emissive_shader->bind();
    emissive_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);

    for (LLDrawInfo* draw : emissives)
    {
        renderLegacyEmissiveDraw(draw);
    }
}

void LLDrawPoolAlpha::renderPbrEmissives(std::vector<LLDrawInfo*>& emissives)
{
    pbr_emissive_shader->bind();

    for (LLDrawInfo* draw : emissives)
    {
        renderPbrEmissiveDraw(draw);
    }
}

void LLDrawPoolAlpha::renderRiggedEmissives(std::vector<LLDrawInfo*>& emissives)
{
    LLGLDepthTest depth(GL_TRUE, GL_FALSE); //disable depth writes since "emissive" is additive so sorting doesn't matter
    LLGLSLShader* shader = emissive_shader->mRiggedVariant;
    shader->bind();
    shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (LLDrawInfo* draw : emissives)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("Emissives");

        if (uploadMatrixPalette(draw->mAvatar, draw->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        {
            renderLegacyEmissiveDraw(draw);
        }
    }
}

void LLDrawPoolAlpha::renderRiggedPbrEmissives(std::vector<LLDrawInfo*>& emissives)
{
    LLGLDepthTest depth(GL_TRUE, GL_FALSE); //disable depth writes since "emissive" is additive so sorting doesn't matter
    pbr_emissive_shader->bind(true);

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (LLDrawInfo* draw : emissives)
    {
        if (!uploadMatrixPalette(draw->mAvatar, draw->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        { // failed to upload matrix palette, skip rendering
            continue;
        }

        renderPbrEmissiveDraw(draw);
    }
}

void LLDrawPoolAlpha::queueAlphaEmissive(LLDrawInfo& params, AlphaEmissiveQueues& queues)
{
    if (params.mAvatar != nullptr)
    {
        if (params.mGLTFMaterial.isNull())
        {
            queues.rigged_emissives.push_back(&params);
        }
        else
        {
            queues.pbr_rigged_emissives.push_back(&params);
        }
    }
    else
    {
        if (params.mGLTFMaterial.isNull())
        {
            queues.emissives.push_back(&params);
        }
        else
        {
            queues.pbr_emissives.push_back(&params);
        }
    }
}

void LLDrawPoolAlpha::renderAlphaEmissiveSubpass(AlphaEmissiveQueues& queues, bool& light_enabled)
{
    gPipeline.enableLightsDynamic();

    // install glow-accumulating blend mode
    // don't touch color, add to alpha (glow)
    gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE);

    bool rebind = false;
    LLGLSLShader* lastShader = current_shader;
    if (!queues.emissives.empty())
    {
        light_enabled = true;
        renderEmissives(queues.emissives);
        rebind = true;
    }

    if (!queues.pbr_emissives.empty())
    {
        light_enabled = true;
        renderPbrEmissives(queues.pbr_emissives);
        rebind = true;
    }

    if (!queues.rigged_emissives.empty())
    {
        light_enabled = true;
        renderRiggedEmissives(queues.rigged_emissives);
        rebind = true;
    }

    if (!queues.pbr_rigged_emissives.empty())
    {
        light_enabled = true;
        renderRiggedPbrEmissives(queues.pbr_rigged_emissives);
        rebind = true;
    }

    // restore our alpha blend mode
    gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

    if (lastShader && rebind)
    {
        lastShader->bind();
    }
}

void LLDrawPoolAlpha::renderAlphaDraw(
    LLDrawInfo& params,
    const LLVOAvatar*& lastAvatar,
    U64& lastMeshId,
    const LLGLSLShader*& lastAvatarShader,
    bool& skipLastSkin,
    bool& initialized_lighting,
    bool& light_enabled,
    AlphaEmissiveQueues& queues)
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("ra - push batch");

    LLRenderPass::applyModelMatrix(params);

    LLMaterial* mat = NULL;
    LLGLTFMaterial *gltf_mat = params.mGLTFMaterial;

    LLGLDisable cull_face(gltf_mat && gltf_mat->mDoubleSided ? GL_CULL_FACE : 0);

    if (gltf_mat && gltf_mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_BLEND)
    {
        target_shader = get_gltf_alpha_shader(pbr_shader, params);

        // shader must be bound before LLGLTFMaterial::bind
        if (current_shader != target_shader)
        {
            gPipeline.bindDeferredShaderFast(*target_shader);
        }

        params.mGLTFMaterial->bind(params.mTexture);
    }
    else
// RLV:PBR this code block looks to need relocating
/*
//MK
    LLFace* facep = params.mFace;
    if (facep)
    {
        LLDrawable* drawable = facep->getDrawable();
        if (drawable)
        {
            LLVOVolume* vovolume = drawable->getVOVolume();
            if (vovolume)
            {
                if (vision_restricted)
                {
                    // If we are under @camtextures, do not render this alpha surface if it is phantom and it is not an attachment
                    if (gAgent.mRRInterface.mContainsCamTextures && vovolume->flagPhantom() && !vovolume->isAttachment())
                    {
                        continue;
                    }

                    // Do not render any alpha surface (except on our HUDs) if the vision is restricted and
                    // the face is farther than the outer vision sphere.
                    LLVector3 face_pos = LLVector3::zero;
                    LLVector3 face_avatar_offset = LLVector3::zero;
                    F32 face_distance_to_avatar_squared = EXTREMUM;

                    if (!vovolume->isHUDAttachment())
                    {
                        face_pos = facep->getPositionAgent();
                        face_avatar_offset = face_pos - joint_pos;
                        face_distance_to_avatar_squared = (F32)face_avatar_offset.magVecSquared();
                        if (face_distance_to_avatar_squared > gAgent.mRRInterface.mLeastDistMaxSquared)
                        {
                            continue;
                        }
                    }
                }
            }
        }
    }
//mk
*/
    {
        mat = get_non_gltf_alpha_material(params);
        update_non_gltf_alpha_lighting_state(
            params,
            initialized_lighting,
            light_enabled);
        target_shader = get_non_gltf_alpha_shader(
            params,
            mat,
            simple_shader,
            fullbright_shader);
        bind_alpha_shader_if_needed(target_shader, params.mFullbright);
        set_non_gltf_alpha_uniforms(get_non_gltf_alpha_uniforms(params, mat));
    }

    if (params.mAvatar && !uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, lastAvatarShader, skipLastSkin))
    {
        return;
    }

    bool tex_setup = TexSetup(&params, (mat != nullptr));

    render_alpha_batch(params, mAlphaSFactor, mAlphaDFactor);

    // If this alpha mesh has glow, then draw it a second time to add the destination-alpha (=glow). Interleaving these state-changing calls is expensive, but glow must be drawn Z-sorted with alpha.
    if (should_queue_alpha_emissive(getType(), params))
    {
        queueAlphaEmissive(params, queues);
    }

    RestoreTexSetup(tex_setup);
}

void LLDrawPoolAlpha::renderAlpha(U32 mask, bool depth_only, bool rigged)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    bool initialized_lighting = false;
    bool light_enabled = true;

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    const LLGLSLShader* lastAvatarShader = nullptr;
    bool skipLastSkin = false;

    LLCullResult::sg_iterator begin = begin_alpha_groups(rigged);
    LLCullResult::sg_iterator end = end_alpha_groups(rigged);

    LLEnvironment& env = LLEnvironment::instance();
    F32 water_height = env.getWaterHeight();

    const bool above_water = is_above_water_alpha_pool(getType());


//MK
    // Calculate the position of the avatar here so we don't have to do it for each face
    bool vision_restricted = (gRRenabled && gAgent.mRRInterface.mVisionRestricted);
    // Optimization : Rather than compare the distances for every face (which involves square roots, which are costly), we compare squared distances.
    // KKA-835 Further optimisation - the least square is precomputed
    LLVector3 joint_pos = LLVector3::zero;
    // We don't need to calculate all that stuff if the vision is not restricted.
    if (vision_restricted && isAgentAvatarValid()) // KKA-862 avoid similar crash to the one observed in llvoavatar (isTooComplex)
    {
        joint_pos = gAgent.mRRInterface.getCamDistDrawFromJoint()->getWorldPosition();
    }
//mk

    for (LLCullResult::sg_iterator i = begin; i != end; ++i)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("renderAlpha - group");
        LLSpatialGroup* group = *i;
        llassert(group);
        llassert(group->getSpatialPartition());

        if (is_renderable_alpha_group(group))
        {
            if (!is_alpha_group_on_rendered_side_of_water(group, above_water, water_height))
            {
                continue;
            }

            static AlphaEmissiveQueues emissive_queues;
            emissive_queues.clear();

            const bool disable_cull = is_particle_or_hud_particle_group(group);
            LLGLDisable cull(disable_cull ? GL_CULL_FACE : 0);

            LLSpatialGroup::drawmap_elem_t& draw_info = get_alpha_draw_info(group, rigged);

            for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
            {
                LLDrawInfo& params = **k;
                if (!is_alpha_draw_info_for_pass(params, rigged))
                {
                    continue;
                }

                renderAlphaDraw(params,
                                lastAvatar,
                                lastMeshId,
                                lastAvatarShader,
                                skipLastSkin,
                                initialized_lighting,
                                light_enabled,
                                emissive_queues);
            }

            // render emissive faces into alpha channel for bloom effects
            if (!depth_only)
            {
                renderAlphaEmissiveSubpass(emissive_queues, light_enabled);
            }
        }
    }

    finish_alpha_render(light_enabled);
}
