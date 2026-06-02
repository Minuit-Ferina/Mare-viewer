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
#include "llfetchedgltfmaterial.h"
#include "llenvironment.h"
#include "llmaterial.h"
#include "m4math.h"
#include "llrenderbackend.h"
#include "llrenderstate.h"
#include "llsettingssky.h"
#include "llspatialpartition.h"
#include "llstring.h"
#include "llvoavatar.h"
#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "pipeline.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <string>

namespace
{
constexpr F32 WORLD_RENDER_MINIMUM_ALPHA = 0.004f;
constexpr F32 AVATAR_RENDER_MINIMUM_ALPHA = 0.2f;
constexpr F32 WORLD_RENDER_SHADOW_ALPHA_BLEND_CUTOFF = 0.598f;
constexpr U32 WORLD_RENDER_SCENE_DEPTH_TEXTURE_UNIT = 8;
constexpr U32 WORLD_RENDER_SCENE_COLOR_TEXTURE_UNIT = 9;

LLRenderWorldTextureTransform get_world_texture_transform(const LLMatrix4* matrix);

void classify_world_render_command(LLWorldRenderCommand& command)
{
    const LLWorldRenderPipelineContract contract =
        ::get_world_render_pipeline_contract(command.mMaterialClass);
    command.mPassClass = contract.mPassClass;
    command.mBlendMode = contract.mBlendMode;
    command.mDepthMode = contract.mDepthMode;
    command.mCullMode = contract.mCullMode;
    command.mPolygonOffsetEnabled = contract.mPolygonOffsetEnabled;
    command.mPolygonOffsetFactor = contract.mPolygonOffsetFactor;
    command.mPolygonOffsetUnits = contract.mPolygonOffsetUnits;
    command.mWriteColor = contract.mWriteColor;
    command.mWriteAlpha = contract.mWriteAlpha;
}

const char* get_world_render_material_class_log_name(LLWorldRenderMaterialClass material_class)
{
    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Sky: return "Sky";
        case LLWorldRenderMaterialClass::Terrain: return "Terrain";
        case LLWorldRenderMaterialClass::SimpleOpaque: return "SimpleOpaque";
        case LLWorldRenderMaterialClass::AlphaMask: return "AlphaMask";
        case LLWorldRenderMaterialClass::Grass: return "Grass";
        case LLWorldRenderMaterialClass::Tree: return "Tree";
        case LLWorldRenderMaterialClass::Fullbright: return "Fullbright";
        case LLWorldRenderMaterialClass::FullbrightAlphaMask: return "FullbrightAlphaMask";
        case LLWorldRenderMaterialClass::Bump: return "Bump";
        case LLWorldRenderMaterialClass::LegacyMaterial: return "LegacyMaterial";
        case LLWorldRenderMaterialClass::GLTFPBR: return "GLTFPBR";
        case LLWorldRenderMaterialClass::GLTFPBRAlphaMask: return "GLTFPBRAlphaMask";
        case LLWorldRenderMaterialClass::Avatar: return "Avatar";
        case LLWorldRenderMaterialClass::AvatarImpostor: return "AvatarImpostor";
        case LLWorldRenderMaterialClass::Alpha: return "Alpha";
        case LLWorldRenderMaterialClass::Glow: return "Glow";
        case LLWorldRenderMaterialClass::Water: return "Water";
        case LLWorldRenderMaterialClass::WaterExclusionMask: return "WaterExclusionMask";
        case LLWorldRenderMaterialClass::WaterExclusionSurface: return "WaterExclusionSurface";
        case LLWorldRenderMaterialClass::AtmosphericHaze: return "AtmosphericHaze";
        case LLWorldRenderMaterialClass::WaterHaze: return "WaterHaze";
        case LLWorldRenderMaterialClass::FullbrightShiny: return "FullbrightShiny";
        case LLWorldRenderMaterialClass::PostBump: return "PostBump";
        case LLWorldRenderMaterialClass::Shadow: return "Shadow";
        case LLWorldRenderMaterialClass::ShadowAlphaMask: return "ShadowAlphaMask";
        case LLWorldRenderMaterialClass::AvatarShadow: return "AvatarShadow";
        case LLWorldRenderMaterialClass::AvatarAlphaShadow: return "AvatarAlphaShadow";
        case LLWorldRenderMaterialClass::AvatarAlphaMaskShadow: return "AvatarAlphaMaskShadow";
        case LLWorldRenderMaterialClass::TreeShadow: return "TreeShadow";
        case LLWorldRenderMaterialClass::PBRAlphaMaskShadow: return "PBRAlphaMaskShadow";
        case LLWorldRenderMaterialClass::PBRAlphaBlendShadow: return "PBRAlphaBlendShadow";
    }
    return "Unknown";
}

void log_vulkan_world_command_summary(const LLWorldRenderCommandBuffer& command_buffer)
{
    static U32 sLoggedCommandBuffers = 0;
    if (sLoggedCommandBuffers >= 12)
    {
        return;
    }

    constexpr U32 material_class_count =
        static_cast<U32>(LLWorldRenderMaterialClass::PBRAlphaBlendShadow) + 1;
    U32 material_counts[material_class_count] = {};
    U32 deferred_count = 0;
    U32 post_deferred_count = 0;
    U32 valid_draw_count = 0;
    U32 textured_draw_count = 0;
    U32 batched_texture_draw_count = 0;
    U32 skinned_draw_count = 0;
    U32 indexed_triangle_count = 0;
    U32 array_draw_count = 0;
    U32 total_indices = 0;

    for (const LLWorldRenderCommand& command : command_buffer.commands())
    {
        const U32 material_index = static_cast<U32>(command.mMaterialClass);
        if (material_index < material_class_count)
        {
            ++material_counts[material_index];
        }

        if (command.mPassClass == LLWorldRenderPassClass::Deferred)
        {
            ++deferred_count;
        }
        else
        {
            ++post_deferred_count;
        }

        if (command.mVertexBuffer && command.mCount)
        {
            ++valid_draw_count;
            total_indices += command.mCount;
            if (command.mTexture)
            {
                ++textured_draw_count;
            }
            if (command.mBatchTextures)
            {
                ++batched_texture_draw_count;
            }
            if (command.mSkinningMatrixCount > 0)
            {
                ++skinned_draw_count;
            }
            if (command.mOffset || command.mEnd)
            {
                ++indexed_triangle_count;
            }
            else
            {
                ++array_draw_count;
            }
        }
    }

    LL_INFOS("RenderBackend")
        << "Vulkan world command buffer "
        << (sLoggedCommandBuffers + 1)
        << ": commands "
        << command_buffer.commands().size()
        << ", valid draws "
        << valid_draw_count
        << ", deferred "
        << deferred_count
        << ", post-deferred "
        << post_deferred_count
        << ", textured "
        << textured_draw_count
        << ", batched textures "
        << batched_texture_draw_count
        << ", skinned "
        << skinned_draw_count
        << ", indexed-like "
        << indexed_triangle_count
        << ", array-like "
        << array_draw_count
        << ", total count "
        << total_indices
        << "."
        << LL_ENDL;

    for (U32 i = 0; i < material_class_count; ++i)
    {
        if (material_counts[i] == 0)
        {
            continue;
        }

        LL_INFOS("RenderBackend")
            << "Vulkan world command buffer "
            << sLoggedCommandBuffers + 1
            << " material "
            << get_world_render_material_class_log_name(static_cast<LLWorldRenderMaterialClass>(i))
            << ": "
            << material_counts[i]
            << " draw command(s)."
            << LL_ENDL;
    }

    ++sLoggedCommandBuffers;
}

