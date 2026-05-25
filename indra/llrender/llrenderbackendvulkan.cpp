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
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

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

struct LLVkClearValue
{
    F32 color[4];
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

struct LLVkPhysicalDevicePropertiesHeader
{
    U32 apiVersion;
    U32 driverVersion;
    U32 vendorID;
    U32 deviceID;
    S32 deviceType;
    char deviceName[LL_VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
};

struct LLVkBufferImageCopy
{
    U64 bufferOffset;
    U32 bufferRowLength;
    U32 bufferImageHeight;
    LLVkImageSubresourceLayers imageSubresource;
    S32 imageOffset[3];
    LLVkExtent3D imageExtent;
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
using LLVulkanUpdateDescriptorSets =
    void (*)(LLVkDevice, U32, const LLVkWriteDescriptorSet*, U32, const void*);
using LLVulkanCmdBindDescriptorSets =
    void (*)(LLVkCommandBuffer, S32, LLVkPipelineLayout, U32, U32, const LLVkDescriptorSet*, U32, const U32*);

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
constexpr S32 LL_VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR = 1000001000;
constexpr S32 LL_VK_STRUCTURE_TYPE_PRESENT_INFO_KHR = 1000001001;
constexpr S32 LL_VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT = 1000217000;
constexpr U32 LL_VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR = 0x00000001;
constexpr U32 LL_VK_QUEUE_GRAPHICS_BIT = 0x00000001;
constexpr U32 LL_VK_QUEUE_FAMILY_IGNORED = 0xffffffff;
constexpr U64 LL_VK_TIMEOUT_FOREVER = 0xffffffffffffffffULL;
constexpr S32 LL_VK_SUBOPTIMAL_KHR = 1000001003;
constexpr S32 LL_VK_ERROR_OUT_OF_DATE_KHR = -1000001004;
constexpr U32 LL_VK_EXTENT_UNDEFINED = 0xffffffff;
constexpr S32 LL_VK_FORMAT_UNDEFINED = 0;
constexpr S32 LL_VK_FORMAT_R8G8B8A8_UNORM = 37;
constexpr S32 LL_VK_FORMAT_B8G8R8A8_UNORM = 44;
constexpr S32 LL_VK_FORMAT_R8G8B8A8_SRGB = 43;
constexpr S32 LL_VK_FORMAT_B8G8R8A8_SRGB = 50;
constexpr S32 LL_VK_FORMAT_R32G32_SFLOAT = 103;
constexpr S32 LL_VK_FORMAT_R32G32B32_SFLOAT = 106;
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
constexpr U32 LL_VK_IMAGE_USAGE_TRANSFER_DST_BIT = 0x00000002;
constexpr U32 LL_VK_IMAGE_USAGE_SAMPLED_BIT = 0x00000004;
constexpr U32 LL_VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 0x00000001;
constexpr U32 LL_VK_BUFFER_USAGE_INDEX_BUFFER_BIT = 0x00000040;
constexpr U32 LL_VK_BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x00000080;
constexpr U32 MARE_VULKAN_DEFAULT_UI_ATTRIBUTE_VERTICES = 262144;
constexpr U32 LL_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 0x00000001;
constexpr U32 LL_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 0x00000002;
constexpr U32 LL_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 0x00000004;
constexpr U32 LL_VK_IMAGE_ASPECT_COLOR_BIT = 0x00000001;
constexpr S32 LL_VK_IMAGE_VIEW_TYPE_2D = 1;
constexpr S32 LL_VK_COMPONENT_SWIZZLE_IDENTITY = 0;
constexpr S32 LL_VK_FILTER_NEAREST = 0;
constexpr S32 LL_VK_FILTER_LINEAR = 1;
constexpr S32 LL_VK_SAMPLER_MIPMAP_MODE_NEAREST = 0;
constexpr S32 LL_VK_SAMPLER_MIPMAP_MODE_LINEAR = 1;
constexpr S32 LL_VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2;
constexpr S32 LL_VK_BORDER_COLOR_INT_OPAQUE_BLACK = 3;
constexpr S32 LL_VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1;
constexpr S32 LL_VK_DYNAMIC_STATE_VIEWPORT = 0;
constexpr S32 LL_VK_DYNAMIC_STATE_SCISSOR = 1;
constexpr S32 LL_VK_COMMAND_BUFFER_LEVEL_PRIMARY = 0;
constexpr U32 LL_VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 0x00000001;
constexpr U32 LL_VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT = 0x00000002;
constexpr U32 LL_VK_FENCE_CREATE_SIGNALED_BIT = 0x00000001;
constexpr S32 LL_VK_IMAGE_LAYOUT_UNDEFINED = 0;
constexpr S32 LL_VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2;
constexpr S32 LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5;
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
constexpr U32 LL_VK_PIPELINE_STAGE_TRANSFER_BIT = 0x00001000;
constexpr U32 LL_VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = 0x00000100;
constexpr U32 LL_VK_ACCESS_SHADER_READ_BIT = 0x00000020;
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
constexpr S32 LL_VK_FRONT_FACE_COUNTER_CLOCKWISE = 0;
constexpr S32 LL_VK_BLEND_FACTOR_ZERO = 0;
constexpr S32 LL_VK_BLEND_FACTOR_ONE = 1;
constexpr S32 LL_VK_BLEND_FACTOR_SRC_ALPHA = 6;
constexpr S32 LL_VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7;
constexpr S32 LL_VK_BLEND_OP_ADD = 0;
constexpr U32 LL_VK_COLOR_COMPONENT_R_BIT = 0x00000001;
constexpr U32 LL_VK_COLOR_COMPONENT_G_BIT = 0x00000002;
constexpr U32 LL_VK_COLOR_COMPONENT_B_BIT = 0x00000004;
constexpr U32 LL_VK_COLOR_COMPONENT_A_BIT = 0x00000008;
constexpr const char* LL_VK_KHR_SURFACE_EXTENSION_NAME = "VK_KHR_surface";
constexpr const char* LL_VK_EXT_METAL_SURFACE_EXTENSION_NAME = "VK_EXT_metal_surface";
constexpr const char* LL_VK_KHR_SWAPCHAIN_EXTENSION_NAME = "VK_KHR_swapchain";
constexpr const char* LL_VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME = "VK_KHR_portability_enumeration";
constexpr const char* LL_VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME = "VK_KHR_portability_subset";
constexpr const char* LL_VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME =
    "VK_KHR_get_physical_device_properties2";

static constexpr U32 MARE_VULKAN_BOOTSTRAP_VERT_SPV[] =
{
    0x07230203, 0x00010000, 0x000d000b, 0x00000029,
    0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000000, 0x00000004, 0x6e69616d,
    0x00000000, 0x0000001a, 0x0000001e, 0x00030003,
    0x00000002, 0x000001c2, 0x000a0004, 0x475f4c47,
    0x4c474f4f, 0x70635f45, 0x74735f70, 0x5f656c79,
    0x656e696c, 0x7269645f, 0x69746365, 0x00006576,
    0x00080004, 0x475f4c47, 0x4c474f4f, 0x6e695f45,
    0x64756c63, 0x69645f65, 0x74636572, 0x00657669,
    0x00040005, 0x00000004, 0x6e69616d, 0x00000000,
    0x00050005, 0x0000000c, 0x69736f70, 0x6e6f6974,
    0x00000073, 0x00060005, 0x00000018, 0x505f6c67,
    0x65567265, 0x78657472, 0x00000000, 0x00060006,
    0x00000018, 0x00000000, 0x505f6c67, 0x7469736f,
    0x006e6f69, 0x00070006, 0x00000018, 0x00000001,
    0x505f6c67, 0x746e696f, 0x657a6953, 0x00000000,
    0x00070006, 0x00000018, 0x00000002, 0x435f6c67,
    0x4470696c, 0x61747369, 0x0065636e, 0x00070006,
    0x00000018, 0x00000003, 0x435f6c67, 0x446c6c75,
    0x61747369, 0x0065636e, 0x00030005, 0x0000001a,
    0x00000000, 0x00060005, 0x0000001e, 0x565f6c67,
    0x65747265, 0x646e4978, 0x00007865, 0x00030047,
    0x00000018, 0x00000002, 0x00050048, 0x00000018,
    0x00000000, 0x0000000b, 0x00000000, 0x00050048,
    0x00000018, 0x00000001, 0x0000000b, 0x00000001,
    0x00050048, 0x00000018, 0x00000002, 0x0000000b,
    0x00000003, 0x00050048, 0x00000018, 0x00000003,
    0x0000000b, 0x00000004, 0x00040047, 0x0000001e,
    0x0000000b, 0x0000002a, 0x00020013, 0x00000002,
    0x00030021, 0x00000003, 0x00000002, 0x00030016,
    0x00000006, 0x00000020, 0x00040017, 0x00000007,
    0x00000006, 0x00000002, 0x00040015, 0x00000008,
    0x00000020, 0x00000000, 0x0004002b, 0x00000008,
    0x00000009, 0x00000003, 0x0004001c, 0x0000000a,
    0x00000007, 0x00000009, 0x00040020, 0x0000000b,
    0x00000006, 0x0000000a, 0x0004003b, 0x0000000b,
    0x0000000c, 0x00000006, 0x0004002b, 0x00000006,
    0x0000000d, 0x00000000, 0x0004002b, 0x00000006,
    0x0000000e, 0xbf147ae1, 0x0005002c, 0x00000007,
    0x0000000f, 0x0000000d, 0x0000000e, 0x0004002b,
    0x00000006, 0x00000010, 0x3f147ae1, 0x0004002b,
    0x00000006, 0x00000011, 0x3ef5c28f, 0x0005002c,
    0x00000007, 0x00000012, 0x00000010, 0x00000011,
    0x0005002c, 0x00000007, 0x00000013, 0x0000000e,
    0x00000011, 0x0006002c, 0x0000000a, 0x00000014,
    0x0000000f, 0x00000012, 0x00000013, 0x00040017,
    0x00000015, 0x00000006, 0x00000004, 0x0004002b,
    0x00000008, 0x00000016, 0x00000001, 0x0004001c,
    0x00000017, 0x00000006, 0x00000016, 0x0006001e,
    0x00000018, 0x00000015, 0x00000006, 0x00000017,
    0x00000017, 0x00040020, 0x00000019, 0x00000003,
    0x00000018, 0x0004003b, 0x00000019, 0x0000001a,
    0x00000003, 0x00040015, 0x0000001b, 0x00000020,
    0x00000001, 0x0004002b, 0x0000001b, 0x0000001c,
    0x00000000, 0x00040020, 0x0000001d, 0x00000001,
    0x0000001b, 0x0004003b, 0x0000001d, 0x0000001e,
    0x00000001, 0x00040020, 0x00000020, 0x00000006,
    0x00000007, 0x0004002b, 0x00000006, 0x00000023,
    0x3f800000, 0x00040020, 0x00000027, 0x00000003,
    0x00000015, 0x00050036, 0x00000002, 0x00000004,
    0x00000000, 0x00000003, 0x000200f8, 0x00000005,
    0x0003003e, 0x0000000c, 0x00000014, 0x0004003d,
    0x0000001b, 0x0000001f, 0x0000001e, 0x00050041,
    0x00000020, 0x00000021, 0x0000000c, 0x0000001f,
    0x0004003d, 0x00000007, 0x00000022, 0x00000021,
    0x00050051, 0x00000006, 0x00000024, 0x00000022,
    0x00000000, 0x00050051, 0x00000006, 0x00000025,
    0x00000022, 0x00000001, 0x00070050, 0x00000015,
    0x00000026, 0x00000024, 0x00000025, 0x0000000d,
    0x00000023, 0x00050041, 0x00000027, 0x00000028,
    0x0000001a, 0x0000001c, 0x0003003e, 0x00000028,
    0x00000026, 0x000100fd, 0x00010038,
};

static constexpr U32 MARE_VULKAN_BOOTSTRAP_FRAG_SPV[] =
{
    0x07230203, 0x00010000, 0x000d000b, 0x0000000f,
    0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0006000f, 0x00000004, 0x00000004, 0x6e69616d,
    0x00000000, 0x00000009, 0x00030010, 0x00000004,
    0x00000007, 0x00030003, 0x00000002, 0x000001c2,
    0x000a0004, 0x475f4c47, 0x4c474f4f, 0x70635f45,
    0x74735f70, 0x5f656c79, 0x656e696c, 0x7269645f,
    0x69746365, 0x00006576, 0x00080004, 0x475f4c47,
    0x4c474f4f, 0x6e695f45, 0x64756c63, 0x69645f65,
    0x74636572, 0x00657669, 0x00040005, 0x00000004,
    0x6e69616d, 0x00000000, 0x00050005, 0x00000009,
    0x5f74756f, 0x6f6c6f63, 0x00000072, 0x00040047,
    0x00000009, 0x0000001e, 0x00000000, 0x00020013,
    0x00000002, 0x00030021, 0x00000003, 0x00000002,
    0x00030016, 0x00000006, 0x00000020, 0x00040017,
    0x00000007, 0x00000006, 0x00000004, 0x00040020,
    0x00000008, 0x00000003, 0x00000007, 0x0004003b,
    0x00000008, 0x00000009, 0x00000003, 0x0004002b,
    0x00000006, 0x0000000a, 0x3f733333, 0x0004002b,
    0x00000006, 0x0000000b, 0x3e8f5c29, 0x0004002b,
    0x00000006, 0x0000000c, 0x3e6147ae, 0x0004002b,
    0x00000006, 0x0000000d, 0x3f800000, 0x0007002c,
    0x00000007, 0x0000000e, 0x0000000a, 0x0000000b,
    0x0000000c, 0x0000000d, 0x00050036, 0x00000002,
    0x00000004, 0x00000000, 0x00000003, 0x000200f8,
    0x00000005, 0x0003003e, 0x00000009, 0x0000000e,
    0x000100fd, 0x00010038,
};

static constexpr U32 MARE_VULKAN_UI_VERT_SPV[] =
{
    0x07230203, 0x00010000, 0x000d000b, 0x0000002b,
    0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x000b000f, 0x00000000, 0x00000004, 0x6e69616d,
    0x00000000, 0x0000000d, 0x00000012, 0x00000021,
    0x00000023, 0x00000027, 0x00000029, 0x00030003,
    0x00000002, 0x000001c2, 0x000a0004, 0x475f4c47,
    0x4c474f4f, 0x70635f45, 0x74735f70, 0x5f656c79,
    0x656e696c, 0x7269645f, 0x69746365, 0x00006576,
    0x00080004, 0x475f4c47, 0x4c474f4f, 0x6e695f45,
    0x64756c63, 0x69645f65, 0x74636572, 0x00657669,
    0x00040005, 0x00000004, 0x6e69616d, 0x00000000,
    0x00060005, 0x0000000b, 0x505f6c67, 0x65567265,
    0x78657472, 0x00000000, 0x00060006, 0x0000000b,
    0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69,
    0x00070006, 0x0000000b, 0x00000001, 0x505f6c67,
    0x746e696f, 0x657a6953, 0x00000000, 0x00070006,
    0x0000000b, 0x00000002, 0x435f6c67, 0x4470696c,
    0x61747369, 0x0065636e, 0x00070006, 0x0000000b,
    0x00000003, 0x435f6c67, 0x446c6c75, 0x61747369,
    0x0065636e, 0x00030005, 0x0000000d, 0x00000000,
    0x00050005, 0x00000012, 0x705f6e69, 0x7469736f,
    0x006e6f69, 0x00050005, 0x00000021, 0x67617266,
    0x6c6f635f, 0x0000726f, 0x00050005, 0x00000023,
    0x635f6e69, 0x726f6c6f, 0x00000000, 0x00040005,
    0x00000027, 0x67617266, 0x0076755f, 0x00060005,
    0x00000029, 0x745f6e69, 0x6f637865, 0x3064726f,
    0x00000000, 0x00030047, 0x0000000b, 0x00000002,
    0x00050048, 0x0000000b, 0x00000000, 0x0000000b,
    0x00000000, 0x00050048, 0x0000000b, 0x00000001,
    0x0000000b, 0x00000001, 0x00050048, 0x0000000b,
    0x00000002, 0x0000000b, 0x00000003, 0x00050048,
    0x0000000b, 0x00000003, 0x0000000b, 0x00000004,
    0x00040047, 0x00000012, 0x0000001e, 0x00000000,
    0x00040047, 0x00000021, 0x0000001e, 0x00000000,
    0x00040047, 0x00000023, 0x0000001e, 0x00000006,
    0x00040047, 0x00000027, 0x0000001e, 0x00000001,
    0x00040047, 0x00000029, 0x0000001e, 0x00000002,
    0x00020013, 0x00000002, 0x00030021, 0x00000003,
    0x00000002, 0x00030016, 0x00000006, 0x00000020,
    0x00040017, 0x00000007, 0x00000006, 0x00000004,
    0x00040015, 0x00000008, 0x00000020, 0x00000000,
    0x0004002b, 0x00000008, 0x00000009, 0x00000001,
    0x0004001c, 0x0000000a, 0x00000006, 0x00000009,
    0x0006001e, 0x0000000b, 0x00000007, 0x00000006,
    0x0000000a, 0x0000000a, 0x00040020, 0x0000000c,
    0x00000003, 0x0000000b, 0x0004003b, 0x0000000c,
    0x0000000d, 0x00000003, 0x00040015, 0x0000000e,
    0x00000020, 0x00000001, 0x0004002b, 0x0000000e,
    0x0000000f, 0x00000000, 0x00040017, 0x00000010,
    0x00000006, 0x00000003, 0x00040020, 0x00000011,
    0x00000001, 0x00000010, 0x0004003b, 0x00000011,
    0x00000012, 0x00000001, 0x0004002b, 0x00000008,
    0x00000013, 0x00000000, 0x00040020, 0x00000014,
    0x00000001, 0x00000006, 0x0004002b, 0x00000008,
    0x0000001a, 0x00000002, 0x0004002b, 0x00000006,
    0x0000001d, 0x3f800000, 0x00040020, 0x0000001f,
    0x00000003, 0x00000007, 0x0004003b, 0x0000001f,
    0x00000021, 0x00000003, 0x00040020, 0x00000022,
    0x00000001, 0x00000007, 0x0004003b, 0x00000022,
    0x00000023, 0x00000001, 0x00040017, 0x00000025,
    0x00000006, 0x00000002, 0x00040020, 0x00000026,
    0x00000003, 0x00000025, 0x0004003b, 0x00000026,
    0x00000027, 0x00000003, 0x00040020, 0x00000028,
    0x00000001, 0x00000025, 0x0004003b, 0x00000028,
    0x00000029, 0x00000001, 0x00050036, 0x00000002,
    0x00000004, 0x00000000, 0x00000003, 0x000200f8,
    0x00000005, 0x00050041, 0x00000014, 0x00000015,
    0x00000012, 0x00000013, 0x0004003d, 0x00000006,
    0x00000016, 0x00000015, 0x00050041, 0x00000014,
    0x00000017, 0x00000012, 0x00000009, 0x0004003d,
    0x00000006, 0x00000018, 0x00000017, 0x0004007f,
    0x00000006, 0x00000019, 0x00000018, 0x00050041,
    0x00000014, 0x0000001b, 0x00000012, 0x0000001a,
    0x0004003d, 0x00000006, 0x0000001c, 0x0000001b,
    0x00070050, 0x00000007, 0x0000001e, 0x00000016,
    0x00000019, 0x0000001c, 0x0000001d, 0x00050041,
    0x0000001f, 0x00000020, 0x0000000d, 0x0000000f,
    0x0003003e, 0x00000020, 0x0000001e, 0x0004003d,
    0x00000007, 0x00000024, 0x00000023, 0x0003003e,
    0x00000021, 0x00000024, 0x0004003d, 0x00000025,
    0x0000002a, 0x00000029, 0x0003003e, 0x00000027,
    0x0000002a, 0x000100fd, 0x00010038,
};

static constexpr U32 MARE_VULKAN_UI_FRAG_SPV[] =
{
    0x07230203, 0x00010000, 0x000d000b, 0x00000018,
    0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0008000f, 0x00000004, 0x00000004, 0x6e69616d,
    0x00000000, 0x00000009, 0x0000000b, 0x00000014,
    0x00030010, 0x00000004, 0x00000007, 0x00030003,
    0x00000002, 0x000001c2, 0x000a0004, 0x475f4c47,
    0x4c474f4f, 0x70635f45, 0x74735f70, 0x5f656c79,
    0x656e696c, 0x7269645f, 0x69746365, 0x00006576,
    0x00080004, 0x475f4c47, 0x4c474f4f, 0x6e695f45,
    0x64756c63, 0x69645f65, 0x74636572, 0x00657669,
    0x00040005, 0x00000004, 0x6e69616d, 0x00000000,
    0x00050005, 0x00000009, 0x5f74756f, 0x6f6c6f63,
    0x00000072, 0x00050005, 0x0000000b, 0x67617266,
    0x6c6f635f, 0x0000726f, 0x00050005, 0x00000010,
    0x745f6975, 0x75747865, 0x00006572, 0x00040005,
    0x00000014, 0x67617266, 0x0076755f, 0x00040047,
    0x00000009, 0x0000001e, 0x00000000, 0x00040047,
    0x0000000b, 0x0000001e, 0x00000000, 0x00040047,
    0x00000010, 0x00000021, 0x00000000, 0x00040047,
    0x00000010, 0x00000022, 0x00000000, 0x00040047,
    0x00000014, 0x0000001e, 0x00000001, 0x00020013,
    0x00000002, 0x00030021, 0x00000003, 0x00000002,
    0x00030016, 0x00000006, 0x00000020, 0x00040017,
    0x00000007, 0x00000006, 0x00000004, 0x00040020,
    0x00000008, 0x00000003, 0x00000007, 0x0004003b,
    0x00000008, 0x00000009, 0x00000003, 0x00040020,
    0x0000000a, 0x00000001, 0x00000007, 0x0004003b,
    0x0000000a, 0x0000000b, 0x00000001, 0x00090019,
    0x0000000d, 0x00000006, 0x00000001, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000,
    0x0003001b, 0x0000000e, 0x0000000d, 0x00040020,
    0x0000000f, 0x00000000, 0x0000000e, 0x0004003b,
    0x0000000f, 0x00000010, 0x00000000, 0x00040017,
    0x00000012, 0x00000006, 0x00000002, 0x00040020,
    0x00000013, 0x00000001, 0x00000012, 0x0004003b,
    0x00000013, 0x00000014, 0x00000001, 0x00050036,
    0x00000002, 0x00000004, 0x00000000, 0x00000003,
    0x000200f8, 0x00000005, 0x0004003d, 0x00000007,
    0x0000000c, 0x0000000b, 0x0004003d, 0x0000000e,
    0x00000011, 0x00000010, 0x0004003d, 0x00000012,
    0x00000015, 0x00000014, 0x00050057, 0x00000007,
    0x00000016, 0x00000011, 0x00000015, 0x00050085,
    0x00000007, 0x00000017, 0x0000000c, 0x00000016,
    0x0003003e, 0x00000009, 0x00000017, 0x000100fd,
    0x00010038,
};

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
            "@executable_path/../Frameworks/libvulkan.1.dylib",
            "@executable_path/../Frameworks/libvulkan.dylib",
            "@executable_path/../Frameworks/libMoltenVK.dylib",
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

struct LLVulkanPendingDraw
{
    U32 mBuffer = 0;
    U32 mIndexBuffer = 0;
    U32 mTexture = 0;
    LLRenderPrimitiveType mMode = LLRenderPrimitiveType::Triangles;
    S32 mFirst = 0;
    S32 mCount = 0;
    S32 mIndexType = LL_VK_INDEX_TYPE_UINT16;
    bool mIndexed = false;
    LLRenderViewport mViewport;
    LLRenderScissor mScissor;
    std::array<LLVulkanVertexAttributeState, 16> mAttributes;
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
    void* mMappedData = nullptr;
};

struct LLVulkanTextureResource
{
    LLVkImage mImage = nullptr;
    LLVkDeviceMemory mMemory = nullptr;
    LLVkImageView mImageView = nullptr;
    LLVkSampler mSampler = nullptr;
    LLVkDescriptorSet mDescriptorSet = nullptr;
    S32 mWidth = 0;
    S32 mHeight = 0;
    std::vector<U8> mPixels;
};

struct LLVulkanTextureSamplerState
{
    LLRenderTextureFilter mMinFilter = LLRenderTextureFilter::Linear;
    LLRenderTextureFilter mMagFilter = LLRenderTextureFilter::Linear;
};

struct LLVulkanNativeContext
{
    LLVkInstance mInstance = nullptr;
    LLVkSurfaceKHR mSurface = nullptr;
    LLVkPhysicalDevice mPhysicalDevice = nullptr;
    std::string mPhysicalDeviceVendor;
    std::string mPhysicalDeviceName;
    LLVkDevice mDevice = nullptr;
    LLVkQueue mGraphicsQueue = nullptr;
    LLVkQueue mPresentQueue = nullptr;
    LLVkSwapchainKHR mSwapchain = nullptr;
    LLVkCommandPool mCommandPool = nullptr;
    LLVkRenderPass mRenderPass = nullptr;
    LLVkShaderModule mBootstrapVertexShader = nullptr;
    LLVkShaderModule mBootstrapFragmentShader = nullptr;
    LLVkShaderModule mUIVertexShader = nullptr;
    LLVkShaderModule mUIFragmentShader = nullptr;
    LLVkPipelineLayout mBootstrapPipelineLayout = nullptr;
    LLVkPipelineLayout mUIPipelineLayout = nullptr;
    LLVkDescriptorSetLayout mUIDescriptorSetLayout = nullptr;
    LLVkDescriptorPool mUIDescriptorPool = nullptr;
    LLVkPipeline mBootstrapPipeline = nullptr;
    std::array<LLVkPipeline, 7> mUIPipelines = {};
    LLVulkanBufferResource mDefaultTexCoordBuffer;
    LLVulkanBufferResource mDefaultColorBuffer;
    U32 mGraphicsQueueFamilyIndex = LL_VK_QUEUE_FAMILY_IGNORED;
    U32 mPresentQueueFamilyIndex = LL_VK_QUEUE_FAMILY_IGNORED;
    LLVkExtent2D mSwapchainExtent = {0, 0};
    LLVkExtent2D mNativeViewExtent = {0, 0};
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
    LLVulkanDestroyRenderPass mDestroyRenderPass = nullptr;
    LLVulkanDestroyFramebuffer mDestroyFramebuffer = nullptr;
    LLVulkanDestroySemaphore mDestroySemaphore = nullptr;
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
    LLVulkanDestroyShaderModule mDestroyShaderModule = nullptr;
    LLVulkanDestroyPipelineLayout mDestroyPipelineLayout = nullptr;
    LLVulkanDestroyPipeline mDestroyPipeline = nullptr;
    LLVulkanCmdBindPipeline mCmdBindPipeline = nullptr;
    LLVulkanCmdDraw mCmdDraw = nullptr;
    LLVulkanCmdBindIndexBuffer mCmdBindIndexBuffer = nullptr;
    LLVulkanCmdDrawIndexed mCmdDrawIndexed = nullptr;
    LLVulkanGetPhysicalDeviceMemoryProperties mGetPhysicalDeviceMemoryProperties = nullptr;
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
    LLVulkanUpdateDescriptorSets mUpdateDescriptorSets = nullptr;
    LLVulkanCmdBindDescriptorSets mCmdBindDescriptorSets = nullptr;
    U64 mPresentedFrameCount = 0;
    U64 mQueuedUIDrawCount = 0;
    U64 mQueuedIndexedUIDrawCount = 0;
    U64 mRecordedUIDrawCount = 0;
    U64 mRecordMissingBufferCount = 0;
    U64 mRecordMissingAttributeCount = 0;
    U64 mTextureUploadCount = 0;
    U64 mSkippedTextureSubImageMissingResourceCount = 0;
    U64 mSkippedTextureSubImageOutOfBoundsCount = 0;
    U64 mSkippedTextureUnsupportedUploadCount = 0;
    U32 mLargestTextureHandle = 0;
    S32 mLargestTextureWidth = 0;
    S32 mLargestTextureHeight = 0;
    bool mFrameRenderingFailed = false;
    bool mLoggedFirstUIDraw = false;
    bool mLoggedFirstEmptyFrame = false;
    bool mLoggedUIDrawTelemetry = false;
    std::vector<LLVkPhysicalDevice> mPhysicalDevices;
    std::vector<LLVkImage> mSwapchainImages;
    std::vector<LLVkImageView> mSwapchainImageViews;
    std::vector<LLVkFramebuffer> mSwapchainFramebuffers;
    std::vector<LLVkCommandBuffer> mCommandBuffers;
    std::vector<LLVulkanFrameSync> mFrameSync;
#if LL_DARWIN
    void* mNativeView = nullptr;
    void* mMetalLayer = nullptr;
#endif
    bool mEnableVSync = false;
};

thread_local LLVulkanNativeContext* gCurrentVulkanContext = nullptr;
thread_local LLRenderViewport gCurrentVulkanViewport = {};
thread_local LLRenderScissor gCurrentVulkanScissor = {};
thread_local S32 gVulkanUnpackRowLength = 0;
U32 gNextVulkanBufferHandle = 1;
U32 gNextVulkanTextureHandle = 1;
thread_local S32 gActiveVulkanTextureUnit = 0;
thread_local std::array<U32, 16> gBoundVulkanTextures = {};
U32 gBoundVulkanVertexBuffer = 0;
U32 gBoundVulkanIndexBuffer = 0;
std::unordered_map<U32, LLVulkanBufferResource> gVulkanBuffers;
std::unordered_map<U32, LLVulkanTextureResource> gVulkanTextures;
std::unordered_map<U32, LLVulkanTextureSamplerState> gVulkanTextureSamplerStates;
std::array<LLVulkanVertexAttributeState, 16> gCurrentVulkanVertexAttributes = {};
std::vector<LLVulkanPendingDraw> gPendingVulkanDraws;

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

bool find_vulkan_memory_type(
    const LLVulkanNativeContext& context,
    U32 type_filter,
    U32 properties,
    U32& memory_type_index)
{
    if (!context.mGetPhysicalDeviceMemoryProperties)
    {
        return false;
    }

    LLVkPhysicalDeviceMemoryProperties memory_properties = {};
    context.mGetPhysicalDeviceMemoryProperties(context.mPhysicalDevice, &memory_properties);
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

    resource = {};
}

void destroy_all_vulkan_buffer_resources(LLVulkanNativeContext& context)
{
    destroy_vulkan_buffer_resource(context, context.mDefaultTexCoordBuffer);
    destroy_vulkan_buffer_resource(context, context.mDefaultColorBuffer);

    for (auto& entry : gVulkanBuffers)
    {
        destroy_vulkan_buffer_resource(context, entry.second);
    }

    gVulkanBuffers.clear();
    gPendingVulkanDraws.clear();
    gCurrentVulkanVertexAttributes = {};
    gBoundVulkanVertexBuffer = 0;
    gBoundVulkanIndexBuffer = 0;
    gCurrentVulkanViewport = {};
    gCurrentVulkanScissor = {};
}

bool create_vulkan_buffer_resource(
    LLVulkanNativeContext& context,
    U64 size,
    U32 usage,
    const void* data,
    LLVulkanBufferResource& resource);

bool create_vulkan_default_ui_attribute_buffers(LLVulkanNativeContext& context)
{
    std::vector<F32> default_texcoords(MARE_VULKAN_DEFAULT_UI_ATTRIBUTE_VERTICES * 2, 0.f);
    std::vector<U8> default_colors(MARE_VULKAN_DEFAULT_UI_ATTRIBUTE_VERTICES * 4, 255);

    if (!create_vulkan_buffer_resource(
            context,
            default_texcoords.size() * sizeof(F32),
            LL_VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            default_texcoords.data(),
            context.mDefaultTexCoordBuffer))
    {
        return false;
    }

    if (!create_vulkan_buffer_resource(
            context,
            default_colors.size() * sizeof(U8),
            LL_VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            default_colors.data(),
            context.mDefaultColorBuffer))
    {
        destroy_vulkan_buffer_resource(context, context.mDefaultTexCoordBuffer);
        return false;
    }

    LL_INFOS("RenderBackend")
        << "Vulkan UI default attribute buffers created for "
        << MARE_VULKAN_DEFAULT_UI_ATTRIBUTE_VERTICES
        << " vertices."
        << LL_ENDL;
    return true;
}

bool create_vulkan_buffer_resource(
    LLVulkanNativeContext& context,
    U64 size,
    U32 usage,
    const void* data,
    LLVulkanBufferResource& resource)
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
    if (data && size > 0)
    {
        std::memcpy(resource.mMappedData, data, static_cast<size_t>(size));
    }
    return true;
}

void destroy_vulkan_texture_resource(
    LLVulkanNativeContext& context,
    LLVulkanTextureResource& resource)
{
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

    resource = {};
}

void destroy_all_vulkan_texture_resources(LLVulkanNativeContext& context)
{
    for (auto& entry : gVulkanTextures)
    {
        destroy_vulkan_texture_resource(context, entry.second);
    }

    gVulkanTextures.clear();
    gVulkanTextureSamplerStates.clear();
    gBoundVulkanTextures = {};
}

bool ensure_vulkan_texture_entry_points(LLVulkanNativeContext& context)
{
    if (!context.mCreateImage)
    {
        context.mCreateImage =
            reinterpret_cast<LLVulkanCreateImage>(
                get_vulkan_device_proc_address(context, "vkCreateImage"));
        context.mDestroyImage =
            reinterpret_cast<LLVulkanDestroyImage>(
                get_vulkan_device_proc_address(context, "vkDestroyImage"));
        context.mGetImageMemoryRequirements =
            reinterpret_cast<LLVulkanGetImageMemoryRequirements>(
                get_vulkan_device_proc_address(context, "vkGetImageMemoryRequirements"));
        context.mBindImageMemory =
            reinterpret_cast<LLVulkanBindImageMemory>(
                get_vulkan_device_proc_address(context, "vkBindImageMemory"));
        context.mCreateSampler =
            reinterpret_cast<LLVulkanCreateSampler>(
                get_vulkan_device_proc_address(context, "vkCreateSampler"));
        context.mDestroySampler =
            reinterpret_cast<LLVulkanDestroySampler>(
                get_vulkan_device_proc_address(context, "vkDestroySampler"));
        context.mCmdCopyBufferToImage =
            reinterpret_cast<LLVulkanCmdCopyBufferToImage>(
                get_vulkan_device_proc_address(context, "vkCmdCopyBufferToImage"));
    }

    return context.mCreateImage &&
        context.mDestroyImage &&
        context.mGetImageMemoryRequirements &&
        context.mBindImageMemory &&
        context.mCreateImageView &&
        context.mDestroyImageView &&
        context.mCreateSampler &&
        context.mDestroySampler &&
        context.mCmdCopyBufferToImage &&
        context.mAllocateDescriptorSets &&
        context.mUpdateDescriptorSets &&
        context.mUIDescriptorPool &&
        context.mUIDescriptorSetLayout;
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

    return context.mAllocateCommandBuffers &&
        context.mFreeCommandBuffers &&
        context.mBeginCommandBuffer &&
        context.mEndCommandBuffer &&
        context.mCmdPipelineBarrier &&
        context.mQueueSubmit &&
        context.mDeviceWaitIdle &&
        context.mCommandPool;
}

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

