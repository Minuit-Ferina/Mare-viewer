/**
 * @file llrenderbackendvulkan.cpp
 * @brief Dormant Vulkan render backend implementation.
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

#include "llrenderbackendnull.h"

#if LL_WINDOWS
#include "llwin32headers.h"
#elif LL_DARWIN || LL_LINUX
#include <dlfcn.h>
#endif

#include <array>
#include <string>

namespace
{
constexpr U32 LL_VK_MAKE_API_VERSION(U32 variant, U32 major, U32 minor, U32 patch)
{
    return (variant << 29) | (major << 22) | (minor << 12) | patch;
}

constexpr S32 LL_VK_SUCCESS = 0;

constexpr U32 LL_VK_API_VERSION_MAJOR(U32 version)
{
    return (version >> 22) & 0x7f;
}

constexpr U32 LL_VK_API_VERSION_MINOR(U32 version)
{
    return (version >> 12) & 0x3ff;
}

constexpr U32 LL_VK_API_VERSION_PATCH(U32 version)
{
    return version & 0xfff;
}

using LLVulkanGetInstanceProcAddr = void* (*)(void*, const char*);
using LLVulkanEnumerateInstanceVersion = S32 (*)(U32*);

struct LLVulkanProbeResult
{
    bool mLoaderAvailable = false;
    bool mHasGetInstanceProcAddr = false;
    U32 mApiVersion = 0;
};

LLVulkanProbeResult probe_vulkan_loader()
{
    LLVulkanProbeResult result;

#if LL_WINDOWS
    HMODULE loader = LoadLibraryA("vulkan-1.dll");
    if (!loader)
    {
        return result;
    }

    result.mLoaderAvailable = true;
    auto get_instance_proc_addr =
        reinterpret_cast<LLVulkanGetInstanceProcAddr>(GetProcAddress(loader, "vkGetInstanceProcAddr"));
#elif LL_DARWIN || LL_LINUX
#if LL_DARWIN
    constexpr std::array<const char*, 3> loader_names =
    {
        "libvulkan.1.dylib",
        "libvulkan.dylib",
        "libMoltenVK.dylib",
    };
#else
    constexpr std::array<const char*, 2> loader_names =
    {
        "libvulkan.so.1",
        "libvulkan.so",
    };
#endif

    void* loader = nullptr;
    for (const char* loader_name : loader_names)
    {
        loader = dlopen(loader_name, RTLD_NOW | RTLD_LOCAL);
        if (loader)
        {
            break;
        }
    }

    if (!loader)
    {
        return result;
    }

    result.mLoaderAvailable = true;
    auto get_instance_proc_addr =
        reinterpret_cast<LLVulkanGetInstanceProcAddr>(dlsym(loader, "vkGetInstanceProcAddr"));
#else
    return result;
#endif

    if (get_instance_proc_addr)
    {
        result.mHasGetInstanceProcAddr = true;
        result.mApiVersion = LL_VK_MAKE_API_VERSION(0, 1, 0, 0);

        auto enumerate_instance_version =
            reinterpret_cast<LLVulkanEnumerateInstanceVersion>(
                get_instance_proc_addr(nullptr, "vkEnumerateInstanceVersion"));
        if (enumerate_instance_version)
        {
            U32 api_version = LL_VK_MAKE_API_VERSION(0, 1, 1, 0);
            if (enumerate_instance_version(&api_version) == LL_VK_SUCCESS)
            {
                result.mApiVersion = api_version;
            }
        }
    }

#if LL_WINDOWS
    FreeLibrary(loader);
#elif LL_DARWIN || LL_LINUX
    dlclose(loader);
#endif

    return result;
}

const LLVulkanProbeResult& get_vulkan_probe_result()
{
    static const LLVulkanProbeResult result = probe_vulkan_loader();
    return result;
}

std::string get_vulkan_api_version_string()
{
    U32 version = get_vulkan_probe_result().mApiVersion;
    if (!version)
    {
        return "";
    }

    return std::to_string(LL_VK_API_VERSION_MAJOR(version)) + "." +
        std::to_string(LL_VK_API_VERSION_MINOR(version)) + "." +
        std::to_string(LL_VK_API_VERSION_PATCH(version));
}

class LLVulkanRenderBackend final : public LLNullRenderBackend
{
public:
    LLRenderBackendType getType() const override { return LLRenderBackendType::Vulkan; }
    const char* getName() const override { return "Vulkan"; }
    bool isReady() const override { return false; }
    bool initContextCapabilities() override { return false; }
    bool createNativeContext(const LLRenderNativeContextDesc&, LLRenderNativeContext&) override { return false; }
    const char* getInfoString(LLRenderInfoString parameter) override
    {
        if (parameter == LLRenderInfoString::Version)
        {
            static const std::string version = get_vulkan_api_version_string();
            return version.c_str();
        }

        return "";
    }
};

}

LLRenderBackend& getVulkanRenderBackend()
{
    static LLVulkanRenderBackend backend;
    return backend;
}