U32 get_vulkan_world_command_capture_u32(
    const char* name,
    U32 default_value,
    U32 maximum_value)
{
    const char* value = std::getenv(name);
    if (!value || !value[0])
    {
        return default_value;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (*end != '\0' || parsed > maximum_value)
    {
        return default_value;
    }
    return static_cast<U32>(parsed);
}

bool get_vulkan_world_command_boolean_env(const char* name)
{
    const char* value = std::getenv(name);
    if (!value || !value[0])
    {
        return false;
    }

    std::string text(value);
    LLStringUtil::toLower(text);
    return text == "1" || text == "true" || text == "yes" || text == "on";
}

bool is_vulkan_world_command_capture_trigger_ready()
{
    const char* trigger_path =
        std::getenv("MARE_VULKAN_WORLD_COMMAND_CAPTURE_TRIGGER");
    if (!trigger_path || !trigger_path[0])
    {
        return true;
    }

    std::ifstream input(trigger_path);
    return input.good();
}

S32 get_vulkan_world_command_capture_raw_level(const LLViewerTexture* texture)
{
    if (!texture || texture->getType() != LLViewerTexture::FETCHED_TEXTURE)
    {
        return -1;
    }

    return static_cast<const LLViewerFetchedTexture*>(texture)->getRawImageLevel();
}

void write_vulkan_world_command_capture_texture(
    std::ostream& output,
    const char* name,
    const LLViewerTexture* texture)
{
    output
        << " " << name << " "
        << (texture ? 1 : 0) << " "
        << (texture ? texture->getTexName() : 0) << " "
        << (texture ? texture->getWidth() : 0) << " "
        << (texture ? texture->getHeight() : 0) << " "
        << (texture ? texture->getFullWidth() : 0) << " "
        << (texture ? texture->getFullHeight() : 0) << " "
        << (texture ? texture->getDiscardLevel() : -1) << " "
        << get_vulkan_world_command_capture_raw_level(texture);
}

void write_vulkan_world_command_capture_material_transform(
    std::ostream& output,
    const char* name,
    const LLWorldRenderTextureTransform2D& transform)
{
    output
        << " " << name << " "
        << (transform.mValid ? 1 : 0) << " "
        << transform.mScaleS << " "
        << transform.mScaleT << " "
        << transform.mRotation << " "
        << transform.mOffsetS << " "
        << transform.mOffsetT;
}

void write_vulkan_world_command_capture_texture_matrix(
    std::ostream& output,
    const LLMatrix4* matrix)
{
    const LLRenderWorldTextureTransform transform =
        get_world_texture_transform(matrix);
    output
        << " texture_matrix "
        << (matrix ? 1 : 0) << " "
        << transform.mS[0] << " "
        << transform.mS[1] << " "
        << transform.mS[2] << " "
        << transform.mS[3] << " "
        << transform.mT[0] << " "
        << transform.mT[1] << " "
        << transform.mT[2] << " "
        << transform.mT[3];
}

U32 get_vulkan_world_command_capture_texture_list_count(
    const LLWorldRenderCommand& command)
{
    U32 count = 0;
    for (const LLPointer<LLViewerTexture>& texture : command.mTextureList)
    {
        if (texture.notNull())
        {
            ++count;
        }
    }
    return count;
}

void write_vulkan_world_command_capture(const LLWorldRenderCommandBuffer& command_buffer)
{
    const char* path = std::getenv("MARE_VULKAN_WORLD_COMMAND_CAPTURE");
    if (!path || !path[0])
    {
        return;
    }

    static U32 sCapturedBuffers = 0;
    static U32 sEligibleBuffers = 0;
    static bool sLoggedWaitingForTrigger = false;
    static bool sLoggedWaitingForMinimumCommands = false;
    static bool sLoggedCaptureStarted = false;

    const U32 capture_buffer_limit =
        get_vulkan_world_command_capture_u32(
            "MARE_VULKAN_WORLD_COMMAND_CAPTURE_BUFFERS",
            4,
            1024);
    if (sCapturedBuffers >= capture_buffer_limit)
    {
        return;
    }

    if (!is_vulkan_world_command_capture_trigger_ready())
    {
        if (!sLoggedWaitingForTrigger)
        {
            LL_INFOS("RenderBackend")
                << "Vulkan world command capture is armed and waiting for trigger file "
                << std::getenv("MARE_VULKAN_WORLD_COMMAND_CAPTURE_TRIGGER")
                << "."
                << LL_ENDL;
            sLoggedWaitingForTrigger = true;
        }
        return;
    }

    const U32 minimum_command_count =
        get_vulkan_world_command_capture_u32(
            "MARE_VULKAN_WORLD_COMMAND_CAPTURE_MIN_COMMANDS",
            0,
            1000000);
    if (minimum_command_count != 0 &&
        command_buffer.commands().size() < minimum_command_count)
    {
        if (!sLoggedWaitingForMinimumCommands)
        {
            LL_INFOS("RenderBackend")
                << "Vulkan world command capture is waiting for at least "
                << minimum_command_count
                << " command(s) in a submitted buffer."
                << LL_ENDL;
            sLoggedWaitingForMinimumCommands = true;
        }
        return;
    }

    const U32 skipped_buffer_count =
        get_vulkan_world_command_capture_u32(
            "MARE_VULKAN_WORLD_COMMAND_CAPTURE_SKIP_BUFFERS",
            0,
            1000000);
    if (sEligibleBuffers < skipped_buffer_count)
    {
        ++sEligibleBuffers;
        return;
    }

    if (!sLoggedCaptureStarted)
    {
        LL_INFOS("RenderBackend")
            << "Vulkan world command capture started: writing up to "
            << capture_buffer_limit
            << " eligible buffer(s) to "
            << path
            << "."
            << LL_ENDL;
        sLoggedCaptureStarted = true;
    }

    std::ofstream output;
    output.open(
        path,
        sCapturedBuffers == 0 ?
            std::ios::out | std::ios::trunc :
            std::ios::out | std::ios::app);
    if (!output.is_open())
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Unable to open Vulkan world command capture file: "
            << path
            << LL_ENDL;
        return;
    }

    if (sCapturedBuffers == 0)
    {
        output << "MareVulkanWorldCommandCaptureV1\n";
        output
            << "capture_options buffers "
            << capture_buffer_limit
            << " skip_buffers "
            << skipped_buffer_count
            << " min_commands "
            << minimum_command_count
            << "\n";
    }

    output
        << "buffer "
        << sCapturedBuffers
        << " commands "
        << command_buffer.commands().size()
        << "\n";

    output << std::fixed << std::setprecision(6);
    for (const LLWorldRenderCommand& command : command_buffer.commands())
    {
        if (!command.mVertexBuffer || !command.mCount)
        {
            continue;
        }

        output
            << "cmd"
            << " material " << static_cast<U32>(command.mMaterialClass)
            << " pass " << static_cast<U32>(command.mPassClass)
            << " blend " << static_cast<U32>(command.mBlendMode)
            << " depth " << static_cast<U32>(command.mDepthMode)
            << " cull " << static_cast<U32>(command.mCullMode)
            << " polygon_offset " << (command.mPolygonOffsetEnabled ? 1 : 0)
            << " polygon_offset_factor " << command.mPolygonOffsetFactor
            << " polygon_offset_units " << command.mPolygonOffsetUnits
            << " write_color " << (command.mWriteColor ? 1 : 0)
            << " write_alpha " << (command.mWriteAlpha ? 1 : 0)
            << " source_pass " << command.mSourcePass
            << " attributes " << command.mAttributeMask
            << " count " << command.mCount
            << " mode " << command.mMode
            << " draw_arrays " << (command.mDrawArrays ? 1 : 0)
            << " use_texture " << (command.mUseTexture ? 1 : 0)
            << " batch_textures " << (command.mBatchTextures ? 1 : 0)
            << " rigged " << (command.mRigged || command.mSkinningMatrixCount > 0 ? 1 : 0)
            << " fullbright " << (command.mFullbright ? 1 : 0)
            << " glow " << (command.mHasGlow ? 1 : 0)
            << " double_sided " << (command.mDoubleSided ? 1 : 0)
            << " base "
            << command.mBaseColor.mV[VRED] << " "
            << command.mBaseColor.mV[VGREEN] << " "
            << command.mBaseColor.mV[VBLUE] << " "
            << command.mBaseColor.mV[VALPHA]
            << " emissive "
            << command.mEmissiveColor.mV[VRED] << " "
            << command.mEmissiveColor.mV[VGREEN] << " "
            << command.mEmissiveColor.mV[VBLUE]
            << " spec "
            << command.mSpecColor.mV[VX] << " "
            << command.mSpecColor.mV[VY] << " "
            << command.mSpecColor.mV[VZ] << " "
            << command.mSpecColor.mV[VW]
            << " factors "
            << command.mMetallicFactor << " "
            << command.mRoughnessFactor << " "
            << command.mEnvIntensity << " "
            << command.mAlphaMaskCutoff
            << " alpha_modes "
            << static_cast<U32>(command.mDiffuseAlphaMode) << " "
            << static_cast<U32>(command.mGLTFAlphaMode)
            << " material_modes "
            << static_cast<U32>(command.mBump) << " "
            << static_cast<U32>(command.mShiny)
            << " terrain "
            << command.mTerrainPaintType << " "
            << command.mTerrainPlanarSampleCount;

        write_vulkan_world_command_capture_texture(
            output,
            "primary_texture",
            command.mTexture);
        write_vulkan_world_command_capture_texture(
            output,
            "normal_texture",
            command.mNormalMap);
        write_vulkan_world_command_capture_texture(
            output,
            "specular_texture",
            command.mSpecularMap);
        write_vulkan_world_command_capture_texture(
            output,
            "orm_texture",
            command.mORMMap);
        write_vulkan_world_command_capture_texture(
            output,
            "emissive_texture",
            command.mEmissiveMap);
        output
            << " texture_list "
            << command.mTextureList.size()
            << " "
            << get_vulkan_world_command_capture_texture_list_count(command);
        write_vulkan_world_command_capture_material_transform(
            output,
            "base_transform",
            command.mBaseColorTextureTransform);
        write_vulkan_world_command_capture_material_transform(
            output,
            "normal_transform",
            command.mNormalTextureTransform);
        write_vulkan_world_command_capture_material_transform(
            output,
            "orm_transform",
            command.mORMTextureTransform);
        write_vulkan_world_command_capture_material_transform(
            output,
            "emissive_transform",
            command.mEmissiveTextureTransform);
        write_vulkan_world_command_capture_texture_matrix(
            output,
            command.mTextureMatrix);
        output << "\n";
    }

    LL_INFOS("RenderBackend")
        << "Captured Vulkan world command buffer "
        << (sCapturedBuffers + 1)
        << "/"
        << capture_buffer_limit
        << " to "
        << path
        << "."
        << LL_ENDL;
    ++sCapturedBuffers;
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
        case LLWorldRenderMaterialClass::ShadowAlphaMask:
        case LLWorldRenderMaterialClass::AvatarAlphaShadow:
        case LLWorldRenderMaterialClass::AvatarAlphaMaskShadow:
        case LLWorldRenderMaterialClass::TreeShadow:
        case LLWorldRenderMaterialClass::PBRAlphaMaskShadow:
        case LLWorldRenderMaterialClass::PBRAlphaBlendShadow:
        case LLWorldRenderMaterialClass::Avatar:
        case LLWorldRenderMaterialClass::AvatarImpostor:
            return command.mAlphaMaskCutoff;
        case LLWorldRenderMaterialClass::Alpha:
            return command.mDepthOnlyAlphaPass ?
                command.mAlphaMaskCutoff :
                WORLD_RENDER_MINIMUM_ALPHA;
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

bool has_world_material_texture_bindings(const LLWorldRenderCommand& command)
{
    return command.mNormalMap.notNull() ||
        command.mORMMap.notNull() ||
        command.mSpecularMap.notNull() ||
        command.mEmissiveMap.notNull();
}

void bind_world_texture_unit(U32 unit, LLViewerTexture* texture)
{
    if (texture)
    {
        gGL.getTexUnit(unit)->bindFast(texture);
    }
    else
    {
        gGL.getTexUnit(unit)->unbindFast(LLTexUnit::TT_TEXTURE);
    }
}

LLWorldRenderTextureTransform2D get_world_material_texture_transform(
    const LLGLTFMaterial::TextureTransform& transform)
{
    LLWorldRenderTextureTransform2D result;
    result.mOffsetS = transform.mOffset[VX];
    result.mOffsetT = transform.mOffset[VY];
    result.mScaleS = transform.mScale[VX];
    result.mScaleT = transform.mScale[VY];
    result.mRotation = transform.mRotation;
    result.mValid = true;
    return result;
}

LLRender::eBlendType get_scene_blend_type(LLWorldRenderBlendMode mode)
{
    switch (mode)
    {
        case LLWorldRenderBlendMode::Alpha:
        case LLWorldRenderBlendMode::ForwardAlpha:
            return LLRender::BT_ALPHA;
        case LLWorldRenderBlendMode::Add:
            return LLRender::BT_ADD;
        case LLWorldRenderBlendMode::Haze:
            return LLRender::BT_REPLACE;
        case LLWorldRenderBlendMode::MultiplyX2:
            return LLRender::BT_MULT_X2;
        case LLWorldRenderBlendMode::None:
        default:
            return LLRender::BT_REPLACE;
    }
}

struct LLWorldRenderSceneLighting
{
    LLColor4 mAmbient = LLColor4(0.36f, 0.36f, 0.36f, 1.f);
    LLColor3 mDirect = LLColor3(1.f, 1.f, 1.f);
    F32 mDirectScale = 1.f;
    LLVector4 mDirection = LLVector4(0.35f, 0.45f, 0.82f, 0.f);
};

LLWorldRenderSceneLighting get_world_render_scene_lighting()
{
    LLWorldRenderSceneLighting lighting;

    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsSky::ptr_t sky = environment.getCurrentSky();
    if (sky)
    {
        lighting.mAmbient = sky->getTotalAmbient();
        const F32 cloud_shadow = llclamp(sky->getCloudShadow(), 0.f, 1.f);
        lighting.mAmbient += (LLColor4::white - lighting.mAmbient) * cloud_shadow * 0.5f;
        lighting.mDirectScale = 1.f - cloud_shadow;

        gPipeline.setupHWLights();
        const LLColor4& selected_diffuse =
            environment.getIsSunUp() ? gPipeline.mSunDiffuse : gPipeline.mMoonDiffuse;
        lighting.mDirect = LLColor3(selected_diffuse);
        lighting.mDirection =
            environment.getIsSunUp() ? gPipeline.mSunDir : gPipeline.mMoonDir;
    }
    else
    {
        lighting.mDirection = environment.getClampedLightNorm();
    }

    const F32 direction_length_sq =
        lighting.mDirection.mV[VX] * lighting.mDirection.mV[VX] +
        lighting.mDirection.mV[VY] * lighting.mDirection.mV[VY] +
        lighting.mDirection.mV[VZ] * lighting.mDirection.mV[VZ];
    if (direction_length_sq <= 0.0001f)
    {
        lighting.mDirection = LLVector4(0.35f, 0.45f, 0.82f, 0.f);
    }

    return lighting;
}

bool has_world_deferred_screen_depth()
{
    return gPipeline.mRT &&
        gPipeline.mRT->deferredScreen.isComplete() &&
        gPipeline.mRT->deferredScreen.getDepth() != 0;
}

bool has_world_deferred_scene_color()
{
    return gPipeline.mRT &&
        gPipeline.mRT->deferredScreen.isComplete();
}

bool uses_world_deferred_screen_depth(const LLWorldRenderCommand& command)
{
    if (command.mDepthMode == LLWorldRenderDepthMode::Disabled)
    {
        return false;
    }

    switch (command.mMaterialClass)
    {
        case LLWorldRenderMaterialClass::Water:
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
        case LLWorldRenderMaterialClass::Alpha:
            return has_world_deferred_screen_depth();
        default:
            return false;
    }
}

bool uses_world_deferred_scene_color(const LLWorldRenderCommand& command)
{
    switch (command.mMaterialClass)
    {
        case LLWorldRenderMaterialClass::Water:
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
            return has_world_deferred_scene_color();
        default:
            return false;
    }
}

F32 get_world_material_flags(const LLWorldRenderCommand& command)
{
    U32 flags = 0;
    if (command.mNormalMap.notNull())
    {
        flags |= LLRenderWorldMaterialParameters::HasNormalMap;
    }
    if (command.mORMMap.notNull())
    {
        flags |= LLRenderWorldMaterialParameters::HasORMMap;
    }
    if (command.mSpecularMap.notNull())
    {
        flags |= LLRenderWorldMaterialParameters::HasSpecularMap;
    }
    if (command.mFullbright)
    {
        flags |= LLRenderWorldMaterialParameters::Fullbright;
    }
    if (command.mMaterialClass == LLWorldRenderMaterialClass::Sky ||
        command.mMaterialClass == LLWorldRenderMaterialClass::WaterExclusionMask ||
        command.mMaterialClass == LLWorldRenderMaterialClass::WaterExclusionSurface ||
        command.mMaterialClass == LLWorldRenderMaterialClass::AtmosphericHaze ||
        command.mMaterialClass == LLWorldRenderMaterialClass::WaterHaze)
    {
        flags |= LLRenderWorldMaterialParameters::Fullbright;
    }
    if (command.mHasGlow || command.mMaterialClass == LLWorldRenderMaterialClass::Glow)
    {
        flags |= LLRenderWorldMaterialParameters::Glow;
    }
    if (command.mMaterialClass == LLWorldRenderMaterialClass::Water)
    {
        flags |= LLRenderWorldMaterialParameters::Water;
    }
    if (command.mBlendMode == LLWorldRenderBlendMode::Alpha ||
        command.mBlendMode == LLWorldRenderBlendMode::ForwardAlpha)
    {
        flags |= LLRenderWorldMaterialParameters::AlphaBlend;
    }
    if (get_world_render_alpha_mask_cutoff(command) >= 0.f)
    {
        flags |= LLRenderWorldMaterialParameters::AlphaMask;
    }
    if (command.mDoubleSided)
    {
        flags |= LLRenderWorldMaterialParameters::DoubleSided;
    }
    if (command.mBump != 0)
    {
        flags |= LLRenderWorldMaterialParameters::LegacyBump;
    }
    if (command.mShiny != 0 ||
        command.mSpecularMap.notNull() ||
        command.mEnvIntensity > 0.f)
    {
        flags |= LLRenderWorldMaterialParameters::LegacyShiny;
    }
    if (command.mMaterialClass == LLWorldRenderMaterialClass::GLTFPBR ||
        command.mMaterialClass == LLWorldRenderMaterialClass::GLTFPBRAlphaMask ||
        command.mMaterialClass == LLWorldRenderMaterialClass::PBRAlphaMaskShadow ||
        command.mMaterialClass == LLWorldRenderMaterialClass::PBRAlphaBlendShadow)
    {
        flags |= LLRenderWorldMaterialParameters::GLTFPBR;
    }
    if (command.mPassClass == LLWorldRenderPassClass::PostDeferred)
    {
        flags |= LLRenderWorldMaterialParameters::PostDeferred;
    }
    if (command.mMaterialClass == LLWorldRenderMaterialClass::AtmosphericHaze)
    {
        flags |= LLRenderWorldMaterialParameters::AtmosphericHaze;
    }
    if (command.mMaterialClass == LLWorldRenderMaterialClass::WaterHaze)
    {
        flags |= LLRenderWorldMaterialParameters::WaterHaze;
    }
    if (command.mMaterialClass == LLWorldRenderMaterialClass::WaterExclusionMask ||
        command.mMaterialClass == LLWorldRenderMaterialClass::WaterExclusionSurface)
    {
        flags |= LLRenderWorldMaterialParameters::WaterExclusionMask;
    }
    if (command.mMaterialClass == LLWorldRenderMaterialClass::AvatarImpostor)
    {
        flags |= LLRenderWorldMaterialParameters::AvatarImpostor;
    }
    const bool uses_scene_depth =
        uses_world_deferred_screen_depth(command) &&
        !(command.mMaterialClass == LLWorldRenderMaterialClass::Alpha &&
          get_vulkan_world_command_boolean_env("MARE_VULKAN_DEBUG_ALPHA_DISABLE_SCENE_DEPTH_CLIP"));
    if (uses_scene_depth)
    {
        flags |= LLRenderWorldMaterialParameters::SceneDepth;
        if (command.mMaterialClass == LLWorldRenderMaterialClass::Alpha &&
            get_vulkan_world_command_boolean_env("MARE_VULKAN_DEBUG_ALPHA_SCENE_DEPTH_FLIP_Y"))
        {
            flags |= LLRenderWorldMaterialParameters::SceneDepthFlipY;
        }
        if (command.mMaterialClass == LLWorldRenderMaterialClass::Alpha &&
            get_vulkan_world_command_boolean_env("MARE_VULKAN_DEBUG_ALPHA_SCENE_DEPTH_REVERSED"))
        {
            flags |= LLRenderWorldMaterialParameters::SceneDepthReversed;
        }
    }
    if (uses_world_deferred_scene_color(command))
    {
        flags |= LLRenderWorldMaterialParameters::SceneColor;
    }
    return static_cast<F32>(flags);
}

LLRenderWorldMaterialParameters get_world_material_parameters(
    const LLWorldRenderCommand& command,
    const LLWorldRenderSceneLighting& scene_lighting)
{
    LLRenderWorldMaterialParameters parameters;
    parameters.mBaseColorRed = command.mBaseColor.mV[VRED];
    parameters.mBaseColorGreen = command.mBaseColor.mV[VGREEN];
    parameters.mBaseColorBlue = command.mBaseColor.mV[VBLUE];
    parameters.mBaseColorAlpha = command.mBaseColor.mV[VALPHA];
    parameters.mEmissiveColorRed = command.mEmissiveColor.mV[VRED];
    parameters.mEmissiveColorGreen = command.mEmissiveColor.mV[VGREEN];
    parameters.mEmissiveColorBlue = command.mEmissiveColor.mV[VBLUE];
    parameters.mHasEmissiveMap = command.mEmissiveMap.notNull() ? 1.f : 0.f;
    parameters.mBaseTextureScaleS = command.mBaseColorTextureTransform.mScaleS;
    parameters.mBaseTextureScaleT = command.mBaseColorTextureTransform.mScaleT;
    parameters.mBaseTextureRotation = command.mBaseColorTextureTransform.mRotation;
    parameters.mBaseTextureOffsetS = command.mBaseColorTextureTransform.mOffsetS;
    parameters.mBaseTextureOffsetT = command.mBaseColorTextureTransform.mOffsetT;
    parameters.mRoughnessFactor = command.mRoughnessFactor;
    parameters.mMetallicFactor = command.mMetallicFactor;
    parameters.mHasORMMap = command.mORMMap.notNull() ? 1.f : 0.f;
    parameters.mMaterialFlags = get_world_material_flags(command);
    parameters.mSpecularColorRed = command.mSpecColor.mV[VRED];
    parameters.mSpecularColorGreen = command.mSpecColor.mV[VGREEN];
    parameters.mSpecularColorBlue = command.mSpecColor.mV[VBLUE];
    parameters.mEnvIntensity = command.mEnvIntensity;
    parameters.mDiffuseAlphaMode = static_cast<F32>(command.mDiffuseAlphaMode);
    parameters.mGLTFAlphaMode = static_cast<F32>(command.mGLTFAlphaMode);
    parameters.mBump = static_cast<F32>(command.mBump);
    parameters.mShiny = static_cast<F32>(command.mShiny);
    parameters.mNormalTextureScaleS = command.mNormalTextureTransform.mScaleS;
    parameters.mNormalTextureScaleT = command.mNormalTextureTransform.mScaleT;
    parameters.mNormalTextureRotation = command.mNormalTextureTransform.mRotation;
    parameters.mNormalTextureOffsetS = command.mNormalTextureTransform.mOffsetS;
    parameters.mNormalTextureOffsetT = command.mNormalTextureTransform.mOffsetT;
    parameters.mORMTextureScaleS = command.mORMTextureTransform.mScaleS;
    parameters.mORMTextureScaleT = command.mORMTextureTransform.mScaleT;
    parameters.mORMTextureRotation = command.mORMTextureTransform.mRotation;
    parameters.mORMTextureOffsetS = command.mORMTextureTransform.mOffsetS;
    parameters.mORMTextureOffsetT = command.mORMTextureTransform.mOffsetT;
    parameters.mEmissiveTextureScaleS = command.mEmissiveTextureTransform.mScaleS;
    parameters.mEmissiveTextureScaleT = command.mEmissiveTextureTransform.mScaleT;
    parameters.mEmissiveTextureRotation = command.mEmissiveTextureTransform.mRotation;
    parameters.mEmissiveTextureOffsetS = command.mEmissiveTextureTransform.mOffsetS;
    parameters.mEmissiveTextureOffsetT = command.mEmissiveTextureTransform.mOffsetT;
    parameters.mSceneAmbientRed = llclamp(scene_lighting.mAmbient.mV[VRED], 0.f, 2.f);
    parameters.mSceneAmbientGreen = llclamp(scene_lighting.mAmbient.mV[VGREEN], 0.f, 2.f);
    parameters.mSceneAmbientBlue = llclamp(scene_lighting.mAmbient.mV[VBLUE], 0.f, 2.f);
    parameters.mSceneDirectScale = llclamp(scene_lighting.mDirectScale, 0.f, 2.f);
    parameters.mSceneDirectRed = llclamp(scene_lighting.mDirect.mV[VRED], 0.f, 2.f);
    parameters.mSceneDirectGreen = llclamp(scene_lighting.mDirect.mV[VGREEN], 0.f, 2.f);
    parameters.mSceneDirectBlue = llclamp(scene_lighting.mDirect.mV[VBLUE], 0.f, 2.f);
    parameters.mSceneLightingValid = 1.f;
    parameters.mSceneLightDirectionX = scene_lighting.mDirection.mV[VX];
    parameters.mSceneLightDirectionY = scene_lighting.mDirection.mV[VY];
    parameters.mSceneLightDirectionZ = scene_lighting.mDirection.mV[VZ];
    parameters.mSceneLightDirectionValid = 1.f;
    return parameters;
}

void apply_world_render_command_state(const LLWorldRenderCommand& command)
{
    getRenderBackend().setColorMask(
        {
            command.mWriteColor,
            command.mWriteColor,
            command.mWriteColor,
            command.mWriteAlpha
        });
    getRenderBackend().setCapability(
        LLRenderCapability::Blend,
        command.mBlendMode != LLWorldRenderBlendMode::None);
    if (command.mBlendMode == LLWorldRenderBlendMode::ForwardAlpha)
    {
        gGL.blendFunc(
            LLRender::BF_SOURCE_ALPHA,
            LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
            LLRender::BF_ZERO,
            LLRender::BF_ONE_MINUS_SOURCE_ALPHA);
    }
    else if (command.mBlendMode == LLWorldRenderBlendMode::Haze)
    {
        gGL.blendFunc(
            LLRender::BF_ONE,
            LLRender::BF_SOURCE_ALPHA,
            LLRender::BF_ZERO,
            LLRender::BF_SOURCE_ALPHA);
    }
    else
    {
        gGL.setSceneBlendType(get_scene_blend_type(command.mBlendMode));
    }

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

    getRenderBackend().setCapability(
        LLRenderCapability::PolygonOffsetFill,
        command.mPolygonOffsetEnabled);
    getRenderBackend().setPolygonOffset(
        command.mPolygonOffsetEnabled ? command.mPolygonOffsetFactor : 0.f,
        command.mPolygonOffsetEnabled ? command.mPolygonOffsetUnits : 0.f);
}
}

