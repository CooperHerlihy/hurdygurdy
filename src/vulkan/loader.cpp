#include "vulkan_backend.hpp"

#include "hg_error.hpp"
#include "hg_dynlib.hpp"

namespace hg {
namespace vulkan {

#define HG_MAKE_VULKAN_FUNC(name) PFN_##name name = nullptr

struct VulkanFuncs {
    HG_MAKE_VULKAN_FUNC(vkGetInstanceProcAddr);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceProcAddr);
    HG_MAKE_VULKAN_FUNC(vkCreateInstance);
    HG_MAKE_VULKAN_FUNC(vkDestroyInstance);
    HG_MAKE_VULKAN_FUNC(vkCreateDebugUtilsMessengerEXT);
    HG_MAKE_VULKAN_FUNC(vkDestroyDebugUtilsMessengerEXT);
    HG_MAKE_VULKAN_FUNC(vkEnumeratePhysicalDevices);
    HG_MAKE_VULKAN_FUNC(vkEnumerateDeviceExtensionProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties2);
    HG_MAKE_VULKAN_FUNC(vkDestroySurfaceKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    HG_MAKE_VULKAN_FUNC(vkCreateDevice);
    HG_MAKE_VULKAN_FUNC(vkDestroyDevice);
    HG_MAKE_VULKAN_FUNC(vkDeviceWaitIdle);
    HG_MAKE_VULKAN_FUNC(vkCreateSwapchainKHR);
    HG_MAKE_VULKAN_FUNC(vkDestroySwapchainKHR);
    HG_MAKE_VULKAN_FUNC(vkGetSwapchainImagesKHR);
    HG_MAKE_VULKAN_FUNC(vkAcquireNextImageKHR);
    HG_MAKE_VULKAN_FUNC(vkCreateSemaphore);
    HG_MAKE_VULKAN_FUNC(vkDestroySemaphore);
    HG_MAKE_VULKAN_FUNC(vkCreateFence);
    HG_MAKE_VULKAN_FUNC(vkDestroyFence);
    HG_MAKE_VULKAN_FUNC(vkResetFences);
    HG_MAKE_VULKAN_FUNC(vkWaitForFences);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceQueue);
    HG_MAKE_VULKAN_FUNC(vkQueueWaitIdle);
    HG_MAKE_VULKAN_FUNC(vkQueueSubmit);
    HG_MAKE_VULKAN_FUNC(vkQueuePresentKHR);
    HG_MAKE_VULKAN_FUNC(vkCreateCommandPool);
    HG_MAKE_VULKAN_FUNC(vkDestroyCommandPool);
    HG_MAKE_VULKAN_FUNC(vkResetCommandPool);
    HG_MAKE_VULKAN_FUNC(vkAllocateCommandBuffers);
    HG_MAKE_VULKAN_FUNC(vkFreeCommandBuffers);
    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkResetDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkAllocateDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkFreeDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkUpdateDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorSetLayout);
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorSetLayout);
    HG_MAKE_VULKAN_FUNC(vkCreatePipelineLayout);
    HG_MAKE_VULKAN_FUNC(vkDestroyPipelineLayout);
    HG_MAKE_VULKAN_FUNC(vkCreateShaderModule);
    HG_MAKE_VULKAN_FUNC(vkDestroyShaderModule);
    HG_MAKE_VULKAN_FUNC(vkCreateGraphicsPipelines);
    HG_MAKE_VULKAN_FUNC(vkCreateComputePipelines);
    HG_MAKE_VULKAN_FUNC(vkDestroyPipeline);
    HG_MAKE_VULKAN_FUNC(vkCreateRenderPass);
    HG_MAKE_VULKAN_FUNC(vkDestroyRenderPass);
    HG_MAKE_VULKAN_FUNC(vkCreateFramebuffer);
    HG_MAKE_VULKAN_FUNC(vkDestroyFramebuffer);
    HG_MAKE_VULKAN_FUNC(vkCreateBuffer);
    HG_MAKE_VULKAN_FUNC(vkDestroyBuffer);
    HG_MAKE_VULKAN_FUNC(vkCreateImage);
    HG_MAKE_VULKAN_FUNC(vkDestroyImage);
    HG_MAKE_VULKAN_FUNC(vkCreateImageView);
    HG_MAKE_VULKAN_FUNC(vkDestroyImageView);
    HG_MAKE_VULKAN_FUNC(vkCreateSampler);
    HG_MAKE_VULKAN_FUNC(vkDestroySampler);
    HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements2);
    HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements2);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceBufferMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceImageMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkAllocateMemory);
    HG_MAKE_VULKAN_FUNC(vkFreeMemory);
    HG_MAKE_VULKAN_FUNC(vkBindBufferMemory);
    HG_MAKE_VULKAN_FUNC(vkBindBufferMemory2);
    HG_MAKE_VULKAN_FUNC(vkBindImageMemory);
    HG_MAKE_VULKAN_FUNC(vkBindImageMemory2);
    HG_MAKE_VULKAN_FUNC(vkMapMemory);
    HG_MAKE_VULKAN_FUNC(vkUnmapMemory);
    HG_MAKE_VULKAN_FUNC(vkFlushMappedMemoryRanges);
    HG_MAKE_VULKAN_FUNC(vkInvalidateMappedMemoryRanges);
    HG_MAKE_VULKAN_FUNC(vkBeginCommandBuffer);
    HG_MAKE_VULKAN_FUNC(vkEndCommandBuffer);
    HG_MAKE_VULKAN_FUNC(vkResetCommandBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImage);
    HG_MAKE_VULKAN_FUNC(vkCmdBlitImage);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyBufferToImage);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImageToBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdPipelineBarrier);
    HG_MAKE_VULKAN_FUNC(vkCmdPipelineBarrier2);
    HG_MAKE_VULKAN_FUNC(vkCmdBeginRendering);
    HG_MAKE_VULKAN_FUNC(vkCmdEndRendering);
    HG_MAKE_VULKAN_FUNC(vkCmdBeginRenderPass);
    HG_MAKE_VULKAN_FUNC(vkCmdEndRenderPass);
    HG_MAKE_VULKAN_FUNC(vkCmdSetViewport);
    HG_MAKE_VULKAN_FUNC(vkCmdSetScissor);
    HG_MAKE_VULKAN_FUNC(vkCmdBindPipeline);
    HG_MAKE_VULKAN_FUNC(vkCmdBindDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkCmdPushConstants);
    HG_MAKE_VULKAN_FUNC(vkCmdBindVertexBuffers);
    HG_MAKE_VULKAN_FUNC(vkCmdBindIndexBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdDraw);
    HG_MAKE_VULKAN_FUNC(vkCmdDrawIndexed);
    HG_MAKE_VULKAN_FUNC(vkCmdDispatch);
};

