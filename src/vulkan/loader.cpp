#include "vulkan_internal.hpp"

#include "hg/error.hpp"
#include "hg/dynlib.hpp"

namespace hg {
namespace vulkan {

#define HG_MAKE_VULKAN_FUNC(name) PFN_##name name = nullptr

struct VulkanFuncs {
    HG_MAKE_VULKAN_FUNC(vkGetInstanceProcAddr);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceProcAddr);
    HG_MAKE_VULKAN_FUNC(vkEnumerateInstanceLayerProperties);
    HG_MAKE_VULKAN_FUNC(vkEnumerateInstanceExtensionProperties);
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

#define HG_LOAD_VK_FUNC(name) \
    vulkanFuncs. name = (PFN_##name)vulkanFuncs.vkGetInstanceProcAddr(nullptr, #name); \
    if (vulkanFuncs. name == nullptr) { \
        setError("Could not load " #name); \
        return false; \
    }

    HG_LOAD_VK_FUNC(vkCreateInstance);
    HG_LOAD_VK_FUNC(vkEnumerateInstanceLayerProperties);
    HG_LOAD_VK_FUNC(vkEnumerateInstanceExtensionProperties);

#undef HG_LOAD_VK_FUNC

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
    if (vk.enableDebugMessenger)
    {
        HG_LOAD_VK_INSTANCE_FUNC(vkCreateDebugUtilsMessengerEXT);
        HG_LOAD_VK_INSTANCE_FUNC(vkDestroyDebugUtilsMessengerEXT);
    }
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

extern "C" {

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char* pName)
{
    return ::hg::vulkan::vulkanFuncs.vkGetInstanceProcAddr(instance, pName);
}

PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice device, const char* pName)
{
    return ::hg::vulkan::vulkanFuncs.vkGetDeviceProcAddr(device, pName);
}

VkResult vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

VkResult vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties)
{
    return ::hg::vulkan::vulkanFuncs.vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

VkResult vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties)
{
    return ::hg::vulkan::vulkanFuncs.vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}

void vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyInstance(instance, pAllocator);
}

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

VkResult vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pCount, VkPhysicalDevice* pDevices)
{
    return ::hg::vulkan::vulkanFuncs.vkEnumeratePhysicalDevices(instance, pCount, pDevices);
}

VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice device, const char* pLayerName, uint32_t* pCount, VkExtensionProperties* pProps)
{
    return ::hg::vulkan::vulkanFuncs.vkEnumerateDeviceExtensionProperties(device, pLayerName, pCount, pProps);
}

void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties)
{
    ::hg::vulkan::vulkanFuncs.vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device, uint32_t* pCount, VkQueueFamilyProperties* pProps)
{
    ::hg::vulkan::vulkanFuncs.vkGetPhysicalDeviceQueueFamilyProperties(device, pCount, pProps);
}

void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroySurfaceKHR(instance, surface, pAllocator);
}

VkResult vkCreateDevice(VkPhysicalDevice device, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateDevice(device, pCreateInfo, pAllocator, pDevice);
}

void vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyDevice(device, pAllocator);
}

VkResult vkDeviceWaitIdle(VkDevice device)
{
    return ::hg::vulkan::vulkanFuncs.vkDeviceWaitIdle(device);
}

VkResult vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported)
{
    return ::hg::vulkan::vulkanFuncs.vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
}

VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t* pCount, VkSurfaceFormatKHR* pFormats)
{
    return ::hg::vulkan::vulkanFuncs.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, pCount, pFormats);
}

VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t* pCount, VkPresentModeKHR* pModes)
{
    return ::hg::vulkan::vulkanFuncs.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, pCount, pModes);
}

VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pCaps)
{
    return ::hg::vulkan::vulkanFuncs.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, pCaps);
}

VkResult vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}

void vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

VkResult vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pCount, VkImage* pImages)
{
    return ::hg::vulkan::vulkanFuncs.vkGetSwapchainImagesKHR(device, swapchain, pCount, pImages);
}

VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore sem, VkFence fence, uint32_t* pIndex)
{
    return ::hg::vulkan::vulkanFuncs.vkAcquireNextImageKHR(device, swapchain, timeout, sem, fence, pIndex);
}

VkResult vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

void vkDestroySemaphore(VkDevice device, VkSemaphore sem, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroySemaphore(device, sem, pAllocator);
}

VkResult vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

void vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyFence(device, fence, pAllocator);
}