bool use_vulkan_world_command_path()
{
    return getRenderBackend().getType() == LLRenderBackendType::Vulkan &&
        getRenderBackend().isReady();
}

LLWorldRenderPipelineContract get_world_render_pipeline_contract(
    LLWorldRenderMaterialClass material_class)
{
    LLWorldRenderPipelineContract contract;
    contract.mMaterialClass = material_class;

    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Fullbright:
        case LLWorldRenderMaterialClass::FullbrightAlphaMask:
        case LLWorldRenderMaterialClass::Alpha:
        case LLWorldRenderMaterialClass::Glow:
        case LLWorldRenderMaterialClass::Water:
        case LLWorldRenderMaterialClass::WaterExclusionMask:
        case LLWorldRenderMaterialClass::WaterExclusionSurface:
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
        case LLWorldRenderMaterialClass::FullbrightShiny:
        case LLWorldRenderMaterialClass::PostBump:
            contract.mPassClass = LLWorldRenderPassClass::PostDeferred;
            break;
        case LLWorldRenderMaterialClass::Sky:
        default:
            contract.mPassClass = LLWorldRenderPassClass::Deferred;
            break;
    }

    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Alpha:
            contract.mBlendMode = LLWorldRenderBlendMode::ForwardAlpha;
            break;
        case LLWorldRenderMaterialClass::Fullbright:
        case LLWorldRenderMaterialClass::FullbrightShiny:
            contract.mBlendMode = LLWorldRenderBlendMode::Alpha;
            break;
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
            contract.mBlendMode = LLWorldRenderBlendMode::Haze;
            break;
        case LLWorldRenderMaterialClass::Glow:
            contract.mBlendMode = LLWorldRenderBlendMode::Add;
            break;
        case LLWorldRenderMaterialClass::PostBump:
            contract.mBlendMode = LLWorldRenderBlendMode::MultiplyX2;
            break;
        default:
            contract.mBlendMode = LLWorldRenderBlendMode::None;
            break;
    }

    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Sky:
        case LLWorldRenderMaterialClass::Alpha:
        case LLWorldRenderMaterialClass::Glow:
        case LLWorldRenderMaterialClass::Water:
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
        case LLWorldRenderMaterialClass::PostBump:
            contract.mDepthMode = LLWorldRenderDepthMode::ReadOnly;
            break;
        default:
            contract.mDepthMode = LLWorldRenderDepthMode::ReadWrite;
            break;
    }

    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Alpha:
        case LLWorldRenderMaterialClass::AvatarImpostor:
        case LLWorldRenderMaterialClass::Sky:
        case LLWorldRenderMaterialClass::Water:
        case LLWorldRenderMaterialClass::WaterExclusionMask:
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
            contract.mCullMode = LLWorldRenderCullMode::Disabled;
            break;
        default:
            contract.mCullMode = LLWorldRenderCullMode::Back;
            break;
    }

    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Terrain:
            contract.mShaderClass = LLRenderWorldShaderClass::Terrain;
            break;
        case LLWorldRenderMaterialClass::Sky:
            contract.mShaderClass = LLRenderWorldShaderClass::Sky;
            break;
        case LLWorldRenderMaterialClass::Water:
            contract.mShaderClass = LLRenderWorldShaderClass::Water;
            break;
        case LLWorldRenderMaterialClass::WaterExclusionMask:
        case LLWorldRenderMaterialClass::WaterExclusionSurface:
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
            contract.mShaderClass = LLRenderWorldShaderClass::Haze;
            break;
        case LLWorldRenderMaterialClass::Alpha:
            contract.mShaderClass = LLRenderWorldShaderClass::Alpha;
            break;
        case LLWorldRenderMaterialClass::Glow:
            contract.mShaderClass = LLRenderWorldShaderClass::Glow;
            break;
        case LLWorldRenderMaterialClass::AlphaMask:
        case LLWorldRenderMaterialClass::Grass:
        case LLWorldRenderMaterialClass::Tree:
        case LLWorldRenderMaterialClass::GLTFPBRAlphaMask:
            contract.mShaderClass = LLRenderWorldShaderClass::AlphaMask;
            break;
        case LLWorldRenderMaterialClass::Fullbright:
        case LLWorldRenderMaterialClass::FullbrightAlphaMask:
        case LLWorldRenderMaterialClass::FullbrightShiny:
            contract.mShaderClass = LLRenderWorldShaderClass::Fullbright;
            break;
        case LLWorldRenderMaterialClass::LegacyMaterial:
        case LLWorldRenderMaterialClass::Bump:
        case LLWorldRenderMaterialClass::PostBump:
            contract.mShaderClass = LLRenderWorldShaderClass::Material;
            break;
        case LLWorldRenderMaterialClass::GLTFPBR:
            contract.mShaderClass = LLRenderWorldShaderClass::PBR;
            break;
        case LLWorldRenderMaterialClass::Avatar:
        case LLWorldRenderMaterialClass::AvatarImpostor:
            contract.mShaderClass = LLRenderWorldShaderClass::Avatar;
            break;
        case LLWorldRenderMaterialClass::Shadow:
            contract.mShaderClass = LLRenderWorldShaderClass::Shadow;
            break;
        case LLWorldRenderMaterialClass::ShadowAlphaMask:
            contract.mShaderClass = LLRenderWorldShaderClass::ShadowAlphaMask;
            break;
        case LLWorldRenderMaterialClass::AvatarShadow:
            contract.mShaderClass = LLRenderWorldShaderClass::AvatarShadow;
            break;
        case LLWorldRenderMaterialClass::AvatarAlphaShadow:
            contract.mShaderClass = LLRenderWorldShaderClass::AvatarAlphaShadow;
            break;
        case LLWorldRenderMaterialClass::AvatarAlphaMaskShadow:
            contract.mShaderClass = LLRenderWorldShaderClass::AvatarAlphaMaskShadow;
            break;
        case LLWorldRenderMaterialClass::TreeShadow:
            contract.mShaderClass = LLRenderWorldShaderClass::TreeShadow;
            break;
        case LLWorldRenderMaterialClass::PBRAlphaMaskShadow:
            contract.mShaderClass = LLRenderWorldShaderClass::PBRAlphaMaskShadow;
            break;
        case LLWorldRenderMaterialClass::PBRAlphaBlendShadow:
            contract.mShaderClass = LLRenderWorldShaderClass::PBRAlphaBlendShadow;
            break;
        default:
            contract.mShaderClass = LLRenderWorldShaderClass::Textured;
            break;
    }

    if (material_class == LLWorldRenderMaterialClass::Glow)
    {
        contract.mWriteColor = false;
        contract.mWriteAlpha = true;
    }
    if (material_class == LLWorldRenderMaterialClass::Glow ||
        material_class == LLWorldRenderMaterialClass::PostBump)
    {
        contract.mPolygonOffsetEnabled = true;
        contract.mPolygonOffsetFactor = -1.f;
        contract.mPolygonOffsetUnits = -1.f;
    }

    return contract;
}

