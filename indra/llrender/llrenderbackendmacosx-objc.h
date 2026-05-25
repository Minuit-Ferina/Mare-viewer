/**
 * @file llrenderbackendmacosx-objc.h
 * @brief macOS Objective-C bridge for backend-owned native render contexts.
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

#ifndef LL_LLRENDERBACKENDMACOSX_OBJC_H
#define LL_LLRENDERBACKENDMACOSX_OBJC_H

void* ll_render_macosx_create_native_view(void* window);
bool ll_render_macosx_attach_native_context(void* view, void* pixel_format, bool vsync);
void* ll_render_macosx_get_native_context(void* view);
void ll_render_macosx_destroy_native_view(void* view);
void* ll_render_macosx_create_metal_native_view(void* window);
void* ll_render_macosx_get_metal_layer(void* view);
bool ll_render_macosx_get_metal_layer_drawable_size(void* view, unsigned int* width, unsigned int* height);
bool ll_render_macosx_get_native_view_size(void* view, unsigned int* width, unsigned int* height);

#endif
