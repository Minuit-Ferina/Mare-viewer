/**
 * @file llrenderbackend.h
 * @brief Backend-neutral rendering intent interface.
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

#ifndef LL_LLRENDERBACKEND_H
#define LL_LLRENDERBACKEND_H

#include "stdtypes.h"

using LLRenderDebugMessageCallback = void (*)();

enum class LLRenderBackendType : U8
{
    Unknown,
    OpenGL,
    Vulkan,
    Metal,
    Null,
};

enum class LLRenderLoadAction : U8
{
    Load,
    Clear,
    DontCare,
};

enum class LLRenderStoreAction : U8
{
    Store,
    DontCare,
};

enum class LLRenderBlendFactor : U8
{
    One,
    Zero,
    DestinationColor,
    SourceColor,
    OneMinusDestinationColor,
    OneMinusSourceColor,
    DestinationAlpha,
    SourceAlpha,
    OneMinusDestinationAlpha,
    OneMinusSourceAlpha,
};

enum class LLRenderCapability : U8
{
    DebugOutputSynchronous,
    DepthTest,
    LineSmooth,
    Multisample,
    TextureCubeMapSeamless,
};

enum class LLRenderCullFace : U8
{
    Front,
    Back,
    FrontAndBack,
};

enum class LLRenderDepthFunction : U8
{
    Always,
    Less,
    LessEqual,
    Equal,
    NotEqual,
    GreaterEqual,
    Greater,
};

enum class LLRenderTextureTarget : U8
{
    Texture2D,
    TextureRectangle,
    TextureCubeMap,
    TextureCubeMapArray,
    Texture2DMultisample,
    Texture3D,
};

enum class LLRenderTextureAddressMode : U8
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
};

enum class LLRenderTextureFilter : U8
{
    Nearest,
    Linear,
    LinearMipmapLinear,
    LinearMipmapNearest,
    NearestMipmapNearest,
};

enum class LLRenderPixelStoreParameter : U8
{
    PackAlignment,
    UnpackAlignment,
    UnpackSwapBytes,
    UnpackRowLength,
};

enum class LLRenderBufferTarget : U8
{
    Vertex,
    Index,
    PixelPack,
    PixelUnpack,
    Uniform,
};

enum class LLRenderBufferUsage : U8
{
    StaticDraw,
    DynamicDraw,
    StreamCopy,
};

enum class LLRenderPrimitiveType : U8
{
    Triangles,
    TriangleStrip,
    TriangleFan,
    Points,
    Lines,
    LineStrip,
    LineLoop,
};

enum class LLRenderIndexType : U8
{
    UnsignedShort,
    UnsignedInt,
};

enum class LLRenderVertexAttributeType : U8
{
    Float32,
    UnsignedByte,
    UnsignedShort,
    UnsignedInt,
};

enum class LLRenderFramebufferAttachment : U8
{
    Color0,
    Color1,
    Color2,
    Color3,
    Depth,
};

enum LLRenderClearMask : U32
{
    LL_RENDER_CLEAR_NONE = 0,
    LL_RENDER_CLEAR_COLOR = 1 << 0,
    LL_RENDER_CLEAR_DEPTH = 1 << 1,
    LL_RENDER_CLEAR_STENCIL = 1 << 2,
};

struct LLRenderExtent2D
{
    S32 mWidth = 0;
    S32 mHeight = 0;
};

struct LLRenderFloatRange
{
    F32 mMinimum = 0.f;
    F32 mMaximum = 0.f;
};

struct LLRenderViewport
{
    F32 mX = 0.f;
    F32 mY = 0.f;
    F32 mWidth = 0.f;
    F32 mHeight = 0.f;
    F32 mMinDepth = 0.f;
    F32 mMaxDepth = 1.f;
};

struct LLRenderScissor
{
    S32 mX = 0;
    S32 mY = 0;
    S32 mWidth = 0;
    S32 mHeight = 0;
    bool mEnabled = false;
};

struct LLRenderClearColor
{
    F32 mRed = 0.f;
    F32 mGreen = 0.f;
    F32 mBlue = 0.f;
    F32 mAlpha = 0.f;
};

struct LLRenderColorMask
{
    bool mRed = true;
    bool mGreen = true;
    bool mBlue = true;
    bool mAlpha = true;
};

struct LLRenderBlendState
{
    LLRenderBlendFactor mColorSource = LLRenderBlendFactor::One;
    LLRenderBlendFactor mColorDestination = LLRenderBlendFactor::Zero;
    LLRenderBlendFactor mAlphaSource = LLRenderBlendFactor::One;
    LLRenderBlendFactor mAlphaDestination = LLRenderBlendFactor::Zero;
};

struct LLRenderTargetDesc
{
    const char* mDebugName = nullptr;
    LLRenderExtent2D mExtent;
    U32 mSampleCount = 1;
};

struct LLRenderPassDesc
{
    const char* mDebugName = nullptr;
    LLRenderTargetDesc mTarget;
    LLRenderViewport mViewport;
    LLRenderScissor mScissor;
    U32 mClearMask = LL_RENDER_CLEAR_NONE;
    LLRenderClearColor mClearColor;
    F32 mClearDepth = 1.f;
    S32 mClearStencil = 0;
    LLRenderLoadAction mColorLoadAction = LLRenderLoadAction::Load;
    LLRenderStoreAction mColorStoreAction = LLRenderStoreAction::Store;
    LLRenderLoadAction mDepthLoadAction = LLRenderLoadAction::Load;
    LLRenderStoreAction mDepthStoreAction = LLRenderStoreAction::Store;
};

struct LLRenderFrameDesc
{
    const char* mDebugName = nullptr;
    LLRenderExtent2D mDrawableExtent;
};

class LLRenderBackend
{
public:
    virtual ~LLRenderBackend();

    virtual LLRenderBackendType getType() const = 0;
    virtual const char* getName() const = 0;
    virtual bool isReady() const = 0;

    virtual void beginFrame(const LLRenderFrameDesc& desc) = 0;
    virtual void endFrame() = 0;

    virtual void beginRenderPass(const LLRenderPassDesc& desc) = 0;
    virtual void endRenderPass() = 0;

    virtual void setViewport(const LLRenderViewport& viewport) = 0;
    virtual void setScissor(const LLRenderScissor& scissor) = 0;
    virtual void clear(const LLRenderPassDesc& desc) = 0;
    virtual void setClearColor(const LLRenderClearColor& color) = 0;
    virtual void setColorMask(const LLRenderColorMask& mask) = 0;
    virtual void setBlendState(const LLRenderBlendState& blend) = 0;
    virtual void setLineWidth(F32 width) = 0;
    virtual void setCapability(LLRenderCapability capability, bool enabled) = 0;
    virtual bool isCapabilityEnabled(LLRenderCapability capability) const = 0;
    virtual void setCullFace(LLRenderCullFace face) = 0;
    virtual void setDepthFunction(LLRenderDepthFunction function) = 0;
    virtual void setDepthWriteEnabled(bool enabled) = 0;
    virtual LLRenderFloatRange getLineWidthRange(bool smooth) const = 0;
    virtual void setPixelStoreInteger(LLRenderPixelStoreParameter parameter, S32 value) = 0;
    virtual S32 getActiveTextureUnit() const = 0;
    virtual void setActiveTextureUnit(S32 unit) = 0;
    virtual void bindTexture(LLRenderTextureTarget target, U32 texture) = 0;
    virtual void setTextureAddressMode(
        LLRenderTextureTarget target,
        LLRenderTextureAddressMode mode) = 0;
    virtual void setTextureFilter(
        LLRenderTextureTarget target,
        LLRenderTextureFilter min_filter,
        LLRenderTextureFilter mag_filter) = 0;
    virtual void setTextureMaxAnisotropy(LLRenderTextureTarget target, F32 anisotropy) = 0;
    virtual void generateMipmaps(LLRenderTextureTarget target) = 0;
    virtual void generateTextures(S32 count, U32* textures) = 0;
    virtual void deleteTextures(S32 count, const U32* textures) = 0;
    virtual void generateBuffers(S32 count, U32* buffers) = 0;
    virtual void deleteBuffers(S32 count, const U32* buffers) = 0;
    virtual void bindBuffer(LLRenderBufferTarget target, U32 buffer) = 0;
    virtual void allocateBufferStorage(
        LLRenderBufferTarget target,
        U64 size,
        const void* data,
        LLRenderBufferUsage usage) = 0;
    virtual void updateBufferSubData(
        LLRenderBufferTarget target,
        U32 offset,
        U32 size,
        const void* data) = 0;
    virtual void enableVertexAttributeArray(U32 location) = 0;
    virtual void disableVertexAttributeArray(U32 location) = 0;
    virtual void setVertexAttributePointer(
        U32 location,
        S32 size,
        LLRenderVertexAttributeType type,
        bool normalized,
        S32 stride,
        const void* pointer) = 0;
    virtual void setIntegerVertexAttributePointer(
        U32 location,
        S32 size,
        LLRenderVertexAttributeType type,
        S32 stride,
        const void* pointer) = 0;
    virtual void drawIndexedRange(
        LLRenderPrimitiveType mode,
        U32 start,
        U32 end,
        S32 count,
        LLRenderIndexType index_type,
        const void* indices) = 0;
    virtual void drawArrays(LLRenderPrimitiveType mode, S32 first, S32 count) = 0;
    virtual void generateFramebuffers(S32 count, U32* framebuffers) = 0;
    virtual void deleteFramebuffers(S32 count, const U32* framebuffers) = 0;
    virtual void bindReadWriteFramebuffer(U32 framebuffer) = 0;
    virtual bool isDrawFramebufferComplete() const = 0;
    virtual void attachFramebufferTexture2D(
        LLRenderFramebufferAttachment attachment,
        LLRenderTextureTarget target,
        U32 texture,
        S32 mip_level) = 0;
    virtual void setFramebufferBufferRouting(U32 color_attachment_count) = 0;
    virtual void restoreDefaultFramebufferBufferRouting() = 0;
    virtual bool hasError() = 0;
    virtual void setDebugMessageCallback(LLRenderDebugMessageCallback callback, void* user_param) = 0;
    virtual bool hasVertexArraySupport() const = 0;
    virtual void generateVertexArrays(S32 count, U32* arrays) = 0;
    virtual void bindVertexArray(U32 array) = 0;
};

const char* getRenderBackendTypeName(LLRenderBackendType type);
LLRenderBackend& getNullRenderBackend();
LLRenderBackend& getOpenGLRenderBackend();

#endif
