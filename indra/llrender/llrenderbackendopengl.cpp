/**
 * @file llrenderbackendopengl.cpp
 * @brief OpenGL render backend implementation.
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

#include "llrenderbackendopengl.h"

#include "llglcontainment.h"
#include "llglheaders.h"
#include "llopenglplatform.h"
#include "llrendercontext.h"

#if LL_DARWIN
#include "llrenderbackendmacosx-objc.h"
#include <CoreGraphics/CGDirectDisplay.h>
#endif

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
    case LLRenderCapability::AlphaTest:
        return GL_ALPHA_TEST;
    case LLRenderCapability::Blend:
        return GL_BLEND;
    case LLRenderCapability::CullFace:
        return GL_CULL_FACE;
    case LLRenderCapability::DebugOutputSynchronous:
        return GL_DEBUG_OUTPUT_SYNCHRONOUS;
    case LLRenderCapability::DepthTest:
        return GL_DEPTH_TEST;
    case LLRenderCapability::DepthClamp:
#ifdef GL_DEPTH_CLAMP
        return GL_DEPTH_CLAMP;
#else
        return 0;
#endif
    case LLRenderCapability::LineSmooth:
        return GL_LINE_SMOOTH;
    case LLRenderCapability::Multisample:
        return GL_MULTISAMPLE;
    case LLRenderCapability::PolygonOffsetFill:
        return GL_POLYGON_OFFSET_FILL;
    case LLRenderCapability::PolygonOffsetLine:
        return GL_POLYGON_OFFSET_LINE;
    case LLRenderCapability::ScissorTest:
        return GL_SCISSOR_TEST;
    case LLRenderCapability::StencilTest:
        return GL_STENCIL_TEST;
    case LLRenderCapability::TextureGenS:
        return GL_TEXTURE_GEN_S;
    case LLRenderCapability::TextureGenT:
        return GL_TEXTURE_GEN_T;
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
    case LLRenderBufferUsage::StreamDraw:
        return GL_STREAM_DRAW;
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

LLGLenum to_opengl_framebuffer_bind_point(LLRenderFramebufferBindPoint target)
{
    switch (target)
    {
    case LLRenderFramebufferBindPoint::Read:
        return GL_READ_FRAMEBUFFER;
    case LLRenderFramebufferBindPoint::Draw:
        return GL_DRAW_FRAMEBUFFER;
    case LLRenderFramebufferBindPoint::ReadWrite:
        return GL_FRAMEBUFFER;
    default:
        return GL_FRAMEBUFFER;
    }
}

LLGLenum to_opengl_query_target(LLRenderQueryTarget target)
{
    switch (target)
    {
    case LLRenderQueryTarget::AnySamplesPassed:
#ifdef GL_ANY_SAMPLES_PASSED
        return GL_ANY_SAMPLES_PASSED;
#else
        return GL_SAMPLES_PASSED;
#endif
    case LLRenderQueryTarget::TimeElapsed:
        return GL_TIME_ELAPSED;
    case LLRenderQueryTarget::SamplesPassed:
        return GL_SAMPLES_PASSED;
    case LLRenderQueryTarget::PrimitivesGenerated:
        return GL_PRIMITIVES_GENERATED;
    case LLRenderQueryTarget::TransformFeedbackPrimitivesWritten:
        return GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN;
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
    case LLRenderShaderStage::Geometry:
        return GL_GEOMETRY_SHADER;
    case LLRenderShaderStage::Compute:
#if !LL_DARWIN
        return GL_COMPUTE_SHADER;
#else
        return 0;
#endif
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

LLGLenum to_opengl_texture_format(LLRenderTextureFormat format)
{
    switch (format)
    {
    case LLRenderTextureFormat::None:
        return 0;
    case LLRenderTextureFormat::Alpha:
        return GL_ALPHA;
    case LLRenderTextureFormat::Alpha8:
        return GL_ALPHA8;
    case LLRenderTextureFormat::R8:
        return GL_R8;
    case LLRenderTextureFormat::R16F:
        return GL_R16F;
    case LLRenderTextureFormat::R32F:
        return GL_R32F;
    case LLRenderTextureFormat::RG8:
        return GL_RG8;
    case LLRenderTextureFormat::RG16F:
        return GL_RG16F;
    case LLRenderTextureFormat::RG32F:
        return GL_RG32F;
    case LLRenderTextureFormat::RGB:
        return GL_RGB;
    case LLRenderTextureFormat::RGB8:
        return GL_RGB8;
    case LLRenderTextureFormat::RGB16F:
        return GL_RGB16F;
    case LLRenderTextureFormat::RGB10A2:
        return GL_RGB10_A2;
    case LLRenderTextureFormat::R11G11B10F:
        return GL_R11F_G11F_B10F;
    case LLRenderTextureFormat::RGBA:
        return GL_RGBA;
    case LLRenderTextureFormat::RGBA8:
        return GL_RGBA8;
    case LLRenderTextureFormat::RGBA16:
        return GL_RGBA16;
    case LLRenderTextureFormat::RGBA16F:
        return GL_RGBA16F;
    case LLRenderTextureFormat::DepthComponent:
        return GL_DEPTH_COMPONENT;
    case LLRenderTextureFormat::DepthComponent24:
        return GL_DEPTH_COMPONENT24;
    case LLRenderTextureFormat::Luminance:
        return GL_LUMINANCE;
    default:
        return GL_RGBA;
    }
}

LLGLenum to_opengl_pixel_format(LLRenderPixelFormat format)
{
    switch (format)
    {
    case LLRenderPixelFormat::Alpha:
        return GL_ALPHA;
    case LLRenderPixelFormat::DepthComponent:
        return GL_DEPTH_COMPONENT;
    case LLRenderPixelFormat::Luminance:
        return GL_LUMINANCE;
    case LLRenderPixelFormat::Red:
        return GL_RED;
    case LLRenderPixelFormat::RG:
        return GL_RG;
    case LLRenderPixelFormat::RGB:
        return GL_RGB;
    case LLRenderPixelFormat::RGBA:
        return GL_RGBA;
    default:
        return GL_RGBA;
    }
}

LLGLenum to_opengl_pixel_format_for_texture(LLRenderTextureFormat format)
{
    switch (format)
    {
    case LLRenderTextureFormat::Alpha:
    case LLRenderTextureFormat::Alpha8:
        return GL_ALPHA;
    case LLRenderTextureFormat::DepthComponent:
    case LLRenderTextureFormat::DepthComponent24:
        return GL_DEPTH_COMPONENT;
    case LLRenderTextureFormat::Luminance:
        return GL_LUMINANCE;
    case LLRenderTextureFormat::R8:
    case LLRenderTextureFormat::R16F:
    case LLRenderTextureFormat::R32F:
        return GL_RED;
    case LLRenderTextureFormat::RG8:
    case LLRenderTextureFormat::RG16F:
    case LLRenderTextureFormat::RG32F:
        return GL_RG;
    case LLRenderTextureFormat::RGB:
    case LLRenderTextureFormat::RGB8:
    case LLRenderTextureFormat::RGB16F:
    case LLRenderTextureFormat::RGB10A2:
    case LLRenderTextureFormat::R11G11B10F:
        return GL_RGB;
    case LLRenderTextureFormat::RGBA:
    case LLRenderTextureFormat::RGBA8:
    case LLRenderTextureFormat::RGBA16:
    case LLRenderTextureFormat::RGBA16F:
    default:
        return GL_RGBA;
    }
}

LLGLenum to_opengl_pixel_type(LLRenderPixelType type)
{
    switch (type)
    {
    case LLRenderPixelType::UnsignedByte:
        return GL_UNSIGNED_BYTE;
    case LLRenderPixelType::UnsignedShort:
        return GL_UNSIGNED_SHORT;
    case LLRenderPixelType::UnsignedInt:
        return GL_UNSIGNED_INT;
    case LLRenderPixelType::Float32:
        return GL_FLOAT;
    default:
        return GL_UNSIGNED_BYTE;
    }
}

LLGLenum to_opengl_pixel_type_for_texture(LLRenderTextureFormat format)
{
    switch (format)
    {
    case LLRenderTextureFormat::R16F:
    case LLRenderTextureFormat::R32F:
    case LLRenderTextureFormat::RG16F:
    case LLRenderTextureFormat::RG32F:
    case LLRenderTextureFormat::RGB16F:
    case LLRenderTextureFormat::RGBA16F:
        return GL_FLOAT;
    case LLRenderTextureFormat::DepthComponent:
    case LLRenderTextureFormat::DepthComponent24:
        return GL_UNSIGNED_INT;
    case LLRenderTextureFormat::Alpha:
    case LLRenderTextureFormat::Alpha8:
    case LLRenderTextureFormat::R8:
    case LLRenderTextureFormat::RG8:
    case LLRenderTextureFormat::RGB:
    case LLRenderTextureFormat::RGB8:
    case LLRenderTextureFormat::RGB10A2:
    case LLRenderTextureFormat::R11G11B10F:
    case LLRenderTextureFormat::RGBA:
    case LLRenderTextureFormat::RGBA8:
    case LLRenderTextureFormat::RGBA16:
    case LLRenderTextureFormat::Luminance:
    default:
        return GL_UNSIGNED_BYTE;
    }
}

[[maybe_unused]] LLGLenum to_opengl_image_access(LLRenderImageAccess access)
{
    switch (access)
    {
    case LLRenderImageAccess::WriteOnly:
        return GL_WRITE_ONLY;
    default:
        return GL_WRITE_ONLY;
    }
}

LLGLenum to_opengl_polygon_face(LLRenderPolygonFace face)
{
    switch (face)
    {
    case LLRenderPolygonFace::FrontAndBack:
        return GL_FRONT_AND_BACK;
    default:
        return GL_FRONT_AND_BACK;
    }
}

LLGLenum to_opengl_polygon_mode(LLRenderPolygonMode mode)
{
    switch (mode)
    {
    case LLRenderPolygonMode::Fill:
        return GL_FILL;
    case LLRenderPolygonMode::Line:
        return GL_LINE;
    default:
        return GL_FILL;
    }
}

LLGLenum to_opengl_stencil_function(LLRenderStencilFunction function)
{
    switch (function)
    {
    case LLRenderStencilFunction::Always:
        return GL_ALWAYS;
    case LLRenderStencilFunction::Equal:
        return GL_EQUAL;
    default:
        return GL_ALWAYS;
    }
}

LLGLenum to_opengl_stencil_operation(LLRenderStencilOperation operation)
{
    switch (operation)
    {
    case LLRenderStencilOperation::Keep:
        return GL_KEEP;
    case LLRenderStencilOperation::Replace:
        return GL_REPLACE;
    default:
        return GL_KEEP;
    }
}

LLGLenum to_opengl_matrix_mode(LLRenderMatrixMode mode)
{
    switch (mode)
    {
    case LLRenderMatrixMode::ModelView:
        return GL_MODELVIEW;
    default:
        return GL_MODELVIEW;
    }
}

LLGLenum to_opengl_texture_coordinate(LLRenderTextureCoordinate coordinate)
{
    switch (coordinate)
    {
    case LLRenderTextureCoordinate::S:
        return GL_S;
    case LLRenderTextureCoordinate::T:
        return GL_T;
    default:
        return GL_S;
    }
}

LLGLenum to_opengl_info_string(LLRenderInfoString parameter)
{
    switch (parameter)
    {
    case LLRenderInfoString::Vendor:
        return GL_VENDOR;
    case LLRenderInfoString::Renderer:
        return GL_RENDERER;
    case LLRenderInfoString::Version:
        return GL_VERSION;
    default:
        return GL_VERSION;
    }
}

LLGLenum to_opengl_integer_parameter(LLRenderIntegerParameter parameter)
{
    switch (parameter)
    {
    case LLRenderIntegerParameter::DedicatedVideoMemoryKB:
        return GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX;
    case LLRenderIntegerParameter::FreeVideoMemoryKB:
        return GL_VBO_FREE_MEMORY_ATI;
    case LLRenderIntegerParameter::RedBits:
        return GL_RED_BITS;
    case LLRenderIntegerParameter::GreenBits:
        return GL_GREEN_BITS;
    case LLRenderIntegerParameter::BlueBits:
        return GL_BLUE_BITS;
    case LLRenderIntegerParameter::AlphaBits:
        return GL_ALPHA_BITS;
    case LLRenderIntegerParameter::DepthBits:
        return GL_DEPTH_BITS;
    case LLRenderIntegerParameter::StencilBits:
        return GL_STENCIL_BITS;
    default:
        return GL_RED_BITS;
    }
}

[[maybe_unused]] U32 to_opengl_memory_barriers(LLRenderMemoryBarrierMask barriers)
{
    U32 gl_barriers = 0;
#if !LL_DARWIN
    if (barriers & LL_RENDER_MEMORY_BARRIER_SHADER_IMAGE_ACCESS)
    {
        gl_barriers |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
    }
    if (barriers & LL_RENDER_MEMORY_BARRIER_TEXTURE_FETCH)
    {
        gl_barriers |= GL_TEXTURE_FETCH_BARRIER_BIT;
    }
#endif
    return gl_barriers;
}

LLRenderFramebufferStatus to_render_framebuffer_status(LLGLenum status)
{
    switch (status)
    {
    case GL_FRAMEBUFFER_COMPLETE:
        return LLRenderFramebufferStatus::Complete;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return LLRenderFramebufferStatus::IncompleteMissingAttachment;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return LLRenderFramebufferStatus::IncompleteAttachment;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return LLRenderFramebufferStatus::Unsupported;
    default:
        return LLRenderFramebufferStatus::Unknown;
    }
}

#if LL_DARWIN
U32 get_darwin_vram_megabytes()
{
    S32 num_renderers = 0;
    U32 vram_megabytes = 0;
    CGLRendererInfoObj info = nullptr;
    CGLError error = CGLQueryRendererInfo(
        CGDisplayIDToOpenGLDisplayMask(CGMainDisplayID()),
        &info,
        &num_renderers);

    if (error == kCGLNoError)
    {
        CGLDescribeRenderer(info, 0, kCGLRPVideoMemoryMegabytes, reinterpret_cast<GLint*>(&vram_megabytes));
        CGLDestroyRendererInfo(info);
    }
    else
    {
        vram_megabytes = 256;
    }

    return vram_megabytes;
}
#endif

class LLOpenGLRenderBackend final : public LLRenderBackend
{
public:
    LLRenderBackendType getType() const override { return LLRenderBackendType::OpenGL; }
    const char* getName() const override { return "OpenGL"; }
    bool isReady() const override { return true; }
    void initPlatformContextExtensions() override { gGLManager.initWGL(); }
    bool initContextCapabilities() override { return gGLManager.initGL(); }
    void shutdownContextCapabilities() override { gGLManager.shutdownGL(); }
    bool createNativeContext(
        const LLRenderNativeContextDesc& desc,
        LLRenderNativeContext& context) override
    {
#if LL_DARWIN
        if (!desc.mWindow)
        {
            return false;
        }

        context.mView = ll_render_macosx_create_native_view(desc.mWindow);
        if (!context.mView)
        {
            return false;
        }

        CGLPixelFormatAttribute attribs[] =
        {
            kCGLPFANoRecovery,
            kCGLPFADoubleBuffer,
            kCGLPFAClosestPolicy,
            kCGLPFAAccelerated,
            kCGLPFAMultisample,
            kCGLPFASampleBuffers, static_cast<CGLPixelFormatAttribute>((desc.mSamples > 0 ? 1 : 0)),
            kCGLPFASamples, static_cast<CGLPixelFormatAttribute>(desc.mSamples),
            kCGLPFAStencilSize, static_cast<CGLPixelFormatAttribute>(8),
            kCGLPFADepthSize, static_cast<CGLPixelFormatAttribute>(24),
            kCGLPFAAlphaSize, static_cast<CGLPixelFormatAttribute>(8),
            kCGLPFAColorSize, static_cast<CGLPixelFormatAttribute>(24),
            kCGLPFAOpenGLProfile, static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_GL4_Core),
            static_cast<CGLPixelFormatAttribute>(0)
        };

        S32 num_pixel_formats = 0;
        CGLPixelFormatObj pixel_format = nullptr;
        CGLChoosePixelFormat(attribs, &pixel_format, &num_pixel_formats);

        if (pixel_format == nullptr)
        {
            CGLChoosePixelFormat(attribs, &pixel_format, &num_pixel_formats);
        }

        if (pixel_format == nullptr)
        {
            ll_render_macosx_destroy_native_view(context.mView);
            context = {};
            return false;
        }

        context.mPixelFormat = pixel_format;

        if (!ll_render_macosx_attach_native_context(
                context.mView,
                context.mPixelFormat,
                desc.mEnableVSync))
        {
            CGLDestroyPixelFormat(static_cast<CGLPixelFormatObj>(context.mPixelFormat));
            ll_render_macosx_destroy_native_view(context.mView);
            context = {};
            return false;
        }

        context.mContext = ll_render_macosx_get_native_context(context.mView);
        context.mVRAM = context.mContext ? get_darwin_vram_megabytes() : 0;

        return context.mContext != nullptr;
#else
        return false;
#endif
    }

    void destroyNativeContext(LLRenderNativeContext& context) override
    {
#if LL_DARWIN
        if (context.mView)
        {
            ll_render_macosx_destroy_native_view(context.mView);
        }

        if (context.mPixelFormat)
        {
            CGLDestroyPixelFormat(static_cast<CGLPixelFormatObj>(context.mPixelFormat));
        }
#endif

        context = {};
    }

    bool makeNativeContextCurrent(void* context) override
    {
#if LL_DARWIN
        return CGLSetCurrentContext(static_cast<CGLContextObj>(context)) == kCGLNoError;
#else
        return false;
#endif
    }

    void clearCurrentNativeContext() override
    {
#if LL_DARWIN
        CGLSetCurrentContext(nullptr);
#endif
    }

    void swapNativeBuffers(void* context) override
    {
#if LL_DARWIN
        CGLFlushDrawable(static_cast<CGLContextObj>(context));
#endif
    }

    void setNativeVSync(void* context, bool enable_vsync) override
    {
#if LL_DARWIN
        S32 frames_per_swap = enable_vsync ? 1 : 0;
        CGLSetParameter(static_cast<CGLContextObj>(context), kCGLCPSwapInterval, &frames_per_swap);
#endif
    }

    bool setNativeContextThreadedOptimization(bool enabled) override
    {
#if LL_DARWIN
        CGLContextObj ctx = CGLGetCurrentContext();
        if (!ctx)
        {
            return false;
        }

        if (enabled)
        {
            CGLError cgl_err = CGLEnable(ctx, kCGLCEMPEngine);
            if (cgl_err != kCGLNoError)
            {
                LL_INFOS("GLInit") << "Multi-threaded OpenGL not available." << LL_ENDL;
                return false;
            }
            else
            {
                LL_INFOS("GLInit") << "Multi-threaded OpenGL enabled." << LL_ENDL;
            }
        }
        else
        {
            CGLDisable(ctx, kCGLCEMPEngine);
            LL_INFOS("GLInit") << "Multi-threaded OpenGL disabled." << LL_ENDL;
        }
        return true;
#else
        return false;
#endif
    }

    void* createSharedNativeContext(
        void* pixel_format,
        void* share_context,
        bool enable_threaded_optimization) override
    {
#if LL_DARWIN
        CGLContextObj context = nullptr;
        CGLCreateContext(
            static_cast<CGLPixelFormatObj>(pixel_format),
            static_cast<CGLContextObj>(share_context),
            &context);

        if (enable_threaded_optimization && share_context)
        {
            CGLEnable(static_cast<CGLContextObj>(share_context), kCGLCEMPEngine);
        }

        return context;
#else
        return nullptr;
#endif
    }

    void destroySharedNativeContext(void* context) override
    {
#if LL_DARWIN
        CGLDestroyContext(static_cast<CGLContextObj>(context));
#endif
    }

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

    void setViewport(S32 x, S32 y, S32 width, S32 height) override
    {
        LLGLContainment::setViewport(x, y, width, height);
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

    void setScissor(S32 x, S32 y, S32 width, S32 height) override
    {
        LLGLContainment::setScissorBox(x, y, width, height);
    }

    void clear(const LLRenderPassDesc& desc) override
    {
        LLGLContainment::clearBuffersByIntent(
            (desc.mClearMask & LL_RENDER_CLEAR_COLOR) != 0,
            (desc.mClearMask & LL_RENDER_CLEAR_DEPTH) != 0,
            (desc.mClearMask & LL_RENDER_CLEAR_STENCIL) != 0);
    }

    void clear(LLRenderClearMask clear_mask) override
    {
        LLGLContainment::clearBuffersByIntent(
            (clear_mask & LL_RENDER_CLEAR_COLOR) != 0,
            (clear_mask & LL_RENDER_CLEAR_DEPTH) != 0,
            (clear_mask & LL_RENDER_CLEAR_STENCIL) != 0);
    }

    void setClearColor(const LLRenderClearColor& color) override
    {
        LLGLContainment::setClearColor(color.mRed, color.mGreen, color.mBlue, color.mAlpha);
    }

    void setClearColor(F32 red, F32 green, F32 blue, F32 alpha) override
    {
        LLGLContainment::setClearColor(red, green, blue, alpha);
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

    void setPointSize(F32 size) override
    {
        LLGLContainment::setPointSize(size);
    }

    F32 getLineWidth() override
    {
        LLGLfloat width = 1.f;
        LLGLContainment::getFloat(GL_LINE_WIDTH, &width);
        return width;
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

    void setAlphaMaskCutoff(F32) override {}
    void setWorldDrawEnabled(bool) override {}
    bool isWorldDrawEnabled() const override { return false; }
    void setWorldShaderClass(LLRenderWorldShaderClass) override {}
    void setWorldDeferredShaderLevel(S32) override {}
    void setWorldTerrainParameters(const LLRenderWorldTerrainParameters&) override {}
    void setWorldMaterialParameters(const LLRenderWorldMaterialParameters&) override {}
    void setWorldTextureTransform(const LLRenderWorldTextureTransform&) override {}
    void setWorldSkinningMatrixPalette(U32, const F32*) override {}

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

    void setTextureAddressMode(
        LLRenderTextureTarget target,
        LLRenderTextureCoordinate coordinate,
        LLRenderTextureAddressMode mode) override
    {
        LLGLenum gl_parameter = coordinate == LLRenderTextureCoordinate::T ?
            GL_TEXTURE_WRAP_T :
            GL_TEXTURE_WRAP_S;

        LLGLContainment::setTextureParameterInteger(
            to_opengl_texture_target(target),
            gl_parameter,
            to_opengl_texture_address_mode(mode));
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

    void setTextureMagFilter(
        LLRenderTextureTarget target,
        LLRenderTextureFilter mag_filter) override
    {
        LLGLContainment::setTextureParameterInteger(
            to_opengl_texture_target(target),
            GL_TEXTURE_MAG_FILTER,
            to_opengl_texture_filter(mag_filter));
    }

    void setTextureCompareMode(LLRenderTextureTarget target, bool enabled) override
    {
        LLGLenum gl_target = to_opengl_texture_target(target);
        LLGLContainment::setTextureParameterInteger(
            gl_target,
            GL_TEXTURE_COMPARE_MODE,
            enabled ? GL_COMPARE_R_TO_TEXTURE : GL_NONE);
        if (enabled)
        {
            LLGLContainment::setTextureParameterInteger(
                gl_target,
                GL_TEXTURE_COMPARE_FUNC,
                GL_LEQUAL);
        }
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

    void bindBufferBase(LLRenderBufferTarget target, U32 index, U32 buffer) override
    {
        LLGLContainment::bindBufferBase(to_opengl_buffer_target(target), index, buffer);
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

    void drawElements(
        LLRenderPrimitiveType mode,
        S32 count,
        LLRenderIndexType index_type,
        const void* indices) override
    {
        LLGLContainment::drawElements(
            to_opengl_primitive_type(mode),
            count,
            to_opengl_index_type(index_type),
            indices);
    }

    void setLegacyVertexPointer(
        S32 size,
        LLRenderVertexAttributeType type,
        S32 stride,
        const void* pointer) override
    {
        LLGLContainment::setVertexPointer(
            size,
            to_opengl_vertex_attribute_type(type),
            stride,
            pointer);
    }

    void setLegacyTextureCoordinatePointer(
        S32 size,
        LLRenderVertexAttributeType type,
        S32 stride,
        const void* pointer) override
    {
        LLGLContainment::setTextureCoordinatePointer(
            size,
            to_opengl_vertex_attribute_type(type),
            stride,
            pointer);
    }

    void setLegacyTextureCoordinateArray(bool enabled) override
    {
        if (enabled)
        {
            LLGLContainment::enableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        else
        {
            LLGLContainment::disableClientState(GL_TEXTURE_COORD_ARRAY);
        }
    }

    void generateFramebuffers(S32 count, U32* framebuffers) override
    {
        LLGLContainment::generateFramebuffers(count, framebuffers);
    }

    void deleteFramebuffers(S32 count, const U32* framebuffers) override
    {
        LLGLContainment::deleteFramebuffers(count, framebuffers);
    }

    void bindFramebuffer(LLRenderFramebufferBindPoint target, U32 framebuffer) override
    {
        LLGLContainment::bindFramebuffer(
            to_opengl_framebuffer_bind_point(target),
            framebuffer);
    }

    void bindReadWriteFramebuffer(U32 framebuffer) override
    {
        LLGLContainment::bindReadWriteFramebuffer(framebuffer);
    }

    LLRenderFramebufferStatus getReadWriteFramebufferStatus() const override
    {
        return to_render_framebuffer_status(
            LLGLContainment::getFramebufferStatus(GL_FRAMEBUFFER_EXT));
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

    void setFramebufferReadColorAttachment(U32 attachment) override
    {
        const LLRenderFramebufferAttachment framebuffer_attachment =
            attachment == 1 ?
                LLRenderFramebufferAttachment::Color1 :
                (attachment == 2 ?
                    LLRenderFramebufferAttachment::Color2 :
                    (attachment == 3 ?
                        LLRenderFramebufferAttachment::Color3 :
                        LLRenderFramebufferAttachment::Color0));
        LLGLContainment::setReadBuffer(
            to_opengl_framebuffer_attachment(framebuffer_attachment));
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

    void getQueryObjectUnsignedInteger(
        U32 query,
        LLRenderQueryParameter parameter,
        U32* value) override
    {
        LLGLContainment::getQueryObjectUnsignedInteger(
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

    void bindTextureUnit(U32 unit, LLRenderTextureHandle texture) override
    {
#if !LL_DARWIN
        LLGLContainment::bindTextureUnit(unit, texture.asLegacyName());
#else
        setActiveTextureUnit(static_cast<S32>(unit));
        bindTexture(LLRenderTextureTarget::Texture2D, texture.asLegacyName());
#endif
    }

    void bindImageTexture(
        U32 unit,
        LLRenderTextureHandle texture,
        S32 level,
        bool layered,
        S32 layer,
        LLRenderImageAccess access,
        LLRenderTextureFormat format) override
    {
#if !LL_DARWIN
        LLGLContainment::bindImageTexture(
            unit,
            texture.asLegacyName(),
            level,
            static_cast<LLGLboolean>(layered),
            layer,
            to_opengl_image_access(access),
            to_opengl_texture_format(format));
#endif
    }

    void dispatchCompute(U32 groups_x, U32 groups_y, U32 groups_z) override
    {
#if !LL_DARWIN
        LLGLContainment::dispatchCompute(groups_x, groups_y, groups_z);
#endif
    }

    void setMemoryBarrier(LLRenderMemoryBarrierMask barriers) override
    {
#if !LL_DARWIN
        LLGLContainment::setMemoryBarrier(to_opengl_memory_barriers(barriers));
#endif
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

    void setTextureImage2D(
        LLRenderTextureTarget target,
        S32 level,
        LLRenderTextureFormat internal_format,
        S32 width,
        S32 height,
        S32 border,
        LLRenderPixelFormat format,
        LLRenderPixelType type,
        const void* data) override
    {
        LLGLContainment::setTextureImage2D(
            to_opengl_texture_target(target),
            level,
            to_opengl_texture_format(internal_format),
            width,
            height,
            border,
            to_opengl_pixel_format(format),
            to_opengl_pixel_type(type),
            data);
    }

    LLRenderTextureHandle createTexture2D(LLRenderTextureFormat internal_format, S32 width, S32 height) override
    {
        U32 texture = 0;
#if !LL_DARWIN
        LLGLContainment::createTextures(GL_TEXTURE_2D, 1, &texture);
        LLGLContainment::setTextureStorage2D(
            texture,
            1,
            to_opengl_texture_format(internal_format),
            width,
            height);
        LLGLContainment::setNamedTextureParameterInteger(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        LLGLContainment::setNamedTextureParameterInteger(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        LLGLContainment::setNamedTextureParameterInteger(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        LLGLContainment::setNamedTextureParameterInteger(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
        generateTextures(1, &texture);
        bindTexture(LLRenderTextureTarget::Texture2D, texture);
        LLGLContainment::setTextureImage2D(
            GL_TEXTURE_2D,
            0,
            to_opengl_texture_format(internal_format),
            width,
            height,
            0,
            to_opengl_pixel_format_for_texture(internal_format),
            to_opengl_pixel_type_for_texture(internal_format),
            nullptr);
        setTextureFilter(
            LLRenderTextureTarget::Texture2D,
            LLRenderTextureFilter::Linear,
            LLRenderTextureFilter::Linear);
        setTextureAddressMode(
            LLRenderTextureTarget::Texture2D,
            LLRenderTextureAddressMode::ClampToEdge);
#endif
        return LLRenderTextureHandle(texture);
    }

    void readPixels(
        S32 x,
        S32 y,
        S32 width,
        S32 height,
        LLRenderPixelFormat format,
        LLRenderPixelType type,
        void* pixels) override
    {
        LLGLContainment::readPixels(
            x,
            y,
            width,
            height,
            to_opengl_pixel_format(format),
            to_opengl_pixel_type(type),
            pixels);
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

    void readTextureImage(
        LLRenderTextureTarget target,
        S32 level,
        LLRenderPixelFormat format,
        LLRenderPixelType type,
        void* pixels) override
    {
        LLGLContainment::readTextureImage(
            to_opengl_texture_target(target),
            level,
            to_opengl_pixel_format(format),
            to_opengl_pixel_type(type),
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

    void copyTextureSubImage3D(
        LLRenderTextureTarget target,
        S32 level,
        S32 xoffset,
        S32 yoffset,
        S32 zoffset,
        S32 x,
        S32 y,
        S32 width,
        S32 height) override
    {
        LLGLContainment::copyTextureSubImage3D(
            to_opengl_texture_target(target),
            level,
            xoffset,
            yoffset,
            zoffset,
            x,
            y,
            width,
            height);
    }

    void copyImageSubData(
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
        S32 depth) override
    {
#if !LL_DARWIN
        LLGLContainment::copyImageSubData(
            source_texture.asLegacyName(),
            to_opengl_texture_target(source_target),
            source_level,
            source_x,
            source_y,
            source_z,
            destination_texture.asLegacyName(),
            to_opengl_texture_target(destination_target),
            destination_level,
            destination_x,
            destination_y,
            destination_z,
            width,
            height,
            depth);
#endif
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

    void setTextureGenerationMode(
        LLRenderTextureCoordinate coordinate,
        bool object_linear) override
    {
        LLGLContainment::setTextureGenerationInteger(
            to_opengl_texture_coordinate(coordinate),
            GL_TEXTURE_GEN_MODE,
            object_linear ? GL_OBJECT_LINEAR : GL_EYE_LINEAR);
    }

    void setTextureGenerationObjectPlane(
        LLRenderTextureCoordinate coordinate,
        const F32* values) override
    {
        LLGLContainment::setTextureGenerationFloatVector(
            to_opengl_texture_coordinate(coordinate),
            GL_OBJECT_PLANE,
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

    void getInteger(LLRenderIntegerParameter parameter, S32* value) override
    {
        LLGLContainment::getInteger(to_opengl_integer_parameter(parameter), value);
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

    void setPolygonOffset(F32 factor, F32 units) override
    {
        LLGLContainment::setPolygonOffset(factor, units);
    }

    void setPolygonMode(LLRenderPolygonFace face, LLRenderPolygonMode mode) override
    {
        LLGLContainment::setPolygonMode(
            to_opengl_polygon_face(face),
            to_opengl_polygon_mode(mode));
    }

    void setStencilFunction(LLRenderStencilFunction function, S32 reference, U32 mask) override
    {
        LLGLContainment::setStencilFunction(
            to_opengl_stencil_function(function),
            reference,
            mask);
    }

    void setStencilMask(U32 mask) override
    {
        LLGLContainment::setStencilMask(mask);
    }

    void setStencilOperation(
        LLRenderStencilOperation stencil_fail,
        LLRenderStencilOperation depth_fail,
        LLRenderStencilOperation depth_pass) override
    {
        LLGLContainment::setStencilOperation(
            to_opengl_stencil_operation(stencil_fail),
            to_opengl_stencil_operation(depth_fail),
            to_opengl_stencil_operation(depth_pass));
    }

    void setMatrixMode(LLRenderMatrixMode mode) override
    {
        LLGLContainment::setMatrixMode(to_opengl_matrix_mode(mode));
    }

    void pushMatrix() override
    {
        LLGLContainment::pushMatrix();
    }

    void popMatrix() override
    {
        LLGLContainment::popMatrix();
    }

    const char* getInfoString(LLRenderInfoString parameter) override
    {
        return LLGLContainment::getString(to_opengl_info_string(parameter));
    }
};
}

U32 getOpenGLPixelFormatValue(LLRenderPixelFormat format)
{
    return to_opengl_pixel_format(format);
}

U32 getOpenGLPixelTypeValue(LLRenderPixelType type)
{
    return to_opengl_pixel_type(type);
}

LLRenderBackend& getOpenGLRenderBackend()
{
    static LLOpenGLRenderBackend backend;
    return backend;
}