const char* get_world_render_material_class_name(LLWorldRenderMaterialClass material_class)
{
    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Sky: return "Sky";
        case LLWorldRenderMaterialClass::Terrain: return "Terrain";
        case LLWorldRenderMaterialClass::SimpleOpaque: return "SimpleOpaque";
        case LLWorldRenderMaterialClass::AlphaMask: return "AlphaMask";
        case LLWorldRenderMaterialClass::Grass: return "Grass";
        case LLWorldRenderMaterialClass::Tree: return "Tree";
        case LLWorldRenderMaterialClass::Fullbright: return "Fullbright";
        case LLWorldRenderMaterialClass::FullbrightAlphaMask: return "FullbrightAlphaMask";
        case LLWorldRenderMaterialClass::Bump: return "Bump";
        case LLWorldRenderMaterialClass::LegacyMaterial: return "LegacyMaterial";
        case LLWorldRenderMaterialClass::GLTFPBR: return "GLTFPBR";
        case LLWorldRenderMaterialClass::GLTFPBRAlphaMask: return "GLTFPBRAlphaMask";
        case LLWorldRenderMaterialClass::Avatar: return "Avatar";
        case LLWorldRenderMaterialClass::AvatarImpostor: return "AvatarImpostor";
        case LLWorldRenderMaterialClass::Alpha: return "Alpha";
        case LLWorldRenderMaterialClass::Glow: return "Glow";
        case LLWorldRenderMaterialClass::Water: return "Water";
        case LLWorldRenderMaterialClass::WaterExclusionMask: return "WaterExclusionMask";
        case LLWorldRenderMaterialClass::WaterExclusionSurface: return "WaterExclusionSurface";
        case LLWorldRenderMaterialClass::AtmosphericHaze: return "AtmosphericHaze";
        case LLWorldRenderMaterialClass::WaterHaze: return "WaterHaze";
        case LLWorldRenderMaterialClass::FullbrightShiny: return "FullbrightShiny";
        case LLWorldRenderMaterialClass::PostBump: return "PostBump";
        case LLWorldRenderMaterialClass::Shadow: return "Shadow";
        case LLWorldRenderMaterialClass::ShadowAlphaMask: return "ShadowAlphaMask";
        case LLWorldRenderMaterialClass::AvatarShadow: return "AvatarShadow";
        case LLWorldRenderMaterialClass::AvatarAlphaShadow: return "AvatarAlphaShadow";
        case LLWorldRenderMaterialClass::AvatarAlphaMaskShadow: return "AvatarAlphaMaskShadow";
        case LLWorldRenderMaterialClass::TreeShadow: return "TreeShadow";
        case LLWorldRenderMaterialClass::PBRAlphaMaskShadow: return "PBRAlphaMaskShadow";
        case LLWorldRenderMaterialClass::PBRAlphaBlendShadow: return "PBRAlphaBlendShadow";
    }
    return "Unknown";
}