#undef HG_MAKE_VULKAN_FUNC

Library libvulkan{};
VulkanFuncs vulkanFuncs{};

bool loadVulkan()
{
    Maybe<Library> lib = Library::load(
#if defined(HG_PLATFORM_LINUX)
        "libvulkan.so.1"
#elif defined(HG_PLATFORM_WINDOWS)
        "vulkan-1.dll"
#endif
    );
    if (!lib.has)
    {
        setError("Could not load vulkan");
        return false;
    }
    libvulkan = std::move(*lib);

    *(void**)&vulkanFuncs.vkGetInstanceProcAddr =
        libvulkan.findFunction("vkGetInstanceProcAddr").orElse(nullptr);
    if (vulkanFuncs.vkGetInstanceProcAddr == nullptr)
    {
        setError("Could not load vkGetInstanceProcAddr\n");
        return false;
    }

    vulkanFuncs.vkCreateInstance = (PFN_vkCreateInstance)
        vulkanFuncs.vkGetInstanceProcAddr(nullptr, "vkCreateInstance");
    if (vulkanFuncs.vkCreateInstance == nullptr)
    {
        setError("Could not load vkCreateInstance\n");
        return false;
    }

    return true;
}

void unloadVulkan()
{
    libvulkan = {};
}

#define HG_LOAD_VK_INSTANCE_FUNC(name) \
    vulkanFuncs. name = (PFN_##name)vulkanFuncs.vkGetInstanceProcAddr(instance, #name); \
    if (vulkanFuncs. name == nullptr) { \
        setError("Could not load " #name); \
        return false; \
    }

bool loadVulkanInstanceFuncs(VkInstance instance)
{
    HG_ASSERT(instance != nullptr);

    HG_LOAD_VK_INSTANCE_FUNC(vkGetDeviceProcAddr);
    HG_LOAD_VK_INSTANCE_FUNC(vkDestroyInstance);
#ifdef HG_VK_DEBUG_MESSENGER
    HG_LOAD_VK_INSTANCE_FUNC(vkCreateDebugUtilsMessengerEXT);
    HG_LOAD_VK_INSTANCE_FUNC(vkDestroyDebugUtilsMessengerEXT);
#endif
    HG_LOAD_VK_INSTANCE_FUNC(vkEnumeratePhysicalDevices);
    HG_LOAD_VK_INSTANCE_FUNC(vkEnumerateDeviceExtensionProperties);
    HG_LOAD_VK_INSTANCE_FUNC(vkGetPhysicalDeviceProperties);
    HG_LOAD_VK_INSTANCE_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
    HG_LOAD_VK_INSTANCE_FUNC(vkGetPhysicalDeviceMemoryProperties);
    HG_LOAD_VK_INSTANCE_FUNC(vkGetPhysicalDeviceMemoryProperties2);
    HG_LOAD_VK_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
    HG_LOAD_VK_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_LOAD_VK_INSTANCE_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_LOAD_VK_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    HG_LOAD_VK_INSTANCE_FUNC(vkDestroySurfaceKHR);
    HG_LOAD_VK_INSTANCE_FUNC(vkCreateDevice);

    return true;
}

#undef HG_LOAD_VK_INSTANCE_FUNC

#define HG_LOAD_VK_DEVICE_FUNC(name) \
    vulkanFuncs. name = (PFN_##name)vulkanFuncs.vkGetDeviceProcAddr(device, #name); \
    if (vulkanFuncs. name == nullptr) { \
        setError("Could not load " #name); \
        return false; \
    }

bool loadVulkanDeviceFuncs(VkDevice device)
{
    HG_ASSERT(device != nullptr);

    HG_LOAD_VK_DEVICE_FUNC(vkDestroyDevice);
    HG_LOAD_VK_DEVICE_FUNC(vkDeviceWaitIdle);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateSwapchainKHR);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroySwapchainKHR);
    HG_LOAD_VK_DEVICE_FUNC(vkGetSwapchainImagesKHR);
    HG_LOAD_VK_DEVICE_FUNC(vkAcquireNextImageKHR);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateSemaphore);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroySemaphore);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateFence);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyFence);
    HG_LOAD_VK_DEVICE_FUNC(vkResetFences);
    HG_LOAD_VK_DEVICE_FUNC(vkWaitForFences);
    HG_LOAD_VK_DEVICE_FUNC(vkGetDeviceQueue);
    HG_LOAD_VK_DEVICE_FUNC(vkQueueWaitIdle);
    HG_LOAD_VK_DEVICE_FUNC(vkQueueSubmit);
    HG_LOAD_VK_DEVICE_FUNC(vkQueuePresentKHR);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateCommandPool);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyCommandPool);
    HG_LOAD_VK_DEVICE_FUNC(vkResetCommandPool);
    HG_LOAD_VK_DEVICE_FUNC(vkAllocateCommandBuffers);
    HG_LOAD_VK_DEVICE_FUNC(vkFreeCommandBuffers);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateDescriptorPool);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyDescriptorPool);
    HG_LOAD_VK_DEVICE_FUNC(vkResetDescriptorPool);
    HG_LOAD_VK_DEVICE_FUNC(vkAllocateDescriptorSets);
    HG_LOAD_VK_DEVICE_FUNC(vkFreeDescriptorSets);
    HG_LOAD_VK_DEVICE_FUNC(vkUpdateDescriptorSets);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateDescriptorSetLayout);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyDescriptorSetLayout);
    HG_LOAD_VK_DEVICE_FUNC(vkCreatePipelineLayout);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyPipelineLayout);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateShaderModule);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyShaderModule);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateGraphicsPipelines);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateComputePipelines);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyPipeline);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateRenderPass);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyRenderPass);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateFramebuffer);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyFramebuffer);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateBuffer);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyBuffer);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateImage);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyImage);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateImageView);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroyImageView);
    HG_LOAD_VK_DEVICE_FUNC(vkCreateSampler);
    HG_LOAD_VK_DEVICE_FUNC(vkDestroySampler);
    HG_LOAD_VK_DEVICE_FUNC(vkGetBufferMemoryRequirements);
    HG_LOAD_VK_DEVICE_FUNC(vkGetBufferMemoryRequirements2);
    HG_LOAD_VK_DEVICE_FUNC(vkGetImageMemoryRequirements);
    HG_LOAD_VK_DEVICE_FUNC(vkGetImageMemoryRequirements2);
    HG_LOAD_VK_DEVICE_FUNC(vkGetDeviceBufferMemoryRequirements);
    HG_LOAD_VK_DEVICE_FUNC(vkGetDeviceImageMemoryRequirements);
    HG_LOAD_VK_DEVICE_FUNC(vkAllocateMemory);
    HG_LOAD_VK_DEVICE_FUNC(vkFreeMemory);
    HG_LOAD_VK_DEVICE_FUNC(vkBindBufferMemory);
    HG_LOAD_VK_DEVICE_FUNC(vkBindBufferMemory2);
    HG_LOAD_VK_DEVICE_FUNC(vkBindImageMemory);
    HG_LOAD_VK_DEVICE_FUNC(vkBindImageMemory2);
    HG_LOAD_VK_DEVICE_FUNC(vkMapMemory);
    HG_LOAD_VK_DEVICE_FUNC(vkUnmapMemory);
    HG_LOAD_VK_DEVICE_FUNC(vkFlushMappedMemoryRanges);
    HG_LOAD_VK_DEVICE_FUNC(vkInvalidateMappedMemoryRanges);
    HG_LOAD_VK_DEVICE_FUNC(vkBeginCommandBuffer);
    HG_LOAD_VK_DEVICE_FUNC(vkEndCommandBuffer);
    HG_LOAD_VK_DEVICE_FUNC(vkResetCommandBuffer);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdCopyBuffer);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdCopyImage);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdBlitImage);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdCopyBufferToImage);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdCopyImageToBuffer);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdPipelineBarrier);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdPipelineBarrier2);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdBeginRendering);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdEndRendering);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdBeginRenderPass);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdEndRenderPass);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdSetViewport);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdSetScissor);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdBindPipeline);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdBindDescriptorSets);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdPushConstants);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdBindVertexBuffers);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdBindIndexBuffer);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdDraw);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdDrawIndexed);
    HG_LOAD_VK_DEVICE_FUNC(vkCmdDispatch);

    return true;
}

