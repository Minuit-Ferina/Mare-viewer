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
const char* getPhaseOneScope();

void bindReadWriteFramebuffer(U32 framebuffer_name);
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
void generateTextureMipmap(LLGLenum texture_target);
void clearBuffers(U32 mask);
void setScissorBox(LLGLint x, LLGLint y, U32 width, U32 height);
}

#endif // LL_LLGLCONTAINMENT_H
