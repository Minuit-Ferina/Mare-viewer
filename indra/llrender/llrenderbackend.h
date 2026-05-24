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

enum class LLRenderQueryTarget : U8
{
    TimeElapsed,
    SamplesPassed,
    PrimitivesGenerated,
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
    virtual U32 getErrorCode() = 0;
    virtual void setDebugMessageCallback(LLRenderDebugMessageCallback callback, void* user_param) = 0;
    virtual bool hasVertexArraySupport() const = 0;
    virtual void generateVertexArrays(S32 count, U32* arrays) = 0;
    virtual void bindVertexArray(U32 array) = 0;
    virtual void generateQueries(S32 count, U32* queries) = 0;
    virtual void deleteQueries(S32 count, const U32* queries) = 0;
    virtual void beginQuery(LLRenderQueryTarget target, U32 query) = 0;
    virtual void endQuery(LLRenderQueryTarget target) = 0;
    virtual void getQueryObjectUnsignedInteger64(
        U32 query,
        LLRenderQueryParameter parameter,
        U64* value) = 0;
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
    virtual void readTextureImage(
        LLRenderTextureTarget target,
        S32 level,
        U32 format,
        U32 type,
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
    virtual void areTexturesResident(S32 count, const U32* textures, bool* residences) = 0;
    virtual void getViewport(S32* viewport) = 0;
    virtual U32 getBoundTexture2D() = 0;
    virtual void* createSyncObject() = 0;
    virtual void flushCommands() = 0;
    virtual void clientWaitSyncObject(void* sync) = 0;
    virtual void waitSyncObject(void* sync) = 0;
    virtual void deleteSyncObject(void* sync) = 0;
    virtual void setLegacyMaterialSpecular(const F32* color, S32 shininess) = 0;
};

const char* getRenderBackendTypeName(LLRenderBackendType type);
LLRenderBackend& getNullRenderBackend();
LLRenderBackend& getOpenGLRenderBackend();

#endif
