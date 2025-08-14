#include "hg_vulkan.h"

#include "hg_utils.h"

namespace hg {

#ifdef NDEBUG
inline constexpr std::array<const char*, 0> ValidationLayers{};
#else
inline constexpr std::array ValidationLayers{"VK_LAYER_KHRONOS_validation"};
#endif
constexpr std::array DeviceExtensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
};

static VkBool32 debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void*
) {
    if (severity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                 |  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
        std::printf("Vulkan Info: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::printf("Vulkan Warning: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::printf("Vulkan Error: %s\n", callback_data->pMessage);
    } else {
        std::printf("Vulkan Unknown: %s\n", callback_data->pMessage);
    }
    return VK_FALSE;
}

const VkDebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfo{
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debug_callback,
};

static Result<Slice<const char*>> get_instance_extensions(Vk& vk) {
    u32 sdl_extension_count = 0;
    const char* const* sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
    if (sdl_extensions == nullptr)
        ERRORF("Failed to get required instance extensions from SDL: {}", SDL_GetError());

    auto required_extensions = ok(vk.stack.alloc<const char*>(sdl_extension_count + 1));
    std::copy(sdl_extensions, sdl_extensions + sdl_extension_count, required_extensions->data);
#ifndef NDEBUG
    required_extensions->data[sdl_extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

    u32 extension_count = 0;
    VkResult ext_count_res = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    switch (ext_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            LOG_WARN("Vulkan incomplete instance extension enumeration");
            break;
        }
        case VK_ERROR_LAYER_NOT_PRESENT: {
            LOG_ERROR("Vulkan layer not present");
            return Err::VulkanLayerUnavailable;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    auto extensions = vk.stack.alloc<VkExtensionProperties>(extension_count);
    DEFER(vk.stack.dealloc(extensions));

    VkResult ext_res = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data);
    switch (ext_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            LOG_WARN("Vulkan incomplete instance extension enumeration");
            break;
        }
        case VK_ERROR_LAYER_NOT_PRESENT: {
            LOG_ERROR("Vulkan layer not present");
            return Err::VulkanLayerUnavailable;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    if (!std::ranges::all_of(*required_extensions, [&](const char* required) {
        return std::ranges::any_of(extensions, [&](const VkExtensionProperties& extension) {
            return strcmp(required, extension.extensionName) == 0;
        });
    })) return Err::VulkanExtensionUnavailable;

    return required_extensions;
}

static Result<VkInstance> create_instance(Vk& vk) {
    const VkApplicationInfo app_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hurdy Gurdy",
        .applicationVersion = 0,
        .pEngineName = "Hurdy Gurdy",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };

    const auto extensions = get_instance_extensions(vk);
    DEFER(vk.stack.dealloc(*extensions));
    if (extensions.has_err())
        return extensions.err();

    const VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &DebugUtilsMessengerCreateInfo,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = to_u32(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = to_u32(extensions->count),
        .ppEnabledExtensionNames = extensions->data,
    };

    VkInstance instance;
    const VkResult result = vkCreateInstance(&instance_info, nullptr, &instance);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_LAYER_NOT_PRESENT: {
            LOG_WARN("Vulkan layer not present");
            return Err::VulkanLayerUnavailable;
        }
        case VK_ERROR_EXTENSION_NOT_PRESENT: {
            LOG_WARN("Vulkan extension not present");
            return Err::VulkanExtensionUnavailable;
        }
        case VK_ERROR_INCOMPATIBLE_DRIVER: {
            LOG_WARN("Vulkan incompatible driver");
            return Err::VulkanIncompatibleDriver;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }

    return ok(instance);
}

static VkDebugUtilsMessengerEXT create_debug_messenger(Vk& vk) {
#ifdef NDEBUG
    return nullptr;
#else
    ASSERT(vk.instance != nullptr);

    VkDebugUtilsMessengerEXT messenger = nullptr;
    const VkResult result = g_pfn.vkCreateDebugUtilsMessengerEXT(
        vk.instance,
        &DebugUtilsMessengerCreateInfo,
        nullptr,
        &messenger
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        default: ERROR("Unexpected Vulkan error");
    }

    return messenger;
#endif
}

static Result<u32> find_queue_family(Vk& vk, const VkPhysicalDevice gpu) {
    ASSERT(gpu != nullptr);

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, nullptr);
    if (queue_family_count == 0)
        return Err::VkQueueFamilyUnavailable;

    auto queue_families = vk.stack.alloc<VkQueueFamilyProperties>(queue_family_count);
    DEFER(vk.stack.dealloc(queue_families));
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_families.data);

    const auto queue_family = std::ranges::find_if(queue_families, [](const VkQueueFamilyProperties family) {
        return static_cast<bool>(family.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));
    });
    if (queue_family == end(queue_families))
        return Err::VkQueueFamilyUnavailable;

    return ok(static_cast<u32>(queue_family - begin(queue_families)));
}

