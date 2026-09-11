#include "vulkan_internal.hpp"

#include "internal.hpp"
#include "hg/error.hpp"
#include "hg/utility.hpp"
#include "hg/array.hpp"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

namespace hg {
using namespace vulkan;

static const char* deviceExtensions[]{
    "VK_KHR_swapchain",
};

static VkBool32 debugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    static_cast<void>(type);
    static_cast<void>(userData);

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Error: %s\n", callbackData->pMessage));
        abort();
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Warning: %s\n", callbackData->pMessage));
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Info: %s\n", callbackData->pMessage));
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Verbose: %s\n", callbackData->pMessage));
    } else {
        static_cast<void>(fprintf(stderr, "Vulkan Unknown: %s\n", callbackData->pMessage));
    }
    return VK_FALSE;
}

static const VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerInfo{
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    nullptr,
    0,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    debugCallback,
    nullptr,
};

namespace vulkan {

VulkanState vk;

const char* vkResultToStr(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_VALIDATION_FAILED:
            return "VK_ERROR_VALIDATION_FAILED";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_NOT_PERMITTED:
            return "VK_ERROR_NOT_PERMITTED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT:
            return "VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
            return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
        case VK_PIPELINE_BINARY_MISSING_KHR:
            return "VK_PIPELINE_BINARY_MISSING_KHR";
        case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
            return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
    }
    return "Unrecognized Vulkan result";
}

} // namespace vulkan

u32 formatToSize(Format format)
{
    switch (format)
    {
        case Format_undefined: return 0;
        case Format_r8_unorm: return 1;
        case Format_rg8_unorm: return 2;
        case Format_rgba8_unorm: return 4;
        case Format_bgra8_unorm: return 4;
        case Format_rgba8_srgb: return 4;
        case Format_bgra8_srgb: return 4;
        case Format_r16_unorm: return 2;
        case Format_r16_sfloat: return 2;
        case Format_rg16_unorm: return 4;
        case Format_rg16_sfloat: return 4;
        case Format_rgba16_unorm: return 8;
        case Format_rgba16_sfloat: return 8;
        case Format_r32_uint: return 4;
        case Format_r32_sfloat: return 4;
        case Format_rg32_sfloat: return 8;
        case Format_rgba32_sfloat: return 16;
        case Format_a2b10g10r10_unorm_pack32: return 4;
        case Format_b10g11r11_ufloat_pack32: return 4;
        case Format_d16_unorm: return 2;
        case Format_d32_sfloat: return 4;
        case Format_s8_uint: return 1;
        case Format_d16_unorm_s8_uint: return 3;
        case Format_d24_unorm_s8_uint: return 4;
        case Format_d32_sfloat_s8_uint: return 5;
        case Format_bc1_rgb_unorm_block: return 8;
        case Format_bc3_unorm_block: return 16;
        case Format_bc4_unorm_block: return 16;
        case Format_bc5_unorm_block: return 16;
        case Format_bc7_unorm_block: return 16;
        case Format_astc_4x4_unorm_block: return 16;
        case Format_astc_8x8_unorm_block: return 16;
    }
    return 0;
}

namespace vulkan {

static bool isDebugMessengerAvailable()
{
#ifdef HG_VK_DEBUG_MESSENGER
    ArenaScope scratch = getScratch();

    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    ArrayTemp<VkLayerProperties> layers{scratch, layerCount, layerCount};
    vkEnumerateInstanceLayerProperties(&layerCount, layers.vals);

    for (VkLayerProperties& layer : layers)
    {
        if (strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0)
            goto layerFound;
    }
    return false;
layerFound:

    u32 extCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    ArrayTemp<VkExtensionProperties> exts{scratch, extCount, extCount};
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.vals);

    for (VkExtensionProperties& ext : exts)
    {
        if (strcmp(ext.extensionName, "VK_EXT_debug_utils") == 0)
            goto extFound;
    }
    return false;
extFound:
    return true;
#endif
    return false;
}

