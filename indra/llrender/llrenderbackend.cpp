/**
 * @file llrenderbackend.cpp
 * @brief Common render backend entry points.
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
#include "llrenderbackendopengl.h"
#include "llrenderbackendvulkan.h"
#include "llstring.h"

namespace
{
LLRenderBackend& select_render_backend()
{
    std::string backend_name = LLStringUtil::getenv("MARE_RENDER_BACKEND");
    LLStringUtil::trim(backend_name);
    LLStringUtil::toLower(backend_name);

    if (backend_name == "vulkan" || backend_name == "vk")
    {
        LL_WARNS("RenderBackend")
            << "MARE_RENDER_BACKEND=vulkan selected. "
            << "The Vulkan backend is experimental and currently renders bootstrap UI draw commands."
            << LL_ENDL;
        return getVulkanRenderBackend();
    }

    if (!backend_name.empty() && backend_name != "opengl" && backend_name != "gl")
    {
        LL_WARNS("RenderBackend")
            << "Unknown MARE_RENDER_BACKEND value '" << backend_name
            << "'. Falling back to OpenGL."
            << LL_ENDL;
    }

    return getOpenGLRenderBackend();
}
}

LLRenderBackend::~LLRenderBackend() = default;

U64 LLRenderBackend::getTextureMemoryAllocatedBytes() const
{
    return 0;
}

U64 LLRenderBackend::getTextureMemoryBudgetBytes() const
{
    return 0;
}

U64 LLRenderBackend::getBufferMemoryAllocatedBytes() const
{
    return 0;
}

U64 LLRenderBackend::getBufferMemoryBudgetBytes() const
{
    return 0;
}

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

LLRenderBackend& getRenderBackend()
{
    static LLRenderBackend& backend = select_render_backend();
    return backend;
}