static Slice<VkPhysicalDevice> get_gpus(Vk& vk) {
    ASSERT(vk.instance != nullptr);

    u32 gpu_count = 0;
    VkResult gpu_count_res = vkEnumeratePhysicalDevices(vk.instance, &gpu_count, nullptr);
    switch (gpu_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            LOG_WARN("Vulkan incomplete gpu enumeration");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }

    auto gpus = vk.stack.alloc<VkPhysicalDevice>(gpu_count);
    auto gpu_result = vkEnumeratePhysicalDevices(vk.instance, &gpu_count, gpus.data);
    switch (gpu_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            LOG_WARN("Vulkan incomplete gpu enumeration");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }

    return gpus;
}

static Result<VkPhysicalDevice> find_gpu(Vk& vk) {
    ASSERT(vk.instance != nullptr);

    auto gpus = get_gpus(vk);
    if (gpus.count == 0)
        return Err::NoCompatibleVkPhysicalDevice;
    DEFER(vk.stack.dealloc(gpus));

    for (const auto gpu : gpus) {
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(gpu, &features);
        if (features.sampleRateShading != VK_TRUE || features.samplerAnisotropy != VK_TRUE)
            continue;

        u32 extension_count = 0;
        const VkResult ext_count_res = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extension_count, nullptr);
        switch (ext_count_res) {
            case VK_SUCCESS: break;
            case VK_INCOMPLETE: {
                LOG_WARN("Vulkan incomplete gpu extension enumeration");
                break;
            }
            case VK_ERROR_LAYER_NOT_PRESENT: {
                LOG_WARN("Vulkan layer not present");
                return Err::VulkanLayerUnavailable;
            }
            case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
            default: ERROR("Unexpected Vulkan error");
        }

        auto extensions = vk.stack.alloc<VkExtensionProperties>(extension_count);
        DEFER(vk.stack.dealloc(extensions));
        const VkResult ext_res = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extension_count, extensions.data);
        switch (ext_res) {
            case VK_SUCCESS: break;
            case VK_INCOMPLETE: {
                LOG_WARN("Vulkan incomplete gpu extension enumeration");
                break;
            }
            case VK_ERROR_LAYER_NOT_PRESENT: {
                LOG_WARN("Vulkan layer not present");
                return Err::VulkanLayerUnavailable;
            }
            case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
            default: ERROR("Unexpected Vulkan error");
        }

        if (!std::ranges::all_of(DeviceExtensions, [&](const char* required) -> bool {
            return std::ranges::any_of(extensions, [&](const VkExtensionProperties extension) -> bool {
                return strcmp(required, extension.extensionName);
            });
        })) continue;

        if (find_queue_family(vk, gpu).has_err())
            continue;

        ASSERT(gpu != nullptr);
        return ok(gpu);
    }

    return Err::NoCompatibleVkPhysicalDevice;
}

