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
    void setDebugMessageCallback(LLRenderDebugMessageCallback, void*) override {}
    bool hasVertexArraySupport() const override { return true; }
    void generateVertexArrays(S32, U32*) override {}
    void bindVertexArray(U32) override {}
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