VkResult vkResetFences(VkDevice device, uint32_t count, const VkFence* pFences)
{
    return ::hg::vulkan::vulkanFuncs.vkResetFences(device, count, pFences);
}

VkResult vkWaitForFences(VkDevice device, uint32_t count, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout)
{
    return ::hg::vulkan::vulkanFuncs.vkWaitForFences(device, count, pFences, waitAll, timeout);
}

void vkGetDeviceQueue(VkDevice device, uint32_t family, uint32_t index, VkQueue* pQueue)
{
    ::hg::vulkan::vulkanFuncs.vkGetDeviceQueue(device, family, index, pQueue);
}

VkResult vkQueueWaitIdle(VkQueue queue)
{
    return ::hg::vulkan::vulkanFuncs.vkQueueWaitIdle(queue);
}

VkResult vkQueueSubmit(VkQueue queue, uint32_t count, const VkSubmitInfo* pSubmits, VkFence fence)
{
    return ::hg::vulkan::vulkanFuncs.vkQueueSubmit(queue, count, pSubmits, fence);
}

VkResult vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pInfo)
{
    return ::hg::vulkan::vulkanFuncs.vkQueuePresentKHR(queue, pInfo);
}

VkResult vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pPool)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateCommandPool(device, pCreateInfo, pAllocator, pPool);
}

void vkDestroyCommandPool(VkDevice device, VkCommandPool pool, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyCommandPool(device, pool, pAllocator);
}

VkResult vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    return ::hg::vulkan::vulkanFuncs.vkResetCommandPool(device, commandPool, flags);
}

VkResult vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pInfo, VkCommandBuffer* pBufs)
{
    return ::hg::vulkan::vulkanFuncs.vkAllocateCommandBuffers(device, pInfo, pBufs);
}

void vkFreeCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t count, const VkCommandBuffer* pBufs)
{
    ::hg::vulkan::vulkanFuncs.vkFreeCommandBuffers(device, pool, count, pBufs);
}

VkResult vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pPool)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateDescriptorPool(device, pInfo, pAllocator, pPool);
}

void vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool pool, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyDescriptorPool(device, pool, pAllocator);
}

VkResult vkResetDescriptorPool(VkDevice device, VkDescriptorPool pool, uint32_t flags)
{
    return ::hg::vulkan::vulkanFuncs.vkResetDescriptorPool(device, pool, flags);
}

VkResult vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pInfo, VkDescriptorSet* pSets)
{
    return ::hg::vulkan::vulkanFuncs.vkAllocateDescriptorSets(device, pInfo, pSets);
}

VkResult vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets)
{
    return ::hg::vulkan::vulkanFuncs.vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet* pWrites, uint32_t copyCount, const VkCopyDescriptorSet* pCopies)
{
    ::hg::vulkan::vulkanFuncs.vkUpdateDescriptorSets(device, writeCount, pWrites, copyCount, pCopies);
}

VkResult vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pLayout)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateDescriptorSetLayout(device, pInfo, pAllocator, pLayout);
}

void vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout layout, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyDescriptorSetLayout(device, layout, pAllocator);
}

VkResult vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pLayout)
{
    return ::hg::vulkan::vulkanFuncs.vkCreatePipelineLayout(device, pInfo, pAllocator, pLayout);
}

void vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout layout, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyPipelineLayout(device, layout, pAllocator);
}

VkResult vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pModule)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateShaderModule(device, pInfo, pAllocator, pModule);
}

void vkDestroyShaderModule(VkDevice device, VkShaderModule module, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyShaderModule(device, module, pAllocator);
}

VkResult vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache cache, uint32_t count, const VkGraphicsPipelineCreateInfo* pInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateGraphicsPipelines(device, cache, count, pInfos, pAllocator, pPipelines);
}

VkResult vkCreateComputePipelines(VkDevice device, VkPipelineCache cache, uint32_t count, const VkComputePipelineCreateInfo* pInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateComputePipelines(device, cache, count, pInfos, pAllocator, pPipelines);
}

void vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyPipeline(device, pipeline, pAllocator);
}

VkResult vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}

void vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyRenderPass(device, renderPass, pAllocator);
}

VkResult vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
}

void vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyFramebuffer(device, framebuffer, pAllocator);
}

VkResult vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuf)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateBuffer(device, pInfo, pAllocator, pBuf);
}

void vkDestroyBuffer(VkDevice device, VkBuffer buf, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyBuffer(device, buf, pAllocator);
}

VkResult vkCreateImage(VkDevice device, const VkImageCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateImage(device, pInfo, pAllocator, pImage);
}