static Result<VkDevice> create_device(Vk& vk) {
    ASSERT(vk.gpu != nullptr);
    ASSERT(vk.queue_family_index != UINT32_MAX);

    VkPhysicalDeviceBufferAddressFeaturesEXT buffer_address_feature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_ADDRESS_FEATURES_EXT,
        .pNext = nullptr,
        .bufferDeviceAddress = VK_TRUE,
        .bufferDeviceAddressCaptureReplay = VK_FALSE,
        .bufferDeviceAddressMultiDevice = VK_FALSE,
    };
    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .pNext = &buffer_address_feature,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
    };
    VkPhysicalDeviceShaderObjectFeaturesEXT shader_object_feature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
        .pNext = &descriptor_indexing_features,
        .shaderObject = VK_TRUE,
    };
    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext = &shader_object_feature,
        .dynamicRendering = VK_TRUE,
    };
    VkPhysicalDeviceSynchronization2Features synchronization2_feature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = &dynamic_rendering_feature,
        .synchronization2 = VK_TRUE,
    };
    VkPhysicalDeviceFeatures features{
        .sampleRateShading = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };

    constexpr float queue_priority = 1.0f;
    const VkDeviceQueueCreateInfo queue_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vk.queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    const VkDeviceCreateInfo device_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &synchronization2_feature,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledLayerCount = to_u32(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = to_u32(DeviceExtensions.size()),
        .ppEnabledExtensionNames = DeviceExtensions.data(),
        .pEnabledFeatures = &features,
    };

    VkDevice device = nullptr;
    const VkResult result = vkCreateDevice(vk.gpu, &device_info, nullptr, &device);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_EXTENSION_NOT_PRESENT: {
            LOG_WARN("Vulkan extension not present");
            return Err::VulkanExtensionUnavailable;
        }
        case VK_ERROR_FEATURE_NOT_PRESENT: {
            LOG_WARN("Vulkan feature not present");
            return Err::VulkanFeatureUnavailable;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: ERROR("Vulkan initialization failed");
        case VK_ERROR_TOO_MANY_OBJECTS: ERROR("Vulkan too many objects");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    ASSERT(device != nullptr);
    return ok(device);
}

static VmaAllocator create_gpu_allocator(Vk& vk) {
    ASSERT(vk.instance != nullptr);
    ASSERT(vk.gpu != nullptr);
    ASSERT(vk.device != nullptr);

    VmaAllocatorCreateInfo info{};
    info.physicalDevice = vk.gpu;
    info.device = vk.device;
    info.instance = vk.instance;
    info.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator allocator = nullptr;
    const auto result = vmaCreateAllocator(&info, &allocator);
    if (result != VK_SUCCESS)
        ERROR("Could not create Vma allocator");
    return allocator;
}

static VkCommandPool create_command_pool(Vk& vk, const VkCommandPoolCreateFlags flags) {
    ASSERT(vk.device != nullptr);
    ASSERT(vk.queue_family_index != UINT32_MAX);

    const VkCommandPoolCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = flags,
        .queueFamilyIndex = vk.queue_family_index,
    };
    VkCommandPool pool = nullptr;
    const auto result = vkCreateCommandPool(vk.device, &info, nullptr, &pool);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: ERROR("Vulkan failed to initialize");
        default: ERROR("Vulkan failed to create command pool");
    }
    return pool;
}

Result<Vk> create_vk() {
    auto vk = ok<Vk>();

    vk->stack = Arena{malloc_slice<byte>(1024 * 64)};

    const auto instance = create_instance(*vk);
    if (instance.has_err())
        return instance.err();
    vk->instance = *instance;

    load_instance_procedures(vk->instance);

    vk->debug_messenger = create_debug_messenger(*vk);

    const auto gpu = find_gpu(*vk);
    if (gpu.has_err())
        return gpu.err();
    vk->gpu = *gpu;

    const auto queue_family = find_queue_family(*vk, vk->gpu);
    if (queue_family.has_err())
        return queue_family.err();
    vk->queue_family_index = *queue_family;

    const auto device = create_device(*vk);
    if (device.has_err())
        return device.err();
    vk->device = *device;

    load_device_procedures(vk->device);

    vkGetDeviceQueue(vk->device, vk->queue_family_index, 0, &vk->queue);
    if (vk->queue == nullptr)
        return Err::VkQueueUnavailable;

    vk->gpu_allocator = create_gpu_allocator(*vk);

    vk->command_pool = create_command_pool(
        *vk, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    );
    vk->single_time_command_pool = create_command_pool(
        *vk, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
    );

    return vk;
}

void destroy_vk(Vk& vk) {
    ASSERT(vk.instance != nullptr);
#ifndef NDEBUG
    ASSERT(vk.debug_messenger != nullptr);
#endif
    ASSERT(vk.device != nullptr);
    ASSERT(vk.gpu_allocator != nullptr);
    ASSERT(vk.command_pool != nullptr);
    ASSERT(vk.single_time_command_pool != nullptr);

    const VkResult wait_result = vkDeviceWaitIdle(vk.device);
    switch (wait_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    vkDestroyCommandPool(vk.device, vk.single_time_command_pool, nullptr);
    vkDestroyCommandPool(vk.device, vk.command_pool, nullptr);
    vmaDestroyAllocator(vk.gpu_allocator);
    vkDestroyDevice(vk.device, nullptr);
#ifndef NDEBUG
    g_pfn.vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debug_messenger, nullptr);
#endif
    vkDestroyInstance(vk.instance, nullptr);

    free_slice(vk.stack.release());
}