const char* get_world_render_pass_class_name(LLWorldRenderPassClass pass_class)
{
    switch (pass_class)
    {
        case LLWorldRenderPassClass::Deferred: return "Deferred";
        case LLWorldRenderPassClass::PostDeferred: return "PostDeferred";
    }
    return "Unknown";
}

const char* get_world_render_blend_mode_name(LLWorldRenderBlendMode blend_mode)
{
    switch (blend_mode)
    {
        case LLWorldRenderBlendMode::None: return "None";
        case LLWorldRenderBlendMode::Alpha: return "Alpha";
        case LLWorldRenderBlendMode::ForwardAlpha: return "ForwardAlpha";
        case LLWorldRenderBlendMode::Add: return "Add";
        case LLWorldRenderBlendMode::Haze: return "Haze";
        case LLWorldRenderBlendMode::MultiplyX2: return "MultiplyX2";
    }
    return "Unknown";
}

const char* get_world_render_depth_mode_name(LLWorldRenderDepthMode depth_mode)
{
    switch (depth_mode)
    {
        case LLWorldRenderDepthMode::ReadWrite: return "ReadWrite";
        case LLWorldRenderDepthMode::ReadOnly: return "ReadOnly";
        case LLWorldRenderDepthMode::Disabled: return "Disabled";
    }
    return "Unknown";
}

const char* get_world_render_cull_mode_name(LLWorldRenderCullMode cull_mode)
{
    switch (cull_mode)
    {
        case LLWorldRenderCullMode::Back: return "Back";
        case LLWorldRenderCullMode::Disabled: return "Disabled";
    }
    return "Unknown";
}