void vkDestroyImage(VkDevice device, VkImage img, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyImage(device, img, pAllocator);
}

VkResult vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateImageView(device, pInfo, pAllocator, pView);
}

void vkDestroyImageView(VkDevice device, VkImageView view, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroyImageView(device, view, pAllocator);
}

VkResult vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
    return ::hg::vulkan::vulkanFuncs.vkCreateSampler(device, pInfo, pAllocator, pSampler);
}

void vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkDestroySampler(device, sampler, pAllocator);
}

void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    ::hg::vulkan::vulkanFuncs.vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

void vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2* pMemoryProperties)
{
    ::hg::vulkan::vulkanFuncs.vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
}

void vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements)
{
    ::hg::vulkan::vulkanFuncs.vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

void vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements)
{
    ::hg::vulkan::vulkanFuncs.vkGetBufferMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements)
{
    ::hg::vulkan::vulkanFuncs.vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}

void vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements)
{
    ::hg::vulkan::vulkanFuncs.vkGetImageMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void vkGetDeviceBufferMemoryRequirements(VkDevice device, const VkDeviceBufferMemoryRequirements* pInfo, VkMemoryRequirements2* pMemoryRequirements)
{
    ::hg::vulkan::vulkanFuncs.vkGetDeviceBufferMemoryRequirements(device, pInfo, pMemoryRequirements);
}

void vkGetDeviceImageMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements* pInfo, VkMemoryRequirements2* pMemoryRequirements)
{
    ::hg::vulkan::vulkanFuncs.vkGetDeviceImageMemoryRequirements(device, pInfo, pMemoryRequirements);
}

VkResult vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory)
{
    return ::hg::vulkan::vulkanFuncs.vkAllocateMemory(device, pInfo, pAllocator, pMemory);
}

void vkFreeMemory(VkDevice device, VkDeviceMemory mem, const VkAllocationCallbacks* pAllocator)
{
    ::hg::vulkan::vulkanFuncs.vkFreeMemory(device, mem, pAllocator);
}

VkResult vkBindBufferMemory(VkDevice device, VkBuffer buf, VkDeviceMemory mem, VkDeviceSize offset)
{
    return ::hg::vulkan::vulkanFuncs.vkBindBufferMemory(device, buf, mem, offset);
}

VkResult vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos)
{
    return ::hg::vulkan::vulkanFuncs.vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
}

VkResult vkBindImageMemory(VkDevice device, VkImage img, VkDeviceMemory mem, VkDeviceSize offset)
{
    return ::hg::vulkan::vulkanFuncs.vkBindImageMemory(device, img, mem, offset);
}

VkResult vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos)
{
    return ::hg::vulkan::vulkanFuncs.vkBindImageMemory2(device, bindInfoCount, pBindInfos);
}

VkResult vkMapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
{
    return ::hg::vulkan::vulkanFuncs.vkMapMemory(device, mem, offset, size, flags, ppData);
}

void vkUnmapMemory(VkDevice device, VkDeviceMemory mem)
{
    ::hg::vulkan::vulkanFuncs.vkUnmapMemory(device, mem);
}

VkResult vkFlushMappedMemoryRanges(VkDevice device, uint32_t count, const VkMappedMemoryRange* pRanges)
{
    return ::hg::vulkan::vulkanFuncs.vkFlushMappedMemoryRanges(device, count, pRanges);
}

VkResult vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t count, const VkMappedMemoryRange* pRanges)
{
    return ::hg::vulkan::vulkanFuncs.vkInvalidateMappedMemoryRanges(device, count, pRanges);
}

VkResult vkBeginCommandBuffer(VkCommandBuffer cmd, const VkCommandBufferBeginInfo* pInfo)
{
    return ::hg::vulkan::vulkanFuncs.vkBeginCommandBuffer(cmd, pInfo);
}

VkResult vkEndCommandBuffer(VkCommandBuffer cmd)
{
    return ::hg::vulkan::vulkanFuncs.vkEndCommandBuffer(cmd);
}

VkResult vkResetCommandBuffer(VkCommandBuffer cmd, VkCommandBufferResetFlags flags)
{
    return ::hg::vulkan::vulkanFuncs.vkResetCommandBuffer(cmd, flags);
}

void vkCmdCopyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, uint32_t count, const VkBufferCopy* pRegions)
{
    ::hg::vulkan::vulkanFuncs.vkCmdCopyBuffer(cmd, src, dst, count, pRegions);
}

