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

void setPixelStoreInteger(LLGLenum parameter, LLGLint value)
{
    glPixelStorei(parameter, value);
}

void getTextureLevelParameterInteger(LLGLenum target, S32 level, LLGLenum parameter, LLGLint* value)
{
    glGetTexLevelParameteriv(target, level, parameter, value);
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

void setViewport(LLGLint x, LLGLint y, LLGLint width, LLGLint height)
{
    glViewport(x, y, width, height);
}
}
