/**
 * @file llworldrendercommand.h
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

#ifndef LL_LLWORLDRENDERCOMMAND_H
#define LL_LLWORLDRENDERCOMMAND_H

#include "llpointer.h"
#include "m4math.h"
#include "llrender.h"
#include "llrenderbackendtypes.h"
#include "llviewertexture.h"
#include "llvertexbuffer.h"
#include "v3color.h"
#include "v4color.h"

#include <vector>

class LLDrawInfo;
class LLFace;
class LLMatrix4;
class LLMeshSkinInfo;
class LLVOAvatar;

enum class LLWorldRenderMaterialClass : U8
{
    Sky,
    Terrain,
    SimpleOpaque,
    AlphaMask,
    Grass,
    Tree,
    Fullbright,
    FullbrightAlphaMask,
    Bump,
    LegacyMaterial,
    GLTFPBR,
    GLTFPBRAlphaMask,
    Avatar,
    AvatarImpostor,
    Alpha,
    Glow,
    Water,
    WaterExclusionMask,
    WaterExclusionSurface,
    AtmosphericHaze,
    WaterHaze,
    FullbrightShiny,
    PostBump,
    Shadow,
    ShadowAlphaMask,
    AvatarShadow,
    AvatarAlphaShadow,
    AvatarAlphaMaskShadow,
    TreeShadow,
    PBRAlphaMaskShadow,
    PBRAlphaBlendShadow,
};

enum class LLWorldRenderPassClass : U8
{
    Deferred,
    PostDeferred,
};

enum class LLWorldRenderBlendMode : U8
{
    None,
    Alpha,
    ForwardAlpha,
    Add,
    Haze,
    MultiplyX2,
};

enum class LLWorldRenderDepthMode : U8
{
    ReadWrite,
    ReadOnly,
    Disabled,
};

enum class LLWorldRenderCullMode : U8
{
    Back,
    Disabled,
};

struct LLWorldRenderTextureTransform2D
{
    F32 mOffsetS = 0.f;
    F32 mOffsetT = 0.f;
    F32 mScaleS = 1.f;
    F32 mScaleT = 1.f;
    F32 mRotation = 0.f;
    bool mValid = false;
};

struct LLWorldRenderCommand
{
    LLWorldRenderMaterialClass mMaterialClass = LLWorldRenderMaterialClass::SimpleOpaque;
    LLWorldRenderPassClass mPassClass = LLWorldRenderPassClass::Deferred;
    LLWorldRenderBlendMode mBlendMode = LLWorldRenderBlendMode::None;
    LLWorldRenderDepthMode mDepthMode = LLWorldRenderDepthMode::ReadWrite;
    LLWorldRenderCullMode mCullMode = LLWorldRenderCullMode::Back;
    bool mPolygonOffsetEnabled = false;
    F32 mPolygonOffsetFactor = 0.f;
    F32 mPolygonOffsetUnits = 0.f;
    bool mWriteColor = true;
    bool mWriteAlpha = true;
    bool mDepthOnlyAlphaPass = false;
    U32 mSourcePass = 0;
    U32 mAttributeMask = 0;

    LLPointer<LLVertexBuffer> mVertexBuffer;
    LLPointer<LLViewerTexture> mTexture;
    LLPointer<LLViewerTexture> mNormalMap;
    LLPointer<LLViewerTexture> mSpecularMap;
    LLPointer<LLViewerTexture> mORMMap;
    LLPointer<LLViewerTexture> mEmissiveMap;
    std::vector<LLPointer<LLViewerTexture> > mTextureList;

    const LLMatrix4* mModelMatrix = nullptr;
    LLMatrix4 mOwnedModelMatrix;
    bool mHasOwnedModelMatrix = false;
    const LLMatrix4* mTextureMatrix = nullptr;
    const LLMatrix4* mNormalMapMatrix = nullptr;
    const LLMatrix4* mSpecularMapMatrix = nullptr;
    LLVOAvatar* mAvatar = nullptr;
    LLMeshSkinInfo* mSkinInfo = nullptr;
    std::vector<F32> mSkinningMatrixPalette;
    U32 mSkinningMatrixCount = 0;

    U32 mStart = 0;
    U32 mEnd = 0;
    U32 mCount = 0;
    U32 mOffset = 0;
    U32 mFirst = 0;

    LLColor4 mBaseColor = LLColor4(1.f, 1.f, 1.f, 1.f);
    LLColor3 mEmissiveColor = LLColor3(0.f, 0.f, 0.f);
    LLVector4 mSpecColor = LLVector4(1.f, 1.f, 1.f, 0.5f);
    LLWorldRenderTextureTransform2D mBaseColorTextureTransform;
    LLWorldRenderTextureTransform2D mNormalTextureTransform;
    LLWorldRenderTextureTransform2D mORMTextureTransform;
    LLWorldRenderTextureTransform2D mEmissiveTextureTransform;
    F32 mMetallicFactor = 1.f;
    F32 mRoughnessFactor = 1.f;
    F32 mEnvIntensity = 0.f;
    F32 mAlphaMaskCutoff = 0.5f;
    F32 mTerrainDetailScale = 1.f;
    F32 mTerrainOffsetX = 0.f;
    F32 mTerrainOffsetY = 0.f;
    F32 mTerrainRegionScale = 256.f;
    U32 mTerrainPaintType = 0;
    U32 mTerrainPlanarSampleCount = 1;
    F32 mTerrainTriplanarBlendFactor = 8.f;
    bool mTerrainUsesPBRMaterials = false;
    LLColor4 mTerrainBaseColorFactors[4] =
    {
        LLColor4(1.f, 1.f, 1.f, 1.f),
        LLColor4(1.f, 1.f, 1.f, 1.f),
        LLColor4(1.f, 1.f, 1.f, 1.f),
        LLColor4(1.f, 1.f, 1.f, 1.f),
    };
    F32 mTerrainMetallicFactors[4] =
    {
        0.f, 0.f, 0.f, 0.f,
    };
    F32 mTerrainRoughnessFactors[4] =
    {
        1.f, 1.f, 1.f, 1.f,
    };
    LLColor4 mTerrainEmissiveMinimumAlphas[4] =
    {
        LLColor4(0.f, 0.f, 0.f, 0.f),
        LLColor4(0.f, 0.f, 0.f, 0.f),
        LLColor4(0.f, 0.f, 0.f, 0.f),
        LLColor4(0.f, 0.f, 0.f, 0.f),
    };
    F32 mTerrainTextureTransforms[20] =
    {
        1.f, 1.f, 0.f, 0.f, 0.f,
        1.f, 1.f, 0.f, 0.f, 0.f,
        1.f, 1.f, 0.f, 0.f, 0.f,
        1.f, 1.f, 0.f, 0.f, 0.f,
    };
    LLRender::eBlendFactor mBlendFuncSrc = LLRender::BF_SOURCE_ALPHA;
    LLRender::eBlendFactor mBlendFuncDst = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;
    U8 mDiffuseAlphaMode = 0;
    U8 mGLTFAlphaMode = 0;
    U8 mBump = 0;
    U8 mShiny = 0;
    bool mUseTexture = true;
    bool mBatchTextures = false;
    bool mRigged = false;
    bool mDoubleSided = false;
    bool mFullbright = false;
    bool mHasGlow = false;
    U32 mMode = LLRender::TRIANGLES;
    bool mDrawArrays = false;
};

struct LLWorldRenderPipelineContract
{
    LLWorldRenderMaterialClass mMaterialClass = LLWorldRenderMaterialClass::SimpleOpaque;
    LLWorldRenderPassClass mPassClass = LLWorldRenderPassClass::Deferred;
    LLWorldRenderBlendMode mBlendMode = LLWorldRenderBlendMode::None;
    LLWorldRenderDepthMode mDepthMode = LLWorldRenderDepthMode::ReadWrite;
    LLWorldRenderCullMode mCullMode = LLWorldRenderCullMode::Back;
    LLRenderWorldShaderClass mShaderClass = LLRenderWorldShaderClass::Textured;
    bool mPolygonOffsetEnabled = false;
    F32 mPolygonOffsetFactor = 0.f;
    F32 mPolygonOffsetUnits = 0.f;
    bool mWriteColor = true;
    bool mWriteAlpha = true;
};

class LLWorldRenderCommandBuffer
{
public:
    using command_list_t = std::vector<LLWorldRenderCommand>;

    void clear();
    bool empty() const { return mCommands.empty(); }
    U32 size() const { return static_cast<U32>(mCommands.size()); }

    void appendDrawInfo(
        const LLDrawInfo& params,
        LLWorldRenderMaterialClass material_class,
        U32 source_pass,
        bool texture,
        bool batch_textures,
        U32 attribute_mask,
        bool depth_only_alpha_pass = false,
        bool alpha_depth_write_pass = false);

    void appendFace(
        const LLFace& face,
        LLWorldRenderMaterialClass material_class,
        U32 source_pass);

    void appendTerrainFace(
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
        F32 offset_y);

    void appendDrawRange(
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
        U32 mode = LLRender::TRIANGLES);

    LLWorldRenderCommand* appendOwnedDrawRange(
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
        U32 mode = LLRender::TRIANGLES);

    LLWorldRenderCommand* appendDrawArrays(
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
        U32 mode = LLRender::TRIANGLES);

    void appendAvatarDrawRange(
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
        U32 attribute_mask);

    void appendAvatarRigidDrawRange(
        LLVertexBuffer* vertex_buffer,
        LLViewerTexture* texture,
        const LLMatrix4& model_matrix,
        LLWorldRenderMaterialClass material_class,
        U32 source_pass,
        U32 start,
        U32 end,
        U32 count,
        U32 offset,
        U32 attribute_mask);

    void appendRenderMap(
        U32 source_pass,
        LLWorldRenderMaterialClass material_class,
        bool texture,
        bool batch_textures,
        U32 attribute_mask);

    void appendShadowRenderMap(
        U32 source_pass,
        LLWorldRenderMaterialClass material_class,
        bool texture,
        bool batch_textures,
        U32 attribute_mask,
        bool write_color,
        bool write_alpha,
        F32 alpha_mask_cutoff = -1.f);

    void appendShadowAlphaRenderMap(
        bool rigged,
        U32 attribute_mask,
        bool write_color,
        bool write_alpha);

    void appendRenderMapWithColor(
        U32 source_pass,
        LLWorldRenderMaterialClass material_class,
        bool texture,
        bool batch_textures,
        U32 attribute_mask,
        const LLColor4& base_color,
        bool fullbright);

    const command_list_t& commands() const { return mCommands; }

private:
    command_list_t mCommands;
};

bool use_vulkan_world_command_path();
void submit_vulkan_world_commands(const LLWorldRenderCommandBuffer& command_buffer);
LLWorldRenderPipelineContract get_world_render_pipeline_contract(
    LLWorldRenderMaterialClass material_class);
const char* get_world_render_material_class_name(
    LLWorldRenderMaterialClass material_class);
const char* get_world_render_pass_class_name(LLWorldRenderPassClass pass_class);
const char* get_world_render_blend_mode_name(LLWorldRenderBlendMode blend_mode);
const char* get_world_render_depth_mode_name(LLWorldRenderDepthMode depth_mode);
const char* get_world_render_cull_mode_name(LLWorldRenderCullMode cull_mode);
const char* get_world_render_shader_class_name(
    LLRenderWorldShaderClass shader_class);

#endif
