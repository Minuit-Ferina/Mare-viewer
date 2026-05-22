/**
 * @file llglcontainment.h
 * @brief Narrow OpenGL containment helpers.
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

#ifndef LL_LLGLCONTAINMENT_H
#define LL_LLGLCONTAINMENT_H

#include "stdtypes.h"
#include "llgltypes.h"

namespace LLGLContainment
{
using DebugMessageCallback = void (*)();

const char* getPhaseOneScope();

void bindReadWriteFramebuffer(U32 framebuffer_name);
void bindFramebuffer(LLGLenum target, U32 framebuffer_name);
U32 getDrawFramebufferStatus();
void setReadWriteFramebufferTexture2D(
    LLGLenum attachment,
    LLGLenum texture_target,
    LLGLuint texture_name,
    LLGLint mip_level);
void setDrawBuffer(LLGLenum buffer);
void setReadBuffer(LLGLenum buffer);
void setDrawBuffers(S32 count, const LLGLenum* buffers);
void generateFramebuffers(S32 count, LLGLuint* framebuffers);
void deleteFramebuffers(S32 count, const LLGLuint* framebuffers);
void generateTextures(S32 count, LLGLuint* textures);
void deleteTextures(S32 count, const LLGLuint* textures);
void setActiveTexture(LLGLenum texture);
void bindTexture(LLGLenum target, LLGLuint texture);
void generateBufferObjects(S32 count, LLGLuint* buffers);
void deleteBufferObjects(S32 count, const LLGLuint* buffers);
void bindBufferObject(LLGLenum target, LLGLuint buffer);
void bindBufferBase(LLGLenum target, LLGLuint index, LLGLuint buffer);
void allocateBufferObjectStorage(LLGLenum target, U64 size, const void* data, LLGLenum usage);
void updateBufferObjectSubData(LLGLenum target, U32 offset, U32 size, const void* data);
void generateQueries(S32 count, LLGLuint* queries);
void deleteQueries(S32 count, const LLGLuint* queries);
void beginQuery(LLGLenum target, LLGLuint query);
void endQuery(LLGLenum target);
void getQueryObjectUnsignedInteger(LLGLuint query, LLGLenum parameter, LLGLuint* value);
void getQueryObjectUnsignedInteger64(LLGLuint query, LLGLenum parameter, U64* value);
void enableVertexAttributeArray(LLGLuint location);
void disableVertexAttributeArray(LLGLuint location);
void setVertexAttributePointer(
    LLGLuint location,
    LLGLint size,
    LLGLenum type,
    LLGLboolean normalized,
    S32 stride,
    const void* pointer);
void setIntegerVertexAttributePointer(
    LLGLuint location,
    LLGLint size,
    LLGLenum type,
    S32 stride,
    const void* pointer);
void setVertexAttribute4(
    LLGLuint location,
    LLGLfloat first,
    LLGLfloat second,
    LLGLfloat third,
    LLGLfloat fourth);
void setVertexAttributeVector4(LLGLuint location, const LLGLfloat* values);
void setVertexPointer(LLGLint size, LLGLenum type, S32 stride, const void* pointer);
void enableClientState(LLGLenum array);
void disableClientState(LLGLenum array);
void setTextureCoordinatePointer(LLGLint size, LLGLenum type, S32 stride, const void* pointer);
void drawVertexBufferRange(
    LLGLenum mode,
    LLGLuint start,
    LLGLuint end,
    S32 count,
    LLGLenum index_type,
    const void* indices);
void drawVertexBufferArrays(LLGLenum mode, LLGLint first, S32 count);
void drawElements(LLGLenum mode, S32 count, LLGLenum index_type, const void* indices);
void getInteger(LLGLenum parameter, LLGLint* value);
void setPixelStoreInteger(LLGLenum parameter, LLGLint value);
void getFloat(LLGLenum parameter, LLGLfloat* value);
void setMatrixMode(LLGLenum mode);
void pushMatrix();
void popMatrix();
void setDebugMessageCallback(DebugMessageCallback callback, void* user_param);
bool hasVertexArrayGenerator();
LLGLint getUniformLocation(LLGLuint program, const char* name);
LLGLint getAttributeLocation(LLGLuint program, const char* name);
void bindAttributeLocation(LLGLuint program, LLGLuint index, const char* name);
LLGLuint createProgram();
void attachShader(LLGLuint program, LLGLuint shader);
LLGLuint createShader(LLGLenum type);
void setShaderSource(LLGLuint shader, S32 count, const char* const* strings);
void compileShader(LLGLuint shader);
void getAttachedShaders(
    LLGLuint program,
    S32 max_count,
    LLGLint* count,
    LLGLuint* shaders);
void detachShader(LLGLuint program, LLGLuint shader);
bool isShader(LLGLuint shader);
void deleteShader(LLGLuint shader);
void deleteProgram(LLGLuint program);
void getShaderInfoLog(LLGLuint shader, S32 buffer_size, LLGLint* length, char* info_log);
bool isProgram(LLGLuint program);
void linkProgram(LLGLuint program);
void validateProgram(LLGLuint program);
void setProgramParameterInteger(LLGLuint program, LLGLenum parameter, LLGLint value);
void setProgramBinary(LLGLuint program, LLGLenum binary_format, const void* binary, S32 length);
void getProgramBinary(
    LLGLuint program,
    S32 buffer_size,
    LLGLint* length,
    LLGLenum* binary_format,
    void* binary);
void getShaderInteger(LLGLuint shader, LLGLenum parameter, LLGLint* value);
void getProgramInteger(LLGLuint program, LLGLenum parameter, LLGLint* value);
void getProgramInfoLog(LLGLuint program, S32 buffer_size, LLGLint* length, char* info_log);
void getActiveUniform(
    LLGLuint program,
    LLGLuint index,
    S32 buffer_size,
    LLGLint* length,
    LLGLint* size,
    LLGLenum* type,
    char* name);
LLGLuint getUniformBlockIndex(LLGLuint program, const char* name);
void bindUniformBlock(LLGLuint program, LLGLuint block_index, LLGLuint binding);
void useProgram(LLGLuint program);
void setUniformInteger(LLGLint location, LLGLint value);
void setUniformInteger2(LLGLint location, LLGLint first, LLGLint second);
void setUniformIntegerVector(LLGLint location, S32 count, const LLGLint* values);
void setUniformIntegerVector4(LLGLint location, S32 count, const LLGLint* values);
void setUniformUnsignedIntegerVector4(LLGLint location, S32 count, const LLGLuint* values);
void setUniformFloat(LLGLint location, LLGLfloat value);
void setUniformFloat2(LLGLint location, LLGLfloat first, LLGLfloat second);
void setUniformFloat3(LLGLint location, LLGLfloat first, LLGLfloat second, LLGLfloat third);
void setUniformFloat4(
    LLGLint location,
    LLGLfloat first,
    LLGLfloat second,
    LLGLfloat third,
    LLGLfloat fourth);
void setUniformFloatVector(LLGLint location, S32 count, const LLGLfloat* values);
void setUniformFloatVector2(LLGLint location, S32 count, const LLGLfloat* values);
void setUniformFloatVector3(LLGLint location, S32 count, const LLGLfloat* values);
void setUniformFloatVector4(LLGLint location, S32 count, const LLGLfloat* values);
void setUniformMatrix2(LLGLint location, S32 count, LLGLboolean transpose, const LLGLfloat* values);
void setUniformMatrix3(LLGLint location, S32 count, LLGLboolean transpose, const LLGLfloat* values);
void setUniformMatrix3x4(LLGLint location, S32 count, LLGLboolean transpose, const LLGLfloat* values);
void setUniformMatrix4(LLGLint location, S32 count, LLGLboolean transpose, const LLGLfloat* values);
void setMaterialFloatVector(LLGLenum face, LLGLenum parameter, const LLGLfloat* values);
void setMaterialInteger(LLGLenum face, LLGLenum parameter, LLGLint value);
void pushAttributeBits(U32 bits);
void pushClientAttributeBits(U32 bits);
void popClientAttributes();
void popAttributes();
void setClearColor(LLGLfloat red, LLGLfloat green, LLGLfloat blue, LLGLfloat alpha);
void getTextureLevelParameterInteger(LLGLenum target, S32 level, LLGLenum parameter, LLGLint* value);
void setTextureSubImage2D(
    LLGLenum target,
    S32 level,
    S32 xoffset,
    S32 yoffset,
    S32 width,
    S32 height,
    LLGLenum format,
    LLGLenum type,
    const void* pixels);
void setTextureParameterInteger(
    LLGLenum target,
    LLGLenum parameter,
    LLGLint value);
void setTextureParameterFloat(
    LLGLenum target,
    LLGLenum parameter,
    LLGLfloat value);
void setTextureParameterIntegerVector(
    LLGLenum target,
    LLGLenum parameter,
    const LLGLint* values);
void setTextureGenerationInteger(LLGLenum coordinate, LLGLenum parameter, LLGLint value);
void setTextureGenerationFloatVector(LLGLenum coordinate, LLGLenum parameter, const LLGLfloat* values);
void setCompressedTextureImage2D(
    LLGLenum target,
    S32 level,
    LLGLint internal_format,
    S32 width,
    S32 height,
    S32 border,
    S32 image_size,
    const void* data);
void setTextureImage2D(
    LLGLenum target,
    S32 level,
    LLGLint internal_format,
    S32 width,
    S32 height,
    S32 border,
    LLGLenum format,
    LLGLenum type,
    const void* data);
void setTextureSubImage3D(
    LLGLenum target,
    S32 level,
    S32 xoffset,
    S32 yoffset,
    S32 zoffset,
    S32 width,
    S32 height,
    S32 depth,
    LLGLenum format,
    LLGLenum type,
    const void* pixels);
void setTextureImage3D(
    LLGLenum target,
    S32 level,
    LLGLint internal_format,
    S32 width,
    S32 height,
    S32 depth,
    S32 border,
    LLGLenum format,
    LLGLenum type,
    const void* data);
void areTexturesResident(S32 count, const LLGLuint* textures, LLGLboolean* residences);
void readCompressedTextureImage(LLGLenum target, S32 level, void* pixels);
void readTextureImage(LLGLenum target, S32 level, LLGLenum format, LLGLenum type, void* pixels);
void readPixels(
    LLGLint x,
    LLGLint y,
    S32 width,
    S32 height,
    LLGLenum format,
    LLGLenum type,
    void* pixels);
void copyTextureSubImage2D(
    LLGLenum target,
    S32 level,
    S32 xoffset,
    S32 yoffset,
    S32 x,
    S32 y,
    S32 width,
    S32 height);
void copyTextureSubImage3D(
    LLGLenum target,
    S32 level,
    S32 xoffset,
    S32 yoffset,
    S32 zoffset,
    LLGLint x,
    LLGLint y,
    S32 width,
    S32 height);
void copyTextureImage2D(
    LLGLenum target,
    S32 level,
    LLGLenum internal_format,
    LLGLint x,
    LLGLint y,
    S32 width,
    S32 height,
    S32 border);
LLGLsync createSyncObject();
void flushCommands();
void finishCommands();
void clientWaitSyncObject(LLGLsync sync);
void waitSyncObject(LLGLsync sync);
void deleteSyncObject(LLGLsync sync);
void generateTextureMipmap(LLGLenum texture_target);
void clearBuffers(U32 mask);
void setPolygonOffset(LLGLfloat factor, LLGLfloat units);
void setPolygonMode(LLGLenum face, LLGLenum mode);
void setScissorBox(LLGLint x, LLGLint y, U32 width, U32 height);
void setStencilFunction(LLGLenum function, LLGLint reference, LLGLuint mask);
void setStencilMask(LLGLuint mask);
void setStencilOperation(LLGLenum stencil_fail, LLGLenum depth_fail, LLGLenum depth_pass);
const char* getString(LLGLenum parameter);
LLGLenum getError();
void enableCapability(LLGLenum capability);
void disableCapability(LLGLenum capability);
bool isCapabilityEnabled(LLGLenum capability);
void setCullFace(LLGLenum mode);
void generateVertexArrays(S32 count, LLGLuint* arrays);
void bindVertexArray(LLGLuint array);
void setColorMask(LLGLboolean red, LLGLboolean green, LLGLboolean blue, LLGLboolean alpha);
void setColorUnsignedByteVector(const U8* values);
void setBlendFunction(LLGLenum source_factor, LLGLenum destination_factor);
void setSeparateBlendFunction(
    LLGLenum color_source_factor,
    LLGLenum color_destination_factor,
    LLGLenum alpha_source_factor,
    LLGLenum alpha_destination_factor);
void setLineWidth(LLGLfloat width);
void setPointSize(LLGLfloat size);
void setViewport(LLGLint x, LLGLint y, LLGLint width, LLGLint height);
}

#endif // LL_LLGLCONTAINMENT_H