void vkCmdCopyImage(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageCopy* pRegions)
{
    ::hg::vulkan::vulkanFuncs.vkCmdCopyImage(cmd, src, srcLayout, dst, dstLayout, count, pRegions);
}

void vkCmdBlitImage(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkImageBlit* pRegions, VkFilter filter)
{
    ::hg::vulkan::vulkanFuncs.vkCmdBlitImage(cmd, src, srcLayout, dst, dstLayout, count, pRegions, filter);
}

void vkCmdCopyBufferToImage(VkCommandBuffer cmd, VkBuffer src, VkImage dst, VkImageLayout dstLayout, uint32_t count, const VkBufferImageCopy* pRegions)
{
    ::hg::vulkan::vulkanFuncs.vkCmdCopyBufferToImage(cmd, src, dst, dstLayout, count, pRegions);
}

void vkCmdCopyImageToBuffer(VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkBuffer dst, uint32_t count, const VkBufferImageCopy* pRegions)
{
    ::hg::vulkan::vulkanFuncs.vkCmdCopyImageToBuffer(cmd, src, srcLayout, dst, count, pRegions);
}

void vkCmdPipelineBarrier2(VkCommandBuffer cmd, const VkDependencyInfo* pInfo)
{
    ::hg::vulkan::vulkanFuncs.vkCmdPipelineBarrier2(cmd, pInfo);
}

void vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    ::hg::vulkan::vulkanFuncs.vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void vkCmdBeginRendering(VkCommandBuffer cmd, const VkRenderingInfo* pInfo)
{
    ::hg::vulkan::vulkanFuncs.vkCmdBeginRendering(cmd, pInfo);
}

void vkCmdEndRendering(VkCommandBuffer cmd)
{
    ::hg::vulkan::vulkanFuncs.vkCmdEndRendering(cmd);
}

void vkCmdBeginRenderPass(VkCommandBuffer cmd, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents)
{
    ::hg::vulkan::vulkanFuncs.vkCmdBeginRenderPass(cmd, pRenderPassBegin, contents);
}

void vkCmdEndRenderPass(VkCommandBuffer cmd)
{
    ::hg::vulkan::vulkanFuncs.vkCmdEndRenderPass(cmd);
}

void vkCmdSetViewport(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkViewport* pViewports)
{
    ::hg::vulkan::vulkanFuncs.vkCmdSetViewport(cmd, first, count, pViewports);
}

void vkCmdSetScissor(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkRect2D* pScissors)
{
    ::hg::vulkan::vulkanFuncs.vkCmdSetScissor(cmd, first, count, pScissors);
}

void vkCmdBindPipeline(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipeline pipeline)
{
    ::hg::vulkan::vulkanFuncs.vkCmdBindPipeline(cmd, bindPoint, pipeline);
}

void vkCmdBindDescriptorSets(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t count, const VkDescriptorSet* pSets, uint32_t dynCount, const uint32_t* pDyn)
{
    ::hg::vulkan::vulkanFuncs.vkCmdBindDescriptorSets(cmd, bindPoint, layout, firstSet, count, pSets, dynCount, pDyn);
}

void vkCmdPushConstants(VkCommandBuffer cmd, VkPipelineLayout layout, VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void* pData)
{
    ::hg::vulkan::vulkanFuncs.vkCmdPushConstants(cmd, layout, stages, offset, size, pData);
}

void vkCmdBindVertexBuffers(VkCommandBuffer cmd, uint32_t first, uint32_t count, const VkBuffer* pBufs, const VkDeviceSize* pOffsets)
{
    ::hg::vulkan::vulkanFuncs.vkCmdBindVertexBuffers(cmd, first, count, pBufs, pOffsets);
}

void vkCmdBindIndexBuffer(VkCommandBuffer cmd, VkBuffer buf, VkDeviceSize offset, VkIndexType type)
{
    ::hg::vulkan::vulkanFuncs.vkCmdBindIndexBuffer(cmd, buf, offset, type);
}

void vkCmdDraw(VkCommandBuffer cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    ::hg::vulkan::vulkanFuncs.vkCmdDraw(cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

void vkCmdDrawIndexed(VkCommandBuffer cmd, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    ::hg::vulkan::vulkanFuncs.vkCmdDrawIndexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void vkCmdDispatch(VkCommandBuffer cmd, uint32_t x, uint32_t y, uint32_t z)
{
    ::hg::vulkan::vulkanFuncs.vkCmdDispatch(cmd, x, y, z);
}

} // extern "C"