void load_instance_procedures(VkInstance instance) {
    g_pfn.vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
    );
    if (g_pfn.vkCreateDebugUtilsMessengerEXT == nullptr)
        ERROR("Could not find vkCreateDebugUtilsMessengerEXT");

    g_pfn.vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")
    );
    if (g_pfn.vkDestroyDebugUtilsMessengerEXT == nullptr)
        ERROR("Could not find vkDestroyDebugUtilsMessengerEXT");
}

void load_device_procedures(VkDevice device) {
    g_pfn.vkCreateShadersEXT = reinterpret_cast<PFN_vkCreateShadersEXT>(
        vkGetDeviceProcAddr(device, "vkCreateShadersEXT")
    );
    if (g_pfn.vkCreateShadersEXT == nullptr)
        ERROR("Could not find vkCreateShadersEXT");

    g_pfn.vkDestroyShaderEXT = reinterpret_cast<PFN_vkDestroyShaderEXT>(
        vkGetDeviceProcAddr(device, "vkDestroyShaderEXT")
    );
    if (g_pfn.vkDestroyShaderEXT == nullptr)
        ERROR("Could not find vkDestroyShaderEXT");

    g_pfn.vkCmdSetPolygonModeEXT = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
        vkGetDeviceProcAddr(device, "vkCmdSetPolygonModeEXT")
    );
    if (g_pfn.vkCmdSetPolygonModeEXT == nullptr)
        ERROR("Could not find vkCmdSetPolygonModeEXT");

    g_pfn.vkCmdSetRasterizationSamplesEXT = reinterpret_cast<PFN_vkCmdSetRasterizationSamplesEXT>(
        vkGetDeviceProcAddr(device, "vkCmdSetRasterizationSamplesEXT")
    );
    if (g_pfn.vkCmdSetRasterizationSamplesEXT == nullptr)
        ERROR("Could not find vkCmdSetRasterizationSamplesEXT");

    g_pfn.vkCmdSetSampleMaskEXT = reinterpret_cast<PFN_vkCmdSetSampleMaskEXT>(
        vkGetDeviceProcAddr(device, "vkCmdSetSampleMaskEXT")
    );
    if (g_pfn.vkCmdSetSampleMaskEXT == nullptr)
        ERROR("Could not find vkCmdSetSampleMaskEXT");

    g_pfn.vkCmdSetAlphaToCoverageEnableEXT = reinterpret_cast<PFN_vkCmdSetAlphaToCoverageEnableEXT>(
        vkGetDeviceProcAddr(device, "vkCmdSetAlphaToCoverageEnableEXT")
    );
    if (g_pfn.vkCmdSetAlphaToCoverageEnableEXT == nullptr)
        ERROR("Could not find vkCmdSetAlphaToCoverageEnableEXT");

    g_pfn.vkCmdSetColorWriteMaskEXT = reinterpret_cast<PFN_vkCmdSetColorWriteMaskEXT>(
        vkGetDeviceProcAddr(device, "vkCmdSetColorWriteMaskEXT")
    );
    if (g_pfn.vkCmdSetColorWriteMaskEXT == nullptr)
        ERROR("Could not find vkCmdSetColorWriteMaskEXT");

    g_pfn.vkCmdSetColorBlendEnableEXT = reinterpret_cast<PFN_vkCmdSetColorBlendEnableEXT>(
        vkGetDeviceProcAddr(device, "vkCmdSetColorBlendEnableEXT")
    );
    if (g_pfn.vkCmdSetColorBlendEnableEXT == nullptr)
        ERROR("Could not find vkCmdSetColorBlendEnableEXT");

    g_pfn.vkCmdBindShadersEXT = reinterpret_cast<PFN_vkCmdBindShadersEXT>(
        vkGetDeviceProcAddr(device, "vkCmdBindShadersEXT")
    );
    if (g_pfn.vkCmdBindShadersEXT == nullptr)
        ERROR("Could not find vkCmdBindShadersEXT");

    g_pfn.vkCmdSetVertexInputEXT = reinterpret_cast<PFN_vkCmdSetVertexInputEXT>(
        vkGetDeviceProcAddr(device, "vkCmdSetVertexInputEXT")
    );
    if (g_pfn.vkCmdSetVertexInputEXT == nullptr)
        ERROR("Could not find vkCmdSetVertexInputEXT");
}

} // namespace hg
