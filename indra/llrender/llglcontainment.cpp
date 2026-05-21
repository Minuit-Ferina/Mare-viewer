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
}