const char* get_world_render_shader_class_name(LLRenderWorldShaderClass shader_class)
{
    switch (shader_class)
    {
        case LLRenderWorldShaderClass::Textured: return "Textured";
        case LLRenderWorldShaderClass::Sky: return "Sky";
        case LLRenderWorldShaderClass::Water: return "Water";
        case LLRenderWorldShaderClass::Haze: return "Haze";
        case LLRenderWorldShaderClass::Alpha: return "Alpha";
        case LLRenderWorldShaderClass::Glow: return "Glow";
        case LLRenderWorldShaderClass::AlphaMask: return "AlphaMask";
        case LLRenderWorldShaderClass::Fullbright: return "Fullbright";
        case LLRenderWorldShaderClass::Material: return "Material";
        case LLRenderWorldShaderClass::PBR: return "PBR";
        case LLRenderWorldShaderClass::Avatar: return "Avatar";
        case LLRenderWorldShaderClass::Terrain: return "Terrain";
        case LLRenderWorldShaderClass::Shadow: return "Shadow";
        case LLRenderWorldShaderClass::ShadowAlphaMask: return "ShadowAlphaMask";
        case LLRenderWorldShaderClass::AvatarShadow: return "AvatarShadow";
        case LLRenderWorldShaderClass::AvatarAlphaShadow: return "AvatarAlphaShadow";
        case LLRenderWorldShaderClass::AvatarAlphaMaskShadow: return "AvatarAlphaMaskShadow";
        case LLRenderWorldShaderClass::TreeShadow: return "TreeShadow";
        case LLRenderWorldShaderClass::PBRAlphaMaskShadow: return "PBRAlphaMaskShadow";
        case LLRenderWorldShaderClass::PBRAlphaBlendShadow: return "PBRAlphaBlendShadow";
        case LLRenderWorldShaderClass::PointLight: return "PointLight";
        case LLRenderWorldShaderClass::MultiPointLight: return "MultiPointLight";
        case LLRenderWorldShaderClass::SpotLight: return "SpotLight";
        case LLRenderWorldShaderClass::MultiSpotLight: return "MultiSpotLight";
        case LLRenderWorldShaderClass::Copy: return "Copy";
        case LLRenderWorldShaderClass::DeferredLightMap: return "DeferredLightMap";
        case LLRenderWorldShaderClass::DeferredSoften: return "DeferredSoften";
        case LLRenderWorldShaderClass::DeferredComposite: return "DeferredComposite";
        case LLRenderWorldShaderClass::FinalComposite: return "FinalComposite";
    }
    return "Unknown";
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
    U32 attribute_mask,
    bool depth_only_alpha_pass,
    bool alpha_depth_write_pass)
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
    command.mNormalMap = params.mNormalMap;
    command.mSpecularMap = params.mSpecularMap;
    command.mNormalMapMatrix = params.mNormalMapMatrix;
    command.mSpecularMapMatrix = params.mSpecularMapMatrix;
    command.mSpecColor = params.mSpecColor;
    command.mEnvIntensity = params.mEnvIntensity;
    command.mAlphaMaskCutoff = params.mAlphaMaskCutoff;
    if (depth_only_alpha_pass)
    {
        command.mDepthOnlyAlphaPass = true;
        command.mWriteColor = false;
        command.mWriteAlpha = false;
        command.mBlendMode = LLWorldRenderBlendMode::None;
        command.mDepthMode = LLWorldRenderDepthMode::ReadWrite;
        command.mAlphaMaskCutoff = 0.33f;
    }
    command.mBlendFuncSrc = params.mBlendFuncSrc;
    command.mBlendFuncDst = params.mBlendFuncDst;
    command.mDiffuseAlphaMode = params.mDiffuseAlphaMode;
    if (command.mMaterialClass == LLWorldRenderMaterialClass::Alpha &&
        params.mGLTFMaterial.isNull())
    {
        // The legacy OpenGL alpha pass always preserves texture/vertex alpha.
        // Some alpha-pool draw infos carry DIFFUSE_ALPHA_MODE_NONE because
        // they are not material-alpha draws; forcing blend here keeps the
        // backend-neutral command equivalent to alphaF.glsl.
        command.mDiffuseAlphaMode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
    }
    command.mBump = params.mBump;
    command.mShiny = params.mShiny;
    command.mFullbright = params.mFullbright;
    command.mHasGlow = params.mHasGlow;
    command.mAvatar = params.mAvatar;
    command.mSkinInfo = params.mSkinInfo;
    command.mRigged = params.mAvatar != nullptr && params.mSkinInfo != nullptr;
    if (command.mRigged &&
        command.mMaterialClass == LLWorldRenderMaterialClass::Alpha)
    {
        // Match the legacy alpha path: rigged alpha draws populate depth before
        // later alpha sorting uses that depth.
        command.mDepthMode = LLWorldRenderDepthMode::ReadWrite;
    }
    if (alpha_depth_write_pass &&
        command.mMaterialClass == LLWorldRenderMaterialClass::Alpha)
    {
        command.mDepthMode = LLWorldRenderDepthMode::ReadWrite;
    }
    if (params.mGLTFMaterial.notNull())
    {
        command.mBaseColor = params.mGLTFMaterial->mBaseColor;
        command.mEmissiveColor = params.mGLTFMaterial->mEmissiveColor;
        command.mMetallicFactor = params.mGLTFMaterial->mMetallicFactor;
        command.mRoughnessFactor = params.mGLTFMaterial->mRoughnessFactor;
        command.mGLTFAlphaMode = static_cast<U8>(params.mGLTFMaterial->mAlphaMode);
        command.mDoubleSided = params.mGLTFMaterial->mDoubleSided;
        command.mNormalMap = params.mGLTFMaterial->mNormalTexture;
        command.mORMMap = params.mGLTFMaterial->mMetallicRoughnessTexture;
        command.mEmissiveMap = params.mGLTFMaterial->mEmissiveTexture;
        command.mBaseColorTextureTransform =
            get_world_material_texture_transform(
                params.mGLTFMaterial->mTextureTransform[
                    LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR]);
        command.mNormalTextureTransform =
            get_world_material_texture_transform(
                params.mGLTFMaterial->mTextureTransform[
                    LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL]);
        command.mORMTextureTransform =
            get_world_material_texture_transform(
                params.mGLTFMaterial->mTextureTransform[
                    LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS]);
        command.mEmissiveTextureTransform =
            get_world_material_texture_transform(
                params.mGLTFMaterial->mTextureTransform[
                    LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE]);
        if (params.mGLTFMaterial->mDoubleSided)
        {
            command.mCullMode = LLWorldRenderCullMode::Disabled;
        }
        if (command.mTexture.isNull() &&
            params.mGLTFMaterial->mBaseColorTexture.notNull())
        {
            command.mTexture = params.mGLTFMaterial->mBaseColorTexture;
        }
        command.mAlphaMaskCutoff = params.mGLTFMaterial->mAlphaCutoff;
    }
    if (LLPipeline::sRenderingHUDs)
    {
        // HUD attachments are composited over the world. They must not be
        // rejected by the world depth buffer or culled with world-facing state.
        command.mDepthMode = LLWorldRenderDepthMode::Disabled;
        command.mCullMode = LLWorldRenderCullMode::Disabled;
    }
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
    LLViewerTexture* const* orm_textures,
    LLViewerTexture* const* emissive_textures,
    LLViewerTexture* const* normal_textures,
    const LLColor4* base_color_factors,
    const F32* metallic_factors,
    const F32* roughness_factors,
    const LLColor4* emissive_minimum_alphas,
    const F32* texture_transforms,
    F32 region_scale,
    U32 paint_type,
    U32 planar_sample_count,
    F32 triplanar_blend_factor,
    bool uses_pbr_materials,
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
    command.mAttributeMask |=
        LLVertexBuffer::MAP_NORMAL |
        LLVertexBuffer::MAP_TANGENT |
        LLVertexBuffer::MAP_TEXCOORD1;
    command.mTexture = detail_texture0;
    command.mTextureList.clear();
    command.mTextureList.reserve(17);
    command.mTextureList.push_back(detail_texture0);
    command.mTextureList.push_back(detail_texture1);
    command.mTextureList.push_back(detail_texture2);
    command.mTextureList.push_back(detail_texture3);
    command.mTextureList.push_back(alpha_ramp);
    for (U32 i = 0; i < 4; ++i)
    {
        command.mTextureList.push_back(orm_textures ? orm_textures[i] : nullptr);
    }
    for (U32 i = 0; i < 4; ++i)
    {
        command.mTextureList.push_back(emissive_textures ? emissive_textures[i] : nullptr);
    }
    for (U32 i = 0; i < 4; ++i)
    {
        command.mTextureList.push_back(normal_textures ? normal_textures[i] : nullptr);
    }
    command.mUseTexture = true;
    command.mBatchTextures = true;
    command.mTerrainDetailScale = detail_scale;
    command.mTerrainOffsetX = offset_x;
    command.mTerrainOffsetY = offset_y;
    command.mTerrainRegionScale = region_scale;
    command.mTerrainPaintType = paint_type;
    command.mTerrainPlanarSampleCount = planar_sample_count;
    command.mTerrainTriplanarBlendFactor = triplanar_blend_factor;
    command.mTerrainUsesPBRMaterials = uses_pbr_materials;
    if (base_color_factors)
    {
        for (U32 i = 0; i < 4; ++i)
        {
            command.mTerrainBaseColorFactors[i] = base_color_factors[i];
        }
    }
    if (metallic_factors)
    {
        for (U32 i = 0; i < 4; ++i)
        {
            command.mTerrainMetallicFactors[i] = metallic_factors[i];
        }
    }
    if (roughness_factors)
    {
        for (U32 i = 0; i < 4; ++i)
        {
            command.mTerrainRoughnessFactors[i] = roughness_factors[i];
        }
    }
    if (emissive_minimum_alphas)
    {
        for (U32 i = 0; i < 4; ++i)
        {
            command.mTerrainEmissiveMinimumAlphas[i] = emissive_minimum_alphas[i];
        }
    }
    if (texture_transforms)
    {
        for (U32 i = 0; i < 20; ++i)
        {
            command.mTerrainTextureTransforms[i] = texture_transforms[i];
        }
    }
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
    U32 attribute_mask,
    U32 mode)
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
    command.mMode = mode;
    mCommands.push_back(command);
}

