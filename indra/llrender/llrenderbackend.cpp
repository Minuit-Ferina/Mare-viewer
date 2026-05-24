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