static VkInstance createInstance(Span<StringView> extensions)
{
    if (extensions.count > 0)
        HG_ASSERT(extensions.data != nullptr);

    ArenaScope scratch = getScratch();

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hurdy Gurdy Application",
    appInfo.applicationVersion = 0;
    appInfo.pEngineName = "Hurdy Gurdy Engine";
    appInfo.engineVersion = 0;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;

    if (vk.enableDebugMessenger)
    {
        instanceInfo.pNext = &debugUtilsMessengerInfo;
        const char* layers[]{
            "VK_LAYER_KHRONOS_validation",
        };
        instanceInfo.enabledLayerCount = static_cast<u32>(size(layers));
        instanceInfo.ppEnabledLayerNames = layers;
    }

    ArrayTemp<const char*> extCStrs{scratch};
    for (u32 i = 0; i < extensions.count; ++i)
    {
        extCStrs.push(cString(scratch, extensions[i]));
    }
    if (vk.enableDebugMessenger)
        extCStrs.push("VK_EXT_debug_utils");
    instanceInfo.enabledExtensionCount = (u32)extCStrs.count;
    instanceInfo.ppEnabledExtensionNames = extCStrs.vals;

    VkInstance instance = nullptr;
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
    if (instance == nullptr)
    {
        setError("Failed to create Vulkan instance: %s", vkResultToStr(result));
    }

    return instance;
}

static VkDebugUtilsMessengerEXT createDebugUtilsMessenger()
{
    HG_ASSERT(vk.instance != nullptr);

    VkDebugUtilsMessengerEXT messenger = nullptr;
    VkResult result = vkCreateDebugUtilsMessengerEXT(vk.instance, &debugUtilsMessengerInfo, nullptr, &messenger);
    if (messenger == nullptr)
    {
        setError("Failed to create Vulkan debug messenger: %s", vkResultToStr(result));
    }

    return messenger;
}

static bool findQueueFamily(VkPhysicalDevice gpu, u32* queueFamily, VkQueueFlags queueFlags)
{
    HG_ASSERT(gpu != nullptr);
    HG_ASSERT(queueFamily != nullptr);

    ArenaScope scratch = getScratch();

    u32 familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
    VkQueueFamilyProperties* families = scratch.alloc<VkQueueFamilyProperties>(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families);

    for (u32 i = 0; i < familyCount; ++i)
    {
        if (families[i].queueFlags & queueFlags)
        {
            *queueFamily = i;
            return true;
        }
    }

    setError("Could not find Vulkan queue family");
    *queueFamily = (u32)-1;
    return false;
}

static VkPhysicalDevice findPhysicalDevice()
{
    HG_ASSERT(vk.instance != nullptr);

    ArenaScope scratch = getScratch();

    u32 gpuCount;
    vkEnumeratePhysicalDevices(vk.instance, &gpuCount, nullptr);
    VkPhysicalDevice* gpus = scratch.alloc<VkPhysicalDevice>(gpuCount);
    vkEnumeratePhysicalDevices(vk.instance, &gpuCount, gpus);

    ArrayTemp<VkExtensionProperties> extProps{scratch, 0, 0};

    for (u32 i = 0; i < gpuCount; ++i)
    {
        VkPhysicalDevice gpu = gpus[i];
        u32 family;

        u32 propCount = 0;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &propCount, nullptr);
        extProps.resize(propCount);
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &propCount, extProps.vals);

        for (u32 j = 0; j < static_cast<u32>(size(deviceExtensions)); j++)
        {
            for (u32 k = 0; k < propCount; k++)
            {
                if (strcmp(deviceExtensions[j], extProps[k].extensionName) == 0)
                    goto nextExt;
            }
            goto nextGpu;
nextExt:
            continue;
        }

        if (!findQueueFamily(gpu, &family,
                VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT))
            goto nextGpu;

        return gpu;

nextGpu:
        continue;
    }

    setError("Could not find suitable gpu");
    return nullptr;
}

static VkDevice createDevice()
{
    HG_ASSERT(vk.physicalDevice != nullptr);
    HG_ASSERT(vk.queueFamily != (u32)-1);

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeature{};
    descriptorIndexingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeature.pNext = nullptr;
    descriptorIndexingFeature.shaderInputAttachmentArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingPartiallyBound = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptorIndexingFeature.runtimeDescriptorArray = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{};
    dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature.pNext = &descriptorIndexingFeature;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features synchronization2Feature{};
    synchronization2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2Feature.pNext = &dynamicRenderingFeature;
    synchronization2Feature.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures features{};

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = vk.queueFamily;
    queueInfo.queueCount = 1;
    f32 queuePriority = 1.0f;
    queueInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = &synchronization2Feature;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = static_cast<u32>(size(deviceExtensions));
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceInfo.pEnabledFeatures = &features;

    VkDevice device = nullptr;
    VkResult result = vkCreateDevice(vk.physicalDevice, &deviceInfo, nullptr, &device);
    if (device == nullptr)
    {
        setError("Could not create VkDevice: %s", vkResultToStr(result));
    }

    return device;
}