LLWorldRenderCommand* LLWorldRenderCommandBuffer::appendOwnedDrawRange(
    LLVertexBuffer* vertex_buffer,
    LLViewerTexture* texture,
    LLWorldRenderMaterialClass material_class,
    U32 source_pass,
    const LLMatrix4& model_matrix,
    U32 start,
    U32 end,
    U32 count,
    U32 offset,
    bool use_texture,
    bool batch_textures,
    U32 attribute_mask,
    U32 mode)
{
    const size_t command_count = mCommands.size();
    appendDrawRange(
        vertex_buffer,
        texture,
        material_class,
        source_pass,
        nullptr,
        start,
        end,
        count,
        offset,
        use_texture,
        batch_textures,
        attribute_mask,
        mode);
    if (mCommands.size() == command_count)
    {
        return nullptr;
    }

    LLWorldRenderCommand& command = mCommands.back();
    command.mOwnedModelMatrix = model_matrix;
    command.mHasOwnedModelMatrix = true;
    return &command;
}

LLWorldRenderCommand* LLWorldRenderCommandBuffer::appendDrawArrays(
    LLVertexBuffer* vertex_buffer,
    LLViewerTexture* texture,
    LLWorldRenderMaterialClass material_class,
    U32 source_pass,
    const LLMatrix4* model_matrix,
    U32 first,
    U32 count,
    bool use_texture,
    bool batch_textures,
    U32 attribute_mask,
    U32 mode)
{
    if (!vertex_buffer || !count)
    {
        return nullptr;
    }

    LLWorldRenderCommand command;
    command.mVertexBuffer = vertex_buffer;
    command.mTexture = texture;
    command.mMaterialClass = material_class;
    classify_world_render_command(command);
    command.mSourcePass = source_pass;
    command.mModelMatrix = model_matrix;
    command.mStart = first;
    command.mEnd = first + count - 1;
    command.mFirst = first;
    command.mCount = count;
    command.mOffset = 0;
    command.mUseTexture = use_texture;
    command.mBatchTextures = batch_textures;
    command.mAttributeMask = attribute_mask;
    command.mMode = mode;
    command.mDrawArrays = true;
    mCommands.push_back(command);
    return &mCommands.back();
}

void LLWorldRenderCommandBuffer::appendAvatarDrawRange(
    LLVertexBuffer* vertex_buffer,
    LLViewerTexture* texture,
    LLWorldRenderMaterialClass material_class,
    U32 source_pass,
    U32 start,
    U32 end,
    U32 count,
    U32 offset,
    const std::vector<F32>& skinning_matrix_palette,
    U32 skinning_matrix_count,
    U32 attribute_mask)
{
    const size_t command_count = mCommands.size();
    appendDrawRange(
        vertex_buffer,
        texture,
        material_class,
        source_pass,
        nullptr,
        start,
        end,
        count,
        offset,
        true,
        false,
        attribute_mask);
    if (mCommands.size() == command_count)
    {
        return;
    }

    LLWorldRenderCommand& command = mCommands.back();
    command.mSkinningMatrixPalette = skinning_matrix_palette;
    command.mSkinningMatrixCount = skinning_matrix_count;
    command.mAlphaMaskCutoff = AVATAR_RENDER_MINIMUM_ALPHA;
}

