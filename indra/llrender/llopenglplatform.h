/**
 * @file llopenglplatform.h
 * @brief Platform OpenGL headers owned by the OpenGL render backend boundary.
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

#ifndef LL_LLOPENGLPLATFORM_H
#define LL_LLOPENGLPLATFORM_H

#if LL_DARWIN
#include <OpenGL/OpenGL.h>
#endif

#if LL_LINUX
#define GLX_GLXEXT_PROTOTYPES 1
#include <GL/glx.h>
#endif

#if LL_MESA_HEADLESS
#include "GL/glu.h"
#include "GL/osmesa.h"
#endif

#endif // LL_LLOPENGLPLATFORM_H