static VmaAllocator createVma()
{
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = vk.physicalDevice;
    allocatorInfo.device = vk.device;
    allocatorInfo.instance = vk.instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator vma = nullptr;
    VkResult result = vmaCreateAllocator(&allocatorInfo, &vma);
    if (vma == nullptr)
    {
        setError("Could not create Vulkan memory allocator: %s", vkResultToStr(result));
    }

    return vma;
}

static VkDescriptorPool createBindlessDescriptorPool()
{
    VkDescriptorPoolSize sizes[]{
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, UINT16_MAX},
    };

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    info.maxSets = 1;
    info.poolSizeCount = static_cast<u32>(size(sizes));
    info.pPoolSizes = sizes;

    VkDescriptorPool pool = nullptr;
    [[maybe_unused]] VkResult result = vkCreateDescriptorPool(vk.device, &info, nullptr, &pool);
    if (pool == nullptr)
        HG_PANIC("Could not create VkDescriptorPool: %s\n", vkResultToStr(result));

    return pool;
}

static VkDescriptorSetLayout createBindlessDescriptorLayout()
{
    VkDescriptorSetLayoutBinding bindings[DescriptorType_count]{};
    VkDescriptorBindingFlags flags[DescriptorType_count]{};
    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = descriptorTypeToVk(static_cast<DescriptorType>(i));
        bindings[i].descriptorCount = UINT16_MAX;
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
                 | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
    flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagsInfo.bindingCount = static_cast<u32>(size(bindings));
    flagsInfo.pBindingFlags = flags;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = &flagsInfo;
    info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    info.bindingCount = static_cast<u32>(size(bindings));
    info.pBindings = bindings;

    VkDescriptorSetLayout layout = nullptr;
    [[maybe_unused]] VkResult result = vkCreateDescriptorSetLayout(vk.device, &info, nullptr, &layout);
    if (layout == nullptr)
        HG_PANIC("Could not create bindless VkDescriptorSetLayout: %s\n", vkResultToStr(result));

    return layout;
}

static Frame createFrame()
{
    Frame frame{};

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = vk.queueFamily;

    [[maybe_unused]]
    VkResult poolResult = vkCreateCommandPool(vk.device, &poolInfo, nullptr, &frame.cmdPool);
    if (frame.cmdPool == nullptr)
        HG_PANIC("Could not create Vulkan command pool: %s\n", vkResultToStr(poolResult));

    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = frame.cmdPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    [[maybe_unused]]
    VkResult cmdResult = vkAllocateCommandBuffers(vk.device, &cmdInfo, &frame.cmd);
    if (frame.cmd == nullptr)
        HG_PANIC("Could not create Vulkan command buffer: %s\n", vkResultToStr(cmdResult));

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    [[maybe_unused]]
    VkResult fenceResult = vkCreateFence(vk.device, &fenceInfo, nullptr, &frame.fence);
    if (frame.fence == nullptr)
        HG_PANIC("Could not create Vulkan fence: %s\n", vkResultToStr(fenceResult));

    return frame;
}

GpuDescriptor createBufferDescriptor(
    DescriptorType type,
    const GpuBuffer& buffer,
    u64 offset,
    u64 range)
{
    HG_ASSERT(type < DescriptorType_count);

    ArenaScope scratch = getScratch();

    GpuDescriptor desc = vk.descriptorPools[type].alloc();

    VkDescriptorBufferInfo bufferInfo{buffer.data->buffer, offset, range};

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vk.bindlessSet;
    write.dstBinding = type;
    write.dstArrayElement = desc.idx();
    write.descriptorCount = 1;
    write.descriptorType = descriptorTypeToVk(type);
    write.pBufferInfo = &bufferInfo;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk.device, 1, &write, 0, nullptr);

    return desc;
}