void LLWorldRenderCommandBuffer::appendAvatarRigidDrawRange(
    LLVertexBuffer* vertex_buffer,
    LLViewerTexture* texture,
    const LLMatrix4& model_matrix,
    LLWorldRenderMaterialClass material_class,
    U32 source_pass,
    U32 start,
    U32 end,
    U32 count,
    U32 offset,
    U32 attribute_mask)
{
    const size_t command_count = mCommands.size();
    appendDrawRange(
        vertex_buffer,
        texture,
        material_class,
        source_pass,
        nullptr,
        start,
        end,
        count,
        offset,
        true,
        false,
        attribute_mask);
    if (mCommands.size() == command_count)
    {
        return;
    }

    LLWorldRenderCommand& command = mCommands.back();
    command.mOwnedModelMatrix = model_matrix;
    command.mHasOwnedModelMatrix = true;
    command.mAlphaMaskCutoff = AVATAR_RENDER_MINIMUM_ALPHA;
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

void LLWorldRenderCommandBuffer::appendShadowRenderMap(
    U32 source_pass,
    LLWorldRenderMaterialClass material_class,
    bool texture,
    bool batch_textures,
    U32 attribute_mask,
    bool write_color,
    bool write_alpha,
    F32 alpha_mask_cutoff)
{
    auto* begin = gPipeline.beginRenderMap(source_pass);
    auto* end = gPipeline.endRenderMap(source_pass);
    for (LLCullResult::drawinfo_iterator iter = begin; iter != end; )
    {
        LLDrawInfo* params = *iter;
        LLCullResult::increment_iterator(iter, end);
        if (!params)
        {
            continue;
        }

        const size_t command_count = mCommands.size();
        appendDrawInfo(
            *params,
            material_class,
            source_pass,
            texture,
            batch_textures,
            attribute_mask);
        if (mCommands.size() == command_count)
        {
            continue;
        }

        LLWorldRenderCommand& command = mCommands.back();
        command.mPassClass = LLWorldRenderPassClass::Deferred;
        command.mBlendMode = LLWorldRenderBlendMode::None;
        command.mDepthMode = LLWorldRenderDepthMode::ReadWrite;
        command.mWriteColor = write_color;
        command.mWriteAlpha = write_alpha;
        if (alpha_mask_cutoff >= 0.f)
        {
            command.mAlphaMaskCutoff = alpha_mask_cutoff;
        }
    }
}

void LLWorldRenderCommandBuffer::appendShadowAlphaRenderMap(
    bool rigged,
    U32 attribute_mask,
    bool write_color,
    bool write_alpha)
{
    auto* begin = gPipeline.beginRenderMap(LLRenderPass::PASS_ALPHA);
    auto* end = gPipeline.endRenderMap(LLRenderPass::PASS_ALPHA);
    for (LLCullResult::drawinfo_iterator iter = begin; iter != end; )
    {
        LLDrawInfo* params = *iter;
        LLCullResult::increment_iterator(iter, end);
        if (!params || rigged != (params->mAvatar != nullptr))
        {
            continue;
        }

        const LLWorldRenderMaterialClass material_class =
            params->mGLTFMaterial.notNull() ?
                LLWorldRenderMaterialClass::PBRAlphaBlendShadow :
                LLWorldRenderMaterialClass::ShadowAlphaMask;
        const size_t command_count = mCommands.size();
        appendDrawInfo(
            *params,
            material_class,
            LLRenderPass::PASS_ALPHA,
            true,
            true,
            attribute_mask);
        if (mCommands.size() == command_count)
        {
            continue;
        }

        LLWorldRenderCommand& command = mCommands.back();
        command.mPassClass = LLWorldRenderPassClass::Deferred;
        command.mBlendMode = LLWorldRenderBlendMode::None;
        command.mDepthMode = LLWorldRenderDepthMode::ReadWrite;
        command.mWriteColor = write_color;
        command.mWriteAlpha = write_alpha;
        command.mAlphaMaskCutoff = WORLD_RENDER_SHADOW_ALPHA_BLEND_CUTOFF;
    }
}

void LLWorldRenderCommandBuffer::appendRenderMapWithColor(
    U32 source_pass,
    LLWorldRenderMaterialClass material_class,
    bool texture,
    bool batch_textures,
    U32 attribute_mask,
    const LLColor4& base_color,
    bool fullbright)
{
    auto* begin = gPipeline.beginRenderMap(source_pass);
    auto* end = gPipeline.endRenderMap(source_pass);
    for (LLCullResult::drawinfo_iterator iter = begin; iter != end; )
    {
        LLDrawInfo* params = *iter;
        LLCullResult::increment_iterator(iter, end);
        if (!params)
        {
            continue;
        }

        const size_t command_count = mCommands.size();
        appendDrawInfo(
            *params,
            material_class,
            source_pass,
            texture,
            batch_textures,
            attribute_mask);
        if (mCommands.size() != command_count)
        {
            mCommands.back().mBaseColor = base_color;
            mCommands.back().mFullbright = fullbright;
        }
    }
}

void submit_vulkan_world_commands(const LLWorldRenderCommandBuffer& command_buffer)
{
    if (command_buffer.empty())
    {
        return;
    }

    log_vulkan_world_command_summary(command_buffer);
    write_vulkan_world_command_capture(command_buffer);

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
    bool active_polygon_offset_enabled = false;
    F32 active_polygon_offset_factor = 0.f;
    F32 active_polygon_offset_units = 0.f;
    bool active_write_color = true;
    bool active_write_alpha = true;
    bool state_bound = false;
    const LLWorldRenderSceneLighting scene_lighting = get_world_render_scene_lighting();

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
            active_cull_mode != command.mCullMode ||
            active_polygon_offset_enabled != command.mPolygonOffsetEnabled ||
            active_polygon_offset_factor != command.mPolygonOffsetFactor ||
            active_polygon_offset_units != command.mPolygonOffsetUnits ||
            active_write_color != command.mWriteColor ||
            active_write_alpha != command.mWriteAlpha)
        {
            apply_world_render_command_state(command);
            active_blend_mode = command.mBlendMode;
            active_depth_mode = command.mDepthMode;
            active_cull_mode = command.mCullMode;
            active_polygon_offset_enabled = command.mPolygonOffsetEnabled;
            active_polygon_offset_factor = command.mPolygonOffsetFactor;
            active_polygon_offset_units = command.mPolygonOffsetUnits;
            active_write_color = command.mWriteColor;
            active_write_alpha = command.mWriteAlpha;
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
        const LLWorldRenderPipelineContract contract =
            get_world_render_pipeline_contract(command.mMaterialClass);
        if (command.mMaterialClass == LLWorldRenderMaterialClass::Terrain)
        {
            LLRenderWorldTerrainParameters terrain_parameters;
            terrain_parameters.mDetailScale = command.mTerrainDetailScale;
            terrain_parameters.mOffsetX = command.mTerrainOffsetX;
            terrain_parameters.mOffsetY = command.mTerrainOffsetY;
            terrain_parameters.mRegionScale = command.mTerrainRegionScale;
            terrain_parameters.mPaintType = static_cast<F32>(command.mTerrainPaintType);
            terrain_parameters.mPlanarSampleCount =
                static_cast<F32>(command.mTerrainPlanarSampleCount);
            terrain_parameters.mTriplanarBlendFactor =
                command.mTerrainTriplanarBlendFactor;
            terrain_parameters.mUsesPBRMaterials =
                command.mTerrainUsesPBRMaterials ? 1.f : 0.f;
            for (U32 i = 0; i < 4; ++i)
            {
                terrain_parameters.mBaseColorFactors[i * 4 + 0] =
                    command.mTerrainBaseColorFactors[i].mV[VRED];
                terrain_parameters.mBaseColorFactors[i * 4 + 1] =
                    command.mTerrainBaseColorFactors[i].mV[VGREEN];
                terrain_parameters.mBaseColorFactors[i * 4 + 2] =
                    command.mTerrainBaseColorFactors[i].mV[VBLUE];
                terrain_parameters.mBaseColorFactors[i * 4 + 3] =
                    command.mTerrainBaseColorFactors[i].mV[VALPHA];
                terrain_parameters.mMetallicFactors[i] =
                    command.mTerrainMetallicFactors[i];
                terrain_parameters.mRoughnessFactors[i] =
                    command.mTerrainRoughnessFactors[i];
                terrain_parameters.mEmissiveMinimumAlpha[i * 4 + 0] =
                    command.mTerrainEmissiveMinimumAlphas[i].mV[VRED];
                terrain_parameters.mEmissiveMinimumAlpha[i * 4 + 1] =
                    command.mTerrainEmissiveMinimumAlphas[i].mV[VGREEN];
                terrain_parameters.mEmissiveMinimumAlpha[i * 4 + 2] =
                    command.mTerrainEmissiveMinimumAlphas[i].mV[VBLUE];
                terrain_parameters.mEmissiveMinimumAlpha[i * 4 + 3] =
                    command.mTerrainEmissiveMinimumAlphas[i].mV[VALPHA];
            }
            for (U32 i = 0; i < 20; ++i)
            {
                terrain_parameters.mTextureTransforms[i] =
                    command.mTerrainTextureTransforms[i];
            }
            getRenderBackend().setWorldTerrainParameters(terrain_parameters);
        }
        getRenderBackend().setWorldShaderClass(contract.mShaderClass);

        getRenderBackend().setWorldMaterialParameters(
            get_world_material_parameters(command, scene_lighting));

        if (!program_bound || active_attribute_mask != command.mAttributeMask)
        {
            if (program_bound)
            {
                gUIProgram.unbind();
            }

            active_attribute_mask = command.mAttributeMask;
            gUIProgram.mAttributeMask = active_attribute_mask;
            gUIProgram.bind();
            LLVertexBuffer::setupClientArrays(active_attribute_mask);
            program_bound = true;
        }

        if (command.mRigged)
        {
            if (!LLRenderPass::uploadMatrixPalette(command.mAvatar, command.mSkinInfo))
            {
                continue;
            }
        }
        else if (command.mSkinningMatrixCount > 0 && !command.mSkinningMatrixPalette.empty())
        {
            getRenderBackend().setWorldSkinningMatrixPalette(
                command.mSkinningMatrixCount,
                command.mSkinningMatrixPalette.data());
        }
        else
        {
            getRenderBackend().setWorldSkinningMatrixPalette(0, nullptr);
        }

        LLRenderPass::applyModelMatrix(
            command.mHasOwnedModelMatrix ?
                &command.mOwnedModelMatrix :
                command.mModelMatrix);

        bool tex_setup = false;
        if (command.mMaterialClass == LLWorldRenderMaterialClass::AvatarImpostor &&
            command.mAvatar &&
            command.mAvatar->mImpostor.isComplete())
        {
            command.mAvatar->mImpostor.bindTexture(0, 0, LLTexUnit::TFO_BILINEAR);
            const U32 texture_count = command.mAvatar->mImpostor.getNumTextures();
            if (texture_count >= 3)
            {
                command.mAvatar->mImpostor.bindTexture(2, 1, LLTexUnit::TFO_BILINEAR);
            }
            else
            {
                gGL.getTexUnit(1)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
            if (texture_count >= 2)
            {
                command.mAvatar->mImpostor.bindTexture(1, 2, LLTexUnit::TFO_BILINEAR);
            }
            else
            {
                gGL.getTexUnit(2)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
        }
        else if (command.mUseTexture)
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
            else if (has_world_material_texture_bindings(command))
            {
                bind_world_texture_unit(0, command.mTexture);
                bind_world_texture_unit(1, command.mNormalMap);
                bind_world_texture_unit(2, command.mORMMap.notNull() ?
                    command.mORMMap.get() :
                    command.mSpecularMap.get());
                bind_world_texture_unit(3, command.mEmissiveMap);
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

        if (command.mMaterialClass == LLWorldRenderMaterialClass::Water ||
            command.mMaterialClass == LLWorldRenderMaterialClass::AtmosphericHaze ||
            command.mMaterialClass == LLWorldRenderMaterialClass::WaterHaze ||
            command.mMaterialClass == LLWorldRenderMaterialClass::Alpha)
        {
            if (has_world_deferred_screen_depth())
            {
                gGL.getTexUnit(WORLD_RENDER_SCENE_DEPTH_TEXTURE_UNIT)->bind(
                    &gPipeline.mRT->deferredScreen,
                    true);
            }
            else
            {
                gGL.getTexUnit(WORLD_RENDER_SCENE_DEPTH_TEXTURE_UNIT)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
        }

        if (command.mMaterialClass == LLWorldRenderMaterialClass::Water ||
            command.mMaterialClass == LLWorldRenderMaterialClass::WaterHaze)
        {
            if (gPipeline.mWaterExclusionMask.isComplete())
            {
                gPipeline.mWaterExclusionMask.bindTexture(0, 5, LLTexUnit::TFO_BILINEAR);
            }
            else
            {
                gGL.getTexUnit(5)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
        }

        if (command.mMaterialClass == LLWorldRenderMaterialClass::Water ||
            command.mMaterialClass == LLWorldRenderMaterialClass::AtmosphericHaze ||
            command.mMaterialClass == LLWorldRenderMaterialClass::WaterHaze)
        {
            if (has_world_deferred_scene_color())
            {
                gPipeline.mRT->deferredScreen.bindTexture(
                    0,
                    WORLD_RENDER_SCENE_COLOR_TEXTURE_UNIT,
                    LLTexUnit::TFO_BILINEAR);
            }
            else
            {
                gGL.getTexUnit(WORLD_RENDER_SCENE_COLOR_TEXTURE_UNIT)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
        }

        vertex_buffer->setBuffer();
        if (command.mDrawArrays)
        {
            vertex_buffer->drawArrays(
                command.mMode,
                command.mFirst,
                command.mCount);
        }
        else
        {
            vertex_buffer->drawRange(
                command.mMode,
                command.mStart,
                command.mEnd,
                command.mCount,
                command.mOffset);
        }

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
    getRenderBackend().setWorldMaterialParameters({});
    getRenderBackend().setWorldTextureTransform({});
    getRenderBackend().setWorldSkinningMatrixPalette(0, nullptr);
    getRenderBackend().setCapability(LLRenderCapability::PolygonOffsetFill, false);
    getRenderBackend().setPolygonOffset(0.f, 0.f);
    getRenderBackend().setColorMask({ true, true, true, true });
    gUIProgram.mAttributeMask = saved_attribute_mask;
    LLVertexBuffer::setupClientArrays(saved_attribute_mask);
}