    result = context.mQueueSubmit(context.mGraphicsQueue, 1, &submit_info, nullptr);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkQueueSubmit(texture upload) failed with result "
            << result
            << LL_ENDL;
        context.mFreeCommandBuffers(context.mDevice, context.mCommandPool, 1, &command_buffer);
        return false;
    }

    result = context.mDeviceWaitIdle(context.mDevice);
    context.mFreeCommandBuffers(context.mDevice, context.mCommandPool, 1, &command_buffer);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkDeviceWaitIdle(texture upload) failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    return true;
}

void transition_vulkan_texture_layout(
    LLVulkanNativeContext& context,
    LLVkCommandBuffer command_buffer,
    LLVkImage image,
    S32 old_layout,
    S32 new_layout)
{
    U32 src_access = 0;
    U32 dst_access = 0;
    U32 src_stage = LL_VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    U32 dst_stage = LL_VK_PIPELINE_STAGE_TRANSFER_BIT;

    if (old_layout == LL_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        new_layout == LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        src_access = LL_VK_ACCESS_TRANSFER_WRITE_BIT;
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
            LL_VK_IMAGE_ASPECT_COLOR_BIT,
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
        LL_VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        LL_VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        LL_VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
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

void update_vulkan_texture_descriptor(
    LLVulkanNativeContext& context,
    const LLVulkanTextureResource& resource)
{
    if (!resource.mDescriptorSet || !resource.mSampler || !resource.mImageView)
    {
        return;
    }

    LLVkDescriptorImageInfo descriptor_image_info =
    {
        resource.mSampler,
        resource.mImageView,
        LL_VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    LLVkWriteDescriptorSet write_descriptor =
    {
        LL_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        resource.mDescriptorSet,
        0,
        0,
        1,
        LL_VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        &descriptor_image_info,
        nullptr,
        nullptr
    };
    context.mUpdateDescriptorSets(context.mDevice, 1, &write_descriptor, 0, nullptr);
}

bool update_vulkan_texture_sampler(
    LLVulkanNativeContext& context,
    U32 handle,
    LLVulkanTextureResource& resource)
{
    if (!resource.mImageView || !resource.mDescriptorSet ||
        !ensure_vulkan_texture_entry_points(context))
    {
        return false;
    }

    if (context.mDeviceWaitIdle)
    {
        context.mDeviceWaitIdle(context.mDevice);
    }

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

    update_vulkan_texture_descriptor(context, resource);
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

    LLVulkanTextureResource& resource = gVulkanTextures[handle];
    destroy_vulkan_texture_resource(context, resource);
    resource.mWidth = width;
    resource.mHeight = height;
    resource.mPixels = pixels;

    LLVulkanBufferResource staging;
    if (!create_vulkan_buffer_resource(
            context,
            pixels.size(),
            LL_VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            pixels.data(),
            staging))
    {
        return false;
    }

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
        LL_VK_IMAGE_USAGE_TRANSFER_DST_BIT | LL_VK_IMAGE_USAGE_SAMPLED_BIT,
        LL_VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        LL_VK_IMAGE_LAYOUT_UNDEFINED
    };

    S32 result = context.mCreateImage(
        context.mDevice,
        &image_create_info,
        nullptr,
        &resource.mImage);
    if (result != LL_VK_SUCCESS || !resource.mImage)
    {
        LL_WARNS("RenderBackend") << "vkCreateImage(texture) failed with result " << result << LL_ENDL;
        destroy_vulkan_buffer_resource(context, staging);
        destroy_vulkan_texture_resource(context, resource);
        return false;
    }

    LLVkMemoryRequirements memory_requirements = {};
    context.mGetImageMemoryRequirements(context.mDevice, resource.mImage, &memory_requirements);

    U32 memory_type_index = 0;
    if (!find_vulkan_memory_type(
            context,
            memory_requirements.memoryTypeBits,
            LL_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            memory_type_index))
    {
        LL_WARNS("RenderBackend") << "No device-local Vulkan memory type for texture." << LL_ENDL;
        destroy_vulkan_buffer_resource(context, staging);
        destroy_vulkan_texture_resource(context, resource);
        return false;
    }

    LLVkMemoryAllocateInfo allocate_info =
    {
        LL_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memory_requirements.size,
        memory_type_index
    };

    result = context.mAllocateMemory(context.mDevice, &allocate_info, nullptr, &resource.mMemory);
    if (result != LL_VK_SUCCESS || !resource.mMemory)
    {
        LL_WARNS("RenderBackend") << "vkAllocateMemory(texture) failed with result " << result << LL_ENDL;
        destroy_vulkan_buffer_resource(context, staging);
        destroy_vulkan_texture_resource(context, resource);
        return false;
    }

    result = context.mBindImageMemory(context.mDevice, resource.mImage, resource.mMemory, 0);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend") << "vkBindImageMemory(texture) failed with result " << result << LL_ENDL;
        destroy_vulkan_buffer_resource(context, staging);
        destroy_vulkan_texture_resource(context, resource);
        return false;
    }

    LLVkCommandBuffer command_buffer = nullptr;
    if (!begin_vulkan_one_time_commands(context, command_buffer))
    {
        destroy_vulkan_buffer_resource(context, staging);
        destroy_vulkan_texture_resource(context, resource);
        return false;
    }

    transition_vulkan_texture_layout(
        context,
        command_buffer,
        resource.mImage,
        LL_VK_IMAGE_LAYOUT_UNDEFINED,
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
        { 0, 0, 0 },
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

    if (!end_vulkan_one_time_commands(context, command_buffer))
    {
        destroy_vulkan_buffer_resource(context, staging);
        destroy_vulkan_texture_resource(context, resource);
        return false;
    }
    destroy_vulkan_buffer_resource(context, staging);

    LLVkImageViewCreateInfo image_view_create_info =
    {
        LL_VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        resource.mImage,
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
        &resource.mImageView);
    if (result != LL_VK_SUCCESS || !resource.mImageView)
    {
        LL_WARNS("RenderBackend") << "vkCreateImageView(texture) failed with result " << result << LL_ENDL;
        destroy_vulkan_texture_resource(context, resource);
        return false;
    }

    if (!create_vulkan_texture_sampler(
            context,
            get_vulkan_texture_sampler_state(handle),
            resource.mSampler))
    {
        destroy_vulkan_texture_resource(context, resource);
        return false;
    }

    LLVkDescriptorSetAllocateInfo descriptor_allocate_info =
    {
        LL_VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        context.mUIDescriptorPool,
        1,
        &context.mUIDescriptorSetLayout
    };

    result = context.mAllocateDescriptorSets(
        context.mDevice,
        &descriptor_allocate_info,
        &resource.mDescriptorSet);
    if (result != LL_VK_SUCCESS || !resource.mDescriptorSet)
    {
        LL_WARNS("RenderBackend") << "vkAllocateDescriptorSets(texture) failed with result " << result << LL_ENDL;
        destroy_vulkan_texture_resource(context, resource);
        return false;
    }

    update_vulkan_texture_descriptor(context, resource);

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
    LLVulkanCreateRenderPass create_render_pass =
        reinterpret_cast<LLVulkanCreateRenderPass>(
            get_vulkan_device_proc_address(context, "vkCreateRenderPass"));
    context.mDestroyRenderPass =
        reinterpret_cast<LLVulkanDestroyRenderPass>(
            get_vulkan_device_proc_address(context, "vkDestroyRenderPass"));

    if (!create_render_pass || !context.mDestroyRenderPass)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend is missing required render-pass entry points."
            << LL_ENDL;
        return false;
    }

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
        LL_VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    LLVkAttachmentReference color_attachment_reference =
    {
        0,
        LL_VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
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
        nullptr,
        0,
        nullptr
    };

    LLVkSubpassDependency dependency =
    {
        LL_VK_SUBPASS_EXTERNAL,
        0,
        LL_VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        LL_VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        LL_VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0
    };

    LLVkRenderPassCreateInfo create_info =
    {
        LL_VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
        0,
        1,
        &color_attachment,
        1,
        &subpass,
        1,
        &dependency
    };

    S32 result = create_render_pass(
        context.mDevice,
        &create_info,
        nullptr,
        &context.mRenderPass);
    if (result != LL_VK_SUCCESS || !context.mRenderPass)
    {
        LL_WARNS("RenderBackend")
            << "vkCreateRenderPass failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    LL_INFOS("RenderBackend")
        << "Vulkan bootstrap render pass created."
        << LL_ENDL;
    return true;
}

void destroy_vulkan_render_pass(LLVulkanNativeContext& context)
{
    if (context.mDestroyRenderPass && context.mDevice && context.mRenderPass)
    {
        context.mDestroyRenderPass(context.mDevice, context.mRenderPass, nullptr);
    }

    context.mDestroyRenderPass = nullptr;
    context.mRenderPass = nullptr;
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
    }

    if (context.mDestroyPipelineLayout && context.mDevice && context.mBootstrapPipelineLayout)
    {
        context.mDestroyPipelineLayout(context.mDevice, context.mBootstrapPipelineLayout, nullptr);
    }

    if (context.mDestroyPipelineLayout && context.mDevice && context.mUIPipelineLayout)
    {
        context.mDestroyPipelineLayout(context.mDevice, context.mUIPipelineLayout, nullptr);
    }

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
        if (context.mBootstrapVertexShader)
        {
            context.mDestroyShaderModule(context.mDevice, context.mBootstrapVertexShader, nullptr);
        }
        if (context.mBootstrapFragmentShader)
        {
            context.mDestroyShaderModule(context.mDevice, context.mBootstrapFragmentShader, nullptr);
        }
        if (context.mUIVertexShader)
        {
            context.mDestroyShaderModule(context.mDevice, context.mUIVertexShader, nullptr);
        }
        if (context.mUIFragmentShader)
        {
            context.mDestroyShaderModule(context.mDevice, context.mUIFragmentShader, nullptr);
        }
    }

    context.mBootstrapPipeline = nullptr;
    context.mUIPipelines = {};
    context.mBootstrapPipelineLayout = nullptr;
    context.mUIPipelineLayout = nullptr;
    context.mUIDescriptorSetLayout = nullptr;
    context.mUIDescriptorPool = nullptr;
    context.mBootstrapVertexShader = nullptr;
    context.mBootstrapFragmentShader = nullptr;
    context.mUIVertexShader = nullptr;
    context.mUIFragmentShader = nullptr;
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
    context.mDestroyDescriptorSetLayout = nullptr;
    context.mDestroyDescriptorPool = nullptr;
    context.mAllocateDescriptorSets = nullptr;
    context.mUpdateDescriptorSets = nullptr;
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
    context.mUpdateDescriptorSets =
        reinterpret_cast<LLVulkanUpdateDescriptorSets>(
            get_vulkan_device_proc_address(context, "vkUpdateDescriptorSets"));
    context.mCmdBindDescriptorSets =
        reinterpret_cast<LLVulkanCmdBindDescriptorSets>(
            get_vulkan_device_proc_address(context, "vkCmdBindDescriptorSets"));

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
        !context.mUpdateDescriptorSets ||
        !context.mCmdBindDescriptorSets)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend is missing required graphics-pipeline entry points."
            << LL_ENDL;
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }

    if (!create_vulkan_shader_module(
            context,
            create_shader_module,
            MARE_VULKAN_BOOTSTRAP_VERT_SPV,
            sizeof(MARE_VULKAN_BOOTSTRAP_VERT_SPV),
            "bootstrap vertex",
            context.mBootstrapVertexShader) ||
        !create_vulkan_shader_module(
            context,
            create_shader_module,
            MARE_VULKAN_BOOTSTRAP_FRAG_SPV,
            sizeof(MARE_VULKAN_BOOTSTRAP_FRAG_SPV),
            "bootstrap fragment",
            context.mBootstrapFragmentShader) ||
        !create_vulkan_shader_module(
            context,
            create_shader_module,
            MARE_VULKAN_UI_VERT_SPV,
            sizeof(MARE_VULKAN_UI_VERT_SPV),
            "UI vertex",
            context.mUIVertexShader) ||
        !create_vulkan_shader_module(
            context,
            create_shader_module,
            MARE_VULKAN_UI_FRAG_SPV,
            sizeof(MARE_VULKAN_UI_FRAG_SPV),
            "UI fragment",
            context.mUIFragmentShader))
    {
        destroy_vulkan_graphics_pipelines(context);
        return false;
    }

    LLVkDescriptorSetLayoutBinding ui_sampler_binding =
    {
        0,
        LL_VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1,
        LL_VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr
    };
    LLVkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info =
    {
        LL_VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        1,
        &ui_sampler_binding
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

    LLVkDescriptorPoolSize descriptor_pool_size =
    {
        LL_VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        4096
    };
    LLVkDescriptorPoolCreateInfo descriptor_pool_create_info =
    {
        LL_VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        nullptr,
        0,
        4096,
        1,
        &descriptor_pool_size
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
        nullptr,
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

    LLVkVertexInputBindingDescription ui_bindings[3] =
    {
        { 0, 16, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 1, 8, LL_VK_VERTEX_INPUT_RATE_VERTEX },
        { 2, 4, LL_VK_VERTEX_INPUT_RATE_VERTEX },
    };

    LLVkVertexInputAttributeDescription ui_attributes[3] =
    {
        { 0, 0, LL_VK_FORMAT_R32G32B32_SFLOAT, 0 },
        { 2, 1, LL_VK_FORMAT_R32G32_SFLOAT, 0 },
        { 6, 2, LL_VK_FORMAT_R8G8B8A8_UNORM, 0 },
    };

    LLVkPipelineVertexInputStateCreateInfo ui_vertex_input =
    {
        LL_VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr,
        0,
        3,
        ui_bindings,
        3,
        ui_attributes
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
            nullptr,
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
    }

    LL_INFOS("RenderBackend")
        << "Vulkan bootstrap and UI graphics pipelines created."
        << LL_ENDL;
    return true;
}

bool create_vulkan_swapchain_framebuffers(LLVulkanNativeContext& context)
{
    LLVulkanCreateFramebuffer create_framebuffer =
        reinterpret_cast<LLVulkanCreateFramebuffer>(
            get_vulkan_device_proc_address(context, "vkCreateFramebuffer"));
    context.mDestroyFramebuffer =
        reinterpret_cast<LLVulkanDestroyFramebuffer>(
            get_vulkan_device_proc_address(context, "vkDestroyFramebuffer"));

    if (!create_framebuffer || !context.mDestroyFramebuffer)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan backend is missing required framebuffer entry points."
            << LL_ENDL;
        return false;
    }

    context.mSwapchainFramebuffers.reserve(context.mSwapchainImageViews.size());
    for (LLVkImageView image_view : context.mSwapchainImageViews)
    {
        LLVkFramebuffer framebuffer = nullptr;
        LLVkFramebufferCreateInfo create_info =
        {
            LL_VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr,
            0,
            context.mRenderPass,
            1,
            &image_view,
            context.mSwapchainExtent.width,
            context.mSwapchainExtent.height,
            1
        };

        S32 result = create_framebuffer(
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
    context.mDestroyFramebuffer = nullptr;
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
    LLVulkanCreateFence create_fence =
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
        !create_fence ||
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

        result = create_fence(
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
    destroy_vulkan_swapchain_image_views(context);
    destroy_vulkan_swapchain(context);

    if (!create_vulkan_swapchain(context, context.mNativeView, context.mEnableVSync) ||
        !create_vulkan_swapchain_image_views(context) ||
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

    if (!context.mBeginCommandBuffer ||
        !context.mEndCommandBuffer ||
        !context.mResetCommandBuffer ||
        !context.mCmdBeginRenderPass ||
        !context.mCmdEndRenderPass ||
        !context.mCmdBindPipeline ||
        !context.mCmdDraw ||
        !context.mCmdBindIndexBuffer ||
        !context.mCmdDrawIndexed ||
        !context.mCmdBindVertexBuffers ||
        !context.mCmdSetViewport ||
        !context.mCmdSetScissor ||
        !context.mCmdBindDescriptorSets)
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

    LLVkClearValue clear_value =
    {
        { 0.f, 0.f, 0.f, 1.f }
    };
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
        1,
        &clear_value
    };

    context.mCmdBeginRenderPass(
        command_buffer,
        &render_pass_begin,
        LL_VK_SUBPASS_CONTENTS_INLINE);

    U32 ui_draw_count = 0;
    U32 missing_buffer_count = 0;
    U32 missing_attribute_count = 0;
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

        U32 pipeline_index = to_vulkan_ui_pipeline_index(draw.mMode);
        if (pipeline_index >= context.mUIPipelines.size() ||
            !context.mUIPipelines[pipeline_index])
        {
            pipeline_index = to_vulkan_ui_pipeline_index(LLRenderPrimitiveType::Triangles);
        }

        LLVkBuffer texcoord_buffer = draw.mAttributes[2].mEnabled ?
            buffer_iter->second.mBuffer :
            context.mDefaultTexCoordBuffer.mBuffer;
        LLVkBuffer color_buffer = draw.mAttributes[6].mEnabled ?
            buffer_iter->second.mBuffer :
            context.mDefaultColorBuffer.mBuffer;

        if (!texcoord_buffer || !color_buffer)
        {
            ++missing_attribute_count;
            continue;
        }

        LLVkBuffer vertex_buffers[3] =
        {
            buffer_iter->second.mBuffer,
            texcoord_buffer,
            color_buffer
        };
        U64 offsets[3] =
        {
            draw.mAttributes[0].mOffset,
            draw.mAttributes[2].mEnabled ? draw.mAttributes[2].mOffset : 0,
            draw.mAttributes[6].mEnabled ? draw.mAttributes[6].mOffset : 0
        };

        LLVkDescriptorSet descriptor_set = nullptr;
        auto texture_iter = gVulkanTextures.find(draw.mTexture);
        if (texture_iter != gVulkanTextures.end())
        {
            descriptor_set = texture_iter->second.mDescriptorSet;
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
            auto fallback_iter = gVulkanTextures.find(0);
            if (fallback_iter != gVulkanTextures.end())
            {
                descriptor_set = fallback_iter->second.mDescriptorSet;
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
            context.mUIPipelines[pipeline_index]);
        context.mCmdBindDescriptorSets(
            command_buffer,
            LL_VK_PIPELINE_BIND_POINT_GRAPHICS,
            context.mUIPipelineLayout,
            0,
            1,
            &descriptor_set,
            0,
            nullptr);
        context.mCmdBindVertexBuffers(command_buffer, 0, 3, vertex_buffers, offsets);
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
            << ", missing buffers "
            << context.mRecordMissingBufferCount
            << ", missing attributes "
            << context.mRecordMissingAttributeCount
            << ", textures "
            << gVulkanTextures.size()
            << ", texture uploads "
            << context.mTextureUploadCount
            << ", largest texture "
            << context.mLargestTextureHandle
            << " ("
            << context.mLargestTextureWidth
            << "x"
            << context.mLargestTextureHeight
            << "), missing texture subimages "
            << context.mSkippedTextureSubImageMissingResourceCount
            << ", out-of-bounds texture subimages "
            << context.mSkippedTextureSubImageOutOfBoundsCount
            << ", unsupported texture uploads "
            << context.mSkippedTextureUnsupportedUploadCount
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
    S32 result = context.mWaitForFences(
        context.mDevice,
        1,
        &frame_sync.mInFlightFence,
        1,
        LL_VK_TIMEOUT_FOREVER);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkWaitForFences failed with result "
            << result
            << LL_ENDL;
        return false;
    }

    result = context.mResetFences(context.mDevice, 1, &frame_sync.mInFlightFence);
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

    result = context.mWaitForFences(
        context.mDevice,
        1,
        &frame_sync.mInFlightFence,
        1,
        LL_VK_TIMEOUT_FOREVER);
    if (result != LL_VK_SUCCESS)
    {
        LL_WARNS("RenderBackend")
            << "vkWaitForFences after submit failed with result "
            << result
            << LL_ENDL;
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

    if (context.mDeviceWaitIdle)
    {
        result = context.mDeviceWaitIdle(context.mDevice);
        if (result != LL_VK_SUCCESS)
        {
            LL_WARNS("RenderBackend")
                << "vkDeviceWaitIdle after Vulkan frame failed with result "
                << result
                << LL_ENDL;
            return false;
        }
    }

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
                << "Continuing with null high-level rendering and Vulkan swapchain presentation only."
                << LL_ENDL;
            return true;
        }

        LL_WARNS("RenderBackend")
            << "Vulkan backend stopped before capability initialization. "
            << "Only bootstrap UI rendering is implemented; real scene rendering is not implemented yet. "
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
        context.mVRAM = 0;

        LL_WARNS("RenderBackend")
            << "Vulkan backend created a macOS CAMetalLayer native view, VkInstance, surface, device, swapchain, "
            << "image views, render pass, bootstrap/UI pipelines, framebuffers, command buffers, and frame sync. "
            << "A first clear frame was presented. Real scene rendering is still pending."
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
        if (!gCurrentVulkanContext || level != 0)
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

        upload_vulkan_texture_resource(*gCurrentVulkanContext, texture, width, height, pixels);
    }

    void setTextureImage2D(
        LLRenderTextureTarget target,
        S32 level,
        LLRenderTextureFormat,
        S32 width,
        S32 height,
        S32 border,
        LLRenderPixelFormat format,
        LLRenderPixelType type,
        const void* data) override
    {
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

        std::vector<U8> converted;
        const S32 source_row_length = gVulkanUnpackRowLength > 0 ? gVulkanUnpackRowLength : width;
        if (!convert_texture_pixels_to_rgba8(width, height, source_row_length, format, type, pixels, converted))
        {
            ++gCurrentVulkanContext->mSkippedTextureUnsupportedUploadCount;
            return;
        }

        std::vector<U8> updated_pixels = resource.mPixels;
        for (S32 row = 0; row < height; ++row)
        {
            U8* destination =
                updated_pixels.data() +
                ((static_cast<size_t>(yoffset + row) * resource.mWidth + xoffset) * 4);
            const U8* source = converted.data() + static_cast<size_t>(row * width * 4);
            std::memcpy(destination, source, static_cast<size_t>(width * 4));
        }

        upload_vulkan_texture_resource(
            *gCurrentVulkanContext,
            texture,
            resource.mWidth,
            resource.mHeight,
            updated_pixels);
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
        LLRenderBufferUsage) override
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
        destroy_vulkan_buffer_resource(*gCurrentVulkanContext, resource);
        if (!create_vulkan_buffer_resource(
                *gCurrentVulkanContext,
                size,
                to_vulkan_buffer_usage(target),
                data,
                resource))
        {
            gVulkanBuffers.erase(handle);
        }
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
        auto iter = gVulkanBuffers.find(handle);
        if (iter == gVulkanBuffers.end() ||
            !iter->second.mMappedData ||
            !data ||
            offset + size > iter->second.mSize)
        {
            return;
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
        draw.mTexture = gBoundVulkanTextures[0];
        draw.mMode = mode;
        draw.mFirst = indexed ? first_index : first;
        draw.mCount = count;
        draw.mIndexType = vulkan_index_type;
        draw.mIndexed = indexed;
        draw.mViewport = gCurrentVulkanViewport;
        draw.mScissor = gCurrentVulkanScissor;
        draw.mAttributes = gCurrentVulkanVertexAttributes;
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
