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

#include "llrenderbackendvulkan.h"

#include "lldir.h"
#include "llfile.h"
#include "llrender.h"
#include "llrenderbackendnull.h"
#include "llstring.h"
#if LL_DARWIN
#include "llrenderbackendmacosx-objc.h"
#endif

#if LL_WINDOWS
#include "llwin32headers.h"
#elif LL_DARWIN || LL_LINUX
#include <dlfcn.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <ios>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef MARE_VULKAN_FINAL_SHADER_DIR
#define MARE_VULKAN_FINAL_SHADER_DIR ""
#endif

namespace
{
constexpr U32 LL_VK_MAKE_API_VERSION(U32 variant, U32 major, U32 minor, U32 patch)
{
    return (variant << 29) | (major << 22) | (minor << 12) | patch;
}

constexpr S32 LL_VK_SUCCESS = 0;
constexpr S32 LL_VK_TIMEOUT = 2;

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

using LLVkInstance = struct LLVkInstance_T*;
using LLVkPhysicalDevice = struct LLVkPhysicalDevice_T*;
using LLVkSurfaceKHR = struct LLVkSurfaceKHR_T*;
using LLVkDevice = struct LLVkDevice_T*;
using LLVkQueue = struct LLVkQueue_T*;
using LLVkSwapchainKHR = struct LLVkSwapchainKHR_T*;
using LLVkImage = struct LLVkImage_T*;
using LLVkImageView = struct LLVkImageView_T*;
using LLVkCommandPool = struct LLVkCommandPool_T*;
using LLVkCommandBuffer = struct LLVkCommandBuffer_T*;
using LLVkSemaphore = struct LLVkSemaphore_T*;
using LLVkFence = struct LLVkFence_T*;
using LLVkRenderPass = struct LLVkRenderPass_T*;
using LLVkFramebuffer = struct LLVkFramebuffer_T*;
using LLVkShaderModule = struct LLVkShaderModule_T*;
using LLVkPipelineLayout = struct LLVkPipelineLayout_T*;
using LLVkPipeline = struct LLVkPipeline_T*;
using LLVkPipelineCache = struct LLVkPipelineCache_T*;
using LLVkBuffer = struct LLVkBuffer_T*;
using LLVkDeviceMemory = struct LLVkDeviceMemory_T*;
using LLVkSampler = struct LLVkSampler_T*;
using LLVkDescriptorSetLayout = struct LLVkDescriptorSetLayout_T*;
using LLVkDescriptorPool = struct LLVkDescriptorPool_T*;
using LLVkDescriptorSet = struct LLVkDescriptorSet_T*;

constexpr U32 LL_GL_VENDOR = 0x1f00;
constexpr U32 LL_GL_RENDERER = 0x1f01;
constexpr U32 LL_VK_MAX_PHYSICAL_DEVICE_NAME_SIZE = 256;

using LLVulkanGetInstanceProcAddr = void* (*)(LLVkInstance, const char*);
using LLVulkanGetDeviceProcAddr = void* (*)(LLVkDevice, const char*);
using LLVulkanEnumerateInstanceVersion = S32 (*)(U32*);

struct LLVkApplicationInfo
{
    S32 sType;
    const void* pNext;
    const char* pApplicationName;
    U32 applicationVersion;
    const char* pEngineName;
    U32 engineVersion;
    U32 apiVersion;
};

struct LLVkInstanceCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    const LLVkApplicationInfo* pApplicationInfo;
    U32 enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    U32 enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
};

struct LLVkExtensionProperties
{
    char extensionName[256];
    U32 specVersion;
};

struct LLVkExtent3D
{
    U32 width;
    U32 height;
    U32 depth;
};

struct LLVkExtent2D
{
    U32 width;
    U32 height;
};

struct LLVkMemoryType
{
    U32 propertyFlags;
    U32 heapIndex;
};

struct LLVkMemoryHeap
{
    U64 size;
    U32 flags;
};

struct LLVkPhysicalDeviceMemoryProperties
{
    U32 memoryTypeCount;
    LLVkMemoryType memoryTypes[32];
    U32 memoryHeapCount;
    LLVkMemoryHeap memoryHeaps[16];
};

struct LLVkPhysicalDeviceMemoryBudgetPropertiesEXT
{
    S32 sType;
    void* pNext;
    U64 heapBudget[16];
    U64 heapUsage[16];
};

struct LLVkPhysicalDeviceMemoryProperties2
{
    S32 sType;
    void* pNext;
    LLVkPhysicalDeviceMemoryProperties memoryProperties;
};

struct LLVkMemoryRequirements
{
    U64 size;
    U64 alignment;
    U32 memoryTypeBits;
};

struct LLVkMemoryAllocateInfo
{
    S32 sType;
    const void* pNext;
    U64 allocationSize;
    U32 memoryTypeIndex;
};

struct LLVkBufferCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U64 size;
    U32 usage;
    S32 sharingMode;
    U32 queueFamilyIndexCount;
    const U32* pQueueFamilyIndices;
};

struct LLVkImageCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    S32 imageType;
    S32 format;
    LLVkExtent3D extent;
    U32 mipLevels;
    U32 arrayLayers;
    S32 samples;
    S32 tiling;
    U32 usage;
    S32 sharingMode;
    U32 queueFamilyIndexCount;
    const U32* pQueueFamilyIndices;
    S32 initialLayout;
};

struct LLVkQueueFamilyProperties
{
    U32 queueFlags;
    U32 queueCount;
    U32 timestampValidBits;
    LLVkExtent3D minImageTransferGranularity;
};

struct LLVkMetalSurfaceCreateInfoEXT
{
    S32 sType;
    const void* pNext;
    U32 flags;
    const void* pLayer;
};

struct LLVkDeviceQueueCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 queueFamilyIndex;
    U32 queueCount;
    const F32* pQueuePriorities;
};

struct LLVkDeviceCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 queueCreateInfoCount;
    const LLVkDeviceQueueCreateInfo* pQueueCreateInfos;
    U32 enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    U32 enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
    const void* pEnabledFeatures;
};

struct LLVkSurfaceCapabilitiesKHR
{
    U32 minImageCount;
    U32 maxImageCount;
    LLVkExtent2D currentExtent;
    LLVkExtent2D minImageExtent;
    LLVkExtent2D maxImageExtent;
    U32 maxImageArrayLayers;
    U32 supportedTransforms;
    U32 currentTransform;
    U32 supportedCompositeAlpha;
    U32 supportedUsageFlags;
};

struct LLVkSurfaceFormatKHR
{
    S32 format;
    S32 colorSpace;
};

struct LLVkSwapchainCreateInfoKHR
{
    S32 sType;
    const void* pNext;
    U32 flags;
    LLVkSurfaceKHR surface;
    U32 minImageCount;
    S32 imageFormat;
    S32 imageColorSpace;
    LLVkExtent2D imageExtent;
    U32 imageArrayLayers;
    U32 imageUsage;
    S32 imageSharingMode;
    U32 queueFamilyIndexCount;
    const U32* pQueueFamilyIndices;
    U32 preTransform;
    U32 compositeAlpha;
    S32 presentMode;
    U32 clipped;
    LLVkSwapchainKHR oldSwapchain;
};

struct LLVkComponentMapping
{
    S32 r;
    S32 g;
    S32 b;
    S32 a;
};

struct LLVkImageSubresourceRange
{
    U32 aspectMask;
    U32 baseMipLevel;
    U32 levelCount;
    U32 baseArrayLayer;
    U32 layerCount;
};

struct LLVkImageSubresourceLayers
{
    U32 aspectMask;
    U32 mipLevel;
    U32 baseArrayLayer;
    U32 layerCount;
};

struct LLVkImageViewCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    LLVkImage image;
    S32 viewType;
    S32 format;
    LLVkComponentMapping components;
    LLVkImageSubresourceRange subresourceRange;
};

struct LLVkCommandPoolCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 queueFamilyIndex;
};

struct LLVkCommandBufferAllocateInfo
{
    S32 sType;
    const void* pNext;
    LLVkCommandPool commandPool;
    S32 level;
    U32 commandBufferCount;
};

struct LLVkSemaphoreCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
};

struct LLVkFenceCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
};

struct LLVkCommandBufferBeginInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    const void* pInheritanceInfo;
};

struct LLVkImageMemoryBarrier
{
    S32 sType;
    const void* pNext;
    U32 srcAccessMask;
    U32 dstAccessMask;
    S32 oldLayout;
    S32 newLayout;
    U32 srcQueueFamilyIndex;
    U32 dstQueueFamilyIndex;
    LLVkImage image;
    LLVkImageSubresourceRange subresourceRange;
};

struct LLVkSubmitInfo
{
    S32 sType;
    const void* pNext;
    U32 waitSemaphoreCount;
    const LLVkSemaphore* pWaitSemaphores;
    const U32* pWaitDstStageMask;
    U32 commandBufferCount;
    const LLVkCommandBuffer* pCommandBuffers;
    U32 signalSemaphoreCount;
    const LLVkSemaphore* pSignalSemaphores;
};

struct LLVkPresentInfoKHR
{
    S32 sType;
    const void* pNext;
    U32 waitSemaphoreCount;
    const LLVkSemaphore* pWaitSemaphores;
    U32 swapchainCount;
    const LLVkSwapchainKHR* pSwapchains;
    const U32* pImageIndices;
    S32* pResults;
};

struct LLVkOffset2D
{
    S32 x;
    S32 y;
};

struct LLVkOffset3D
{
    S32 x;
    S32 y;
    S32 z;
};

struct LLVkRect2D
{
    LLVkOffset2D offset;
    LLVkExtent2D extent;
};

struct LLVkAttachmentDescription
{
    U32 flags;
    S32 format;
    S32 samples;
    S32 loadOp;
    S32 storeOp;
    S32 stencilLoadOp;
    S32 stencilStoreOp;
    S32 initialLayout;
    S32 finalLayout;
};

struct LLVkAttachmentReference
{
    U32 attachment;
    S32 layout;
};

struct LLVkSubpassDescription
{
    U32 flags;
    S32 pipelineBindPoint;
    U32 inputAttachmentCount;
    const LLVkAttachmentReference* pInputAttachments;
    U32 colorAttachmentCount;
    const LLVkAttachmentReference* pColorAttachments;
    const LLVkAttachmentReference* pResolveAttachments;
    const LLVkAttachmentReference* pDepthStencilAttachment;
    U32 preserveAttachmentCount;
    const U32* pPreserveAttachments;
};

struct LLVkSubpassDependency
{
    U32 srcSubpass;
    U32 dstSubpass;
    U32 srcStageMask;
    U32 dstStageMask;
    U32 srcAccessMask;
    U32 dstAccessMask;
    U32 dependencyFlags;
};

struct LLVkRenderPassCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 attachmentCount;
    const LLVkAttachmentDescription* pAttachments;
    U32 subpassCount;
    const LLVkSubpassDescription* pSubpasses;
    U32 dependencyCount;
    const LLVkSubpassDependency* pDependencies;
};

struct LLVkFramebufferCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    LLVkRenderPass renderPass;
    U32 attachmentCount;
    const LLVkImageView* pAttachments;
    U32 width;
    U32 height;
    U32 layers;
};

struct LLVkClearDepthStencilValue
{
    F32 depth;
    U32 stencil;
};

union LLVkClearValue
{
    F32 color[4];
    LLVkClearDepthStencilValue depthStencil;
};

struct LLVkRenderPassBeginInfo
{
    S32 sType;
    const void* pNext;
    LLVkRenderPass renderPass;
    LLVkFramebuffer framebuffer;
    LLVkRect2D renderArea;
    U32 clearValueCount;
    const LLVkClearValue* pClearValues;
};

struct LLVkClearAttachment
{
    U32 aspectMask;
    U32 colorAttachment;
    LLVkClearValue clearValue;
};

struct LLVkClearRect
{
    LLVkRect2D rect;
    U32 baseArrayLayer;
    U32 layerCount;
};

struct LLVkShaderModuleCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    size_t codeSize;
    const U32* pCode;
};

struct LLVkPipelineShaderStageCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 stage;
    LLVkShaderModule module;
    const char* pName;
    const void* pSpecializationInfo;
};

struct LLVkPipelineVertexInputStateCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 vertexBindingDescriptionCount;
    const void* pVertexBindingDescriptions;
    U32 vertexAttributeDescriptionCount;
    const void* pVertexAttributeDescriptions;
};

struct LLVkVertexInputBindingDescription
{
    U32 binding;
    U32 stride;
    S32 inputRate;
};

struct LLVkVertexInputAttributeDescription
{
    U32 location;
    U32 binding;
    S32 format;
    U32 offset;
};

struct LLVkPipelineInputAssemblyStateCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    S32 topology;
    U32 primitiveRestartEnable;
};

struct LLVkViewport
{
    F32 x;
    F32 y;
    F32 width;
    F32 height;
    F32 minDepth;
    F32 maxDepth;
};

struct LLVkPipelineViewportStateCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 viewportCount;
    const LLVkViewport* pViewports;
    U32 scissorCount;
    const LLVkRect2D* pScissors;
};

struct LLVkPipelineRasterizationStateCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 depthClampEnable;
    U32 rasterizerDiscardEnable;
    S32 polygonMode;
    U32 cullMode;
    S32 frontFace;
    U32 depthBiasEnable;
    F32 depthBiasConstantFactor;
    F32 depthBiasClamp;
    F32 depthBiasSlopeFactor;
    F32 lineWidth;
};

struct LLVkPipelineMultisampleStateCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 rasterizationSamples;
    U32 sampleShadingEnable;
    F32 minSampleShading;
    const U32* pSampleMask;
    U32 alphaToCoverageEnable;
    U32 alphaToOneEnable;
};

struct LLVkStencilOpState
{
    S32 failOp;
    S32 passOp;
    S32 depthFailOp;
    S32 compareOp;
    U32 compareMask;
    U32 writeMask;
    U32 reference;
};

struct LLVkPipelineDepthStencilStateCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 depthTestEnable;
    U32 depthWriteEnable;
    S32 depthCompareOp;
    U32 depthBoundsTestEnable;
    U32 stencilTestEnable;
    LLVkStencilOpState front;
    LLVkStencilOpState back;
    F32 minDepthBounds;
    F32 maxDepthBounds;
};

struct LLVkPipelineColorBlendAttachmentState
{
    U32 blendEnable;
    S32 srcColorBlendFactor;
    S32 dstColorBlendFactor;
    S32 colorBlendOp;
    S32 srcAlphaBlendFactor;
    S32 dstAlphaBlendFactor;
    S32 alphaBlendOp;
    U32 colorWriteMask;
};

struct LLVkPipelineColorBlendStateCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 logicOpEnable;
    S32 logicOp;
    U32 attachmentCount;
    const LLVkPipelineColorBlendAttachmentState* pAttachments;
    F32 blendConstants[4];
};

struct LLVkPipelineDynamicStateCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 dynamicStateCount;
    const S32* pDynamicStates;
};

struct LLVkPushConstantRange
{
    U32 stageFlags;
    U32 offset;
    U32 size;
};

struct LLVkPipelineLayoutCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 setLayoutCount;
    const void* pSetLayouts;
    U32 pushConstantRangeCount;
    const void* pPushConstantRanges;
};

struct LLVkGraphicsPipelineCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 stageCount;
    const LLVkPipelineShaderStageCreateInfo* pStages;
    const LLVkPipelineVertexInputStateCreateInfo* pVertexInputState;
    const LLVkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState;
    const void* pTessellationState;
    const LLVkPipelineViewportStateCreateInfo* pViewportState;
    const LLVkPipelineRasterizationStateCreateInfo* pRasterizationState;
    const LLVkPipelineMultisampleStateCreateInfo* pMultisampleState;
    const void* pDepthStencilState;
    const LLVkPipelineColorBlendStateCreateInfo* pColorBlendState;
    const void* pDynamicState;
    LLVkPipelineLayout layout;
    LLVkRenderPass renderPass;
    U32 subpass;
    LLVkPipeline basePipelineHandle;
    S32 basePipelineIndex;
};

struct LLVkSamplerCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    S32 magFilter;
    S32 minFilter;
    S32 mipmapMode;
    S32 addressModeU;
    S32 addressModeV;
    S32 addressModeW;
    F32 mipLodBias;
    U32 anisotropyEnable;
    F32 maxAnisotropy;
    U32 compareEnable;
    S32 compareOp;
    F32 minLod;
    F32 maxLod;
    S32 borderColor;
    U32 unnormalizedCoordinates;
};

struct LLVkDescriptorSetLayoutBinding
{
    U32 binding;
    S32 descriptorType;
    U32 descriptorCount;
    U32 stageFlags;
    const void* pImmutableSamplers;
};

struct LLVkDescriptorSetLayoutCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 bindingCount;
    const LLVkDescriptorSetLayoutBinding* pBindings;
};

struct LLVkDescriptorPoolSize
{
    S32 type;
    U32 descriptorCount;
};

struct LLVkDescriptorPoolCreateInfo
{
    S32 sType;
    const void* pNext;
    U32 flags;
    U32 maxSets;
    U32 poolSizeCount;
    const LLVkDescriptorPoolSize* pPoolSizes;
};

struct LLVkDescriptorSetAllocateInfo
{
    S32 sType;
    const void* pNext;
    LLVkDescriptorPool descriptorPool;
    U32 descriptorSetCount;
    const LLVkDescriptorSetLayout* pSetLayouts;
};

struct LLVkDescriptorImageInfo
{
    LLVkSampler sampler;
    LLVkImageView imageView;
    S32 imageLayout;
};

struct LLVkDescriptorBufferInfo
{
    LLVkBuffer buffer;
    U64 offset;
    U64 range;
};

struct LLVkWriteDescriptorSet
{
    S32 sType;
    const void* pNext;
    LLVkDescriptorSet dstSet;
    U32 dstBinding;
    U32 dstArrayElement;
    U32 descriptorCount;
    S32 descriptorType;
    const LLVkDescriptorImageInfo* pImageInfo;
    const void* pBufferInfo;
    const void* pTexelBufferView;
};

struct alignas(8) LLVkPhysicalDeviceLimitsHeader
{
    U32 maxImageDimension1D;
    U32 maxImageDimension2D;
    U32 maxImageDimension3D;
    U32 maxImageDimensionCube;
    U32 maxImageArrayLayers;
    U32 maxTexelBufferElements;
    U32 maxUniformBufferRange;
    U32 maxStorageBufferRange;
    U32 maxPushConstantsSize;
};

struct LLVkPhysicalDevicePropertiesHeader
{
    U32 apiVersion;
    U32 driverVersion;
    U32 vendorID;
    U32 deviceID;
    S32 deviceType;
    char deviceName[LL_VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    U8 pipelineCacheUUID[16];
    LLVkPhysicalDeviceLimitsHeader limits;
};

static_assert(
    offsetof(LLVkPhysicalDevicePropertiesHeader, limits) == 296,
    "LLVkPhysicalDevicePropertiesHeader must match VkPhysicalDeviceProperties layout");
static_assert(
    offsetof(LLVkPhysicalDeviceLimitsHeader, maxPushConstantsSize) == 32,
    "LLVkPhysicalDeviceLimitsHeader must match VkPhysicalDeviceLimits layout");

struct LLVkBufferImageCopy
{
    U64 bufferOffset;
    U32 bufferRowLength;
    U32 bufferImageHeight;
    LLVkImageSubresourceLayers imageSubresource;
    S32 imageOffset[3];
    LLVkExtent3D imageExtent;
};

struct LLVkImageCopy
{
    LLVkImageSubresourceLayers srcSubresource;
    LLVkOffset3D srcOffset;
    LLVkImageSubresourceLayers dstSubresource;
    LLVkOffset3D dstOffset;
    LLVkExtent3D extent;
};

using LLVulkanCreateInstance = S32 (*)(const LLVkInstanceCreateInfo*, const void*, LLVkInstance*);
using LLVulkanDestroyInstance = void (*)(LLVkInstance, const void*);
using LLVulkanEnumeratePhysicalDevices = S32 (*)(LLVkInstance, U32*, LLVkPhysicalDevice*);
using LLVulkanEnumerateInstanceExtensionProperties = S32 (*)(const char*, U32*, LLVkExtensionProperties*);
using LLVulkanCreateMetalSurfaceEXT = S32 (*)(
    LLVkInstance,
    const LLVkMetalSurfaceCreateInfoEXT*,
    const void*,
    LLVkSurfaceKHR*);
using LLVulkanDestroySurfaceKHR = void (*)(LLVkInstance, LLVkSurfaceKHR, const void*);
using LLVulkanGetPhysicalDeviceQueueFamilyProperties =
    void (*)(LLVkPhysicalDevice, U32*, LLVkQueueFamilyProperties*);
using LLVulkanGetPhysicalDeviceProperties =
    void (*)(LLVkPhysicalDevice, void*);
using LLVulkanGetPhysicalDeviceMemoryProperties =
    void (*)(LLVkPhysicalDevice, LLVkPhysicalDeviceMemoryProperties*);
using LLVulkanGetPhysicalDeviceMemoryProperties2 =
    void (*)(LLVkPhysicalDevice, LLVkPhysicalDeviceMemoryProperties2*);
using LLVulkanGetPhysicalDeviceSurfaceSupportKHR =
    S32 (*)(LLVkPhysicalDevice, U32, LLVkSurfaceKHR, U32*);
using LLVulkanEnumerateDeviceExtensionProperties =
    S32 (*)(LLVkPhysicalDevice, const char*, U32*, LLVkExtensionProperties*);
using LLVulkanCreateDevice = S32 (*)(LLVkPhysicalDevice, const LLVkDeviceCreateInfo*, const void*, LLVkDevice*);
using LLVulkanDestroyDevice = void (*)(LLVkDevice, const void*);
using LLVulkanGetDeviceQueue = void (*)(LLVkDevice, U32, U32, LLVkQueue*);
using LLVulkanGetPhysicalDeviceSurfaceCapabilitiesKHR =
    S32 (*)(LLVkPhysicalDevice, LLVkSurfaceKHR, LLVkSurfaceCapabilitiesKHR*);
using LLVulkanGetPhysicalDeviceSurfaceFormatsKHR =
    S32 (*)(LLVkPhysicalDevice, LLVkSurfaceKHR, U32*, LLVkSurfaceFormatKHR*);
using LLVulkanGetPhysicalDeviceSurfacePresentModesKHR =
    S32 (*)(LLVkPhysicalDevice, LLVkSurfaceKHR, U32*, S32*);
using LLVulkanCreateSwapchainKHR =
    S32 (*)(LLVkDevice, const LLVkSwapchainCreateInfoKHR*, const void*, LLVkSwapchainKHR*);
using LLVulkanDestroySwapchainKHR = void (*)(LLVkDevice, LLVkSwapchainKHR, const void*);
using LLVulkanGetSwapchainImagesKHR = S32 (*)(LLVkDevice, LLVkSwapchainKHR, U32*, LLVkImage*);
using LLVulkanCreateImageView = S32 (*)(LLVkDevice, const LLVkImageViewCreateInfo*, const void*, LLVkImageView*);
using LLVulkanDestroyImageView = void (*)(LLVkDevice, LLVkImageView, const void*);
using LLVulkanCreateCommandPool = S32 (*)(LLVkDevice, const LLVkCommandPoolCreateInfo*, const void*, LLVkCommandPool*);
using LLVulkanDestroyCommandPool = void (*)(LLVkDevice, LLVkCommandPool, const void*);
using LLVulkanAllocateCommandBuffers = S32 (*)(LLVkDevice, const LLVkCommandBufferAllocateInfo*, LLVkCommandBuffer*);
using LLVulkanFreeCommandBuffers = void (*)(LLVkDevice, LLVkCommandPool, U32, const LLVkCommandBuffer*);
using LLVulkanCreateSemaphore = S32 (*)(LLVkDevice, const LLVkSemaphoreCreateInfo*, const void*, LLVkSemaphore*);
using LLVulkanDestroySemaphore = void (*)(LLVkDevice, LLVkSemaphore, const void*);
using LLVulkanCreateFence = S32 (*)(LLVkDevice, const LLVkFenceCreateInfo*, const void*, LLVkFence*);
using LLVulkanDestroyFence = void (*)(LLVkDevice, LLVkFence, const void*);
using LLVulkanWaitForFences = S32 (*)(LLVkDevice, U32, const LLVkFence*, U32, U64);
using LLVulkanResetFences = S32 (*)(LLVkDevice, U32, const LLVkFence*);
using LLVulkanAcquireNextImageKHR = S32 (*)(LLVkDevice, LLVkSwapchainKHR, U64, LLVkSemaphore, LLVkFence, U32*);
using LLVulkanBeginCommandBuffer = S32 (*)(LLVkCommandBuffer, const LLVkCommandBufferBeginInfo*);
using LLVulkanEndCommandBuffer = S32 (*)(LLVkCommandBuffer);
using LLVulkanResetCommandBuffer = S32 (*)(LLVkCommandBuffer, U32);
using LLVulkanCmdPipelineBarrier = void (*)(
    LLVkCommandBuffer,
    U32,
    U32,
    U32,
    U32,
    const void*,
    U32,
    const void*,
    U32,
    const LLVkImageMemoryBarrier*);
using LLVulkanQueueSubmit = S32 (*)(LLVkQueue, U32, const LLVkSubmitInfo*, LLVkFence);
using LLVulkanQueuePresentKHR = S32 (*)(LLVkQueue, const LLVkPresentInfoKHR*);
using LLVulkanDeviceWaitIdle = S32 (*)(LLVkDevice);
using LLVulkanCreateRenderPass = S32 (*)(LLVkDevice, const LLVkRenderPassCreateInfo*, const void*, LLVkRenderPass*);
using LLVulkanDestroyRenderPass = void (*)(LLVkDevice, LLVkRenderPass, const void*);
using LLVulkanCreateFramebuffer = S32 (*)(LLVkDevice, const LLVkFramebufferCreateInfo*, const void*, LLVkFramebuffer*);
using LLVulkanDestroyFramebuffer = void (*)(LLVkDevice, LLVkFramebuffer, const void*);
using LLVulkanCmdBeginRenderPass = void (*)(LLVkCommandBuffer, const LLVkRenderPassBeginInfo*, S32);
using LLVulkanCmdEndRenderPass = void (*)(LLVkCommandBuffer);
using LLVulkanCmdClearAttachments =
    void (*)(LLVkCommandBuffer, U32, const LLVkClearAttachment*, U32, const LLVkClearRect*);
using LLVulkanCreateShaderModule = S32 (*)(LLVkDevice, const LLVkShaderModuleCreateInfo*, const void*, LLVkShaderModule*);
using LLVulkanDestroyShaderModule = void (*)(LLVkDevice, LLVkShaderModule, const void*);
using LLVulkanCreatePipelineLayout =
    S32 (*)(LLVkDevice, const LLVkPipelineLayoutCreateInfo*, const void*, LLVkPipelineLayout*);
using LLVulkanDestroyPipelineLayout = void (*)(LLVkDevice, LLVkPipelineLayout, const void*);
using LLVulkanCreateGraphicsPipelines =
    S32 (*)(LLVkDevice, LLVkPipelineCache, U32, const LLVkGraphicsPipelineCreateInfo*, const void*, LLVkPipeline*);
using LLVulkanDestroyPipeline = void (*)(LLVkDevice, LLVkPipeline, const void*);
using LLVulkanCmdBindPipeline = void (*)(LLVkCommandBuffer, S32, LLVkPipeline);
using LLVulkanCmdDraw = void (*)(LLVkCommandBuffer, U32, U32, U32, U32);
using LLVulkanCmdBindIndexBuffer = void (*)(LLVkCommandBuffer, LLVkBuffer, U64, S32);
using LLVulkanCmdDrawIndexed = void (*)(LLVkCommandBuffer, U32, U32, U32, S32, U32);
using LLVulkanCreateBuffer = S32 (*)(LLVkDevice, const LLVkBufferCreateInfo*, const void*, LLVkBuffer*);
using LLVulkanDestroyBuffer = void (*)(LLVkDevice, LLVkBuffer, const void*);
using LLVulkanGetBufferMemoryRequirements = void (*)(LLVkDevice, LLVkBuffer, LLVkMemoryRequirements*);
using LLVulkanCreateImage = S32 (*)(LLVkDevice, const LLVkImageCreateInfo*, const void*, LLVkImage*);
using LLVulkanDestroyImage = void (*)(LLVkDevice, LLVkImage, const void*);
using LLVulkanGetImageMemoryRequirements = void (*)(LLVkDevice, LLVkImage, LLVkMemoryRequirements*);
using LLVulkanAllocateMemory = S32 (*)(LLVkDevice, const LLVkMemoryAllocateInfo*, const void*, LLVkDeviceMemory*);
using LLVulkanFreeMemory = void (*)(LLVkDevice, LLVkDeviceMemory, const void*);
using LLVulkanBindBufferMemory = S32 (*)(LLVkDevice, LLVkBuffer, LLVkDeviceMemory, U64);
using LLVulkanBindImageMemory = S32 (*)(LLVkDevice, LLVkImage, LLVkDeviceMemory, U64);
using LLVulkanMapMemory = S32 (*)(LLVkDevice, LLVkDeviceMemory, U64, U64, U32, void**);
using LLVulkanUnmapMemory = void (*)(LLVkDevice, LLVkDeviceMemory);
using LLVulkanCmdBindVertexBuffers =
    void (*)(LLVkCommandBuffer, U32, U32, const LLVkBuffer*, const U64*);
using LLVulkanCmdSetViewport = void (*)(LLVkCommandBuffer, U32, U32, const LLVkViewport*);
using LLVulkanCmdSetScissor = void (*)(LLVkCommandBuffer, U32, U32, const LLVkRect2D*);
using LLVulkanCmdCopyBufferToImage =
    void (*)(LLVkCommandBuffer, LLVkBuffer, LLVkImage, S32, U32, const LLVkBufferImageCopy*);
using LLVulkanCmdCopyImage =
    void (*)(LLVkCommandBuffer, LLVkImage, S32, LLVkImage, S32, U32, const LLVkImageCopy*);
using LLVulkanCreateSampler = S32 (*)(LLVkDevice, const LLVkSamplerCreateInfo*, const void*, LLVkSampler*);
using LLVulkanDestroySampler = void (*)(LLVkDevice, LLVkSampler, const void*);
using LLVulkanCreateDescriptorSetLayout =
    S32 (*)(LLVkDevice, const LLVkDescriptorSetLayoutCreateInfo*, const void*, LLVkDescriptorSetLayout*);
using LLVulkanDestroyDescriptorSetLayout = void (*)(LLVkDevice, LLVkDescriptorSetLayout, const void*);
using LLVulkanCreateDescriptorPool =
    S32 (*)(LLVkDevice, const LLVkDescriptorPoolCreateInfo*, const void*, LLVkDescriptorPool*);
using LLVulkanDestroyDescriptorPool = void (*)(LLVkDevice, LLVkDescriptorPool, const void*);
using LLVulkanAllocateDescriptorSets =
    S32 (*)(LLVkDevice, const LLVkDescriptorSetAllocateInfo*, LLVkDescriptorSet*);
using LLVulkanFreeDescriptorSets =
    S32 (*)(LLVkDevice, LLVkDescriptorPool, U32, const LLVkDescriptorSet*);
using LLVulkanUpdateDescriptorSets =
    void (*)(LLVkDevice, U32, const LLVkWriteDescriptorSet*, U32, const void*);
using LLVulkanCmdBindDescriptorSets =
    void (*)(LLVkCommandBuffer, S32, LLVkPipelineLayout, U32, U32, const LLVkDescriptorSet*, U32, const U32*);
using LLVulkanCmdPushConstants =
    void (*)(LLVkCommandBuffer, LLVkPipelineLayout, U32, U32, U32, const void*);

constexpr S32 LL_VK_STRUCTURE_TYPE_APPLICATION_INFO = 0;
constexpr S32 LL_VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1;
constexpr S32 LL_VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2;
constexpr S32 LL_VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3;
constexpr S32 LL_VK_STRUCTURE_TYPE_SUBMIT_INFO = 4;
constexpr S32 LL_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 5;
constexpr S32 LL_VK_STRUCTURE_TYPE_FENCE_CREATE_INFO = 8;
constexpr S32 LL_VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO = 9;
constexpr S32 LL_VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO = 12;
constexpr S32 LL_VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO = 14;
constexpr S32 LL_VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO = 16;
constexpr S32 LL_VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO = 18;
constexpr S32 LL_VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO = 19;
constexpr S32 LL_VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO = 20;
constexpr S32 LL_VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO = 22;
constexpr S32 LL_VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO = 23;
constexpr S32 LL_VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO = 24;
constexpr S32 LL_VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO = 25;
constexpr S32 LL_VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO = 26;
constexpr S32 LL_VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO = 27;
constexpr S32 LL_VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO = 28;
constexpr S32 LL_VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO = 30;
constexpr S32 LL_VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO = 31;
constexpr S32 LL_VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO = 32;
constexpr S32 LL_VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO = 33;
constexpr S32 LL_VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO = 34;
constexpr S32 LL_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET = 35;
constexpr S32 LL_VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO = 15;
constexpr S32 LL_VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO = 37;
constexpr S32 LL_VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO = 38;
constexpr S32 LL_VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO = 39;
constexpr S32 LL_VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO = 40;
constexpr S32 LL_VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO = 42;
constexpr S32 LL_VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO = 43;
constexpr S32 LL_VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER = 44;
constexpr S32 LL_VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 = 1000059006;
constexpr S32 LL_VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR = 1000001000;
constexpr S32 LL_VK_STRUCTURE_TYPE_PRESENT_INFO_KHR = 1000001001;
constexpr S32 LL_VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT = 1000217000;
constexpr S32 LL_VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT = 1000237000;
constexpr U32 LL_VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR = 0x00000001;
constexpr U32 LL_VK_QUEUE_GRAPHICS_BIT = 0x00000001;
constexpr U32 LL_VK_QUEUE_FAMILY_IGNORED = 0xffffffff;
constexpr U64 LL_VK_TIMEOUT_FOREVER = 0xffffffffffffffffULL;
constexpr S32 LL_VK_SUBOPTIMAL_KHR = 1000001003;
constexpr S32 LL_VK_ERROR_OUT_OF_DATE_KHR = -1000001004;
constexpr U32 LL_VK_EXTENT_UNDEFINED = 0xffffffff;
constexpr S32 LL_VK_FORMAT_UNDEFINED = 0;
constexpr S32 LL_VK_FORMAT_R8_UNORM = 9;
constexpr S32 LL_VK_FORMAT_R8G8_UNORM = 16;
constexpr S32 LL_VK_FORMAT_R8G8B8A8_UNORM = 37;
constexpr S32 LL_VK_FORMAT_B8G8R8A8_UNORM = 44;
constexpr S32 LL_VK_FORMAT_R8G8B8A8_SRGB = 43;
constexpr S32 LL_VK_FORMAT_B8G8R8A8_SRGB = 50;
constexpr S32 LL_VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64;
constexpr S32 LL_VK_FORMAT_R16_UNORM = 70;
constexpr S32 LL_VK_FORMAT_R16_SFLOAT = 76;
constexpr S32 LL_VK_FORMAT_R16G16_SFLOAT = 83;
constexpr S32 LL_VK_FORMAT_R16G16B16A16_UNORM = 91;
constexpr S32 LL_VK_FORMAT_R16G16B16A16_SFLOAT = 97;
constexpr S32 LL_VK_FORMAT_R32_UINT = 98;
constexpr S32 LL_VK_FORMAT_R32_SFLOAT = 100;
constexpr S32 LL_VK_FORMAT_R32G32_SFLOAT = 103;
constexpr S32 LL_VK_FORMAT_R32G32B32_SFLOAT = 106;
constexpr S32 LL_VK_FORMAT_R32G32B32A32_SFLOAT = 109;
constexpr S32 LL_VK_FORMAT_B10G11R11_UFLOAT_PACK32 = 122;
constexpr S32 LL_VK_FORMAT_D32_SFLOAT = 126;
constexpr S32 LL_VK_INDEX_TYPE_UINT16 = 0;
constexpr S32 LL_VK_INDEX_TYPE_UINT32 = 1;
constexpr S32 LL_VK_COLOR_SPACE_SRGB_NONLINEAR_KHR = 0;
constexpr S32 LL_VK_IMAGE_TYPE_2D = 1;
constexpr S32 LL_VK_IMAGE_TILING_OPTIMAL = 0;
constexpr S32 LL_VK_SHARING_MODE_EXCLUSIVE = 0;
constexpr S32 LL_VK_SHARING_MODE_CONCURRENT = 1;
constexpr S32 LL_VK_PRESENT_MODE_IMMEDIATE_KHR = 0;
constexpr S32 LL_VK_PRESENT_MODE_MAILBOX_KHR = 1;
constexpr S32 LL_VK_PRESENT_MODE_FIFO_KHR = 2;
constexpr U32 LL_VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR = 0x00000001;
constexpr U32 LL_VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR = 0x00000002;
constexpr U32 LL_VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR = 0x00000004;
constexpr U32 LL_VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR = 0x00000008;
constexpr U32 LL_VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 0x00000010;
constexpr U32 LL_VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020;
constexpr U32 LL_VK_IMAGE_USAGE_TRANSFER_SRC_BIT = 0x00000001;
constexpr U32 LL_VK_IMAGE_USAGE_TRANSFER_DST_BIT = 0x00000002;
constexpr U32 LL_VK_IMAGE_USAGE_SAMPLED_BIT = 0x00000004;
constexpr U32 LL_VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 0x00000001;
constexpr U32 LL_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020;
constexpr U32 LL_VK_BUFFER_USAGE_INDEX_BUFFER_BIT = 0x00000040;
constexpr U32 LL_VK_BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x00000080;
constexpr U32 MARE_VULKAN_DEFAULT_UI_ATTRIBUTE_VERTICES = 262144;
constexpr U32 MARE_VULKAN_MAX_TEXTURE_BINDINGS = 8;
constexpr U32 MARE_VULKAN_SKINNING_DESCRIPTOR_BINDING = 8;
constexpr U32 MARE_VULKAN_TEXTURE_DESCRIPTOR_SET_CAPACITY = 4096;
constexpr U32 MARE_VULKAN_TEXTURE_DESCRIPTOR_CAPACITY =
    MARE_VULKAN_TEXTURE_DESCRIPTOR_SET_CAPACITY *
    MARE_VULKAN_MAX_TEXTURE_BINDINGS;
constexpr U64 MARE_VULKAN_BYTES_PER_MEGABYTE = 1024 * 1024;
constexpr U64 MARE_VULKAN_DEFAULT_TEXTURE_MEMORY_BUDGET_MB = 1024;
constexpr U64 MARE_VULKAN_DEFAULT_BUFFER_MEMORY_BUDGET_MB = 1024;
constexpr U64 MARE_VULKAN_DEFAULT_BUFFER_MEMORY_RESERVE_MB = 128;
constexpr U64 MARE_VULKAN_DEFAULT_HEAP_MEMORY_RESERVE_MB = 512;
constexpr U64 MARE_VULKAN_DEFAULT_SKINNED_POSITION_FRAME_BUDGET_MB = 64;
constexpr U64 MARE_VULKAN_SKINNING_PALETTE_BUFFER_GROWTH_BYTES =
    4 * MARE_VULKAN_BYTES_PER_MEGABYTE;
constexpr U64 MARE_VULKAN_DEFAULT_TEXTURE_UPLOAD_FRAME_BUDGET_MB = 32;
constexpr U32 MARE_VULKAN_DEFAULT_TEXTURE_UPLOAD_FRAME_COUNT_BUDGET = 0;
constexpr S32 MARE_VULKAN_DEFAULT_MAX_TEXTURE_DIMENSION = 2048;
constexpr U32 MARE_VULKAN_MAX_SKINNING_MATRICES = 110;
constexpr U32 LL_VK_MEMORY_HEAP_DEVICE_LOCAL_BIT = 0x00000001;
constexpr U32 LL_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 0x00000001;
constexpr U32 LL_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 0x00000002;
constexpr U32 LL_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 0x00000004;
constexpr U32 LL_VK_IMAGE_ASPECT_COLOR_BIT = 0x00000001;
constexpr U32 LL_VK_IMAGE_ASPECT_DEPTH_BIT = 0x00000002;
constexpr S32 LL_VK_IMAGE_VIEW_TYPE_2D = 1;
constexpr S32 LL_VK_COMPONENT_SWIZZLE_IDENTITY = 0;
constexpr S32 LL_VK_FILTER_NEAREST = 0;
constexpr S32 LL_VK_FILTER_LINEAR = 1;
constexpr S32 LL_VK_SAMPLER_MIPMAP_MODE_NEAREST = 0;
constexpr S32 LL_VK_SAMPLER_MIPMAP_MODE_LINEAR = 1;
constexpr S32 LL_VK_SAMPLER_ADDRESS_MODE_REPEAT = 0;
constexpr S32 LL_VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1;
constexpr S32 LL_VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2;
constexpr S32 LL_VK_BORDER_COLOR_INT_OPAQUE_BLACK = 3;
constexpr S32 LL_VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1;
constexpr S32 LL_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7;
constexpr U32 LL_VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT = 0x00000001;
constexpr S32 LL_VK_DYNAMIC_STATE_VIEWPORT = 0;
constexpr S32 LL_VK_DYNAMIC_STATE_SCISSOR = 1;
constexpr S32 LL_VK_COMMAND_BUFFER_LEVEL_PRIMARY = 0;
constexpr U32 LL_VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 0x00000001;
constexpr U32 LL_VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT = 0x00000002;
constexpr U32 LL_VK_FENCE_CREATE_SIGNALED_BIT = 0x00000001;
constexpr S32 LL_VK_IMAGE_LAYOUT_UNDEFINED = 0;
constexpr S32 LL_VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2;
constexpr S32 LL_VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3;
constexpr S32 LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5;
constexpr S32 LL_VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL = 6;
constexpr S32 LL_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7;
constexpr S32 LL_VK_IMAGE_LAYOUT_PRESENT_SRC_KHR = 1000001002;
constexpr S32 LL_VK_ATTACHMENT_LOAD_OP_CLEAR = 1;
constexpr S32 LL_VK_ATTACHMENT_STORE_OP_STORE = 0;
constexpr S32 LL_VK_ATTACHMENT_LOAD_OP_DONT_CARE = 2;
constexpr S32 LL_VK_ATTACHMENT_STORE_OP_DONT_CARE = 1;
constexpr S32 LL_VK_PIPELINE_BIND_POINT_GRAPHICS = 0;
constexpr S32 LL_VK_SAMPLE_COUNT_1_BIT = 1;
constexpr U32 LL_VK_SUBPASS_EXTERNAL = 0xffffffff;
constexpr U32 LL_VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x00000001;
constexpr U32 LL_VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000;
constexpr U32 LL_VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 0x00000400;
constexpr U32 LL_VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT = 0x00000080;
constexpr U32 LL_VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT = 0x00000100;
constexpr U32 LL_VK_PIPELINE_STAGE_TRANSFER_BIT = 0x00001000;
constexpr U32 LL_VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = 0x00000100;
constexpr U32 LL_VK_ACCESS_SHADER_READ_BIT = 0x00000020;
constexpr U32 LL_VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 0x00000400;
constexpr U32 LL_VK_ACCESS_TRANSFER_READ_BIT = 0x00000800;
constexpr U32 LL_VK_ACCESS_TRANSFER_WRITE_BIT = 0x00001000;
constexpr S32 LL_VK_SUBPASS_CONTENTS_INLINE = 0;
constexpr U32 LL_VK_SHADER_STAGE_VERTEX_BIT = 0x00000001;
constexpr U32 LL_VK_SHADER_STAGE_FRAGMENT_BIT = 0x00000010;
constexpr S32 LL_VK_VERTEX_INPUT_RATE_VERTEX = 0;
constexpr S32 LL_VK_PRIMITIVE_TOPOLOGY_POINT_LIST = 0;
constexpr S32 LL_VK_PRIMITIVE_TOPOLOGY_LINE_LIST = 1;
constexpr S32 LL_VK_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2;
constexpr S32 LL_VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3;
constexpr S32 LL_VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4;
constexpr S32 LL_VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5;
constexpr S32 LL_VK_POLYGON_MODE_FILL = 0;
constexpr U32 LL_VK_CULL_MODE_NONE = 0;
constexpr U32 LL_VK_CULL_MODE_BACK_BIT = 0x00000002;
constexpr S32 LL_VK_FRONT_FACE_COUNTER_CLOCKWISE = 0;
constexpr S32 LL_VK_COMPARE_OP_LESS_OR_EQUAL = 3;
constexpr S32 LL_VK_COMPARE_OP_ALWAYS = 7;
constexpr S32 LL_VK_BLEND_FACTOR_ZERO = 0;
constexpr S32 LL_VK_BLEND_FACTOR_ONE = 1;
constexpr S32 LL_VK_BLEND_FACTOR_SRC_ALPHA = 6;
constexpr S32 LL_VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7;
constexpr S32 LL_VK_BLEND_OP_ADD = 0;
constexpr U32 LL_VK_COLOR_COMPONENT_R_BIT = 0x00000001;
constexpr U32 LL_VK_COLOR_COMPONENT_G_BIT = 0x00000002;
constexpr U32 LL_VK_COLOR_COMPONENT_B_BIT = 0x00000004;
constexpr U32 LL_VK_COLOR_COMPONENT_A_BIT = 0x00000008;
constexpr U32 MARE_VULKAN_PRIMITIVE_PIPELINE_COUNT = 7;
constexpr U32 MARE_VULKAN_WORLD_BLEND_PIPELINE_COUNT = 3;
constexpr U32 MARE_VULKAN_WORLD_DEPTH_PIPELINE_COUNT = 3;
constexpr U32 MARE_VULKAN_WORLD_CULL_PIPELINE_COUNT = 2;
constexpr U32 MARE_VULKAN_WORLD_PIPELINE_COUNT =
    MARE_VULKAN_PRIMITIVE_PIPELINE_COUNT *
    MARE_VULKAN_WORLD_BLEND_PIPELINE_COUNT *
    MARE_VULKAN_WORLD_DEPTH_PIPELINE_COUNT *
    MARE_VULKAN_WORLD_CULL_PIPELINE_COUNT;
constexpr const char* LL_VK_KHR_SURFACE_EXTENSION_NAME = "VK_KHR_surface";
constexpr const char* LL_VK_EXT_METAL_SURFACE_EXTENSION_NAME = "VK_EXT_metal_surface";
constexpr const char* LL_VK_KHR_SWAPCHAIN_EXTENSION_NAME = "VK_KHR_swapchain";
constexpr const char* LL_VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME = "VK_KHR_portability_enumeration";
constexpr const char* LL_VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME = "VK_KHR_portability_subset";
constexpr const char* LL_VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME =
    "VK_KHR_get_physical_device_properties2";
constexpr const char* LL_VK_EXT_MEMORY_BUDGET_EXTENSION_NAME = "VK_EXT_memory_budget";

std::string format_vulkan_megabytes(U64 bytes)
{
    std::ostringstream stream;
    stream << std::fixed
        << std::setprecision(2)
        << (static_cast<double>(bytes) /
            static_cast<double>(MARE_VULKAN_BYTES_PER_MEGABYTE))
        << "MB";
    return stream.str();
}

struct LLVulkanProbeResult
{
    bool mLoaderAvailable = false;
    bool mHasGetInstanceProcAddr = false;
    U32 mApiVersion = 0;
    std::string mLoaderPath;
};

class LLVulkanLoader final
{
public:
    LLVulkanLoader()
    {
        open();
    }

    ~LLVulkanLoader()
    {
#if LL_WINDOWS
        if (mLoader)
        {
            FreeLibrary(mLoader);
        }
#elif LL_DARWIN || LL_LINUX
        if (mLoader)
        {
            dlclose(mLoader);
        }
#endif
    }

    const LLVulkanProbeResult& getProbeResult() const
    {
        return mProbeResult;
    }

    bool isAvailable() const
    {
        return mProbeResult.mLoaderAvailable && mProbeResult.mHasGetInstanceProcAddr;
    }

    void* getInstanceProcAddress(LLVkInstance instance, const char* name) const
    {
        return mGetInstanceProcAddr ? mGetInstanceProcAddr(instance, name) : nullptr;
    }

    void* getDeviceProcAddress(LLVkDevice device, const char* name) const
    {
        return mGetDeviceProcAddr ? mGetDeviceProcAddr(device, name) : nullptr;
    }

private:
    void open()
    {
        std::string loader_override = LLStringUtil::getenv("MARE_VULKAN_LOADER");
        LLStringUtil::trim(loader_override);
        if (!loader_override.empty())
        {
            if (!tryOpen(loader_override))
            {
                LL_WARNS("RenderBackend")
                    << "MARE_VULKAN_LOADER is set, but no usable Vulkan loader was found at "
                    << loader_override
                    << LL_ENDL;
            }
            return;
        }

        std::string vulkan_sdk = LLStringUtil::getenv("VULKAN_SDK");
        LLStringUtil::trim(vulkan_sdk);
        if (!vulkan_sdk.empty())
        {
#if LL_DARWIN
            const std::array<std::string, 3> sdk_loader_names =
            {
                vulkan_sdk + "/lib/libMoltenVK.dylib",
                vulkan_sdk + "/lib/libvulkan.1.dylib",
                vulkan_sdk + "/lib/libvulkan.dylib",
            };
#elif LL_WINDOWS
            const std::array<std::string, 1> sdk_loader_names =
            {
                vulkan_sdk + "\\Bin\\vulkan-1.dll",
            };
#elif LL_LINUX
            const std::array<std::string, 2> sdk_loader_names =
            {
                vulkan_sdk + "/lib/libvulkan.so.1",
                vulkan_sdk + "/lib/libvulkan.so",
            };
#endif

            for (const std::string& loader_name : sdk_loader_names)
            {
                if (tryOpen(loader_name))
                {
                    return;
                }
            }

            LL_WARNS("RenderBackend")
                << "VULKAN_SDK is set, but no usable Vulkan loader was found under "
                << vulkan_sdk
                << LL_ENDL;
        }

#if LL_WINDOWS
        constexpr std::array<const char*, 1> loader_names =
        {
            "vulkan-1.dll",
        };
#elif LL_DARWIN || LL_LINUX
#if LL_DARWIN
        constexpr std::array<const char*, 6> loader_names =
        {
            "@executable_path/../Frameworks/libMoltenVK.dylib",
            "@executable_path/../Frameworks/libvulkan.1.dylib",
            "@executable_path/../Frameworks/libvulkan.dylib",
            "libMoltenVK.dylib",
            "libvulkan.1.dylib",
            "libvulkan.dylib",
        };
#else
        constexpr std::array<const char*, 2> loader_names =
        {
            "libvulkan.so.1",
            "libvulkan.so",
        };
#endif
#else
        return;
#endif

        for (const char* loader_name : loader_names)
        {
            if (tryOpen(loader_name))
            {
                return;
            }
        }
    }

    bool tryOpen(const std::string& loader_name)
    {
        if (loader_name.empty())
        {
            return false;
        }

#if LL_WINDOWS
        mLoader = LoadLibraryA(loader_name.c_str());
        if (!mLoader)
        {
            return false;
        }

        auto get_instance_proc_addr =
            reinterpret_cast<LLVulkanGetInstanceProcAddr>(GetProcAddress(mLoader, "vkGetInstanceProcAddr"));
#elif LL_DARWIN || LL_LINUX
        mLoader = dlopen(loader_name.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!mLoader)
        {
            return false;
        }

        auto get_instance_proc_addr =
            reinterpret_cast<LLVulkanGetInstanceProcAddr>(dlsym(mLoader, "vkGetInstanceProcAddr"));
#else
        return false;
#endif

        if (!get_instance_proc_addr)
        {
            close();
            return false;
        }

        mProbeResult.mLoaderAvailable = true;
        mProbeResult.mHasGetInstanceProcAddr = true;
        mProbeResult.mLoaderPath = loader_name;
        mProbeResult.mApiVersion = LL_VK_MAKE_API_VERSION(0, 1, 0, 0);
        mGetInstanceProcAddr = get_instance_proc_addr;
        mGetDeviceProcAddr =
            reinterpret_cast<LLVulkanGetDeviceProcAddr>(
                get_instance_proc_addr(nullptr, "vkGetDeviceProcAddr"));

        auto enumerate_instance_version =
            reinterpret_cast<LLVulkanEnumerateInstanceVersion>(
                get_instance_proc_addr(nullptr, "vkEnumerateInstanceVersion"));
        if (enumerate_instance_version)
        {
            U32 api_version = LL_VK_MAKE_API_VERSION(0, 1, 1, 0);
            if (enumerate_instance_version(&api_version) == LL_VK_SUCCESS)
            {
                mProbeResult.mApiVersion = api_version;
            }
        }

        LL_INFOS("RenderBackend")
            << "Loaded Vulkan loader: " << mProbeResult.mLoaderPath
            << LL_ENDL;
        return true;
    }

    void close()
    {
#if LL_WINDOWS
        if (mLoader)
        {
            FreeLibrary(mLoader);
            mLoader = nullptr;
        }
#elif LL_DARWIN || LL_LINUX
        if (mLoader)
        {
            dlclose(mLoader);
            mLoader = nullptr;
        }
#endif
        mGetInstanceProcAddr = nullptr;
        mGetDeviceProcAddr = nullptr;
    }

private:
#if LL_WINDOWS
    HMODULE mLoader = nullptr;
#elif LL_DARWIN || LL_LINUX
    void* mLoader = nullptr;
#endif
    LLVulkanGetInstanceProcAddr mGetInstanceProcAddr = nullptr;
    LLVulkanGetDeviceProcAddr mGetDeviceProcAddr = nullptr;
    LLVulkanProbeResult mProbeResult;
};

struct LLVulkanFrameSync
{
    LLVkSemaphore mImageAvailableSemaphore = nullptr;
    LLVkSemaphore mRenderFinishedSemaphore = nullptr;
    LLVkFence mInFlightFence = nullptr;
};

struct LLVulkanVertexAttributeState
{
    bool mEnabled = false;
    U32 mStride = 0;
    U64 mOffset = 0;
};

enum class LLVulkanWorldBlendPipeline : U8
{
    Opaque,
    Alpha,
    Add,
};

enum class LLVulkanWorldDepthPipeline : U8
{
    Disabled,
    ReadOnly,
    ReadWrite,
};

enum class LLVulkanWorldCullPipeline : U8
{
    None,
    Back,
};

struct LLVulkanPendingDraw
{
    using texture_bindings_t = std::array<U32, MARE_VULKAN_MAX_TEXTURE_BINDINGS>;

    bool mClearOnly = false;
    LLRenderClearMask mClearMask = LL_RENDER_CLEAR_NONE;
    LLRenderClearColor mClearColor = {0.f, 0.f, 0.f, 1.f};
    F32 mClearDepth = 1.f;
    U32 mClearStencil = 0;
    U32 mFramebuffer = 0;
    U32 mBuffer = 0;
    U32 mIndexBuffer = 0;
    U32 mTexture = 0;
    texture_bindings_t mTextures = {};
    LLRenderPrimitiveType mMode = LLRenderPrimitiveType::Triangles;
    S32 mFirst = 0;
    S32 mCount = 0;
    S32 mIndexType = LL_VK_INDEX_TYPE_UINT16;
    bool mIndexed = false;
    LLRenderViewport mViewport;
    LLRenderScissor mScissor;
    std::array<LLVulkanVertexAttributeState, 16> mAttributes;
    glm::mat4 mModelviewProjection = glm::mat4(1.f);
    bool mUseWorldVertexShader = false;
    LLVulkanWorldBlendPipeline mWorldBlendPipeline = LLVulkanWorldBlendPipeline::Opaque;
    LLVulkanWorldDepthPipeline mWorldDepthPipeline = LLVulkanWorldDepthPipeline::ReadWrite;
    LLVulkanWorldCullPipeline mWorldCullPipeline = LLVulkanWorldCullPipeline::Back;
    LLRenderWorldShaderClass mWorldShaderClass = LLRenderWorldShaderClass::Textured;
    LLRenderWorldTerrainParameters mTerrainParameters;
    LLRenderWorldMaterialParameters mMaterialParameters;
    LLRenderWorldTextureTransform mTextureTransform;
    std::vector<F32> mSkinningMatrixPalette;
    U32 mSkinningMatrixOffset = 0;
    U32 mSkinningMatrixCount = 0;
    bool mDepthTestEnabled = false;
    bool mDepthWriteEnabled = true;
    LLRenderDepthFunction mDepthFunction = LLRenderDepthFunction::LessEqual;
    bool mCullFaceEnabled = false;
    LLRenderCullFace mCullFace = LLRenderCullFace::Back;
    F32 mAlphaMaskCutoff = -1.f;
};

struct LLVulkanWorldPushConstants
{
    glm::mat4 mModelviewProjection = glm::mat4(1.f);
    glm::vec4 mParams = glm::vec4(-1.f, 0.f, 0.f, 0.f);
    glm::vec4 mTerrainParameters = glm::vec4(1.f, 0.f, 0.f, 0.f);
    glm::vec4 mTextureTransformS = glm::vec4(1.f, 0.f, 0.f, 0.f);
    glm::vec4 mTextureTransformT = glm::vec4(0.f, 1.f, 0.f, 0.f);
    glm::vec4 mMaterialExtra = glm::vec4(0.f, 0.f, 0.f, 0.f);
    glm::vec4 mBaseTextureTransform0 = glm::vec4(1.f, 1.f, 0.f, 0.f);
    glm::vec4 mBaseTextureTransform1 = glm::vec4(0.f, 0.f, 0.f, 0.f);
    glm::vec4 mMaterialPBR = glm::vec4(1.f, 1.f, 0.f, 0.f);
    glm::vec4 mMaterialLegacy = glm::vec4(1.f, 1.f, 1.f, 0.f);
    glm::vec4 mMaterialModes = glm::vec4(0.f, 0.f, 0.f, 0.f);
};

struct LLVulkanDrawBounds
{
    bool mValid = false;
    F32 mMinX = 0.f;
    F32 mMaxX = 0.f;
    F32 mMinY = 0.f;
    F32 mMaxY = 0.f;
};

struct LLVulkanDrawColor
{
    bool mValid = false;
    U8 mR = 0;
    U8 mG = 0;
    U8 mB = 0;
    U8 mA = 0;
};

struct LLVulkanBufferResource
{
    LLVkBuffer mBuffer = nullptr;
    LLVkDeviceMemory mMemory = nullptr;
    U64 mSize = 0;
    U64 mMemorySize = 0;
    bool mMemoryAccounted = false;
    void* mMappedData = nullptr;
};

struct LLVulkanPendingBufferAllocation
{
    U64 mSize = 0;
    U32 mUsageFlags = 0;
    LLRenderBufferUsage mUsage = LLRenderBufferUsage::StaticDraw;
};

struct LLVulkanTextureResource
{
    LLVkImage mImage = nullptr;
    LLVkDeviceMemory mMemory = nullptr;
    LLVkImageView mImageView = nullptr;
    LLVkSampler mSampler = nullptr;
    LLVkDescriptorSet mDescriptorSet = nullptr;
    U64 mMemorySize = 0;
    S32 mWidth = 0;
    S32 mHeight = 0;
    S32 mFormat = LL_VK_FORMAT_R8G8B8A8_UNORM;
    U32 mAspectMask = LL_VK_IMAGE_ASPECT_COLOR_BIT;
    bool mMemoryAccounted = false;
    U64 mLastBoundFrame = 0;
};

struct LLVulkanFramebufferResource
{
    std::array<U32, 4> mColorTextures = {};
    U32 mDepthTexture = 0;
    U32 mColorAttachmentCount = 0;
    LLVkFramebuffer mFramebuffer = nullptr;
    U64 mRenderPassKey = 0;
    S32 mWidth = 0;
    S32 mHeight = 0;
    bool mDirty = true;
};

struct LLVulkanDepthAttachment
{
    LLVkImage mImage = nullptr;
    LLVkDeviceMemory mMemory = nullptr;
    LLVkImageView mImageView = nullptr;
    S32 mFormat = LL_VK_FORMAT_D32_SFLOAT;
};

struct LLVulkanTextureSamplerState
{
    LLRenderTextureFilter mMinFilter = LLRenderTextureFilter::Linear;
    LLRenderTextureFilter mMagFilter = LLRenderTextureFilter::Linear;
    LLRenderTextureAddressMode mAddressModeS = LLRenderTextureAddressMode::Repeat;
    LLRenderTextureAddressMode mAddressModeT = LLRenderTextureAddressMode::Repeat;
    LLRenderTextureAddressMode mAddressModeW = LLRenderTextureAddressMode::Repeat;
};

struct LLVulkanFinalShaderModule
{
    LLVkShaderModule mModule = nullptr;
    std::string mPath;
};

struct LLVulkanNativeContext
{
    LLVkInstance mInstance = nullptr;
    LLVkSurfaceKHR mSurface = nullptr;
    LLVkPhysicalDevice mPhysicalDevice = nullptr;
    std::string mPhysicalDeviceVendor;
    std::string mPhysicalDeviceName;
    U32 mMaxPushConstantsSize = 0;
    LLVkDevice mDevice = nullptr;
    LLVkQueue mGraphicsQueue = nullptr;
    LLVkQueue mPresentQueue = nullptr;
    LLVkSwapchainKHR mSwapchain = nullptr;
    LLVkCommandPool mCommandPool = nullptr;
    LLVkRenderPass mRenderPass = nullptr;
    LLVkRenderPass mOffscreenRenderPass = nullptr;
    std::unordered_map<U64, LLVkRenderPass> mOffscreenRenderPasses;
    LLVkShaderModule mBootstrapVertexShader = nullptr;
    LLVkShaderModule mBootstrapFragmentShader = nullptr;
    LLVkShaderModule mUIVertexShader = nullptr;
    LLVkShaderModule mUIFragmentShader = nullptr;
    LLVkShaderModule mWorldVertexShader = nullptr;
    LLVkShaderModule mWorldFragmentShader = nullptr;
    LLVkShaderModule mTerrainVertexShader = nullptr;
    LLVkShaderModule mTerrainFragmentShader = nullptr;
    std::unordered_map<std::string, LLVulkanFinalShaderModule> mFinalShaderModules;
    LLVkPipelineLayout mBootstrapPipelineLayout = nullptr;
    LLVkPipelineLayout mUIPipelineLayout = nullptr;
    LLVkPipelineLayout mWorldPipelineLayout = nullptr;
    LLVkDescriptorSetLayout mUIDescriptorSetLayout = nullptr;
    LLVkDescriptorPool mUIDescriptorPool = nullptr;
    LLVkPipeline mBootstrapPipeline = nullptr;
    std::array<LLVkPipeline, MARE_VULKAN_PRIMITIVE_PIPELINE_COUNT> mUIPipelines = {};
    std::array<LLVkPipeline, MARE_VULKAN_WORLD_PIPELINE_COUNT> mWorldPipelines = {};
    std::array<LLVkPipeline, MARE_VULKAN_WORLD_PIPELINE_COUNT> mTerrainPipelines = {};
    LLVulkanBufferResource mDefaultTexCoordBuffer;
    LLVulkanBufferResource mDefaultColorBuffer;
    LLVulkanBufferResource mDefaultNormalBuffer;
    LLVulkanBufferResource mDefaultTangentBuffer;
    LLVulkanBufferResource mSkinningMatrixPaletteBuffer;
    std::vector<LLVulkanBufferResource> mTransientFrameBuffers;
    U64 mTransientFrameBufferBytes = 0;
    U32 mGraphicsQueueFamilyIndex = LL_VK_QUEUE_FAMILY_IGNORED;
    U32 mPresentQueueFamilyIndex = LL_VK_QUEUE_FAMILY_IGNORED;
    LLVkExtent2D mSwapchainExtent = {0, 0};
    LLVkExtent2D mNativeViewExtent = {0, 0};
    LLVkPhysicalDeviceMemoryProperties mMemoryProperties = {};
    std::array<U64, 16> mMemoryHeapBudgetBytes = {};
    std::array<U64, 16> mMemoryHeapUsageBytes = {};
    U32 mReportedVideoMemoryMB = 0;
    F32 mDrawableScaleX = 1.f;
    F32 mDrawableScaleY = 1.f;
    S32 mSwapchainImageFormat = LL_VK_FORMAT_UNDEFINED;
    LLVulkanDestroyInstance mDestroyInstance = nullptr;
    LLVulkanEnumeratePhysicalDevices mEnumeratePhysicalDevices = nullptr;
    LLVulkanDestroySurfaceKHR mDestroySurface = nullptr;
    LLVulkanDestroyDevice mDestroyDevice = nullptr;
    LLVulkanDestroySwapchainKHR mDestroySwapchain = nullptr;
    LLVulkanCreateImageView mCreateImageView = nullptr;
    LLVulkanDestroyImageView mDestroyImageView = nullptr;
    LLVulkanDestroyCommandPool mDestroyCommandPool = nullptr;
    LLVulkanAllocateCommandBuffers mAllocateCommandBuffers = nullptr;
    LLVulkanFreeCommandBuffers mFreeCommandBuffers = nullptr;
    LLVulkanCreateRenderPass mCreateRenderPass = nullptr;
    LLVulkanDestroyRenderPass mDestroyRenderPass = nullptr;
    LLVulkanCreateFramebuffer mCreateFramebuffer = nullptr;
    LLVulkanDestroyFramebuffer mDestroyFramebuffer = nullptr;
    LLVulkanDestroySemaphore mDestroySemaphore = nullptr;
    LLVulkanCreateFence mCreateFence = nullptr;
    LLVulkanDestroyFence mDestroyFence = nullptr;
    LLVulkanWaitForFences mWaitForFences = nullptr;
    LLVulkanResetFences mResetFences = nullptr;
    LLVulkanAcquireNextImageKHR mAcquireNextImage = nullptr;
    LLVulkanBeginCommandBuffer mBeginCommandBuffer = nullptr;
    LLVulkanEndCommandBuffer mEndCommandBuffer = nullptr;
    LLVulkanResetCommandBuffer mResetCommandBuffer = nullptr;
    LLVulkanCmdPipelineBarrier mCmdPipelineBarrier = nullptr;
    LLVulkanQueueSubmit mQueueSubmit = nullptr;
    LLVulkanQueuePresentKHR mQueuePresent = nullptr;
    LLVulkanDeviceWaitIdle mDeviceWaitIdle = nullptr;
    LLVulkanCmdBeginRenderPass mCmdBeginRenderPass = nullptr;
    LLVulkanCmdEndRenderPass mCmdEndRenderPass = nullptr;
    LLVulkanCmdClearAttachments mCmdClearAttachments = nullptr;
    LLVulkanDestroyShaderModule mDestroyShaderModule = nullptr;
    LLVulkanDestroyPipelineLayout mDestroyPipelineLayout = nullptr;
    LLVulkanDestroyPipeline mDestroyPipeline = nullptr;
    LLVulkanCmdBindPipeline mCmdBindPipeline = nullptr;
    LLVulkanCmdDraw mCmdDraw = nullptr;
    LLVulkanCmdBindIndexBuffer mCmdBindIndexBuffer = nullptr;
    LLVulkanCmdDrawIndexed mCmdDrawIndexed = nullptr;
    LLVulkanGetPhysicalDeviceMemoryProperties mGetPhysicalDeviceMemoryProperties = nullptr;
    LLVulkanGetPhysicalDeviceMemoryProperties2 mGetPhysicalDeviceMemoryProperties2 = nullptr;
    LLVulkanCreateBuffer mCreateBuffer = nullptr;
    LLVulkanDestroyBuffer mDestroyBuffer = nullptr;
    LLVulkanGetBufferMemoryRequirements mGetBufferMemoryRequirements = nullptr;
    LLVulkanCreateImage mCreateImage = nullptr;
    LLVulkanDestroyImage mDestroyImage = nullptr;
    LLVulkanGetImageMemoryRequirements mGetImageMemoryRequirements = nullptr;
    LLVulkanAllocateMemory mAllocateMemory = nullptr;
    LLVulkanFreeMemory mFreeMemory = nullptr;
    LLVulkanBindBufferMemory mBindBufferMemory = nullptr;
    LLVulkanBindImageMemory mBindImageMemory = nullptr;
    LLVulkanMapMemory mMapMemory = nullptr;
    LLVulkanUnmapMemory mUnmapMemory = nullptr;
    LLVulkanCmdBindVertexBuffers mCmdBindVertexBuffers = nullptr;
    LLVulkanCmdSetViewport mCmdSetViewport = nullptr;
    LLVulkanCmdSetScissor mCmdSetScissor = nullptr;
    LLVulkanCmdCopyBufferToImage mCmdCopyBufferToImage = nullptr;
    LLVulkanCreateSampler mCreateSampler = nullptr;
    LLVulkanDestroySampler mDestroySampler = nullptr;
    LLVulkanCreateDescriptorSetLayout mCreateDescriptorSetLayout = nullptr;
    LLVulkanDestroyDescriptorSetLayout mDestroyDescriptorSetLayout = nullptr;
    LLVulkanCreateDescriptorPool mCreateDescriptorPool = nullptr;
    LLVulkanDestroyDescriptorPool mDestroyDescriptorPool = nullptr;
    LLVulkanAllocateDescriptorSets mAllocateDescriptorSets = nullptr;
    LLVulkanFreeDescriptorSets mFreeDescriptorSets = nullptr;
    LLVulkanUpdateDescriptorSets mUpdateDescriptorSets = nullptr;
    LLVulkanCmdBindDescriptorSets mCmdBindDescriptorSets = nullptr;
    LLVulkanCmdPushConstants mCmdPushConstants = nullptr;
    U64 mPresentedFrameCount = 0;
    U64 mQueuedUIDrawCount = 0;
    U64 mQueuedIndexedUIDrawCount = 0;
    U64 mRecordedUIDrawCount = 0;
    U64 mRecordMissingBufferCount = 0;
    U64 mRecordMissingAttributeCount = 0;
    U64 mBufferMemoryAllocatedBytes = 0;
    U64 mEffectiveBufferMemoryBudgetBytes = 0;
    U64 mTextureUploadCount = 0;
    U64 mTextureMemoryAllocatedBytes = 0;
    U64 mEffectiveTextureMemoryBudgetBytes = 0;
    U64 mTextureUploadBudgetFrame = std::numeric_limits<U64>::max();
    U64 mTextureUploadBytesThisFrame = 0;
    U32 mTextureUploadsThisFrame = 0;
    U64 mSkippedTextureSubImageMissingResourceCount = 0;
    U64 mSkippedTextureSubImageOutOfBoundsCount = 0;
    U64 mSkippedTextureUnsupportedUploadCount = 0;
    U64 mSkippedTextureMemoryBudgetCount = 0;
    U64 mSkippedTextureUploadThrottleCount = 0;
    U64 mSkippedTextureOversizeCount = 0;
    U64 mEvictedTextureCount = 0;
    U64 mEvictedTextureMemoryBytes = 0;
    U64 mSkippedBufferMemoryBudgetCount = 0;
    U32 mLargestTextureHandle = 0;
    S32 mLargestTextureWidth = 0;
    S32 mLargestTextureHeight = 0;
    bool mFrameRenderingFailed = false;
    bool mMemoryBudgetExtensionEnabled = false;
    bool mHasMemoryProperties = false;
    bool mHasMemoryBudget = false;
    bool mLoggedFirstUIDraw = false;
    bool mLoggedFirstEmptyFrame = false;
    bool mLoggedUIDrawTelemetry = false;
    bool mLoggedTextureDescriptorCacheFull = false;
    bool mLoggedTextureMemoryBudget = false;
    bool mLoggedTextureMemoryBudgetExceeded = false;
    bool mLoggedTextureUploadThrottle = false;
    bool mLoggedTextureOversize = false;
    bool mLoggedTextureEviction = false;
    bool mLoggedBufferMemoryBudgetExceeded = false;
    bool mLoggedTransientFrameBufferBudgetExceeded = false;
    bool mLoggedHeapMemoryBudgetExceeded = false;
    bool mLoggedSkinningPaletteBufferAllocationFailed = false;
    bool mLastTextureUploadSucceeded = true;
    bool mLastTextureUploadDeferred = false;
    std::vector<LLVkPhysicalDevice> mPhysicalDevices;
    std::vector<LLVkImage> mSwapchainImages;
    std::vector<LLVkImageView> mSwapchainImageViews;
    std::vector<LLVkFramebuffer> mSwapchainFramebuffers;
    std::vector<LLVkCommandBuffer> mCommandBuffers;
    std::vector<LLVulkanFrameSync> mFrameSync;
    LLVulkanDepthAttachment mDepthAttachment;
#if LL_DARWIN
    void* mNativeView = nullptr;
    void* mMetalLayer = nullptr;
#endif
    bool mEnableVSync = false;
};

thread_local LLVulkanNativeContext* gCurrentVulkanContext = nullptr;
thread_local LLRenderViewport gCurrentVulkanViewport = {};
thread_local LLRenderScissor gCurrentVulkanScissor = {};
thread_local LLRenderClearColor gCurrentVulkanClearColor = { 0.f, 0.f, 0.f, 1.f };
thread_local bool gCurrentVulkanBlendEnabled = false;
thread_local LLRenderBlendState gCurrentVulkanBlendState;
thread_local bool gCurrentVulkanDepthTestEnabled = false;
thread_local bool gCurrentVulkanDepthWriteEnabled = true;
thread_local LLRenderDepthFunction gCurrentVulkanDepthFunction = LLRenderDepthFunction::LessEqual;
thread_local bool gCurrentVulkanCullFaceEnabled = false;
thread_local LLRenderCullFace gCurrentVulkanCullFace = LLRenderCullFace::Back;
thread_local F32 gCurrentVulkanAlphaMaskCutoff = -1.f;
thread_local bool gCurrentVulkanWorldDrawEnabled = false;
thread_local LLRenderWorldShaderClass gCurrentVulkanWorldShaderClass = LLRenderWorldShaderClass::Textured;
thread_local LLRenderWorldTerrainParameters gCurrentVulkanTerrainParameters;
thread_local LLRenderWorldMaterialParameters gCurrentVulkanMaterialParameters;
thread_local LLRenderWorldTextureTransform gCurrentVulkanTextureTransform;
thread_local std::vector<F32> gCurrentVulkanSkinningMatrixPalette;
thread_local U32 gCurrentVulkanSkinningMatrixCount = 0;
thread_local S32 gVulkanUnpackRowLength = 0;
U32 gNextVulkanBufferHandle = 1;
U32 gNextVulkanTextureHandle = 1;
U32 gNextVulkanFramebufferHandle = 1;
thread_local S32 gActiveVulkanTextureUnit = 0;
thread_local std::array<U32, 16> gBoundVulkanTextures = {};
thread_local U32 gBoundVulkanReadFramebuffer = 0;
thread_local U32 gBoundVulkanDrawFramebuffer = 0;
thread_local U32 gVulkanFramebufferColorAttachmentCount = 1;
U32 gBoundVulkanVertexBuffer = 0;
U32 gBoundVulkanIndexBuffer = 0;
std::unordered_map<U32, LLVulkanBufferResource> gVulkanBuffers;
std::unordered_map<U32, LLVulkanPendingBufferAllocation> gPendingVulkanBufferAllocations;
std::unordered_map<U32, LLVulkanTextureResource> gVulkanTextures;
std::unordered_map<U32, LLVulkanTextureSamplerState> gVulkanTextureSamplerStates;
std::unordered_map<U32, LLVulkanFramebufferResource> gVulkanFramebuffers;
std::map<LLVulkanPendingDraw::texture_bindings_t, LLVkDescriptorSet> gVulkanTextureDescriptorSetCache;
std::array<LLVulkanVertexAttributeState, 16> gCurrentVulkanVertexAttributes = {};
std::vector<LLVulkanPendingDraw> gPendingVulkanDraws;

void destroy_vulkan_framebuffer_resource(
    LLVulkanNativeContext& context,
    LLVulkanFramebufferResource& resource)
{
    if (resource.mFramebuffer && context.mDestroyFramebuffer && context.mDevice)
    {
        context.mDestroyFramebuffer(context.mDevice, resource.mFramebuffer, nullptr);
    }
    resource.mFramebuffer = nullptr;
    resource.mRenderPassKey = 0;
    resource.mWidth = 0;
    resource.mHeight = 0;
    resource.mDirty = true;
}

void destroy_all_vulkan_framebuffer_resources(LLVulkanNativeContext& context)
{
    for (auto& entry : gVulkanFramebuffers)
    {
        destroy_vulkan_framebuffer_resource(context, entry.second);
    }
}

void invalidate_vulkan_framebuffers_for_texture(U32 texture)
{
    if (!texture)
    {
        return;
    }

    for (auto& entry : gVulkanFramebuffers)
    {
        LLVulkanFramebufferResource& framebuffer = entry.second;
        if (framebuffer.mDepthTexture == texture ||
            std::find(
                framebuffer.mColorTextures.begin(),
                framebuffer.mColorTextures.end(),
                texture) != framebuffer.mColorTextures.end())
        {
            framebuffer.mDirty = true;
        }
    }
}

U64 make_vulkan_offscreen_render_pass_key(
    const std::array<S32, 4>& color_formats,
    U32 color_format_count,
    S32 depth_format)
{
    U64 hash = 1469598103934665603ULL;
    auto mix = [&hash](U32 value)
    {
        hash ^= value;
        hash *= 1099511628211ULL;
    };

    mix(color_format_count);
    const U32 max_color_format_count = static_cast<U32>(color_formats.size());
    for (U32 i = 0; i < color_format_count && i < max_color_format_count; ++i)
    {
        mix(static_cast<U32>(color_formats[i]));
    }
    mix(static_cast<U32>(depth_format));
    return hash;
}

LLVkRenderPass get_vulkan_offscreen_render_pass(
    LLVulkanNativeContext& context,
    const std::array<S32, 4>& color_formats,
    U32 color_format_count,
    S32 depth_format);

LLVkFramebuffer get_vulkan_offscreen_framebuffer(
    LLVulkanNativeContext& context,
    U32 framebuffer_handle)
{
    if (!framebuffer_handle ||
        !context.mCreateFramebuffer ||
        !context.mDestroyFramebuffer)
    {
        return nullptr;
    }

    auto framebuffer_iter = gVulkanFramebuffers.find(framebuffer_handle);
    if (framebuffer_iter == gVulkanFramebuffers.end())
    {
        return nullptr;
    }

    LLVulkanFramebufferResource& framebuffer = framebuffer_iter->second;
    const U32 color_attachment_count =
        llclamp(framebuffer.mColorAttachmentCount, 0U, static_cast<U32>(framebuffer.mColorTextures.size()));
    if (color_attachment_count == 0 ||
        !framebuffer.mDepthTexture)
    {
        return nullptr;
    }

    std::array<LLVkImageView, 5> attachments = {};
    std::array<S32, 4> color_formats = {};
    S32 width = 0;
    S32 height = 0;
    for (U32 i = 0; i < color_attachment_count; ++i)
    {
        U32 color_texture_handle = framebuffer.mColorTextures[i];
        if (!color_texture_handle)
        {
            return nullptr;
        }

        auto color_iter = gVulkanTextures.find(color_texture_handle);
        if (color_iter == gVulkanTextures.end())
        {
            return nullptr;
        }

        const LLVulkanTextureResource& color_texture = color_iter->second;
        if (!color_texture.mImageView ||
            color_texture.mAspectMask != LL_VK_IMAGE_ASPECT_COLOR_BIT ||
            color_texture.mWidth <= 0 ||
            color_texture.mHeight <= 0)
        {
            return nullptr;
        }

        if (i == 0)
        {
            width = color_texture.mWidth;
            height = color_texture.mHeight;
        }
        else if (color_texture.mWidth != width ||
            color_texture.mHeight != height)
        {
            return nullptr;
        }

        attachments[i] = color_texture.mImageView;
        color_formats[i] = color_texture.mFormat;
    }

    auto depth_iter = gVulkanTextures.find(framebuffer.mDepthTexture);
    if (depth_iter == gVulkanTextures.end())
    {
        return nullptr;
    }

    const LLVulkanTextureResource& depth_texture = depth_iter->second;
    if (!depth_texture.mImageView ||
        depth_texture.mAspectMask != LL_VK_IMAGE_ASPECT_DEPTH_BIT ||
        depth_texture.mWidth != width ||
        depth_texture.mHeight != height)
    {
        return nullptr;
    }
    attachments[color_attachment_count] = depth_texture.mImageView;

    const U64 render_pass_key =
        make_vulkan_offscreen_render_pass_key(color_formats, color_attachment_count, depth_texture.mFormat);
    LLVkRenderPass render_pass =
        get_vulkan_offscreen_render_pass(
            context,
            color_formats,
            color_attachment_count,
            depth_texture.mFormat);
    if (!render_pass)
    {
        return nullptr;
    }

    if (!framebuffer.mDirty &&
        framebuffer.mFramebuffer &&
        framebuffer.mRenderPassKey == render_pass_key &&
        framebuffer.mWidth == width &&
        framebuffer.mHeight == height)
    {
        return framebuffer.mFramebuffer;
    }

    destroy_vulkan_framebuffer_resource(context, framebuffer);

    LLVkFramebufferCreateInfo create_info =
    {
        LL_VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        nullptr,
        0,
        render_pass,
        color_attachment_count + 1,
        attachments.data(),
        static_cast<U32>(width),
        static_cast<U32>(height),
        1
    };

    S32 result = context.mCreateFramebuffer(
        context.mDevice,
        &create_info,
        nullptr,
        &framebuffer.mFramebuffer);
    if (result != LL_VK_SUCCESS || !framebuffer.mFramebuffer)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateFramebuffer(offscreen) failed with result "
            << result
            << LL_ENDL;
        framebuffer.mFramebuffer = nullptr;
        framebuffer.mDirty = true;
        framebuffer.mWidth = 0;
        framebuffer.mHeight = 0;
        return nullptr;
    }

    framebuffer.mWidth = width;
    framebuffer.mHeight = height;
    framebuffer.mRenderPassKey = render_pass_key;
    framebuffer.mDirty = false;
    return framebuffer.mFramebuffer;
}

void destroy_vulkan_texture_descriptor_set_cache(LLVulkanNativeContext& context)
{
    if (gVulkanTextureDescriptorSetCache.empty())
    {
        return;
    }

    if (context.mFreeDescriptorSets &&
        context.mDevice &&
        context.mUIDescriptorPool)
    {
        std::vector<LLVkDescriptorSet> descriptor_sets;
        descriptor_sets.reserve(gVulkanTextureDescriptorSetCache.size());
        for (const auto& entry : gVulkanTextureDescriptorSetCache)
        {
            if (entry.second)
            {
                descriptor_sets.push_back(entry.second);
            }
        }

        if (!descriptor_sets.empty())
        {
            const S32 result = context.mFreeDescriptorSets(
                context.mDevice,
                context.mUIDescriptorPool,
                static_cast<U32>(descriptor_sets.size()),
                descriptor_sets.data());
            if (result != LL_VK_SUCCESS)
            {
                LL_WARNS("RenderBackend")
                    << "vkFreeDescriptorSets(texture cache) failed with result "
                    << result
                    << LL_ENDL;
            }
        }
    }

    gVulkanTextureDescriptorSetCache.clear();
}

const LLVulkanLoader& get_vulkan_loader()
{
    static const LLVulkanLoader loader;
    return loader;
}

const LLVulkanProbeResult& get_vulkan_probe_result()
{
    return get_vulkan_loader().getProbeResult();
}

void* get_vulkan_device_proc_address(const LLVulkanNativeContext& context, const char* name)
{
    const LLVulkanLoader& loader = get_vulkan_loader();
    void* proc = loader.getDeviceProcAddress(context.mDevice, name);
    if (proc)
    {
        return proc;
    }

    LLVulkanGetDeviceProcAddr get_device_proc_addr =
        reinterpret_cast<LLVulkanGetDeviceProcAddr>(
            loader.getInstanceProcAddress(context.mInstance, "vkGetDeviceProcAddr"));
    return get_device_proc_addr ? get_device_proc_addr(context.mDevice, name) : nullptr;
}

bool has_vulkan_extension(
    const std::vector<LLVkExtensionProperties>& extensions,
    const char* extension_name)
{
    for (const LLVkExtensionProperties& extension : extensions)
    {
        if (std::strcmp(extension.extensionName, extension_name) == 0)
        {
            return true;
        }
    }

    return false;
}

bool get_vulkan_boolean_env(const char* name)
{
    std::string value = LLStringUtil::getenv(name);
    LLStringUtil::trim(value);
    LLStringUtil::toLower(value);
    return value == "1" ||
        value == "true" ||
        value == "yes" ||
        value == "on";
}

bool should_continue_after_vulkan_probe()
{
    return get_vulkan_boolean_env("MARE_VULKAN_CONTINUE_AFTER_PROBE");
}

S32 to_vulkan_topology(LLRenderPrimitiveType mode)
{
    switch (mode)
    {
    case LLRenderPrimitiveType::Points:
        return LL_VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case LLRenderPrimitiveType::Lines:
        return LL_VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case LLRenderPrimitiveType::LineStrip:
    case LLRenderPrimitiveType::LineLoop:
        return LL_VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case LLRenderPrimitiveType::TriangleStrip:
        return LL_VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case LLRenderPrimitiveType::TriangleFan:
        return LL_VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    case LLRenderPrimitiveType::Triangles:
    default:
        return LL_VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

S32 to_vulkan_index_type(LLRenderIndexType type)
{
    switch (type)
    {
    case LLRenderIndexType::UnsignedInt:
        return LL_VK_INDEX_TYPE_UINT32;
    case LLRenderIndexType::UnsignedShort:
    default:
        return LL_VK_INDEX_TYPE_UINT16;
    }
}

U32 to_vulkan_ui_pipeline_index(LLRenderPrimitiveType mode)
{
    return static_cast<U32>(mode);
}

LLVulkanWorldBlendPipeline to_vulkan_world_blend_pipeline()
{
    if (!gCurrentVulkanBlendEnabled)
    {
        return LLVulkanWorldBlendPipeline::Opaque;
    }

    if (gCurrentVulkanBlendState.mColorSource == LLRenderBlendFactor::SourceAlpha &&
        gCurrentVulkanBlendState.mColorDestination == LLRenderBlendFactor::One)
    {
        return LLVulkanWorldBlendPipeline::Add;
    }

    return LLVulkanWorldBlendPipeline::Alpha;
}

LLVulkanWorldDepthPipeline to_vulkan_world_depth_pipeline()
{
    if (!gCurrentVulkanDepthTestEnabled)
    {
        return LLVulkanWorldDepthPipeline::Disabled;
    }

    return gCurrentVulkanDepthWriteEnabled ?
        LLVulkanWorldDepthPipeline::ReadWrite :
        LLVulkanWorldDepthPipeline::ReadOnly;
}

LLVulkanWorldCullPipeline to_vulkan_world_cull_pipeline()
{
    return gCurrentVulkanCullFaceEnabled && gCurrentVulkanCullFace == LLRenderCullFace::Back ?
        LLVulkanWorldCullPipeline::Back :
        LLVulkanWorldCullPipeline::None;
}

U32 to_vulkan_world_pipeline_index(
    U32 primitive_pipeline_index,
    LLVulkanWorldBlendPipeline blend_pipeline,
    LLVulkanWorldDepthPipeline depth_pipeline,
    LLVulkanWorldCullPipeline cull_pipeline)
{
    return primitive_pipeline_index +
        MARE_VULKAN_PRIMITIVE_PIPELINE_COUNT *
            (static_cast<U32>(blend_pipeline) +
             MARE_VULKAN_WORLD_BLEND_PIPELINE_COUNT *
                (static_cast<U32>(depth_pipeline) +
                 MARE_VULKAN_WORLD_DEPTH_PIPELINE_COUNT * static_cast<U32>(cull_pipeline)));
}

U32 to_vulkan_buffer_usage(LLRenderBufferTarget target)
{
    switch (target)
    {
    case LLRenderBufferTarget::Index:
        return LL_VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case LLRenderBufferTarget::Vertex:
    default:
        return LL_VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
}

F32 get_vulkan_viewport_scale_x(
    const LLVulkanNativeContext& context,
    const LLRenderViewport& viewport)
{
    if (context.mNativeViewExtent.width > 0 &&
        context.mDrawableScaleX > 1.01f &&
        viewport.mWidth > 0.f &&
        viewport.mWidth <= static_cast<F32>(context.mNativeViewExtent.width + 1))
    {
        return context.mDrawableScaleX;
    }

    return 1.f;
}

F32 get_vulkan_viewport_scale_y(
    const LLVulkanNativeContext& context,
    const LLRenderViewport& viewport)
{
    if (context.mNativeViewExtent.height > 0 &&
        context.mDrawableScaleY > 1.01f &&
        viewport.mHeight > 0.f &&
        viewport.mHeight <= static_cast<F32>(context.mNativeViewExtent.height + 1))
    {
        return context.mDrawableScaleY;
    }

    return 1.f;
}

U32 get_vulkan_reported_video_memory_megabytes(const LLVulkanNativeContext& context);

bool refresh_vulkan_memory_properties(LLVulkanNativeContext& context)
{
    if (!context.mPhysicalDevice || !context.mGetPhysicalDeviceMemoryProperties)
    {
        return false;
    }

    context.mMemoryHeapBudgetBytes.fill(0);
    context.mMemoryHeapUsageBytes.fill(0);
    context.mHasMemoryBudget = false;

    if (context.mMemoryBudgetExtensionEnabled && context.mGetPhysicalDeviceMemoryProperties2)
    {
        LLVkPhysicalDeviceMemoryBudgetPropertiesEXT budget_properties =
        {
            LL_VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT,
            nullptr,
            {},
            {}
        };
        LLVkPhysicalDeviceMemoryProperties2 memory_properties =
        {
            LL_VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
            &budget_properties,
            {}
        };

        context.mGetPhysicalDeviceMemoryProperties2(context.mPhysicalDevice, &memory_properties);
        context.mMemoryProperties = memory_properties.memoryProperties;
        context.mHasMemoryProperties = true;
        context.mHasMemoryBudget = true;

        const U32 heap_count = llmin<U32>(context.mMemoryProperties.memoryHeapCount, 16);
        for (U32 i = 0; i < heap_count; ++i)
        {
            context.mMemoryHeapBudgetBytes[i] = budget_properties.heapBudget[i];
            context.mMemoryHeapUsageBytes[i] = budget_properties.heapUsage[i];
        }
        context.mReportedVideoMemoryMB =
            get_vulkan_reported_video_memory_megabytes(context);
        return true;
    }

    context.mGetPhysicalDeviceMemoryProperties(context.mPhysicalDevice, &context.mMemoryProperties);
    context.mHasMemoryProperties = true;

    const U32 heap_count = llmin<U32>(context.mMemoryProperties.memoryHeapCount, 16);
    for (U32 i = 0; i < heap_count; ++i)
    {
        context.mMemoryHeapBudgetBytes[i] = context.mMemoryProperties.memoryHeaps[i].size;
    }
    context.mReportedVideoMemoryMB =
        get_vulkan_reported_video_memory_megabytes(context);
    return true;
}

bool vulkan_heap_has_host_visible_memory_type(
    const LLVulkanNativeContext& context,
    U32 heap_index)
{
    if (!context.mHasMemoryProperties)
    {
        return false;
    }

    const U32 memory_type_count = llmin<U32>(context.mMemoryProperties.memoryTypeCount, 32);
    for (U32 i = 0; i < memory_type_count; ++i)
    {
        const LLVkMemoryType& memory_type = context.mMemoryProperties.memoryTypes[i];
        if (memory_type.heapIndex == heap_index &&
            (memory_type.propertyFlags & LL_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
        {
            return true;
        }
    }
    return false;
}

void log_vulkan_memory_properties(const LLVulkanNativeContext& context)
{
    if (!context.mHasMemoryProperties)
    {
        return;
    }

    const U32 heap_count = llmin<U32>(context.mMemoryProperties.memoryHeapCount, 16);
    LL_INFOS("RenderBackend")
        << "Vulkan memory budget extension "
        << (context.mHasMemoryBudget ? "enabled" : "unavailable")
        << "; reporting "
        << heap_count
        << " heap(s)."
        << LL_ENDL;

    for (U32 i = 0; i < heap_count; ++i)
    {
        const LLVkMemoryHeap& heap = context.mMemoryProperties.memoryHeaps[i];
        const bool device_local = (heap.flags & LL_VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0;
        const bool host_visible = vulkan_heap_has_host_visible_memory_type(context, i);
        LL_INFOS("RenderBackend")
            << "Vulkan heap "
            << i
            << ": size "
            << format_vulkan_megabytes(heap.size)
            << ", budget "
            << format_vulkan_megabytes(context.mMemoryHeapBudgetBytes[i])
            << ", usage "
            << format_vulkan_megabytes(context.mMemoryHeapUsageBytes[i])
            << ", device-local "
            << device_local
            << ", host-visible-type "
            << host_visible
            << "."
            << LL_ENDL;
    }
}

U64 get_vulkan_reported_video_memory_bytes(const LLVulkanNativeContext& context)
{
    if (!context.mHasMemoryProperties)
    {
        return 0;
    }

    U64 largest_device_local_budget = 0;
    U64 largest_heap_budget = 0;
    const U32 heap_count = llmin<U32>(context.mMemoryProperties.memoryHeapCount, 16);
    for (U32 i = 0; i < heap_count; ++i)
    {
        const LLVkMemoryHeap& heap = context.mMemoryProperties.memoryHeaps[i];
        U64 budget = context.mMemoryHeapBudgetBytes[i];
        if (budget == 0)
        {
            budget = heap.size;
        }

        largest_heap_budget = llmax(largest_heap_budget, budget);
        if ((heap.flags & LL_VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
        {
            largest_device_local_budget = llmax(largest_device_local_budget, budget);
        }
    }

    return largest_device_local_budget > 0 ? largest_device_local_budget : largest_heap_budget;
}

U64 get_vulkan_reported_free_video_memory_bytes(const LLVulkanNativeContext& context)
{
    if (!context.mHasMemoryProperties)
    {
        return 0;
    }

    U64 selected_budget = 0;
    U64 selected_usage = 0;
    U64 largest_any_budget = 0;
    U64 largest_any_usage = 0;
    const U32 heap_count = llmin<U32>(context.mMemoryProperties.memoryHeapCount, 16);
    for (U32 i = 0; i < heap_count; ++i)
    {
        const LLVkMemoryHeap& heap = context.mMemoryProperties.memoryHeaps[i];
        U64 budget = context.mMemoryHeapBudgetBytes[i];
        if (budget == 0)
        {
            budget = heap.size;
        }
        const U64 usage = context.mMemoryHeapUsageBytes[i];

        if (budget > largest_any_budget)
        {
            largest_any_budget = budget;
            largest_any_usage = usage;
        }

        if ((heap.flags & LL_VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0 &&
            budget > selected_budget)
        {
            selected_budget = budget;
            selected_usage = usage;
        }
    }

    if (selected_budget == 0)
    {
        selected_budget = largest_any_budget;
        selected_usage = largest_any_usage;
    }
    return selected_budget > selected_usage ? selected_budget - selected_usage : 0;
}

U32 get_vulkan_reported_video_memory_megabytes(const LLVulkanNativeContext& context)
{
    const U64 bytes = get_vulkan_reported_video_memory_bytes(context);
    return static_cast<U32>(
        std::min<U64>(
            bytes / MARE_VULKAN_BYTES_PER_MEGABYTE,
            std::numeric_limits<U32>::max()));
}

U32 get_vulkan_memory_type_heap_index(
    const LLVulkanNativeContext& context,
    U32 memory_type_index)
{
    if (!context.mHasMemoryProperties ||
        memory_type_index >= context.mMemoryProperties.memoryTypeCount ||
        memory_type_index >= 32)
    {
        return 16;
    }

    return context.mMemoryProperties.memoryTypes[memory_type_index].heapIndex;
}

U64 get_vulkan_heap_memory_reserve_bytes()
{
    static const U64 reserve_bytes = []()
    {
        U64 reserve_mb = MARE_VULKAN_DEFAULT_HEAP_MEMORY_RESERVE_MB;
        if (const char* reserve_override = std::getenv("MARE_VULKAN_HEAP_MEMORY_RESERVE_MB"))
        {
            reserve_mb = std::strtoull(reserve_override, nullptr, 10);
        }
        return reserve_mb * MARE_VULKAN_BYTES_PER_MEGABYTE;
    }();

    return reserve_bytes;
}

bool get_vulkan_memory_budget_override_bytes(
    const char* name,
    U64& budget_bytes)
{
    if (const char* budget_override = std::getenv(name))
    {
        const U64 parsed_budget = std::strtoull(budget_override, nullptr, 10);
        if (parsed_budget > 0)
        {
            budget_bytes = parsed_budget * MARE_VULKAN_BYTES_PER_MEGABYTE;
            return true;
        }
    }
    return false;
}

U64 get_vulkan_derived_memory_budget_bytes(
    LLVulkanNativeContext& context,
    U32 memory_type_index,
    U64 fallback_budget,
    U32 unified_heap_divisor,
    U32 dedicated_heap_numerator,
    U32 dedicated_heap_denominator)
{
    if (!context.mMemoryBudgetExtensionEnabled ||
        !context.mGetPhysicalDeviceMemoryProperties2 ||
        !refresh_vulkan_memory_properties(context) ||
        !context.mHasMemoryBudget)
    {
        return fallback_budget;
    }

    const U32 heap_index = get_vulkan_memory_type_heap_index(context, memory_type_index);
    if (heap_index >= 16 ||
        heap_index >= context.mMemoryProperties.memoryHeapCount)
    {
        return fallback_budget;
    }

    const U64 heap_budget = context.mMemoryHeapBudgetBytes[heap_index];
    if (heap_budget == 0)
    {
        return fallback_budget;
    }

    const U64 heap_reserve = llmin(get_vulkan_heap_memory_reserve_bytes(), heap_budget);
    const U64 usable_budget = heap_budget > heap_reserve ? heap_budget - heap_reserve : 0;
    if (usable_budget == 0)
    {
        return fallback_budget;
    }

    const LLVkMemoryType& memory_type = context.mMemoryProperties.memoryTypes[memory_type_index];
    const bool device_local =
        (context.mMemoryProperties.memoryHeaps[heap_index].flags &
         LL_VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0;
    const bool host_visible =
        (memory_type.propertyFlags & LL_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
    const U64 conservative_floor = llmin(fallback_budget, 256 * MARE_VULKAN_BYTES_PER_MEGABYTE);

    if (device_local && !host_visible && dedicated_heap_denominator > 0)
    {
        const U64 dedicated_budget =
            (usable_budget / dedicated_heap_denominator) * dedicated_heap_numerator;
        return llmax(fallback_budget, dedicated_budget);
    }

    const U64 unified_budget =
        unified_heap_divisor > 0 ?
        usable_budget / unified_heap_divisor :
        fallback_budget;
    return llmin(fallback_budget, llmax(conservative_floor, unified_budget));
}

bool can_commit_vulkan_heap_memory(
    LLVulkanNativeContext& context,
    U32 memory_type_index,
    U64 size,
    const char* allocation_label)
{
    if (!context.mMemoryBudgetExtensionEnabled ||
        !context.mGetPhysicalDeviceMemoryProperties2 ||
        !refresh_vulkan_memory_properties(context) ||
        !context.mHasMemoryBudget)
    {
        return true;
    }

    const U32 heap_index = get_vulkan_memory_type_heap_index(context, memory_type_index);
    if (heap_index >= 16)
    {
        return true;
    }

    const U64 heap_budget = context.mMemoryHeapBudgetBytes[heap_index];
    const U64 heap_usage = context.mMemoryHeapUsageBytes[heap_index];
    if (heap_budget == 0)
    {
        return true;
    }

    const U64 heap_reserve = llmin(get_vulkan_heap_memory_reserve_bytes(), heap_budget);
    const U64 available =
        heap_budget > heap_usage + heap_reserve ?
        heap_budget - heap_usage - heap_reserve :
        0;
    if (size <= available)
    {
        return true;
    }

    if (!context.mLoggedHeapMemoryBudgetExceeded)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan "
            << allocation_label
            << " allocation refused by heap memory budget. Heap "
            << heap_index
            << " budget "
            << format_vulkan_megabytes(heap_budget)
            << ", usage "
            << format_vulkan_megabytes(heap_usage)
            << ", reserve "
            << format_vulkan_megabytes(heap_reserve)
            << ", available "
            << format_vulkan_megabytes(available)
            << ", requested "
            << format_vulkan_megabytes(size)
            << ". Override reserve with MARE_VULKAN_HEAP_MEMORY_RESERVE_MB for testing."
            << LL_ENDL;
        context.mLoggedHeapMemoryBudgetExceeded = true;
    }
    return false;
}

bool find_vulkan_memory_type(
    LLVulkanNativeContext& context,
    U32 type_filter,
    U32 properties,
    U32& memory_type_index)
{
    if (!context.mHasMemoryProperties &&
        !refresh_vulkan_memory_properties(context))
    {
        return false;
    }

    const LLVkPhysicalDeviceMemoryProperties& memory_properties = context.mMemoryProperties;
    for (U32 i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1u << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            memory_type_index = i;
            return true;
        }
    }

    return false;
}

LLVkViewport to_vulkan_viewport(
    const LLVulkanNativeContext& context,
    const LLRenderViewport& viewport)
{
    if (viewport.mWidth <= 0.f || viewport.mHeight <= 0.f)
    {
        return LLVkViewport
        {
            0.f,
            0.f,
            static_cast<F32>(context.mSwapchainExtent.width),
            static_cast<F32>(context.mSwapchainExtent.height),
            0.f,
            1.f
        };
    }

    F32 scale_x = get_vulkan_viewport_scale_x(context, viewport);
    F32 scale_y = get_vulkan_viewport_scale_y(context, viewport);
    return LLVkViewport
    {
        viewport.mX * scale_x,
        viewport.mY * scale_y,
        viewport.mWidth * scale_x,
        viewport.mHeight * scale_y,
        viewport.mMinDepth,
        viewport.mMaxDepth
    };
}

LLVkRect2D to_vulkan_scissor(
    const LLVulkanNativeContext& context,
    const LLRenderViewport& viewport,
    const LLRenderScissor& scissor)
{
    if (!scissor.mEnabled || scissor.mWidth <= 0 || scissor.mHeight <= 0)
    {
        return LLVkRect2D
        {
            LLVkOffset2D { 0, 0 },
            context.mSwapchainExtent
        };
    }

    F32 scale_x = get_vulkan_viewport_scale_x(context, viewport);
    F32 scale_y = get_vulkan_viewport_scale_y(context, viewport);
    S32 scaled_x = static_cast<S32>(std::lround(static_cast<F32>(scissor.mX) * scale_x));
    S32 scaled_y = static_cast<S32>(std::lround(static_cast<F32>(scissor.mY) * scale_y));
    S32 scaled_width = static_cast<S32>(std::lround(static_cast<F32>(scissor.mWidth) * scale_x));
    S32 scaled_height = static_cast<S32>(std::lround(static_cast<F32>(scissor.mHeight) * scale_y));

    S32 y = static_cast<S32>(context.mSwapchainExtent.height) - scaled_y - scaled_height;
    S32 x = llclamp(scaled_x, 0, static_cast<S32>(context.mSwapchainExtent.width));
    y = llclamp(y, 0, static_cast<S32>(context.mSwapchainExtent.height));
    S32 width = llclamp(scaled_width, 0, static_cast<S32>(context.mSwapchainExtent.width) - x);
    S32 height = llclamp(scaled_height, 0, static_cast<S32>(context.mSwapchainExtent.height) - y);

    return LLVkRect2D
    {
        LLVkOffset2D { x, y },
        LLVkExtent2D
        {
            static_cast<U32>(width),
            static_cast<U32>(height)
        }
    };
}

void record_vulkan_clear_command(
    LLVulkanNativeContext& context,
    LLVkCommandBuffer command_buffer,
    const LLVulkanPendingDraw& clear_command)
{
    if (!context.mCmdClearAttachments || clear_command.mClearMask == LL_RENDER_CLEAR_NONE)
    {
        return;
    }
    if (clear_command.mFramebuffer != 0)
    {
        (void)get_vulkan_offscreen_framebuffer(context, clear_command.mFramebuffer);
        return;
    }

    std::array<LLVkClearAttachment, 2> attachments = {};
    U32 attachment_count = 0;

    if (clear_command.mClearMask & LL_RENDER_CLEAR_COLOR)
    {
        LLVkClearAttachment& attachment = attachments[attachment_count++];
        attachment.aspectMask = LL_VK_IMAGE_ASPECT_COLOR_BIT;
        attachment.colorAttachment = 0;
        attachment.clearValue.color[0] = clear_command.mClearColor.mRed;
        attachment.clearValue.color[1] = clear_command.mClearColor.mGreen;
        attachment.clearValue.color[2] = clear_command.mClearColor.mBlue;
        attachment.clearValue.color[3] = clear_command.mClearColor.mAlpha;
    }

    if (clear_command.mClearMask & LL_RENDER_CLEAR_DEPTH)
    {
        LLVkClearAttachment& attachment = attachments[attachment_count++];
        attachment.aspectMask = LL_VK_IMAGE_ASPECT_DEPTH_BIT;
        attachment.clearValue.depthStencil.depth = clear_command.mClearDepth;
        attachment.clearValue.depthStencil.stencil = clear_command.mClearStencil;
    }

    if (attachment_count == 0)
    {
        return;
    }

    LLVkRect2D clear_rect =
        to_vulkan_scissor(context, clear_command.mViewport, clear_command.mScissor);
    LLVkClearRect rect =
    {
        clear_rect,
        0,
        1
    };

    context.mCmdClearAttachments(
        command_buffer,
        attachment_count,
        attachments.data(),
        1,
        &rect);
}

bool read_vulkan_draw_position(
    const LLVulkanPendingDraw& draw,
    const LLVulkanBufferResource& vertex_buffer,
    U32 vertex_index,
    F32& x,
    F32& y)
{
    if (!vertex_buffer.mMappedData || !draw.mAttributes[0].mEnabled)
    {
        return false;
    }

    const U32 stride = draw.mAttributes[0].mStride > 0 ? draw.mAttributes[0].mStride : 16;
    const U64 offset = draw.mAttributes[0].mOffset + static_cast<U64>(vertex_index) * stride;
    if (offset + sizeof(F32) * 3 > vertex_buffer.mSize)
    {
        return false;
    }

    const U8* bytes = static_cast<const U8*>(vertex_buffer.mMappedData);
    const F32* position = reinterpret_cast<const F32*>(bytes + offset);
    if (!std::isfinite(position[0]) || !std::isfinite(position[1]))
    {
        return false;
    }

    x = position[0];
    y = position[1];
    return true;
}

bool read_vulkan_draw_index(
    const LLVulkanPendingDraw& draw,
    const LLVulkanBufferResource& index_buffer,
    U32 draw_vertex,
    U32& vertex_index)
{
    if (!index_buffer.mMappedData)
    {
        return false;
    }

    const bool uint32_indices = draw.mIndexType == LL_VK_INDEX_TYPE_UINT32;
    const U32 index_size = uint32_indices ? 4 : 2;
    const U64 offset = static_cast<U64>(draw.mFirst + draw_vertex) * index_size;
    if (offset + index_size > index_buffer.mSize)
    {
        return false;
    }

    const U8* bytes = static_cast<const U8*>(index_buffer.mMappedData) + offset;
    if (uint32_indices)
    {
        std::memcpy(&vertex_index, bytes, sizeof(vertex_index));
    }
    else
    {
        U16 vertex_index_16 = 0;
        std::memcpy(&vertex_index_16, bytes, sizeof(vertex_index_16));
        vertex_index = static_cast<U32>(vertex_index_16);
    }
    return true;
}

bool read_vulkan_draw_color(
    const LLVulkanPendingDraw& draw,
    const LLVulkanBufferResource& vertex_buffer,
    U32 vertex_index,
    LLVulkanDrawColor& color)
{
    if (!vertex_buffer.mMappedData || !draw.mAttributes[6].mEnabled)
    {
        return false;
    }

    const U32 stride = draw.mAttributes[6].mStride > 0 ? draw.mAttributes[6].mStride : 4;
    const U64 offset = draw.mAttributes[6].mOffset + static_cast<U64>(vertex_index) * stride;
    if (offset + 4 > vertex_buffer.mSize)
    {
        return false;
    }

    const U8* bytes = static_cast<const U8*>(vertex_buffer.mMappedData) + offset;
    color.mValid = true;
    color.mR = bytes[0];
    color.mG = bytes[1];
    color.mB = bytes[2];
    color.mA = bytes[3];
    return true;
}

LLVulkanDrawColor read_vulkan_draw_first_color(
    const LLVulkanPendingDraw& draw,
    const LLVulkanBufferResource& vertex_buffer,
    const LLVulkanBufferResource* index_buffer)
{
    LLVulkanDrawColor color;
    U32 vertex_index = static_cast<U32>(draw.mFirst);
    if (draw.mIndexed)
    {
        if (!index_buffer || !read_vulkan_draw_index(draw, *index_buffer, 0, vertex_index))
        {
            return color;
        }
    }

    read_vulkan_draw_color(draw, vertex_buffer, vertex_index, color);
    return color;
}

LLVulkanDrawBounds compute_vulkan_draw_bounds(
    const LLVulkanPendingDraw& draw,
    const LLVulkanBufferResource& vertex_buffer,
    const LLVulkanBufferResource* index_buffer)
{
    LLVulkanDrawBounds bounds;
    for (S32 i = 0; i < draw.mCount; ++i)
    {
        U32 vertex_index = static_cast<U32>(draw.mFirst + i);
        if (draw.mIndexed)
        {
            if (!index_buffer || !read_vulkan_draw_index(draw, *index_buffer, static_cast<U32>(i), vertex_index))
            {
                continue;
            }
        }

        F32 x = 0.f;
        F32 y = 0.f;
        if (!read_vulkan_draw_position(draw, vertex_buffer, vertex_index, x, y))
        {
            continue;
        }

        if (!bounds.mValid)
        {
            bounds.mMinX = bounds.mMaxX = x;
            bounds.mMinY = bounds.mMaxY = y;
            bounds.mValid = true;
        }
        else
        {
            bounds.mMinX = llmin(bounds.mMinX, x);
            bounds.mMaxX = llmax(bounds.mMaxX, x);
            bounds.mMinY = llmin(bounds.mMinY, y);
            bounds.mMaxY = llmax(bounds.mMaxY, y);
        }
    }
    return bounds;
}

bool find_vulkan_draw_max_vertex_index(
    const LLVulkanPendingDraw& draw,
    const LLVulkanBufferResource* index_buffer,
    U32& max_vertex_index)
{
    if (draw.mCount <= 0)
    {
        return false;
    }

    max_vertex_index = 0;
    if (!draw.mIndexed)
    {
        max_vertex_index = static_cast<U32>(draw.mFirst + draw.mCount - 1);
        return true;
    }

    if (!index_buffer)
    {
        return false;
    }

    bool found_index = false;
    for (S32 i = 0; i < draw.mCount; ++i)
    {
        U32 vertex_index = 0;
        if (!read_vulkan_draw_index(draw, *index_buffer, static_cast<U32>(i), vertex_index))
        {
            return false;
        }
        max_vertex_index = found_index ? llmax(max_vertex_index, vertex_index) : vertex_index;
        found_index = true;
    }
    return found_index;
}

bool read_vulkan_vertex_vec3(
    const LLVulkanBufferResource& vertex_buffer,
    const LLVulkanVertexAttributeState& attribute,
    U32 vertex_index,
    glm::vec3& value)
{
    const U32 stride = attribute.mStride > 0 ? attribute.mStride : 16;
    const U64 offset = attribute.mOffset + static_cast<U64>(vertex_index) * stride;
    if (!vertex_buffer.mMappedData || offset + sizeof(F32) * 3 > vertex_buffer.mSize)
    {
        return false;
    }

    const F32* source =
        reinterpret_cast<const F32*>(static_cast<const U8*>(vertex_buffer.mMappedData) + offset);
    value = glm::vec3(source[0], source[1], source[2]);
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool read_vulkan_vertex_weight4(
    const LLVulkanBufferResource& vertex_buffer,
    const LLVulkanVertexAttributeState& attribute,
    U32 vertex_index,
    glm::vec4& value)
{
    const U32 stride = attribute.mStride > 0 ? attribute.mStride : 16;
    const U64 offset = attribute.mOffset + static_cast<U64>(vertex_index) * stride;
    if (!vertex_buffer.mMappedData || offset + sizeof(F32) * 4 > vertex_buffer.mSize)
    {
        return false;
    }

    const F32* source =
        reinterpret_cast<const F32*>(static_cast<const U8*>(vertex_buffer.mMappedData) + offset);
    value = glm::vec4(source[0], source[1], source[2], source[3]);
    return std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z) && std::isfinite(value.w);
}

glm::vec3 skin_vulkan_position(
    const glm::vec3& position,
    const glm::vec4& packed_weights,
    const LLVulkanPendingDraw& draw)
{
    glm::vec4 indices = glm::floor(packed_weights);
    glm::vec4 weights = packed_weights - indices;
    F32 weight_sum = weights.x + weights.y + weights.z + weights.w;
    if (weight_sum <= 0.f)
    {
        weights = glm::vec4(1.f, 0.f, 0.f, 0.f);
        weight_sum = 1.f;
    }
    weights /= weight_sum;

    glm::vec3 skinned(0.f);
    for (U32 i = 0; i < 4; ++i)
    {
        const F32 weight = weights[i];
        if (weight <= 0.f)
        {
            continue;
        }

        const U32 matrix_index = static_cast<U32>(
            llclamp(
                static_cast<S32>(indices[i]),
                0,
                static_cast<S32>(draw.mSkinningMatrixCount - 1)));
        const F32* matrix = draw.mSkinningMatrixPalette.data() + matrix_index * 12;
        glm::vec3 transformed(
            matrix[0] * position.x + matrix[4] * position.y + matrix[8] * position.z + matrix[3],
            matrix[1] * position.x + matrix[5] * position.y + matrix[9] * position.z + matrix[7],
            matrix[2] * position.x + matrix[6] * position.y + matrix[10] * position.z + matrix[11]);
        skinned += transformed * weight;
    }
    return skinned;
}

bool create_vulkan_buffer_resource(
    LLVulkanNativeContext& context,
    U64 size,
    U32 usage,
    const void* data,
    LLVulkanBufferResource& resource,
    U64 replaced_memory_size = 0,
    bool allow_reserved_budget = false);

void destroy_vulkan_buffer_resource(
    LLVulkanNativeContext& context,
    LLVulkanBufferResource& resource)
{
    if (resource.mMappedData && context.mUnmapMemory && context.mDevice && resource.mMemory)
    {
        context.mUnmapMemory(context.mDevice, resource.mMemory);
    }

    if (resource.mBuffer && context.mDestroyBuffer && context.mDevice)
    {
        context.mDestroyBuffer(context.mDevice, resource.mBuffer, nullptr);
    }

    if (resource.mMemory && context.mFreeMemory && context.mDevice)
    {
        context.mFreeMemory(context.mDevice, resource.mMemory, nullptr);
    }

    if (resource.mMemoryAccounted)
    {
        context.mBufferMemoryAllocatedBytes =
            resource.mMemorySize <= context.mBufferMemoryAllocatedBytes ?
            context.mBufferMemoryAllocatedBytes - resource.mMemorySize :
            0;
    }

    resource = {};
}

void destroy_vulkan_transient_frame_buffers(LLVulkanNativeContext& context)
{
    for (LLVulkanBufferResource& resource : context.mTransientFrameBuffers)
    {
        destroy_vulkan_buffer_resource(context, resource);
    }
    context.mTransientFrameBuffers.clear();
    context.mTransientFrameBufferBytes = 0;
}

void destroy_all_vulkan_buffer_resources(LLVulkanNativeContext& context)
{
    destroy_vulkan_transient_frame_buffers(context);
    destroy_vulkan_buffer_resource(context, context.mDefaultTexCoordBuffer);
    destroy_vulkan_buffer_resource(context, context.mDefaultColorBuffer);
    destroy_vulkan_buffer_resource(context, context.mDefaultNormalBuffer);
    destroy_vulkan_buffer_resource(context, context.mDefaultTangentBuffer);
    destroy_vulkan_buffer_resource(context, context.mSkinningMatrixPaletteBuffer);

    for (auto& entry : gVulkanBuffers)
    {
        destroy_vulkan_buffer_resource(context, entry.second);
    }

    gVulkanBuffers.clear();
    gPendingVulkanBufferAllocations.clear();
    gPendingVulkanDraws.clear();
    gCurrentVulkanVertexAttributes = {};
    gCurrentVulkanMaterialParameters = {};
    gCurrentVulkanSkinningMatrixPalette.clear();
    gCurrentVulkanSkinningMatrixCount = 0;
    gBoundVulkanVertexBuffer = 0;
    gBoundVulkanIndexBuffer = 0;
    gCurrentVulkanViewport = {};
    gCurrentVulkanScissor = {};
    gBoundVulkanReadFramebuffer = 0;
    gBoundVulkanDrawFramebuffer = 0;
    gVulkanFramebufferColorAttachmentCount = 1;
}

bool create_vulkan_buffer_resource(
    LLVulkanNativeContext& context,
    U64 size,
    U32 usage,
    const void* data,
    LLVulkanBufferResource& resource,
    U64 replaced_memory_size,
    bool allow_reserved_budget);

bool create_vulkan_default_ui_attribute_buffers(LLVulkanNativeContext& context)
{
    std::vector<F32> default_texcoords(MARE_VULKAN_DEFAULT_UI_ATTRIBUTE_VERTICES * 2, 0.f);
    std::vector<U8> default_colors(MARE_VULKAN_DEFAULT_UI_ATTRIBUTE_VERTICES * 4, 255);
    std::vector<F32> default_normals(MARE_VULKAN_DEFAULT_UI_ATTRIBUTE_VERTICES * 4, 0.f);
    std::vector<F32> default_tangents(MARE_VULKAN_DEFAULT_UI_ATTRIBUTE_VERTICES * 4, 0.f);
    std::array<F32, 12> default_skinning_palette = {};

    for (U32 i = 0; i < MARE_VULKAN_DEFAULT_UI_ATTRIBUTE_VERTICES; ++i)
    {
        default_normals[(i * 4) + 2] = 1.f;
        default_tangents[(i * 4) + 0] = 1.f;
        default_tangents[(i * 4) + 3] = 1.f;
    }

    if (!create_vulkan_buffer_resource(
            context,
            default_texcoords.size() * sizeof(F32),
            LL_VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            default_texcoords.data(),
            context.mDefaultTexCoordBuffer,
            0,
            true))
    {
        return false;
    }

    if (!create_vulkan_buffer_resource(
            context,
            default_colors.size() * sizeof(U8),
            LL_VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            default_colors.data(),
            context.mDefaultColorBuffer,
            0,
            true))
    {
        destroy_vulkan_buffer_resource(context, context.mDefaultTexCoordBuffer);
        return false;
    }

    if (!create_vulkan_buffer_resource(
            context,
            default_normals.size() * sizeof(F32),
            LL_VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            default_normals.data(),
            context.mDefaultNormalBuffer,
            0,
            true))
    {
        destroy_vulkan_buffer_resource(context, context.mDefaultTexCoordBuffer);
        destroy_vulkan_buffer_resource(context, context.mDefaultColorBuffer);
        return false;
    }

    if (!create_vulkan_buffer_resource(
            context,
            default_tangents.size() * sizeof(F32),
            LL_VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            default_tangents.data(),
            context.mDefaultTangentBuffer,
            0,
            true))
    {
        destroy_vulkan_buffer_resource(context, context.mDefaultTexCoordBuffer);
        destroy_vulkan_buffer_resource(context, context.mDefaultColorBuffer);
        destroy_vulkan_buffer_resource(context, context.mDefaultNormalBuffer);
        return false;
    }

    if (!create_vulkan_buffer_resource(
            context,
            default_skinning_palette.size() * sizeof(F32),
            LL_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            default_skinning_palette.data(),
            context.mSkinningMatrixPaletteBuffer,
            0,
            true))
    {
        destroy_vulkan_buffer_resource(context, context.mDefaultTexCoordBuffer);
        destroy_vulkan_buffer_resource(context, context.mDefaultColorBuffer);
        destroy_vulkan_buffer_resource(context, context.mDefaultNormalBuffer);
        destroy_vulkan_buffer_resource(context, context.mDefaultTangentBuffer);
        return false;
    }

    LL_INFOS("RenderBackend")
        << "Vulkan UI default attribute buffers created for "
        << MARE_VULKAN_DEFAULT_UI_ATTRIBUTE_VERTICES
        << " vertices."
        << LL_ENDL;
    return true;
}

U64 get_vulkan_buffer_memory_budget_bytes()
{
    static const U64 budget_bytes = []()
    {
        U64 override_bytes = 0;
        if (get_vulkan_memory_budget_override_bytes(
                "MARE_VULKAN_BUFFER_MEMORY_BUDGET_MB",
                override_bytes))
        {
            return override_bytes;
        }
        return MARE_VULKAN_DEFAULT_BUFFER_MEMORY_BUDGET_MB * MARE_VULKAN_BYTES_PER_MEGABYTE;
    }();

    return budget_bytes;
}

U64 get_vulkan_buffer_memory_budget_bytes(
    LLVulkanNativeContext& context,
    U32 memory_type_index)
{
    U64 override_bytes = 0;
    if (get_vulkan_memory_budget_override_bytes(
            "MARE_VULKAN_BUFFER_MEMORY_BUDGET_MB",
            override_bytes))
    {
        return override_bytes;
    }

    return get_vulkan_derived_memory_budget_bytes(
        context,
        memory_type_index,
        get_vulkan_buffer_memory_budget_bytes(),
        8,
        1,
        4);
}

U64 get_vulkan_buffer_memory_reserve_bytes()
{
    static const U64 reserve_bytes = []()
    {
        U64 reserve_mb = MARE_VULKAN_DEFAULT_BUFFER_MEMORY_RESERVE_MB;
        if (const char* reserve_override = std::getenv("MARE_VULKAN_BUFFER_MEMORY_RESERVE_MB"))
        {
            reserve_mb = std::strtoull(reserve_override, nullptr, 10);
        }
        return reserve_mb * MARE_VULKAN_BYTES_PER_MEGABYTE;
    }();

    return reserve_bytes;
}

U64 get_vulkan_skinned_position_frame_budget_bytes()
{
    static const U64 budget_bytes = []()
    {
        U64 budget_mb = MARE_VULKAN_DEFAULT_SKINNED_POSITION_FRAME_BUDGET_MB;
        if (const char* budget_override = std::getenv("MARE_VULKAN_SKINNED_POSITION_FRAME_BUDGET_MB"))
        {
            const U64 parsed_budget = std::strtoull(budget_override, nullptr, 10);
            if (parsed_budget > 0)
            {
                budget_mb = parsed_budget;
            }
        }
        return budget_mb * MARE_VULKAN_BYTES_PER_MEGABYTE;
    }();

    return budget_bytes;
}

bool can_use_vulkan_reserved_buffer_memory(LLRenderBufferUsage usage)
{
    return usage == LLRenderBufferUsage::DynamicDraw ||
           usage == LLRenderBufferUsage::StreamDraw ||
           usage == LLRenderBufferUsage::StreamCopy;
}

bool can_commit_vulkan_buffer_memory(
    LLVulkanNativeContext& context,
    U64 size,
    U32 usage,
    U32 memory_type_index,
    U64 replaced_memory_size = 0,
    bool allow_reserved_budget = false)
{
    const U64 total_budget = get_vulkan_buffer_memory_budget_bytes(context, memory_type_index);
    context.mEffectiveBufferMemoryBudgetBytes = total_budget;
    const U64 reserve = llmin(get_vulkan_buffer_memory_reserve_bytes(), total_budget);
    const U64 budget = allow_reserved_budget ? total_budget : total_budget - reserve;
    const U64 current = context.mBufferMemoryAllocatedBytes;
    const U64 current_without_replaced =
        current >= replaced_memory_size ?
        current - replaced_memory_size :
        0;
    if (current_without_replaced + size > budget)
    {
        ++context.mSkippedBufferMemoryBudgetCount;
        if (!context.mLoggedBufferMemoryBudgetExceeded)
        {
            LL_WARNS("RenderBackend")
                << "Vulkan buffer allocation refused because local buffer memory budget would be exceeded. "
                << "Current "
                << (current / MARE_VULKAN_BYTES_PER_MEGABYTE)
                << "MB, requested "
                << (size / MARE_VULKAN_BYTES_PER_MEGABYTE)
                << "MB, budget "
                << (budget / MARE_VULKAN_BYTES_PER_MEGABYTE)
                << "MB, reserve "
                << (reserve / MARE_VULKAN_BYTES_PER_MEGABYTE)
                << "MB, reserved access "
                << allow_reserved_budget
                << ", usage flags "
                << usage
                << ". Override with MARE_VULKAN_BUFFER_MEMORY_BUDGET_MB for testing."
                << LL_ENDL;
            context.mLoggedBufferMemoryBudgetExceeded = true;
        }
        return false;
    }

    if (!can_commit_vulkan_heap_memory(
            context,
            memory_type_index,
            size,
            "buffer"))
    {
        ++context.mSkippedBufferMemoryBudgetCount;
        return false;
    }

    return true;
}

bool create_vulkan_buffer_resource(
    LLVulkanNativeContext& context,
    U64 size,
    U32 usage,
    const void* data,
    LLVulkanBufferResource& resource,
    U64 replaced_memory_size,
    bool allow_reserved_budget)
{
    if (!context.mCreateBuffer ||
        !context.mGetBufferMemoryRequirements ||
        !context.mAllocateMemory ||
        !context.mBindBufferMemory ||
        !context.mMapMemory)
    {
        return false;
    }

    LLVkBufferCreateInfo buffer_create_info =
    {
        LL_VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr,
        0,
        size,
        usage,
        LL_VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr
    };

    S32 result = context.mCreateBuffer(
        context.mDevice,
        &buffer_create_info,
        nullptr,
        &resource.mBuffer);
    if (result != LL_VK_SUCCESS || !resource.mBuffer)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateBuffer failed with result "
            << result
            << LL_ENDL;
        resource = {};
        return false;
    }

    LLVkMemoryRequirements memory_requirements = {};
    context.mGetBufferMemoryRequirements(
        context.mDevice,
        resource.mBuffer,
        &memory_requirements);

    U32 memory_type_index = 0;
    if (!find_vulkan_memory_type(
            context,
            memory_requirements.memoryTypeBits,
            LL_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | LL_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            memory_type_index))
    {
        LL_WARNS("RenderBackend")
            << "No host-visible coherent Vulkan memory type for buffer."
            << LL_ENDL;
        destroy_vulkan_buffer_resource(context, resource);
        return false;
    }

    if (!can_commit_vulkan_buffer_memory(
            context,
            memory_requirements.size,
            usage,
            memory_type_index,
            replaced_memory_size,
            allow_reserved_budget))
    {
        destroy_vulkan_buffer_resource(context, resource);
        return false;
    }

    LLVkMemoryAllocateInfo allocate_info =
    {
        LL_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memory_requirements.size,
        memory_type_index
    };

    result = context.mAllocateMemory(
        context.mDevice,
        &allocate_info,
        nullptr,
        &resource.mMemory);
    if (result != LL_VK_SUCCESS || !resource.mMemory)
    {
        LL_WARNS("RenderBackend")
            << "vkAllocateMemory(buffer) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_buffer_resource(context, resource);
        return false;
    }

    result = context.mBindBufferMemory(
        context.mDevice,
        resource.mBuffer,
        resource.mMemory,
        0);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkBindBufferMemory failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_buffer_resource(context, resource);
        return false;
    }

    result = context.mMapMemory(
        context.mDevice,
        resource.mMemory,
        0,
        memory_requirements.size,
        0,
        &resource.mMappedData);
    if (result != LL_VK_SUCCESS || !resource.mMappedData)
    {
        LL_WARNS("RenderBackend")
            << "vkMapMemory(buffer) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_buffer_resource(context, resource);
        return false;
    }

    resource.mSize = size;
    resource.mMemorySize = memory_requirements.size;
    resource.mMemoryAccounted = true;
    context.mBufferMemoryAllocatedBytes += resource.mMemorySize;
    if (data && size > 0)
    {
        std::memcpy(resource.mMappedData, data, static_cast<size_t>(size));
    }
    return true;
}

U64 get_vulkan_skinning_palette_buffer_size(U64 required_size)
{
    const U64 minimum_size = 12 * sizeof(F32);
    if (required_size <= minimum_size)
    {
        return minimum_size;
    }

    const U64 growth = MARE_VULKAN_SKINNING_PALETTE_BUFFER_GROWTH_BYTES;
    return ((required_size + growth - 1) / growth) * growth;
}

bool ensure_vulkan_skinning_matrix_palette_buffer(
    LLVulkanNativeContext& context,
    U64 required_size)
{
    const U64 buffer_size = get_vulkan_skinning_palette_buffer_size(required_size);
    if (context.mSkinningMatrixPaletteBuffer.mBuffer &&
        context.mSkinningMatrixPaletteBuffer.mMappedData &&
        context.mSkinningMatrixPaletteBuffer.mSize >= buffer_size)
    {
        return true;
    }

    LLVulkanBufferResource new_resource;
    const U64 replaced_memory_size =
        context.mSkinningMatrixPaletteBuffer.mMemoryAccounted ?
        context.mSkinningMatrixPaletteBuffer.mMemorySize :
        0;
    if (!create_vulkan_buffer_resource(
            context,
            buffer_size,
            LL_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            nullptr,
            new_resource,
            replaced_memory_size,
            true))
    {
        if (!context.mLoggedSkinningPaletteBufferAllocationFailed)
        {
            LL_WARNS("RenderBackend")
                << "Unable to allocate Vulkan skinning matrix palette buffer of "
                << (buffer_size / MARE_VULKAN_BYTES_PER_MEGABYTE)
                << "MB. Rigged world draws will fall back to the CPU skinning bridge."
                << LL_ENDL;
            context.mLoggedSkinningPaletteBufferAllocationFailed = true;
        }
        return false;
    }

    destroy_vulkan_texture_descriptor_set_cache(context);
    destroy_vulkan_buffer_resource(context, context.mSkinningMatrixPaletteBuffer);
    context.mSkinningMatrixPaletteBuffer = new_resource;
    return true;
}

bool prepare_vulkan_skinning_matrix_palette_buffer(LLVulkanNativeContext& context)
{
    std::vector<F32> palette_data;
    for (LLVulkanPendingDraw& draw : gPendingVulkanDraws)
    {
        draw.mSkinningMatrixOffset = 0;
        if (draw.mSkinningMatrixCount == 0)
        {
            continue;
        }

        const U32 matrix_count =
            llmin(draw.mSkinningMatrixCount, MARE_VULKAN_MAX_SKINNING_MATRICES);
        const size_t float_count = static_cast<size_t>(matrix_count) * 12;
        if (draw.mSkinningMatrixPalette.size() < float_count)
        {
            draw.mSkinningMatrixCount = 0;
            continue;
        }

        draw.mSkinningMatrixOffset = static_cast<U32>(palette_data.size() / 12);
        palette_data.insert(
            palette_data.end(),
            draw.mSkinningMatrixPalette.begin(),
            draw.mSkinningMatrixPalette.begin() + float_count);
        draw.mSkinningMatrixCount = matrix_count;
    }

    if (palette_data.empty())
    {
        return true;
    }

    const U64 required_size = palette_data.size() * sizeof(F32);
    if (!ensure_vulkan_skinning_matrix_palette_buffer(context, required_size) ||
        !context.mSkinningMatrixPaletteBuffer.mMappedData ||
        context.mSkinningMatrixPaletteBuffer.mSize < required_size)
    {
        return false;
    }

    std::memcpy(
        context.mSkinningMatrixPaletteBuffer.mMappedData,
        palette_data.data(),
        static_cast<size_t>(required_size));
    return true;
}

void track_vulkan_pending_buffer_allocation(
    U32 handle,
    U64 size,
    U32 usage_flags,
    LLRenderBufferUsage usage)
{
    if (!handle || size == 0)
    {
        return;
    }

    gPendingVulkanBufferAllocations[handle] =
    {
        size,
        usage_flags,
        usage
    };
}

bool retry_vulkan_pending_buffer_allocation(
    LLVulkanNativeContext& context,
    U32 handle,
    U64 minimum_size)
{
    auto pending_iter = gPendingVulkanBufferAllocations.find(handle);
    if (pending_iter == gPendingVulkanBufferAllocations.end())
    {
        return false;
    }

    LLVulkanPendingBufferAllocation pending = pending_iter->second;
    pending.mSize = llmax(pending.mSize, minimum_size);

    auto resource_iter = gVulkanBuffers.find(handle);
    const U64 replaced_memory_size =
        resource_iter != gVulkanBuffers.end() && resource_iter->second.mMemoryAccounted ?
        resource_iter->second.mMemorySize :
        0;

    LLVulkanBufferResource new_resource;
    if (!create_vulkan_buffer_resource(
            context,
            pending.mSize,
            pending.mUsageFlags,
            nullptr,
            new_resource,
            replaced_memory_size,
            can_use_vulkan_reserved_buffer_memory(pending.mUsage)))
    {
        pending_iter->second = pending;
        return false;
    }

    if (new_resource.mMappedData && new_resource.mSize > 0)
    {
        std::memset(new_resource.mMappedData, 0, static_cast<size_t>(new_resource.mSize));
    }

    if (resource_iter != gVulkanBuffers.end())
    {
        destroy_vulkan_buffer_resource(context, resource_iter->second);
        resource_iter->second = new_resource;
    }
    else
    {
        gVulkanBuffers.emplace(handle, new_resource);
    }
    gPendingVulkanBufferAllocations.erase(pending_iter);
    return true;
}

const LLVulkanBufferResource* create_vulkan_skinned_position_buffer(
    LLVulkanNativeContext& context,
    const LLVulkanPendingDraw& draw,
    const LLVulkanBufferResource& vertex_buffer,
    const LLVulkanBufferResource* index_buffer)
{
    if (draw.mSkinningMatrixCount == 0 ||
        draw.mSkinningMatrixPalette.size() < static_cast<size_t>(draw.mSkinningMatrixCount) * 12 ||
        !draw.mAttributes[0].mEnabled ||
        !draw.mAttributes[10].mEnabled)
    {
        return nullptr;
    }

    U32 max_vertex_index = 0;
    if (!find_vulkan_draw_max_vertex_index(draw, index_buffer, max_vertex_index))
    {
        return nullptr;
    }

    std::vector<glm::vec4> skinned_positions(static_cast<size_t>(max_vertex_index) + 1);
    for (U32 vertex_index = 0; vertex_index <= max_vertex_index; ++vertex_index)
    {
        glm::vec3 position;
        glm::vec4 weights;
        if (!read_vulkan_vertex_vec3(
                vertex_buffer,
                draw.mAttributes[0],
                vertex_index,
                position) ||
            !read_vulkan_vertex_weight4(
                vertex_buffer,
                draw.mAttributes[10],
                vertex_index,
                weights))
        {
            return nullptr;
        }

        const glm::vec3 skinned = skin_vulkan_position(position, weights, draw);
        skinned_positions[vertex_index] = glm::vec4(skinned, 1.f);
    }

    const U64 skinned_position_size = skinned_positions.size() * sizeof(glm::vec4);
    const U64 transient_budget = get_vulkan_skinned_position_frame_budget_bytes();
    if (transient_budget > 0 &&
        context.mTransientFrameBufferBytes + skinned_position_size > transient_budget)
    {
        if (!context.mLoggedTransientFrameBufferBudgetExceeded)
        {
            LL_WARNS("RenderBackend")
                << "Vulkan skinned position buffer skipped because transient frame buffer budget would be exceeded. "
                << "Current "
                << (context.mTransientFrameBufferBytes / MARE_VULKAN_BYTES_PER_MEGABYTE)
                << "MB, requested "
                << (skinned_position_size / MARE_VULKAN_BYTES_PER_MEGABYTE)
                << "MB, budget "
                << (transient_budget / MARE_VULKAN_BYTES_PER_MEGABYTE)
                << "MB. Override with MARE_VULKAN_SKINNED_POSITION_FRAME_BUDGET_MB for testing."
                << LL_ENDL;
            context.mLoggedTransientFrameBufferBudgetExceeded = true;
        }
        return nullptr;
    }

    LLVulkanBufferResource skinned_buffer;
    if (!create_vulkan_buffer_resource(
            context,
            skinned_position_size,
            LL_VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            skinned_positions.data(),
            skinned_buffer,
            0,
            false))
    {
        return nullptr;
    }

    context.mTransientFrameBufferBytes += skinned_buffer.mMemorySize;
    context.mTransientFrameBuffers.push_back(skinned_buffer);
    return &context.mTransientFrameBuffers.back();
}

U64 get_vulkan_texture_memory_budget_bytes()
{
    static const U64 budget_bytes = []()
    {
        U64 override_bytes = 0;
        if (get_vulkan_memory_budget_override_bytes(
                "MARE_VULKAN_TEXTURE_MEMORY_BUDGET_MB",
                override_bytes))
        {
            return override_bytes;
        }
        return MARE_VULKAN_DEFAULT_TEXTURE_MEMORY_BUDGET_MB * MARE_VULKAN_BYTES_PER_MEGABYTE;
    }();

    return budget_bytes;
}

U64 get_vulkan_texture_memory_budget_bytes(
    LLVulkanNativeContext& context,
    U32 memory_type_index)
{
    U64 override_bytes = 0;
    if (get_vulkan_memory_budget_override_bytes(
            "MARE_VULKAN_TEXTURE_MEMORY_BUDGET_MB",
            override_bytes))
    {
        return override_bytes;
    }

    return get_vulkan_derived_memory_budget_bytes(
        context,
        memory_type_index,
        get_vulkan_texture_memory_budget_bytes(),
        4,
        3,
        5);
}

U64 get_vulkan_texture_upload_frame_budget_bytes()
{
    static const U64 budget_bytes = []()
    {
        U64 budget_mb = MARE_VULKAN_DEFAULT_TEXTURE_UPLOAD_FRAME_BUDGET_MB;
        if (const char* budget_override = std::getenv("MARE_VULKAN_TEXTURE_UPLOAD_FRAME_BUDGET_MB"))
        {
            const U64 parsed_budget = std::strtoull(budget_override, nullptr, 10);
            if (parsed_budget > 0)
            {
                budget_mb = parsed_budget;
            }
        }
        return budget_mb * MARE_VULKAN_BYTES_PER_MEGABYTE;
    }();

    return budget_bytes;
}

U32 get_vulkan_texture_upload_frame_count_budget()
{
    static const U32 count_budget = []()
    {
        U64 budget = MARE_VULKAN_DEFAULT_TEXTURE_UPLOAD_FRAME_COUNT_BUDGET;
        if (const char* budget_override = std::getenv("MARE_VULKAN_TEXTURE_UPLOAD_FRAME_COUNT_BUDGET"))
        {
            const U64 parsed_budget = std::strtoull(budget_override, nullptr, 10);
            if (parsed_budget > 0)
            {
                budget = parsed_budget;
            }
        }
        return static_cast<U32>(std::min<U64>(budget, std::numeric_limits<U32>::max()));
    }();

    return count_budget;
}

U64 get_vulkan_texture_eviction_min_age_frames()
{
    static const U64 min_age_frames = []()
    {
        U64 frames = 180;
        if (const char* age_override = std::getenv("MARE_VULKAN_TEXTURE_EVICTION_MIN_AGE_FRAMES"))
        {
            frames = std::strtoull(age_override, nullptr, 10);
        }
        return frames;
    }();

    return min_age_frames;
}

S32 get_vulkan_max_texture_dimension()
{
    static const S32 max_dimension = []()
    {
        U64 dimension = MARE_VULKAN_DEFAULT_MAX_TEXTURE_DIMENSION;
        if (const char* dimension_override = std::getenv("MARE_VULKAN_TEXTURE_MAX_DIMENSION"))
        {
            const U64 parsed_dimension = std::strtoull(dimension_override, nullptr, 10);
            if (parsed_dimension > 0)
            {
                dimension = parsed_dimension;
            }
        }
        return static_cast<S32>(std::min<U64>(dimension, std::numeric_limits<S32>::max()));
    }();

    return max_dimension;
}

void reset_vulkan_texture_upload_frame_budget(LLVulkanNativeContext& context)
{
    if (context.mTextureUploadBudgetFrame == context.mPresentedFrameCount)
    {
        return;
    }

    context.mTextureUploadBudgetFrame = context.mPresentedFrameCount;
    context.mTextureUploadBytesThisFrame = 0;
    context.mTextureUploadsThisFrame = 0;
}

bool can_accept_vulkan_texture_upload_request(
    LLVulkanNativeContext& context,
    U32 handle,
    S32 width,
    S32 height,
    U64 upload_bytes,
    bool check_dimensions,
    bool count_against_frame_count = true,
    bool count_against_frame_bytes = true)
{
    if (check_dimensions)
    {
        const S32 max_dimension = get_vulkan_max_texture_dimension();
        if (width > max_dimension || height > max_dimension)
        {
            ++context.mSkippedTextureOversizeCount;
            if (!context.mLoggedTextureOversize)
            {
                LL_WARNS("RenderBackend")
                    << "Vulkan texture upload refused because texture "
                    << handle
                    << " is "
                    << width
                    << "x"
                    << height
                    << ", above max dimension "
                    << max_dimension
                    << ". Override with MARE_VULKAN_TEXTURE_MAX_DIMENSION for testing."
                    << LL_ENDL;
                context.mLoggedTextureOversize = true;
            }
            return false;
        }
    }

    if (!count_against_frame_count && !count_against_frame_bytes)
    {
        return true;
    }

    reset_vulkan_texture_upload_frame_budget(context);

    const U64 frame_byte_budget = get_vulkan_texture_upload_frame_budget_bytes();
    const U32 frame_count_budget = get_vulkan_texture_upload_frame_count_budget();
    const bool too_many_uploads =
        count_against_frame_count &&
        frame_count_budget > 0 &&
        context.mTextureUploadsThisFrame >= frame_count_budget;
    const bool too_many_bytes =
        count_against_frame_bytes &&
        frame_byte_budget > 0 &&
        context.mTextureUploadBytesThisFrame + upload_bytes > frame_byte_budget;

    if (too_many_uploads || too_many_bytes)
    {
        ++context.mSkippedTextureUploadThrottleCount;
        context.mLastTextureUploadDeferred = true;
        if (!context.mLoggedTextureUploadThrottle)
        {
            LL_WARNS("RenderBackend")
                << "Vulkan texture upload throttled. Frame "
                << context.mPresentedFrameCount
                << " already uploaded "
                << context.mTextureUploadsThisFrame
                << " texture(s), "
                << (context.mTextureUploadBytesThisFrame / MARE_VULKAN_BYTES_PER_MEGABYTE)
                << "MB; requested upload is "
                << (upload_bytes / MARE_VULKAN_BYTES_PER_MEGABYTE)
                << "MB. Per-frame limits are "
                << frame_count_budget
                << " texture(s) and "
                << (frame_byte_budget / MARE_VULKAN_BYTES_PER_MEGABYTE)
                << "MB. Override with MARE_VULKAN_TEXTURE_UPLOAD_FRAME_* for testing."
                << LL_ENDL;
            context.mLoggedTextureUploadThrottle = true;
        }
        return false;
    }

    if (count_against_frame_count)
    {
        ++context.mTextureUploadsThisFrame;
    }
    if (count_against_frame_bytes)
    {
        context.mTextureUploadBytesThisFrame += upload_bytes;
    }
    return true;
}

bool can_commit_vulkan_texture_memory(
    LLVulkanNativeContext& context,
    U64 old_size,
    U64 new_size,
    U32 memory_type_index,
    U32 handle,
    S32 width,
    S32 height)
{
    const U64 budget = get_vulkan_texture_memory_budget_bytes(context, memory_type_index);
    context.mEffectiveTextureMemoryBudgetBytes = budget;
    const U64 current = context.mTextureMemoryAllocatedBytes;
    const U64 current_without_old = current >= old_size ? current - old_size : 0;
    const U64 requested_total = current_without_old + new_size;

    if (!context.mLoggedTextureMemoryBudget)
    {
        LL_INFOS("RenderBackend")
            << "Vulkan local texture memory budget is "
            << format_vulkan_megabytes(budget)
            << ". Override with MARE_VULKAN_TEXTURE_MEMORY_BUDGET_MB for testing."
            << LL_ENDL;
        context.mLoggedTextureMemoryBudget = true;
    }

    if (requested_total > budget)
    {
        ++context.mSkippedTextureMemoryBudgetCount;
        if (!context.mLoggedTextureMemoryBudgetExceeded)
        {
            LL_WARNS("RenderBackend")
                << "Vulkan texture upload refused by local texture memory budget. Handle "
                << handle
                << " size "
                << width
                << "x"
                << height
                << " requires "
                << format_vulkan_megabytes(new_size)
                << ", current texture memory is "
                << format_vulkan_megabytes(current)
                << ", requested total would be "
                << format_vulkan_megabytes(requested_total)
                << ", budget is "
                << format_vulkan_megabytes(budget)
                << "."
                << LL_ENDL;
            context.mLoggedTextureMemoryBudgetExceeded = true;
        }
        return false;
    }

    if (!can_commit_vulkan_heap_memory(
            context,
            memory_type_index,
            new_size,
            "texture"))
    {
        ++context.mSkippedTextureMemoryBudgetCount;
        return false;
    }

    return true;
}

void destroy_vulkan_texture_resource(
    LLVulkanNativeContext& context,
    LLVulkanTextureResource& resource)
{
    destroy_vulkan_texture_descriptor_set_cache(context);

    if (resource.mDescriptorSet &&
        context.mFreeDescriptorSets &&
        context.mDevice &&
        context.mUIDescriptorPool)
    {
        const S32 result = context.mFreeDescriptorSets(
            context.mDevice,
            context.mUIDescriptorPool,
            1,
            &resource.mDescriptorSet);
        if (result != LL_VK_SUCCESS)
        {
            LL_WARNS("RenderBackend")
                << "vkFreeDescriptorSets(texture) failed with result "
                << result
                << LL_ENDL;
        }
    }
    resource.mDescriptorSet = nullptr;

    if (resource.mSampler && context.mDestroySampler && context.mDevice)
    {
        context.mDestroySampler(context.mDevice, resource.mSampler, nullptr);
    }

    if (resource.mImageView && context.mDestroyImageView && context.mDevice)
    {
        context.mDestroyImageView(context.mDevice, resource.mImageView, nullptr);
    }

    if (resource.mImage && context.mDestroyImage && context.mDevice)
    {
        context.mDestroyImage(context.mDevice, resource.mImage, nullptr);
    }

    if (resource.mMemory && context.mFreeMemory && context.mDevice)
    {
        context.mFreeMemory(context.mDevice, resource.mMemory, nullptr);
    }

    if (resource.mMemoryAccounted && resource.mMemorySize > 0)
    {
        if (context.mTextureMemoryAllocatedBytes >= resource.mMemorySize)
        {
            context.mTextureMemoryAllocatedBytes -= resource.mMemorySize;
        }
        else
        {
            context.mTextureMemoryAllocatedBytes = 0;
        }
    }

    resource = {};
}

bool evict_vulkan_texture_resources_for_upload(
    LLVulkanNativeContext& context,
    U32 protected_handle,
    U64 old_size,
    U64 new_size,
    U64 budget)
{
    const U64 current_without_old =
        context.mTextureMemoryAllocatedBytes >= old_size ?
        context.mTextureMemoryAllocatedBytes - old_size :
        0;
    if (current_without_old + new_size <= budget)
    {
        return true;
    }

    const U64 min_age_frames = get_vulkan_texture_eviction_min_age_frames();
    bool evicted = false;
    while (context.mTextureMemoryAllocatedBytes >= old_size &&
           context.mTextureMemoryAllocatedBytes - old_size + new_size > budget)
    {
        auto candidate = gVulkanTextures.end();
        U64 candidate_age = 0;
        U64 candidate_size = 0;

        for (auto iter = gVulkanTextures.begin(); iter != gVulkanTextures.end(); ++iter)
        {
            const U32 handle = iter->first;
            const LLVulkanTextureResource& resource = iter->second;
            if (handle == 0 ||
                handle == protected_handle ||
                !resource.mMemoryAccounted ||
                resource.mMemorySize == 0)
            {
                continue;
            }

            const U64 last_bound_frame = resource.mLastBoundFrame;
            const U64 age =
                context.mPresentedFrameCount >= last_bound_frame ?
                context.mPresentedFrameCount - last_bound_frame :
                0;
            if (age < min_age_frames)
            {
                continue;
            }

            if (candidate == gVulkanTextures.end() ||
                age > candidate_age ||
                (age == candidate_age && resource.mMemorySize > candidate_size))
            {
                candidate = iter;
                candidate_age = age;
                candidate_size = resource.mMemorySize;
            }
        }

        if (candidate == gVulkanTextures.end())
        {
            break;
        }

        const U32 evicted_handle = candidate->first;
        const U64 evicted_size = candidate->second.mMemorySize;
        destroy_vulkan_texture_resource(context, candidate->second);
        gVulkanTextures.erase(candidate);
        ++context.mEvictedTextureCount;
        context.mEvictedTextureMemoryBytes += evicted_size;
        evicted = true;

        if (!context.mLoggedTextureEviction)
        {
            LL_WARNS("RenderBackend")
                << "Evicted Vulkan texture "
                << evicted_handle
                << " after "
                << candidate_age
                << " unbound frame(s), freeing "
                << format_vulkan_megabytes(evicted_size)
                << " for a new upload. Override age with "
                << "MARE_VULKAN_TEXTURE_EVICTION_MIN_AGE_FRAMES."
                << LL_ENDL;
            context.mLoggedTextureEviction = true;
        }
    }

    return evicted &&
        context.mTextureMemoryAllocatedBytes >= old_size &&
        context.mTextureMemoryAllocatedBytes - old_size + new_size <= budget;
}

void destroy_all_vulkan_texture_resources(LLVulkanNativeContext& context)
{
    destroy_all_vulkan_framebuffer_resources(context);
    destroy_vulkan_texture_descriptor_set_cache(context);

    for (auto& entry : gVulkanTextures)
    {
        destroy_vulkan_texture_resource(context, entry.second);
    }

    gVulkanTextures.clear();
    gVulkanTextureSamplerStates.clear();
    gVulkanFramebuffers.clear();
    gBoundVulkanTextures = {};
}

void destroy_vulkan_depth_attachment(LLVulkanNativeContext& context)
{
    LLVulkanDepthAttachment& depth = context.mDepthAttachment;
    if (depth.mImageView && context.mDestroyImageView && context.mDevice)
    {
        context.mDestroyImageView(context.mDevice, depth.mImageView, nullptr);
    }
    if (depth.mImage && context.mDestroyImage && context.mDevice)
    {
        context.mDestroyImage(context.mDevice, depth.mImage, nullptr);
    }
    if (depth.mMemory && context.mFreeMemory && context.mDevice)
    {
        context.mFreeMemory(context.mDevice, depth.mMemory, nullptr);
    }
    depth = {};
    depth.mFormat = LL_VK_FORMAT_D32_SFLOAT;
}

bool ensure_vulkan_image_resource_entry_points(LLVulkanNativeContext& context)
{
    if (!context.mCreateImage)
    {
        context.mCreateImage =
            reinterpret_cast<LLVulkanCreateImage>(
                get_vulkan_device_proc_address(context, "vkCreateImage"));
    }
    if (!context.mDestroyImage)
    {
        context.mDestroyImage =
            reinterpret_cast<LLVulkanDestroyImage>(
                get_vulkan_device_proc_address(context, "vkDestroyImage"));
    }
    if (!context.mGetImageMemoryRequirements)
    {
        context.mGetImageMemoryRequirements =
            reinterpret_cast<LLVulkanGetImageMemoryRequirements>(
                get_vulkan_device_proc_address(context, "vkGetImageMemoryRequirements"));
    }
    if (!context.mBindImageMemory)
    {
        context.mBindImageMemory =
            reinterpret_cast<LLVulkanBindImageMemory>(
                get_vulkan_device_proc_address(context, "vkBindImageMemory"));
    }

    return context.mCreateImage &&
        context.mDestroyImage &&
        context.mGetImageMemoryRequirements &&
        context.mBindImageMemory &&
        context.mCreateImageView &&
        context.mDestroyImageView &&
        context.mAllocateMemory &&
        context.mFreeMemory &&
        context.mGetPhysicalDeviceMemoryProperties;
}

bool ensure_vulkan_texture_entry_points(LLVulkanNativeContext& context)
{
    if (!ensure_vulkan_image_resource_entry_points(context))
    {
        return false;
    }

    if (!context.mCreateSampler)
    {
        context.mCreateSampler =
            reinterpret_cast<LLVulkanCreateSampler>(
                get_vulkan_device_proc_address(context, "vkCreateSampler"));
    }
    if (!context.mDestroySampler)
    {
        context.mDestroySampler =
            reinterpret_cast<LLVulkanDestroySampler>(
                get_vulkan_device_proc_address(context, "vkDestroySampler"));
    }
    if (!context.mCmdCopyBufferToImage)
    {
        context.mCmdCopyBufferToImage =
            reinterpret_cast<LLVulkanCmdCopyBufferToImage>(
                get_vulkan_device_proc_address(context, "vkCmdCopyBufferToImage"));
    }

    return context.mCreateSampler &&
        context.mDestroySampler &&
        context.mCmdCopyBufferToImage &&
        context.mAllocateDescriptorSets &&
        context.mUpdateDescriptorSets &&
        context.mUIDescriptorPool &&
        context.mUIDescriptorSetLayout;
}

bool create_vulkan_depth_attachment(LLVulkanNativeContext& context)
{
    if (context.mSwapchainExtent.width == 0 ||
        context.mSwapchainExtent.height == 0 ||
        !ensure_vulkan_image_resource_entry_points(context))
    {
        LL_WARNS("RenderBackend")
            << "Vulkan depth attachment cannot be created without image entry points and swapchain extent."
            << LL_ENDL;
        return false;
    }

    destroy_vulkan_depth_attachment(context);

    LLVulkanDepthAttachment& depth = context.mDepthAttachment;
    depth.mFormat = LL_VK_FORMAT_D32_SFLOAT;

    LLVkImageCreateInfo image_create_info =
    {
        LL_VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,
        LL_VK_IMAGE_TYPE_2D,
        depth.mFormat,
        LLVkExtent3D
        {
            context.mSwapchainExtent.width,
            context.mSwapchainExtent.height,
            1
        },
        1,
        1,
        LL_VK_SAMPLE_COUNT_1_BIT,
        LL_VK_IMAGE_TILING_OPTIMAL,
        LL_VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        LL_VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        LL_VK_IMAGE_LAYOUT_UNDEFINED
    };

    S32 result = context.mCreateImage(
        context.mDevice,
        &image_create_info,
        nullptr,
        &depth.mImage);
    if (result != LL_VK_SUCCESS || !depth.mImage)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateImage(depth) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_depth_attachment(context);
        return false;
    }

    LLVkMemoryRequirements memory_requirements = {};
    context.mGetImageMemoryRequirements(context.mDevice, depth.mImage, &memory_requirements);

    U32 memory_type_index = 0;
    if (!find_vulkan_memory_type(
            context,
            memory_requirements.memoryTypeBits,
            LL_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            memory_type_index))
    {
        LL_WARNS("RenderBackend")
            << "No device-local Vulkan memory type for depth attachment."
            << LL_ENDL;
        destroy_vulkan_depth_attachment(context);
        return false;
    }

    if (!can_commit_vulkan_heap_memory(
            context,
            memory_type_index,
            memory_requirements.size,
            "depth"))
    {
        destroy_vulkan_depth_attachment(context);
        return false;
    }

    LLVkMemoryAllocateInfo allocate_info =
    {
        LL_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memory_requirements.size,
        memory_type_index
    };

    result = context.mAllocateMemory(context.mDevice, &allocate_info, nullptr, &depth.mMemory);
    if (result != LL_VK_SUCCESS || !depth.mMemory)
    {
        LL_WARNS("RenderBackend")
            << "vkAllocateMemory(depth) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_depth_attachment(context);
        return false;
    }

    result = context.mBindImageMemory(context.mDevice, depth.mImage, depth.mMemory, 0);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkBindImageMemory(depth) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_depth_attachment(context);
        return false;
    }

    LLVkImageViewCreateInfo image_view_create_info =
    {
        LL_VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        depth.mImage,
        LL_VK_IMAGE_VIEW_TYPE_2D,
        depth.mFormat,
        LLVkComponentMapping
        {
            LL_VK_COMPONENT_SWIZZLE_IDENTITY,
            LL_VK_COMPONENT_SWIZZLE_IDENTITY,
            LL_VK_COMPONENT_SWIZZLE_IDENTITY,
            LL_VK_COMPONENT_SWIZZLE_IDENTITY
        },
        LLVkImageSubresourceRange
        {
            LL_VK_IMAGE_ASPECT_DEPTH_BIT,
            0,
            1,
            0,
            1
        }
    };

    result = context.mCreateImageView(
        context.mDevice,
        &image_view_create_info,
        nullptr,
        &depth.mImageView);
    if (result != LL_VK_SUCCESS || !depth.mImageView)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateImageView(depth) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_depth_attachment(context);
        return false;
    }

    LL_INFOS("RenderBackend")
        << "Vulkan depth attachment created: "
        << context.mSwapchainExtent.width
        << "x"
        << context.mSwapchainExtent.height
        << "."
        << LL_ENDL;
    return true;
}

bool ensure_vulkan_command_entry_points(LLVulkanNativeContext& context)
{
    if (!context.mBeginCommandBuffer)
    {
        context.mBeginCommandBuffer =
            reinterpret_cast<LLVulkanBeginCommandBuffer>(
                get_vulkan_device_proc_address(context, "vkBeginCommandBuffer"));
    }
    if (!context.mEndCommandBuffer)
    {
        context.mEndCommandBuffer =
            reinterpret_cast<LLVulkanEndCommandBuffer>(
                get_vulkan_device_proc_address(context, "vkEndCommandBuffer"));
    }
    if (!context.mResetCommandBuffer)
    {
        context.mResetCommandBuffer =
            reinterpret_cast<LLVulkanResetCommandBuffer>(
                get_vulkan_device_proc_address(context, "vkResetCommandBuffer"));
    }
    if (!context.mCmdPipelineBarrier)
    {
        context.mCmdPipelineBarrier =
            reinterpret_cast<LLVulkanCmdPipelineBarrier>(
                get_vulkan_device_proc_address(context, "vkCmdPipelineBarrier"));
    }
    if (!context.mCreateFence)
    {
        context.mCreateFence =
            reinterpret_cast<LLVulkanCreateFence>(
                get_vulkan_device_proc_address(context, "vkCreateFence"));
    }
    if (!context.mDestroyFence)
    {
        context.mDestroyFence =
            reinterpret_cast<LLVulkanDestroyFence>(
                get_vulkan_device_proc_address(context, "vkDestroyFence"));
    }
    if (!context.mWaitForFences)
    {
        context.mWaitForFences =
            reinterpret_cast<LLVulkanWaitForFences>(
                get_vulkan_device_proc_address(context, "vkWaitForFences"));
    }

    return context.mAllocateCommandBuffers &&
        context.mFreeCommandBuffers &&
        context.mBeginCommandBuffer &&
        context.mEndCommandBuffer &&
        context.mCmdPipelineBarrier &&
        context.mQueueSubmit &&
        context.mCreateFence &&
        context.mDestroyFence &&
        context.mWaitForFences &&
        context.mCommandPool;
}

bool wait_for_vulkan_fence_sleeping(
    LLVulkanNativeContext& context,
    LLVkFence fence,
    const char* operation);

bool begin_vulkan_one_time_commands(
    LLVulkanNativeContext& context,
    LLVkCommandBuffer& command_buffer)
{
    if (!ensure_vulkan_command_entry_points(context))
    {
        return false;
    }

    LLVkCommandBufferAllocateInfo allocate_info =
    {
        LL_VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        context.mCommandPool,
        LL_VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };

    S32 result = context.mAllocateCommandBuffers(
        context.mDevice,
        &allocate_info,
        &command_buffer);
    if (result != LL_VK_SUCCESS || !command_buffer)
    {
        LL_WARNS("RenderBackend")
            << "vkAllocateCommandBuffers(texture upload) failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    LLVkCommandBufferBeginInfo begin_info =
    {
        LL_VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        LL_VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr
    };

    result = context.mBeginCommandBuffer(command_buffer, &begin_info);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkBeginCommandBuffer(texture upload) failed with result "
            << result
            << LL_ENDL;
        context.mFreeCommandBuffers(context.mDevice, context.mCommandPool, 1, &command_buffer);
        command_buffer = nullptr;
        return false;
    }

    return true;
}

bool end_vulkan_one_time_commands(
    LLVulkanNativeContext& context,
    LLVkCommandBuffer command_buffer)
{
    S32 result = context.mEndCommandBuffer(command_buffer);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkEndCommandBuffer(texture upload) failed with result "
            << result
            << LL_ENDL;
        context.mFreeCommandBuffers(context.mDevice, context.mCommandPool, 1, &command_buffer);
        return false;
    }

    LLVkSubmitInfo submit_info =
    {
        LL_VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        0,
        nullptr,
        nullptr,
        1,
        &command_buffer,
        0,
        nullptr
    };

    LLVkFence upload_fence = nullptr;
    LLVkFenceCreateInfo fence_create_info =
    {
        LL_VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        nullptr,
        0
    };
    result = context.mCreateFence(
        context.mDevice,
        &fence_create_info,
        nullptr,
        &upload_fence);
    if (result != LL_VK_SUCCESS || !upload_fence)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateFence(texture upload) failed with result "
            << result
            << LL_ENDL;
        context.mFreeCommandBuffers(context.mDevice, context.mCommandPool, 1, &command_buffer);
        return false;
    }

    result = context.mQueueSubmit(context.mGraphicsQueue, 1, &submit_info, upload_fence);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkQueueSubmit(texture upload) failed with result "
            << result
            << LL_ENDL;
        context.mDestroyFence(context.mDevice, upload_fence, nullptr);
        context.mFreeCommandBuffers(context.mDevice, context.mCommandPool, 1, &command_buffer);
        return false;
    }

    const bool upload_finished = wait_for_vulkan_fence_sleeping(
        context,
        upload_fence,
        "texture upload");
    context.mDestroyFence(context.mDevice, upload_fence, nullptr);
    context.mFreeCommandBuffers(context.mDevice, context.mCommandPool, 1, &command_buffer);
    return upload_finished;
}

void transition_vulkan_texture_layout(
    LLVulkanNativeContext& context,
    LLVkCommandBuffer command_buffer,
    LLVkImage image,
    S32 old_layout,
    S32 new_layout,
    U32 aspect_mask = LL_VK_IMAGE_ASPECT_COLOR_BIT)
{
    U32 src_access = 0;
    U32 dst_access = 0;
    U32 src_stage = LL_VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    U32 dst_stage = LL_VK_PIPELINE_STAGE_TRANSFER_BIT;

    if (old_layout == LL_VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == LL_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        dst_access = LL_VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if (old_layout == LL_VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        dst_access = LL_VK_ACCESS_SHADER_READ_BIT;
        dst_stage = LL_VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (old_layout == LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
        new_layout == LL_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        src_access = LL_VK_ACCESS_SHADER_READ_BIT;
        dst_access = LL_VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = LL_VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dst_stage = LL_VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
        new_layout == LL_VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        src_access = LL_VK_ACCESS_SHADER_READ_BIT;
        dst_access = LL_VK_ACCESS_TRANSFER_READ_BIT;
        src_stage = LL_VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dst_stage = LL_VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == LL_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        new_layout == LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        src_access = LL_VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_access = LL_VK_ACCESS_SHADER_READ_BIT;
        src_stage = LL_VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = LL_VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (old_layout == LL_VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
        new_layout == LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        src_access = LL_VK_ACCESS_TRANSFER_READ_BIT;
        dst_access = LL_VK_ACCESS_SHADER_READ_BIT;
        src_stage = LL_VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = LL_VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    LLVkImageMemoryBarrier barrier =
    {
        LL_VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        src_access,
        dst_access,
        old_layout,
        new_layout,
        LL_VK_QUEUE_FAMILY_IGNORED,
        LL_VK_QUEUE_FAMILY_IGNORED,
        image,
        LLVkImageSubresourceRange
        {
            aspect_mask,
            0,
            1,
            0,
            1
        }
    };

    context.mCmdPipelineBarrier(
        command_buffer,
        src_stage,
        dst_stage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);
}

S32 to_vulkan_sampler_filter(LLRenderTextureFilter filter)
{
    switch (filter)
    {
    case LLRenderTextureFilter::Nearest:
    case LLRenderTextureFilter::NearestMipmapNearest:
        return LL_VK_FILTER_NEAREST;
    case LLRenderTextureFilter::Linear:
    case LLRenderTextureFilter::LinearMipmapLinear:
    case LLRenderTextureFilter::LinearMipmapNearest:
    default:
        return LL_VK_FILTER_LINEAR;
    }
}

S32 to_vulkan_sampler_mipmap_mode(LLRenderTextureFilter filter)
{
    switch (filter)
    {
    case LLRenderTextureFilter::LinearMipmapLinear:
    case LLRenderTextureFilter::LinearMipmapNearest:
        return LL_VK_SAMPLER_MIPMAP_MODE_LINEAR;
    case LLRenderTextureFilter::Nearest:
    case LLRenderTextureFilter::NearestMipmapNearest:
    case LLRenderTextureFilter::Linear:
    default:
        return LL_VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
}

S32 to_vulkan_sampler_address_mode(LLRenderTextureAddressMode mode)
{
    switch (mode)
    {
    case LLRenderTextureAddressMode::MirroredRepeat:
        return LL_VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case LLRenderTextureAddressMode::ClampToEdge:
        return LL_VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case LLRenderTextureAddressMode::Repeat:
    default:
        return LL_VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

LLVulkanTextureSamplerState get_vulkan_texture_sampler_state(U32 handle)
{
    auto state_iter = gVulkanTextureSamplerStates.find(handle);
    if (state_iter != gVulkanTextureSamplerStates.end())
    {
        return state_iter->second;
    }
    return {};
}

bool create_vulkan_texture_sampler(
    LLVulkanNativeContext& context,
    const LLVulkanTextureSamplerState& state,
    LLVkSampler& sampler)
{
    LLVkSamplerCreateInfo sampler_create_info =
    {
        LL_VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        nullptr,
        0,
        to_vulkan_sampler_filter(state.mMagFilter),
        to_vulkan_sampler_filter(state.mMinFilter),
        to_vulkan_sampler_mipmap_mode(state.mMinFilter),
        to_vulkan_sampler_address_mode(state.mAddressModeS),
        to_vulkan_sampler_address_mode(state.mAddressModeT),
        to_vulkan_sampler_address_mode(state.mAddressModeW),
        0.f,
        0,
        1.f,
        0,
        0,
        0.f,
        0.f,
        LL_VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        0
    };

    S32 result = context.mCreateSampler(context.mDevice, &sampler_create_info, nullptr, &sampler);
    if (result != LL_VK_SUCCESS || !sampler)
    {
        LL_WARNS("RenderBackend") << "vkCreateSampler(texture) failed with result " << result << LL_ENDL;
        return false;
    }
    return true;
}

LLVulkanTextureResource* get_vulkan_texture_resource_or_fallback(
    U32 handle,
    U32& resolved_handle)
{
    auto texture_iter = gVulkanTextures.find(handle);
    if (texture_iter != gVulkanTextures.end() &&
        texture_iter->second.mSampler &&
        texture_iter->second.mImageView)
    {
        resolved_handle = handle;
        return &texture_iter->second;
    }

    auto fallback_iter = gVulkanTextures.find(0);
    if (fallback_iter != gVulkanTextures.end() &&
        fallback_iter->second.mSampler &&
        fallback_iter->second.mImageView)
    {
        resolved_handle = 0;
        return &fallback_iter->second;
    }

    return nullptr;
}

LLVkDescriptorSet get_vulkan_texture_descriptor_set(
    LLVulkanNativeContext& context,
    const LLVulkanPendingDraw::texture_bindings_t& textures)
{
    if (!context.mAllocateDescriptorSets ||
        !context.mUpdateDescriptorSets ||
        !context.mUIDescriptorPool ||
        !context.mUIDescriptorSetLayout)
    {
        return nullptr;
    }

    LLVulkanPendingDraw::texture_bindings_t key = {};
    std::array<LLVkDescriptorImageInfo, MARE_VULKAN_MAX_TEXTURE_BINDINGS> image_infos = {};
    for (U32 i = 0; i < MARE_VULKAN_MAX_TEXTURE_BINDINGS; ++i)
    {
        const U32 requested_handle = textures[i];
        U32 resolved_handle = 0;
        LLVulkanTextureResource* resource =
            get_vulkan_texture_resource_or_fallback(requested_handle, resolved_handle);
        if (!resource)
        {
            return nullptr;
        }

        resource->mLastBoundFrame = context.mPresentedFrameCount;
        key[i] = resolved_handle;
        image_infos[i] =
        {
            resource->mSampler,
            resource->mImageView,
            LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
    }

    auto cached_iter = gVulkanTextureDescriptorSetCache.find(key);
    if (cached_iter != gVulkanTextureDescriptorSetCache.end())
    {
        return cached_iter->second;
    }

    if (gVulkanTextureDescriptorSetCache.size() >=
        MARE_VULKAN_TEXTURE_DESCRIPTOR_SET_CAPACITY)
    {
        if (!context.mLoggedTextureDescriptorCacheFull)
        {
            LL_WARNS("RenderBackend")
                << "Vulkan texture descriptor cache reached its safety cap of "
                << MARE_VULKAN_TEXTURE_DESCRIPTOR_SET_CAPACITY
                << " set(s); additional unique texture bindings will be skipped."
                << LL_ENDL;
            context.mLoggedTextureDescriptorCacheFull = true;
        }
        return nullptr;
    }

    LLVkDescriptorSet descriptor_set = nullptr;
    LLVkDescriptorSetAllocateInfo descriptor_allocate_info =
    {
        LL_VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        context.mUIDescriptorPool,
        1,
        &context.mUIDescriptorSetLayout
    };

    S32 result = context.mAllocateDescriptorSets(
        context.mDevice,
        &descriptor_allocate_info,
        &descriptor_set);
    if (result != LL_VK_SUCCESS || !descriptor_set)
    {
        LL_WARNS("RenderBackend")
            << "vkAllocateDescriptorSets(texture bindings) failed with result "
            << result
            << LL_ENDL;
        return nullptr;
    }

    LLVkDescriptorBufferInfo skinning_buffer_info =
    {
        context.mSkinningMatrixPaletteBuffer.mBuffer,
        0,
        context.mSkinningMatrixPaletteBuffer.mSize
    };

    std::array<LLVkWriteDescriptorSet, MARE_VULKAN_MAX_TEXTURE_BINDINGS + 1> write_descriptors = {};
    for (U32 i = 0; i < MARE_VULKAN_MAX_TEXTURE_BINDINGS; ++i)
    {
        write_descriptors[i] =
        {
            LL_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptor_set,
            i,
            0,
            1,
            LL_VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            &image_infos[i],
            nullptr,
            nullptr
        };
    }

    U32 write_descriptor_count = MARE_VULKAN_MAX_TEXTURE_BINDINGS;
    if (context.mSkinningMatrixPaletteBuffer.mBuffer &&
        context.mSkinningMatrixPaletteBuffer.mSize > 0)
    {
        write_descriptors[write_descriptor_count] =
        {
            LL_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptor_set,
            MARE_VULKAN_SKINNING_DESCRIPTOR_BINDING,
            0,
            1,
            LL_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            nullptr,
            &skinning_buffer_info,
            nullptr
        };
        ++write_descriptor_count;
    }

    context.mUpdateDescriptorSets(
        context.mDevice,
        write_descriptor_count,
        write_descriptors.data(),
        0,
        nullptr);

    gVulkanTextureDescriptorSetCache[key] = descriptor_set;
    return descriptor_set;
}

bool upload_vulkan_texture_pixels_to_image(
    LLVulkanNativeContext& context,
    LLVulkanTextureResource& resource,
    const std::vector<U8>& pixels,
    S32 xoffset,
    S32 yoffset,
    S32 width,
    S32 height,
    S32 old_layout)
{
    if (width <= 0 ||
        height <= 0 ||
        pixels.size() != static_cast<size_t>(width * height * 4) ||
        !resource.mImage)
    {
        return false;
    }

    if (!ensure_vulkan_texture_entry_points(context) ||
        !ensure_vulkan_command_entry_points(context))
    {
        return false;
    }

    LLVulkanBufferResource staging;
    if (!create_vulkan_buffer_resource(
            context,
            pixels.size(),
            LL_VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            pixels.data(),
            staging,
            0,
            false))
    {
        return false;
    }

    LLVkCommandBuffer command_buffer = nullptr;
    if (!begin_vulkan_one_time_commands(context, command_buffer))
    {
        destroy_vulkan_buffer_resource(context, staging);
        return false;
    }

    transition_vulkan_texture_layout(
        context,
        command_buffer,
        resource.mImage,
        old_layout,
        LL_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    LLVkBufferImageCopy copy_region =
    {
        0,
        0,
        0,
        LLVkImageSubresourceLayers
        {
            LL_VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            0,
            1
        },
        { xoffset, yoffset, 0 },
        LLVkExtent3D
        {
            static_cast<U32>(width),
            static_cast<U32>(height),
            1
        }
    };

    context.mCmdCopyBufferToImage(
        command_buffer,
        staging.mBuffer,
        resource.mImage,
        LL_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy_region);

    transition_vulkan_texture_layout(
        context,
        command_buffer,
        resource.mImage,
        LL_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    const bool upload_ok = end_vulkan_one_time_commands(context, command_buffer);
    destroy_vulkan_buffer_resource(context, staging);
    return upload_ok;
}

bool update_vulkan_texture_sampler(
    LLVulkanNativeContext& context,
    U32 handle,
    LLVulkanTextureResource& resource)
{
    if (!resource.mImageView || !ensure_vulkan_texture_entry_points(context))
    {
        return false;
    }

    destroy_vulkan_texture_descriptor_set_cache(context);

    if (resource.mSampler)
    {
        context.mDestroySampler(context.mDevice, resource.mSampler, nullptr);
        resource.mSampler = nullptr;
    }

    if (!create_vulkan_texture_sampler(
            context,
            get_vulkan_texture_sampler_state(handle),
            resource.mSampler))
    {
        return false;
    }

    return true;
}

bool upload_vulkan_texture_resource(
    LLVulkanNativeContext& context,
    U32 handle,
    S32 width,
    S32 height,
    const std::vector<U8>& pixels)
{
    if (width <= 0 || height <= 0 || pixels.size() != static_cast<size_t>(width * height * 4))
    {
        return false;
    }

    if (!ensure_vulkan_texture_entry_points(context) ||
        !ensure_vulkan_command_entry_points(context))
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan texture upload skipped because required entry points are not ready."
            << LL_ENDL;
        return false;
    }

    auto existing_iter = gVulkanTextures.find(handle);
    const U64 old_memory_size =
        existing_iter != gVulkanTextures.end() ?
        existing_iter->second.mMemorySize :
        0;

    LLVulkanTextureResource new_resource;
    new_resource.mWidth = width;
    new_resource.mHeight = height;
    new_resource.mFormat = LL_VK_FORMAT_R8G8B8A8_UNORM;
    new_resource.mAspectMask = LL_VK_IMAGE_ASPECT_COLOR_BIT;
    new_resource.mLastBoundFrame = context.mPresentedFrameCount;

    invalidate_vulkan_framebuffers_for_texture(handle);

    LLVkImageCreateInfo image_create_info =
    {
        LL_VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,
        LL_VK_IMAGE_TYPE_2D,
        LL_VK_FORMAT_R8G8B8A8_UNORM,
        LLVkExtent3D
        {
            static_cast<U32>(width),
            static_cast<U32>(height),
            1
        },
        1,
        1,
        LL_VK_SAMPLE_COUNT_1_BIT,
        LL_VK_IMAGE_TILING_OPTIMAL,
        LL_VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            LL_VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            LL_VK_IMAGE_USAGE_SAMPLED_BIT |
            LL_VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        LL_VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        LL_VK_IMAGE_LAYOUT_UNDEFINED
    };

    S32 result = context.mCreateImage(
        context.mDevice,
        &image_create_info,
        nullptr,
        &new_resource.mImage);
    if (result != LL_VK_SUCCESS || !new_resource.mImage)
    {
        LL_WARNS("RenderBackend") << "vkCreateImage(texture) failed with result " << result << LL_ENDL;
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    LLVkMemoryRequirements memory_requirements = {};
    context.mGetImageMemoryRequirements(context.mDevice, new_resource.mImage, &memory_requirements);
    new_resource.mMemorySize = memory_requirements.size;

    U32 memory_type_index = 0;
    if (!find_vulkan_memory_type(
            context,
            memory_requirements.memoryTypeBits,
            LL_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            memory_type_index))
    {
        LL_WARNS("RenderBackend") << "No device-local Vulkan memory type for texture." << LL_ENDL;
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    evict_vulkan_texture_resources_for_upload(
        context,
        handle,
        old_memory_size,
        new_resource.mMemorySize,
        get_vulkan_texture_memory_budget_bytes(context, memory_type_index));

    if (!can_commit_vulkan_texture_memory(
            context,
            old_memory_size,
            new_resource.mMemorySize,
            memory_type_index,
            handle,
            width,
            height))
    {
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    LLVkMemoryAllocateInfo allocate_info =
    {
        LL_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memory_requirements.size,
        memory_type_index
    };

    result = context.mAllocateMemory(context.mDevice, &allocate_info, nullptr, &new_resource.mMemory);
    if (result != LL_VK_SUCCESS || !new_resource.mMemory)
    {
        LL_WARNS("RenderBackend") << "vkAllocateMemory(texture) failed with result " << result << LL_ENDL;
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    result = context.mBindImageMemory(context.mDevice, new_resource.mImage, new_resource.mMemory, 0);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend") << "vkBindImageMemory(texture) failed with result " << result << LL_ENDL;
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    if (!upload_vulkan_texture_pixels_to_image(
            context,
            new_resource,
            pixels,
            0,
            0,
            width,
            height,
            LL_VK_IMAGE_LAYOUT_UNDEFINED))
    {
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    LLVkImageViewCreateInfo image_view_create_info =
    {
        LL_VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        new_resource.mImage,
        LL_VK_IMAGE_VIEW_TYPE_2D,
        LL_VK_FORMAT_R8G8B8A8_UNORM,
        LLVkComponentMapping
        {
            LL_VK_COMPONENT_SWIZZLE_IDENTITY,
            LL_VK_COMPONENT_SWIZZLE_IDENTITY,
            LL_VK_COMPONENT_SWIZZLE_IDENTITY,
            LL_VK_COMPONENT_SWIZZLE_IDENTITY
        },
        LLVkImageSubresourceRange
        {
            LL_VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            1,
            0,
            1
        }
    };

    result = context.mCreateImageView(
        context.mDevice,
        &image_view_create_info,
        nullptr,
        &new_resource.mImageView);
    if (result != LL_VK_SUCCESS || !new_resource.mImageView)
    {
        LL_WARNS("RenderBackend") << "vkCreateImageView(texture) failed with result " << result << LL_ENDL;
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    if (!create_vulkan_texture_sampler(
            context,
            get_vulkan_texture_sampler_state(handle),
            new_resource.mSampler))
    {
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    new_resource.mMemoryAccounted = true;
    context.mTextureMemoryAllocatedBytes += new_resource.mMemorySize;
    if (existing_iter != gVulkanTextures.end())
    {
        destroy_vulkan_texture_resource(context, existing_iter->second);
        existing_iter->second = new_resource;
    }
    else
    {
        gVulkanTextures.emplace(handle, new_resource);
    }

    ++context.mTextureUploadCount;
    if (width * height > context.mLargestTextureWidth * context.mLargestTextureHeight)
    {
        context.mLargestTextureHandle = handle;
        context.mLargestTextureWidth = width;
        context.mLargestTextureHeight = height;
        if (handle != 0 && (width >= 512 || height >= 512))
        {
            LL_INFOS("RenderBackend")
                << "Vulkan UI texture bridge largest texture is now "
                << handle
                << " ("
                << width
                << "x"
                << height
                << ")."
                << LL_ENDL;
        }
    }

    static bool logged_first_texture = false;
    if (!logged_first_texture)
    {
        LL_INFOS("RenderBackend")
            << "Vulkan UI texture bridge uploaded texture "
            << handle
            << " ("
            << width
            << "x"
            << height
            << ")."
            << LL_ENDL;
        logged_first_texture = true;
    }

    return true;
}

bool create_vulkan_fallback_texture(LLVulkanNativeContext& context)
{
    const std::vector<U8> white_pixel = { 255, 255, 255, 255 };
    return upload_vulkan_texture_resource(context, 0, 1, 1, white_pixel);
}

constexpr U32 LL_LEGACY_GL_ALPHA = 0x1906;
constexpr U32 LL_LEGACY_GL_RGB = 0x1907;
constexpr U32 LL_LEGACY_GL_RGBA = 0x1908;
constexpr U32 LL_LEGACY_GL_LUMINANCE = 0x1909;
constexpr U32 LL_LEGACY_GL_LUMINANCE_ALPHA = 0x190a;
constexpr U32 LL_LEGACY_GL_RED = 0x1903;
constexpr U32 LL_LEGACY_GL_RG = 0x8227;
constexpr U32 LL_LEGACY_GL_BGRA = 0x80e1;
constexpr U32 LL_LEGACY_GL_UNSIGNED_BYTE = 0x1401;
constexpr U32 LL_LEGACY_GL_SCISSOR_TEST = 0x0c11;

bool is_vulkan_glyph_texture_format(U32 format)
{
    return format == LL_LEGACY_GL_ALPHA ||
           format == LL_LEGACY_GL_LUMINANCE ||
           format == LL_LEGACY_GL_LUMINANCE_ALPHA ||
           format == LL_LEGACY_GL_RED ||
           format == LL_LEGACY_GL_RG;
}

bool convert_texture_pixels_to_rgba8(
    S32 width,
    S32 height,
    S32 source_row_length,
    U32 format,
    U32 type,
    const void* data,
    std::vector<U8>& pixels)
{
    if (width <= 0 || height <= 0 || type != LL_LEGACY_GL_UNSIGNED_BYTE)
    {
        return false;
    }

    const size_t pixel_count = static_cast<size_t>(width * height);
    pixels.assign(pixel_count * 4, 0);
    if (!data)
    {
        return true;
    }

    const S32 row_length = source_row_length > 0 ? source_row_length : width;
    if (row_length < width)
    {
        return false;
    }

    const U8* source = static_cast<const U8*>(data);
    for (S32 row = 0; row < height; ++row)
    {
        for (S32 col = 0; col < width; ++col)
        {
            const size_t source_index = static_cast<size_t>(row * row_length + col);
            const size_t destination_index = static_cast<size_t>(row * width + col);
            U8* destination = pixels.data() + destination_index * 4;
            switch (format)
            {
            case LL_LEGACY_GL_RGBA:
                destination[0] = source[source_index * 4 + 0];
                destination[1] = source[source_index * 4 + 1];
                destination[2] = source[source_index * 4 + 2];
                destination[3] = source[source_index * 4 + 3];
                break;
            case LL_LEGACY_GL_BGRA:
                destination[0] = source[source_index * 4 + 2];
                destination[1] = source[source_index * 4 + 1];
                destination[2] = source[source_index * 4 + 0];
                destination[3] = source[source_index * 4 + 3];
                break;
            case LL_LEGACY_GL_RGB:
                destination[0] = source[source_index * 3 + 0];
                destination[1] = source[source_index * 3 + 1];
                destination[2] = source[source_index * 3 + 2];
                destination[3] = 255;
                break;
            case LL_LEGACY_GL_ALPHA:
            case LL_LEGACY_GL_RED:
                destination[0] = 255;
                destination[1] = 255;
                destination[2] = 255;
                destination[3] = source[source_index];
                break;
            case LL_LEGACY_GL_LUMINANCE:
                destination[0] = source[source_index];
                destination[1] = source[source_index];
                destination[2] = source[source_index];
                destination[3] = 255;
                break;
            case LL_LEGACY_GL_LUMINANCE_ALPHA:
            case LL_LEGACY_GL_RG:
                destination[0] = source[source_index * 2 + 0];
                destination[1] = source[source_index * 2 + 0];
                destination[2] = source[source_index * 2 + 0];
                destination[3] = source[source_index * 2 + 1];
                break;
            default:
                return false;
            }
        }
    }

    return true;
}

bool is_vulkan_depth_render_texture_format(LLRenderTextureFormat format)
{
    return format == LLRenderTextureFormat::DepthComponent ||
        format == LLRenderTextureFormat::DepthComponent24;
}

S32 to_vulkan_image_format(LLRenderTextureFormat format)
{
    switch (format)
    {
    case LLRenderTextureFormat::DepthComponent:
    case LLRenderTextureFormat::DepthComponent24:
        return LL_VK_FORMAT_D32_SFLOAT;
    case LLRenderTextureFormat::RGBA:
    case LLRenderTextureFormat::RGBA8:
    case LLRenderTextureFormat::RGB:
    case LLRenderTextureFormat::RGB8:
        return LL_VK_FORMAT_R8G8B8A8_UNORM;
    case LLRenderTextureFormat::RGBA16:
        return LL_VK_FORMAT_R16G16B16A16_UNORM;
    case LLRenderTextureFormat::RGBA16F:
    case LLRenderTextureFormat::RGB16F:
        return LL_VK_FORMAT_R16G16B16A16_SFLOAT;
    case LLRenderTextureFormat::RGB10A2:
        return LL_VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    case LLRenderTextureFormat::R11G11B10F:
        return LL_VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case LLRenderTextureFormat::Alpha:
    case LLRenderTextureFormat::Alpha8:
    case LLRenderTextureFormat::R8:
    case LLRenderTextureFormat::Luminance:
        return LL_VK_FORMAT_R8_UNORM;
    case LLRenderTextureFormat::R16F:
        return LL_VK_FORMAT_R16_SFLOAT;
    case LLRenderTextureFormat::R32F:
        return LL_VK_FORMAT_R32_SFLOAT;
    case LLRenderTextureFormat::RG8:
        return LL_VK_FORMAT_R8G8_UNORM;
    case LLRenderTextureFormat::RG16F:
        return LL_VK_FORMAT_R16G16_SFLOAT;
    case LLRenderTextureFormat::RG32F:
        return LL_VK_FORMAT_R32G32_SFLOAT;
    case LLRenderTextureFormat::None:
    default:
        return LL_VK_FORMAT_R8G8B8A8_UNORM;
    }
}

U32 to_vulkan_image_aspect_mask(LLRenderTextureFormat format)
{
    return is_vulkan_depth_render_texture_format(format) ?
        LL_VK_IMAGE_ASPECT_DEPTH_BIT :
        LL_VK_IMAGE_ASPECT_COLOR_BIT;
}

U32 to_vulkan_image_usage(LLRenderTextureFormat format)
{
    U32 usage =
        LL_VK_IMAGE_USAGE_SAMPLED_BIT |
        LL_VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        LL_VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (is_vulkan_depth_render_texture_format(format))
    {
        usage |= LL_VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    else
    {
        usage |= LL_VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    return usage;
}

S32 to_vulkan_framebuffer_color_index(LLRenderFramebufferAttachment attachment)
{
    switch (attachment)
    {
    case LLRenderFramebufferAttachment::Color0:
        return 0;
    case LLRenderFramebufferAttachment::Color1:
        return 1;
    case LLRenderFramebufferAttachment::Color2:
        return 2;
    case LLRenderFramebufferAttachment::Color3:
        return 3;
    default:
        return -1;
    }
}

bool is_vulkan_framebuffer_complete(U32 framebuffer)
{
    if (framebuffer == 0)
    {
        return true;
    }

    auto framebuffer_iter = gVulkanFramebuffers.find(framebuffer);
    if (framebuffer_iter == gVulkanFramebuffers.end())
    {
        return false;
    }

    const LLVulkanFramebufferResource& resource = framebuffer_iter->second;
    bool has_attachment = false;
    for (U32 i = 0; i < resource.mColorAttachmentCount && i < resource.mColorTextures.size(); ++i)
    {
        U32 texture = resource.mColorTextures[i];
        if (!texture)
        {
            return false;
        }

        auto texture_iter = gVulkanTextures.find(texture);
        if (texture_iter == gVulkanTextures.end() ||
            !texture_iter->second.mImage ||
            !texture_iter->second.mImageView ||
            texture_iter->second.mAspectMask != LL_VK_IMAGE_ASPECT_COLOR_BIT)
        {
            return false;
        }
        has_attachment = true;
    }

    if (resource.mDepthTexture)
    {
        auto texture_iter = gVulkanTextures.find(resource.mDepthTexture);
        if (texture_iter == gVulkanTextures.end() ||
            !texture_iter->second.mImage ||
            !texture_iter->second.mImageView ||
            texture_iter->second.mAspectMask != LL_VK_IMAGE_ASPECT_DEPTH_BIT)
        {
            return false;
        }
        has_attachment = true;
    }

    return has_attachment;
}

bool create_empty_vulkan_texture_resource(
    LLVulkanNativeContext& context,
    U32 handle,
    S32 width,
    S32 height,
    LLRenderTextureFormat render_format)
{
    if (width <= 0 || height <= 0)
    {
        return false;
    }

    if (!ensure_vulkan_texture_entry_points(context) ||
        !ensure_vulkan_command_entry_points(context))
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan empty texture allocation skipped because required entry points are not ready."
            << LL_ENDL;
        return false;
    }

    auto existing_iter = gVulkanTextures.find(handle);
    const U64 old_memory_size =
        existing_iter != gVulkanTextures.end() ?
        existing_iter->second.mMemorySize :
        0;

    LLVulkanTextureResource new_resource;
    new_resource.mWidth = width;
    new_resource.mHeight = height;
    new_resource.mAspectMask = to_vulkan_image_aspect_mask(render_format);
    new_resource.mFormat = to_vulkan_image_format(render_format);
    new_resource.mLastBoundFrame = context.mPresentedFrameCount;
    invalidate_vulkan_framebuffers_for_texture(handle);

    LLVkImageCreateInfo image_create_info =
    {
        LL_VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,
        LL_VK_IMAGE_TYPE_2D,
        new_resource.mFormat,
        LLVkExtent3D
        {
            static_cast<U32>(width),
            static_cast<U32>(height),
            1
        },
        1,
        1,
        LL_VK_SAMPLE_COUNT_1_BIT,
        LL_VK_IMAGE_TILING_OPTIMAL,
        to_vulkan_image_usage(render_format),
        LL_VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        LL_VK_IMAGE_LAYOUT_UNDEFINED
    };

    S32 result = context.mCreateImage(
        context.mDevice,
        &image_create_info,
        nullptr,
        &new_resource.mImage);
    if (result != LL_VK_SUCCESS || !new_resource.mImage)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateImage(empty texture) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    LLVkMemoryRequirements memory_requirements = {};
    context.mGetImageMemoryRequirements(context.mDevice, new_resource.mImage, &memory_requirements);
    new_resource.mMemorySize = memory_requirements.size;

    U32 memory_type_index = 0;
    if (!find_vulkan_memory_type(
            context,
            memory_requirements.memoryTypeBits,
            LL_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            memory_type_index))
    {
        LL_WARNS("RenderBackend")
            << "No device-local Vulkan memory type for empty texture."
            << LL_ENDL;
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    evict_vulkan_texture_resources_for_upload(
        context,
        handle,
        old_memory_size,
        new_resource.mMemorySize,
        get_vulkan_texture_memory_budget_bytes(context, memory_type_index));

    if (!can_commit_vulkan_texture_memory(
            context,
            old_memory_size,
            new_resource.mMemorySize,
            memory_type_index,
            handle,
            width,
            height))
    {
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    LLVkMemoryAllocateInfo allocate_info =
    {
        LL_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memory_requirements.size,
        memory_type_index
    };

    result = context.mAllocateMemory(context.mDevice, &allocate_info, nullptr, &new_resource.mMemory);
    if (result != LL_VK_SUCCESS || !new_resource.mMemory)
    {
        LL_WARNS("RenderBackend")
            << "vkAllocateMemory(empty texture) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    result = context.mBindImageMemory(context.mDevice, new_resource.mImage, new_resource.mMemory, 0);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkBindImageMemory(empty texture) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    LLVkImageViewCreateInfo image_view_create_info =
    {
        LL_VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        new_resource.mImage,
        LL_VK_IMAGE_VIEW_TYPE_2D,
        new_resource.mFormat,
        LLVkComponentMapping
        {
            LL_VK_COMPONENT_SWIZZLE_IDENTITY,
            LL_VK_COMPONENT_SWIZZLE_IDENTITY,
            LL_VK_COMPONENT_SWIZZLE_IDENTITY,
            LL_VK_COMPONENT_SWIZZLE_IDENTITY
        },
        LLVkImageSubresourceRange
        {
            new_resource.mAspectMask,
            0,
            1,
            0,
            1
        }
    };

    result = context.mCreateImageView(
        context.mDevice,
        &image_view_create_info,
        nullptr,
        &new_resource.mImageView);
    if (result != LL_VK_SUCCESS || !new_resource.mImageView)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateImageView(empty texture) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    if (!create_vulkan_texture_sampler(
            context,
            get_vulkan_texture_sampler_state(handle),
            new_resource.mSampler))
    {
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    LLVkCommandBuffer command_buffer = nullptr;
    if (!begin_vulkan_one_time_commands(context, command_buffer))
    {
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    transition_vulkan_texture_layout(
        context,
        command_buffer,
        new_resource.mImage,
        LL_VK_IMAGE_LAYOUT_UNDEFINED,
        LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        new_resource.mAspectMask);

    if (!end_vulkan_one_time_commands(context, command_buffer))
    {
        destroy_vulkan_texture_resource(context, new_resource);
        return false;
    }

    new_resource.mMemoryAccounted = true;
    context.mTextureMemoryAllocatedBytes += new_resource.mMemorySize;
    if (existing_iter != gVulkanTextures.end())
    {
        destroy_vulkan_texture_resource(context, existing_iter->second);
        existing_iter->second = new_resource;
    }
    else
    {
        gVulkanTextures.emplace(handle, new_resource);
    }

    return true;
}

bool copy_vulkan_texture_resource(
    LLVulkanNativeContext& context,
    LLVulkanTextureResource& source,
    S32 source_x,
    S32 source_y,
    LLVulkanTextureResource& destination,
    S32 destination_x,
    S32 destination_y,
    S32 width,
    S32 height)
{
    if (width <= 0 ||
        height <= 0 ||
        !source.mImage ||
        !destination.mImage ||
        source.mAspectMask != destination.mAspectMask)
    {
        return false;
    }

    LLVulkanCmdCopyImage copy_image =
        reinterpret_cast<LLVulkanCmdCopyImage>(
            get_vulkan_device_proc_address(context, "vkCmdCopyImage"));
    if (!copy_image || !ensure_vulkan_command_entry_points(context))
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan image copy skipped because required entry points are not ready."
            << LL_ENDL;
        return false;
    }

    LLVkCommandBuffer command_buffer = nullptr;
    if (!begin_vulkan_one_time_commands(context, command_buffer))
    {
        return false;
    }

    transition_vulkan_texture_layout(
        context,
        command_buffer,
        source.mImage,
        LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        LL_VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        source.mAspectMask);
    transition_vulkan_texture_layout(
        context,
        command_buffer,
        destination.mImage,
        LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        LL_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        destination.mAspectMask);

    LLVkImageCopy copy_region =
    {
        LLVkImageSubresourceLayers
        {
            source.mAspectMask,
            0,
            0,
            1
        },
        LLVkOffset3D
        {
            source_x,
            source_y,
            0
        },
        LLVkImageSubresourceLayers
        {
            destination.mAspectMask,
            0,
            0,
            1
        },
        LLVkOffset3D
        {
            destination_x,
            destination_y,
            0
        },
        LLVkExtent3D
        {
            static_cast<U32>(width),
            static_cast<U32>(height),
            1
        }
    };

    copy_image(
        command_buffer,
        source.mImage,
        LL_VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        destination.mImage,
        LL_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy_region);

    transition_vulkan_texture_layout(
        context,
        command_buffer,
        source.mImage,
        LL_VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        source.mAspectMask);
    transition_vulkan_texture_layout(
        context,
        command_buffer,
        destination.mImage,
        LL_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        destination.mAspectMask);

    return end_vulkan_one_time_commands(context, command_buffer);
}

std::vector<LLVkExtensionProperties> get_instance_extensions(
    LLVulkanEnumerateInstanceExtensionProperties enumerate_instance_extension_properties)
{
    std::vector<LLVkExtensionProperties> extensions;
    if (!enumerate_instance_extension_properties)
    {
        return extensions;
    }

    U32 extension_count = 0;
    if (enumerate_instance_extension_properties(nullptr, &extension_count, nullptr) != LL_VK_SUCCESS ||
        extension_count == 0)
    {
        return extensions;
    }

    extensions.resize(extension_count);
    if (enumerate_instance_extension_properties(
            nullptr,
            &extension_count,
            extensions.data()) != LL_VK_SUCCESS)
    {
        extensions.clear();
        return extensions;
    }

    extensions.resize(extension_count);
    return extensions;
}

std::vector<LLVkExtensionProperties> get_device_extensions(
    LLVulkanEnumerateDeviceExtensionProperties enumerate_device_extension_properties,
    LLVkPhysicalDevice physical_device)
{
    std::vector<LLVkExtensionProperties> extensions;
    if (!enumerate_device_extension_properties || !physical_device)
    {
        return extensions;
    }

    U32 extension_count = 0;
    if (enumerate_device_extension_properties(
            physical_device,
            nullptr,
            &extension_count,
            nullptr) != LL_VK_SUCCESS ||
        extension_count == 0)
    {
        return extensions;
    }

    extensions.resize(extension_count);
    if (enumerate_device_extension_properties(
            physical_device,
            nullptr,
            &extension_count,
            extensions.data()) != LL_VK_SUCCESS)
    {
        extensions.clear();
        return extensions;
    }

    extensions.resize(extension_count);
    return extensions;
}

bool create_vulkan_instance(LLVulkanNativeContext& context)
{
    const LLVulkanLoader& loader = get_vulkan_loader();

    auto enumerate_instance_extension_properties =
        reinterpret_cast<LLVulkanEnumerateInstanceExtensionProperties>(
            loader.getInstanceProcAddress(nullptr, "vkEnumerateInstanceExtensionProperties"));
    LLVulkanCreateInstance create_instance =
        reinterpret_cast<LLVulkanCreateInstance>(
            loader.getInstanceProcAddress(nullptr, "vkCreateInstance"));
    if (!enumerate_instance_extension_properties || !create_instance)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan loader is missing required instance entry points."
            << LL_ENDL;
        return false;
    }

    std::vector<LLVkExtensionProperties> available_extensions =
        get_instance_extensions(enumerate_instance_extension_properties);
    std::vector<const char*> enabled_extensions;

    if (!has_vulkan_extension(available_extensions, LL_VK_KHR_SURFACE_EXTENSION_NAME) ||
        !has_vulkan_extension(available_extensions, LL_VK_EXT_METAL_SURFACE_EXTENSION_NAME))
    {
        LL_WARNS("RenderBackend")
            << "Vulkan loader does not expose the macOS surface extensions required by MoltenVK."
            << LL_ENDL;
        return false;
    }

    enabled_extensions.push_back(LL_VK_KHR_SURFACE_EXTENSION_NAME);
    enabled_extensions.push_back(LL_VK_EXT_METAL_SURFACE_EXTENSION_NAME);

    U32 instance_flags = 0;
    if (has_vulkan_extension(available_extensions, LL_VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
        enabled_extensions.push_back(LL_VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        instance_flags |= LL_VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    if (has_vulkan_extension(available_extensions, LL_VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    {
        enabled_extensions.push_back(LL_VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

    U32 api_version = get_vulkan_probe_result().mApiVersion;
    if (!api_version)
    {
        api_version = LL_VK_MAKE_API_VERSION(0, 1, 0, 0);
    }

    LLVkApplicationInfo application_info =
    {
        LL_VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "Mare Viewer",
        1,
        "Mare Render Backend",
        1,
        api_version
    };

    LLVkInstanceCreateInfo create_info =
    {
        LL_VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        instance_flags,
        &application_info,
        0,
        nullptr,
        static_cast<U32>(enabled_extensions.size()),
        enabled_extensions.data()
    };

    S32 result = create_instance(&create_info, nullptr, &context.mInstance);
    if (result != LL_VK_SUCCESS || !context.mInstance)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateInstance failed with result " << result
            << LL_ENDL;
        return false;
    }

    context.mDestroyInstance =
        reinterpret_cast<LLVulkanDestroyInstance>(
            loader.getInstanceProcAddress(context.mInstance, "vkDestroyInstance"));
    context.mEnumeratePhysicalDevices =
        reinterpret_cast<LLVulkanEnumeratePhysicalDevices>(
            loader.getInstanceProcAddress(context.mInstance, "vkEnumeratePhysicalDevices"));
    if (!context.mDestroyInstance || !context.mEnumeratePhysicalDevices)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan instance is missing required device enumeration entry points."
            << LL_ENDL;
        return false;
    }

    U32 physical_device_count = 0;
    result = context.mEnumeratePhysicalDevices(context.mInstance, &physical_device_count, nullptr);
    if (result != LL_VK_SUCCESS || physical_device_count == 0)
    {
        LL_WARNS("RenderBackend")
            << "vkEnumeratePhysicalDevices failed or returned no devices, result "
            << result
            << LL_ENDL;
        return false;
    }

    context.mPhysicalDevices.resize(physical_device_count);
    result = context.mEnumeratePhysicalDevices(
        context.mInstance,
        &physical_device_count,
        context.mPhysicalDevices.data());
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkEnumeratePhysicalDevices failed while fetching devices, result "
            << result
            << LL_ENDL;
        context.mPhysicalDevices.clear();
        return false;
    }

    context.mPhysicalDevices.resize(physical_device_count);

    LL_INFOS("RenderBackend")
        << "Vulkan instance created with "
        << context.mPhysicalDevices.size()
        << " physical device(s)."
        << LL_ENDL;
    return true;
}

bool create_vulkan_surface(LLVulkanNativeContext& context)
{
#if LL_DARWIN
    const LLVulkanLoader& loader = get_vulkan_loader();

    LLVulkanCreateMetalSurfaceEXT create_metal_surface =
        reinterpret_cast<LLVulkanCreateMetalSurfaceEXT>(
            loader.getInstanceProcAddress(context.mInstance, "vkCreateMetalSurfaceEXT"));
    context.mDestroySurface =
        reinterpret_cast<LLVulkanDestroySurfaceKHR>(
            loader.getInstanceProcAddress(context.mInstance, "vkDestroySurfaceKHR"));

    if (!create_metal_surface || !context.mDestroySurface)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan instance is missing required macOS surface entry points."
            << LL_ENDL;
        return false;
    }

    LLVkMetalSurfaceCreateInfoEXT create_info =
    {
        LL_VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
        nullptr,
        0,
        context.mMetalLayer
    };

    S32 result = create_metal_surface(
        context.mInstance,
        &create_info,
        nullptr,
        &context.mSurface);
    if (result != LL_VK_SUCCESS || !context.mSurface)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateMetalSurfaceEXT failed with result " << result
            << LL_ENDL;
        return false;
    }

    LL_INFOS("RenderBackend")
        << "Vulkan Metal surface created."
        << LL_ENDL;
    return true;
#else
    return false;
#endif
}

void destroy_vulkan_surface(LLVulkanNativeContext& context)
{
    if (context.mDestroySurface && context.mInstance && context.mSurface)
    {
        context.mDestroySurface(context.mInstance, context.mSurface, nullptr);
    }

    context.mDestroySurface = nullptr;
    context.mSurface = nullptr;
}

bool find_vulkan_queue_families(
    LLVulkanNativeContext& context,
    LLVkPhysicalDevice physical_device,
    LLVulkanGetPhysicalDeviceQueueFamilyProperties get_queue_family_properties,
    LLVulkanGetPhysicalDeviceSurfaceSupportKHR get_surface_support)
{
    U32 queue_family_count = 0;
    get_queue_family_properties(physical_device, &queue_family_count, nullptr);
    if (queue_family_count == 0)
    {
        return false;
    }

    std::vector<LLVkQueueFamilyProperties> queue_families(queue_family_count);
    get_queue_family_properties(physical_device, &queue_family_count, queue_families.data());

    U32 graphics_family = LL_VK_QUEUE_FAMILY_IGNORED;
    U32 present_family = LL_VK_QUEUE_FAMILY_IGNORED;

    for (U32 i = 0; i < queue_family_count; ++i)
    {
        const LLVkQueueFamilyProperties& queue_family = queue_families[i];
        if (queue_family.queueCount == 0)
        {
            continue;
        }

        U32 supports_present = 0;
        if (get_surface_support(physical_device, i, context.mSurface, &supports_present) != LL_VK_SUCCESS)
        {
            supports_present = 0;
        }

        const bool supports_graphics = (queue_family.queueFlags & LL_VK_QUEUE_GRAPHICS_BIT) != 0;
        if (supports_graphics && supports_present)
        {
            context.mGraphicsQueueFamilyIndex = i;
            context.mPresentQueueFamilyIndex = i;
            return true;
        }

        if (supports_graphics && graphics_family == LL_VK_QUEUE_FAMILY_IGNORED)
        {
            graphics_family = i;
        }

        if (supports_present && present_family == LL_VK_QUEUE_FAMILY_IGNORED)
        {
            present_family = i;
        }
    }

    if (graphics_family != LL_VK_QUEUE_FAMILY_IGNORED &&
        present_family != LL_VK_QUEUE_FAMILY_IGNORED)
    {
        context.mGraphicsQueueFamilyIndex = graphics_family;
        context.mPresentQueueFamilyIndex = present_family;
        return true;
    }

    return false;
}

std::string get_vulkan_vendor_name(U32 vendor_id)
{
    switch (vendor_id)
    {
    case 0x1002:
    case 0x1022:
        return "AMD";
    case 0x1010:
        return "Imagination Technologies";
    case 0x106b:
        return "Apple";
    case 0x10de:
        return "NVIDIA";
    case 0x13b5:
        return "ARM";
    case 0x5143:
        return "Qualcomm";
    case 0x8086:
        return "Intel";
    default:
        break;
    }

    std::ostringstream stream;
    stream << "Vulkan vendor 0x"
        << std::uppercase
        << std::hex
        << std::setw(4)
        << std::setfill('0')
        << vendor_id;
    return stream.str();
}

void populate_vulkan_physical_device_info(
    LLVulkanNativeContext& context,
    LLVulkanGetPhysicalDeviceProperties get_physical_device_properties)
{
    context.mPhysicalDeviceVendor = "Vulkan";
    context.mPhysicalDeviceName = "Vulkan device";
    context.mMaxPushConstantsSize = 0;

    if (!get_physical_device_properties || !context.mPhysicalDevice)
    {
        return;
    }

    alignas(8) std::array<U8, 4096> properties_storage = {};
    get_physical_device_properties(
        context.mPhysicalDevice,
        properties_storage.data());

    const LLVkPhysicalDevicePropertiesHeader* properties =
        reinterpret_cast<const LLVkPhysicalDevicePropertiesHeader*>(properties_storage.data());
    context.mPhysicalDeviceVendor = get_vulkan_vendor_name(properties->vendorID);
    context.mMaxPushConstantsSize = properties->limits.maxPushConstantsSize;

    const char* name_begin = properties->deviceName;
    const char* name_end = static_cast<const char*>(
        std::memchr(
            name_begin,
            '\0',
            LL_VK_MAX_PHYSICAL_DEVICE_NAME_SIZE));
    context.mPhysicalDeviceName.assign(
        name_begin,
        name_end ? name_end - name_begin : LL_VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
    if (context.mPhysicalDeviceName.empty())
    {
        context.mPhysicalDeviceName = "Vulkan device";
    }
}

bool create_vulkan_device(LLVulkanNativeContext& context)
{
    const LLVulkanLoader& loader = get_vulkan_loader();

    LLVulkanGetPhysicalDeviceQueueFamilyProperties get_queue_family_properties =
        reinterpret_cast<LLVulkanGetPhysicalDeviceQueueFamilyProperties>(
            loader.getInstanceProcAddress(context.mInstance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    LLVulkanGetPhysicalDeviceProperties get_physical_device_properties =
        reinterpret_cast<LLVulkanGetPhysicalDeviceProperties>(
            loader.getInstanceProcAddress(context.mInstance, "vkGetPhysicalDeviceProperties"));
    LLVulkanGetPhysicalDeviceSurfaceSupportKHR get_surface_support =
        reinterpret_cast<LLVulkanGetPhysicalDeviceSurfaceSupportKHR>(
            loader.getInstanceProcAddress(context.mInstance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    LLVulkanEnumerateDeviceExtensionProperties enumerate_device_extension_properties =
        reinterpret_cast<LLVulkanEnumerateDeviceExtensionProperties>(
            loader.getInstanceProcAddress(context.mInstance, "vkEnumerateDeviceExtensionProperties"));
    LLVulkanCreateDevice create_device =
        reinterpret_cast<LLVulkanCreateDevice>(
            loader.getInstanceProcAddress(context.mInstance, "vkCreateDevice"));

    if (!get_queue_family_properties ||
        !get_surface_support ||
        !enumerate_device_extension_properties ||
        !create_device)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan instance is missing required logical-device entry points."
            << LL_ENDL;
        return false;
    }

    std::vector<const char*> enabled_device_extensions;
    context.mMemoryBudgetExtensionEnabled = false;
    for (LLVkPhysicalDevice physical_device : context.mPhysicalDevices)
    {
        std::vector<LLVkExtensionProperties> available_extensions =
            get_device_extensions(enumerate_device_extension_properties, physical_device);
        if (!has_vulkan_extension(available_extensions, LL_VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        {
            continue;
        }

        context.mGraphicsQueueFamilyIndex = LL_VK_QUEUE_FAMILY_IGNORED;
        context.mPresentQueueFamilyIndex = LL_VK_QUEUE_FAMILY_IGNORED;
        if (!find_vulkan_queue_families(
                context,
                physical_device,
                get_queue_family_properties,
                get_surface_support))
        {
            continue;
        }

        enabled_device_extensions.clear();
        enabled_device_extensions.push_back(LL_VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        if (has_vulkan_extension(available_extensions, LL_VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
        {
            enabled_device_extensions.push_back(LL_VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        }
        if (has_vulkan_extension(available_extensions, LL_VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
        {
            enabled_device_extensions.push_back(LL_VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
            context.mMemoryBudgetExtensionEnabled = true;
        }

        context.mPhysicalDevice = physical_device;
        break;
    }

    if (!context.mPhysicalDevice)
    {
        LL_WARNS("RenderBackend")
            << "No Vulkan physical device supports graphics, present, and swapchain requirements."
            << LL_ENDL;
        return false;
    }
    populate_vulkan_physical_device_info(context, get_physical_device_properties);

    const F32 queue_priority = 1.f;
    std::vector<LLVkDeviceQueueCreateInfo> queue_create_infos;
    queue_create_infos.push_back(
        LLVkDeviceQueueCreateInfo
        {
            LL_VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr,
            0,
            context.mGraphicsQueueFamilyIndex,
            1,
            &queue_priority
        });

    if (context.mPresentQueueFamilyIndex != context.mGraphicsQueueFamilyIndex)
    {
        queue_create_infos.push_back(
            LLVkDeviceQueueCreateInfo
            {
                LL_VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                nullptr,
                0,
                context.mPresentQueueFamilyIndex,
                1,
                &queue_priority
            });
    }

    LLVkDeviceCreateInfo create_info =
    {
        LL_VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        nullptr,
        0,
        static_cast<U32>(queue_create_infos.size()),
        queue_create_infos.data(),
        0,
        nullptr,
        static_cast<U32>(enabled_device_extensions.size()),
        enabled_device_extensions.data(),
        nullptr
    };

    S32 result = create_device(
        context.mPhysicalDevice,
        &create_info,
        nullptr,
        &context.mDevice);
    if (result != LL_VK_SUCCESS || !context.mDevice)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateDevice failed with result " << result
            << LL_ENDL;
        return false;
    }

    context.mDestroyDevice =
        reinterpret_cast<LLVulkanDestroyDevice>(
            get_vulkan_device_proc_address(context, "vkDestroyDevice"));
    context.mDeviceWaitIdle =
        reinterpret_cast<LLVulkanDeviceWaitIdle>(
            get_vulkan_device_proc_address(context, "vkDeviceWaitIdle"));
    context.mGetPhysicalDeviceMemoryProperties =
        reinterpret_cast<LLVulkanGetPhysicalDeviceMemoryProperties>(
            loader.getInstanceProcAddress(context.mInstance, "vkGetPhysicalDeviceMemoryProperties"));
    context.mGetPhysicalDeviceMemoryProperties2 =
        reinterpret_cast<LLVulkanGetPhysicalDeviceMemoryProperties2>(
            loader.getInstanceProcAddress(context.mInstance, "vkGetPhysicalDeviceMemoryProperties2"));
    if (!context.mGetPhysicalDeviceMemoryProperties2)
    {
        context.mGetPhysicalDeviceMemoryProperties2 =
            reinterpret_cast<LLVulkanGetPhysicalDeviceMemoryProperties2>(
                loader.getInstanceProcAddress(context.mInstance, "vkGetPhysicalDeviceMemoryProperties2KHR"));
    }
    context.mCreateBuffer =
        reinterpret_cast<LLVulkanCreateBuffer>(
            get_vulkan_device_proc_address(context, "vkCreateBuffer"));
    context.mDestroyBuffer =
        reinterpret_cast<LLVulkanDestroyBuffer>(
            get_vulkan_device_proc_address(context, "vkDestroyBuffer"));
    context.mGetBufferMemoryRequirements =
        reinterpret_cast<LLVulkanGetBufferMemoryRequirements>(
            get_vulkan_device_proc_address(context, "vkGetBufferMemoryRequirements"));
    context.mAllocateMemory =
        reinterpret_cast<LLVulkanAllocateMemory>(
            get_vulkan_device_proc_address(context, "vkAllocateMemory"));
    context.mFreeMemory =
        reinterpret_cast<LLVulkanFreeMemory>(
            get_vulkan_device_proc_address(context, "vkFreeMemory"));
    context.mBindBufferMemory =
        reinterpret_cast<LLVulkanBindBufferMemory>(
            get_vulkan_device_proc_address(context, "vkBindBufferMemory"));
    context.mBindImageMemory =
        reinterpret_cast<LLVulkanBindImageMemory>(
            get_vulkan_device_proc_address(context, "vkBindImageMemory"));
    context.mMapMemory =
        reinterpret_cast<LLVulkanMapMemory>(
            get_vulkan_device_proc_address(context, "vkMapMemory"));
    context.mUnmapMemory =
        reinterpret_cast<LLVulkanUnmapMemory>(
            get_vulkan_device_proc_address(context, "vkUnmapMemory"));
    LLVulkanGetDeviceQueue get_device_queue =
        reinterpret_cast<LLVulkanGetDeviceQueue>(
            get_vulkan_device_proc_address(context, "vkGetDeviceQueue"));

    if (!context.mDestroyDevice ||
        !context.mDeviceWaitIdle ||
        !context.mGetPhysicalDeviceMemoryProperties ||
        !context.mCreateBuffer ||
        !context.mDestroyBuffer ||
        !context.mGetBufferMemoryRequirements ||
        !context.mAllocateMemory ||
        !context.mFreeMemory ||
        !context.mBindBufferMemory ||
        !context.mBindImageMemory ||
        !context.mMapMemory ||
        !context.mUnmapMemory ||
        !get_device_queue)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan device is missing required queue/device/buffer entry points."
            << LL_ENDL;
        return false;
    }
    if (context.mMemoryBudgetExtensionEnabled && !context.mGetPhysicalDeviceMemoryProperties2)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan device exposes "
            << LL_VK_EXT_MEMORY_BUDGET_EXTENSION_NAME
            << " but the memory properties 2 entry point is unavailable; falling back to static memory caps."
            << LL_ENDL;
        context.mMemoryBudgetExtensionEnabled = false;
    }

    refresh_vulkan_memory_properties(context);
    log_vulkan_memory_properties(context);

    get_device_queue(context.mDevice, context.mGraphicsQueueFamilyIndex, 0, &context.mGraphicsQueue);
    get_device_queue(context.mDevice, context.mPresentQueueFamilyIndex, 0, &context.mPresentQueue);
    if (!context.mGraphicsQueue || !context.mPresentQueue)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan device did not return usable graphics/present queues."
            << LL_ENDL;
        return false;
    }

    LL_INFOS("RenderBackend")
        << "Vulkan logical device created for "
        << context.mPhysicalDeviceVendor
        << " "
        << context.mPhysicalDeviceName
        << ". Graphics queue family "
        << context.mGraphicsQueueFamilyIndex
        << ", present queue family "
        << context.mPresentQueueFamilyIndex
        << "."
        << LL_ENDL;
    if (context.mMaxPushConstantsSize)
    {
        LL_INFOS("RenderBackend")
            << "Vulkan max push constants size: "
            << context.mMaxPushConstantsSize
            << " bytes."
            << LL_ENDL;
    }
    return true;
}

void destroy_vulkan_device(LLVulkanNativeContext& context)
{
    context.mGraphicsQueue = nullptr;
    context.mPresentQueue = nullptr;

    if (context.mDestroyDevice && context.mDevice)
    {
        context.mDestroyDevice(context.mDevice, nullptr);
    }

    context.mDestroyDevice = nullptr;
    context.mDeviceWaitIdle = nullptr;
    context.mGetPhysicalDeviceMemoryProperties = nullptr;
    context.mGetPhysicalDeviceMemoryProperties2 = nullptr;
    context.mMemoryBudgetExtensionEnabled = false;
    context.mHasMemoryProperties = false;
    context.mHasMemoryBudget = false;
    context.mMemoryProperties = {};
    context.mMemoryHeapBudgetBytes = {};
    context.mMemoryHeapUsageBytes = {};
    context.mReportedVideoMemoryMB = 0;
    context.mCreateBuffer = nullptr;
    context.mDestroyBuffer = nullptr;
    context.mGetBufferMemoryRequirements = nullptr;
    context.mCreateImage = nullptr;
    context.mDestroyImage = nullptr;
    context.mGetImageMemoryRequirements = nullptr;
    context.mAllocateMemory = nullptr;
    context.mFreeMemory = nullptr;
    context.mBindBufferMemory = nullptr;
    context.mBindImageMemory = nullptr;
    context.mMapMemory = nullptr;
    context.mUnmapMemory = nullptr;
    context.mCreateSampler = nullptr;
    context.mDestroySampler = nullptr;
    context.mCmdCopyBufferToImage = nullptr;
    context.mCreateRenderPass = nullptr;
    context.mCreateFramebuffer = nullptr;
    context.mDestroyFramebuffer = nullptr;
    context.mDevice = nullptr;
    context.mPhysicalDevice = nullptr;
    context.mGraphicsQueueFamilyIndex = LL_VK_QUEUE_FAMILY_IGNORED;
    context.mPresentQueueFamilyIndex = LL_VK_QUEUE_FAMILY_IGNORED;
}

U32 clamp_vulkan_extent_dimension(U32 value, U32 min_value, U32 max_value)
{
    value = std::max(value, min_value);
    if (max_value > 0)
    {
        value = std::min(value, max_value);
    }
    return value;
}

LLVkExtent2D choose_vulkan_swapchain_extent(
    const LLVkSurfaceCapabilitiesKHR& capabilities,
    void* native_view)
{
    if (capabilities.currentExtent.width != LL_VK_EXTENT_UNDEFINED)
    {
        return capabilities.currentExtent;
    }

    LLVkExtent2D extent = capabilities.minImageExtent;
#if LL_DARWIN
    U32 drawable_width = 0;
    U32 drawable_height = 0;
    if (ll_render_macosx_get_metal_layer_drawable_size(native_view, &drawable_width, &drawable_height))
    {
        extent.width = drawable_width;
        extent.height = drawable_height;
    }
#endif

    extent.width = clamp_vulkan_extent_dimension(
        extent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    extent.height = clamp_vulkan_extent_dimension(
        extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);
    return extent;
}

LLVkSurfaceFormatKHR choose_vulkan_surface_format(
    const std::vector<LLVkSurfaceFormatKHR>& formats)
{
    if (formats.size() == 1 && formats.front().format == LL_VK_FORMAT_UNDEFINED)
    {
        return LLVkSurfaceFormatKHR
        {
            LL_VK_FORMAT_B8G8R8A8_UNORM,
            LL_VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        };
    }

    for (const LLVkSurfaceFormatKHR& format : formats)
    {
        if ((format.format == LL_VK_FORMAT_B8G8R8A8_UNORM ||
             format.format == LL_VK_FORMAT_B8G8R8A8_SRGB ||
             format.format == LL_VK_FORMAT_R8G8B8A8_UNORM ||
             format.format == LL_VK_FORMAT_R8G8B8A8_SRGB) &&
            format.colorSpace == LL_VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    return formats.front();
}

S32 choose_vulkan_present_mode(const std::vector<S32>& present_modes, bool enable_vsync)
{
    if (!enable_vsync)
    {
        for (S32 present_mode : present_modes)
        {
            if (present_mode == LL_VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return present_mode;
            }
        }

        for (S32 present_mode : present_modes)
        {
            if (present_mode == LL_VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                return present_mode;
            }
        }
    }

    return LL_VK_PRESENT_MODE_FIFO_KHR;
}

U32 choose_vulkan_composite_alpha(U32 supported_composite_alpha)
{
    constexpr std::array<U32, 4> preferred_modes =
    {
        LL_VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        LL_VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        LL_VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        LL_VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (U32 mode : preferred_modes)
    {
        if ((supported_composite_alpha & mode) != 0)
        {
            return mode;
        }
    }

    return LL_VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

std::vector<LLVkSurfaceFormatKHR> get_vulkan_surface_formats(
    LLVulkanGetPhysicalDeviceSurfaceFormatsKHR get_surface_formats,
    const LLVulkanNativeContext& context)
{
    std::vector<LLVkSurfaceFormatKHR> formats;
    U32 format_count = 0;
    if (get_surface_formats(context.mPhysicalDevice, context.mSurface, &format_count, nullptr) != LL_VK_SUCCESS ||
        format_count == 0)
    {
        return formats;
    }

    formats.resize(format_count);
    if (get_surface_formats(
            context.mPhysicalDevice,
            context.mSurface,
            &format_count,
            formats.data()) != LL_VK_SUCCESS)
    {
        formats.clear();
        return formats;
    }

    formats.resize(format_count);
    return formats;
}

std::vector<S32> get_vulkan_present_modes(
    LLVulkanGetPhysicalDeviceSurfacePresentModesKHR get_present_modes,
    const LLVulkanNativeContext& context)
{
    std::vector<S32> present_modes;
    U32 present_mode_count = 0;
    if (get_present_modes(context.mPhysicalDevice, context.mSurface, &present_mode_count, nullptr) != LL_VK_SUCCESS ||
        present_mode_count == 0)
    {
        return present_modes;
    }

    present_modes.resize(present_mode_count);
    if (get_present_modes(
            context.mPhysicalDevice,
            context.mSurface,
            &present_mode_count,
            present_modes.data()) != LL_VK_SUCCESS)
    {
        present_modes.clear();
        return present_modes;
    }

    present_modes.resize(present_mode_count);
    return present_modes;
}

bool create_vulkan_swapchain(
    LLVulkanNativeContext& context,
    void* native_view,
    bool enable_vsync)
{
    const LLVulkanLoader& loader = get_vulkan_loader();

    LLVulkanGetPhysicalDeviceSurfaceCapabilitiesKHR get_surface_capabilities =
        reinterpret_cast<LLVulkanGetPhysicalDeviceSurfaceCapabilitiesKHR>(
            loader.getInstanceProcAddress(context.mInstance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    LLVulkanGetPhysicalDeviceSurfaceFormatsKHR get_surface_formats =
        reinterpret_cast<LLVulkanGetPhysicalDeviceSurfaceFormatsKHR>(
            loader.getInstanceProcAddress(context.mInstance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    LLVulkanGetPhysicalDeviceSurfacePresentModesKHR get_present_modes =
        reinterpret_cast<LLVulkanGetPhysicalDeviceSurfacePresentModesKHR>(
            loader.getInstanceProcAddress(context.mInstance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    LLVulkanCreateSwapchainKHR create_swapchain =
        reinterpret_cast<LLVulkanCreateSwapchainKHR>(
            get_vulkan_device_proc_address(context, "vkCreateSwapchainKHR"));
    context.mDestroySwapchain =
        reinterpret_cast<LLVulkanDestroySwapchainKHR>(
            get_vulkan_device_proc_address(context, "vkDestroySwapchainKHR"));
    LLVulkanGetSwapchainImagesKHR get_swapchain_images =
        reinterpret_cast<LLVulkanGetSwapchainImagesKHR>(
            get_vulkan_device_proc_address(context, "vkGetSwapchainImagesKHR"));

    if (!get_surface_capabilities ||
        !get_surface_formats ||
        !get_present_modes ||
        !create_swapchain ||
        !context.mDestroySwapchain ||
        !get_swapchain_images)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend is missing required swapchain entry points."
            << LL_ENDL;
        return false;
    }

    LLVkSurfaceCapabilitiesKHR capabilities = {};
    S32 result = get_surface_capabilities(
        context.mPhysicalDevice,
        context.mSurface,
        &capabilities);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    if ((capabilities.supportedUsageFlags & LL_VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan surface does not support color-attachment swapchain images."
            << LL_ENDL;
        return false;
    }

    std::vector<LLVkSurfaceFormatKHR> surface_formats =
        get_vulkan_surface_formats(get_surface_formats, context);
    std::vector<S32> present_modes =
        get_vulkan_present_modes(get_present_modes, context);
    if (surface_formats.empty() || present_modes.empty())
    {
        LL_WARNS("RenderBackend")
            << "Vulkan surface has no usable formats or present modes."
            << LL_ENDL;
        return false;
    }

    LLVkSurfaceFormatKHR surface_format = choose_vulkan_surface_format(surface_formats);
    LLVkExtent2D swapchain_extent =
        choose_vulkan_swapchain_extent(capabilities, native_view);
    LLVkExtent2D native_view_extent = swapchain_extent;
#if LL_DARWIN
    U32 native_view_width = 0;
    U32 native_view_height = 0;
    if (ll_render_macosx_get_native_view_size(native_view, &native_view_width, &native_view_height))
    {
        native_view_extent.width = native_view_width;
        native_view_extent.height = native_view_height;
    }
#endif
    U32 image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
    {
        image_count = capabilities.maxImageCount;
    }

    std::array<U32, 2> queue_family_indices =
    {
        context.mGraphicsQueueFamilyIndex,
        context.mPresentQueueFamilyIndex
    };
    const bool concurrent_sharing =
        context.mGraphicsQueueFamilyIndex != context.mPresentQueueFamilyIndex;

    LLVkSwapchainCreateInfoKHR create_info =
    {
        LL_VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        0,
        context.mSurface,
        image_count,
        surface_format.format,
        surface_format.colorSpace,
        swapchain_extent,
        1,
        LL_VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        concurrent_sharing ? LL_VK_SHARING_MODE_CONCURRENT : LL_VK_SHARING_MODE_EXCLUSIVE,
        concurrent_sharing ? static_cast<U32>(queue_family_indices.size()) : 0,
        concurrent_sharing ? queue_family_indices.data() : nullptr,
        capabilities.currentTransform,
        choose_vulkan_composite_alpha(capabilities.supportedCompositeAlpha),
        choose_vulkan_present_mode(present_modes, enable_vsync),
        1,
        nullptr
    };

    result = create_swapchain(
        context.mDevice,
        &create_info,
        nullptr,
        &context.mSwapchain);
    if (result != LL_VK_SUCCESS || !context.mSwapchain)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateSwapchainKHR failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    U32 swapchain_image_count = 0;
    result = get_swapchain_images(
        context.mDevice,
        context.mSwapchain,
        &swapchain_image_count,
        nullptr);
    if (result != LL_VK_SUCCESS || swapchain_image_count == 0)
    {
        LL_WARNS("RenderBackend")
            << "vkGetSwapchainImagesKHR failed or returned no images, result "
            << result
            << LL_ENDL;
        return false;
    }

    context.mSwapchainImages.resize(swapchain_image_count);
    result = get_swapchain_images(
        context.mDevice,
        context.mSwapchain,
        &swapchain_image_count,
        context.mSwapchainImages.data());
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkGetSwapchainImagesKHR failed while fetching images, result "
            << result
            << LL_ENDL;
        context.mSwapchainImages.clear();
        return false;
    }

    context.mSwapchainImages.resize(swapchain_image_count);
    context.mSwapchainExtent = swapchain_extent;
    context.mNativeViewExtent = native_view_extent;
    context.mDrawableScaleX = context.mNativeViewExtent.width > 0 ?
        static_cast<F32>(context.mSwapchainExtent.width) /
            static_cast<F32>(context.mNativeViewExtent.width) :
        1.f;
    context.mDrawableScaleY = context.mNativeViewExtent.height > 0 ?
        static_cast<F32>(context.mSwapchainExtent.height) /
            static_cast<F32>(context.mNativeViewExtent.height) :
        1.f;
    context.mSwapchainImageFormat = surface_format.format;

    LL_INFOS("RenderBackend")
        << "Vulkan swapchain created with "
        << context.mSwapchainImages.size()
        << " image(s), extent "
        << context.mSwapchainExtent.width
        << "x"
        << context.mSwapchainExtent.height
        << ", native view extent "
        << context.mNativeViewExtent.width
        << "x"
        << context.mNativeViewExtent.height
        << ", drawable scale "
        << context.mDrawableScaleX
        << "x"
        << context.mDrawableScaleY
        << ", format "
        << context.mSwapchainImageFormat
        << "."
        << LL_ENDL;
    return true;
}

bool create_vulkan_swapchain_image_views(LLVulkanNativeContext& context)
{
    context.mCreateImageView =
        reinterpret_cast<LLVulkanCreateImageView>(
            get_vulkan_device_proc_address(context, "vkCreateImageView"));
    context.mDestroyImageView =
        reinterpret_cast<LLVulkanDestroyImageView>(
            get_vulkan_device_proc_address(context, "vkDestroyImageView"));

    if (!context.mCreateImageView || !context.mDestroyImageView)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend is missing required image-view entry points."
            << LL_ENDL;
        return false;
    }

    context.mSwapchainImageViews.reserve(context.mSwapchainImages.size());
    for (LLVkImage image : context.mSwapchainImages)
    {
        LLVkImageView image_view = nullptr;
        LLVkImageViewCreateInfo create_info =
        {
            LL_VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            image,
            LL_VK_IMAGE_VIEW_TYPE_2D,
            context.mSwapchainImageFormat,
            LLVkComponentMapping
            {
                LL_VK_COMPONENT_SWIZZLE_IDENTITY,
                LL_VK_COMPONENT_SWIZZLE_IDENTITY,
                LL_VK_COMPONENT_SWIZZLE_IDENTITY,
                LL_VK_COMPONENT_SWIZZLE_IDENTITY
            },
            LLVkImageSubresourceRange
            {
                LL_VK_IMAGE_ASPECT_COLOR_BIT,
                0,
                1,
                0,
                1
            }
        };

        S32 result = context.mCreateImageView(
            context.mDevice,
            &create_info,
            nullptr,
            &image_view);
        if (result != LL_VK_SUCCESS || !image_view)
        {
            LL_WARNS("RenderBackend")
                << "vkCreateImageView failed with result "
                << result
                << LL_ENDL;
            return false;
        }

        context.mSwapchainImageViews.push_back(image_view);
    }

    LL_INFOS("RenderBackend")
        << "Vulkan swapchain image views created: "
        << context.mSwapchainImageViews.size()
        << "."
        << LL_ENDL;
    return true;
}

void destroy_vulkan_swapchain_image_views(LLVulkanNativeContext& context)
{
    if (context.mDestroyImageView && context.mDevice)
    {
        for (LLVkImageView image_view : context.mSwapchainImageViews)
        {
            if (image_view)
            {
                context.mDestroyImageView(context.mDevice, image_view, nullptr);
            }
        }
    }

    context.mSwapchainImageViews.clear();
    context.mCreateImageView = nullptr;
    context.mDestroyImageView = nullptr;
}

bool create_vulkan_render_pass(LLVulkanNativeContext& context)
{
    context.mCreateRenderPass =
        reinterpret_cast<LLVulkanCreateRenderPass>(
            get_vulkan_device_proc_address(context, "vkCreateRenderPass"));
    context.mDestroyRenderPass =
        reinterpret_cast<LLVulkanDestroyRenderPass>(
            get_vulkan_device_proc_address(context, "vkDestroyRenderPass"));

    if (!context.mCreateRenderPass || !context.mDestroyRenderPass)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend is missing required render-pass entry points."
            << LL_ENDL;
        return false;
    }

    auto create_single_color_depth_render_pass =
        [&](S32 color_final_layout, LLVkRenderPass& render_pass) -> bool
    {
        LLVkAttachmentDescription color_attachment =
        {
            0,
            context.mSwapchainImageFormat,
            LL_VK_SAMPLE_COUNT_1_BIT,
            LL_VK_ATTACHMENT_LOAD_OP_CLEAR,
            LL_VK_ATTACHMENT_STORE_OP_STORE,
            LL_VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            LL_VK_ATTACHMENT_STORE_OP_DONT_CARE,
            LL_VK_IMAGE_LAYOUT_UNDEFINED,
            color_final_layout
        };
        LLVkAttachmentDescription depth_attachment =
        {
            0,
            LL_VK_FORMAT_D32_SFLOAT,
            LL_VK_SAMPLE_COUNT_1_BIT,
            LL_VK_ATTACHMENT_LOAD_OP_CLEAR,
            LL_VK_ATTACHMENT_STORE_OP_DONT_CARE,
            LL_VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            LL_VK_ATTACHMENT_STORE_OP_DONT_CARE,
            LL_VK_IMAGE_LAYOUT_UNDEFINED,
            LL_VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };
        LLVkAttachmentDescription attachments[2] =
        {
            color_attachment,
            depth_attachment
        };

        LLVkAttachmentReference color_attachment_reference =
        {
            0,
            LL_VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };
        LLVkAttachmentReference depth_attachment_reference =
        {
            1,
            LL_VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        LLVkSubpassDescription subpass =
        {
            0,
            LL_VK_PIPELINE_BIND_POINT_GRAPHICS,
            0,
            nullptr,
            1,
            &color_attachment_reference,
            nullptr,
            &depth_attachment_reference,
            0,
            nullptr
        };

        LLVkSubpassDependency dependencies[2] =
        {
            LLVkSubpassDependency
            {
                LL_VK_SUBPASS_EXTERNAL,
                0,
                LL_VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                    LL_VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                LL_VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                    LL_VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                0,
                LL_VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                    LL_VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                0
            },
            LLVkSubpassDependency
            {
                0,
                LL_VK_SUBPASS_EXTERNAL,
                LL_VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                    LL_VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                LL_VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                LL_VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                    LL_VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                LL_VK_ACCESS_SHADER_READ_BIT,
                0
            }
        };

        LLVkRenderPassCreateInfo create_info =
        {
            LL_VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            nullptr,
            0,
            2,
            attachments,
            1,
            &subpass,
            2,
            dependencies
        };

        S32 result = context.mCreateRenderPass(
            context.mDevice,
            &create_info,
            nullptr,
            &render_pass);
        if (result != LL_VK_SUCCESS || !render_pass)
        {
            LL_WARNS("RenderBackend")
                << "vkCreateRenderPass failed with result "
                << result
                << LL_ENDL;
            return false;
        }

        return true;
    };

    if (!create_single_color_depth_render_pass(
            LL_VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            context.mRenderPass))
    {
        return false;
    }

    if (!create_single_color_depth_render_pass(
            LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            context.mOffscreenRenderPass))
    {
        return false;
    }

    LL_INFOS("RenderBackend")
        << "Vulkan swapchain and offscreen render passes created."
        << LL_ENDL;
    return true;
}

LLVkRenderPass get_vulkan_offscreen_render_pass(
    LLVulkanNativeContext& context,
    const std::array<S32, 4>& color_formats,
    U32 color_format_count,
    S32 depth_format)
{
    color_format_count = llclamp(color_format_count, 0U, static_cast<U32>(color_formats.size()));
    if (!context.mCreateRenderPass ||
        !context.mDestroyRenderPass ||
        color_format_count == 0)
    {
        return nullptr;
    }

    const U64 key =
        make_vulkan_offscreen_render_pass_key(color_formats, color_format_count, depth_format);
    if (color_format_count == 1 &&
        color_formats[0] == context.mSwapchainImageFormat &&
        depth_format == LL_VK_FORMAT_D32_SFLOAT &&
        context.mOffscreenRenderPass)
    {
        return context.mOffscreenRenderPass;
    }

    auto existing_iter = context.mOffscreenRenderPasses.find(key);
    if (existing_iter != context.mOffscreenRenderPasses.end())
    {
        return existing_iter->second;
    }

    std::array<LLVkAttachmentDescription, 5> attachments = {};
    std::array<LLVkAttachmentReference, 4> color_attachment_references = {};
    for (U32 i = 0; i < color_format_count; ++i)
    {
        attachments[i] =
        {
            0,
            color_formats[i],
            LL_VK_SAMPLE_COUNT_1_BIT,
            LL_VK_ATTACHMENT_LOAD_OP_CLEAR,
            LL_VK_ATTACHMENT_STORE_OP_STORE,
            LL_VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            LL_VK_ATTACHMENT_STORE_OP_DONT_CARE,
            LL_VK_IMAGE_LAYOUT_UNDEFINED,
            LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        color_attachment_references[i] =
        {
            i,
            LL_VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };
    }

    attachments[color_format_count] =
    {
        0,
        depth_format,
        LL_VK_SAMPLE_COUNT_1_BIT,
        LL_VK_ATTACHMENT_LOAD_OP_CLEAR,
        LL_VK_ATTACHMENT_STORE_OP_DONT_CARE,
        LL_VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        LL_VK_ATTACHMENT_STORE_OP_DONT_CARE,
        LL_VK_IMAGE_LAYOUT_UNDEFINED,
        LL_VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    LLVkAttachmentReference depth_attachment_reference =
    {
        color_format_count,
        LL_VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    LLVkSubpassDescription subpass =
    {
        0,
        LL_VK_PIPELINE_BIND_POINT_GRAPHICS,
        0,
        nullptr,
        color_format_count,
        color_attachment_references.data(),
        nullptr,
        &depth_attachment_reference,
        0,
        nullptr
    };
    LLVkSubpassDependency dependencies[2] =
    {
        LLVkSubpassDependency
        {
            LL_VK_SUBPASS_EXTERNAL,
            0,
            LL_VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                LL_VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            LL_VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                LL_VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            LL_VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                LL_VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            0
        },
        LLVkSubpassDependency
        {
            0,
            LL_VK_SUBPASS_EXTERNAL,
            LL_VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                LL_VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            LL_VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            LL_VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                LL_VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            LL_VK_ACCESS_SHADER_READ_BIT,
            0
        }
    };

    LLVkRenderPassCreateInfo create_info =
    {
        LL_VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
        0,
        color_format_count + 1,
        attachments.data(),
        1,
        &subpass,
        2,
        dependencies
    };

    LLVkRenderPass render_pass = nullptr;
    S32 result = context.mCreateRenderPass(
        context.mDevice,
        &create_info,
        nullptr,
        &render_pass);
    if (result != LL_VK_SUCCESS || !render_pass)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateRenderPass(offscreen color attachments "
            << color_format_count
            << ") failed with result "
            << result
            << LL_ENDL;
        return nullptr;
    }

    context.mOffscreenRenderPasses.emplace(key, render_pass);
    return render_pass;
}

void destroy_vulkan_render_pass(LLVulkanNativeContext& context)
{
    if (context.mDestroyRenderPass && context.mDevice)
    {
        for (const auto& entry : context.mOffscreenRenderPasses)
        {
            if (entry.second)
            {
                context.mDestroyRenderPass(context.mDevice, entry.second, nullptr);
            }
        }
    }
    context.mOffscreenRenderPasses.clear();

    if (context.mDestroyRenderPass && context.mDevice && context.mRenderPass)
    {
        context.mDestroyRenderPass(context.mDevice, context.mRenderPass, nullptr);
    }
    if (context.mDestroyRenderPass && context.mDevice && context.mOffscreenRenderPass)
    {
        context.mDestroyRenderPass(context.mDevice, context.mOffscreenRenderPass, nullptr);
    }

    context.mCreateRenderPass = nullptr;
    context.mDestroyRenderPass = nullptr;
    context.mRenderPass = nullptr;
    context.mOffscreenRenderPass = nullptr;
}

bool create_vulkan_shader_module(
    LLVulkanNativeContext& context,
    LLVulkanCreateShaderModule create_shader_module,
    const U32* code,
    size_t code_size,
    const char* label,
    LLVkShaderModule& shader_module)
{
    shader_module = nullptr;
    LLVkShaderModuleCreateInfo create_info =
    {
        LL_VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        nullptr,
        0,
        code_size,
        code
    };

    S32 result = create_shader_module(
        context.mDevice,
        &create_info,
        nullptr,
        &shader_module);
    if (result != LL_VK_SUCCESS || !shader_module)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateShaderModule(" << label << ") failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    return true;
}

void add_unique_vulkan_shader_directory(
    std::vector<std::string>& directories,
    const std::string& directory)
{
    if (!directory.empty() &&
        std::find(directories.begin(), directories.end(), directory) == directories.end())
    {
        directories.push_back(directory);
    }
}

std::vector<std::string> get_vulkan_final_shader_directories()
{
    std::vector<std::string> directories;

    if (const char* override_dir = std::getenv("MARE_VULKAN_SHADER_DIR"))
    {
        if (override_dir[0])
        {
            add_unique_vulkan_shader_directory(
                directories,
                gDirUtilp ?
                    gDirUtilp->add(override_dir, "final") :
                    std::string(override_dir) + "/final");
            add_unique_vulkan_shader_directory(directories, override_dir);
        }
    }

    if (MARE_VULKAN_FINAL_SHADER_DIR[0])
    {
        add_unique_vulkan_shader_directory(directories, MARE_VULKAN_FINAL_SHADER_DIR);
    }

    if (gDirUtilp)
    {
        add_unique_vulkan_shader_directory(
            directories,
            gDirUtilp->getExpandedFilename(
                LL_PATH_APP_SETTINGS,
                "shaders/vulkan",
                "final"));
    }

    return directories;
}

bool load_vulkan_shader_spirv_file(
    const std::string& shader_path,
    const char* label,
    std::vector<U32>& spirv)
{
    llifstream input(shader_path, std::ios::binary | std::ios::ate);
    if (!input)
    {
        LL_WARNS("RenderBackend")
            << "Unable to open Vulkan " << label << " shader: "
            << shader_path
            << LL_ENDL;
        return false;
    }

    const std::streamoff size = input.tellg();
    if (size <= 0 || (size % sizeof(U32)) != 0)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan " << label << " shader has invalid SPIR-V byte size "
            << size
            << ": "
            << shader_path
            << LL_ENDL;
        return false;
    }

    spirv.resize(static_cast<size_t>(size) / sizeof(U32));
    input.seekg(0, std::ios::beg);
    input.read(
        reinterpret_cast<char*>(spirv.data()),
        static_cast<std::streamsize>(size));
    if (!input)
    {
        LL_WARNS("RenderBackend")
            << "Unable to read Vulkan " << label << " shader: "
            << shader_path
            << LL_ENDL;
        spirv.clear();
        return false;
    }

    return true;
}

std::vector<std::pair<std::string, std::string>> discover_vulkan_final_shader_spirv()
{
    for (const std::string& directory : get_vulkan_final_shader_directories())
    {
        if (!LLFile::isdir(directory))
        {
            continue;
        }

        std::vector<std::pair<std::string, std::string>> shaders;
        std::error_code ec;
        std::filesystem::path root(directory);
        std::filesystem::recursive_directory_iterator iter(
            root,
            std::filesystem::directory_options::skip_permission_denied,
            ec);
        std::filesystem::recursive_directory_iterator end;
        for (; iter != end; iter.increment(ec))
        {
            if (ec)
            {
                ec.clear();
                continue;
            }

            std::error_code file_ec;
            if (!iter->is_regular_file(file_ec) ||
                iter->path().extension() != ".spv")
            {
                continue;
            }

            std::filesystem::path relative = iter->path().lexically_relative(root);
            shaders.emplace_back(
                relative.generic_string(),
                iter->path().string());
        }

        if (!shaders.empty())
        {
            std::sort(shaders.begin(), shaders.end());
            return shaders;
        }
    }

    return {};
}

void destroy_vulkan_final_shader_modules(LLVulkanNativeContext& context)
{
    if (context.mDestroyShaderModule && context.mDevice)
    {
        for (auto& shader : context.mFinalShaderModules)
        {
            if (shader.second.mModule)
            {
                context.mDestroyShaderModule(
                    context.mDevice,
                    shader.second.mModule,
                    nullptr);
            }
        }
    }

    context.mFinalShaderModules.clear();
}

bool create_vulkan_final_shader_modules(
    LLVulkanNativeContext& context,
    LLVulkanCreateShaderModule create_shader_module)
{
    const std::vector<std::pair<std::string, std::string>> shader_paths =
        discover_vulkan_final_shader_spirv();
    if (shader_paths.empty())
    {
        LL_WARNS("RenderBackend")
            << "No Vulkan final SPIR-V shaders were found."
            << LL_ENDL;
        return false;
    }

    for (const auto& shader_path : shader_paths)
    {
        std::vector<U32> spirv;
        if (!load_vulkan_shader_spirv_file(
                shader_path.second,
                shader_path.first.c_str(),
                spirv))
        {
            destroy_vulkan_final_shader_modules(context);
            return false;
        }

        LLVkShaderModule shader_module = nullptr;
        if (!create_vulkan_shader_module(
                context,
                create_shader_module,
                spirv.data(),
                spirv.size() * sizeof(U32),
                shader_path.first.c_str(),
                shader_module))
        {
            destroy_vulkan_final_shader_modules(context);
            return false;
        }

        context.mFinalShaderModules.emplace(
            shader_path.first,
            LLVulkanFinalShaderModule { shader_module, shader_path.second });
    }

    LL_INFOS("RenderBackend")
        << "Loaded "
        << context.mFinalShaderModules.size()
        << " Vulkan final shader modules."
        << LL_ENDL;
    return true;
}

LLVkShaderModule get_vulkan_final_shader_module(
    LLVulkanNativeContext& context,
    const std::string& shader_name,
    const char* label)
{
    auto iter = context.mFinalShaderModules.find(shader_name);
    if (iter == context.mFinalShaderModules.end() || !iter->second.mModule)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan final " << label << " shader module is missing: "
            << shader_name
            << LL_ENDL;
        return nullptr;
    }

    return iter->second.mModule;
}

void destroy_vulkan_graphics_pipelines(LLVulkanNativeContext& context)
{
    if (context.mDestroyPipeline && context.mDevice && context.mBootstrapPipeline)
    {
        context.mDestroyPipeline(context.mDevice, context.mBootstrapPipeline, nullptr);
    }

    if (context.mDestroyPipeline && context.mDevice)
    {
        for (LLVkPipeline pipeline : context.mUIPipelines)
        {
            if (pipeline)
            {
                context.mDestroyPipeline(context.mDevice, pipeline, nullptr);
            }
        }
        for (LLVkPipeline pipeline : context.mWorldPipelines)
        {
            if (pipeline)
            {
                context.mDestroyPipeline(context.mDevice, pipeline, nullptr);
            }
        }
        for (LLVkPipeline pipeline : context.mTerrainPipelines)
        {
            if (pipeline)
            {
                context.mDestroyPipeline(context.mDevice, pipeline, nullptr);
            }
        }
    }

    if (context.mDestroyPipelineLayout && context.mDevice && context.mBootstrapPipelineLayout)
    {
        context.mDestroyPipelineLayout(context.mDevice, context.mBootstrapPipelineLayout, nullptr);
    }

    if (context.mDestroyPipelineLayout && context.mDevice && context.mUIPipelineLayout)
    {
        context.mDestroyPipelineLayout(context.mDevice, context.mUIPipelineLayout, nullptr);
    }

    if (context.mDestroyPipelineLayout && context.mDevice && context.mWorldPipelineLayout)
    {
        context.mDestroyPipelineLayout(context.mDevice, context.mWorldPipelineLayout, nullptr);
    }

    destroy_vulkan_texture_descriptor_set_cache(context);

    if (context.mDestroyDescriptorPool && context.mDevice && context.mUIDescriptorPool)
    {
        context.mDestroyDescriptorPool(context.mDevice, context.mUIDescriptorPool, nullptr);
    }

    if (context.mDestroyDescriptorSetLayout && context.mDevice && context.mUIDescriptorSetLayout)
    {
        context.mDestroyDescriptorSetLayout(context.mDevice, context.mUIDescriptorSetLayout, nullptr);
    }

    if (context.mDestroyShaderModule && context.mDevice)
    {
        std::unordered_set<LLVkShaderModule> destroyed_modules;
        auto destroy_shader_module_once = [&](LLVkShaderModule shader_module)
        {
            if (shader_module && destroyed_modules.insert(shader_module).second)
            {
                context.mDestroyShaderModule(context.mDevice, shader_module, nullptr);
            }
        };

        for (const auto& shader : context.mFinalShaderModules)
        {
            destroy_shader_module_once(shader.second.mModule);
        }
        context.mFinalShaderModules.clear();

        if (context.mBootstrapVertexShader)
        {
            destroy_shader_module_once(context.mBootstrapVertexShader);
        }
        if (context.mBootstrapFragmentShader)
        {
            destroy_shader_module_once(context.mBootstrapFragmentShader);
        }
        if (context.mUIVertexShader)
        {
            destroy_shader_module_once(context.mUIVertexShader);
        }
        if (context.mUIFragmentShader)
        {
            destroy_shader_module_once(context.mUIFragmentShader);
        }
        if (context.mWorldVertexShader)
        {
            destroy_shader_module_once(context.mWorldVertexShader);
        }
        if (context.mWorldFragmentShader)
        {
            destroy_shader_module_once(context.mWorldFragmentShader);
        }
        if (context.mTerrainVertexShader)
        {
            destroy_shader_module_once(context.mTerrainVertexShader);
        }
        if (context.mTerrainFragmentShader)
        {
            destroy_shader_module_once(context.mTerrainFragmentShader);
        }
    }

    context.mBootstrapPipeline = nullptr;
    context.mUIPipelines = {};
    context.mWorldPipelines = {};
    context.mTerrainPipelines = {};
    context.mBootstrapPipelineLayout = nullptr;
    context.mUIPipelineLayout = nullptr;
    context.mWorldPipelineLayout = nullptr;
    context.mUIDescriptorSetLayout = nullptr;
    context.mUIDescriptorPool = nullptr;
    context.mBootstrapVertexShader = nullptr;
    context.mBootstrapFragmentShader = nullptr;
    context.mUIVertexShader = nullptr;
    context.mUIFragmentShader = nullptr;
    context.mWorldVertexShader = nullptr;
    context.mWorldFragmentShader = nullptr;
    context.mTerrainVertexShader = nullptr;
    context.mTerrainFragmentShader = nullptr;
    context.mDestroyPipeline = nullptr;
    context.mDestroyPipelineLayout = nullptr;
    context.mDestroyShaderModule = nullptr;
    context.mCmdBindPipeline = nullptr;
    context.mCmdDraw = nullptr;
    context.mCmdBindIndexBuffer = nullptr;
    context.mCmdDrawIndexed = nullptr;
    context.mCmdBindVertexBuffers = nullptr;
    context.mCmdSetViewport = nullptr;
    context.mCmdSetScissor = nullptr;
    context.mCmdBindDescriptorSets = nullptr;
    context.mCmdPushConstants = nullptr;
    context.mDestroyDescriptorSetLayout = nullptr;
    context.mDestroyDescriptorPool = nullptr;
    context.mAllocateDescriptorSets = nullptr;
    context.mFreeDescriptorSets = nullptr;
    context.mUpdateDescriptorSets = nullptr;
}

LLVkPipelineColorBlendAttachmentState make_vulkan_world_color_blend_attachment(
    LLVulkanWorldBlendPipeline blend_pipeline)
{
    LLVkPipelineColorBlendAttachmentState attachment =
    {
        0,
        LL_VK_BLEND_FACTOR_ONE,
        LL_VK_BLEND_FACTOR_ZERO,
        LL_VK_BLEND_OP_ADD,
        LL_VK_BLEND_FACTOR_ONE,
        LL_VK_BLEND_FACTOR_ZERO,
        LL_VK_BLEND_OP_ADD,
        LL_VK_COLOR_COMPONENT_R_BIT |
            LL_VK_COLOR_COMPONENT_G_BIT |
            LL_VK_COLOR_COMPONENT_B_BIT |
            LL_VK_COLOR_COMPONENT_A_BIT
    };

    if (blend_pipeline == LLVulkanWorldBlendPipeline::Alpha)
    {
        attachment.blendEnable = 1;
        attachment.srcColorBlendFactor = LL_VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.dstColorBlendFactor = LL_VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.srcAlphaBlendFactor = LL_VK_BLEND_FACTOR_ONE;
        attachment.dstAlphaBlendFactor = LL_VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    }
    else if (blend_pipeline == LLVulkanWorldBlendPipeline::Add)
    {
        attachment.blendEnable = 1;
        attachment.srcColorBlendFactor = LL_VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.dstColorBlendFactor = LL_VK_BLEND_FACTOR_ONE;
        attachment.srcAlphaBlendFactor = LL_VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.dstAlphaBlendFactor = LL_VK_BLEND_FACTOR_ONE;
    }

    return attachment;
}

LLVkPipelineDepthStencilStateCreateInfo make_vulkan_depth_stencil_state(
    LLVulkanWorldDepthPipeline depth_pipeline)
{
    LLVkPipelineDepthStencilStateCreateInfo state =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        nullptr,
        0,
        0,
        0,
        LL_VK_COMPARE_OP_ALWAYS,
        0,
        0,
        {},
        {},
        0.f,
        1.f
    };

    if (depth_pipeline != LLVulkanWorldDepthPipeline::Disabled)
    {
        state.depthTestEnable = 1;
        state.depthWriteEnable = depth_pipeline == LLVulkanWorldDepthPipeline::ReadWrite ? 1 : 0;
        state.depthCompareOp = LL_VK_COMPARE_OP_LESS_OR_EQUAL;
    }

    return state;
}

bool create_vulkan_graphics_pipelines(LLVulkanNativeContext& context)
{
    LLVulkanCreateShaderModule create_shader_module =
        reinterpret_cast<LLVulkanCreateShaderModule>(
            get_vulkan_device_proc_address(context, "vkCreateShaderModule"));
    context.mDestroyShaderModule =
        reinterpret_cast<LLVulkanDestroyShaderModule>(
            get_vulkan_device_proc_address(context, "vkDestroyShaderModule"));
    LLVulkanCreatePipelineLayout create_pipeline_layout =
        reinterpret_cast<LLVulkanCreatePipelineLayout>(
            get_vulkan_device_proc_address(context, "vkCreatePipelineLayout"));
    context.mDestroyPipelineLayout =
        reinterpret_cast<LLVulkanDestroyPipelineLayout>(
            get_vulkan_device_proc_address(context, "vkDestroyPipelineLayout"));
    LLVulkanCreateGraphicsPipelines create_graphics_pipelines =
        reinterpret_cast<LLVulkanCreateGraphicsPipelines>(
            get_vulkan_device_proc_address(context, "vkCreateGraphicsPipelines"));
    context.mDestroyPipeline =
        reinterpret_cast<LLVulkanDestroyPipeline>(
            get_vulkan_device_proc_address(context, "vkDestroyPipeline"));
    context.mCmdBindPipeline =
        reinterpret_cast<LLVulkanCmdBindPipeline>(
            get_vulkan_device_proc_address(context, "vkCmdBindPipeline"));
    context.mCmdDraw =
        reinterpret_cast<LLVulkanCmdDraw>(
            get_vulkan_device_proc_address(context, "vkCmdDraw"));
    context.mCmdBindIndexBuffer =
        reinterpret_cast<LLVulkanCmdBindIndexBuffer>(
            get_vulkan_device_proc_address(context, "vkCmdBindIndexBuffer"));
    context.mCmdDrawIndexed =
        reinterpret_cast<LLVulkanCmdDrawIndexed>(
            get_vulkan_device_proc_address(context, "vkCmdDrawIndexed"));
    context.mCmdBindVertexBuffers =
        reinterpret_cast<LLVulkanCmdBindVertexBuffers>(
            get_vulkan_device_proc_address(context, "vkCmdBindVertexBuffers"));
    context.mCmdSetViewport =
        reinterpret_cast<LLVulkanCmdSetViewport>(
            get_vulkan_device_proc_address(context, "vkCmdSetViewport"));
    context.mCmdSetScissor =
        reinterpret_cast<LLVulkanCmdSetScissor>(
            get_vulkan_device_proc_address(context, "vkCmdSetScissor"));
    context.mCreateDescriptorSetLayout =
        reinterpret_cast<LLVulkanCreateDescriptorSetLayout>(
            get_vulkan_device_proc_address(context, "vkCreateDescriptorSetLayout"));
    context.mDestroyDescriptorSetLayout =
        reinterpret_cast<LLVulkanDestroyDescriptorSetLayout>(
            get_vulkan_device_proc_address(context, "vkDestroyDescriptorSetLayout"));
    context.mCreateDescriptorPool =
        reinterpret_cast<LLVulkanCreateDescriptorPool>(
            get_vulkan_device_proc_address(context, "vkCreateDescriptorPool"));
    context.mDestroyDescriptorPool =
        reinterpret_cast<LLVulkanDestroyDescriptorPool>(
            get_vulkan_device_proc_address(context, "vkDestroyDescriptorPool"));
    context.mAllocateDescriptorSets =
        reinterpret_cast<LLVulkanAllocateDescriptorSets>(
            get_vulkan_device_proc_address(context, "vkAllocateDescriptorSets"));
    context.mFreeDescriptorSets =
        reinterpret_cast<LLVulkanFreeDescriptorSets>(
            get_vulkan_device_proc_address(context, "vkFreeDescriptorSets"));
    context.mUpdateDescriptorSets =
        reinterpret_cast<LLVulkanUpdateDescriptorSets>(
            get_vulkan_device_proc_address(context, "vkUpdateDescriptorSets"));
    context.mCmdBindDescriptorSets =
        reinterpret_cast<LLVulkanCmdBindDescriptorSets>(
            get_vulkan_device_proc_address(context, "vkCmdBindDescriptorSets"));
    context.mCmdPushConstants =
        reinterpret_cast<LLVulkanCmdPushConstants>(
            get_vulkan_device_proc_address(context, "vkCmdPushConstants"));

    if (!create_shader_module ||
        !context.mDestroyShaderModule ||
        !create_pipeline_layout ||
        !context.mDestroyPipelineLayout ||
        !create_graphics_pipelines ||
        !context.mDestroyPipeline ||
        !context.mCmdBindPipeline ||
        !context.mCmdDraw ||
        !context.mCmdBindIndexBuffer ||
        !context.mCmdDrawIndexed ||
        !context.mCmdBindVertexBuffers ||
        !context.mCmdSetViewport ||
        !context.mCmdSetScissor ||
        !context.mCreateDescriptorSetLayout ||
        !context.mDestroyDescriptorSetLayout ||
        !context.mCreateDescriptorPool ||
        !context.mDestroyDescriptorPool ||
        !context.mAllocateDescriptorSets ||
        !context.mFreeDescriptorSets ||
        !context.mUpdateDescriptorSets ||
        !context.mCmdBindDescriptorSets ||
        !context.mCmdPushConstants)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend is missing required graphics-pipeline entry points."
            << LL_ENDL;
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }

    if (!create_vulkan_final_shader_modules(context, create_shader_module))
    {
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }

    context.mBootstrapVertexShader =
        get_vulkan_final_shader_module(context, "bootstrap.vert.spv", "bootstrap vertex");
    context.mBootstrapFragmentShader =
        get_vulkan_final_shader_module(context, "bootstrap.frag.spv", "bootstrap fragment");
    context.mUIVertexShader =
        get_vulkan_final_shader_module(context, "active/ui.vert.spv", "UI vertex");
    context.mUIFragmentShader =
        get_vulkan_final_shader_module(context, "active/ui.frag.spv", "UI fragment");
    context.mWorldVertexShader =
        get_vulkan_final_shader_module(context, "active/world_textured.vert.spv", "world vertex");
    context.mWorldFragmentShader =
        get_vulkan_final_shader_module(context, "active/world_textured.frag.spv", "world fragment");
    context.mTerrainVertexShader =
        get_vulkan_final_shader_module(context, "active/terrain.vert.spv", "terrain vertex");
    context.mTerrainFragmentShader =
        get_vulkan_final_shader_module(context, "active/terrain.frag.spv", "terrain fragment");
    if (!context.mBootstrapVertexShader ||
        !context.mBootstrapFragmentShader ||
        !context.mUIVertexShader ||
        !context.mUIFragmentShader ||
        !context.mWorldVertexShader ||
        !context.mWorldFragmentShader ||
        !context.mTerrainVertexShader ||
        !context.mTerrainFragmentShader)
    {
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }

    std::array<LLVkDescriptorSetLayoutBinding, MARE_VULKAN_MAX_TEXTURE_BINDINGS + 1> sampler_bindings = {};
    for (U32 i = 0; i < MARE_VULKAN_MAX_TEXTURE_BINDINGS; ++i)
    {
        sampler_bindings[i] =
        {
            i,
            LL_VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            LL_VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        };
    }
    sampler_bindings[MARE_VULKAN_MAX_TEXTURE_BINDINGS] =
    {
        MARE_VULKAN_SKINNING_DESCRIPTOR_BINDING,
        LL_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        1,
        LL_VK_SHADER_STAGE_VERTEX_BIT,
        nullptr
    };

    LLVkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info =
    {
        LL_VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        static_cast<U32>(sampler_bindings.size()),
        sampler_bindings.data()
    };

    S32 result = context.mCreateDescriptorSetLayout(
        context.mDevice,
        &descriptor_set_layout_create_info,
        nullptr,
        &context.mUIDescriptorSetLayout);
    if (result != LL_VK_SUCCESS || !context.mUIDescriptorSetLayout)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateDescriptorSetLayout(UI) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }

    std::array<LLVkDescriptorPoolSize, 2> descriptor_pool_sizes =
    {
        LLVkDescriptorPoolSize
        {
            LL_VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            MARE_VULKAN_TEXTURE_DESCRIPTOR_CAPACITY
        },
        LLVkDescriptorPoolSize
        {
            LL_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            MARE_VULKAN_TEXTURE_DESCRIPTOR_SET_CAPACITY
        }
    };
    LLVkDescriptorPoolCreateInfo descriptor_pool_create_info =
    {
        LL_VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        nullptr,
        LL_VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        MARE_VULKAN_TEXTURE_DESCRIPTOR_SET_CAPACITY,
        static_cast<U32>(descriptor_pool_sizes.size()),
        descriptor_pool_sizes.data()
    };

    result = context.mCreateDescriptorPool(
        context.mDevice,
        &descriptor_pool_create_info,
        nullptr,
        &context.mUIDescriptorPool);
    if (result != LL_VK_SUCCESS || !context.mUIDescriptorPool)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateDescriptorPool(UI) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }

    LLVkPipelineLayoutCreateInfo layout_create_info =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        0,
        nullptr,
        0,
        nullptr
    };
    LLVkDescriptorSetLayout ui_descriptor_set_layout = context.mUIDescriptorSetLayout;
    LLVkPipelineLayoutCreateInfo ui_layout_create_info =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        1,
        &ui_descriptor_set_layout,
        0,
        nullptr
    };
    const U32 world_push_constant_size =
        static_cast<U32>(sizeof(LLVulkanWorldPushConstants));
    const U32 max_push_constant_size =
        context.mMaxPushConstantsSize ? context.mMaxPushConstantsSize : 128;
    if (world_push_constant_size > max_push_constant_size)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan world push constants require "
            << world_push_constant_size
            << " bytes, but the selected device only supports "
            << max_push_constant_size
            << " bytes."
            << LL_ENDL;
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }
    LLVkPushConstantRange world_push_constant_range =
    {
        LL_VK_SHADER_STAGE_VERTEX_BIT | LL_VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        world_push_constant_size
    };
    LLVkPipelineLayoutCreateInfo world_layout_create_info =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        1,
        &ui_descriptor_set_layout,
        1,
        &world_push_constant_range
    };

    result = create_pipeline_layout(
        context.mDevice,
        &layout_create_info,
        nullptr,
        &context.mBootstrapPipelineLayout);
    if (result != LL_VK_SUCCESS || !context.mBootstrapPipelineLayout)
    {
        LL_WARNS("RenderBackend")
            << "vkCreatePipelineLayout(bootstrap) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }

    LLVkPipelineShaderStageCreateInfo shader_stages[2] =
    {
        LLVkPipelineShaderStageCreateInfo
        {
            LL_VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            LL_VK_SHADER_STAGE_VERTEX_BIT,
            context.mBootstrapVertexShader,
            "main",
            nullptr
        },
        LLVkPipelineShaderStageCreateInfo
        {
            LL_VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            LL_VK_SHADER_STAGE_FRAGMENT_BIT,
            context.mBootstrapFragmentShader,
            "main",
            nullptr
        }
    };

    LLVkPipelineVertexInputStateCreateInfo vertex_input =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr,
        0,
        0,
        nullptr,
        0,
        nullptr
    };

    LLVkPipelineInputAssemblyStateCreateInfo input_assembly =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        nullptr,
        0,
        LL_VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        0
    };

    LLVkViewport viewport =
    {
        0.f,
        0.f,
        static_cast<F32>(context.mSwapchainExtent.width),
        static_cast<F32>(context.mSwapchainExtent.height),
        0.f,
        1.f
    };

    LLVkRect2D scissor =
    {
        LLVkOffset2D { 0, 0 },
        context.mSwapchainExtent
    };

    LLVkPipelineViewportStateCreateInfo viewport_state =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        nullptr,
        0,
        1,
        &viewport,
        1,
        &scissor
    };

    LLVkPipelineRasterizationStateCreateInfo rasterization =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        nullptr,
        0,
        0,
        0,
        LL_VK_POLYGON_MODE_FILL,
        LL_VK_CULL_MODE_NONE,
        LL_VK_FRONT_FACE_COUNTER_CLOCKWISE,
        0,
        0.f,
        0.f,
        0.f,
        1.f
    };

    LLVkPipelineMultisampleStateCreateInfo multisample =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        nullptr,
        0,
        LL_VK_SAMPLE_COUNT_1_BIT,
        0,
        0.f,
        nullptr,
        0,
        0
    };
    LLVkPipelineDepthStencilStateCreateInfo disabled_depth_stencil =
        make_vulkan_depth_stencil_state(LLVulkanWorldDepthPipeline::Disabled);

    LLVkPipelineColorBlendAttachmentState color_blend_attachment =
    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        LL_VK_COLOR_COMPONENT_R_BIT |
            LL_VK_COLOR_COMPONENT_G_BIT |
            LL_VK_COLOR_COMPONENT_B_BIT |
            LL_VK_COLOR_COMPONENT_A_BIT
    };

    LLVkPipelineColorBlendStateCreateInfo color_blend =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr,
        0,
        0,
        0,
        1,
        &color_blend_attachment,
        { 0.f, 0.f, 0.f, 0.f }
    };

    S32 dynamic_states[2] =
    {
        LL_VK_DYNAMIC_STATE_VIEWPORT,
        LL_VK_DYNAMIC_STATE_SCISSOR
    };
    LLVkPipelineDynamicStateCreateInfo dynamic_state =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        nullptr,
        0,
        2,
        dynamic_states
    };

    LLVkGraphicsPipelineCreateInfo pipeline_create_info =
    {
        LL_VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        nullptr,
        0,
        2,
        shader_stages,
        &vertex_input,
        &input_assembly,
        nullptr,
        &viewport_state,
        &rasterization,
        &multisample,
        &disabled_depth_stencil,
        &color_blend,
        &dynamic_state,
        context.mBootstrapPipelineLayout,
        context.mRenderPass,
        0,
        nullptr,
        -1
    };

    result = create_graphics_pipelines(
        context.mDevice,
        nullptr,
        1,
        &pipeline_create_info,
        nullptr,
        &context.mBootstrapPipeline);
    if (result != LL_VK_SUCCESS || !context.mBootstrapPipeline)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateGraphicsPipelines(bootstrap) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }

    result = create_pipeline_layout(
        context.mDevice,
        &ui_layout_create_info,
        nullptr,
        &context.mUIPipelineLayout);
    if (result != LL_VK_SUCCESS || !context.mUIPipelineLayout)
    {
        LL_WARNS("RenderBackend")
            << "vkCreatePipelineLayout(UI) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }

    result = create_pipeline_layout(
        context.mDevice,
        &world_layout_create_info,
        nullptr,
        &context.mWorldPipelineLayout);
    if (result != LL_VK_SUCCESS || !context.mWorldPipelineLayout)
    {
        LL_WARNS("RenderBackend")
            << "vkCreatePipelineLayout(world) failed with result "
            << result
            << LL_ENDL;
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }

    LLVkPipelineShaderStageCreateInfo ui_shader_stages[2] =
    {
        LLVkPipelineShaderStageCreateInfo
        {
            LL_VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            LL_VK_SHADER_STAGE_VERTEX_BIT,
            context.mUIVertexShader,
            "main",
            nullptr
        },
        LLVkPipelineShaderStageCreateInfo
        {
            LL_VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            LL_VK_SHADER_STAGE_FRAGMENT_BIT,
            context.mUIFragmentShader,
            "main",
            nullptr
        }
    };
    LLVkPipelineShaderStageCreateInfo world_shader_stages[2] =
    {
        LLVkPipelineShaderStageCreateInfo
        {
            LL_VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            LL_VK_SHADER_STAGE_VERTEX_BIT,
            context.mWorldVertexShader,
            "main",
            nullptr
        },
        LLVkPipelineShaderStageCreateInfo
        {
            LL_VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            LL_VK_SHADER_STAGE_FRAGMENT_BIT,
            context.mWorldFragmentShader,
            "main",
            nullptr
        }
    };
    LLVkPipelineShaderStageCreateInfo terrain_shader_stages[2] =
    {
        LLVkPipelineShaderStageCreateInfo
        {
            LL_VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            LL_VK_SHADER_STAGE_VERTEX_BIT,
            context.mTerrainVertexShader,
            "main",
            nullptr
        },
        LLVkPipelineShaderStageCreateInfo
        {
            LL_VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            LL_VK_SHADER_STAGE_FRAGMENT_BIT,
            context.mTerrainFragmentShader,
            "main",
            nullptr
        }
    };

    LLVkVertexInputBindingDescription ui_bindings[5] =
    {
        { 0, 16, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 1, 8, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 2, 4, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 3, 8, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 4, 16, LL_VK_VERTEX_INPUT_RATE_VERTEX },
    };

    LLVkVertexInputAttributeDescription ui_attributes[5] =
    {
        { 0, 0, LL_VK_FORMAT_R32G32B32_SFLOAT, 0 },
        { 2, 1, LL_VK_FORMAT_R32G32_SFLOAT, 0 },
        { 6, 2, LL_VK_FORMAT_R8G8B8A8_UNORM, 0 },
        { 3, 3, LL_VK_FORMAT_R32G32_SFLOAT, 0 },
        { 13, 4, LL_VK_FORMAT_R32_UINT, 0 },
    };

    LLVkPipelineVertexInputStateCreateInfo ui_vertex_input =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr,
        0,
        5,
        ui_bindings,
        5,
        ui_attributes
    };

    LLVkVertexInputBindingDescription world_bindings[9] =
    {
        { 0, 16, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 1, 8, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 2, 4, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 3, 8, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 4, 16, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 5, 16, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 6, 4, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 7, 16, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 8, 16, LL_VK_VERTEX_INPUT_RATE_VERTEX },
    };

    LLVkVertexInputAttributeDescription world_attributes[9] =
    {
        { 0, 0, LL_VK_FORMAT_R32G32B32_SFLOAT, 0 },
        { 2, 1, LL_VK_FORMAT_R32G32_SFLOAT, 0 },
        { 6, 2, LL_VK_FORMAT_R8G8B8A8_UNORM, 0 },
        { 3, 3, LL_VK_FORMAT_R32G32_SFLOAT, 0 },
        { 13, 4, LL_VK_FORMAT_R32_UINT, 0 },
        { 10, 5, LL_VK_FORMAT_R32G32B32A32_SFLOAT, 0 },
        { 9, 6, LL_VK_FORMAT_R32_SFLOAT, 0 },
        { 1, 7, LL_VK_FORMAT_R32G32B32_SFLOAT, 0 },
        { 8, 8, LL_VK_FORMAT_R32G32B32A32_SFLOAT, 0 },
    };

    LLVkPipelineVertexInputStateCreateInfo world_vertex_input =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr,
        0,
        9,
        world_bindings,
        9,
        world_attributes
    };

    LLVkPipelineColorBlendAttachmentState ui_color_blend_attachment =
    {
        1,
        LL_VK_BLEND_FACTOR_SRC_ALPHA,
        LL_VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        LL_VK_BLEND_OP_ADD,
        LL_VK_BLEND_FACTOR_ONE,
        LL_VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        LL_VK_BLEND_OP_ADD,
        LL_VK_COLOR_COMPONENT_R_BIT |
            LL_VK_COLOR_COMPONENT_G_BIT |
            LL_VK_COLOR_COMPONENT_B_BIT |
            LL_VK_COLOR_COMPONENT_A_BIT
    };

    LLVkPipelineColorBlendStateCreateInfo ui_color_blend =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr,
        0,
        0,
        0,
        1,
        &ui_color_blend_attachment,
        { 0.f, 0.f, 0.f, 0.f }
    };

    for (U32 i = 0; i < context.mUIPipelines.size(); ++i)
    {
        LLVkPipelineInputAssemblyStateCreateInfo ui_input_assembly =
        {
            LL_VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            nullptr,
            0,
            to_vulkan_topology(static_cast<LLRenderPrimitiveType>(i)),
            0
        };

        LLVkGraphicsPipelineCreateInfo ui_pipeline_create_info =
        {
            LL_VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            nullptr,
            0,
            2,
            ui_shader_stages,
            &ui_vertex_input,
            &ui_input_assembly,
            nullptr,
            &viewport_state,
            &rasterization,
            &multisample,
            &disabled_depth_stencil,
            &ui_color_blend,
            &dynamic_state,
            context.mUIPipelineLayout,
            context.mRenderPass,
            0,
            nullptr,
            -1
        };

        result = create_graphics_pipelines(
            context.mDevice,
            nullptr,
            1,
            &ui_pipeline_create_info,
            nullptr,
            &context.mUIPipelines[i]);
        if (result != LL_VK_SUCCESS || !context.mUIPipelines[i])
        {
            LL_WARNS("RenderBackend")
                << "vkCreateGraphicsPipelines(UI mode "
                << i
                << ") failed with result "
                << result
                << LL_ENDL;
            destroy_vulkan_graphics_pipelines(context);
            return false;
        }

        for (U32 cull_index = 0; cull_index < MARE_VULKAN_WORLD_CULL_PIPELINE_COUNT; ++cull_index)
        {
            LLVulkanWorldCullPipeline cull_pipeline =
                static_cast<LLVulkanWorldCullPipeline>(cull_index);
            LLVkPipelineRasterizationStateCreateInfo world_rasterization = rasterization;
            world_rasterization.cullMode =
                cull_pipeline == LLVulkanWorldCullPipeline::Back ?
                LL_VK_CULL_MODE_BACK_BIT :
                LL_VK_CULL_MODE_NONE;

            for (U32 depth_index = 0; depth_index < MARE_VULKAN_WORLD_DEPTH_PIPELINE_COUNT; ++depth_index)
            {
                LLVulkanWorldDepthPipeline depth_pipeline =
                    static_cast<LLVulkanWorldDepthPipeline>(depth_index);
                LLVkPipelineDepthStencilStateCreateInfo world_depth_stencil =
                    make_vulkan_depth_stencil_state(depth_pipeline);

                for (U32 blend_index = 0; blend_index < MARE_VULKAN_WORLD_BLEND_PIPELINE_COUNT; ++blend_index)
                {
                    LLVulkanWorldBlendPipeline blend_pipeline =
                        static_cast<LLVulkanWorldBlendPipeline>(blend_index);
                    LLVkPipelineColorBlendAttachmentState world_color_blend_attachment =
                        make_vulkan_world_color_blend_attachment(blend_pipeline);
                    LLVkPipelineColorBlendStateCreateInfo world_color_blend =
                    {
                        LL_VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                        nullptr,
                        0,
                        0,
                        0,
                        1,
                        &world_color_blend_attachment,
                        { 0.f, 0.f, 0.f, 0.f }
                    };

                    U32 world_pipeline_index =
                        to_vulkan_world_pipeline_index(i, blend_pipeline, depth_pipeline, cull_pipeline);
                    LLVkGraphicsPipelineCreateInfo world_pipeline_create_info = ui_pipeline_create_info;
                    world_pipeline_create_info.pStages = world_shader_stages;
                    world_pipeline_create_info.pVertexInputState = &world_vertex_input;
                    world_pipeline_create_info.pRasterizationState = &world_rasterization;
                    world_pipeline_create_info.pDepthStencilState = &world_depth_stencil;
                    world_pipeline_create_info.pColorBlendState = &world_color_blend;
                    world_pipeline_create_info.layout = context.mWorldPipelineLayout;

                    result = create_graphics_pipelines(
                        context.mDevice,
                        nullptr,
                        1,
                        &world_pipeline_create_info,
                        nullptr,
                        &context.mWorldPipelines[world_pipeline_index]);
                    if (result != LL_VK_SUCCESS || !context.mWorldPipelines[world_pipeline_index])
                    {
                        LL_WARNS("RenderBackend")
                            << "vkCreateGraphicsPipelines(world mode "
                            << i
                            << ", blend "
                            << blend_index
                            << ", depth "
                            << depth_index
                            << ", cull "
                            << cull_index
                            << ") failed with result "
                            << result
                            << LL_ENDL;
                        destroy_vulkan_graphics_pipelines(context);
                        return false;
                    }

                    LLVkGraphicsPipelineCreateInfo terrain_pipeline_create_info = world_pipeline_create_info;
                    terrain_pipeline_create_info.pStages = terrain_shader_stages;
                    terrain_pipeline_create_info.pVertexInputState = &ui_vertex_input;

                    result = create_graphics_pipelines(
                        context.mDevice,
                        nullptr,
                        1,
                        &terrain_pipeline_create_info,
                        nullptr,
                        &context.mTerrainPipelines[world_pipeline_index]);
                    if (result != LL_VK_SUCCESS || !context.mTerrainPipelines[world_pipeline_index])
                    {
                        LL_WARNS("RenderBackend")
                            << "vkCreateGraphicsPipelines(terrain mode "
                            << i
                            << ", blend "
                            << blend_index
                            << ", depth "
                            << depth_index
                            << ", cull "
                            << cull_index
                            << ") failed with result "
                            << result
                            << LL_ENDL;
                        destroy_vulkan_graphics_pipelines(context);
                        return false;
                    }
                }
            }
        }
    }

    LL_INFOS("RenderBackend")
        << "Vulkan bootstrap, UI, world MVP, and terrain graphics pipelines created."
        << LL_ENDL;
    return true;
}

bool create_vulkan_swapchain_framebuffers(LLVulkanNativeContext& context)
{
    context.mCreateFramebuffer =
        reinterpret_cast<LLVulkanCreateFramebuffer>(
            get_vulkan_device_proc_address(context, "vkCreateFramebuffer"));
    context.mDestroyFramebuffer =
        reinterpret_cast<LLVulkanDestroyFramebuffer>(
            get_vulkan_device_proc_address(context, "vkDestroyFramebuffer"));

    if (!context.mCreateFramebuffer || !context.mDestroyFramebuffer)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend is missing required framebuffer entry points."
            << LL_ENDL;
        return false;
    }
    if (!context.mDepthAttachment.mImageView)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan framebuffer creation requires a depth attachment."
            << LL_ENDL;
        return false;
    }

    context.mSwapchainFramebuffers.reserve(context.mSwapchainImageViews.size());
    for (LLVkImageView image_view : context.mSwapchainImageViews)
    {
        LLVkFramebuffer framebuffer = nullptr;
        LLVkImageView attachments[2] =
        {
            image_view,
            context.mDepthAttachment.mImageView
        };
        LLVkFramebufferCreateInfo create_info =
        {
            LL_VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr,
            0,
            context.mRenderPass,
            2,
            attachments,
            context.mSwapchainExtent.width,
            context.mSwapchainExtent.height,
            1
        };

        S32 result = context.mCreateFramebuffer(
            context.mDevice,
            &create_info,
            nullptr,
            &framebuffer);
        if (result != LL_VK_SUCCESS || !framebuffer)
        {
            LL_WARNS("RenderBackend")
                << "vkCreateFramebuffer failed with result "
                << result
                << LL_ENDL;
            return false;
        }

        context.mSwapchainFramebuffers.push_back(framebuffer);
    }

    LL_INFOS("RenderBackend")
        << "Vulkan swapchain framebuffers created: "
        << context.mSwapchainFramebuffers.size()
        << "."
        << LL_ENDL;
    return true;
}

void destroy_vulkan_swapchain_framebuffers(LLVulkanNativeContext& context)
{
    if (context.mDestroyFramebuffer && context.mDevice)
    {
        for (LLVkFramebuffer framebuffer : context.mSwapchainFramebuffers)
        {
            if (framebuffer)
            {
                context.mDestroyFramebuffer(context.mDevice, framebuffer, nullptr);
            }
        }
    }

    context.mSwapchainFramebuffers.clear();
}

bool create_vulkan_command_buffers(LLVulkanNativeContext& context)
{
    LLVulkanCreateCommandPool create_command_pool =
        reinterpret_cast<LLVulkanCreateCommandPool>(
            get_vulkan_device_proc_address(context, "vkCreateCommandPool"));
    context.mDestroyCommandPool =
        reinterpret_cast<LLVulkanDestroyCommandPool>(
            get_vulkan_device_proc_address(context, "vkDestroyCommandPool"));
    context.mAllocateCommandBuffers =
        reinterpret_cast<LLVulkanAllocateCommandBuffers>(
            get_vulkan_device_proc_address(context, "vkAllocateCommandBuffers"));
    context.mFreeCommandBuffers =
        reinterpret_cast<LLVulkanFreeCommandBuffers>(
            get_vulkan_device_proc_address(context, "vkFreeCommandBuffers"));

    if (!create_command_pool ||
        !context.mDestroyCommandPool ||
        !context.mAllocateCommandBuffers ||
        !context.mFreeCommandBuffers)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend is missing required command-buffer entry points."
            << LL_ENDL;
        return false;
    }

    LLVkCommandPoolCreateInfo pool_create_info =
    {
        LL_VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        LL_VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        context.mGraphicsQueueFamilyIndex
    };

    S32 result = create_command_pool(
        context.mDevice,
        &pool_create_info,
        nullptr,
        &context.mCommandPool);
    if (result != LL_VK_SUCCESS || !context.mCommandPool)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateCommandPool failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    context.mCommandBuffers.resize(context.mSwapchainImageViews.size());
    LLVkCommandBufferAllocateInfo allocate_info =
    {
        LL_VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        context.mCommandPool,
        LL_VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<U32>(context.mCommandBuffers.size())
    };

    result = context.mAllocateCommandBuffers(
        context.mDevice,
        &allocate_info,
        context.mCommandBuffers.data());
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkAllocateCommandBuffers failed with result "
            << result
            << LL_ENDL;
        context.mCommandBuffers.clear();
        return false;
    }

    LL_INFOS("RenderBackend")
        << "Vulkan command pool and command buffers created: "
        << context.mCommandBuffers.size()
        << "."
        << LL_ENDL;
    return true;
}

bool is_vulkan_swapchain_success(S32 result)
{
    return result == LL_VK_SUCCESS || result == LL_VK_SUBOPTIMAL_KHR;
}

bool is_vulkan_swapchain_stale_result(S32 result)
{
    return result == LL_VK_ERROR_OUT_OF_DATE_KHR || result == LL_VK_SUBOPTIMAL_KHR;
}

bool create_vulkan_frame_sync(LLVulkanNativeContext& context)
{
    LLVulkanCreateSemaphore create_semaphore =
        reinterpret_cast<LLVulkanCreateSemaphore>(
            get_vulkan_device_proc_address(context, "vkCreateSemaphore"));
    context.mDestroySemaphore =
        reinterpret_cast<LLVulkanDestroySemaphore>(
            get_vulkan_device_proc_address(context, "vkDestroySemaphore"));
    context.mCreateFence =
        reinterpret_cast<LLVulkanCreateFence>(
            get_vulkan_device_proc_address(context, "vkCreateFence"));
    context.mDestroyFence =
        reinterpret_cast<LLVulkanDestroyFence>(
            get_vulkan_device_proc_address(context, "vkDestroyFence"));
    context.mWaitForFences =
        reinterpret_cast<LLVulkanWaitForFences>(
            get_vulkan_device_proc_address(context, "vkWaitForFences"));
    context.mResetFences =
        reinterpret_cast<LLVulkanResetFences>(
            get_vulkan_device_proc_address(context, "vkResetFences"));
    context.mAcquireNextImage =
        reinterpret_cast<LLVulkanAcquireNextImageKHR>(
            get_vulkan_device_proc_address(context, "vkAcquireNextImageKHR"));
    context.mQueueSubmit =
        reinterpret_cast<LLVulkanQueueSubmit>(
            get_vulkan_device_proc_address(context, "vkQueueSubmit"));
    context.mQueuePresent =
        reinterpret_cast<LLVulkanQueuePresentKHR>(
            get_vulkan_device_proc_address(context, "vkQueuePresentKHR"));

    if (!create_semaphore ||
        !context.mDestroySemaphore ||
        !context.mCreateFence ||
        !context.mDestroyFence ||
        !context.mWaitForFences ||
        !context.mResetFences ||
        !context.mAcquireNextImage ||
        !context.mQueueSubmit ||
        !context.mQueuePresent)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend is missing required frame synchronization entry points."
            << LL_ENDL;
        return false;
    }

    LLVkSemaphoreCreateInfo semaphore_create_info =
    {
        LL_VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        nullptr,
        0
    };
    LLVkFenceCreateInfo fence_create_info =
    {
        LL_VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        nullptr,
        LL_VK_FENCE_CREATE_SIGNALED_BIT
    };

    context.mFrameSync.resize(context.mCommandBuffers.size());
    for (LLVulkanFrameSync& frame_sync : context.mFrameSync)
    {
        S32 result = create_semaphore(
            context.mDevice,
            &semaphore_create_info,
            nullptr,
            &frame_sync.mImageAvailableSemaphore);
        if (result != LL_VK_SUCCESS || !frame_sync.mImageAvailableSemaphore)
        {
            LL_WARNS("RenderBackend")
                << "vkCreateSemaphore(image available) failed with result "
                << result
                << LL_ENDL;
            return false;
        }

        result = create_semaphore(
            context.mDevice,
            &semaphore_create_info,
            nullptr,
            &frame_sync.mRenderFinishedSemaphore);
        if (result != LL_VK_SUCCESS || !frame_sync.mRenderFinishedSemaphore)
        {
            LL_WARNS("RenderBackend")
                << "vkCreateSemaphore(render finished) failed with result "
                << result
                << LL_ENDL;
            return false;
        }

        result = context.mCreateFence(
            context.mDevice,
            &fence_create_info,
            nullptr,
            &frame_sync.mInFlightFence);
        if (result != LL_VK_SUCCESS || !frame_sync.mInFlightFence)
        {
            LL_WARNS("RenderBackend")
                << "vkCreateFence failed with result "
                << result
                << LL_ENDL;
            return false;
        }
    }

    LL_INFOS("RenderBackend")
        << "Vulkan frame synchronization objects created: "
        << context.mFrameSync.size()
        << " frame slot(s)."
        << LL_ENDL;
    return true;
}

bool wait_for_vulkan_fence_sleeping(
    LLVulkanNativeContext& context,
    LLVkFence fence,
    const char* operation)
{
    if (!context.mWaitForFences || !context.mDevice || !fence)
    {
        return false;
    }

    U32 wait_count = 0;
    for (;;)
    {
        S32 result = context.mWaitForFences(
            context.mDevice,
            1,
            &fence,
            1,
            0);
        if (result == LL_VK_SUCCESS)
        {
            return true;
        }

        if (result != LL_VK_TIMEOUT)
        {
            LL_WARNS("RenderBackend")
                << "vkWaitForFences "
                << operation
                << " failed with result "
                << result
                << LL_ENDL;
            return false;
        }

        if (wait_count == 0)
        {
            std::this_thread::yield();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        ++wait_count;
    }
}

void destroy_vulkan_frame_sync(LLVulkanNativeContext& context)
{
    if (context.mDestroySemaphore && context.mDevice)
    {
        for (LLVulkanFrameSync& frame_sync : context.mFrameSync)
        {
            if (frame_sync.mImageAvailableSemaphore)
            {
                context.mDestroySemaphore(context.mDevice, frame_sync.mImageAvailableSemaphore, nullptr);
            }
            if (frame_sync.mRenderFinishedSemaphore)
            {
                context.mDestroySemaphore(context.mDevice, frame_sync.mRenderFinishedSemaphore, nullptr);
            }
        }
    }

    if (context.mDestroyFence && context.mDevice)
    {
        for (LLVulkanFrameSync& frame_sync : context.mFrameSync)
        {
            if (frame_sync.mInFlightFence)
            {
                context.mDestroyFence(context.mDevice, frame_sync.mInFlightFence, nullptr);
            }
        }
    }

    context.mFrameSync.clear();
    context.mDestroySemaphore = nullptr;
    context.mCreateFence = nullptr;
    context.mDestroyFence = nullptr;
    context.mWaitForFences = nullptr;
    context.mResetFences = nullptr;
    context.mAcquireNextImage = nullptr;
    context.mQueueSubmit = nullptr;
    context.mQueuePresent = nullptr;
}

void destroy_vulkan_command_buffers(LLVulkanNativeContext& context);
void destroy_vulkan_swapchain(LLVulkanNativeContext& context);

bool is_vulkan_swapchain_extent_stale(LLVulkanNativeContext& context)
{
#if LL_DARWIN
    if (!context.mNativeView)
    {
        return false;
    }

    U32 drawable_width = 0;
    U32 drawable_height = 0;
    if (!ll_render_macosx_get_metal_layer_drawable_size(
            context.mNativeView,
            &drawable_width,
            &drawable_height))
    {
        return false;
    }

    return drawable_width != context.mSwapchainExtent.width ||
        drawable_height != context.mSwapchainExtent.height;
#else
    return false;
#endif
}

bool recreate_vulkan_swapchain_resources(LLVulkanNativeContext& context)
{
#if LL_DARWIN
    if (!context.mNativeView)
    {
        return false;
    }

    if (context.mDeviceWaitIdle)
    {
        S32 idle_result = context.mDeviceWaitIdle(context.mDevice);
        if (idle_result != LL_VK_SUCCESS)
        {
            LL_WARNS("RenderBackend")
                << "vkDeviceWaitIdle before swapchain recreation failed with result "
                << idle_result
                << LL_ENDL;
            return false;
        }
    }

    LL_INFOS("RenderBackend")
        << "Recreating Vulkan swapchain resources for resized native view."
        << LL_ENDL;

    destroy_vulkan_frame_sync(context);
    destroy_vulkan_command_buffers(context);
    destroy_vulkan_swapchain_framebuffers(context);
    destroy_vulkan_depth_attachment(context);
    destroy_vulkan_swapchain_image_views(context);
    destroy_vulkan_swapchain(context);

    if (!create_vulkan_swapchain(context, context.mNativeView, context.mEnableVSync) ||
        !create_vulkan_swapchain_image_views(context) ||
        !create_vulkan_depth_attachment(context) ||
        !create_vulkan_swapchain_framebuffers(context) ||
        !create_vulkan_command_buffers(context) ||
        !create_vulkan_frame_sync(context))
    {
        LL_WARNS("RenderBackend")
            << "Failed to recreate Vulkan swapchain resources after resize."
            << LL_ENDL;
        return false;
    }

    context.mLoggedUIDrawTelemetry = false;
    return true;
#else
    return false;
#endif
}

bool record_vulkan_frame_command_buffer(
    LLVulkanNativeContext& context,
    U32 image_index)
{
    context.mBeginCommandBuffer =
        reinterpret_cast<LLVulkanBeginCommandBuffer>(
            get_vulkan_device_proc_address(context, "vkBeginCommandBuffer"));
    context.mEndCommandBuffer =
        reinterpret_cast<LLVulkanEndCommandBuffer>(
            get_vulkan_device_proc_address(context, "vkEndCommandBuffer"));
    context.mResetCommandBuffer =
        reinterpret_cast<LLVulkanResetCommandBuffer>(
            get_vulkan_device_proc_address(context, "vkResetCommandBuffer"));
    context.mCmdBeginRenderPass =
        reinterpret_cast<LLVulkanCmdBeginRenderPass>(
            get_vulkan_device_proc_address(context, "vkCmdBeginRenderPass"));
    context.mCmdEndRenderPass =
        reinterpret_cast<LLVulkanCmdEndRenderPass>(
            get_vulkan_device_proc_address(context, "vkCmdEndRenderPass"));
    context.mCmdClearAttachments =
        reinterpret_cast<LLVulkanCmdClearAttachments>(
            get_vulkan_device_proc_address(context, "vkCmdClearAttachments"));

    if (!context.mBeginCommandBuffer ||
        !context.mEndCommandBuffer ||
        !context.mResetCommandBuffer ||
        !context.mCmdBeginRenderPass ||
        !context.mCmdEndRenderPass ||
        !context.mCmdClearAttachments ||
        !context.mCmdBindPipeline ||
        !context.mCmdDraw ||
        !context.mCmdBindIndexBuffer ||
        !context.mCmdDrawIndexed ||
        !context.mCmdBindVertexBuffers ||
        !context.mCmdSetViewport ||
        !context.mCmdSetScissor ||
        !context.mCmdBindDescriptorSets ||
        !context.mCmdPushConstants)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend is missing required command recording entry points."
            << LL_ENDL;
        return false;
    }

    if (image_index >= context.mCommandBuffers.size() ||
        image_index >= context.mSwapchainFramebuffers.size())
    {
        LL_WARNS("RenderBackend")
            << "Vulkan acquired swapchain image index is out of range: "
            << image_index
            << LL_ENDL;
        return false;
    }

    LLVkCommandBuffer command_buffer = context.mCommandBuffers[image_index];
    S32 result = context.mResetCommandBuffer(command_buffer, 0);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkResetCommandBuffer failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    const bool gpu_skinning_palette_ready =
        prepare_vulkan_skinning_matrix_palette_buffer(context);

    LLVkCommandBufferBeginInfo begin_info =
    {
        LL_VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        LL_VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr
    };

    result = context.mBeginCommandBuffer(command_buffer, &begin_info);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkBeginCommandBuffer failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    LLVkClearValue clear_values[2] = {};
    clear_values[0].color[0] = gCurrentVulkanClearColor.mRed;
    clear_values[0].color[1] = gCurrentVulkanClearColor.mGreen;
    clear_values[0].color[2] = gCurrentVulkanClearColor.mBlue;
    clear_values[0].color[3] = gCurrentVulkanClearColor.mAlpha;
    clear_values[1].depthStencil.depth = 1.f;
    clear_values[1].depthStencil.stencil = 0;
    LLVkRenderPassBeginInfo render_pass_begin =
    {
        LL_VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        context.mRenderPass,
        context.mSwapchainFramebuffers[image_index],
        LLVkRect2D
        {
            LLVkOffset2D { 0, 0 },
            context.mSwapchainExtent
        },
        2,
        clear_values
    };

    context.mCmdBeginRenderPass(
        command_buffer,
        &render_pass_begin,
        LL_VK_SUBPASS_CONTENTS_INLINE);

    U32 ui_draw_count = 0;
    U32 missing_buffer_count = 0;
    U32 missing_attribute_count = 0;
    U32 classic_avatar_skinning_draw_count = 0;
    U32 rigged_skinning_draw_count = 0;
    U32 skipped_classic_avatar_skinning_draw_count = 0;
    U32 skipped_rigged_skinning_draw_count = 0;
    U32 offscreen_tagged_draw_count = 0;
    U32 offscreen_tagged_clear_count = 0;
    LLVulkanDrawBounds largest_textured_bounds;
    U32 largest_textured_draw_texture = 0;
    S32 largest_textured_draw_texture_width = 0;
    S32 largest_textured_draw_texture_height = 0;
    LLRenderViewport largest_textured_draw_viewport = {};
    F32 largest_textured_draw_area = -1.f;
    LLVulkanDrawBounds largest_texture_draw_bounds;
    U32 largest_texture_draw_texture = 0;
    S32 largest_texture_draw_texture_width = 0;
    S32 largest_texture_draw_texture_height = 0;
    LLRenderViewport largest_texture_draw_viewport = {};
    F32 largest_texture_draw_texture_area = -1.f;
    LLVulkanDrawBounds largest_solid_draw_bounds;
    LLVulkanDrawColor largest_solid_draw_color;
    LLRenderViewport largest_solid_draw_viewport = {};
    LLRenderScissor largest_solid_draw_scissor = {};
    U32 largest_solid_draw_mode = 0;
    S32 largest_solid_draw_count = 0;
    bool largest_solid_draw_indexed = false;
    F32 largest_solid_draw_area = -1.f;
    LLVulkanDrawBounds largest_solid_strip_bounds;
    LLVulkanDrawColor largest_solid_strip_color;
    LLRenderViewport largest_solid_strip_viewport = {};
    LLRenderScissor largest_solid_strip_scissor = {};
    U32 largest_solid_strip_mode = 0;
    S32 largest_solid_strip_count = 0;
    bool largest_solid_strip_indexed = false;
    F32 largest_solid_strip_area = -1.f;
    for (const LLVulkanPendingDraw& draw : gPendingVulkanDraws)
    {
        if (draw.mClearOnly)
        {
            if (draw.mFramebuffer != 0)
            {
                ++offscreen_tagged_clear_count;
            }
            record_vulkan_clear_command(context, command_buffer, draw);
            continue;
        }
        if (draw.mFramebuffer != 0)
        {
            ++offscreen_tagged_draw_count;
        }

        auto buffer_iter = gVulkanBuffers.find(draw.mBuffer);
        if (buffer_iter == gVulkanBuffers.end() ||
            !buffer_iter->second.mBuffer ||
            draw.mCount <= 0)
        {
            ++missing_buffer_count;
            continue;
        }

        LLVkBuffer index_buffer = nullptr;
        const LLVulkanBufferResource* index_resource = nullptr;
        if (draw.mIndexed)
        {
            auto index_iter = gVulkanBuffers.find(draw.mIndexBuffer);
            if (index_iter == gVulkanBuffers.end() || !index_iter->second.mBuffer)
            {
                ++missing_buffer_count;
                continue;
            }
            index_buffer = index_iter->second.mBuffer;
            index_resource = &index_iter->second;
        }

        if (!draw.mAttributes[0].mEnabled)
        {
            ++missing_attribute_count;
            continue;
        }

        LLVkPipelineLayout pipeline_layout =
            draw.mUseWorldVertexShader ? context.mWorldPipelineLayout : context.mUIPipelineLayout;
        U32 primitive_pipeline_index = to_vulkan_ui_pipeline_index(draw.mMode);
        LLVkPipeline pipeline = nullptr;
        const bool draw_has_rigged_skinning =
            draw.mUseWorldVertexShader &&
            draw.mWorldShaderClass != LLRenderWorldShaderClass::Terrain &&
            draw.mSkinningMatrixCount > 0 &&
            draw.mAttributes[10].mEnabled;
        const bool draw_has_classic_avatar_skinning =
            draw.mUseWorldVertexShader &&
            draw.mWorldShaderClass != LLRenderWorldShaderClass::Terrain &&
            draw.mSkinningMatrixCount > 0 &&
            draw.mAttributes[9].mEnabled;
        const bool draw_has_skinning =
            draw_has_rigged_skinning || draw_has_classic_avatar_skinning;
        bool use_gpu_skinning =
            draw_has_skinning &&
            gpu_skinning_palette_ready &&
            context.mSkinningMatrixPaletteBuffer.mBuffer;
        if (draw_has_classic_avatar_skinning)
        {
            ++classic_avatar_skinning_draw_count;
        }
        if (draw_has_rigged_skinning)
        {
            ++rigged_skinning_draw_count;
        }
        if (draw.mUseWorldVertexShader)
        {
            U32 world_pipeline_index =
                to_vulkan_world_pipeline_index(
                    primitive_pipeline_index,
                    draw.mWorldBlendPipeline,
                    draw.mWorldDepthPipeline,
                    draw.mWorldCullPipeline);
            if (world_pipeline_index < context.mWorldPipelines.size())
            {
                pipeline = draw.mWorldShaderClass == LLRenderWorldShaderClass::Terrain ?
                    context.mTerrainPipelines[world_pipeline_index] :
                    context.mWorldPipelines[world_pipeline_index];
            }
            if (!pipeline)
            {
                use_gpu_skinning = false;
                world_pipeline_index = to_vulkan_world_pipeline_index(
                    to_vulkan_ui_pipeline_index(LLRenderPrimitiveType::Triangles),
                    draw.mWorldBlendPipeline,
                    draw.mWorldDepthPipeline,
                    draw.mWorldCullPipeline);
                if (world_pipeline_index < context.mWorldPipelines.size())
                {
                    pipeline = draw.mWorldShaderClass == LLRenderWorldShaderClass::Terrain ?
                        context.mTerrainPipelines[world_pipeline_index] :
                        context.mWorldPipelines[world_pipeline_index];
                }
            }
        }
        else
        {
            if (primitive_pipeline_index < context.mUIPipelines.size())
            {
                pipeline = context.mUIPipelines[primitive_pipeline_index];
            }
            if (!pipeline)
            {
                primitive_pipeline_index = to_vulkan_ui_pipeline_index(LLRenderPrimitiveType::Triangles);
                if (primitive_pipeline_index < context.mUIPipelines.size())
                {
                    pipeline = context.mUIPipelines[primitive_pipeline_index];
                }
            }
        }

        if (!pipeline_layout || !pipeline)
        {
            ++missing_attribute_count;
            continue;
        }

        LLVkBuffer texcoord_buffer = draw.mAttributes[2].mEnabled ?
            buffer_iter->second.mBuffer :
            context.mDefaultTexCoordBuffer.mBuffer;
        LLVkBuffer color_buffer = draw.mAttributes[6].mEnabled ?
            buffer_iter->second.mBuffer :
            context.mDefaultColorBuffer.mBuffer;
        LLVkBuffer texcoord1_buffer = draw.mAttributes[3].mEnabled ?
            buffer_iter->second.mBuffer :
            context.mDefaultTexCoordBuffer.mBuffer;
        LLVkBuffer normal_buffer = draw.mAttributes[1].mEnabled ?
            buffer_iter->second.mBuffer :
            context.mDefaultNormalBuffer.mBuffer;
        LLVkBuffer tangent_buffer = draw.mAttributes[8].mEnabled ?
            buffer_iter->second.mBuffer :
            context.mDefaultTangentBuffer.mBuffer;

        if (!texcoord_buffer || !color_buffer || !texcoord1_buffer || !normal_buffer || !tangent_buffer)
        {
            ++missing_attribute_count;
            continue;
        }

        LLVkBuffer position_buffer = buffer_iter->second.mBuffer;
        U64 position_offset = draw.mAttributes[0].mOffset;
        const bool needs_cpu_skinning =
            draw_has_rigged_skinning &&
            !use_gpu_skinning;
        if (draw_has_classic_avatar_skinning && !use_gpu_skinning)
        {
            ++skipped_classic_avatar_skinning_draw_count;
            ++missing_attribute_count;
            continue;
        }
        if (needs_cpu_skinning)
        {
            const LLVulkanBufferResource* skinned_position_buffer =
                create_vulkan_skinned_position_buffer(
                    context,
                    draw,
                    buffer_iter->second,
                    index_resource);
            if (!skinned_position_buffer)
            {
                ++skipped_rigged_skinning_draw_count;
                ++missing_attribute_count;
                continue;
            }

            position_buffer = skinned_position_buffer->mBuffer;
            position_offset = 0;
        }

        LLVkBuffer weight_buffer = draw.mAttributes[9].mEnabled ?
            buffer_iter->second.mBuffer :
            context.mDefaultColorBuffer.mBuffer;
        if (!weight_buffer)
        {
            ++missing_attribute_count;
            continue;
        }

        LLVkBuffer vertex_buffers[9] =
        {
            position_buffer,
            texcoord_buffer,
            color_buffer,
            texcoord1_buffer,
            buffer_iter->second.mBuffer,
            buffer_iter->second.mBuffer,
            weight_buffer,
            normal_buffer,
            tangent_buffer
        };
        U64 offsets[9] =
        {
            position_offset,
            draw.mAttributes[2].mEnabled ? draw.mAttributes[2].mOffset : 0,
            draw.mAttributes[6].mEnabled ? draw.mAttributes[6].mOffset : 0,
            draw.mAttributes[3].mEnabled ? draw.mAttributes[3].mOffset : 0,
            draw.mAttributes[13].mEnabled ?
                draw.mAttributes[13].mOffset :
                position_offset + 12,
            draw.mAttributes[10].mEnabled ?
                draw.mAttributes[10].mOffset :
                position_offset,
            draw.mAttributes[9].mEnabled ?
                draw.mAttributes[9].mOffset :
                0,
            draw.mAttributes[1].mEnabled ? draw.mAttributes[1].mOffset : 0,
            draw.mAttributes[8].mEnabled ? draw.mAttributes[8].mOffset : 0
        };

        LLVkDescriptorSet descriptor_set =
            get_vulkan_texture_descriptor_set(context, draw.mTextures);
        auto texture_iter = gVulkanTextures.find(draw.mTexture);
        if (texture_iter != gVulkanTextures.end())
        {
            LLVulkanDrawBounds bounds = compute_vulkan_draw_bounds(
                draw,
                buffer_iter->second,
                index_resource);
            if (bounds.mValid)
            {
                const F32 area =
                    (bounds.mMaxX - bounds.mMinX) *
                    (bounds.mMaxY - bounds.mMinY);
                if (draw.mTexture == 0 && area > largest_solid_draw_area)
                {
                    largest_solid_draw_area = area;
                    largest_solid_draw_bounds = bounds;
                    largest_solid_draw_color = read_vulkan_draw_first_color(
                        draw,
                        buffer_iter->second,
                        index_resource);
                    largest_solid_draw_viewport = draw.mViewport;
                    largest_solid_draw_scissor = draw.mScissor;
                    largest_solid_draw_mode = static_cast<U32>(draw.mMode);
                    largest_solid_draw_count = draw.mCount;
                    largest_solid_draw_indexed = draw.mIndexed;
                }

                const F32 width = bounds.mMaxX - bounds.mMinX;
                const F32 height = bounds.mMaxY - bounds.mMinY;
                if (draw.mTexture == 0 &&
                    width >= 1.5f &&
                    height > 0.f &&
                    height <= 0.35f &&
                    area > largest_solid_strip_area)
                {
                    largest_solid_strip_area = area;
                    largest_solid_strip_bounds = bounds;
                    largest_solid_strip_color = read_vulkan_draw_first_color(
                        draw,
                        buffer_iter->second,
                        index_resource);
                    largest_solid_strip_viewport = draw.mViewport;
                    largest_solid_strip_scissor = draw.mScissor;
                    largest_solid_strip_mode = static_cast<U32>(draw.mMode);
                    largest_solid_strip_count = draw.mCount;
                    largest_solid_strip_indexed = draw.mIndexed;
                }

                if (area > largest_textured_draw_area)
                {
                    largest_textured_draw_area = area;
                    largest_textured_bounds = bounds;
                    largest_textured_draw_texture = draw.mTexture;
                    largest_textured_draw_texture_width = texture_iter->second.mWidth;
                    largest_textured_draw_texture_height = texture_iter->second.mHeight;
                    largest_textured_draw_viewport = draw.mViewport;
                }

                const F32 texture_area =
                    static_cast<F32>(texture_iter->second.mWidth) *
                    static_cast<F32>(texture_iter->second.mHeight);
                if (texture_area > largest_texture_draw_texture_area)
                {
                    largest_texture_draw_texture_area = texture_area;
                    largest_texture_draw_bounds = bounds;
                    largest_texture_draw_texture = draw.mTexture;
                    largest_texture_draw_texture_width = texture_iter->second.mWidth;
                    largest_texture_draw_texture_height = texture_iter->second.mHeight;
                    largest_texture_draw_viewport = draw.mViewport;
                }
            }
        }
        if (!descriptor_set)
        {
            continue;
        }

        LLVkViewport viewport = to_vulkan_viewport(context, draw.mViewport);
        LLVkRect2D scissor = to_vulkan_scissor(context, draw.mViewport, draw.mScissor);
        context.mCmdSetViewport(command_buffer, 0, 1, &viewport);
        context.mCmdSetScissor(command_buffer, 0, 1, &scissor);
        context.mCmdBindPipeline(
            command_buffer,
            LL_VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline);
        context.mCmdBindDescriptorSets(
            command_buffer,
            LL_VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout,
            0,
            1,
            &descriptor_set,
            0,
            nullptr);
        if (draw.mUseWorldVertexShader)
        {
            LLVulkanWorldPushConstants push_constants;
            push_constants.mModelviewProjection = draw.mModelviewProjection;
            push_constants.mParams = glm::vec4(
                draw.mAlphaMaskCutoff,
                draw.mAttributes[13].mEnabled ? 1.f : 0.f,
                use_gpu_skinning ? static_cast<F32>(draw.mSkinningMatrixOffset) : 0.f,
                use_gpu_skinning ? static_cast<F32>(draw.mSkinningMatrixCount) : 0.f);
            if (draw.mWorldShaderClass == LLRenderWorldShaderClass::Terrain)
            {
                push_constants.mTerrainParameters = glm::vec4(
                    draw.mTerrainParameters.mDetailScale,
                    draw.mTerrainParameters.mOffsetX,
                    draw.mTerrainParameters.mOffsetY,
                    draw_has_classic_avatar_skinning && use_gpu_skinning ? 1.f : 0.f);
            }
            else
            {
                push_constants.mTerrainParameters = glm::vec4(
                    draw.mMaterialParameters.mBaseColorRed,
                    draw.mMaterialParameters.mBaseColorGreen,
                    draw.mMaterialParameters.mBaseColorBlue,
                    draw_has_classic_avatar_skinning && use_gpu_skinning ? 1.f : 0.f);
            }
            push_constants.mTextureTransformS = glm::vec4(
                draw.mTextureTransform.mS[0],
                draw.mTextureTransform.mS[1],
                draw.mTextureTransform.mS[2],
                draw.mTextureTransform.mS[3]);
            push_constants.mTextureTransformT = glm::vec4(
                draw.mTextureTransform.mT[0],
                draw.mTextureTransform.mT[1],
                draw.mTextureTransform.mT[2],
                draw.mTextureTransform.mT[3]);
            push_constants.mMaterialExtra = glm::vec4(
                draw.mMaterialParameters.mEmissiveColorRed,
                draw.mMaterialParameters.mEmissiveColorGreen,
                draw.mMaterialParameters.mEmissiveColorBlue,
                draw.mMaterialParameters.mHasEmissiveMap);
            push_constants.mBaseTextureTransform0 = glm::vec4(
                draw.mMaterialParameters.mBaseTextureScaleS,
                draw.mMaterialParameters.mBaseTextureScaleT,
                draw.mMaterialParameters.mBaseTextureRotation,
                draw.mMaterialParameters.mBaseTextureOffsetS);
            push_constants.mBaseTextureTransform1 = glm::vec4(
                draw.mMaterialParameters.mBaseTextureOffsetT,
                0.f,
                0.f,
                0.f);
            push_constants.mMaterialPBR = glm::vec4(
                draw.mMaterialParameters.mRoughnessFactor,
                draw.mMaterialParameters.mMetallicFactor,
                draw.mMaterialParameters.mMaterialFlags,
                draw.mMaterialParameters.mBaseColorAlpha);
            push_constants.mMaterialLegacy = glm::vec4(
                draw.mMaterialParameters.mSpecularColorRed,
                draw.mMaterialParameters.mSpecularColorGreen,
                draw.mMaterialParameters.mSpecularColorBlue,
                draw.mMaterialParameters.mEnvIntensity);
            push_constants.mMaterialModes = glm::vec4(
                draw.mMaterialParameters.mDiffuseAlphaMode,
                draw.mMaterialParameters.mGLTFAlphaMode,
                draw.mMaterialParameters.mBump,
                draw.mMaterialParameters.mShiny);
            context.mCmdPushConstants(
                command_buffer,
                pipeline_layout,
                LL_VK_SHADER_STAGE_VERTEX_BIT | LL_VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(push_constants),
                &push_constants);
        }
        const U32 vertex_buffer_count =
            draw.mUseWorldVertexShader &&
            draw.mWorldShaderClass != LLRenderWorldShaderClass::Terrain ?
            9 :
            5;
        context.mCmdBindVertexBuffers(
            command_buffer,
            0,
            vertex_buffer_count,
            vertex_buffers,
            offsets);
        if (draw.mIndexed)
        {
            context.mCmdBindIndexBuffer(
                command_buffer,
                index_buffer,
                0,
                draw.mIndexType);
            context.mCmdDrawIndexed(
                command_buffer,
                static_cast<U32>(draw.mCount),
                1,
                static_cast<U32>(draw.mFirst),
                0,
                0);
        }
        else
        {
            context.mCmdDraw(
                command_buffer,
                static_cast<U32>(draw.mCount),
                1,
                static_cast<U32>(draw.mFirst),
                0);
        }
        ++ui_draw_count;
    }

    if (ui_draw_count == 0)
    {
        if (!gPendingVulkanDraws.empty() && !context.mLoggedFirstUIDraw)
        {
            LL_WARNS("RenderBackend")
                << "Vulkan UI bridge had "
                << gPendingVulkanDraws.size()
                << " pending draw(s), but none were recordable. Missing buffer: "
                << missing_buffer_count
                << ", missing attributes: "
                << missing_attribute_count
                << "."
                << LL_ENDL;
            context.mLoggedFirstUIDraw = true;
        }

        if (gPendingVulkanDraws.empty() && !context.mLoggedFirstEmptyFrame)
        {
            LL_INFOS("RenderBackend")
                << "Vulkan presented an empty clear frame before UI draw commands were available."
                << LL_ENDL;
            context.mLoggedFirstEmptyFrame = true;
        }
    }
    else if (!context.mLoggedFirstUIDraw)
    {
        LL_INFOS("RenderBackend")
            << "Vulkan recorded "
            << ui_draw_count
            << " UI draw command(s) for the current frame."
            << LL_ENDL;
        context.mLoggedFirstUIDraw = true;
    }

    context.mRecordedUIDrawCount += ui_draw_count;
    context.mRecordMissingBufferCount += missing_buffer_count;
    context.mRecordMissingAttributeCount += missing_attribute_count;
    if (!context.mLoggedUIDrawTelemetry && context.mPresentedFrameCount >= 60)
    {
        LL_INFOS("RenderBackend")
            << "Vulkan UI telemetry after "
            << context.mPresentedFrameCount
            << " frame(s): queued "
            << context.mQueuedUIDrawCount
            << " draw(s), indexed queued "
            << context.mQueuedIndexedUIDrawCount
            << ", recorded "
            << context.mRecordedUIDrawCount
            << ", pending this frame "
            << gPendingVulkanDraws.size()
            << ", recorded this frame "
            << ui_draw_count
            << ", classic avatar skinning draws "
            << classic_avatar_skinning_draw_count
            << ", rigged skinning draws "
            << rigged_skinning_draw_count
            << ", skipped classic avatar skinning draws "
            << skipped_classic_avatar_skinning_draw_count
            << ", skipped rigged skinning draws "
            << skipped_rigged_skinning_draw_count
            << ", offscreen-tagged draws "
            << offscreen_tagged_draw_count
            << ", offscreen-tagged clears "
            << offscreen_tagged_clear_count
            << ", missing buffers "
            << context.mRecordMissingBufferCount
            << ", missing attributes "
            << context.mRecordMissingAttributeCount
            << ", buffers "
            << gVulkanBuffers.size()
            << ", buffer memory "
            << (context.mBufferMemoryAllocatedBytes / MARE_VULKAN_BYTES_PER_MEGABYTE)
            << "MB/"
            << ((context.mEffectiveBufferMemoryBudgetBytes ?
                    context.mEffectiveBufferMemoryBudgetBytes :
                    get_vulkan_buffer_memory_budget_bytes()) /
                MARE_VULKAN_BYTES_PER_MEGABYTE)
            << "MB, budget-refused buffer allocations "
            << context.mSkippedBufferMemoryBudgetCount
            << ", textures "
            << gVulkanTextures.size()
            << ", evicted textures "
            << context.mEvictedTextureCount
            << " ("
            << (context.mEvictedTextureMemoryBytes / MARE_VULKAN_BYTES_PER_MEGABYTE)
            << "MB)"
            << ", texture uploads "
            << context.mTextureUploadCount
            << ", largest texture "
            << context.mLargestTextureHandle
            << " ("
            << context.mLargestTextureWidth
            << "x"
            << context.mLargestTextureHeight
            << "), texture memory "
            << (context.mTextureMemoryAllocatedBytes / MARE_VULKAN_BYTES_PER_MEGABYTE)
            << "MB/"
            << ((context.mEffectiveTextureMemoryBudgetBytes ?
                    context.mEffectiveTextureMemoryBudgetBytes :
                    get_vulkan_texture_memory_budget_bytes()) /
                MARE_VULKAN_BYTES_PER_MEGABYTE)
            << "MB, missing texture subimages "
            << context.mSkippedTextureSubImageMissingResourceCount
            << ", out-of-bounds texture subimages "
            << context.mSkippedTextureSubImageOutOfBoundsCount
            << ", unsupported texture uploads "
            << context.mSkippedTextureUnsupportedUploadCount
            << ", budget-refused texture uploads "
            << context.mSkippedTextureMemoryBudgetCount
            << ", throttled texture uploads "
            << context.mSkippedTextureUploadThrottleCount
            << ", oversized texture uploads "
            << context.mSkippedTextureOversizeCount
            << "."
            << LL_ENDL;
        if (largest_textured_bounds.mValid)
        {
            LL_INFOS("RenderBackend")
                << "Vulkan largest textured draw this frame: texture "
                << largest_textured_draw_texture
                << " ("
                << largest_textured_draw_texture_width
                << "x"
                << largest_textured_draw_texture_height
                << "), clip x "
                << largest_textured_bounds.mMinX
                << ".."
                << largest_textured_bounds.mMaxX
                << ", clip y "
                << largest_textured_bounds.mMinY
                << ".."
                << largest_textured_bounds.mMaxY
                << ", viewport "
                << largest_textured_draw_viewport.mWidth
                << "x"
                << largest_textured_draw_viewport.mHeight
                << "."
                << LL_ENDL;
        }
        if (largest_solid_draw_bounds.mValid)
        {
            LL_INFOS("RenderBackend")
                << "Vulkan largest solid draw this frame: color rgba "
                << static_cast<U32>(largest_solid_draw_color.mR)
                << ","
                << static_cast<U32>(largest_solid_draw_color.mG)
                << ","
                << static_cast<U32>(largest_solid_draw_color.mB)
                << ","
                << static_cast<U32>(largest_solid_draw_color.mA)
                << ", color valid "
                << largest_solid_draw_color.mValid
                << ", mode "
                << largest_solid_draw_mode
                << ", count "
                << largest_solid_draw_count
                << ", indexed "
                << largest_solid_draw_indexed
                << ", clip x "
                << largest_solid_draw_bounds.mMinX
                << ".."
                << largest_solid_draw_bounds.mMaxX
                << ", clip y "
                << largest_solid_draw_bounds.mMinY
                << ".."
                << largest_solid_draw_bounds.mMaxY
                << ", viewport "
                << largest_solid_draw_viewport.mWidth
                << "x"
                << largest_solid_draw_viewport.mHeight
                << ", scissor "
                << largest_solid_draw_scissor.mEnabled
                << " "
                << largest_solid_draw_scissor.mX
                << ","
                << largest_solid_draw_scissor.mY
                << " "
                << largest_solid_draw_scissor.mWidth
                << "x"
                << largest_solid_draw_scissor.mHeight
                << "."
                << LL_ENDL;
        }
        if (largest_solid_strip_bounds.mValid)
        {
            LL_INFOS("RenderBackend")
                << "Vulkan largest wide solid strip this frame: color rgba "
                << static_cast<U32>(largest_solid_strip_color.mR)
                << ","
                << static_cast<U32>(largest_solid_strip_color.mG)
                << ","
                << static_cast<U32>(largest_solid_strip_color.mB)
                << ","
                << static_cast<U32>(largest_solid_strip_color.mA)
                << ", color valid "
                << largest_solid_strip_color.mValid
                << ", mode "
                << largest_solid_strip_mode
                << ", count "
                << largest_solid_strip_count
                << ", indexed "
                << largest_solid_strip_indexed
                << ", clip x "
                << largest_solid_strip_bounds.mMinX
                << ".."
                << largest_solid_strip_bounds.mMaxX
                << ", clip y "
                << largest_solid_strip_bounds.mMinY
                << ".."
                << largest_solid_strip_bounds.mMaxY
                << ", viewport "
                << largest_solid_strip_viewport.mWidth
                << "x"
                << largest_solid_strip_viewport.mHeight
                << ", scissor "
                << largest_solid_strip_scissor.mEnabled
                << " "
                << largest_solid_strip_scissor.mX
                << ","
                << largest_solid_strip_scissor.mY
                << " "
                << largest_solid_strip_scissor.mWidth
                << "x"
                << largest_solid_strip_scissor.mHeight
                << "."
                << LL_ENDL;
        }
        if (largest_texture_draw_bounds.mValid)
        {
            LL_INFOS("RenderBackend")
                << "Vulkan largest texture draw this frame: texture "
                << largest_texture_draw_texture
                << " ("
                << largest_texture_draw_texture_width
                << "x"
                << largest_texture_draw_texture_height
                << "), clip x "
                << largest_texture_draw_bounds.mMinX
                << ".."
                << largest_texture_draw_bounds.mMaxX
                << ", clip y "
                << largest_texture_draw_bounds.mMinY
                << ".."
                << largest_texture_draw_bounds.mMaxY
                << ", viewport "
                << largest_texture_draw_viewport.mWidth
                << "x"
                << largest_texture_draw_viewport.mHeight
                << "."
                << LL_ENDL;
        }
        context.mLoggedUIDrawTelemetry = true;
    }

    context.mCmdEndRenderPass(command_buffer);

    result = context.mEndCommandBuffer(command_buffer);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkEndCommandBuffer failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    return true;
}

bool present_vulkan_frame(
    LLVulkanNativeContext& context,
    bool log_success)
{
    if (is_vulkan_swapchain_extent_stale(context) &&
        !recreate_vulkan_swapchain_resources(context))
    {
        return false;
    }

    if (context.mFrameSync.empty() ||
        context.mCommandBuffers.empty() ||
        !context.mAcquireNextImage ||
        !context.mQueueSubmit ||
        !context.mQueuePresent ||
        !context.mWaitForFences ||
        !context.mResetFences)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend cannot present a frame without frame resources."
            << LL_ENDL;
        return false;
    }

    U32 frame_slot = static_cast<U32>(context.mPresentedFrameCount % context.mFrameSync.size());
    LLVulkanFrameSync& frame_sync = context.mFrameSync[frame_slot];
    if (!wait_for_vulkan_fence_sleeping(
            context,
            frame_sync.mInFlightFence,
            "before frame"))
    {
        return false;
    }
    destroy_vulkan_transient_frame_buffers(context);

    S32 result = context.mResetFences(context.mDevice, 1, &frame_sync.mInFlightFence);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkResetFences failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    U32 image_index = 0;
    result = context.mAcquireNextImage(
        context.mDevice,
        context.mSwapchain,
        LL_VK_TIMEOUT_FOREVER,
        frame_sync.mImageAvailableSemaphore,
        nullptr,
        &image_index);
    if (!is_vulkan_swapchain_success(result))
    {
        if (is_vulkan_swapchain_stale_result(result))
        {
            gPendingVulkanDraws.clear();
            return recreate_vulkan_swapchain_resources(context);
        }

        LL_WARNS("RenderBackend")
            << "vkAcquireNextImageKHR failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    if (!record_vulkan_frame_command_buffer(context, image_index))
    {
        return false;
    }

    LLVkCommandBuffer command_buffer = context.mCommandBuffers[image_index];
    U32 wait_stage = LL_VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    LLVkSubmitInfo submit_info =
    {
        LL_VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        1,
        &frame_sync.mImageAvailableSemaphore,
        &wait_stage,
        1,
        &command_buffer,
        1,
        &frame_sync.mRenderFinishedSemaphore
    };

    result = context.mQueueSubmit(
        context.mGraphicsQueue,
        1,
        &submit_info,
        frame_sync.mInFlightFence);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkQueueSubmit failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    if (!wait_for_vulkan_fence_sleeping(
            context,
            frame_sync.mInFlightFence,
            "after submit"))
    {
        return false;
    }

    LLVkPresentInfoKHR present_info =
    {
        LL_VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        1,
        &frame_sync.mRenderFinishedSemaphore,
        1,
        &context.mSwapchain,
        &image_index,
        nullptr
    };

    result = context.mQueuePresent(context.mPresentQueue, &present_info);
    if (!is_vulkan_swapchain_success(result))
    {
        if (is_vulkan_swapchain_stale_result(result))
        {
            gPendingVulkanDraws.clear();
            return recreate_vulkan_swapchain_resources(context);
        }

        LL_WARNS("RenderBackend")
            << "vkQueuePresentKHR failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    gPendingVulkanDraws.clear();

    ++context.mPresentedFrameCount;
    if (log_success)
    {
        LL_INFOS("RenderBackend")
            << "Vulkan frame acquired, submitted, and presented swapchain image "
            << image_index
            << " using frame slot "
            << frame_slot
            << "."
            << LL_ENDL;
    }
    return true;
}

void destroy_vulkan_command_buffers(LLVulkanNativeContext& context)
{
    if (context.mDeviceWaitIdle && context.mDevice)
    {
        context.mDeviceWaitIdle(context.mDevice);
    }

    if (context.mFreeCommandBuffers &&
        context.mDevice &&
        context.mCommandPool &&
        !context.mCommandBuffers.empty())
    {
        context.mFreeCommandBuffers(
            context.mDevice,
            context.mCommandPool,
            static_cast<U32>(context.mCommandBuffers.size()),
            context.mCommandBuffers.data());
    }

    context.mCommandBuffers.clear();
    context.mAllocateCommandBuffers = nullptr;
    context.mFreeCommandBuffers = nullptr;

    if (context.mDestroyCommandPool && context.mDevice && context.mCommandPool)
    {
        context.mDestroyCommandPool(context.mDevice, context.mCommandPool, nullptr);
    }

    context.mDestroyCommandPool = nullptr;
    context.mCommandPool = nullptr;
    context.mBeginCommandBuffer = nullptr;
    context.mEndCommandBuffer = nullptr;
    context.mResetCommandBuffer = nullptr;
    context.mCmdPipelineBarrier = nullptr;
    context.mCmdBeginRenderPass = nullptr;
    context.mCmdEndRenderPass = nullptr;
}

void destroy_vulkan_swapchain(LLVulkanNativeContext& context)
{
    context.mSwapchainImages.clear();
    context.mSwapchainExtent = {0, 0};
    context.mNativeViewExtent = {0, 0};
    context.mDrawableScaleX = 1.f;
    context.mDrawableScaleY = 1.f;
    context.mSwapchainImageFormat = LL_VK_FORMAT_UNDEFINED;

    if (context.mDestroySwapchain && context.mDevice && context.mSwapchain)
    {
        context.mDestroySwapchain(context.mDevice, context.mSwapchain, nullptr);
    }

    context.mDestroySwapchain = nullptr;
    context.mSwapchain = nullptr;
}

void destroy_vulkan_instance(LLVulkanNativeContext& context)
{
    context.mPhysicalDevices.clear();

    if (context.mDestroyInstance && context.mInstance)
    {
        context.mDestroyInstance(context.mInstance, nullptr);
    }

    context.mEnumeratePhysicalDevices = nullptr;
    context.mDestroyInstance = nullptr;
    context.mInstance = nullptr;
}

void destroy_vulkan_native_context_resources(LLVulkanNativeContext& context)
{
    destroy_all_vulkan_buffer_resources(context);
    destroy_all_vulkan_texture_resources(context);
    destroy_vulkan_command_buffers(context);
    destroy_vulkan_frame_sync(context);
    destroy_vulkan_graphics_pipelines(context);
    destroy_vulkan_swapchain_framebuffers(context);
    destroy_vulkan_depth_attachment(context);
    destroy_vulkan_render_pass(context);
    destroy_vulkan_swapchain_image_views(context);
    destroy_vulkan_swapchain(context);
    destroy_vulkan_device(context);
    destroy_vulkan_surface(context);
    destroy_vulkan_instance(context);
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
    bool isReady() const override { return should_continue_after_vulkan_probe(); }
    bool initContextCapabilities() override
    {
        if (should_continue_after_vulkan_probe())
        {
            LL_WARNS("RenderBackend")
                << "MARE_VULKAN_CONTINUE_AFTER_PROBE is enabled. "
                << "Continuing with Vulkan swapchain presentation, the UI bridge, and the guarded world command path."
                << LL_ENDL;
            return true;
        }

        LL_WARNS("RenderBackend")
            << "Vulkan backend stopped before capability initialization. "
            << "Only bootstrap UI rendering and the guarded world command path are implemented; full scene rendering is not implemented yet. "
            << "Set MARE_VULKAN_CONTINUE_AFTER_PROBE=1 to probe the next startup blocker."
            << LL_ENDL;
        return false;
    }

    bool createNativeContext(
        const LLRenderNativeContextDesc& desc,
        LLRenderNativeContext& context) override
    {
        if (!get_vulkan_loader().isAvailable())
        {
            LL_WARNS("RenderBackend")
                << "Vulkan backend requested, but no usable Vulkan loader was found."
                << LL_ENDL;
            return false;
        }

#if LL_DARWIN
        void* view = ll_render_macosx_create_metal_native_view(desc.mWindow);
        if (!view)
        {
            LL_WARNS("RenderBackend")
                << "Vulkan backend could not create a CAMetalLayer-backed native view."
                << LL_ENDL;
            return false;
        }

        LLVulkanNativeContext* native_context = new LLVulkanNativeContext();
        native_context->mNativeView = view;
        native_context->mEnableVSync = desc.mEnableVSync;
        native_context->mMetalLayer = ll_render_macosx_get_metal_layer(view);
        if (!native_context->mMetalLayer)
        {
            LL_WARNS("RenderBackend")
                << "Vulkan backend created a native view without a CAMetalLayer."
                << LL_ENDL;
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_instance(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_surface(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_device(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_swapchain(*native_context, view, desc.mEnableVSync))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_swapchain_image_views(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_render_pass(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_graphics_pipelines(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_depth_attachment(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_swapchain_framebuffers(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_command_buffers(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_frame_sync(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_default_ui_attribute_buffers(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!create_vulkan_fallback_texture(*native_context))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        if (!present_vulkan_frame(*native_context, true))
        {
            destroy_vulkan_native_context_resources(*native_context);
            delete native_context;
            ll_render_macosx_destroy_native_view(view);
            return false;
        }

        context.mView = view;
        context.mContext = native_context;
        context.mPixelFormat = nullptr;
        context.mVRAM = native_context->mReportedVideoMemoryMB;

        LL_WARNS("RenderBackend")
            << "Vulkan backend created a macOS CAMetalLayer native view, VkInstance, surface, device, swapchain, "
            << "image views, render pass, bootstrap/UI pipelines, framebuffers, command buffers, and frame sync. "
            << "A first clear frame was presented. Real scene rendering is still pending."
            << LL_ENDL;
        LL_INFOS("RenderBackend")
            << "Vulkan reported texture memory budget for viewer info: "
            << context.mVRAM
            << "MB."
            << LL_ENDL;
        return true;
#else
        LL_WARNS("RenderBackend")
            << "Vulkan native context creation is not implemented on this platform yet."
            << LL_ENDL;
        return false;
#endif
    }

    void destroyNativeContext(LLRenderNativeContext& context) override
    {
        LLVulkanNativeContext* native_context = static_cast<LLVulkanNativeContext*>(context.mContext);
        if (native_context)
        {
            destroy_all_vulkan_buffer_resources(*native_context);
            destroy_all_vulkan_texture_resources(*native_context);
            destroy_vulkan_native_context_resources(*native_context);
            if (gCurrentVulkanContext == native_context)
            {
                gCurrentVulkanContext = nullptr;
            }
            delete native_context;
        }
        context.mContext = nullptr;

#if LL_DARWIN
        if (context.mView)
        {
            ll_render_macosx_destroy_native_view(context.mView);
        }
#endif

        context.mView = nullptr;
        context.mPixelFormat = nullptr;
        context.mVRAM = 0;
    }

    bool makeNativeContextCurrent(void* context) override
    {
        gCurrentVulkanContext = static_cast<LLVulkanNativeContext*>(context);
        return gCurrentVulkanContext != nullptr;
    }

    void clearCurrentNativeContext() override {}

    void swapNativeBuffers(void* context) override
    {
        LLVulkanNativeContext* native_context = static_cast<LLVulkanNativeContext*>(context);
        if (!native_context || native_context->mFrameRenderingFailed)
        {
            return;
        }

        if (!present_vulkan_frame(*native_context, false))
        {
            native_context->mFrameRenderingFailed = true;
            LL_WARNS("RenderBackend")
                << "Vulkan frame presentation failed; suppressing further swap attempts."
                << LL_ENDL;
        }
    }

    void setNativeVSync(void*, bool) override {}
    bool setNativeContextThreadedOptimization(bool) override { return false; }
    void* createSharedNativeContext(void*, void*, bool) override { return nullptr; }
    void destroySharedNativeContext(void*) override {}

    void setPixelStoreInteger(LLRenderPixelStoreParameter parameter, S32 value) override
    {
        if (parameter == LLRenderPixelStoreParameter::UnpackRowLength)
        {
            gVulkanUnpackRowLength = llmax(0, value);
        }
    }

    void setViewport(const LLRenderViewport& viewport) override
    {
        gCurrentVulkanViewport = viewport;
    }

    void setViewport(S32 x, S32 y, S32 width, S32 height) override
    {
        gCurrentVulkanViewport =
        {
            static_cast<F32>(x),
            static_cast<F32>(y),
            static_cast<F32>(width),
            static_cast<F32>(height),
            0.f,
            1.f
        };
    }

    void setScissor(const LLRenderScissor& scissor) override
    {
        gCurrentVulkanScissor = scissor;
    }

    void setScissor(S32 x, S32 y, S32 width, S32 height) override
    {
        gCurrentVulkanScissor =
        {
            x,
            y,
            width,
            height,
            true
        };
    }

    void clear(const LLRenderPassDesc& desc) override
    {
        if (desc.mClearMask == LL_RENDER_CLEAR_NONE)
        {
            return;
        }

        LLVulkanPendingDraw clear_command;
        clear_command.mClearOnly = true;
        clear_command.mClearMask = desc.mClearMask;
        clear_command.mClearColor = desc.mClearColor;
        clear_command.mClearDepth = desc.mClearDepth;
        clear_command.mClearStencil = static_cast<U32>(desc.mClearStencil);
        clear_command.mFramebuffer = gBoundVulkanDrawFramebuffer;
        clear_command.mViewport =
            desc.mViewport.mWidth > 0.f && desc.mViewport.mHeight > 0.f ?
            desc.mViewport :
            gCurrentVulkanViewport;
        clear_command.mScissor =
            desc.mScissor.mEnabled ?
            desc.mScissor :
            gCurrentVulkanScissor;
        gPendingVulkanDraws.push_back(clear_command);
    }

    void clear(LLRenderClearMask clear_mask) override
    {
        if (clear_mask == LL_RENDER_CLEAR_NONE)
        {
            return;
        }

        LLVulkanPendingDraw clear_command;
        clear_command.mClearOnly = true;
        clear_command.mClearMask = clear_mask;
        clear_command.mClearColor = gCurrentVulkanClearColor;
        clear_command.mClearDepth = 1.f;
        clear_command.mClearStencil = 0;
        clear_command.mFramebuffer = gBoundVulkanDrawFramebuffer;
        clear_command.mViewport = gCurrentVulkanViewport;
        clear_command.mScissor = gCurrentVulkanScissor;
        gPendingVulkanDraws.push_back(clear_command);
    }

    void setClearColor(const LLRenderClearColor& color) override
    {
        gCurrentVulkanClearColor = color;
    }

    void setClearColor(F32 red, F32 green, F32 blue, F32 alpha) override
    {
        gCurrentVulkanClearColor = { red, green, blue, alpha };
    }

    void setBlendState(const LLRenderBlendState& blend) override
    {
        gCurrentVulkanBlendState = blend;
    }

    void setCapability(LLRenderCapability capability, bool enabled) override
    {
        switch (capability)
        {
        case LLRenderCapability::Blend:
            gCurrentVulkanBlendEnabled = enabled;
            break;
        case LLRenderCapability::CullFace:
            gCurrentVulkanCullFaceEnabled = enabled;
            break;
        case LLRenderCapability::DepthTest:
            gCurrentVulkanDepthTestEnabled = enabled;
            break;
        case LLRenderCapability::ScissorTest:
            gCurrentVulkanScissor.mEnabled = enabled;
            break;
        default:
            break;
        }
    }

    bool isCapabilityEnabled(LLRenderCapability capability) const override
    {
        switch (capability)
        {
        case LLRenderCapability::Blend:
            return gCurrentVulkanBlendEnabled;
        case LLRenderCapability::CullFace:
            return gCurrentVulkanCullFaceEnabled;
        case LLRenderCapability::DepthTest:
            return gCurrentVulkanDepthTestEnabled;
        case LLRenderCapability::ScissorTest:
            return gCurrentVulkanScissor.mEnabled;
        default:
            return false;
        }
    }

    void setCullFace(LLRenderCullFace face) override
    {
        gCurrentVulkanCullFace = face;
    }

    void setDepthFunction(LLRenderDepthFunction function) override
    {
        gCurrentVulkanDepthFunction = function;
    }

    void setDepthWriteEnabled(bool enabled) override
    {
        gCurrentVulkanDepthWriteEnabled = enabled;
    }

    void setAlphaMaskCutoff(F32 cutoff) override
    {
        gCurrentVulkanAlphaMaskCutoff = cutoff;
    }

    void setWorldDrawEnabled(bool enabled) override
    {
        gCurrentVulkanWorldDrawEnabled = enabled;
    }

    void setWorldShaderClass(LLRenderWorldShaderClass shader_class) override
    {
        gCurrentVulkanWorldShaderClass = shader_class;
    }

    void setWorldTerrainParameters(const LLRenderWorldTerrainParameters& parameters) override
    {
        gCurrentVulkanTerrainParameters = parameters;
    }

    void setWorldMaterialParameters(const LLRenderWorldMaterialParameters& parameters) override
    {
        gCurrentVulkanMaterialParameters = parameters;
    }

    void setWorldTextureTransform(const LLRenderWorldTextureTransform& transform) override
    {
        gCurrentVulkanTextureTransform = transform;
    }

    void setWorldSkinningMatrixPalette(U32 count, const F32* values) override
    {
        if (!values || count == 0)
        {
            gCurrentVulkanSkinningMatrixPalette.clear();
            gCurrentVulkanSkinningMatrixCount = 0;
            return;
        }

        gCurrentVulkanSkinningMatrixCount =
            llmin(count, MARE_VULKAN_MAX_SKINNING_MATRICES);
        gCurrentVulkanSkinningMatrixPalette.assign(
            values,
            values + static_cast<size_t>(gCurrentVulkanSkinningMatrixCount) * 12);
    }

    void setLegacyCapability(U32 capability, bool enabled) override
    {
        if (capability == LL_LEGACY_GL_SCISSOR_TEST)
        {
            gCurrentVulkanScissor.mEnabled = enabled;
        }
    }

    bool isLegacyCapabilityEnabled(U32 capability) override
    {
        if (capability == LL_LEGACY_GL_SCISSOR_TEST)
        {
            return gCurrentVulkanScissor.mEnabled;
        }
        return false;
    }

    S32 getActiveTextureUnit() const override
    {
        return gActiveVulkanTextureUnit;
    }

    void setActiveTextureUnit(S32 unit) override
    {
        gActiveVulkanTextureUnit = llclamp(unit, 0, static_cast<S32>(gBoundVulkanTextures.size() - 1));
    }

    void bindTexture(LLRenderTextureTarget, U32 texture) override
    {
        gBoundVulkanTextures[gActiveVulkanTextureUnit] = texture;
    }

    void generateTextures(S32 count, U32* textures) override
    {
        if (count <= 0 || !textures)
        {
            return;
        }

        for (S32 i = 0; i < count; ++i)
        {
            textures[i] = gNextVulkanTextureHandle++;
        }
    }

    void deleteTextures(S32 count, const U32* textures) override
    {
        if (count <= 0 || !textures || !gCurrentVulkanContext)
        {
            return;
        }

        for (S32 i = 0; i < count; ++i)
        {
            auto iter = gVulkanTextures.find(textures[i]);
            if (iter != gVulkanTextures.end())
            {
                destroy_vulkan_texture_resource(*gCurrentVulkanContext, iter->second);
                gVulkanTextures.erase(iter);
            }
            gVulkanTextureSamplerStates.erase(textures[i]);
        }
    }

    bool isTextureResident(U32 texture) const override
    {
        if (texture == 0)
        {
            return true;
        }

        auto iter = gVulkanTextures.find(texture);
        return iter != gVulkanTextures.end() &&
            iter->second.mImage &&
            iter->second.mImageView &&
            iter->second.mSampler;
    }

    void setTextureAddressMode(
        LLRenderTextureTarget,
        LLRenderTextureAddressMode mode) override
    {
        U32 texture = gBoundVulkanTextures[gActiveVulkanTextureUnit];
        if (!texture)
        {
            return;
        }

        LLVulkanTextureSamplerState& state = gVulkanTextureSamplerStates[texture];
        if (state.mAddressModeS == mode &&
            state.mAddressModeT == mode &&
            state.mAddressModeW == mode)
        {
            return;
        }

        state.mAddressModeS = mode;
        state.mAddressModeT = mode;
        state.mAddressModeW = mode;

        if (!gCurrentVulkanContext)
        {
            return;
        }

        auto texture_iter = gVulkanTextures.find(texture);
        if (texture_iter != gVulkanTextures.end())
        {
            update_vulkan_texture_sampler(
                *gCurrentVulkanContext,
                texture,
                texture_iter->second);
        }
    }

    void setTextureAddressMode(
        LLRenderTextureTarget,
        LLRenderTextureCoordinate coordinate,
        LLRenderTextureAddressMode mode) override
    {
        U32 texture = gBoundVulkanTextures[gActiveVulkanTextureUnit];
        if (!texture)
        {
            return;
        }

        LLVulkanTextureSamplerState& state = gVulkanTextureSamplerStates[texture];
        LLRenderTextureAddressMode& target_mode =
            coordinate == LLRenderTextureCoordinate::T ?
                state.mAddressModeT :
                state.mAddressModeS;
        if (target_mode == mode)
        {
            return;
        }

        target_mode = mode;

        if (!gCurrentVulkanContext)
        {
            return;
        }

        auto texture_iter = gVulkanTextures.find(texture);
        if (texture_iter != gVulkanTextures.end())
        {
            update_vulkan_texture_sampler(
                *gCurrentVulkanContext,
                texture,
                texture_iter->second);
        }
    }

    void setTextureFilter(
        LLRenderTextureTarget,
        LLRenderTextureFilter min_filter,
        LLRenderTextureFilter mag_filter) override
    {
        U32 texture = gBoundVulkanTextures[gActiveVulkanTextureUnit];
        if (!texture)
        {
            return;
        }

        LLVulkanTextureSamplerState& state = gVulkanTextureSamplerStates[texture];
        if (state.mMinFilter == min_filter && state.mMagFilter == mag_filter)
        {
            return;
        }

        state.mMinFilter = min_filter;
        state.mMagFilter = mag_filter;

        if (!gCurrentVulkanContext)
        {
            return;
        }

        auto texture_iter = gVulkanTextures.find(texture);
        if (texture_iter != gVulkanTextures.end())
        {
            update_vulkan_texture_sampler(
                *gCurrentVulkanContext,
                texture,
                texture_iter->second);
        }
    }

    void setTextureMagFilter(
        LLRenderTextureTarget target,
        LLRenderTextureFilter mag_filter) override
    {
        U32 texture = gBoundVulkanTextures[gActiveVulkanTextureUnit];
        LLVulkanTextureSamplerState state = get_vulkan_texture_sampler_state(texture);
        setTextureFilter(target, state.mMinFilter, mag_filter);
    }

    void setTextureImage2D(
        LLRenderTextureTarget,
        S32 level,
        S32,
        S32 width,
        S32 height,
        S32,
        U32 format,
        U32 type,
        const void* data) override
    {
        if (gCurrentVulkanContext)
        {
            gCurrentVulkanContext->mLastTextureUploadSucceeded = true;
        }

        if (!gCurrentVulkanContext)
        {
            return;
        }

        if (level != 0)
        {
            return;
        }

        if (width <= 0 || height <= 0)
        {
            return;
        }

        U32 texture = gBoundVulkanTextures[gActiveVulkanTextureUnit];
        if (!texture)
        {
            return;
        }

        gCurrentVulkanContext->mLastTextureUploadSucceeded = false;
        gCurrentVulkanContext->mLastTextureUploadDeferred = false;

        auto existing_texture = gVulkanTextures.find(texture);
        if (data == nullptr &&
            existing_texture != gVulkanTextures.end() &&
            existing_texture->second.mWidth == width &&
            existing_texture->second.mHeight == height)
        {
            gCurrentVulkanContext->mLastTextureUploadSucceeded = true;
            return;
        }

        const U64 upload_bytes = static_cast<U64>(width) * static_cast<U64>(height) * 4;
        const bool is_glyph_texture = is_vulkan_glyph_texture_format(format);
        if (!can_accept_vulkan_texture_upload_request(
                *gCurrentVulkanContext,
                texture,
                width,
                height,
                upload_bytes,
                true,
                data != nullptr && !is_glyph_texture,
                data != nullptr && !is_glyph_texture))
        {
            return;
        }

        std::vector<U8> pixels;
        if (!convert_texture_pixels_to_rgba8(width, height, width, format, type, data, pixels))
        {
            ++gCurrentVulkanContext->mSkippedTextureUnsupportedUploadCount;
            LL_WARNS_ONCE("RenderBackend")
                << "Vulkan UI texture bridge skipped unsupported legacy texture upload format "
                << format
                << ", type "
                << type
                << "."
                << LL_ENDL;
            return;
        }

        gCurrentVulkanContext->mLastTextureUploadSucceeded =
            upload_vulkan_texture_resource(*gCurrentVulkanContext, texture, width, height, pixels);
    }

    void setTextureImage2D(
        LLRenderTextureTarget target,
        S32 level,
        LLRenderTextureFormat internal_format,
        S32 width,
        S32 height,
        S32 border,
        LLRenderPixelFormat format,
        LLRenderPixelType type,
        const void* data) override
    {
        if (!data &&
            level == 0 &&
            gCurrentVulkanContext &&
            width > 0 &&
            height > 0)
        {
            U32 texture = gBoundVulkanTextures[gActiveVulkanTextureUnit];
            if (!texture)
            {
                gCurrentVulkanContext->mLastTextureUploadSucceeded = false;
                gCurrentVulkanContext->mLastTextureUploadDeferred = false;
                return;
            }

            auto existing_texture = gVulkanTextures.find(texture);
            const S32 expected_format = to_vulkan_image_format(internal_format);
            if (existing_texture != gVulkanTextures.end() &&
                existing_texture->second.mWidth == width &&
                existing_texture->second.mHeight == height &&
                existing_texture->second.mFormat == expected_format)
            {
                gCurrentVulkanContext->mLastTextureUploadSucceeded = true;
                gCurrentVulkanContext->mLastTextureUploadDeferred = false;
                return;
            }

            gCurrentVulkanContext->mLastTextureUploadSucceeded =
                create_empty_vulkan_texture_resource(
                    *gCurrentVulkanContext,
                    texture,
                    width,
                    height,
                    internal_format);
            gCurrentVulkanContext->mLastTextureUploadDeferred = false;
            return;
        }

        U32 legacy_format = LL_LEGACY_GL_RGBA;
        switch (format)
        {
        case LLRenderPixelFormat::Alpha:
            legacy_format = LL_LEGACY_GL_ALPHA;
            break;
        case LLRenderPixelFormat::Luminance:
            legacy_format = LL_LEGACY_GL_LUMINANCE;
            break;
        case LLRenderPixelFormat::Red:
            legacy_format = LL_LEGACY_GL_RED;
            break;
        case LLRenderPixelFormat::RG:
            legacy_format = LL_LEGACY_GL_RG;
            break;
        case LLRenderPixelFormat::RGB:
            legacy_format = LL_LEGACY_GL_RGB;
            break;
        case LLRenderPixelFormat::RGBA:
        default:
            legacy_format = LL_LEGACY_GL_RGBA;
            break;
        }

        U32 legacy_type = type == LLRenderPixelType::UnsignedByte ?
            LL_LEGACY_GL_UNSIGNED_BYTE :
            0;
        setTextureImage2D(target, level, 0, width, height, border, legacy_format, legacy_type, data);
    }

    bool didLastTextureUploadSucceed() const override
    {
        return !gCurrentVulkanContext || gCurrentVulkanContext->mLastTextureUploadSucceeded;
    }

    bool shouldRetryLastTextureUploadLater() const override
    {
        return gCurrentVulkanContext && gCurrentVulkanContext->mLastTextureUploadDeferred;
    }

    void setCompressedTextureImage2D(
        LLRenderTextureTarget,
        S32,
        S32,
        S32,
        S32,
        S32,
        S32,
        const void*) override
    {
        if (gCurrentVulkanContext)
        {
            gCurrentVulkanContext->mLastTextureUploadSucceeded = false;
            gCurrentVulkanContext->mLastTextureUploadDeferred = false;
            ++gCurrentVulkanContext->mSkippedTextureUnsupportedUploadCount;
        }
    }

    void setTextureSubImage2D(
        LLRenderTextureTarget,
        S32 level,
        S32 xoffset,
        S32 yoffset,
        S32 width,
        S32 height,
        U32 format,
        U32 type,
        const void* pixels) override
    {
        if (!gCurrentVulkanContext || level != 0 || !pixels)
        {
            return;
        }

        gCurrentVulkanContext->mLastTextureUploadSucceeded = false;
        gCurrentVulkanContext->mLastTextureUploadDeferred = false;

        U32 texture = gBoundVulkanTextures[gActiveVulkanTextureUnit];
        auto iter = gVulkanTextures.find(texture);
        if (iter == gVulkanTextures.end())
        {
            ++gCurrentVulkanContext->mSkippedTextureSubImageMissingResourceCount;
            return;
        }

        LLVulkanTextureResource& resource = iter->second;
        if (xoffset < 0 ||
            yoffset < 0 ||
            width <= 0 ||
            height <= 0 ||
            xoffset + width > resource.mWidth ||
            yoffset + height > resource.mHeight)
        {
            ++gCurrentVulkanContext->mSkippedTextureSubImageOutOfBoundsCount;
            return;
        }

        const U64 upload_bytes = static_cast<U64>(width) * static_cast<U64>(height) * 4;
        const bool is_glyph_texture = is_vulkan_glyph_texture_format(format);
        if (!can_accept_vulkan_texture_upload_request(
                *gCurrentVulkanContext,
                texture,
                width,
                height,
                upload_bytes,
                false,
                false,
                !is_glyph_texture))
        {
            return;
        }

        std::vector<U8> converted;
        const S32 source_row_length = gVulkanUnpackRowLength > 0 ? gVulkanUnpackRowLength : width;
        if (!convert_texture_pixels_to_rgba8(width, height, source_row_length, format, type, pixels, converted))
        {
            ++gCurrentVulkanContext->mSkippedTextureUnsupportedUploadCount;
            return;
        }

        gCurrentVulkanContext->mLastTextureUploadSucceeded =
            upload_vulkan_texture_pixels_to_image(
                *gCurrentVulkanContext,
                resource,
                converted,
                xoffset,
                yoffset,
                width,
                height,
                LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void copyImageSubData(
        LLRenderTextureHandle source_texture,
        LLRenderTextureTarget,
        S32 source_level,
        S32 source_x,
        S32 source_y,
        S32,
        LLRenderTextureHandle destination_texture,
        LLRenderTextureTarget,
        S32 destination_level,
        S32 destination_x,
        S32 destination_y,
        S32,
        S32 width,
        S32 height,
        S32) override
    {
        if (!gCurrentVulkanContext ||
            source_level != 0 ||
            destination_level != 0 ||
            width <= 0 ||
            height <= 0)
        {
            return;
        }

        auto source_iter = gVulkanTextures.find(source_texture.asLegacyName());
        auto destination_iter = gVulkanTextures.find(destination_texture.asLegacyName());
        if (source_iter == gVulkanTextures.end() ||
            destination_iter == gVulkanTextures.end())
        {
            return;
        }

        LLVulkanTextureResource& source = source_iter->second;
        LLVulkanTextureResource& destination = destination_iter->second;
        if (source_x < 0 ||
            source_y < 0 ||
            destination_x < 0 ||
            destination_y < 0 ||
            source_x + width > source.mWidth ||
            source_y + height > source.mHeight ||
            destination_x + width > destination.mWidth ||
            destination_y + height > destination.mHeight)
        {
            return;
        }

        copy_vulkan_texture_resource(
            *gCurrentVulkanContext,
            source,
            source_x,
            source_y,
            destination,
            destination_x,
            destination_y,
            width,
            height);
    }

    void copyTextureSubImage2D(
        LLRenderTextureTarget,
        S32 level,
        S32 xoffset,
        S32 yoffset,
        S32 x,
        S32 y,
        S32 width,
        S32 height) override
    {
        if (!gCurrentVulkanContext ||
            level != 0 ||
            width <= 0 ||
            height <= 0)
        {
            return;
        }

        U32 destination_texture = gBoundVulkanTextures[gActiveVulkanTextureUnit];
        if (!destination_texture || !gBoundVulkanReadFramebuffer)
        {
            return;
        }

        auto framebuffer_iter = gVulkanFramebuffers.find(gBoundVulkanReadFramebuffer);
        if (framebuffer_iter == gVulkanFramebuffers.end())
        {
            return;
        }

        U32 source_texture = framebuffer_iter->second.mColorTextures[0];
        auto source_iter = gVulkanTextures.find(source_texture);
        auto destination_iter = gVulkanTextures.find(destination_texture);
        if (source_iter == gVulkanTextures.end() ||
            destination_iter == gVulkanTextures.end())
        {
            return;
        }

        LLVulkanTextureResource& source = source_iter->second;
        LLVulkanTextureResource& destination = destination_iter->second;
        if (x < 0 ||
            y < 0 ||
            xoffset < 0 ||
            yoffset < 0 ||
            x + width > source.mWidth ||
            y + height > source.mHeight ||
            xoffset + width > destination.mWidth ||
            yoffset + height > destination.mHeight)
        {
            return;
        }

        copy_vulkan_texture_resource(
            *gCurrentVulkanContext,
            source,
            x,
            y,
            destination,
            xoffset,
            yoffset,
            width,
            height);
    }

    U32 getBoundTexture2D() override
    {
        return gBoundVulkanTextures[gActiveVulkanTextureUnit];
    }

    void generateBuffers(S32 count, U32* buffers) override
    {
        if (count <= 0 || !buffers)
        {
            return;
        }

        for (S32 i = 0; i < count; ++i)
        {
            buffers[i] = gNextVulkanBufferHandle++;
        }
    }

    void deleteBuffers(S32 count, const U32* buffers) override
    {
        if (count <= 0 || !buffers || !gCurrentVulkanContext)
        {
            return;
        }

        for (S32 i = 0; i < count; ++i)
        {
            gPendingVulkanBufferAllocations.erase(buffers[i]);
            auto iter = gVulkanBuffers.find(buffers[i]);
            if (iter != gVulkanBuffers.end())
            {
                destroy_vulkan_buffer_resource(*gCurrentVulkanContext, iter->second);
                gVulkanBuffers.erase(iter);
            }
        }
    }

    void bindBuffer(LLRenderBufferTarget target, U32 buffer) override
    {
        if (target == LLRenderBufferTarget::Index)
        {
            gBoundVulkanIndexBuffer = buffer;
        }
        else if (target == LLRenderBufferTarget::Vertex)
        {
            gBoundVulkanVertexBuffer = buffer;
        }
    }

    void allocateBufferStorage(
        LLRenderBufferTarget target,
        U64 size,
        const void* data,
        LLRenderBufferUsage usage) override
    {
        if (!gCurrentVulkanContext || size == 0)
        {
            return;
        }

        U32 handle = target == LLRenderBufferTarget::Index ?
            gBoundVulkanIndexBuffer :
            gBoundVulkanVertexBuffer;
        if (!handle)
        {
            return;
        }

        LLVulkanBufferResource& resource = gVulkanBuffers[handle];
        const bool had_existing_resource = resource.mBuffer != nullptr || resource.mMemory != nullptr;
        const U64 replaced_memory_size =
            resource.mMemoryAccounted ?
            resource.mMemorySize :
            0;
        LLVulkanBufferResource new_resource;
        if (!create_vulkan_buffer_resource(
                *gCurrentVulkanContext,
                size,
                to_vulkan_buffer_usage(target),
                data,
                new_resource,
                replaced_memory_size,
                can_use_vulkan_reserved_buffer_memory(usage)))
        {
            track_vulkan_pending_buffer_allocation(
                handle,
                size,
                to_vulkan_buffer_usage(target),
                usage);
            if (!had_existing_resource)
            {
                gVulkanBuffers.erase(handle);
            }
            return;
        }

        destroy_vulkan_buffer_resource(*gCurrentVulkanContext, resource);
        resource = new_resource;
        gPendingVulkanBufferAllocations.erase(handle);
    }

    void updateBufferSubData(
        LLRenderBufferTarget target,
        U32 offset,
        U32 size,
        const void* data) override
    {
        U32 handle = target == LLRenderBufferTarget::Index ?
            gBoundVulkanIndexBuffer :
            gBoundVulkanVertexBuffer;
        if (!gCurrentVulkanContext || !handle || !data || size == 0)
        {
            return;
        }

        const U64 update_end = static_cast<U64>(offset) + size;
        auto iter = gVulkanBuffers.find(handle);
        if (iter == gVulkanBuffers.end() ||
            !iter->second.mMappedData ||
            update_end > iter->second.mSize)
        {
            if (!retry_vulkan_pending_buffer_allocation(
                    *gCurrentVulkanContext,
                    handle,
                    update_end))
            {
                return;
            }

            iter = gVulkanBuffers.find(handle);
            if (iter == gVulkanBuffers.end() ||
                !iter->second.mMappedData ||
                update_end > iter->second.mSize)
            {
                return;
            }
        }

        std::memcpy(
            static_cast<U8*>(iter->second.mMappedData) + offset,
            data,
            size);
    }

    void enableVertexAttributeArray(U32 location) override
    {
        if (location < gCurrentVulkanVertexAttributes.size())
        {
            gCurrentVulkanVertexAttributes[location].mEnabled = true;
        }
    }

    void disableVertexAttributeArray(U32 location) override
    {
        if (location < gCurrentVulkanVertexAttributes.size())
        {
            gCurrentVulkanVertexAttributes[location].mEnabled = false;
        }
    }

    void setVertexAttributePointer(
        U32 location,
        S32,
        LLRenderVertexAttributeType,
        bool,
        S32 stride,
        const void* pointer) override
    {
        if (location >= gCurrentVulkanVertexAttributes.size())
        {
            return;
        }

        LLVulkanVertexAttributeState& attribute = gCurrentVulkanVertexAttributes[location];
        attribute.mEnabled = true;
        attribute.mStride = static_cast<U32>(stride);
        attribute.mOffset = static_cast<U64>(reinterpret_cast<uintptr_t>(pointer));
    }

    void setIntegerVertexAttributePointer(
        U32 location,
        S32 size,
        LLRenderVertexAttributeType type,
        S32 stride,
        const void* pointer) override
    {
        setVertexAttributePointer(location, size, type, false, stride, pointer);
    }

    bool queueDraw(
        LLRenderPrimitiveType mode,
        S32 first,
        S32 count,
        bool indexed,
        LLRenderIndexType index_type,
        const void* indices)
    {
        if (!gCurrentVulkanContext ||
            !gBoundVulkanVertexBuffer ||
            gVulkanBuffers.find(gBoundVulkanVertexBuffer) == gVulkanBuffers.end() ||
            count <= 0)
        {
            static bool logged_skipped_draw_arrays = false;
            if (!logged_skipped_draw_arrays)
            {
                LL_WARNS("RenderBackend")
                    << "Vulkan UI bridge skipped drawArrays. Has context: "
                    << (gCurrentVulkanContext != nullptr)
                    << ", bound vertex buffer: "
                    << gBoundVulkanVertexBuffer
                    << ", known buffer: "
                    << (gVulkanBuffers.find(gBoundVulkanVertexBuffer) != gVulkanBuffers.end())
                    << ", count: "
                    << count
                    << "."
                    << LL_ENDL;
                logged_skipped_draw_arrays = true;
            }
            return false;
        }

        U32 index_buffer = 0;
        S32 first_index = 0;
        S32 vulkan_index_type = to_vulkan_index_type(index_type);
        if (indexed)
        {
            index_buffer = gBoundVulkanIndexBuffer;
            if (!index_buffer || gVulkanBuffers.find(index_buffer) == gVulkanBuffers.end())
            {
                static bool logged_skipped_indexed_draw = false;
                if (!logged_skipped_indexed_draw)
                {
                    LL_WARNS("RenderBackend")
                        << "Vulkan UI bridge skipped indexed draw. Bound index buffer: "
                        << index_buffer
                        << ", known buffer: "
                        << (gVulkanBuffers.find(index_buffer) != gVulkanBuffers.end())
                        << ", count: "
                        << count
                        << "."
                        << LL_ENDL;
                    logged_skipped_indexed_draw = true;
                }
                return false;
            }

            U64 index_offset = static_cast<U64>(reinterpret_cast<uintptr_t>(indices));
            U32 index_size = index_type == LLRenderIndexType::UnsignedInt ? 4 : 2;
            first_index = static_cast<S32>(index_offset / index_size);
        }

        static bool logged_queued_array_draw = false;
        static bool logged_queued_indexed_draw = false;
        bool& logged_queued_draw = indexed ? logged_queued_indexed_draw : logged_queued_array_draw;
        if (!logged_queued_draw)
        {
            LL_INFOS("RenderBackend")
                << "Vulkan UI bridge queued "
                << (indexed ? "indexed " : "")
                << "draw mode "
                << static_cast<U32>(mode)
                << ", first "
                << (indexed ? first_index : first)
                << ", count "
                << count
                << ", vertex buffer "
                << gBoundVulkanVertexBuffer
                << "."
                << LL_ENDL;
            logged_queued_draw = true;
        }

        LLVulkanPendingDraw draw;
        draw.mBuffer = gBoundVulkanVertexBuffer;
        draw.mIndexBuffer = index_buffer;
        for (U32 i = 0; i < draw.mTextures.size() && i < gBoundVulkanTextures.size(); ++i)
        {
            draw.mTextures[i] = gBoundVulkanTextures[i];
        }
        draw.mTexture = draw.mTextures[0];
        draw.mMode = mode;
        draw.mFirst = indexed ? first_index : first;
        draw.mCount = count;
        draw.mIndexType = vulkan_index_type;
        draw.mIndexed = indexed;
        draw.mFramebuffer = gBoundVulkanDrawFramebuffer;
        draw.mViewport = gCurrentVulkanViewport;
        draw.mScissor = gCurrentVulkanScissor;
        draw.mAttributes = gCurrentVulkanVertexAttributes;
        draw.mModelviewProjection = gGL.getProjectionMatrix() * gGL.getModelviewMatrix();
        draw.mUseWorldVertexShader = gCurrentVulkanWorldDrawEnabled;
        draw.mWorldBlendPipeline = to_vulkan_world_blend_pipeline();
        draw.mWorldDepthPipeline = to_vulkan_world_depth_pipeline();
        draw.mWorldCullPipeline = to_vulkan_world_cull_pipeline();
        draw.mWorldShaderClass = gCurrentVulkanWorldShaderClass;
        draw.mTerrainParameters = gCurrentVulkanTerrainParameters;
        draw.mMaterialParameters = gCurrentVulkanMaterialParameters;
        draw.mTextureTransform = gCurrentVulkanTextureTransform;
        if (draw.mUseWorldVertexShader &&
            (draw.mAttributes[9].mEnabled || draw.mAttributes[10].mEnabled) &&
            gCurrentVulkanSkinningMatrixCount > 0 &&
            !gCurrentVulkanSkinningMatrixPalette.empty())
        {
            draw.mSkinningMatrixCount = gCurrentVulkanSkinningMatrixCount;
            draw.mSkinningMatrixPalette = gCurrentVulkanSkinningMatrixPalette;
        }
        if (draw.mUseWorldVertexShader &&
            (draw.mAttributes[9].mEnabled || draw.mAttributes[10].mEnabled))
        {
            static U32 sLoggedSkinningDraws = 0;
            if (sLoggedSkinningDraws < 32)
            {
                LL_INFOS("RenderBackend")
                    << "Vulkan queued skinning draw "
                    << sLoggedSkinningDraws
                    << ": classic weight "
                    << draw.mAttributes[9].mEnabled
                    << ", weight4 "
                    << draw.mAttributes[10].mEnabled
                    << ", matrix count "
                    << draw.mSkinningMatrixCount
                    << ", count "
                    << draw.mCount
                    << ", indexed "
                    << draw.mIndexed
                    << LL_ENDL;
                ++sLoggedSkinningDraws;
            }
        }
        draw.mDepthTestEnabled = gCurrentVulkanDepthTestEnabled;
        draw.mDepthWriteEnabled = gCurrentVulkanDepthWriteEnabled;
        draw.mDepthFunction = gCurrentVulkanDepthFunction;
        draw.mCullFaceEnabled = gCurrentVulkanCullFaceEnabled;
        draw.mCullFace = gCurrentVulkanCullFace;
        draw.mAlphaMaskCutoff = gCurrentVulkanAlphaMaskCutoff;
        gPendingVulkanDraws.push_back(draw);

        if (gCurrentVulkanContext)
        {
            ++gCurrentVulkanContext->mQueuedUIDrawCount;
            if (indexed)
            {
                ++gCurrentVulkanContext->mQueuedIndexedUIDrawCount;
            }
        }
        return true;
    }

    void drawIndexedRange(
        LLRenderPrimitiveType mode,
        U32,
        U32,
        S32 count,
        LLRenderIndexType index_type,
        const void* indices) override
    {
        queueDraw(mode, 0, count, true, index_type, indices);
    }

    void drawArrays(LLRenderPrimitiveType mode, S32 first, S32 count) override
    {
        queueDraw(mode, first, count, false, LLRenderIndexType::UnsignedShort, nullptr);
    }

    void drawElements(
        LLRenderPrimitiveType mode,
        S32 count,
        LLRenderIndexType index_type,
        const void* indices) override
    {
        queueDraw(mode, 0, count, true, index_type, indices);
    }

    void generateFramebuffers(S32 count, U32* framebuffers) override
    {
        if (count <= 0 || !framebuffers)
        {
            return;
        }

        for (S32 i = 0; i < count; ++i)
        {
            U32 handle = gNextVulkanFramebufferHandle++;
            framebuffers[i] = handle;
            gVulkanFramebuffers.emplace(handle, LLVulkanFramebufferResource());
        }
    }

    void deleteFramebuffers(S32 count, const U32* framebuffers) override
    {
        if (count <= 0 || !framebuffers)
        {
            return;
        }

        for (S32 i = 0; i < count; ++i)
        {
            U32 framebuffer = framebuffers[i];
            if (!framebuffer)
            {
                continue;
            }

            if (gBoundVulkanReadFramebuffer == framebuffer)
            {
                gBoundVulkanReadFramebuffer = 0;
            }
            if (gBoundVulkanDrawFramebuffer == framebuffer)
            {
                gBoundVulkanDrawFramebuffer = 0;
            }

            auto framebuffer_iter = gVulkanFramebuffers.find(framebuffer);
            if (framebuffer_iter != gVulkanFramebuffers.end())
            {
                if (gCurrentVulkanContext)
                {
                    destroy_vulkan_framebuffer_resource(
                        *gCurrentVulkanContext,
                        framebuffer_iter->second);
                }
                gVulkanFramebuffers.erase(framebuffer_iter);
            }
        }
    }

    void bindFramebuffer(LLRenderFramebufferBindPoint target, U32 framebuffer) override
    {
        if (framebuffer != 0 && gVulkanFramebuffers.find(framebuffer) == gVulkanFramebuffers.end())
        {
            gVulkanFramebuffers.emplace(framebuffer, LLVulkanFramebufferResource());
        }

        switch (target)
        {
        case LLRenderFramebufferBindPoint::Read:
            gBoundVulkanReadFramebuffer = framebuffer;
            break;
        case LLRenderFramebufferBindPoint::Draw:
            gBoundVulkanDrawFramebuffer = framebuffer;
            break;
        case LLRenderFramebufferBindPoint::ReadWrite:
        default:
            gBoundVulkanReadFramebuffer = framebuffer;
            gBoundVulkanDrawFramebuffer = framebuffer;
            break;
        }
    }

    void bindReadWriteFramebuffer(U32 framebuffer) override
    {
        bindFramebuffer(LLRenderFramebufferBindPoint::ReadWrite, framebuffer);
    }

    LLRenderFramebufferStatus getReadWriteFramebufferStatus() const override
    {
        return is_vulkan_framebuffer_complete(gBoundVulkanDrawFramebuffer) ?
            LLRenderFramebufferStatus::Complete :
            LLRenderFramebufferStatus::IncompleteMissingAttachment;
    }

    bool isDrawFramebufferComplete() const override
    {
        return is_vulkan_framebuffer_complete(gBoundVulkanDrawFramebuffer);
    }

    void attachFramebufferTexture2D(
        LLRenderFramebufferAttachment attachment,
        LLRenderTextureTarget,
        U32 texture,
        S32) override
    {
        if (!gBoundVulkanDrawFramebuffer)
        {
            return;
        }

        LLVulkanFramebufferResource& framebuffer =
            gVulkanFramebuffers[gBoundVulkanDrawFramebuffer];
        if (attachment == LLRenderFramebufferAttachment::Depth)
        {
            if (framebuffer.mDepthTexture != texture)
            {
                framebuffer.mDepthTexture = texture;
                framebuffer.mDirty = true;
            }
            return;
        }

        S32 color_index = to_vulkan_framebuffer_color_index(attachment);
        if (color_index < 0 ||
            static_cast<size_t>(color_index) >= framebuffer.mColorTextures.size())
        {
            return;
        }

        U32& color_texture = framebuffer.mColorTextures[static_cast<size_t>(color_index)];
        if (color_texture != texture)
        {
            color_texture = texture;
            framebuffer.mDirty = true;
        }
        if (texture)
        {
            framebuffer.mColorAttachmentCount = llmax(
                framebuffer.mColorAttachmentCount,
                static_cast<U32>(color_index + 1));
        }
        else
        {
            while (framebuffer.mColorAttachmentCount > 0 &&
                framebuffer.mColorTextures[framebuffer.mColorAttachmentCount - 1] == 0)
            {
                --framebuffer.mColorAttachmentCount;
            }
        }
    }

    void setFramebufferBufferRouting(U32 color_attachment_count) override
    {
        gVulkanFramebufferColorAttachmentCount =
            llclamp(color_attachment_count, 0U, 4U);
        if (gBoundVulkanDrawFramebuffer)
        {
            LLVulkanFramebufferResource& framebuffer =
                gVulkanFramebuffers[gBoundVulkanDrawFramebuffer];
            if (framebuffer.mColorAttachmentCount != gVulkanFramebufferColorAttachmentCount)
            {
                framebuffer.mColorAttachmentCount = gVulkanFramebufferColorAttachmentCount;
                framebuffer.mDirty = true;
            }
        }
    }

    void restoreDefaultFramebufferBufferRouting() override
    {
        gVulkanFramebufferColorAttachmentCount = 1;
    }

    bool hasError() override
    {
        return gCurrentVulkanContext &&
            !gCurrentVulkanContext->mLastTextureUploadSucceeded &&
            !gCurrentVulkanContext->mLastTextureUploadDeferred;
    }

    U32 getErrorCode() override
    {
        return hasError() ? 1 : 0;
    }

    void getInteger(LLRenderIntegerParameter parameter, S32* value) override
    {
        if (!value)
        {
            return;
        }

        *value = 0;
        LLVulkanNativeContext* context = gCurrentVulkanContext;
        if (!context)
        {
            return;
        }

        refresh_vulkan_memory_properties(*context);
        U64 bytes = 0;
        switch (parameter)
        {
        case LLRenderIntegerParameter::DedicatedVideoMemoryKB:
            bytes = get_vulkan_reported_video_memory_bytes(*context);
            break;
        case LLRenderIntegerParameter::FreeVideoMemoryKB:
            bytes = get_vulkan_reported_free_video_memory_bytes(*context);
            break;
        default:
            return;
        }

        *value = static_cast<S32>(
            std::min<U64>(
                bytes / 1024,
                static_cast<U64>(std::numeric_limits<S32>::max())));
    }

    U64 getTextureMemoryAllocatedBytes() const override
    {
        return gCurrentVulkanContext ?
            gCurrentVulkanContext->mTextureMemoryAllocatedBytes :
            0;
    }

    U64 getTextureMemoryBudgetBytes() const override
    {
        if (!gCurrentVulkanContext)
        {
            return get_vulkan_texture_memory_budget_bytes();
        }

        return gCurrentVulkanContext->mEffectiveTextureMemoryBudgetBytes ?
            gCurrentVulkanContext->mEffectiveTextureMemoryBudgetBytes :
            get_vulkan_texture_memory_budget_bytes();
    }

    U64 getBufferMemoryAllocatedBytes() const override
    {
        return gCurrentVulkanContext ?
            gCurrentVulkanContext->mBufferMemoryAllocatedBytes :
            0;
    }

    U64 getBufferMemoryBudgetBytes() const override
    {
        if (!gCurrentVulkanContext)
        {
            return get_vulkan_buffer_memory_budget_bytes();
        }

        return gCurrentVulkanContext->mEffectiveBufferMemoryBudgetBytes ?
            gCurrentVulkanContext->mEffectiveBufferMemoryBudgetBytes :
            get_vulkan_buffer_memory_budget_bytes();
    }

    const char* getInfoString(LLRenderInfoString parameter) override
    {
        if (parameter == LLRenderInfoString::Vendor)
        {
            if (gCurrentVulkanContext && !gCurrentVulkanContext->mPhysicalDeviceVendor.empty())
            {
                return gCurrentVulkanContext->mPhysicalDeviceVendor.c_str();
            }
            return "Vulkan";
        }
        if (parameter == LLRenderInfoString::Renderer)
        {
            if (gCurrentVulkanContext && !gCurrentVulkanContext->mPhysicalDeviceName.empty())
            {
                return gCurrentVulkanContext->mPhysicalDeviceName.c_str();
            }
            return "Vulkan device";
        }
        if (parameter == LLRenderInfoString::Version)
        {
            static const std::string version = get_vulkan_api_version_string();
            return version.c_str();
        }

        return "";
    }

    const char* getLegacyString(U32 parameter) override
    {
        if (parameter == LL_GL_VENDOR)
        {
            return getInfoString(LLRenderInfoString::Vendor);
        }
        if (parameter == LL_GL_RENDERER)
        {
            return getInfoString(LLRenderInfoString::Renderer);
        }

        return LLNullRenderBackend::getLegacyString(parameter);
    }
};

}

LLRenderBackend& getVulkanRenderBackend()
{
    static LLVulkanRenderBackend backend;
    return backend;
}
