/**
 * @file llrenderbackend.cpp
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

#include "linden_common.h"

#include "llrenderbackend.h"

#include "llglcontainment.h"
#include "llglheaders.h"

#include <vector>

namespace
{
LLGLenum to_opengl_blend_factor(LLRenderBlendFactor factor)
{
    switch (factor)
    {
    case LLRenderBlendFactor::One:
        return GL_ONE;
    case LLRenderBlendFactor::Zero:
        return GL_ZERO;
    case LLRenderBlendFactor::DestinationColor:
        return GL_DST_COLOR;
    case LLRenderBlendFactor::SourceColor:
        return GL_SRC_COLOR;
    case LLRenderBlendFactor::OneMinusDestinationColor:
        return GL_ONE_MINUS_DST_COLOR;
    case LLRenderBlendFactor::OneMinusSourceColor:
        return GL_ONE_MINUS_SRC_COLOR;
    case LLRenderBlendFactor::DestinationAlpha:
        return GL_DST_ALPHA;
    case LLRenderBlendFactor::SourceAlpha:
        return GL_SRC_ALPHA;
    case LLRenderBlendFactor::OneMinusDestinationAlpha:
        return GL_ONE_MINUS_DST_ALPHA;
    case LLRenderBlendFactor::OneMinusSourceAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    default:
        return GL_ZERO;
    }
}

LLGLenum to_opengl_capability(LLRenderCapability capability)
{
    switch (capability)
    {
    case LLRenderCapability::DebugOutputSynchronous:
        return GL_DEBUG_OUTPUT_SYNCHRONOUS;
    case LLRenderCapability::DepthTest:
        return GL_DEPTH_TEST;
    case LLRenderCapability::LineSmooth:
        return GL_LINE_SMOOTH;
    case LLRenderCapability::Multisample:
        return GL_MULTISAMPLE;
    case LLRenderCapability::TextureCubeMapSeamless:
        return GL_TEXTURE_CUBE_MAP_SEAMLESS;
    default:
        return 0;
    }
}

LLGLenum to_opengl_cull_face(LLRenderCullFace face)
{
    switch (face)
    {
    case LLRenderCullFace::Front:
        return GL_FRONT;
    case LLRenderCullFace::Back:
        return GL_BACK;
    case LLRenderCullFace::FrontAndBack:
        return GL_FRONT_AND_BACK;
    default:
        return GL_BACK;
    }
}

LLGLenum to_opengl_depth_function(LLRenderDepthFunction function)
{
    switch (function)
    {
    case LLRenderDepthFunction::Always:
        return GL_ALWAYS;
    case LLRenderDepthFunction::Less:
        return GL_LESS;
    case LLRenderDepthFunction::LessEqual:
        return GL_LEQUAL;
    case LLRenderDepthFunction::Equal:
        return GL_EQUAL;
    case LLRenderDepthFunction::NotEqual:
        return GL_NOTEQUAL;
    case LLRenderDepthFunction::GreaterEqual:
        return GL_GEQUAL;
    case LLRenderDepthFunction::Greater:
        return GL_GREATER;
    default:
        return GL_LESS;
    }
}

LLGLenum to_opengl_texture_target(LLRenderTextureTarget target)
{
    switch (target)
    {
    case LLRenderTextureTarget::Texture2D:
        return GL_TEXTURE_2D;
    case LLRenderTextureTarget::TextureRectangle:
        return GL_TEXTURE_RECTANGLE;
    case LLRenderTextureTarget::TextureCubeMap:
        return GL_TEXTURE_CUBE_MAP;
    case LLRenderTextureTarget::TextureCubeMapPositiveX:
        return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    case LLRenderTextureTarget::TextureCubeMapNegativeX:
        return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
    case LLRenderTextureTarget::TextureCubeMapPositiveY:
        return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
    case LLRenderTextureTarget::TextureCubeMapNegativeY:
        return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
    case LLRenderTextureTarget::TextureCubeMapPositiveZ:
        return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
    case LLRenderTextureTarget::TextureCubeMapNegativeZ:
        return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    case LLRenderTextureTarget::TextureCubeMapArray:
        return GL_TEXTURE_CUBE_MAP_ARRAY;
    case LLRenderTextureTarget::Texture2DMultisample:
        return GL_TEXTURE_2D_MULTISAMPLE;
    case LLRenderTextureTarget::Texture3D:
        return GL_TEXTURE_3D;
    default:
        return GL_TEXTURE_2D;
    }
}

LLGLenum to_opengl_texture_level_parameter(LLRenderTextureLevelParameter parameter)
{
    switch (parameter)
    {
    case LLRenderTextureLevelParameter::Width:
        return GL_TEXTURE_WIDTH;
    case LLRenderTextureLevelParameter::Height:
        return GL_TEXTURE_HEIGHT;
    case LLRenderTextureLevelParameter::Compressed:
        return GL_TEXTURE_COMPRESSED;
    case LLRenderTextureLevelParameter::CompressedImageSize:
        return GL_TEXTURE_COMPRESSED_IMAGE_SIZE;
    default:
        return GL_TEXTURE_WIDTH;
    }
}

LLGLenum to_opengl_texture_parameter(LLRenderTextureParameter parameter)
{
    switch (parameter)
    {
    case LLRenderTextureParameter::GenerateMipmap:
        return GL_GENERATE_MIPMAP;
    case LLRenderTextureParameter::BaseLevel:
        return GL_TEXTURE_BASE_LEVEL;
    case LLRenderTextureParameter::MaxLevel:
        return GL_TEXTURE_MAX_LEVEL;
    case LLRenderTextureParameter::SwizzleRGBA:
        return GL_TEXTURE_SWIZZLE_RGBA;
    default:
        return GL_TEXTURE_BASE_LEVEL;
    }
}

LLGLenum to_opengl_texture_address_mode(LLRenderTextureAddressMode mode)
{
    switch (mode)
    {
    case LLRenderTextureAddressMode::Repeat:
        return GL_REPEAT;
    case LLRenderTextureAddressMode::MirroredRepeat:
        return GL_MIRRORED_REPEAT;
    case LLRenderTextureAddressMode::ClampToEdge:
        return GL_CLAMP_TO_EDGE;
    default:
        return GL_CLAMP_TO_EDGE;
    }
}

LLGLenum to_opengl_texture_filter(LLRenderTextureFilter filter)
{
    switch (filter)
    {
    case LLRenderTextureFilter::Nearest:
        return GL_NEAREST;
    case LLRenderTextureFilter::Linear:
        return GL_LINEAR;
    case LLRenderTextureFilter::LinearMipmapLinear:
        return GL_LINEAR_MIPMAP_LINEAR;
    case LLRenderTextureFilter::LinearMipmapNearest:
        return GL_LINEAR_MIPMAP_NEAREST;
    case LLRenderTextureFilter::NearestMipmapNearest:
        return GL_NEAREST_MIPMAP_NEAREST;
    default:
        return GL_LINEAR;
    }
}

LLGLenum to_opengl_pixel_store_parameter(LLRenderPixelStoreParameter parameter)
{
    switch (parameter)
    {
    case LLRenderPixelStoreParameter::PackAlignment:
        return GL_PACK_ALIGNMENT;
    case LLRenderPixelStoreParameter::UnpackAlignment:
        return GL_UNPACK_ALIGNMENT;
    case LLRenderPixelStoreParameter::UnpackSwapBytes:
        return GL_UNPACK_SWAP_BYTES;
    case LLRenderPixelStoreParameter::UnpackRowLength:
        return GL_UNPACK_ROW_LENGTH;
    default:
        return GL_UNPACK_ALIGNMENT;
    }
}

bool has_depth_texture_coordinate(LLRenderTextureTarget target)
{
    return target == LLRenderTextureTarget::TextureCubeMap ||
        target == LLRenderTextureTarget::TextureCubeMapArray ||
        target == LLRenderTextureTarget::Texture3D;
}

LLGLenum to_opengl_buffer_target(LLRenderBufferTarget target)
{
    switch (target)
    {
    case LLRenderBufferTarget::Vertex:
        return GL_ARRAY_BUFFER;
    case LLRenderBufferTarget::Index:
        return GL_ELEMENT_ARRAY_BUFFER;
    case LLRenderBufferTarget::PixelPack:
        return GL_PIXEL_PACK_BUFFER;
    case LLRenderBufferTarget::PixelUnpack:
        return GL_PIXEL_UNPACK_BUFFER;
    case LLRenderBufferTarget::Uniform:
        return GL_UNIFORM_BUFFER;
    default:
        return GL_ARRAY_BUFFER;
    }
}

LLGLenum to_opengl_buffer_usage(LLRenderBufferUsage usage)
{
    switch (usage)
    {
    case LLRenderBufferUsage::StaticDraw:
        return GL_STATIC_DRAW;
    case LLRenderBufferUsage::DynamicDraw:
        return GL_DYNAMIC_DRAW;
    case LLRenderBufferUsage::StreamCopy:
        return GL_STREAM_COPY;
    default:
        return GL_STATIC_DRAW;
    }
}

LLGLenum to_opengl_primitive_type(LLRenderPrimitiveType mode)
{
    switch (mode)
    {
    case LLRenderPrimitiveType::Triangles:
        return GL_TRIANGLES;
    case LLRenderPrimitiveType::TriangleStrip:
        return GL_TRIANGLE_STRIP;
    case LLRenderPrimitiveType::TriangleFan:
        return GL_TRIANGLE_FAN;
    case LLRenderPrimitiveType::Points:
        return GL_POINTS;
    case LLRenderPrimitiveType::Lines:
        return GL_LINES;
    case LLRenderPrimitiveType::LineStrip:
        return GL_LINE_STRIP;
    case LLRenderPrimitiveType::LineLoop:
        return GL_LINE_LOOP;
    default:
        return GL_TRIANGLES;
    }
}

LLGLenum to_opengl_index_type(LLRenderIndexType type)
{
    switch (type)
    {
    case LLRenderIndexType::UnsignedShort:
        return GL_UNSIGNED_SHORT;
    case LLRenderIndexType::UnsignedInt:
        return GL_UNSIGNED_INT;
    default:
        return GL_UNSIGNED_SHORT;
    }
}

LLGLenum to_opengl_vertex_attribute_type(LLRenderVertexAttributeType type)
{
    switch (type)
    {
    case LLRenderVertexAttributeType::Float32:
        return GL_FLOAT;
    case LLRenderVertexAttributeType::UnsignedByte:
        return GL_UNSIGNED_BYTE;
    case LLRenderVertexAttributeType::UnsignedShort:
        return GL_UNSIGNED_SHORT;
    case LLRenderVertexAttributeType::UnsignedInt:
        return GL_UNSIGNED_INT;
    default:
        return GL_FLOAT;
    }
}

LLGLenum to_opengl_framebuffer_attachment(LLRenderFramebufferAttachment attachment)
{
    switch (attachment)
    {
    case LLRenderFramebufferAttachment::Color0:
        return GL_COLOR_ATTACHMENT0;
    case LLRenderFramebufferAttachment::Color1:
        return GL_COLOR_ATTACHMENT1;
    case LLRenderFramebufferAttachment::Color2:
        return GL_COLOR_ATTACHMENT2;
    case LLRenderFramebufferAttachment::Color3:
        return GL_COLOR_ATTACHMENT3;
    case LLRenderFramebufferAttachment::Depth:
        return GL_DEPTH_ATTACHMENT;
    default:
        return GL_COLOR_ATTACHMENT0;
    }
}

LLGLenum to_opengl_query_target(LLRenderQueryTarget target)
{
    switch (target)
    {
    case LLRenderQueryTarget::TimeElapsed:
        return GL_TIME_ELAPSED;
    case LLRenderQueryTarget::SamplesPassed:
        return GL_SAMPLES_PASSED;
    case LLRenderQueryTarget::PrimitivesGenerated:
        return GL_PRIMITIVES_GENERATED;
    default:
        return GL_TIME_ELAPSED;
    }
}

LLGLenum to_opengl_query_parameter(LLRenderQueryParameter parameter)
{
    switch (parameter)
    {
    case LLRenderQueryParameter::ResultAvailable:
        return GL_QUERY_RESULT_AVAILABLE;
    case LLRenderQueryParameter::Result:
        return GL_QUERY_RESULT;
    default:
        return GL_QUERY_RESULT;
    }
}

LLGLenum to_opengl_shader_stage(LLRenderShaderStage stage)
{
    switch (stage)
    {
    case LLRenderShaderStage::Vertex:
        return GL_VERTEX_SHADER;
    case LLRenderShaderStage::Fragment:
        return GL_FRAGMENT_SHADER;
    default:
        return GL_VERTEX_SHADER;
    }
}

LLGLenum to_opengl_shader_parameter(LLRenderShaderParameter parameter)
{
    switch (parameter)
    {
    case LLRenderShaderParameter::InfoLogLength:
        return GL_INFO_LOG_LENGTH;
    case LLRenderShaderParameter::CompileStatus:
        return GL_COMPILE_STATUS;
    default:
        return GL_INFO_LOG_LENGTH;
    }
}

LLGLenum to_opengl_program_parameter(LLRenderProgramParameter parameter)
{
    switch (parameter)
    {
    case LLRenderProgramParameter::InfoLogLength:
        return GL_INFO_LOG_LENGTH;
    case LLRenderProgramParameter::LinkStatus:
        return GL_LINK_STATUS;
    case LLRenderProgramParameter::ActiveUniforms:
        return GL_ACTIVE_UNIFORMS;
    case LLRenderProgramParameter::BinaryLength:
        return GL_PROGRAM_BINARY_LENGTH;
    default:
        return GL_INFO_LOG_LENGTH;
    }
}

LLGLenum to_opengl_program_setting(LLRenderProgramSetting parameter)
{
    switch (parameter)
    {
    case LLRenderProgramSetting::BinaryRetrievableHint:
        return GL_PROGRAM_BINARY_RETRIEVABLE_HINT;
    default:
        return GL_PROGRAM_BINARY_RETRIEVABLE_HINT;
    }
}

class LLNullRenderBackend final : public LLRenderBackend
{
public:
    LLRenderBackendType getType() const override { return LLRenderBackendType::Null; }
    const char* getName() const override { return "Null"; }
    bool isReady() const override { return true; }

    void beginFrame(const LLRenderFrameDesc&) override {}
    void endFrame() override {}

    void beginRenderPass(const LLRenderPassDesc&) override {}
    void endRenderPass() override {}

    void setViewport(const LLRenderViewport&) override {}
    void setScissor(const LLRenderScissor&) override {}
    void clear(const LLRenderPassDesc&) override {}
    void setClearColor(const LLRenderClearColor&) override {}
    void setColorMask(const LLRenderColorMask&) override {}
    void setBlendState(const LLRenderBlendState&) override {}
    void setLineWidth(F32) override {}
    void setCapability(LLRenderCapability, bool) override {}
    bool isCapabilityEnabled(LLRenderCapability) const override { return false; }
    void setCullFace(LLRenderCullFace) override {}
    void setDepthFunction(LLRenderDepthFunction) override {}
    void setDepthWriteEnabled(bool) override {}
    LLRenderFloatRange getLineWidthRange(bool) const override { return {1.f, 1.f}; }
    void setPixelStoreInteger(LLRenderPixelStoreParameter, S32) override {}
    S32 getActiveTextureUnit() const override { return 0; }
    void setActiveTextureUnit(S32) override {}
    void bindTexture(LLRenderTextureTarget, U32) override {}
    void setTextureAddressMode(LLRenderTextureTarget, LLRenderTextureAddressMode) override {}
    void setTextureFilter(LLRenderTextureTarget, LLRenderTextureFilter, LLRenderTextureFilter) override {}
    void setTextureMaxAnisotropy(LLRenderTextureTarget, F32) override {}
    void generateMipmaps(LLRenderTextureTarget) override {}
    void generateTextures(S32, U32*) override {}
    void deleteTextures(S32, const U32*) override {}
    void generateBuffers(S32, U32*) override {}
    void deleteBuffers(S32, const U32*) override {}
    void bindBuffer(LLRenderBufferTarget, U32) override {}
    void allocateBufferStorage(LLRenderBufferTarget, U64, const void*, LLRenderBufferUsage) override {}
    void updateBufferSubData(LLRenderBufferTarget, U32, U32, const void*) override {}
    void enableVertexAttributeArray(U32) override {}
    void disableVertexAttributeArray(U32) override {}
    void setVertexAttributePointer(
        U32,
        S32,
        LLRenderVertexAttributeType,
        bool,
        S32,
        const void*) override {}
    void setIntegerVertexAttributePointer(
        U32,
        S32,
        LLRenderVertexAttributeType,
        S32,
        const void*) override {}
    void drawIndexedRange(
        LLRenderPrimitiveType,
        U32,
        U32,
        S32,
        LLRenderIndexType,
        const void*) override {}
    void drawArrays(LLRenderPrimitiveType, S32, S32) override {}
    void generateFramebuffers(S32, U32*) override {}
    void deleteFramebuffers(S32, const U32*) override {}
    void bindReadWriteFramebuffer(U32) override {}
    bool isDrawFramebufferComplete() const override { return true; }
    void attachFramebufferTexture2D(
        LLRenderFramebufferAttachment,
        LLRenderTextureTarget,
        U32,
        S32) override {}
    void setFramebufferBufferRouting(U32) override {}
    void restoreDefaultFramebufferBufferRouting() override {}
    bool hasError() override { return false; }
    U32 getErrorCode() override { return 0; }
    void setDebugMessageCallback(LLRenderDebugMessageCallback, void*) override {}
    bool hasVertexArraySupport() const override { return true; }
    void generateVertexArrays(S32, U32*) override {}
    void bindVertexArray(U32) override {}
    void generateQueries(S32, U32*) override {}
    void deleteQueries(S32, const U32*) override {}
    void beginQuery(LLRenderQueryTarget, U32) override {}
    void endQuery(LLRenderQueryTarget) override {}
    void getQueryObjectUnsignedInteger64(U32, LLRenderQueryParameter, U64* value) override { *value = 0; }
    U32 createProgram() override { return 0; }
    void deleteProgram(U32) override {}
    U32 createShader(LLRenderShaderStage) override { return 0; }
    void deleteShader(U32) override {}
    bool isShader(U32) const override { return false; }
    bool isProgram(U32) const override { return false; }
    void attachShader(U32, U32) override {}
    void detachShader(U32, U32) override {}
    void getAttachedShaders(U32, S32, S32* count, U32*) override { *count = 0; }
    void setShaderSource(U32, S32, const char* const*) override {}
    void compileShader(U32) override {}
    void linkProgram(U32) override {}
    void validateProgram(U32) override {}
    void useProgram(U32) override {}
    void getShaderInteger(U32, LLRenderShaderParameter, S32* value) override { *value = 0; }
    void getProgramInteger(U32, LLRenderProgramParameter, S32* value) override { *value = 0; }
    void getShaderInfoLog(U32, S32, S32* length, char*) override { *length = 0; }
    void getProgramInfoLog(U32, S32, S32* length, char*) override { *length = 0; }
    void setProgramParameterInteger(U32, LLRenderProgramSetting, S32) override {}
    void setProgramBinary(U32, U32, const void*, S32) override {}
    void getProgramBinary(U32, S32, S32* length, U32* binary_format, void*) override
    {
        *length = 0;
        *binary_format = 0;
    }
    S32 getUniformLocation(U32, const char*) override { return -1; }
    S32 getAttributeLocation(U32, const char*) override { return -1; }
    void bindAttributeLocation(U32, U32, const char*) override {}
    void getActiveUniform(U32, U32, S32, S32* length, S32* size, U32* type, char* name) override
    {
        *length = 0;
        *size = 0;
        *type = 0;
        if (name)
        {
            name[0] = '\0';
        }
    }
    U32 getUniformBlockIndex(U32, const char*) override { return 0; }
    void bindUniformBlock(U32, U32, U32) override {}
    void setUniformInteger(S32, S32) override {}
    void setUniformInteger2(S32, S32, S32) override {}
    void setUniformIntegerVector(S32, S32, const S32*) override {}
    void setUniformIntegerVector4(S32, S32, const S32*) override {}
    void setUniformUnsignedIntegerVector4(S32, S32, const U32*) override {}
    void setUniformFloat(S32, F32) override {}
    void setUniformFloat2(S32, F32, F32) override {}
    void setUniformFloat3(S32, F32, F32, F32) override {}
    void setUniformFloat4(S32, F32, F32, F32, F32) override {}
    void setUniformFloatVector(S32, S32, const F32*) override {}
    void setUniformFloatVector2(S32, S32, const F32*) override {}
    void setUniformFloatVector3(S32, S32, const F32*) override {}
    void setUniformFloatVector4(S32, S32, const F32*) override {}
    void setUniformMatrix2(S32, S32, bool, const F32*) override {}
    void setUniformMatrix3(S32, S32, bool, const F32*) override {}
    void setUniformMatrix3x4(S32, S32, bool, const F32*) override {}
    void setUniformMatrix4(S32, S32, bool, const F32*) override {}
    void setVertexAttribute4(U32, F32, F32, F32, F32) override {}
    void setVertexAttributeVector4(U32, const F32*) override {}
    void pushLegacyAllAttributes() override {}
    void pushLegacyAllClientAttributes() override {}
    void popLegacyClientAttributes() override {}
    void popLegacyAttributes() override {}
    void copyTextureImage2D(LLRenderTextureTarget, S32, U32, S32, S32, S32, S32, S32) override {}
    void setTextureImage2D(
        LLRenderTextureTarget,
        S32,
        S32,
        S32,
        S32,
        S32,
        U32,
        U32,
        const void*) override {}
    void readTextureImage(LLRenderTextureTarget, S32, U32, U32, void*) override {}
    void readCompressedTextureImage(LLRenderTextureTarget, S32, void*) override {}
    void copyTextureSubImage2D(LLRenderTextureTarget, S32, S32, S32, S32, S32, S32, S32) override {}
    void setCompressedTextureImage2D(LLRenderTextureTarget, S32, S32, S32, S32, S32, S32, const void*) override {}
    void setTextureSubImage2D(LLRenderTextureTarget, S32, S32, S32, S32, S32, U32, U32, const void*) override {}
    void setTextureSubImage3D(
        LLRenderTextureTarget,
        S32,
        S32,
        S32,
        S32,
        S32,
        S32,
        S32,
        U32,
        U32,
        const void*) override {}
    void setTextureImage3D(
        LLRenderTextureTarget,
        S32,
        S32,
        S32,
        S32,
        S32,
        S32,
        U32,
        U32,
        const void*) override {}
    void getTextureLevelParameterInteger(LLRenderTextureTarget, S32, LLRenderTextureLevelParameter, S32* value) override
    {
        *value = 0;
    }
    void setTextureParameterInteger(LLRenderTextureTarget, LLRenderTextureParameter, S32) override {}
    void setTextureParameterIntegerVector(LLRenderTextureTarget, LLRenderTextureParameter, const S32*) override {}
    void areTexturesResident(S32 count, const U32*, bool* residences) override
    {
        for (S32 i = 0; i < count; ++i)
        {
            residences[i] = false;
        }
    }
    void getViewport(S32* viewport) override
    {
        viewport[0] = 0;
        viewport[1] = 0;
        viewport[2] = 0;
        viewport[3] = 0;
    }
    U32 getBoundTexture2D() override { return 0; }
    void* createSyncObject() override { return nullptr; }
    void flushCommands() override {}
    void finishCommands() override {}
    void clientWaitSyncObject(void*) override {}
    U32 clientWaitSyncObjectStatus(void*, U64) override { return 0; }
    void waitSyncObject(void*) override {}
    void deleteSyncObject(void*) override {}
    void setLegacyMaterialSpecular(const F32*, S32) override {}
    void getLegacyInteger(U32, S32* value) override { *value = 0; }
    void getLegacyBoolean(U32, U8* value) override { *value = 0; }
    void getLegacyFloat(U32, F32* value) override { *value = 0.f; }
    void getLegacyBufferObjectParameterInteger(U32, U32, S32* value) override { *value = 0; }
    const char* getLegacyString(U32) override { return ""; }
    const char* getLegacyStringIndexed(U32, U32) override { return ""; }
    void setLegacyHint(U32, U32) override {}
    void setClientActiveTextureUnit(S32) override {}
    void setLegacyCapability(U32, bool) override {}
    bool isLegacyCapabilityEnabled(U32) override { return false; }
};

class LLOpenGLRenderBackend final : public LLRenderBackend
{
public:
    LLRenderBackendType getType() const override { return LLRenderBackendType::OpenGL; }
    const char* getName() const override { return "OpenGL"; }
    bool isReady() const override { return true; }

    void beginFrame(const LLRenderFrameDesc&) override {}
    void endFrame() override {}

    void beginRenderPass(const LLRenderPassDesc&) override {}
    void endRenderPass() override {}

    void setViewport(const LLRenderViewport& viewport) override
    {
        LLGLContainment::setViewport(
            static_cast<LLGLint>(viewport.mX),
            static_cast<LLGLint>(viewport.mY),
            static_cast<LLGLint>(viewport.mWidth),
            static_cast<LLGLint>(viewport.mHeight));
    }

    void setScissor(const LLRenderScissor& scissor) override
    {
        if (!scissor.mEnabled)
        {
            return;
        }

        LLGLContainment::setScissorBox(
            scissor.mX,
            scissor.mY,
            static_cast<U32>(scissor.mWidth),
            static_cast<U32>(scissor.mHeight));
    }

    void clear(const LLRenderPassDesc& desc) override
    {
        LLGLContainment::clearBuffersByIntent(
            (desc.mClearMask & LL_RENDER_CLEAR_COLOR) != 0,
            (desc.mClearMask & LL_RENDER_CLEAR_DEPTH) != 0,
            (desc.mClearMask & LL_RENDER_CLEAR_STENCIL) != 0);
    }

    void setClearColor(const LLRenderClearColor& color) override
    {
        LLGLContainment::setClearColor(color.mRed, color.mGreen, color.mBlue, color.mAlpha);
    }

    void setColorMask(const LLRenderColorMask& mask) override
    {
        LLGLContainment::setColorMask(
            static_cast<LLGLboolean>(mask.mRed),
            static_cast<LLGLboolean>(mask.mGreen),
            static_cast<LLGLboolean>(mask.mBlue),
            static_cast<LLGLboolean>(mask.mAlpha));
    }

    void setBlendState(const LLRenderBlendState& blend) override
    {
        LLGLContainment::setSeparateBlendFunction(
            to_opengl_blend_factor(blend.mColorSource),
            to_opengl_blend_factor(blend.mColorDestination),
            to_opengl_blend_factor(blend.mAlphaSource),
            to_opengl_blend_factor(blend.mAlphaDestination));
    }

    void setLineWidth(F32 width) override
    {
        LLGLContainment::setLineWidth(width);
    }

    void setCapability(LLRenderCapability capability, bool enabled) override
    {
        LLGLenum gl_capability = to_opengl_capability(capability);
        if (!gl_capability)
        {
            return;
        }

        if (enabled)
        {
            LLGLContainment::enableCapability(gl_capability);
        }
        else
        {
            LLGLContainment::disableCapability(gl_capability);
        }
    }

    bool isCapabilityEnabled(LLRenderCapability capability) const override
    {
        LLGLenum gl_capability = to_opengl_capability(capability);
        if (!gl_capability)
        {
            return false;
        }

        return LLGLContainment::isCapabilityEnabled(gl_capability);
    }

    void setCullFace(LLRenderCullFace face) override
    {
        LLGLContainment::setCullFace(to_opengl_cull_face(face));
    }

    void setDepthFunction(LLRenderDepthFunction function) override
    {
        LLGLContainment::setDepthFunction(to_opengl_depth_function(function));
    }

    void setDepthWriteEnabled(bool enabled) override
    {
        LLGLContainment::setDepthMask(static_cast<LLGLboolean>(enabled));
    }

    LLRenderFloatRange getLineWidthRange(bool smooth) const override
    {
        LLGLfloat range[2] = {1.f, 1.f};
        LLGLContainment::getFloat(
            smooth ? GL_SMOOTH_LINE_WIDTH_RANGE : GL_ALIASED_LINE_WIDTH_RANGE,
            range);

        LLRenderFloatRange result;
        result.mMinimum = range[0];
        result.mMaximum = range[1];
        return result;
    }

    void setPixelStoreInteger(LLRenderPixelStoreParameter parameter, S32 value) override
    {
        LLGLContainment::setPixelStoreInteger(to_opengl_pixel_store_parameter(parameter), value);
    }

    S32 getActiveTextureUnit() const override
    {
        LLGLint active_texture = GL_TEXTURE0;
        LLGLContainment::getInteger(GL_ACTIVE_TEXTURE, &active_texture);
        return active_texture - GL_TEXTURE0;
    }

    void setActiveTextureUnit(S32 unit) override
    {
        LLGLContainment::setActiveTexture(GL_TEXTURE0 + unit);
    }

    void bindTexture(LLRenderTextureTarget target, U32 texture) override
    {
        LLGLContainment::bindTexture(to_opengl_texture_target(target), texture);
    }

    void setTextureAddressMode(
        LLRenderTextureTarget target,
        LLRenderTextureAddressMode mode) override
    {
        LLGLenum gl_target = to_opengl_texture_target(target);
        LLGLenum gl_mode = to_opengl_texture_address_mode(mode);
        LLGLContainment::setTextureParameterInteger(gl_target, GL_TEXTURE_WRAP_S, gl_mode);
        LLGLContainment::setTextureParameterInteger(gl_target, GL_TEXTURE_WRAP_T, gl_mode);
        if (has_depth_texture_coordinate(target))
        {
            LLGLContainment::setTextureParameterInteger(gl_target, GL_TEXTURE_WRAP_R, gl_mode);
        }
    }

    void setTextureFilter(
        LLRenderTextureTarget target,
        LLRenderTextureFilter min_filter,
        LLRenderTextureFilter mag_filter) override
    {
        LLGLenum gl_target = to_opengl_texture_target(target);
        LLGLContainment::setTextureParameterInteger(
            gl_target,
            GL_TEXTURE_MAG_FILTER,
            to_opengl_texture_filter(mag_filter));
        LLGLContainment::setTextureParameterInteger(
            gl_target,
            GL_TEXTURE_MIN_FILTER,
            to_opengl_texture_filter(min_filter));
    }

    void setTextureMaxAnisotropy(LLRenderTextureTarget target, F32 anisotropy) override
    {
        LLGLContainment::setTextureParameterFloat(
            to_opengl_texture_target(target),
            GL_TEXTURE_MAX_ANISOTROPY,
            anisotropy);
    }

    void generateMipmaps(LLRenderTextureTarget target) override
    {
        LLGLContainment::generateTextureMipmap(to_opengl_texture_target(target));
    }

    void generateTextures(S32 count, U32* textures) override
    {
        LLGLContainment::generateTextures(count, textures);
    }

    void deleteTextures(S32 count, const U32* textures) override
    {
        LLGLContainment::deleteTextures(count, textures);
    }

    void generateBuffers(S32 count, U32* buffers) override
    {
        LLGLContainment::generateBufferObjects(count, buffers);
    }

    void deleteBuffers(S32 count, const U32* buffers) override
    {
        LLGLContainment::deleteBufferObjects(count, buffers);
    }

    void bindBuffer(LLRenderBufferTarget target, U32 buffer) override
    {
        LLGLContainment::bindBufferObject(to_opengl_buffer_target(target), buffer);
    }

    void allocateBufferStorage(
        LLRenderBufferTarget target,
        U64 size,
        const void* data,
        LLRenderBufferUsage usage) override
    {
        LLGLContainment::allocateBufferObjectStorage(
            to_opengl_buffer_target(target),
            size,
            data,
            to_opengl_buffer_usage(usage));
    }

    void updateBufferSubData(
        LLRenderBufferTarget target,
        U32 offset,
        U32 size,
        const void* data) override
    {
        LLGLContainment::updateBufferObjectSubData(to_opengl_buffer_target(target), offset, size, data);
    }

    void enableVertexAttributeArray(U32 location) override
    {
        LLGLContainment::enableVertexAttributeArray(location);
    }

    void disableVertexAttributeArray(U32 location) override
    {
        LLGLContainment::disableVertexAttributeArray(location);
    }

    void setVertexAttributePointer(
        U32 location,
        S32 size,
        LLRenderVertexAttributeType type,
        bool normalized,
        S32 stride,
        const void* pointer) override
    {
        LLGLContainment::setVertexAttributePointer(
            location,
            size,
            to_opengl_vertex_attribute_type(type),
            static_cast<LLGLboolean>(normalized),
            stride,
            pointer);
    }

    void setIntegerVertexAttributePointer(
        U32 location,
        S32 size,
        LLRenderVertexAttributeType type,
        S32 stride,
        const void* pointer) override
    {
        LLGLContainment::setIntegerVertexAttributePointer(
            location,
            size,
            to_opengl_vertex_attribute_type(type),
            stride,
            pointer);
    }

    void drawIndexedRange(
        LLRenderPrimitiveType mode,
        U32 start,
        U32 end,
        S32 count,
        LLRenderIndexType index_type,
        const void* indices) override
    {
        LLGLContainment::drawVertexBufferRange(
            to_opengl_primitive_type(mode),
            start,
            end,
            count,
            to_opengl_index_type(index_type),
            indices);
    }

    void drawArrays(LLRenderPrimitiveType mode, S32 first, S32 count) override
    {
        LLGLContainment::drawVertexBufferArrays(to_opengl_primitive_type(mode), first, count);
    }

    void generateFramebuffers(S32 count, U32* framebuffers) override
    {
        LLGLContainment::generateFramebuffers(count, framebuffers);
    }

    void deleteFramebuffers(S32 count, const U32* framebuffers) override
    {
        LLGLContainment::deleteFramebuffers(count, framebuffers);
    }

    void bindReadWriteFramebuffer(U32 framebuffer) override
    {
        LLGLContainment::bindReadWriteFramebuffer(framebuffer);
    }

    bool isDrawFramebufferComplete() const override
    {
        return LLGLContainment::getDrawFramebufferStatus() == GL_FRAMEBUFFER_COMPLETE;
    }

    void attachFramebufferTexture2D(
        LLRenderFramebufferAttachment attachment,
        LLRenderTextureTarget target,
        U32 texture,
        S32 mip_level) override
    {
        LLGLContainment::setReadWriteFramebufferTexture2D(
            to_opengl_framebuffer_attachment(attachment),
            to_opengl_texture_target(target),
            texture,
            mip_level);
    }

    void setFramebufferBufferRouting(U32 color_attachment_count) override
    {
        LLGLenum drawbuffers[] = {GL_COLOR_ATTACHMENT0,
                                  GL_COLOR_ATTACHMENT1,
                                  GL_COLOR_ATTACHMENT2,
                                  GL_COLOR_ATTACHMENT3};

        if (color_attachment_count == 0)
        {
            LLGLContainment::setDrawBuffer(GL_NONE);
            LLGLContainment::setReadBuffer(GL_NONE);
        }
        else
        {
            LLGLContainment::setDrawBuffers(static_cast<S32>(color_attachment_count), drawbuffers);
            LLGLContainment::setReadBuffer(GL_COLOR_ATTACHMENT0);
        }
    }

    void restoreDefaultFramebufferBufferRouting() override
    {
        LLGLContainment::setReadBuffer(GL_BACK);
        LLGLContainment::setDrawBuffer(GL_BACK);
    }

    bool hasError() override
    {
        return LLGLContainment::getError() != GL_NO_ERROR;
    }

    U32 getErrorCode() override
    {
        return LLGLContainment::getError();
    }

    void setDebugMessageCallback(LLRenderDebugMessageCallback callback, void* user_param) override
    {
        LLGLContainment::setDebugMessageCallback(
            reinterpret_cast<LLGLContainment::DebugMessageCallback>(callback),
            user_param);
    }

    bool hasVertexArraySupport() const override
    {
        return LLGLContainment::hasVertexArrayGenerator();
    }

    void generateVertexArrays(S32 count, U32* arrays) override
    {
        LLGLContainment::generateVertexArrays(count, arrays);
    }

    void bindVertexArray(U32 array) override
    {
        LLGLContainment::bindVertexArray(array);
    }

    void generateQueries(S32 count, U32* queries) override
    {
        LLGLContainment::generateQueries(count, queries);
    }

    void deleteQueries(S32 count, const U32* queries) override
    {
        LLGLContainment::deleteQueries(count, queries);
    }

    void beginQuery(LLRenderQueryTarget target, U32 query) override
    {
        LLGLContainment::beginQuery(to_opengl_query_target(target), query);
    }

    void endQuery(LLRenderQueryTarget target) override
    {
        LLGLContainment::endQuery(to_opengl_query_target(target));
    }

    void getQueryObjectUnsignedInteger64(
        U32 query,
        LLRenderQueryParameter parameter,
        U64* value) override
    {
        LLGLContainment::getQueryObjectUnsignedInteger64(
            query,
            to_opengl_query_parameter(parameter),
            value);
    }

    U32 createProgram() override
    {
        return LLGLContainment::createProgram();
    }

    void deleteProgram(U32 program) override
    {
        LLGLContainment::deleteProgram(program);
    }

    U32 createShader(LLRenderShaderStage stage) override
    {
        return LLGLContainment::createShader(to_opengl_shader_stage(stage));
    }

    void deleteShader(U32 shader) override
    {
        LLGLContainment::deleteShader(shader);
    }

    bool isShader(U32 shader) const override
    {
        return LLGLContainment::isShader(shader);
    }

    bool isProgram(U32 program) const override
    {
        return LLGLContainment::isProgram(program);
    }

    void attachShader(U32 program, U32 shader) override
    {
        LLGLContainment::attachShader(program, shader);
    }

    void detachShader(U32 program, U32 shader) override
    {
        LLGLContainment::detachShader(program, shader);
    }

    void getAttachedShaders(U32 program, S32 max_count, S32* count, U32* shaders) override
    {
        LLGLContainment::getAttachedShaders(program, max_count, count, shaders);
    }

    void setShaderSource(U32 shader, S32 count, const char* const* strings) override
    {
        LLGLContainment::setShaderSource(shader, count, strings);
    }

    void compileShader(U32 shader) override
    {
        LLGLContainment::compileShader(shader);
    }

    void linkProgram(U32 program) override
    {
        LLGLContainment::linkProgram(program);
    }

    void validateProgram(U32 program) override
    {
        LLGLContainment::validateProgram(program);
    }

    void useProgram(U32 program) override
    {
        LLGLContainment::useProgram(program);
    }

    void getShaderInteger(U32 shader, LLRenderShaderParameter parameter, S32* value) override
    {
        LLGLContainment::getShaderInteger(shader, to_opengl_shader_parameter(parameter), value);
    }

    void getProgramInteger(U32 program, LLRenderProgramParameter parameter, S32* value) override
    {
        LLGLContainment::getProgramInteger(program, to_opengl_program_parameter(parameter), value);
    }

    void getShaderInfoLog(U32 shader, S32 buffer_size, S32* length, char* info_log) override
    {
        LLGLContainment::getShaderInfoLog(shader, buffer_size, length, info_log);
    }

    void getProgramInfoLog(U32 program, S32 buffer_size, S32* length, char* info_log) override
    {
        LLGLContainment::getProgramInfoLog(program, buffer_size, length, info_log);
    }

    void setProgramParameterInteger(U32 program, LLRenderProgramSetting parameter, S32 value) override
    {
        LLGLContainment::setProgramParameterInteger(program, to_opengl_program_setting(parameter), value);
    }

    void setProgramBinary(U32 program, U32 binary_format, const void* binary, S32 length) override
    {
        LLGLContainment::setProgramBinary(program, binary_format, binary, length);
    }

    void getProgramBinary(
        U32 program,
        S32 buffer_size,
        S32* length,
        U32* binary_format,
        void* binary) override
    {
        LLGLContainment::getProgramBinary(program, buffer_size, length, binary_format, binary);
    }

    S32 getUniformLocation(U32 program, const char* name) override
    {
        return LLGLContainment::getUniformLocation(program, name);
    }

    S32 getAttributeLocation(U32 program, const char* name) override
    {
        return LLGLContainment::getAttributeLocation(program, name);
    }

    void bindAttributeLocation(U32 program, U32 index, const char* name) override
    {
        LLGLContainment::bindAttributeLocation(program, index, name);
    }

    void getActiveUniform(
        U32 program,
        U32 index,
        S32 buffer_size,
        S32* length,
        S32* size,
        U32* type,
        char* name) override
    {
        LLGLenum gl_type = 0;
        LLGLContainment::getActiveUniform(program, index, buffer_size, length, size, &gl_type, name);
        *type = gl_type;
    }

    U32 getUniformBlockIndex(U32 program, const char* name) override
    {
        return LLGLContainment::getUniformBlockIndex(program, name);
    }

    void bindUniformBlock(U32 program, U32 block_index, U32 binding) override
    {
        LLGLContainment::bindUniformBlock(program, block_index, binding);
    }

    void setUniformInteger(S32 location, S32 value) override
    {
        LLGLContainment::setUniformInteger(location, value);
    }

    void setUniformInteger2(S32 location, S32 first, S32 second) override
    {
        LLGLContainment::setUniformInteger2(location, first, second);
    }

    void setUniformIntegerVector(S32 location, S32 count, const S32* values) override
    {
        LLGLContainment::setUniformIntegerVector(location, count, values);
    }

    void setUniformIntegerVector4(S32 location, S32 count, const S32* values) override
    {
        LLGLContainment::setUniformIntegerVector4(location, count, values);
    }

    void setUniformUnsignedIntegerVector4(S32 location, S32 count, const U32* values) override
    {
        LLGLContainment::setUniformUnsignedIntegerVector4(location, count, values);
    }

    void setUniformFloat(S32 location, F32 value) override
    {
        LLGLContainment::setUniformFloat(location, value);
    }

    void setUniformFloat2(S32 location, F32 first, F32 second) override
    {
        LLGLContainment::setUniformFloat2(location, first, second);
    }

    void setUniformFloat3(S32 location, F32 first, F32 second, F32 third) override
    {
        LLGLContainment::setUniformFloat3(location, first, second, third);
    }

    void setUniformFloat4(S32 location, F32 first, F32 second, F32 third, F32 fourth) override
    {
        LLGLContainment::setUniformFloat4(location, first, second, third, fourth);
    }

    void setUniformFloatVector(S32 location, S32 count, const F32* values) override
    {
        LLGLContainment::setUniformFloatVector(location, count, values);
    }

    void setUniformFloatVector2(S32 location, S32 count, const F32* values) override
    {
        LLGLContainment::setUniformFloatVector2(location, count, values);
    }

    void setUniformFloatVector3(S32 location, S32 count, const F32* values) override
    {
        LLGLContainment::setUniformFloatVector3(location, count, values);
    }

    void setUniformFloatVector4(S32 location, S32 count, const F32* values) override
    {
        LLGLContainment::setUniformFloatVector4(location, count, values);
    }

    void setUniformMatrix2(S32 location, S32 count, bool transpose, const F32* values) override
    {
        LLGLContainment::setUniformMatrix2(location, count, transpose, values);
    }

    void setUniformMatrix3(S32 location, S32 count, bool transpose, const F32* values) override
    {
        LLGLContainment::setUniformMatrix3(location, count, transpose, values);
    }

    void setUniformMatrix3x4(S32 location, S32 count, bool transpose, const F32* values) override
    {
        LLGLContainment::setUniformMatrix3x4(location, count, transpose, values);
    }

    void setUniformMatrix4(S32 location, S32 count, bool transpose, const F32* values) override
    {
        LLGLContainment::setUniformMatrix4(location, count, transpose, values);
    }

    void setVertexAttribute4(U32 location, F32 first, F32 second, F32 third, F32 fourth) override
    {
        LLGLContainment::setVertexAttribute4(location, first, second, third, fourth);
    }

    void setVertexAttributeVector4(U32 location, const F32* values) override
    {
        LLGLContainment::setVertexAttributeVector4(location, values);
    }

    void pushLegacyAllAttributes() override
    {
        LLGLContainment::pushAttributeBits(GL_ALL_ATTRIB_BITS);
    }

    void pushLegacyAllClientAttributes() override
    {
        LLGLContainment::pushClientAttributeBits(GL_ALL_ATTRIB_BITS);
    }

    void popLegacyClientAttributes() override
    {
        LLGLContainment::popClientAttributes();
    }

    void popLegacyAttributes() override
    {
        LLGLContainment::popAttributes();
    }

    void copyTextureImage2D(
        LLRenderTextureTarget target,
        S32 level,
        U32 internal_format,
        S32 x,
        S32 y,
        S32 width,
        S32 height,
        S32 border) override
    {
        LLGLContainment::copyTextureImage2D(
            to_opengl_texture_target(target),
            level,
            internal_format,
            x,
            y,
            width,
            height,
            border);
    }

    void setTextureImage2D(
        LLRenderTextureTarget target,
        S32 level,
        S32 internal_format,
        S32 width,
        S32 height,
        S32 border,
        U32 format,
        U32 type,
        const void* data) override
    {
        LLGLContainment::setTextureImage2D(
            to_opengl_texture_target(target),
            level,
            internal_format,
            width,
            height,
            border,
            format,
            type,
            data);
    }

    void readTextureImage(
        LLRenderTextureTarget target,
        S32 level,
        U32 format,
        U32 type,
        void* pixels) override
    {
        LLGLContainment::readTextureImage(
            to_opengl_texture_target(target),
            level,
            format,
            type,
            pixels);
    }

    void readCompressedTextureImage(LLRenderTextureTarget target, S32 level, void* pixels) override
    {
        LLGLContainment::readCompressedTextureImage(to_opengl_texture_target(target), level, pixels);
    }

    void copyTextureSubImage2D(
        LLRenderTextureTarget target,
        S32 level,
        S32 xoffset,
        S32 yoffset,
        S32 x,
        S32 y,
        S32 width,
        S32 height) override
    {
        LLGLContainment::copyTextureSubImage2D(
            to_opengl_texture_target(target),
            level,
            xoffset,
            yoffset,
            x,
            y,
            width,
            height);
    }

    void setCompressedTextureImage2D(
        LLRenderTextureTarget target,
        S32 level,
        S32 internal_format,
        S32 width,
        S32 height,
        S32 border,
        S32 image_size,
        const void* data) override
    {
        LLGLContainment::setCompressedTextureImage2D(
            to_opengl_texture_target(target),
            level,
            internal_format,
            width,
            height,
            border,
            image_size,
            data);
    }

    void setTextureSubImage2D(
        LLRenderTextureTarget target,
        S32 level,
        S32 xoffset,
        S32 yoffset,
        S32 width,
        S32 height,
        U32 format,
        U32 type,
        const void* pixels) override
    {
        LLGLContainment::setTextureSubImage2D(
            to_opengl_texture_target(target),
            level,
            xoffset,
            yoffset,
            width,
            height,
            format,
            type,
            pixels);
    }

    void setTextureSubImage3D(
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
        const void* pixels) override
    {
        LLGLContainment::setTextureSubImage3D(
            to_opengl_texture_target(target),
            level,
            xoffset,
            yoffset,
            zoffset,
            width,
            height,
            depth,
            format,
            type,
            pixels);
    }

    void setTextureImage3D(
        LLRenderTextureTarget target,
        S32 level,
        S32 internal_format,
        S32 width,
        S32 height,
        S32 depth,
        S32 border,
        U32 format,
        U32 type,
        const void* data) override
    {
        LLGLContainment::setTextureImage3D(
            to_opengl_texture_target(target),
            level,
            internal_format,
            width,
            height,
            depth,
            border,
            format,
            type,
            data);
    }

    void getTextureLevelParameterInteger(
        LLRenderTextureTarget target,
        S32 level,
        LLRenderTextureLevelParameter parameter,
        S32* value) override
    {
        LLGLContainment::getTextureLevelParameterInteger(
            to_opengl_texture_target(target),
            level,
            to_opengl_texture_level_parameter(parameter),
            value);
    }

    void setTextureParameterInteger(
        LLRenderTextureTarget target,
        LLRenderTextureParameter parameter,
        S32 value) override
    {
        LLGLContainment::setTextureParameterInteger(
            to_opengl_texture_target(target),
            to_opengl_texture_parameter(parameter),
            value);
    }

    void setTextureParameterIntegerVector(
        LLRenderTextureTarget target,
        LLRenderTextureParameter parameter,
        const S32* values) override
    {
        LLGLContainment::setTextureParameterIntegerVector(
            to_opengl_texture_target(target),
            to_opengl_texture_parameter(parameter),
            values);
    }

    void areTexturesResident(S32 count, const U32* textures, bool* residences) override
    {
        std::vector<LLGLboolean> gl_residences(count);
        LLGLContainment::areTexturesResident(count, textures, gl_residences.data());
        for (S32 i = 0; i < count; ++i)
        {
            residences[i] = gl_residences[i] != 0;
        }
    }

    void getViewport(S32* viewport) override
    {
        LLGLContainment::getInteger(GL_VIEWPORT, viewport);
    }

    U32 getBoundTexture2D() override
    {
        LLGLint texture = 0;
        LLGLContainment::getInteger(GL_TEXTURE_BINDING_2D, &texture);
        return static_cast<U32>(texture);
    }

    void* createSyncObject() override
    {
        return LLGLContainment::createSyncObject();
    }

    void flushCommands() override
    {
        LLGLContainment::flushCommands();
    }

    void finishCommands() override
    {
        LLGLContainment::finishCommands();
    }

    void clientWaitSyncObject(void* sync) override
    {
        LLGLContainment::clientWaitSyncObject(sync);
    }

    U32 clientWaitSyncObjectStatus(void* sync, U64 timeout) override
    {
        return LLGLContainment::clientWaitSyncObjectStatus(sync, timeout);
    }

    void waitSyncObject(void* sync) override
    {
        LLGLContainment::waitSyncObject(sync);
    }

    void deleteSyncObject(void* sync) override
    {
        LLGLContainment::deleteSyncObject(sync);
    }

    void setLegacyMaterialSpecular(const F32* color, S32 shininess) override
    {
        LLGLContainment::setMaterialFloatVector(GL_FRONT_AND_BACK, GL_SPECULAR, color);
        LLGLContainment::setMaterialInteger(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    }

    void getLegacyInteger(U32 parameter, S32* value) override
    {
        LLGLContainment::getInteger(parameter, value);
    }

    void getLegacyBoolean(U32 parameter, U8* value) override
    {
        LLGLContainment::getBoolean(parameter, value);
    }

    void getLegacyFloat(U32 parameter, F32* value) override
    {
        LLGLContainment::getFloat(parameter, value);
    }

    void getLegacyBufferObjectParameterInteger(U32 target, U32 parameter, S32* value) override
    {
        LLGLContainment::getBufferObjectParameterInteger(target, parameter, value);
    }

    const char* getLegacyString(U32 parameter) override
    {
        return LLGLContainment::getString(parameter);
    }

    const char* getLegacyStringIndexed(U32 parameter, U32 index) override
    {
        return LLGLContainment::getStringIndexed(parameter, index);
    }

    void setLegacyHint(U32 target, U32 mode) override
    {
        LLGLContainment::setHint(target, mode);
    }

    void setClientActiveTextureUnit(S32 unit) override
    {
        LLGLContainment::setClientActiveTexture(GL_TEXTURE0 + unit);
    }

    void setLegacyCapability(U32 capability, bool enabled) override
    {
        if (enabled)
        {
            LLGLContainment::enableCapability(capability);
        }
        else
        {
            LLGLContainment::disableCapability(capability);
        }
    }

    bool isLegacyCapabilityEnabled(U32 capability) override
    {
        return LLGLContainment::isCapabilityEnabled(capability);
    }
};
}

LLRenderBackend::~LLRenderBackend() = default;

const char* getRenderBackendTypeName(LLRenderBackendType type)
{
    switch (type)
    {
    case LLRenderBackendType::OpenGL:
        return "OpenGL";
    case LLRenderBackendType::Vulkan:
        return "Vulkan";
    case LLRenderBackendType::Metal:
        return "Metal";
    case LLRenderBackendType::Null:
        return "Null";
    case LLRenderBackendType::Unknown:
    default:
        return "Unknown";
    }
}

LLRenderBackend& getNullRenderBackend()
{
    static LLNullRenderBackend backend;
    return backend;
}

LLRenderBackend& getOpenGLRenderBackend()
{
    static LLOpenGLRenderBackend backend;
    return backend;
}
