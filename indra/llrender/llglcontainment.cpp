/**
 * @file llglcontainment.cpp
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

#include "linden_common.h"

#include "llglcontainment.h"

#include "llglheaders.h"

namespace LLGLContainment
{
const char* getPhaseOneScope()
{
    return "phase-1-inventory-only";
}

void bindReadWriteFramebuffer(U32 framebuffer_name)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_name);
}

U32 getDrawFramebufferStatus()
{
    return glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
}

void setReadWriteFramebufferTexture2D(
    LLGLenum attachment,
    LLGLenum texture_target,
    LLGLuint texture_name,
    LLGLint mip_level)
{
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texture_target, texture_name, mip_level);
}

void setDrawBuffer(LLGLenum buffer)
{
    glDrawBuffer(buffer);
}

void setReadBuffer(LLGLenum buffer)
{
    glReadBuffer(buffer);
}

void setDrawBuffers(S32 count, const LLGLenum* buffers)
{
    glDrawBuffers(static_cast<GLsizei>(count), buffers);
}

void generateFramebuffers(S32 count, LLGLuint* framebuffers)
{
    glGenFramebuffers(static_cast<GLsizei>(count), framebuffers);
}

void deleteFramebuffers(S32 count, const LLGLuint* framebuffers)
{
    glDeleteFramebuffers(static_cast<GLsizei>(count), framebuffers);
}

void generateTextures(S32 count, LLGLuint* textures)
{
    glGenTextures(static_cast<GLsizei>(count), textures);
}

void deleteTextures(S32 count, const LLGLuint* textures)
{
    glDeleteTextures(static_cast<GLsizei>(count), textures);
}

void setActiveTexture(LLGLenum texture)
{
    glActiveTexture(texture);
}

void bindTexture(LLGLenum target, LLGLuint texture)
{
    glBindTexture(target, texture);
}

void generateBufferObjects(S32 count, LLGLuint* buffers)
{
    glGenBuffers(static_cast<GLsizei>(count), buffers);
}

void deleteBufferObjects(S32 count, const LLGLuint* buffers)
{
    glDeleteBuffers(static_cast<GLsizei>(count), buffers);
}

void bindBufferObject(LLGLenum target, LLGLuint buffer)
{
    glBindBuffer(target, buffer);
}

void allocateBufferObjectStorage(LLGLenum target, U64 size, const void* data, LLGLenum usage)
{
    glBufferData(target, static_cast<GLsizeiptr>(size), data, usage);
}

void updateBufferObjectSubData(LLGLenum target, U32 offset, U32 size, const void* data)
{
    glBufferSubData(target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
}

void enableVertexAttributeArray(LLGLuint location)
{
    glEnableVertexAttribArray(location);
}

void disableVertexAttributeArray(LLGLuint location)
{
    glDisableVertexAttribArray(location);
}

void setVertexAttributePointer(
    LLGLuint location,
    LLGLint size,
    LLGLenum type,
    LLGLboolean normalized,
    S32 stride,
    const void* pointer)
{
    glVertexAttribPointer(
        location,
        size,
        type,
        static_cast<GLboolean>(normalized),
        static_cast<GLsizei>(stride),
        pointer);
}

void setIntegerVertexAttributePointer(
    LLGLuint location,
    LLGLint size,
    LLGLenum type,
    S32 stride,
    const void* pointer)
{
    glVertexAttribIPointer(
        location,
        size,
        type,
        static_cast<GLsizei>(stride),
        pointer);
}

void drawVertexBufferRange(
    LLGLenum mode,
    LLGLuint start,
    LLGLuint end,
    S32 count,
    LLGLenum index_type,
    const void* indices)
{
    glDrawRangeElements(
        mode,
        start,
        end,
        static_cast<GLsizei>(count),
        index_type,
        indices);
}

void drawVertexBufferArrays(LLGLenum mode, LLGLint first, S32 count)
{
    glDrawArrays(mode, first, static_cast<GLsizei>(count));
}

void getInteger(LLGLenum parameter, LLGLint* value)
{
    glGetIntegerv(parameter, value);
}

void setPixelStoreInteger(LLGLenum parameter, LLGLint value)
{
    glPixelStorei(parameter, value);
}

void getFloat(LLGLenum parameter, LLGLfloat* value)
{
    glGetFloatv(parameter, value);
}

LLGLint getUniformLocation(LLGLuint program, const char* name)
{
    return glGetUniformLocation(program, name);
}

void getShaderInteger(LLGLuint shader, LLGLenum parameter, LLGLint* value)
{
    glGetShaderiv(shader, parameter, value);
}

void getProgramInfoLog(LLGLuint program, S32 buffer_size, LLGLint* length, char* info_log)
{
    glGetProgramInfoLog(program, static_cast<GLsizei>(buffer_size), length, info_log);
}

void setMaterialFloatVector(LLGLenum face, LLGLenum parameter, const LLGLfloat* values)
{
    glMaterialfv(face, parameter, values);
}

void setMaterialInteger(LLGLenum face, LLGLenum parameter, LLGLint value)
{
    glMateriali(face, parameter, value);
}

void pushAttributeBits(U32 bits)
{
    glPushAttrib(bits);
}

void pushClientAttributeBits(U32 bits)
{
    glPushClientAttrib(bits);
}

void popClientAttributes()
{
    glPopClientAttrib();
}

void popAttributes()
{
    glPopAttrib();
}

void setClearColor(LLGLfloat red, LLGLfloat green, LLGLfloat blue, LLGLfloat alpha)
{
    glClearColor(red, green, blue, alpha);
}

void getTextureLevelParameterInteger(LLGLenum target, S32 level, LLGLenum parameter, LLGLint* value)
{
    glGetTexLevelParameteriv(target, level, parameter, value);
}

void setTextureSubImage2D(
    LLGLenum target,
    S32 level,
    S32 xoffset,
    S32 yoffset,
    S32 width,
    S32 height,
    LLGLenum format,
    LLGLenum type,
    const void* pixels)
{
    glTexSubImage2D(
        target,
        level,
        xoffset,
        yoffset,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        format,
        type,
        pixels);
}

void setTextureParameterInteger(
    LLGLenum target,
    LLGLenum parameter,
    LLGLint value)
{
    glTexParameteri(target, parameter, value);
}

void setTextureParameterFloat(
    LLGLenum target,
    LLGLenum parameter,
    LLGLfloat value)
{
    glTexParameterf(target, parameter, value);
}

void setTextureParameterIntegerVector(
    LLGLenum target,
    LLGLenum parameter,
    const LLGLint* values)
{
    glTexParameteriv(target, parameter, values);
}

void setCompressedTextureImage2D(
    LLGLenum target,
    S32 level,
    LLGLint internal_format,
    S32 width,
    S32 height,
    S32 border,
    S32 image_size,
    const void* data)
{
    glCompressedTexImage2D(
        target,
        level,
        internal_format,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        border,
        static_cast<GLsizei>(image_size),
        data);
}

void setTextureImage2D(
    LLGLenum target,
    S32 level,
    LLGLint internal_format,
    S32 width,
    S32 height,
    S32 border,
    LLGLenum format,
    LLGLenum type,
    const void* data)
{
    glTexImage2D(
        target,
        level,
        internal_format,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        border,
        format,
        type,
        data);
}

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
    const void* pixels)
{
    glTexSubImage3D(
        target,
        level,
        xoffset,
        yoffset,
        zoffset,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        static_cast<GLsizei>(depth),
        format,
        type,
        pixels);
}

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
    const void* data)
{
    glTexImage3D(
        target,
        level,
        internal_format,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        static_cast<GLsizei>(depth),
        border,
        format,
        type,
        data);
}

void areTexturesResident(S32 count, const LLGLuint* textures, LLGLboolean* residences)
{
    glAreTexturesResident(static_cast<GLsizei>(count), textures, residences);
}

void readCompressedTextureImage(LLGLenum target, S32 level, void* pixels)
{
    glGetCompressedTexImage(target, level, pixels);
}

void readTextureImage(LLGLenum target, S32 level, LLGLenum format, LLGLenum type, void* pixels)
{
    glGetTexImage(target, level, format, type, pixels);
}

void copyTextureSubImage2D(
    LLGLenum target,
    S32 level,
    S32 xoffset,
    S32 yoffset,
    S32 x,
    S32 y,
    S32 width,
    S32 height)
{
    glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

void copyTextureImage2D(
    LLGLenum target,
    S32 level,
    LLGLenum internal_format,
    LLGLint x,
    LLGLint y,
    S32 width,
    S32 height,
    S32 border)
{
    glCopyTexImage2D(
        target,
        level,
        internal_format,
        x,
        y,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        border);
}

LLGLsync createSyncObject()
{
    return glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void flushCommands()
{
    glFlush();
}

void clientWaitSyncObject(LLGLsync sync)
{
    glClientWaitSync(static_cast<GLsync>(sync), 0, GL_TIMEOUT_IGNORED);
}

void waitSyncObject(LLGLsync sync)
{
    glWaitSync(static_cast<GLsync>(sync), 0, GL_TIMEOUT_IGNORED);
}

void deleteSyncObject(LLGLsync sync)
{
    glDeleteSync(static_cast<GLsync>(sync));
}

void generateTextureMipmap(LLGLenum texture_target)
{
    glGenerateMipmap(texture_target);
}

void clearBuffers(U32 mask)
{
    glClear(mask);
}

void setScissorBox(LLGLint x, LLGLint y, U32 width, U32 height)
{
    glScissor(x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

LLGLenum getError()
{
    return glGetError();
}

void enableCapability(LLGLenum capability)
{
    glEnable(capability);
}

bool isCapabilityEnabled(LLGLenum capability)
{
    return glIsEnabled(capability) == GL_TRUE;
}

void setCullFace(LLGLenum mode)
{
    glCullFace(mode);
}

void generateVertexArrays(S32 count, LLGLuint* arrays)
{
    glGenVertexArrays(static_cast<GLsizei>(count), arrays);
}

void bindVertexArray(LLGLuint array)
{
    glBindVertexArray(array);
}

void setColorMask(LLGLboolean red, LLGLboolean green, LLGLboolean blue, LLGLboolean alpha)
{
    glColorMask(red, green, blue, alpha);
}

void setBlendFunction(LLGLenum source_factor, LLGLenum destination_factor)
{
    glBlendFunc(source_factor, destination_factor);
}

void setSeparateBlendFunction(
    LLGLenum color_source_factor,
    LLGLenum color_destination_factor,
    LLGLenum alpha_source_factor,
    LLGLenum alpha_destination_factor)
{
    glBlendFuncSeparate(
        color_source_factor,
        color_destination_factor,
        alpha_source_factor,
        alpha_destination_factor);
}

void setLineWidth(LLGLfloat width)
{
    glLineWidth(width);
}

void setViewport(LLGLint x, LLGLint y, LLGLint width, LLGLint height)
{
    glViewport(x, y, width, height);
}
}
