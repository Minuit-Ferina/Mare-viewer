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

#include <vector>

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
    AlphaTest,
    Blend,
    ClipPlane0,
    CullFace,
    DebugOutputSynchronous,
    DepthTest,
    DepthClamp,
    LineSmooth,
    Multisample,
    PolygonOffsetFill,
    PolygonOffsetLine,
    ScissorTest,
    StencilTest,
    TextureGenS,
    TextureGenT,
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
    TextureCubeMapPositiveX,
    TextureCubeMapNegativeX,
    TextureCubeMapPositiveY,
    TextureCubeMapNegativeY,
    TextureCubeMapPositiveZ,
    TextureCubeMapNegativeZ,
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

enum class LLRenderTextureLevelParameter : U8
{
    Width,
    Height,
    Compressed,
    CompressedImageSize,
};

enum class LLRenderTextureParameter : U8
{
    GenerateMipmap,
    BaseLevel,
    MaxLevel,
    SwizzleRGBA,
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
    StreamDraw,
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

enum class LLRenderFramebufferBindPoint : U8
{
    Read,
    Draw,
    ReadWrite,
};

enum class LLRenderFramebufferStatus : U8
{
    Complete,
    IncompleteMissingAttachment,
    IncompleteAttachment,
    Unsupported,
    Unknown,
};

enum class LLRenderQueryTarget : U8
{
    AnySamplesPassed,
    TimeElapsed,
    SamplesPassed,
    PrimitivesGenerated,
    TransformFeedbackPrimitivesWritten,
};

enum class LLRenderQueryParameter : U8
{
    ResultAvailable,
    Result,
};

enum class LLRenderShaderStage : U8
{
    Vertex,
    Fragment,
    Geometry,
    Compute,
};

enum class LLRenderShaderParameter : U8
{
    InfoLogLength,
    CompileStatus,
};

enum class LLRenderProgramParameter : U8
{
    InfoLogLength,
    LinkStatus,
    ActiveUniforms,
    BinaryLength,
};

enum class LLRenderProgramSetting : U8
{
    BinaryRetrievableHint,
};

enum class LLRenderTextureFormat : U8
{
    None,
    Alpha,
    Alpha8,
    R8,
    R16F,
    R32F,
    RG8,
    RG16F,
    RG32F,
    RGB,
    RGB8,
    RGB16F,
    RGB10A2,
    R11G11B10F,
    RGBA,
    RGBA8,
    RGBA16,
    RGBA16F,
    DepthComponent,
    DepthComponent24,
    Luminance,
};

enum class LLRenderPixelFormat : U8
{
    Alpha,
    DepthComponent,
    Luminance,
    Red,
    RG,
    RGB,
    RGBA,
};

enum class LLRenderPixelType : U8
{
    UnsignedByte,
    UnsignedShort,
    UnsignedInt,
    Float32,
};

enum class LLRenderImageAccess : U8
{
    WriteOnly,
};

enum class LLRenderPolygonFace : U8
{
    FrontAndBack,
};

enum class LLRenderPolygonMode : U8
{
    Fill,
    Line,
};

enum class LLRenderStencilFunction : U8
{
    Always,
    Equal,
};

enum class LLRenderStencilOperation : U8
{
    Keep,
    Replace,
};

enum class LLRenderMatrixMode : U8
{
    ModelView,
};

enum class LLRenderTextureCoordinate : U8
{
    S,
    T,
};

enum class LLRenderInfoString : U8
{
    Vendor,
    Renderer,
    Version,
};

enum class LLRenderIntegerParameter : U8
{
    DedicatedVideoMemoryKB,
    FreeVideoMemoryKB,
    RedBits,
    GreenBits,
    BlueBits,
    AlphaBits,
    DepthBits,
    StencilBits,
};

enum LLRenderMemoryBarrierMask : U32
{
    LL_RENDER_MEMORY_BARRIER_NONE = 0,
    LL_RENDER_MEMORY_BARRIER_SHADER_IMAGE_ACCESS = 1 << 0,
    LL_RENDER_MEMORY_BARRIER_TEXTURE_FETCH = 1 << 1,
};

inline LLRenderMemoryBarrierMask operator|(LLRenderMemoryBarrierMask lhs, LLRenderMemoryBarrierMask rhs)
{
    return static_cast<LLRenderMemoryBarrierMask>(static_cast<U32>(lhs) | static_cast<U32>(rhs));
}

inline LLRenderMemoryBarrierMask operator&(LLRenderMemoryBarrierMask lhs, LLRenderMemoryBarrierMask rhs)
{
    return static_cast<LLRenderMemoryBarrierMask>(static_cast<U32>(lhs) & static_cast<U32>(rhs));
}

inline LLRenderMemoryBarrierMask& operator|=(LLRenderMemoryBarrierMask& lhs, LLRenderMemoryBarrierMask rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

enum LLRenderClearMask : U32
{
    LL_RENDER_CLEAR_NONE = 0,
    LL_RENDER_CLEAR_COLOR = 1 << 0,
    LL_RENDER_CLEAR_DEPTH = 1 << 1,
    LL_RENDER_CLEAR_STENCIL = 1 << 2,
    LL_RENDER_CLEAR_ALL = LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH | LL_RENDER_CLEAR_STENCIL,
};

inline LLRenderClearMask operator|(LLRenderClearMask lhs, LLRenderClearMask rhs)
{
    return static_cast<LLRenderClearMask>(static_cast<U32>(lhs) | static_cast<U32>(rhs));
}

inline LLRenderClearMask operator&(LLRenderClearMask lhs, LLRenderClearMask rhs)
{
    return static_cast<LLRenderClearMask>(static_cast<U32>(lhs) & static_cast<U32>(rhs));
}

inline LLRenderClearMask& operator|=(LLRenderClearMask& lhs, LLRenderClearMask rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

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

struct LLRenderTextureHandle
{
    U32 mValue = 0;

    LLRenderTextureHandle() = default;
    explicit LLRenderTextureHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderTextureHandle lhs, LLRenderTextureHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderTextureHandle lhs, LLRenderTextureHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderFramebufferHandle
{
    U32 mValue = 0;

    LLRenderFramebufferHandle() = default;
    explicit LLRenderFramebufferHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderFramebufferHandle lhs, LLRenderFramebufferHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderFramebufferHandle lhs, LLRenderFramebufferHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderBufferHandle
{
    U32 mValue = 0;

    LLRenderBufferHandle() = default;
    explicit LLRenderBufferHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderBufferHandle lhs, LLRenderBufferHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderBufferHandle lhs, LLRenderBufferHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderProgramHandle
{
    U32 mValue = 0;

    LLRenderProgramHandle() = default;
    explicit LLRenderProgramHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderProgramHandle lhs, LLRenderProgramHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderProgramHandle lhs, LLRenderProgramHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderShaderHandle
{
    U32 mValue = 0;

    LLRenderShaderHandle() = default;
    explicit LLRenderShaderHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderShaderHandle lhs, LLRenderShaderHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderShaderHandle lhs, LLRenderShaderHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderQueryHandle
{
    U32 mValue = 0;

    LLRenderQueryHandle() = default;
    explicit LLRenderQueryHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderQueryHandle lhs, LLRenderQueryHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderQueryHandle lhs, LLRenderQueryHandle rhs)
    {
        return !(lhs == rhs);
    }

    friend bool operator<(LLRenderQueryHandle lhs, LLRenderQueryHandle rhs)
    {
        return lhs.mValue < rhs.mValue;
    }
};

struct LLRenderVertexArrayHandle
{
    U32 mValue = 0;

    LLRenderVertexArrayHandle() = default;
    explicit LLRenderVertexArrayHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderVertexArrayHandle lhs, LLRenderVertexArrayHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderVertexArrayHandle lhs, LLRenderVertexArrayHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderPassDesc
{
    const char* mDebugName = nullptr;
    LLRenderTargetDesc mTarget;
    LLRenderViewport mViewport;
    LLRenderScissor mScissor;
    LLRenderClearMask mClearMask = LL_RENDER_CLEAR_NONE;
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

struct LLRenderNativeContextDesc
{
    void* mWindow = nullptr;
    U32 mSamples = 0;
    bool mEnableVSync = true;
};

struct LLRenderNativeContext
{
    void* mView = nullptr;
    void* mContext = nullptr;
    void* mPixelFormat = nullptr;
    U32 mVRAM = 0;
};

class LLRenderBackend
{
public:
    virtual ~LLRenderBackend();

    virtual LLRenderBackendType getType() const = 0;
    virtual const char* getName() const = 0;
    virtual bool isReady() const = 0;
    virtual void initPlatformContextExtensions() = 0;
    virtual bool initContextCapabilities() = 0;
    virtual void shutdownContextCapabilities() = 0;
    virtual bool createNativeContext(
        const LLRenderNativeContextDesc& desc,
        LLRenderNativeContext& context) = 0;
    virtual void destroyNativeContext(LLRenderNativeContext& context) = 0;
    virtual bool makeNativeContextCurrent(void* context) = 0;
    virtual void clearCurrentNativeContext() = 0;
    virtual void swapNativeBuffers(void* context) = 0;
    virtual void setNativeVSync(void* context, bool enable_vsync) = 0;
    virtual bool setNativeContextThreadedOptimization(bool enabled) = 0;
    virtual void* createSharedNativeContext(
        void* pixel_format,
        void* share_context,
        bool enable_threaded_optimization) = 0;
    virtual void destroySharedNativeContext(void* context) = 0;

    virtual void beginFrame(const LLRenderFrameDesc& desc) = 0;
    virtual void endFrame() = 0;

    virtual void beginRenderPass(const LLRenderPassDesc& desc) = 0;
    virtual void endRenderPass() = 0;

    virtual void setViewport(const LLRenderViewport& viewport) = 0;
    virtual void setViewport(S32 x, S32 y, S32 width, S32 height) = 0;
    virtual void setScissor(const LLRenderScissor& scissor) = 0;
    virtual void setScissor(S32 x, S32 y, S32 width, S32 height) = 0;
    virtual void clear(const LLRenderPassDesc& desc) = 0;
    virtual void clear(LLRenderClearMask clear_mask) = 0;
    virtual void setClearColor(const LLRenderClearColor& color) = 0;
    virtual void setClearColor(F32 red, F32 green, F32 blue, F32 alpha) = 0;
    virtual void setColorMask(const LLRenderColorMask& mask) = 0;
    virtual void setBlendState(const LLRenderBlendState& blend) = 0;
    virtual void setLineWidth(F32 width) = 0;
    virtual void setPointSize(F32 size) = 0;
    virtual F32 getLineWidth() = 0;
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
    virtual void setTextureAddressMode(
        LLRenderTextureTarget target,
        LLRenderTextureCoordinate coordinate,
        LLRenderTextureAddressMode mode) = 0;
    virtual void setTextureFilter(
        LLRenderTextureTarget target,
        LLRenderTextureFilter min_filter,
        LLRenderTextureFilter mag_filter) = 0;
    virtual void setTextureMagFilter(
        LLRenderTextureTarget target,
        LLRenderTextureFilter mag_filter) = 0;
    virtual void setTextureCompareMode(
        LLRenderTextureTarget target,
        bool enabled) = 0;
    virtual void setTextureMaxAnisotropy(LLRenderTextureTarget target, F32 anisotropy) = 0;
    virtual void generateMipmaps(LLRenderTextureTarget target) = 0;
    virtual void generateTextures(S32 count, U32* textures) = 0;
    virtual void deleteTextures(S32 count, const U32* textures) = 0;

    LLRenderTextureHandle createTextureHandle()
    {
        U32 texture = 0;
        generateTextures(1, &texture);
        return LLRenderTextureHandle(texture);
    }

    void deleteTextureHandle(LLRenderTextureHandle texture)
    {
        U32 legacy_name = texture.asLegacyName();
        if (legacy_name)
        {
            deleteTextures(1, &legacy_name);
        }
    }

    void bindTexture(LLRenderTextureTarget target, LLRenderTextureHandle texture)
    {
        bindTexture(target, texture.asLegacyName());
    }

    virtual void generateBuffers(S32 count, U32* buffers) = 0;
    virtual void deleteBuffers(S32 count, const U32* buffers) = 0;
    virtual void bindBuffer(LLRenderBufferTarget target, U32 buffer) = 0;
    virtual void bindBufferBase(LLRenderBufferTarget target, U32 index, U32 buffer) = 0;

    LLRenderBufferHandle createBufferHandle()
    {
        U32 buffer = 0;
        generateBuffers(1, &buffer);
        return LLRenderBufferHandle(buffer);
    }

    void generateBufferHandles(S32 count, LLRenderBufferHandle* buffers)
    {
        if (count <= 0)
        {
            return;
        }

        std::vector<U32> legacy_names(count);
        generateBuffers(count, legacy_names.data());
        for (S32 i = 0; i < count; ++i)
        {
            buffers[i] = LLRenderBufferHandle(legacy_names[i]);
        }
    }

    void deleteBufferHandle(LLRenderBufferHandle buffer)
    {
        U32 legacy_name = buffer.asLegacyName();
        if (legacy_name)
        {
            deleteBuffers(1, &legacy_name);
        }
    }

    void deleteBufferHandles(S32 count, const LLRenderBufferHandle* buffers)
    {
        if (count <= 0)
        {
            return;
        }

        std::vector<U32> legacy_names(count);
        for (S32 i = 0; i < count; ++i)
        {
            legacy_names[i] = buffers[i].asLegacyName();
        }
        deleteBuffers(count, legacy_names.data());
    }

    void bindBuffer(LLRenderBufferTarget target, LLRenderBufferHandle buffer)
    {
        bindBuffer(target, buffer.asLegacyName());
    }

    void bindBufferBase(LLRenderBufferTarget target, U32 index, LLRenderBufferHandle buffer)
    {
        bindBufferBase(target, index, buffer.asLegacyName());
    }

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
    virtual void drawElements(
        LLRenderPrimitiveType mode,
        S32 count,
        LLRenderIndexType index_type,
        const void* indices) = 0;
    virtual void setLegacyVertexPointer(
        S32 size,
        LLRenderVertexAttributeType type,
        S32 stride,
        const void* pointer) = 0;
    virtual void setLegacyTextureCoordinatePointer(
        S32 size,
        LLRenderVertexAttributeType type,
        S32 stride,
        const void* pointer) = 0;
    virtual void setLegacyTextureCoordinateArray(bool enabled) = 0;
    virtual void generateFramebuffers(S32 count, U32* framebuffers) = 0;
    virtual void deleteFramebuffers(S32 count, const U32* framebuffers) = 0;
    virtual void bindFramebuffer(LLRenderFramebufferBindPoint target, U32 framebuffer) = 0;
    virtual void bindReadWriteFramebuffer(U32 framebuffer) = 0;

    LLRenderFramebufferHandle createFramebufferHandle()
    {
        U32 framebuffer = 0;
        generateFramebuffers(1, &framebuffer);
        return LLRenderFramebufferHandle(framebuffer);
    }

    void deleteFramebufferHandle(LLRenderFramebufferHandle framebuffer)
    {
        U32 legacy_name = framebuffer.asLegacyName();
        if (legacy_name)
        {
            deleteFramebuffers(1, &legacy_name);
        }
    }

    void bindFramebuffer(
        LLRenderFramebufferBindPoint target,
        LLRenderFramebufferHandle framebuffer)
    {
        bindFramebuffer(target, framebuffer.asLegacyName());
    }

    void bindReadWriteFramebuffer(LLRenderFramebufferHandle framebuffer)
    {
        bindReadWriteFramebuffer(framebuffer.asLegacyName());
    }

    virtual LLRenderFramebufferStatus getReadWriteFramebufferStatus() const = 0;
    virtual bool isDrawFramebufferComplete() const = 0;
    virtual void attachFramebufferTexture2D(
        LLRenderFramebufferAttachment attachment,
        LLRenderTextureTarget target,
        U32 texture,
        S32 mip_level) = 0;

    void attachFramebufferTexture2D(
        LLRenderFramebufferAttachment attachment,
        LLRenderTextureTarget target,
        LLRenderTextureHandle texture,
        S32 mip_level)
    {
        attachFramebufferTexture2D(attachment, target, texture.asLegacyName(), mip_level);
    }

    virtual void setFramebufferBufferRouting(U32 color_attachment_count) = 0;
    virtual void restoreDefaultFramebufferBufferRouting() = 0;
    virtual bool hasError() = 0;
    virtual U32 getErrorCode() = 0;
    virtual void setDebugMessageCallback(LLRenderDebugMessageCallback callback, void* user_param) = 0;
    virtual bool hasVertexArraySupport() const = 0;
    virtual void generateVertexArrays(S32 count, U32* arrays) = 0;
    virtual void bindVertexArray(U32 array) = 0;

    LLRenderVertexArrayHandle createVertexArrayHandle()
    {
        U32 array = 0;
        generateVertexArrays(1, &array);
        return LLRenderVertexArrayHandle(array);
    }

    void bindVertexArray(LLRenderVertexArrayHandle array)
    {
        bindVertexArray(array.asLegacyName());
    }

    virtual void generateQueries(S32 count, U32* queries) = 0;
    virtual void deleteQueries(S32 count, const U32* queries) = 0;
    virtual void beginQuery(LLRenderQueryTarget target, U32 query) = 0;
    virtual void endQuery(LLRenderQueryTarget target) = 0;
    virtual void getQueryObjectUnsignedInteger64(
        U32 query,
        LLRenderQueryParameter parameter,
        U64* value) = 0;
    virtual void getQueryObjectUnsignedInteger(
        U32 query,
        LLRenderQueryParameter parameter,
        U32* value) = 0;

    LLRenderQueryHandle createQueryHandle()
    {
        U32 query = 0;
        generateQueries(1, &query);
        return LLRenderQueryHandle(query);
    }

    void generateQueryHandles(S32 count, LLRenderQueryHandle* queries)
    {
        if (count <= 0)
        {
            return;
        }

        std::vector<U32> legacy_names(count);
        generateQueries(count, legacy_names.data());
        for (S32 i = 0; i < count; ++i)
        {
            queries[i] = LLRenderQueryHandle(legacy_names[i]);
        }
    }

    void deleteQueryHandle(LLRenderQueryHandle query)
    {
        U32 legacy_name = query.asLegacyName();
        if (legacy_name)
        {
            deleteQueries(1, &legacy_name);
        }
    }

    void beginQuery(LLRenderQueryTarget target, LLRenderQueryHandle query)
    {
        beginQuery(target, query.asLegacyName());
    }

    void getQueryObjectUnsignedInteger64(
        LLRenderQueryHandle query,
        LLRenderQueryParameter parameter,
        U64* value)
    {
        getQueryObjectUnsignedInteger64(query.asLegacyName(), parameter, value);
    }

    void getQueryObjectUnsignedInteger(
        LLRenderQueryHandle query,
        LLRenderQueryParameter parameter,
        U32* value)
    {
        getQueryObjectUnsignedInteger(query.asLegacyName(), parameter, value);
    }

    virtual U32 createProgram() = 0;
    virtual void deleteProgram(U32 program) = 0;
    virtual U32 createShader(LLRenderShaderStage stage) = 0;
    virtual void deleteShader(U32 shader) = 0;
    virtual bool isShader(U32 shader) const = 0;
    virtual bool isProgram(U32 program) const = 0;
    virtual void attachShader(U32 program, U32 shader) = 0;
    virtual void detachShader(U32 program, U32 shader) = 0;
    virtual void getAttachedShaders(U32 program, S32 max_count, S32* count, U32* shaders) = 0;
    virtual void setShaderSource(U32 shader, S32 count, const char* const* strings) = 0;
    virtual void compileShader(U32 shader) = 0;
    virtual void linkProgram(U32 program) = 0;
    virtual void validateProgram(U32 program) = 0;
    virtual void useProgram(U32 program) = 0;
    virtual void getShaderInteger(U32 shader, LLRenderShaderParameter parameter, S32* value) = 0;
    virtual void getProgramInteger(U32 program, LLRenderProgramParameter parameter, S32* value) = 0;
    virtual void getShaderInfoLog(U32 shader, S32 buffer_size, S32* length, char* info_log) = 0;
    virtual void getProgramInfoLog(U32 program, S32 buffer_size, S32* length, char* info_log) = 0;
    virtual void setProgramParameterInteger(U32 program, LLRenderProgramSetting parameter, S32 value) = 0;
    virtual void setProgramBinary(U32 program, U32 binary_format, const void* binary, S32 length) = 0;
    virtual void getProgramBinary(
        U32 program,
        S32 buffer_size,
        S32* length,
        U32* binary_format,
        void* binary) = 0;
    virtual S32 getUniformLocation(U32 program, const char* name) = 0;
    virtual S32 getAttributeLocation(U32 program, const char* name) = 0;
    virtual void bindAttributeLocation(U32 program, U32 index, const char* name) = 0;
    virtual void getActiveUniform(
        U32 program,
        U32 index,
        S32 buffer_size,
        S32* length,
        S32* size,
        U32* type,
        char* name) = 0;
    virtual U32 getUniformBlockIndex(U32 program, const char* name) = 0;
    virtual void bindUniformBlock(U32 program, U32 block_index, U32 binding) = 0;

    LLRenderProgramHandle createProgramHandle()
    {
        return LLRenderProgramHandle(createProgram());
    }

    void deleteProgram(LLRenderProgramHandle program)
    {
        if (program)
        {
            deleteProgram(program.asLegacyName());
        }
    }

    LLRenderShaderHandle createShaderHandle(LLRenderShaderStage stage)
    {
        return LLRenderShaderHandle(createShader(stage));
    }

    void deleteShader(LLRenderShaderHandle shader)
    {
        if (shader)
        {
            deleteShader(shader.asLegacyName());
        }
    }

    bool isShader(LLRenderShaderHandle shader) const
    {
        return isShader(shader.asLegacyName());
    }

    bool isProgram(LLRenderProgramHandle program) const
    {
        return isProgram(program.asLegacyName());
    }

    void attachShader(LLRenderProgramHandle program, LLRenderShaderHandle shader)
    {
        attachShader(program.asLegacyName(), shader.asLegacyName());
    }

    void attachShader(U32 program, LLRenderShaderHandle shader)
    {
        attachShader(program, shader.asLegacyName());
    }

    void detachShader(LLRenderProgramHandle program, LLRenderShaderHandle shader)
    {
        detachShader(program.asLegacyName(), shader.asLegacyName());
    }

    void detachShader(U32 program, LLRenderShaderHandle shader)
    {
        detachShader(program, shader.asLegacyName());
    }

    void getAttachedShaders(
        LLRenderProgramHandle program,
        S32 max_count,
        S32* count,
        LLRenderShaderHandle* shaders)
    {
        std::vector<U32> legacy_names(max_count);
        getAttachedShaders(program.asLegacyName(), max_count, count, legacy_names.data());
        S32 shader_count = count ? *count : max_count;
        for (S32 i = 0; i < shader_count; ++i)
        {
            shaders[i] = LLRenderShaderHandle(legacy_names[i]);
        }
    }

    void setShaderSource(LLRenderShaderHandle shader, S32 count, const char* const* strings)
    {
        setShaderSource(shader.asLegacyName(), count, strings);
    }

    void compileShader(LLRenderShaderHandle shader)
    {
        compileShader(shader.asLegacyName());
    }

    void linkProgram(LLRenderProgramHandle program)
    {
        linkProgram(program.asLegacyName());
    }

    void validateProgram(LLRenderProgramHandle program)
    {
        validateProgram(program.asLegacyName());
    }

    void useProgram(LLRenderProgramHandle program)
    {
        useProgram(program.asLegacyName());
    }

    void getShaderInteger(LLRenderShaderHandle shader, LLRenderShaderParameter parameter, S32* value)
    {
        getShaderInteger(shader.asLegacyName(), parameter, value);
    }

    void getProgramInteger(LLRenderProgramHandle program, LLRenderProgramParameter parameter, S32* value)
    {
        getProgramInteger(program.asLegacyName(), parameter, value);
    }

    void getShaderInfoLog(LLRenderShaderHandle shader, S32 buffer_size, S32* length, char* info_log)
    {
        getShaderInfoLog(shader.asLegacyName(), buffer_size, length, info_log);
    }

    void getProgramInfoLog(LLRenderProgramHandle program, S32 buffer_size, S32* length, char* info_log)
    {
        getProgramInfoLog(program.asLegacyName(), buffer_size, length, info_log);
    }

    S32 getUniformLocation(LLRenderProgramHandle program, const char* name)
    {
        return getUniformLocation(program.asLegacyName(), name);
    }

    S32 getAttributeLocation(LLRenderProgramHandle program, const char* name)
    {
        return getAttributeLocation(program.asLegacyName(), name);
    }

    void bindAttributeLocation(LLRenderProgramHandle program, U32 index, const char* name)
    {
        bindAttributeLocation(program.asLegacyName(), index, name);
    }

    void getActiveUniform(
        LLRenderProgramHandle program,
        U32 index,
        S32 buffer_size,
        S32* length,
        S32* size,
        U32* type,
        char* name)
    {
        getActiveUniform(program.asLegacyName(), index, buffer_size, length, size, type, name);
    }

    U32 getUniformBlockIndex(LLRenderProgramHandle program, const char* name)
    {
        return getUniformBlockIndex(program.asLegacyName(), name);
    }

    void bindUniformBlock(LLRenderProgramHandle program, U32 block_index, U32 binding)
    {
        bindUniformBlock(program.asLegacyName(), block_index, binding);
    }

    void setProgramParameterInteger(
        LLRenderProgramHandle program,
        LLRenderProgramSetting parameter,
        S32 value)
    {
        setProgramParameterInteger(program.asLegacyName(), parameter, value);
    }

    void setProgramBinary(
        LLRenderProgramHandle program,
        U32 binary_format,
        const void* binary,
        S32 length)
    {
        setProgramBinary(program.asLegacyName(), binary_format, binary, length);
    }

    void getProgramBinary(
        LLRenderProgramHandle program,
        S32 buffer_size,
        S32* length,
        U32* binary_format,
        void* binary)
    {
        getProgramBinary(program.asLegacyName(), buffer_size, length, binary_format, binary);
    }

    virtual void setUniformInteger(S32 location, S32 value) = 0;
    virtual void setUniformInteger2(S32 location, S32 first, S32 second) = 0;
    virtual void setUniformIntegerVector(S32 location, S32 count, const S32* values) = 0;
    virtual void setUniformIntegerVector4(S32 location, S32 count, const S32* values) = 0;
    virtual void setUniformUnsignedIntegerVector4(S32 location, S32 count, const U32* values) = 0;
    virtual void setUniformFloat(S32 location, F32 value) = 0;
    virtual void setUniformFloat2(S32 location, F32 first, F32 second) = 0;
    virtual void setUniformFloat3(S32 location, F32 first, F32 second, F32 third) = 0;
    virtual void setUniformFloat4(S32 location, F32 first, F32 second, F32 third, F32 fourth) = 0;
    virtual void setUniformFloatVector(S32 location, S32 count, const F32* values) = 0;
    virtual void setUniformFloatVector2(S32 location, S32 count, const F32* values) = 0;
    virtual void setUniformFloatVector3(S32 location, S32 count, const F32* values) = 0;
    virtual void setUniformFloatVector4(S32 location, S32 count, const F32* values) = 0;
    virtual void setUniformMatrix2(S32 location, S32 count, bool transpose, const F32* values) = 0;
    virtual void setUniformMatrix3(S32 location, S32 count, bool transpose, const F32* values) = 0;
    virtual void setUniformMatrix3x4(S32 location, S32 count, bool transpose, const F32* values) = 0;
    virtual void setUniformMatrix4(S32 location, S32 count, bool transpose, const F32* values) = 0;
    virtual void setVertexAttribute4(U32 location, F32 first, F32 second, F32 third, F32 fourth) = 0;
    virtual void setVertexAttributeVector4(U32 location, const F32* values) = 0;
    virtual void bindTextureUnit(U32 unit, LLRenderTextureHandle texture) = 0;
    virtual void bindImageTexture(
        U32 unit,
        LLRenderTextureHandle texture,
        S32 level,
        bool layered,
        S32 layer,
        LLRenderImageAccess access,
        LLRenderTextureFormat format) = 0;
    virtual void dispatchCompute(U32 groups_x, U32 groups_y, U32 groups_z) = 0;
    virtual void setMemoryBarrier(LLRenderMemoryBarrierMask barriers) = 0;
    virtual void pushLegacyAllAttributes() = 0;
    virtual void pushLegacyAllClientAttributes() = 0;
    virtual void popLegacyClientAttributes() = 0;
    virtual void popLegacyAttributes() = 0;
    virtual void copyTextureImage2D(
        LLRenderTextureTarget target,
        S32 level,
        U32 internal_format,
        S32 x,
        S32 y,
        S32 width,
        S32 height,
        S32 border) = 0;
    virtual void setTextureImage2D(
        LLRenderTextureTarget target,
        S32 level,
        S32 internal_format,
        S32 width,
        S32 height,
        S32 border,
        U32 format,
        U32 type,
        const void* data) = 0;
    virtual void setTextureImage2D(
        LLRenderTextureTarget target,
        S32 level,
        LLRenderTextureFormat internal_format,
        S32 width,
        S32 height,
        S32 border,
        LLRenderPixelFormat format,
        LLRenderPixelType type,
        const void* data) = 0;
    virtual LLRenderTextureHandle createTexture2D(LLRenderTextureFormat internal_format, S32 width, S32 height) = 0;
    virtual void readPixels(
        S32 x,
        S32 y,
        S32 width,
        S32 height,
        LLRenderPixelFormat format,
        LLRenderPixelType type,
        void* pixels) = 0;
    virtual void readTextureImage(
        LLRenderTextureTarget target,
        S32 level,
        U32 format,
        U32 type,
        void* pixels) = 0;
    virtual void readTextureImage(
        LLRenderTextureTarget target,
        S32 level,
        LLRenderPixelFormat format,
        LLRenderPixelType type,
        void* pixels) = 0;
    virtual void readCompressedTextureImage(LLRenderTextureTarget target, S32 level, void* pixels) = 0;
    virtual void copyTextureSubImage2D(
        LLRenderTextureTarget target,
        S32 level,
        S32 xoffset,
        S32 yoffset,
        S32 x,
        S32 y,
        S32 width,
        S32 height) = 0;
    virtual void copyTextureSubImage3D(
        LLRenderTextureTarget target,
        S32 level,
        S32 xoffset,
        S32 yoffset,
        S32 zoffset,
        S32 x,
        S32 y,
        S32 width,
        S32 height) = 0;
    virtual void copyImageSubData(
        LLRenderTextureHandle source_texture,
        LLRenderTextureTarget source_target,
        S32 source_level,
        S32 source_x,
        S32 source_y,
        S32 source_z,
        LLRenderTextureHandle destination_texture,
        LLRenderTextureTarget destination_target,
        S32 destination_level,
        S32 destination_x,
        S32 destination_y,
        S32 destination_z,
        S32 width,
        S32 height,
        S32 depth) = 0;
    virtual void setCompressedTextureImage2D(
        LLRenderTextureTarget target,
        S32 level,
        S32 internal_format,
        S32 width,
        S32 height,
        S32 border,
        S32 image_size,
        const void* data) = 0;
    virtual void setTextureSubImage2D(
        LLRenderTextureTarget target,
        S32 level,
        S32 xoffset,
        S32 yoffset,
        S32 width,
        S32 height,
        U32 format,
        U32 type,
        const void* pixels) = 0;
    virtual void setTextureSubImage3D(
        LLRenderTextureTarget target,
        S32 level,
        S32 xoffset,
        S32 yoffset,
        S32 zoffset,
        S32 width,
        S32 height,
        S32 depth,
        U32 format,
        U32 type,
        const void* pixels) = 0;
    virtual void setTextureImage3D(
        LLRenderTextureTarget target,
        S32 level,
        S32 internal_format,
        S32 width,
        S32 height,
        S32 depth,
        S32 border,
        U32 format,
        U32 type,
        const void* data) = 0;
    virtual void getTextureLevelParameterInteger(
        LLRenderTextureTarget target,
        S32 level,
        LLRenderTextureLevelParameter parameter,
        S32* value) = 0;
    virtual void setTextureParameterInteger(
        LLRenderTextureTarget target,
        LLRenderTextureParameter parameter,
        S32 value) = 0;
    virtual void setTextureParameterIntegerVector(
        LLRenderTextureTarget target,
        LLRenderTextureParameter parameter,
        const S32* values) = 0;
    virtual void setTextureGenerationMode(
        LLRenderTextureCoordinate coordinate,
        bool object_linear) = 0;
    virtual void setTextureGenerationObjectPlane(
        LLRenderTextureCoordinate coordinate,
        const F32* values) = 0;
    virtual void areTexturesResident(S32 count, const U32* textures, bool* residences) = 0;
    virtual void getViewport(S32* viewport) = 0;
    virtual U32 getBoundTexture2D() = 0;
    virtual void* createSyncObject() = 0;
    virtual void flushCommands() = 0;
    virtual void finishCommands() = 0;
    virtual void clientWaitSyncObject(void* sync) = 0;
    virtual U32 clientWaitSyncObjectStatus(void* sync, U64 timeout) = 0;
    virtual void waitSyncObject(void* sync) = 0;
    virtual void deleteSyncObject(void* sync) = 0;
    virtual void setLegacyMaterialSpecular(const F32* color, S32 shininess) = 0;
    virtual void getInteger(LLRenderIntegerParameter parameter, S32* value) = 0;
    virtual void getLegacyInteger(U32 parameter, S32* value) = 0;
    virtual void getLegacyBoolean(U32 parameter, U8* value) = 0;
    virtual void getLegacyFloat(U32 parameter, F32* value) = 0;
    virtual void getLegacyBufferObjectParameterInteger(U32 target, U32 parameter, S32* value) = 0;
    virtual const char* getLegacyString(U32 parameter) = 0;
    virtual const char* getLegacyStringIndexed(U32 parameter, U32 index) = 0;
    virtual void setLegacyHint(U32 target, U32 mode) = 0;
    virtual void setClientActiveTextureUnit(S32 unit) = 0;
    virtual void setLegacyCapability(U32 capability, bool enabled) = 0;
    virtual bool isLegacyCapabilityEnabled(U32 capability) = 0;
    virtual void setPolygonOffset(F32 factor, F32 units) = 0;
    virtual void setPolygonMode(LLRenderPolygonFace face, LLRenderPolygonMode mode) = 0;
    virtual void setStencilFunction(LLRenderStencilFunction function, S32 reference, U32 mask) = 0;
    virtual void setStencilMask(U32 mask) = 0;
    virtual void setStencilOperation(
        LLRenderStencilOperation stencil_fail,
        LLRenderStencilOperation depth_fail,
        LLRenderStencilOperation depth_pass) = 0;
    virtual void setMatrixMode(LLRenderMatrixMode mode) = 0;
    virtual void pushMatrix() = 0;
    virtual void popMatrix() = 0;
    virtual const char* getInfoString(LLRenderInfoString parameter) = 0;
};

const char* getRenderBackendTypeName(LLRenderBackendType type);
U32 getOpenGLPixelFormatValue(LLRenderPixelFormat format);
U32 getOpenGLPixelTypeValue(LLRenderPixelType type);
LLRenderBackend& getNullRenderBackend();
LLRenderBackend& getRenderBackend();
LLRenderBackend& getOpenGLRenderBackend();

#endif
