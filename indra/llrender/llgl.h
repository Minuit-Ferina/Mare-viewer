/**
 * @file llgl.h
 * @brief LLGL definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
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
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLGL_H
#define LL_LLGL_H

// This file contains various stuff for handling gl extensions and other gl related stuff.

#include "llglheaders.h"
#include "llrendercontext.h"
#include "llrenderstate.h"

// Deal with changing glext.h definitions for newer SDK versions, specifically
// with MAC OSX 10.5 -> 10.6


#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT GL_DEPTH_ATTACHMENT_EXT
#endif

#ifndef GL_STENCIL_ATTACHMENT
#define GL_STENCIL_ATTACHMENT GL_STENCIL_ATTACHMENT_EXT
#endif

#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER GL_FRAMEBUFFER_EXT
#define GL_DRAW_FRAMEBUFFER GL_DRAW_FRAMEBUFFER_EXT
#define GL_READ_FRAMEBUFFER GL_READ_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE_EXT
#define GL_FRAMEBUFFER_UNSUPPORTED GL_FRAMEBUFFER_UNSUPPORTED_EXT
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT
#define glGenFramebuffers glGenFramebuffersEXT
#define glBindFramebuffer glBindFramebufferEXT
#define glCheckFramebufferStatus glCheckFramebufferStatusEXT
#define glBlitFramebuffer glBlitFramebufferEXT
#define glDeleteFramebuffers glDeleteFramebuffersEXT
#define glFramebufferRenderbuffer glFramebufferRenderbufferEXT
#define glFramebufferTexture2D glFramebufferTexture2DEXT
#endif

#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER GL_RENDERBUFFER_EXT
#define glGenRenderbuffers glGenRenderbuffersEXT
#define glBindRenderbuffer glBindRenderbufferEXT
#define glRenderbufferStorage glRenderbufferStorageEXT
#define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleEXT
#define glDeleteRenderbuffers glDeleteRenderbuffersEXT
#endif

#ifndef GL_COLOR_ATTACHMENT
#define GL_COLOR_ATTACHMENT GL_COLOR_ATTACHMENT_EXT
#endif

#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0_EXT
#endif

#ifndef GL_COLOR_ATTACHMENT1
#define GL_COLOR_ATTACHMENT1 GL_COLOR_ATTACHMENT1_EXT
#endif

#ifndef GL_COLOR_ATTACHMENT2
#define GL_COLOR_ATTACHMENT2 GL_COLOR_ATTACHMENT2_EXT
#endif

#ifndef GL_COLOR_ATTACHMENT3
#define GL_COLOR_ATTACHMENT3 GL_COLOR_ATTACHMENT3_EXT
#endif


#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_EXT
#endif

#endif // LL_LLGL_H
