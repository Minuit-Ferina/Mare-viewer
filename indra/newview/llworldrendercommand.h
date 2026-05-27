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
#include "llrender.h"
#include "llviewertexture.h"
#include "llvertexbuffer.h"

#include <vector>

class LLDrawInfo;
class LLFace;
class LLMatrix4;

enum class LLWorldRenderMaterialClass : U8
{
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
    Alpha,
    Glow,
    Water,
    FullbrightShiny,
    PostBump,
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
    Add,
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

struct LLWorldRenderCommand
{
    LLWorldRenderMaterialClass mMaterialClass = LLWorldRenderMaterialClass::SimpleOpaque;
    LLWorldRenderPassClass mPassClass = LLWorldRenderPassClass::Deferred;
    LLWorldRenderBlendMode mBlendMode = LLWorldRenderBlendMode::None;
    LLWorldRenderDepthMode mDepthMode = LLWorldRenderDepthMode::ReadWrite;
    LLWorldRenderCullMode mCullMode = LLWorldRenderCullMode::Back;
    U32 mSourcePass = 0;
    U32 mAttributeMask = 0;

    LLPointer<LLVertexBuffer> mVertexBuffer;
    LLPointer<LLViewerTexture> mTexture;
    std::vector<LLPointer<LLViewerTexture> > mTextureList;

    const LLMatrix4* mModelMatrix = nullptr;
    const LLMatrix4* mTextureMatrix = nullptr;

    U32 mStart = 0;
    U32 mEnd = 0;
    U32 mCount = 0;
    U32 mOffset = 0;

    F32 mAlphaMaskCutoff = 0.5f;
    F32 mTerrainDetailScale = 1.f;
    F32 mTerrainOffsetX = 0.f;
    F32 mTerrainOffsetY = 0.f;
    bool mUseTexture = true;
    bool mBatchTextures = false;
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
        U32 attribute_mask);

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
        U32 attribute_mask);

    void appendRenderMap(
        U32 source_pass,
        LLWorldRenderMaterialClass material_class,
        bool texture,
        bool batch_textures,
        U32 attribute_mask);

    const command_list_t& commands() const { return mCommands; }

private:
    command_list_t mCommands;
};

bool use_vulkan_world_command_path();
void submit_vulkan_world_commands(const LLWorldRenderCommandBuffer& command_buffer);

#endif