#undef HG_LOAD_VK_DEVICE_FUNC

} // namespace vulkan
} // namespace hg

#define VK_WRAPPER(name, ret, params, args) \
    ret name params \
    { \
        return ::hg::vulkan::vulkanFuncs.name args; \
    }

#define VK_WRAPPER_VOID(name, params, args) \
    void name params \
    { \
        ::hg::vulkan::vulkanFuncs.name args; \
    }

extern "C" {

VK_WRAPPER(vkGetInstanceProcAddr, PFN_vkVoidFunction,
    (VkInstance instance, const char* pName),
    (instance, pName))

VK_WRAPPER(vkGetDeviceProcAddr, PFN_vkVoidFunction,
    (VkDevice device, const char* pName),
    (device, pName))

VK_WRAPPER(vkCreateInstance, VkResult,
    (const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance),
    (pCreateInfo, pAllocator, pInstance))

VK_WRAPPER_VOID(vkDestroyInstance,
    (VkInstance instance, const VkAllocationCallbacks* pAllocator),
    (instance, pAllocator))

VK_WRAPPER(vkCreateDebugUtilsMessengerEXT, VkResult,
    (VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger),
    (instance, pCreateInfo, pAllocator, pMessenger))

VK_WRAPPER_VOID(vkDestroyDebugUtilsMessengerEXT,
    (VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator),
    (instance, messenger, pAllocator))

VK_WRAPPER(vkEnumeratePhysicalDevices, VkResult,
    (VkInstance instance, uint32_t* pCount, VkPhysicalDevice* pDevices),
    (instance, pCount, pDevices))

VK_WRAPPER(vkEnumerateDeviceExtensionProperties, VkResult,
    (VkPhysicalDevice device, const char* pLayerName, uint32_t* pCount, VkExtensionProperties* pProps),
    (device, pLayerName, pCount, pProps))

VK_WRAPPER_VOID(vkGetPhysicalDeviceProperties,
    (VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties),
    (physicalDevice, pProperties))

VK_WRAPPER_VOID(vkGetPhysicalDeviceQueueFamilyProperties,
    (VkPhysicalDevice device, uint32_t* pCount, VkQueueFamilyProperties* pProps),
    (device, pCount, pProps))

VK_WRAPPER_VOID(vkDestroySurfaceKHR,
    (VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator),
    (instance, surface, pAllocator))

VK_WRAPPER(vkCreateDevice, VkResult,
    (VkPhysicalDevice device, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice),
    (device, pCreateInfo, pAllocator, pDevice))

VK_WRAPPER_VOID(vkDestroyDevice,
    (VkDevice device, const VkAllocationCallbacks* pAllocator),
    (device, pAllocator))

VK_WRAPPER(vkDeviceWaitIdle, VkResult,
    (VkDevice device),
    (device))

VK_WRAPPER(vkGetPhysicalDeviceSurfaceSupportKHR, VkResult,
    (VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported),
    (physicalDevice, queueFamilyIndex, surface, pSupported))

VK_WRAPPER(vkGetPhysicalDeviceSurfaceFormatsKHR, VkResult,
    (VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t* pCount, VkSurfaceFormatKHR* pFormats),
    (device, surface, pCount, pFormats))

VK_WRAPPER(vkGetPhysicalDeviceSurfacePresentModesKHR, VkResult,
    (VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t* pCount, VkPresentModeKHR* pModes),
    (device, surface, pCount, pModes))

VK_WRAPPER(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, VkResult,
    (VkPhysicalDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pCaps),
    (device, surface, pCaps))

VK_WRAPPER(vkCreateSwapchainKHR, VkResult,
    (VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain),
    (device, pCreateInfo, pAllocator, pSwapchain))

VK_WRAPPER_VOID(vkDestroySwapchainKHR,
    (VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator),
    (device, swapchain, pAllocator))

VK_WRAPPER(vkGetSwapchainImagesKHR, VkResult,
    (VkDevice device, VkSwapchainKHR swapchain, uint32_t* pCount, VkImage* pImages),
    (device, swapchain, pCount, pImages))

VK_WRAPPER(vkAcquireNextImageKHR, VkResult,
    (VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore sem, VkFence fence, uint32_t* pIndex),
    (device, swapchain, timeout, sem, fence, pIndex))

VK_WRAPPER(vkCreateSemaphore, VkResult,
    (VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore),
    (device, pCreateInfo, pAllocator, pSemaphore))

VK_WRAPPER_VOID(vkDestroySemaphore,
    (VkDevice device, VkSemaphore sem, const VkAllocationCallbacks* pAllocator),
    (device, sem, pAllocator))

VK_WRAPPER(vkCreateFence, VkResult,
    (VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence),
    (device, pCreateInfo, pAllocator, pFence))

VK_WRAPPER_VOID(vkDestroyFence,
    (VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator),
    (device, fence, pAllocator))

VK_WRAPPER(vkResetFences, VkResult,
    (VkDevice device, uint32_t count, const VkFence* pFences),
    (device, count, pFences))

VK_WRAPPER(vkWaitForFences, VkResult,
    (VkDevice device, uint32_t count, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout),
    (device, count, pFences, waitAll, timeout))

VK_WRAPPER_VOID(vkGetDeviceQueue,
    (VkDevice device, uint32_t family, uint32_t index, VkQueue* pQueue),
    (device, family, index, pQueue))

VK_WRAPPER(vkQueueWaitIdle, VkResult,
    (VkQueue queue),
    (queue))

VK_WRAPPER(vkQueueSubmit, VkResult,
    (VkQueue queue, uint32_t count, const VkSubmitInfo* pSubmits, VkFence fence),
    (queue, count, pSubmits, fence))

VK_WRAPPER(vkQueuePresentKHR, VkResult,
    (VkQueue queue, const VkPresentInfoKHR* pInfo),
    (queue, pInfo))

VK_WRAPPER(vkCreateCommandPool, VkResult,
    (VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pPool),
    (device, pCreateInfo, pAllocator, pPool))

VK_WRAPPER_VOID(vkDestroyCommandPool,
    (VkDevice device, VkCommandPool pool, const VkAllocationCallbacks* pAllocator),
    (device, pool, pAllocator))

VK_WRAPPER(vkResetCommandPool, VkResult,
    (VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags),
    (device, commandPool, flags))

VK_WRAPPER(vkAllocateCommandBuffers, VkResult,
    (VkDevice device, const VkCommandBufferAllocateInfo* pInfo, VkCommandBuffer* pBufs),
    (device, pInfo, pBufs))

VK_WRAPPER_VOID(vkFreeCommandBuffers,
    (VkDevice device, VkCommandPool pool, uint32_t count, const VkCommandBuffer* pBufs),
    (device, pool, count, pBufs))

VK_WRAPPER(vkCreateDescriptorPool, VkResult,
    (VkDevice device, const VkDescriptorPoolCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pPool),
    (device, pInfo, pAllocator, pPool))

VK_WRAPPER_VOID(vkDestroyDescriptorPool,
    (VkDevice device, VkDescriptorPool pool, const VkAllocationCallbacks* pAllocator),
    (device, pool, pAllocator))

VK_WRAPPER(vkResetDescriptorPool, VkResult,
    (VkDevice device, VkDescriptorPool pool, uint32_t flags),
    (device, pool, flags))

VK_WRAPPER(vkAllocateDescriptorSets, VkResult,
    (VkDevice device, const VkDescriptorSetAllocateInfo* pInfo, VkDescriptorSet* pSets),
    (device, pInfo, pSets))

VK_WRAPPER(vkFreeDescriptorSets, VkResult,
    (VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets),
    (device, descriptorPool, descriptorSetCount, pDescriptorSets))

VK_WRAPPER_VOID(vkUpdateDescriptorSets,
    (VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet* pWrites, uint32_t copyCount, const VkCopyDescriptorSet* pCopies),
    (device, writeCount, pWrites, copyCount, pCopies))

VK_WRAPPER(vkCreateDescriptorSetLayout, VkResult,
    (VkDevice device, const VkDescriptorSetLayoutCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pLayout),
    (device, pInfo, pAllocator, pLayout))

VK_WRAPPER_VOID(vkDestroyDescriptorSetLayout,
    (VkDevice device, VkDescriptorSetLayout layout, const VkAllocationCallbacks* pAllocator),
    (device, layout, pAllocator))

VK_WRAPPER(vkCreatePipelineLayout, VkResult,
    (VkDevice device, const VkPipelineLayoutCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pLayout),
    (device, pInfo, pAllocator, pLayout))

VK_WRAPPER_VOID(vkDestroyPipelineLayout,
    (VkDevice device, VkPipelineLayout layout, const VkAllocationCallbacks* pAllocator),
    (device, layout, pAllocator))

VK_WRAPPER(vkCreateShaderModule, VkResult,
    (VkDevice device, const VkShaderModuleCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pModule),
    (device, pInfo, pAllocator, pModule))

VK_WRAPPER_VOID(vkDestroyShaderModule,
    (VkDevice device, VkShaderModule module, const VkAllocationCallbacks* pAllocator),
    (device, module, pAllocator))

VK_WRAPPER(vkCreateGraphicsPipelines, VkResult,
    (VkDevice device, VkPipelineCache cache, uint32_t count, const VkGraphicsPipelineCreateInfo* pInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines),
    (device, cache, count, pInfos, pAllocator, pPipelines))

VK_WRAPPER(vkCreateComputePipelines, VkResult,
    (VkDevice device, VkPipelineCache cache, uint32_t count, const VkComputePipelineCreateInfo* pInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines),
    (device, cache, count, pInfos, pAllocator, pPipelines))

VK_WRAPPER_VOID(vkDestroyPipeline,
    (VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator),
    (device, pipeline, pAllocator))

VK_WRAPPER(vkCreateRenderPass, VkResult,
    (VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass),
    (device, pCreateInfo, pAllocator, pRenderPass))

VK_WRAPPER_VOID(vkDestroyRenderPass,
    (VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator),
    (device, renderPass, pAllocator))

VK_WRAPPER(vkCreateFramebuffer, VkResult,
    (VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer),
    (device, pCreateInfo, pAllocator, pFramebuffer))

VK_WRAPPER_VOID(vkDestroyFramebuffer,
    (VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator),
    (device, framebuffer, pAllocator))

VK_WRAPPER(vkCreateBuffer, VkResult,
    (VkDevice device, const VkBufferCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuf),
    (device, pInfo, pAllocator, pBuf))

VK_WRAPPER_VOID(vkDestroyBuffer,
    (VkDevice device, VkBuffer buf, const VkAllocationCallbacks* pAllocator),
    (device, buf, pAllocator))

VK_WRAPPER(vkCreateImage, VkResult,
    (VkDevice device, const VkImageCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage),
    (device, pInfo, pAllocator, pImage))

VK_WRAPPER_VOID(vkDestroyImage,
    (VkDevice device, VkImage img, const VkAllocationCallbacks* pAllocator),
    (device, img, pAllocator))

VK_WRAPPER(vkCreateImageView, VkResult,
    (VkDevice device, const VkImageViewCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView),
    (device, pInfo, pAllocator, pView))

VK_WRAPPER_VOID(vkDestroyImageView,
    (VkDevice device, VkImageView view, const VkAllocationCallbacks* pAllocator),
    (device, view, pAllocator))

VK_WRAPPER(vkCreateSampler, VkResult,
    (VkDevice device, const VkSamplerCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler),
    (device, pInfo, pAllocator, pSampler))

VK_WRAPPER_VOID(vkDestroySampler,
    (VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator),
    (device, sampler, pAllocator))

VK_WRAPPER_VOID(vkGetPhysicalDeviceMemoryProperties,
    (VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties),
    (physicalDevice, pMemoryProperties))

VK_WRAPPER_VOID(vkGetPhysicalDeviceMemoryProperties2,
    (VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2* pMemoryProperties),
    (physicalDevice, pMemoryProperties))

VK_WRAPPER_VOID(vkGetBufferMemoryRequirements,
    (VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements),
    (device, buffer, pMemoryRequirements))

VK_WRAPPER_VOID(vkGetBufferMemoryRequirements2,
    (VkDevice device, const VkBufferMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements),
    (device, pInfo, pMemoryRequirements))

VK_WRAPPER_VOID(vkGetImageMemoryRequirements,
    (VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements),
    (device, image, pMemoryRequirements))

VK_WRAPPER_VOID(vkGetImageMemoryRequirements2,
    (VkDevice device, const VkImageMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements),
    (device, pInfo, pMemoryRequirements))

VK_WRAPPER_VOID(vkGetDeviceBufferMemoryRequirements,
    (VkDevice device, const VkDeviceBufferMemoryRequirements* pInfo, VkMemoryRequirements2* pMemoryRequirements),
    (device, pInfo, pMemoryRequirements))

VK_WRAPPER_VOID(vkGetDeviceImageMemoryRequirements,
    (VkDevice device, const VkDeviceImageMemoryRequirements* pInfo, VkMemoryRequirements2* pMemoryRequirements),
    (device, pInfo, pMemoryRequirements))

VK_WRAPPER(vkAllocateMemory, VkResult,
    (VkDevice device, const VkMemoryAllocateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory),
    (device, pInfo, pAllocator, pMemory))

VK_WRAPPER_VOID(vkFreeMemory,
    (VkDevice device, VkDeviceMemory mem, const VkAllocationCallbacks* pAllocator),
    (device, mem, pAllocator))

VK_WRAPPER(vkBindBufferMemory, VkResult,
    (VkDevice device, VkBuffer buf, VkDeviceMemory mem, VkDeviceSize offset),
    (device, buf, mem, offset))

VK_WRAPPER(vkBindBufferMemory2, VkResult,
    (VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos),
    (device, bindInfoCount, pBindInfos))

VK_WRAPPER(vkBindImageMemory, VkResult,
    (VkDevice device, VkImage img, VkDeviceMemory mem, VkDeviceSize offset),
    (device, img, mem, offset))

VK_WRAPPER(vkBindImageMemory2, VkResult,
    (VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos),
    (device, bindInfoCount, pBindInfos))

VK_WRAPPER(vkMapMemory, VkResult,
    (VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData),
    (device, mem, offset, size, flags, ppData))

VK_WRAPPER_VOID(vkUnmapMemory,
    (VkDevice device, VkDeviceMemory mem),
    (device, mem))

VK_WRAPPER(vkFlushMappedMemoryRanges, VkResult,
    (VkDevice device, uint32_t count, const VkMappedMemoryRange* pRanges),
    (device, count, pRanges))

VK_WRAPPER(vkInvalidateMappedMemoryRanges, VkResult,
    (VkDevice device, uint32_t count, const VkMappedMemoryRange* pRanges),
    (device, count, pRanges))

VK_WRAPPER(vkBeginCommandBuffer, VkResult,
    (VkCommandBuffer cmd, const VkCommandBufferBeginInfo* pInfo),
    (cmd, pInfo))

VK_WRAPPER(vkEndCommandBuffer, VkResult,
    (VkCommandBuffer cmd),
    (cmd))

VK_WRAPPER(vkResetCommandBuffer, VkResult,
    (VkCommandBuffer cmd, VkCommandBufferResetFlags flags),
    (cmd, flags))

VK_WRAPPER_VOID(vkCmdCopyBuffer,
    (VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, uint32_t count, const VkBufferCopy* pRegions),
    (cmd, src, dst, count, pRegions))

VK_WRAPPER_VOID(vkCmdCopyImage,
    (VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageCopy* pRegions),
    (cmd, src, srcLayout, dst, dstLayout, count, pRegions))

VK_WRAPPER_VOID(vkCmdBlitImage,
    (VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageBlit* pRegions, VkFilter filter),
    (cmd, src, srcLayout, dst, dstLayout, count, pRegions, filter))

VK_WRAPPER_VOID(vkCmdCopyBufferToImage,
    (VkCommandBuffer cmd, VkBuffer src, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkBufferImageCopy* pRegions),
    (cmd, src, dst, dstLayout, count, pRegions))

VK_WRAPPER_VOID(vkCmdCopyImageToBuffer,
    (VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkBuffer dst, uint32_t count, const VkBufferImageCopy* pRegions),
    (cmd, src, srcLayout, dst, count, pRegions))

VK_WRAPPER_VOID(vkCmdPipelineBarrier2,
    (VkCommandBuffer cmd, const VkDependencyInfo* pInfo),
    (cmd, pInfo))

VK_WRAPPER_VOID(vkCmdPipelineBarrier,
    (VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers),
    (commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers))

VK_WRAPPER_VOID(vkCmdBeginRendering,
    (VkCommandBuffer cmd, const VkRenderingInfo* pInfo),
    (cmd, pInfo))

VK_WRAPPER_VOID(vkCmdEndRendering,
    (VkCommandBuffer cmd),
    (cmd))

VK_WRAPPER_VOID(vkCmdBeginRenderPass,
    (VkCommandBuffer cmd, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents),
    (cmd, pRenderPassBegin, contents))

VK_WRAPPER_VOID(vkCmdEndRenderPass,
    (VkCommandBuffer cmd),
    (cmd))

VK_WRAPPER_VOID(vkCmdSetViewport,
    (VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkViewport* pViewports),
    (cmd, first, count, pViewports))

VK_WRAPPER_VOID(vkCmdSetScissor,
    (VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkRect2D* pScissors),
    (cmd, first, count, pScissors))

VK_WRAPPER_VOID(vkCmdBindPipeline,
    (VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipeline pipeline),
    (cmd, bindPoint, pipeline))

VK_WRAPPER_VOID(vkCmdBindDescriptorSets,
    (VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t count, const VkDescriptorSet* pSets, uint32_t dynCount, const uint32_t* pDyn),
    (cmd, bindPoint, layout, firstSet, count, pSets, dynCount, pDyn))

VK_WRAPPER_VOID(vkCmdPushConstants,
    (VkCommandBuffer cmd, VkPipelineLayout layout, VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void* pData),
    (cmd, layout, stages, offset, size, pData))

VK_WRAPPER_VOID(vkCmdBindVertexBuffers,
    (VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkBuffer* pBufs, const VkDeviceSize* pOffsets),
    (cmd, first, count, pBufs, pOffsets))

VK_WRAPPER_VOID(vkCmdBindIndexBuffer,
    (VkCommandBuffer cmd, VkBuffer buf, VkDeviceSize offset, VkIndexType type),
    (cmd, buf, offset, type))

VK_WRAPPER_VOID(vkCmdDraw,
    (VkCommandBuffer cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance),
    (cmd, vertexCount, instanceCount, firstVertex, firstInstance))

VK_WRAPPER_VOID(vkCmdDrawIndexed,
    (VkCommandBuffer cmd, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance),
    (cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance))

VK_WRAPPER_VOID(vkCmdDispatch,
    (VkCommandBuffer cmd, uint32_t x, uint32_t y, uint32_t z),
    (cmd, x, y, z))

#undef VK_WRAPPER
#undef VK_WRAPPER_VOID

} // extern "C"