GpuDescriptor createImageDescriptor(
    DescriptorType type,
    const GpuView& imageView,
    GpuLayout imageLayout)
{
    HG_ASSERT(type < DescriptorType_count);

    ArenaScope scratch = getScratch();

    GpuDescriptor desc = vk.descriptorPools[type].alloc();

    VkDescriptorImageInfo imageInfo{
        imageView.data->sampler,
        imageView.data->view,
        gpuLayoutToVk(imageLayout)
    };

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vk.bindlessSet;
    write.dstBinding = type;
    write.dstArrayElement = desc.idx();
    write.descriptorCount = 1;
    write.descriptorType = descriptorTypeToVk(type);
    write.pBufferInfo = nullptr;
    write.pImageInfo = &imageInfo;
    write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk.device, 1, &write, 0, nullptr);

    return desc;
}

void descriptorDestroy(GpuDescriptor desc, DescriptorType type)
{
    if (desc != nullHandle)
        vk.descriptorPools[type].free(desc);
}

static VkSampler samplerCreate(SamplerInfo* desc)
{
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = gpuFilterToVk(desc->filter);
    info.minFilter = gpuFilterToVk(desc->filter);
    info.mipmapMode = desc->filter == GpuFilter_linear
        ? VK_SAMPLER_MIPMAP_MODE_LINEAR
        : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = gpuSamplerAddressModeToVk(desc->mode);
    info.addressModeV = gpuSamplerAddressModeToVk(desc->mode);
    info.addressModeW = gpuSamplerAddressModeToVk(desc->mode);
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 1000.0f;
    info.borderColor = gpuSamplerBorderToVk(desc->border);

    VkSampler sampler = nullptr;
    [[maybe_unused]]     VkResult result = vkCreateSampler(vk.device, &info, nullptr, &sampler);
    if (sampler == nullptr)
        HG_PANIC("Could not create VkSampler: %s\n", vkResultToStr(result));

    return sampler;
}

VkSampler samplerGet(
    GpuFilter filter,
    GpuSamplerEdgeMode addressMode,
    GpuSamplerBorder borderColor)
{
    SamplerInfo desc = {filter, addressMode, borderColor};
    VkSampler* sampler = vk.samplers.get(desc);
    if (sampler == nullptr)
    {
        sampler = vk.samplers.add(desc, samplerCreate(&desc));
    }
    return *sampler;
}

} // namespace vulkan

namespace internal {

bool initGpu()
{
    ArenaScope scratch = getScratch();

    if (!loadVulkan())
        goto loadFailed;

    vk.enableDebugMessenger = isDebugMessengerAvailable();

    vk.instance = createInstance(getPlatformVulkanExtensions(scratch));
    if (vk.instance == nullptr)
        goto instanceFailed;
    if (!loadVulkanInstanceFuncs(vk.instance))
        goto loadInstanceFailed;

    if (vk.enableDebugMessenger)
    {
        vk.debugMessenger = createDebugUtilsMessenger();
        if (vk.debugMessenger == nullptr)
            goto debugMessengerFailed;
    }

    vk.physicalDevice = findPhysicalDevice();
    if (vk.physicalDevice == nullptr)
        goto physicalDeviceFailed;

    findQueueFamily(vk.physicalDevice, &vk.queueFamily,
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT);
    if (vk.queueFamily == (u32)-1)
        goto queueFamilyFailed;

    vk.device = createDevice();
    if (vk.device == nullptr)
        goto deviceFailed;

    if (!loadVulkanDeviceFuncs(vk.device))
        goto loadDeviceFailed;

    vkGetDeviceQueue(vk.device, vk.queueFamily, 0, &vk.queue);
    if (vk.queue == nullptr)
        goto queueFailed;

    vk.vma = createVma();
    if (vk.vma == nullptr)
        goto vmaFailed;

    {
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolInfo.queueFamilyIndex = vk.queueFamily;

        [[maybe_unused]]         VkResult result = vkCreateCommandPool(vk.device, &cmdPoolInfo, nullptr, &vk.cmdPool);
        if (vk.cmdPool == nullptr)
            HG_PANIC("Could not create Vulkan command pool: %s\n", vkResultToStr(result));
    }

    vk.bindlessPool = createBindlessDescriptorPool();
    vk.bindlessLayout = createBindlessDescriptorLayout();
    {
        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = vk.bindlessPool;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &vk.bindlessLayout;

        [[maybe_unused]]         VkResult result = vkAllocateDescriptorSets(vk.device, &info, &vk.bindlessSet);
        if (vk.bindlessSet == nullptr)
            HG_PANIC("Could not allocate bindless VkDescriptorSet: %s\n", vkResultToStr(result));
    }

    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        vk.descriptorPools[i] = HandlePool{};
    }

    vk.samplers = Map<SamplerInfo, VkSampler>(
        2 *
        GpuFilter_count *
        GpuSamplerEdgeMode_count *
        GpuSamplerBorder_count);

    vk.frameCount = 2;
    vk.currentFrame = 0;
    vk.frames = heapAlloc<Frame>(vk.frameCount);
    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        new (vk.frames + i) Frame{createFrame()};
    }

    return true;

vmaFailed:
queueFailed:
    vkDestroyDevice(vk.device, nullptr);
loadDeviceFailed:
deviceFailed:
queueFamilyFailed:
physicalDeviceFailed:
    if (vk.enableDebugMessenger)
        vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debugMessenger, nullptr);
debugMessengerFailed:
loadInstanceFailed:
    vkDestroyInstance(vk.instance, nullptr);
instanceFailed:
    unloadVulkan();
loadFailed:
    return false;
}

void deinitGpu()
{
    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        vkDestroyFence(vk.device, vk.frames[i].fence, nullptr);
        vkDestroyCommandPool(vk.device, vk.frames[i].cmdPool, nullptr);
        vk.frames[i].swapchains = {};
    }
    heapFree(vk.frames, vk.frameCount);

    vk.samplers.forEach([](const SamplerInfo&, VkSampler& sampler)
    {
        vkDestroySampler(vk.device, sampler, nullptr);
    });
    vk.samplers = {};

    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        vk.descriptorPools[i].reset();
    }

    vkDestroyDescriptorSetLayout(vk.device, vk.bindlessLayout, nullptr);
    vkDestroyDescriptorPool(vk.device, vk.bindlessPool, nullptr);

    vkDestroyCommandPool(vk.device, vk.cmdPool, nullptr);

    vmaDestroyAllocator(vk.vma);

    vkDestroyDevice(vk.device, nullptr);

    if (vk.enableDebugMessenger)
        vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debugMessenger, nullptr);

    vkDestroyInstance(vk.instance, nullptr);

    unloadVulkan();
}

void* getVulkanInstance()
{
    return vk.instance;
}

void* getVulkanInstanceProcAddr(const char* name)
{
    return reinterpret_cast<void*>(vkGetInstanceProcAddr(static_cast<VkInstance>(vk.instance), name));
}

void initImGuiGpu(
    const GpuSwapchain& swap,
    Format colorFormat,
    Format depthFormat,
    Format stencilFormat)
{
    HG_ASSERT(colorFormat != Format_undefined);

    ArenaScope scratch = getScratch();

    VkFormat colorVkFormat = formatToVk(colorFormat);
    VkFormat depthVkFormat = formatToVk(depthFormat);
    VkFormat stencilVkFormat = formatToVk(stencilFormat);

    ImGui_ImplVulkan_InitInfo imguiInfo{};
    imguiInfo.Instance = vk.instance;
    imguiInfo.PhysicalDevice = vk.physicalDevice;
    imguiInfo.Device = vk.device;
    imguiInfo.QueueFamily = vk.queueFamily;
    imguiInfo.Queue = vk.queue;
    imguiInfo.DescriptorPoolSize = 1000;
    imguiInfo.MinImageCount = swap.imageCount();
    imguiInfo.ImageCount = swap.imageCount();
    imguiInfo.MinAllocationSize = 1 << 20;
    imguiInfo.UseDynamicRendering = true;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorVkFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = depthVkFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = stencilVkFormat;

    ImGui_ImplVulkan_Init(&imguiInfo);
}

void deinitImGuiGpu()
{
    ImGui_ImplVulkan_Shutdown();
}

void beginImGuiFrameGpu()
{
    ImGui_ImplVulkan_NewFrame();
}

} // namespace internal

void* createImGuiTexture(const GpuView& view, GpuLayout layout)
{
    return ImGui_ImplVulkan_AddTexture(view.data->sampler, view.data->view, gpuLayoutToVk(layout));
}

void destroyImGuiTexture(void* texture)
{
    ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(texture));
}

void renderImGui(GpuCmd* cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), reinterpret_cast<VkCommandBuffer>(cmd));
}

} // namespace hg
