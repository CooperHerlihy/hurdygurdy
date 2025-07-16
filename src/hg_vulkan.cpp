#include "hg_vulkan.h"

#include "SDL3/SDL_vulkan.h"
#include "hg_pch.h"
#include "hg_utils.h"
#include "hg_load.h"

#include <algorithm>
#include <array>
#include <fstream>

namespace hg {

#ifdef NDEBUG
inline constexpr std::array<const char*, 0> ValidationLayers{};
#else
inline constexpr std::array ValidationLayers{"VK_LAYER_KHRONOS_validation"};
#endif
constexpr std::array DeviceExtensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
};

static vk::Bool32 debug_callback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    const vk::DebugUtilsMessageTypeFlagsEXT,
    const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void*
) {
    if (severity & (vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
                 |  vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)) {
        std::printf("Vulkan Info: %s\n", callback_data->pMessage);
    } else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        std::printf("Vulkan Warning: %s\n", callback_data->pMessage);
    } else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        std::printf("Vulkan Error: %s\n", callback_data->pMessage);
    } else {
        std::printf("Vulkan Unknown: %s\n", callback_data->pMessage);
    }
    return VK_FALSE;
}

const vk::DebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfo{
    .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                     | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                     | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
    .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                 | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                 | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
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

    const auto extensions = vk::enumerateInstanceExtensionProperties();
    switch (extensions.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eIncomplete: {
            LOG_WARN("Vulkan incomplete instance extension enumeration");
            break;
        }
        case vk::Result::eErrorLayerNotPresent: {
            LOG_WARN("Vulkan layer not present");
            return Err::VulkanLayerUnavailable;
        }
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    if (!std::ranges::all_of(*required_extensions, [&](const char* required) {
        return std::ranges::any_of(extensions.value, [&](const VkExtensionProperties& extension) {
            return strcmp(required, extension.extensionName) == 0;
        });
    })) return Err::VulkanExtensionUnavailable;

    return required_extensions;
}

static Result<vk::Instance> create_instance(Vk& vk) {
    const vk::ApplicationInfo app_info{
        .pApplicationName = "Hurdy Gurdy",
        .applicationVersion = 0,
        .pEngineName = "Hurdy Gurdy",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };

    const auto extensions = get_instance_extensions(vk);
    defer(vk.stack.dealloc(*extensions));
    if (extensions.has_err())
        return extensions.err();

    const auto instance = vk::createInstance({
        .pNext = &DebugUtilsMessengerCreateInfo,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = to_u32(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = to_u32(extensions->count),
        .ppEnabledExtensionNames = extensions->data,
    });
    switch (instance.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorLayerNotPresent: {
            LOG_WARN("Vulkan layer not present");
            return Err::VulkanLayerUnavailable;
        }
        case vk::Result::eErrorExtensionNotPresent: {
            LOG_WARN("Vulkan extension not present");
            return Err::VulkanExtensionUnavailable;
        }
        case vk::Result::eErrorIncompatibleDriver: {
            LOG_WARN("Vulkan incompatible driver");
            return Err::VulkanIncompatibleDriver;
        }
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInitializationFailed: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }

    ASSERT(instance.value != nullptr);
    return ok(instance.value);
}

static vk::DebugUtilsMessengerEXT create_debug_messenger(Vk& vk) {
#ifdef NDEBUG
    return nullptr;
#else
    ASSERT(vk.instance != nullptr);

    const auto messenger = vk.instance.createDebugUtilsMessengerEXT(DebugUtilsMessengerCreateInfo);
        switch (messenger.result) {
            case vk::Result::eSuccess: break;
            case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
            default: ERROR("Unexpected Vulkan error");
    }
    return messenger.value;
#endif
}

static Result<u32> find_queue_family(Vk& vk, const vk::PhysicalDevice gpu) {
    ASSERT(gpu != nullptr);

    u32 queue_family_count = 0;
    gpu.getQueueFamilyProperties(&queue_family_count, nullptr);
    if (queue_family_count == 0)
        return Err::VkQueueFamilyUnavailable;

    auto queue_families = vk.stack.alloc<vk::QueueFamilyProperties>(queue_family_count);
    defer(vk.stack.dealloc(queue_families));
    gpu.getQueueFamilyProperties(&queue_family_count, queue_families.data);

    const auto queue_family = std::ranges::find_if(queue_families, [](const vk::QueueFamilyProperties family) {
        return static_cast<bool>(family.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute));
    });
    if (queue_family == end(queue_families))
        return Err::VkQueueFamilyUnavailable;

    return ok(static_cast<u32>(queue_family - begin(queue_families)));
}

static Result<vk::PhysicalDevice> find_gpu(Vk& vk) {
    ASSERT(vk.instance != nullptr);

    u32 gpu_count = 0;
    auto gpu_count_result = vk.instance.enumeratePhysicalDevices(&gpu_count, nullptr);
    switch (gpu_count_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eIncomplete: {
            LOG_WARN("Vulkan incomplete gpu enumeration");
            break;
        }
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInitializationFailed: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }
    if (gpu_count == 0) {
        LOG_WARN("No gpu found");
        return Err::NoCompatibleVkPhysicalDevice;
    }

    auto gpus = vk.stack.alloc<vk::PhysicalDevice>(gpu_count);
    defer(vk.stack.dealloc(gpus));
    auto gpu_result = vk.instance.enumeratePhysicalDevices(&gpu_count, gpus.data);
    switch (gpu_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eIncomplete: {
            LOG_WARN("Vulkan incomplete gpu enumeration");
            break;
        }
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInitializationFailed: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }

    for (const auto gpu : gpus) {
        const auto features = gpu.getFeatures();
        if (features.sampleRateShading != vk::True || features.samplerAnisotropy != vk::True)
            continue;

        const auto extensions = gpu.enumerateDeviceExtensionProperties();
        switch (extensions.result) {
            case vk::Result::eSuccess: break;
            case vk::Result::eIncomplete: {
                LOG_WARN("Vulkan incomplete gpu extension enumeration");
                break;
            }
            case vk::Result::eErrorLayerNotPresent: {
                LOG_WARN("Vulkan layer not present");
                return Err::VulkanLayerUnavailable;
            }
            case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
            case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
            default: ERROR("Unexpected Vulkan error");
        }

        if (!std::ranges::all_of(DeviceExtensions, [&](const char* required) {
            return std::ranges::any_of(extensions.value, [&](const vk::ExtensionProperties extension) {
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

static Result<vk::Device> create_device(Vk& vk) {
    ASSERT(vk.gpu != nullptr);
    ASSERT(vk.queue_family_index != UINT32_MAX);

    vk::PhysicalDeviceBufferAddressFeaturesEXT buffer_address_feature{.pNext = nullptr, .bufferDeviceAddress = vk::True};
    vk::PhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{
        .pNext = &buffer_address_feature,
        // .shaderInputAttachmentArrayDynamicIndexing = true,
        // .shaderUniformTexelBufferArrayDynamicIndexing = true,
        // .shaderStorageTexelBufferArrayDynamicIndexing = true,
        // .shaderUniformBufferArrayNonUniformIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        // .shaderStorageBufferArrayNonUniformIndexing = true,
        // .shaderStorageImageArrayNonUniformIndexing = true,
        // .shaderInputAttachmentArrayNonUniformIndexing = true,
        // .shaderUniformTexelBufferArrayNonUniformIndexing = true,
        // .shaderStorageTexelBufferArrayNonUniformIndexing = true,
        // .descriptorBindingUniformBufferUpdateAfterBind = true,
        // .descriptorBindingSampledImageUpdateAfterBind = true,
        // .descriptorBindingStorageImageUpdateAfterBind = true,
        // .descriptorBindingStorageBufferUpdateAfterBind = true,
        // .descriptorBindingUniformTexelBufferUpdateAfterBind = true,
        // .descriptorBindingStorageTexelBufferUpdateAfterBind = true,
        // .descriptorBindingUpdateUnusedWhilePending = true,
        .descriptorBindingPartiallyBound = true,
        // .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
    };
    vk::PhysicalDeviceShaderObjectFeaturesEXT shader_object_feature{.pNext = &descriptor_indexing_features, .shaderObject = vk::True};
    vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature{.pNext = &shader_object_feature, .dynamicRendering = vk::True};
    vk::PhysicalDeviceSynchronization2Features synchronization2_feature{.pNext = &dynamic_rendering_feature, .synchronization2 = vk::True};
    constexpr vk::PhysicalDeviceFeatures features{
        .sampleRateShading = vk::True,
        .samplerAnisotropy = vk::True,
    };

    constexpr float queue_priority = 1.0f;
    const vk::DeviceQueueCreateInfo queue_info{
        .queueFamilyIndex = vk.queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    const auto device = vk.gpu.createDevice({
        .pNext = synchronization2_feature,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledLayerCount = to_u32(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = to_u32(DeviceExtensions.size()),
        .ppEnabledExtensionNames = DeviceExtensions.data(),
        .pEnabledFeatures = &features,
    });
    switch (device.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorExtensionNotPresent: {
            LOG_WARN("Vulkan extension not present");
            return Err::VulkanExtensionUnavailable;
        }
        case vk::Result::eErrorFeatureNotPresent: {
            LOG_WARN("Vulkan feature not present");
            return Err::VulkanFeatureUnavailable;
        }
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInitializationFailed: ERROR("Vulkan initialization failed");
        case vk::Result::eErrorTooManyObjects: ERROR("Vulkan too many objects");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    ASSERT(device.value != nullptr);
    return ok(device.value);
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

vk::CommandPool create_command_pool(Vk& vk, const vk::CommandPoolCreateFlags flags) {
    ASSERT(vk.device != nullptr);
    ASSERT(vk.queue_family_index != UINT32_MAX);

    const auto pool = vk.device.createCommandPool({
        .flags = flags,
        .queueFamilyIndex = vk.queue_family_index,
    });
    switch (pool.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
    return pool.value;
}

Result<Vk> Vk::create() {
    auto vk = ok<Vk>();

    vk->stack = Arena{malloc_slice<byte>(1024 * 64)};

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    const auto instance = create_instance(*vk);
    if (instance.has_err())
        return instance.err();
    vk->instance = *instance;

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk->instance);

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

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk->device);

    vk->queue = vk->device.getQueue(vk->queue_family_index, 0);
    if (vk->queue == nullptr)
        return Err::VkQueueUnavailable;

    vk->gpu_allocator = create_gpu_allocator(*vk);

    vk->command_pool = create_command_pool(*vk, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    vk->single_time_command_pool = create_command_pool(
        *vk, vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient
    );

    ASSERT(vk->instance != nullptr);
    ASSERT(vk->debug_messenger != nullptr);
    ASSERT(vk->gpu != nullptr);
    ASSERT(vk->device != nullptr);
    ASSERT(vk->gpu_allocator != nullptr);
    ASSERT(vk->queue_family_index != UINT32_MAX);
    ASSERT(vk->queue != nullptr);
    ASSERT(vk->command_pool != nullptr);
    ASSERT(vk->single_time_command_pool != nullptr);
    return vk;
}

void Vk::destroy() {
    const auto wait_result = device.waitIdle();
    switch (wait_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    ASSERT(instance != nullptr);
#ifndef NDEBUG
    ASSERT(debug_messenger != nullptr);
#endif
    ASSERT(device != nullptr);
    ASSERT(gpu_allocator != nullptr);
    ASSERT(command_pool != nullptr);
    ASSERT(single_time_command_pool != nullptr);

    device.destroyCommandPool(single_time_command_pool);
    device.destroyCommandPool(command_pool);
    vmaDestroyAllocator(gpu_allocator);
    device.destroy();
#ifndef NDEBUG
    instance.destroyDebugUtilsMessengerEXT(debug_messenger);
#endif
    instance.destroy();

    free_slice(stack.release());
}

GpuBuffer GpuBuffer::create(Vk& vk, const Config& config) {
    ASSERT(config.size != 0);
    ASSERT(config.usage != vk::BufferUsageFlags{});

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = config.size;
    buffer_info.usage = static_cast<VkBufferUsageFlags>(config.usage);

    VmaAllocationCreateInfo alloc_info{};
    if (config.memory_type == RandomAccess) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    } else if (config.memory_type == LinearAccess) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    } else if (config.memory_type == DeviceLocal) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_info.flags = 0;
    } else {
        ERROR("Invalid memory type");
    }

    VkBuffer vk_buffer = nullptr;
    VmaAllocation vma_allocation = nullptr;
    const auto buffer_result = vmaCreateBuffer(vk.gpu_allocator, &buffer_info, &alloc_info, &vk_buffer, &vma_allocation, nullptr);
    switch (buffer_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    GpuBuffer buffer{};
    buffer.m_allocation = vma_allocation;
    buffer.m_buffer = vk_buffer;
    buffer.m_type = config.memory_type;

    ASSERT(buffer.m_allocation != nullptr);
    ASSERT(buffer.m_buffer != nullptr);
    return buffer;
}

void GpuBuffer::write_void(Vk& vk, const void* data, const vk::DeviceSize size, const vk::DeviceSize offset) const {
    ASSERT(m_allocation != nullptr);
    ASSERT(m_buffer != nullptr);
    ASSERT(data != nullptr);
    ASSERT(size != 0);
    if (m_type == LinearAccess)
        ASSERT(offset == 0);

    if (m_type == RandomAccess || m_type == LinearAccess) {
        const auto copy_result = vmaCopyMemoryToAllocation(vk.gpu_allocator, data, m_allocation, offset, size);
        switch (copy_result) {
            case VK_SUCCESS: return;
            case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
            case VK_ERROR_MEMORY_MAP_FAILED: ERROR("Vulkan memory map failed");
            default: ERROR("Unexpected Vulkan error");
        }
    }
    ASSERT(m_type == DeviceLocal);

    const auto staging_buffer = create(vk, {size, vk::BufferUsageFlagBits::eTransferSrc, LinearAccess});
    defer(vmaDestroyBuffer(vk.gpu_allocator, staging_buffer.m_buffer, staging_buffer.m_allocation));
    const auto copy_result = vmaCopyMemoryToAllocation(vk.gpu_allocator, data, staging_buffer.m_allocation, 0, size);
    switch (copy_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_MEMORY_MAP_FAILED: ERROR("Vulkan memory map failed");
        default: ERROR("Unexpected Vulkan error");
    }

    submit_single_time_commands(vk, [&](const vk::CommandBuffer cmd) {
        cmd.copyBuffer(staging_buffer.m_buffer, m_buffer, {vk::BufferCopy(offset, 0, size)});
    });
}

GpuImage GpuImage::create(Vk& vk, const Config& config) {
    ASSERT(config.extent.width > 0);
    ASSERT(config.extent.height > 0);
    ASSERT(config.extent.depth > 0);
    ASSERT(config.format != vk::Format::eUndefined);
    ASSERT(config.usage != vk::ImageUsageFlags{});
    ASSERT(config.sample_count != vk::SampleCountFlagBits{});
    ASSERT(config.mip_levels > 0);

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = static_cast<VkFormat>(config.format);
    image_info.extent = config.extent;
    image_info.mipLevels = config.mip_levels;
    image_info.arrayLayers = 1;
    image_info.samples = static_cast<VkSampleCountFlagBits>(config.sample_count);
    image_info.usage = static_cast<VkImageUsageFlags>(config.usage);

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = 0;

    VkImage vk_image = VK_NULL_HANDLE;
    VmaAllocation vma_allocation = VK_NULL_HANDLE;
    const auto image_result = vmaCreateImage(vk.gpu_allocator, &image_info, &alloc_info, &vk_image, &vma_allocation, nullptr);
    switch (image_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: ERROR("Vulkan compression exhausted");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    GpuImage image{};
    image.m_image = vk_image;
    image.m_allocation = vma_allocation;

    ASSERT(image.m_image != nullptr);
    ASSERT(image.m_allocation != nullptr);
    return image;
}

GpuImage GpuImage::create_cubemap(Vk& vk, const CubemapConfig& config) {
    ASSERT(config.face_extent.width > 0);
    ASSERT(config.face_extent.height > 0);
    ASSERT(config.face_extent.depth > 0);
    ASSERT(config.format != vk::Format::eUndefined);
    ASSERT(config.usage != vk::ImageUsageFlags{});

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = static_cast<VkFormat>(config.format);
    image_info.extent = config.face_extent;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 6;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.usage = static_cast<VkImageUsageFlags>(config.usage);
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = 0;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    const auto image_result = vmaCreateImage(vk.gpu_allocator, &image_info, &alloc_info, &image, &allocation, nullptr);
    switch (image_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: ERROR("Vulkan compression exhausted");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    GpuImage cubemap{};
    cubemap.m_allocation = allocation;
    cubemap.m_image = image;

    ASSERT(cubemap.m_image != nullptr);
    ASSERT(cubemap.m_allocation != nullptr);
    return cubemap;
}

void GpuImage::write(Vk& vk, const WriteConfig& config) const {
    ASSERT(m_allocation != nullptr);
    ASSERT(m_image != nullptr);

    const auto& data = config.data;
    ASSERT(data.pixels != nullptr);
    ASSERT(data.alignment > 0);
    ASSERT(data.size.x > 0);
    ASSERT(data.size.y > 0);
    ASSERT(data.size.z == 1);

    const VkDeviceSize size = data.size.x * data.size.y * data.size.z * data.alignment;

    const auto staging_buffer = GpuBuffer::create(vk, {
        size, vk::BufferUsageFlagBits::eTransferSrc, GpuBuffer::MemoryType::LinearAccess
    });
    defer(staging_buffer.destroy(vk));
    staging_buffer.write_void(vk, data.pixels, size, 0);

    submit_single_time_commands(vk, [&](const vk::CommandBuffer cmd) {
        BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 1})
            .add_image_barrier(0, m_image, {config.aspect_flags, 0, vk::RemainingMipLevels, 0, 1})
            .set_image_dst(0, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run(vk);

        const vk::BufferImageCopy2 copy_region{
            .imageSubresource{config.aspect_flags, 0, 0, 1},
            .imageExtent = {data.size.x, data.size.y, data.size.z},
        };
        cmd.copyBufferToImage2({
            .srcBuffer = staging_buffer.get(),
            .dstImage = m_image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions = &copy_region
        });

        BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 1})
            .add_image_barrier(0, m_image, {config.aspect_flags, 0, vk::RemainingMipLevels, 0, 1})
            .set_image_src(0, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(0, vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, config.final_layout)
            .build_and_run(vk);
    });
}

void GpuImage::write_cubemap(Vk& vk, const WriteConfig& config) const {
    ASSERT(m_allocation != nullptr);
    ASSERT(m_image != nullptr);

    const auto& data = config.data;
    ASSERT(data.pixels != nullptr);
    ASSERT(data.alignment > 0);
    ASSERT(data.size.x > 0);
    ASSERT(data.size.y > 0);
    ASSERT(data.size.z == 1);

    const vk::Extent3D staging_extent{data.size.x, data.size.y, data.size.z};
    const vk::Extent3D face_extent{staging_extent.width / 4, staging_extent.height / 3, 1};

    const auto staging_image = GpuImage::create(vk, {
        .extent = staging_extent, 
        .format = config.format,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
    });
    defer(staging_image.destroy(vk));
    staging_image.write(vk, {
        .data = config.data,
        .format = config.format,
        .final_layout = vk::ImageLayout::eTransferSrcOptimal,
        .aspect_flags = config.aspect_flags,
    });

    submit_single_time_commands(vk, [&](const vk::CommandBuffer cmd) {
        BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 1})
            .add_image_barrier(0, m_image, {config.aspect_flags, 0, 1, 0, 6})
            .set_image_dst(0, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run(vk);

        std::array copies{
            vk::ImageCopy2{
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(config.data.size.x) * 2 / 4, to_i32(config.data.size.y) * 1 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 0, 1},
                .extent = face_extent,
            },
            vk::ImageCopy2{
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(config.data.size.x) * 0 / 4, to_i32(config.data.size.y) * 1 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 1, 1},
                .extent = face_extent,
            },
            vk::ImageCopy2{
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(config.data.size.x) * 1 / 4, to_i32(config.data.size.y) * 0 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 2, 1},
                .extent = face_extent,
            },
            vk::ImageCopy2{
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(config.data.size.x) * 1 / 4, to_i32(config.data.size.y) * 2 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 3, 1},
                .extent = face_extent,
            },
            vk::ImageCopy2{
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(config.data.size.x) * 1 / 4, to_i32(config.data.size.y) * 1 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 4, 1},
                .extent = face_extent,
            },
            vk::ImageCopy2{
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(config.data.size.x) * 3 / 4, to_i32(config.data.size.y) * 1 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 5, 1},
                .extent = face_extent,
            },
        };
        cmd.copyImage2({
            .srcImage = staging_image.get(),
            .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
            .dstImage = m_image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = to_u32(copies.size()),
            .pRegions = copies.data(),
        });

        BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 1})
            .add_image_barrier(0, m_image, {config.aspect_flags, 0, 1, 0, 6})
            .set_image_src(0, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(0, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal)
            .build_and_run(vk);
    });
}


GpuImageView GpuImageView::create(Vk& vk, const Config& config) {
    ASSERT(config.image != nullptr);
    ASSERT(config.format != vk::Format::eUndefined);

    const auto vk_view = vk.device.createImageView({
        .image = config.image,
        .viewType = vk::ImageViewType::e2D,
        .format = config.format,
        .subresourceRange = {config.aspect_flags, 0, vk::RemainingMipLevels, 0, 1},
    });
    switch (vk_view.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInvalidOpaqueCaptureAddress: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    GpuImageView view{};
    view.m_view = vk_view.value;
    return view;
}

GpuImageView GpuImageView::create_cubemap(Vk& vk, const Config& config) {
    ASSERT(config.image != nullptr);
    ASSERT(config.format != vk::Format::eUndefined);

    const auto vk_view = vk.device.createImageView({
        .image = config.image,
        .viewType = vk::ImageViewType::eCube,
        .format = config.format,
        .subresourceRange = {config.aspect_flags, 0, vk::RemainingMipLevels, 0, 6},
    });
    switch (vk_view.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInvalidOpaqueCaptureAddress: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    GpuImageView view{};
    view.m_view = vk_view.value;
    return view;
}

GpuImageAndView GpuImageAndView::create(Vk& vk, const Config& config) {
    ASSERT(config.extent.width > 0);
    ASSERT(config.extent.height > 0);
    ASSERT(config.extent.depth > 0);
    ASSERT(config.format != vk::Format::eUndefined);
    ASSERT(config.usage != vk::ImageUsageFlags{});
    ASSERT(config.aspect_flags != vk::ImageAspectFlagBits{});
    ASSERT(config.sample_count != vk::SampleCountFlagBits{});
    ASSERT(config.mip_levels > 0);

    GpuImageAndView image_and_view{};

    image_and_view.m_image = GpuImage::create(vk, {
        .extent = config.extent,
        .format = config.format,
        .usage = config.usage,
        .mip_levels = config.mip_levels,
        .sample_count = config.sample_count,
    });
    if (config.layout != vk::ImageLayout::eUndefined) {
        submit_single_time_commands(vk, [&](const vk::CommandBuffer cmd) {
            BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 1})
                .add_image_barrier(0, image_and_view.m_image.get(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                .set_image_dst(0, vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, config.layout)
                .build_and_run(vk);
        });
    }

    image_and_view.m_view = GpuImageView::create(vk, {
        .image = image_and_view.m_image.get(),
        .format = config.format,
        .aspect_flags = config.aspect_flags,
    });

    return image_and_view;
}

GpuImageAndView GpuImageAndView::create_cubemap(Vk& vk, const CubemapConfig& config) {
    ASSERT(config.data.pixels != nullptr);
    ASSERT(config.data.alignment > 0);
    ASSERT(config.data.size.x > 0);
    ASSERT(config.data.size.y > 0);
    ASSERT(config.data.size.z == 1);
    ASSERT(config.format != vk::Format::eUndefined);
    ASSERT(config.aspect_flags != vk::ImageAspectFlagBits{});

    GpuImageAndView cubemap{};

    const vk::Extent3D face_extent{config.data.size.x / 4, config.data.size.y / 3, 1};
    cubemap.m_image = GpuImage::create_cubemap(vk, {
        .face_extent = face_extent,
        .format = config.format,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
    });
    cubemap.m_image.write_cubemap(vk, {
        .data = config.data,
        .final_layout = vk::ImageLayout::eShaderReadOnlyOptimal
    });

    cubemap.m_view = GpuImageView::create_cubemap(vk, {
        .image = cubemap.m_image.get(),
        .format = config.format,
        .aspect_flags = config.aspect_flags,
    });

    return cubemap;
}

void GpuImageAndView::generate_mipmaps(
    Vk& vk,
    const u32 mip_levels,
    const vk::Extent3D extent,
    const vk::Format format,
    const vk::ImageLayout final_layout
) const {
    ASSERT(mip_levels > 1);
    ASSERT(extent.width > 0);
    ASSERT(extent.height > 0);
    ASSERT(extent.depth > 0);
    ASSERT(format != vk::Format::eUndefined);
    ASSERT(final_layout != vk::ImageLayout::eUndefined);

    const auto format_properties = vk.gpu.getFormatProperties(format);
    if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        ERROR("Format does not support optimal tiling with linear filtering");

    submit_single_time_commands(vk, [&](const vk::CommandBuffer cmd) {
        vk::Offset3D mip_offset{to_i32(extent.width), to_i32(extent.height), to_i32(extent.depth)};

        BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 1})
            .add_image_barrier(0, m_image.get(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_dst(0, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
            .build_and_run(vk);

        for (u32 level = 0; level < mip_levels - 1; ++level) {
            BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 1})
                .add_image_barrier(0, m_image.get(), {vk::ImageAspectFlagBits::eColor, level + 1, 1, 0, 1})
                .set_image_dst(0, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
                .build_and_run(vk);

            vk::ImageBlit2 region{
                .srcSubresource{vk::ImageAspectFlagBits::eColor, level, 0, 1},
                .dstSubresource{vk::ImageAspectFlagBits::eColor, level + 1, 0, 1},
            };
            region.srcOffsets[1] = mip_offset;
            if (mip_offset.x > 1)
                mip_offset.x /= 2;
            if (mip_offset.y > 1)
                mip_offset.y /= 2;
            if (mip_offset.z > 1)
                mip_offset.z /= 2;
            region.dstOffsets[1] = mip_offset;

            cmd.blitImage2({
                .srcImage = m_image.get(),
                .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
                .dstImage = m_image.get(),
                .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
                .regionCount = 1,
                .pRegions = &region,
                .filter = vk::Filter::eLinear,
            });

            BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 1})
                .add_image_barrier(0, m_image.get(), {vk::ImageAspectFlagBits::eColor, level + 1, 1, 0, 1})
                .set_image_src(0, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
                .set_image_dst(0, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
                .build_and_run(vk);
        }

        BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 1})
            .add_image_barrier(0, m_image.get(), {vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1})
            .set_image_src(0, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
            .set_image_dst(0, vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, final_layout)
            .build_and_run(vk);
    });
}

Sampler Sampler::create(Vk& vk, const Config& config) {
    ASSERT(config.mip_levels >= 1);
    const auto limits = vk.gpu.getProperties().limits;

    vk::SamplerCreateInfo sampler_info{
        .addressModeU = config.edge_mode,
        .addressModeV = config.edge_mode,
        .addressModeW = config.edge_mode,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = limits.maxSamplerAnisotropy,
        .maxLod = static_cast<f32>(config.mip_levels),
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
    };
    if (config.type == Linear) {
        sampler_info.magFilter = vk::Filter::eLinear;
        sampler_info.minFilter = vk::Filter::eLinear;
        sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
    } else if (config.type == Nearest) {
        sampler_info.magFilter = vk::Filter::eNearest;
        sampler_info.minFilter = vk::Filter::eNearest;
        sampler_info.mipmapMode = vk::SamplerMipmapMode::eNearest;
    } else {
        ERROR("Invalid sampler type");
    }
    const auto vk_sampler = vk.device.createSampler(sampler_info);
    switch (vk_sampler.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInvalidOpaqueCaptureAddress: ERROR("Vulkan invalid device address");
        default: ERROR("Unexpected Vulkan error");
    }

    Sampler sampler{};
    sampler.m_sampler = vk_sampler.value;

    ASSERT(sampler.m_sampler != nullptr);
    return sampler;
}

Texture Texture::create(Vk& vk, const ImageData& data, const Config& config) {
    ASSERT(data.pixels != nullptr);
    ASSERT(data.alignment > 0);
    ASSERT(data.size.x > 0);
    ASSERT(data.size.y > 0);
    ASSERT(data.size.z == 1);

    Texture texture{};

    vk::Extent3D extent{data.size.x, data.size.y, data.size.z};
    const u32 mip_levels = config.create_mips ? get_mip_count(extent) : 1;

    texture.m_image = GpuImageAndView::create(vk, {
        .extent = extent,
        .format = config.format,
        .usage = vk::ImageUsageFlagBits::eSampled
               | vk::ImageUsageFlagBits::eTransferSrc
               | vk::ImageUsageFlagBits::eTransferDst,
        .aspect_flags = config.aspect_flags,
        .mip_levels = mip_levels,
    });
    texture.m_image.write(vk, {
        .data = data,
        .format = config.format,
        .final_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .aspect_flags = config.aspect_flags,
    });
    if (mip_levels > 1)
        texture.m_image.generate_mipmaps(
            vk, mip_levels, extent, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eShaderReadOnlyOptimal
        );

    texture.m_sampler = Sampler::create(vk, {
        .type = config.sampler_type,
        .edge_mode = config.edge_mode,
        .mip_levels = mip_levels,
    });

    return texture;
}

Texture Texture::create_cubemap(Vk& vk, const ImageData& data, const Config& config) {
    ASSERT(config.create_mips == false);

    Texture texture{};
    texture.m_image = GpuImageAndView::create_cubemap(vk, {
        .data = data,
        .format = config.format,
        .aspect_flags = config.aspect_flags,
    });
    texture.m_sampler = Sampler::create(vk, {
        .type = config.sampler_type,
        .edge_mode = config.edge_mode,
        .mip_levels = 1,
    });
    return texture;
}

DescriptorSetLayout DescriptorSetLayout::create(Vk& vk, const Config& config) {
    ASSERT(config.bindings.count > 0);
    if (config.flags.count > 0)
        ASSERT(config.flags.count == config.bindings.count);

    const vk::DescriptorSetLayoutBindingFlagsCreateInfo flag_info{
        .bindingCount = to_u32(config.flags.count),
        .pBindingFlags = config.flags.data,
    };
    const auto vk_layout = vk.device.createDescriptorSetLayout({
        .pNext = config.flags.count == 0 ? nullptr : &flag_info,
        .bindingCount = to_u32(config.bindings.count),
        .pBindings = config.bindings.data,
    });
    switch (vk_layout.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    DescriptorSetLayout layout{};
    layout.m_descriptor_set_layout = vk_layout.value;

    ASSERT(layout.m_descriptor_set_layout != nullptr);
    return layout;
}

DescriptorPool DescriptorPool::create(Vk& vk, const Config& config) {
    ASSERT(config.max_sets >= 1);
    ASSERT(config.descriptors.count > 0);
    for (const auto& descriptor : config.descriptors) {
        ASSERT(descriptor.descriptorCount > 0);
    }

    const auto pool = vk.device.createDescriptorPool({
        .maxSets = config.max_sets, 
        .poolSizeCount = to_u32(config.descriptors.count), 
        .pPoolSizes = config.descriptors.data,
    });
    switch (pool.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorFragmentation: ERROR("Vulkan fragmentation");
        default: ERROR("Unexpected Vulkan error");
    }

    DescriptorPool descriptor_pool{};
    descriptor_pool.m_pool = pool.value;

    ASSERT(descriptor_pool.m_pool != nullptr);
    return descriptor_pool;
}

Result<void> DescriptorPool::allocate_sets(
    Vk& vk,
    const Slice<const vk::DescriptorSetLayout> layouts,
    const Slice<vk::DescriptorSet> out_sets
) {
    ASSERT(m_pool != nullptr);
    ASSERT(layouts.count > 0);
    for (const auto& layout : layouts) {
        ASSERT(layout != nullptr);
    }
    ASSERT(out_sets.count > 0);
    for (const auto& set : out_sets) {
        ASSERT(set == nullptr);
    }
    ASSERT(layouts.count == out_sets.count);

    const vk::DescriptorSetAllocateInfo alloc_info{
        .descriptorPool = m_pool,
        .descriptorSetCount = to_u32(layouts.count),
        .pSetLayouts = layouts.data,
    };
    const auto set_result = vk.device.allocateDescriptorSets(&alloc_info, out_sets.data);
    switch (set_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfPoolMemory: return Err::OutOfDescriptorSets;
        case vk::Result::eErrorFragmentation: return Err::OutOfDescriptorSets;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    for (const auto& set : out_sets) {
        ASSERT(set != nullptr);
    }
    return ok();
}

void write_uniform_buffer_descriptor(
    Vk& vk, const GpuBuffer::View& buffer,
    const vk::DescriptorSet set, const u32 binding, const u32 binding_array_index
) {
    ASSERT(set != nullptr);
    ASSERT(buffer.buffer != nullptr);
    ASSERT(buffer.range != 0);

    const vk::DescriptorBufferInfo buffer_info{buffer.buffer, buffer.offset, buffer.range};
    const vk::WriteDescriptorSet descriptor_write{
        .dstSet = set,
        .dstBinding = binding,
        .dstArrayElement = binding_array_index,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &buffer_info,
    };
    vk.device.updateDescriptorSets({descriptor_write}, {});
}

void write_image_sampler_descriptor(
    Vk& vk, const Texture& texture,
    const vk::DescriptorSet set, const u32 binding, const u32 binding_array_index
) {
    ASSERT(set != nullptr);

    const vk::DescriptorImageInfo image_info{
        .sampler = texture.get_sampler(),
        .imageView = texture.get_view(),
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};
    const vk::WriteDescriptorSet descriptor_write{
        .dstSet = set,
        .dstBinding = binding,
        .dstArrayElement = binding_array_index,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &image_info,
    };
    vk.device.updateDescriptorSets({descriptor_write}, {});
}

PipelineLayout PipelineLayout::create(Vk& vk, const Config& config) {
    vk::PipelineLayoutCreateInfo layout_info{
        .setLayoutCount = to_u32(config.set_layouts.count),
        .pSetLayouts = config.set_layouts.data,
        .pushConstantRangeCount = to_u32(config.push_ranges.count),
        .pPushConstantRanges = config.push_ranges.data,
    };
    const auto vk_layout= vk.device.createPipelineLayout(layout_info, nullptr);
    switch (vk_layout.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    PipelineLayout layout{};
    layout.m_pipeline_layout = vk_layout.value;

    ASSERT(layout.m_pipeline_layout != nullptr);
    return layout;
}

static Result<Slice<char>> read_shader(Vk& vk, const std::filesystem::path path) {
    ASSERT(!path.empty());

    auto file = std::ifstream{path, std::ios::ate | std::ios::binary};
    if (!file.is_open()) {
        LOGF_ERROR("Could not open shader file: {}", path.string());
        return Err::ShaderFileNotFound;
    }

    auto code = ok<Slice<char>>(vk.stack.alloc<char>(file.tellg()));
    file.seekg(0);
    file.read(code->data, static_cast<std::streamsize>(code->count));
    file.close();

    return code;
}

Result<UnlinkedShader> UnlinkedShader::create(Vk& vk, const Config& config) {
    ASSERT(!config.path.empty());
    ASSERT(config.code_type == vk::ShaderCodeTypeEXT::eSpirv && "binary shader code types untested");
    ASSERT(config.stage != vk::ShaderStageFlagBits{0});

    auto code = read_shader(vk, config.path);
    defer(vk.stack.dealloc(*code));
    if (code.has_err())
        return code.err();

    const auto vk_shader = vk.device.createShaderEXT({
        .stage = config.stage,
        .nextStage = config.next_stage,
        .codeType = config.code_type,
        .codeSize = code->count,
        .pCode = code->data,
        .pName = "main",
        .setLayoutCount = to_u32(config.set_layouts.count),
        .pSetLayouts = config.set_layouts.data,
        .pushConstantRangeCount = to_u32(config.push_ranges.count),
        .pPushConstantRanges = config.push_ranges.data,
    });
    switch (vk_shader.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eIncompatibleShaderBinaryEXT: {
            LOGF_ERROR("Could not load shader; shader invalid: {}", config.path.string());
            return Err::ShaderFileInvalid;
        }
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInitializationFailed: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }

    auto shader = ok<UnlinkedShader>();
    shader->m_shader = vk_shader.value;

    ASSERT(shader->m_shader != nullptr);
    return shader;
}

Result<GraphicsPipeline> GraphicsPipeline::create(Vk& vk, const Config& config) {
    ASSERT(!config.vertex_shader_path.empty());
    ASSERT(!config.fragment_shader_path.empty());
    ASSERT(config.code_type == vk::ShaderCodeTypeEXT::eSpirv && "binary shader code types untested");

    auto pipeline = ok<GraphicsPipeline>();

    pipeline->m_layout = PipelineLayout::create(vk, {
        .set_layouts = config.set_layouts,
        .push_ranges = config.push_ranges,
    });

    const auto vertex_code = read_shader(vk, config.vertex_shader_path);
    defer(vk.stack.dealloc(*vertex_code));
    if (vertex_code.has_err()) {
        LOGF_ERROR("Could not load vertex shader: {}", config.vertex_shader_path.string());
        return vertex_code.err();
    }
    const auto fragment_code = read_shader(vk, config.fragment_shader_path);
    defer(vk.stack.dealloc(*fragment_code));
    if (fragment_code.has_err()) {
        LOGF_ERROR("Could not load fragment shader: {}", config.fragment_shader_path.string());
        return fragment_code.err();
    }

    std::array shader_infos{
        vk::ShaderCreateInfoEXT{
            .flags = vk::ShaderCreateFlagBitsEXT::eLinkStage,
            .stage = vk::ShaderStageFlagBits::eVertex,
            .nextStage = vk::ShaderStageFlagBits::eFragment,
            .codeType = config.code_type,
            .codeSize = vertex_code->count,
            .pCode = vertex_code->data,
            .pName = "main",
            .setLayoutCount = to_u32(config.set_layouts.count),
            .pSetLayouts = config.set_layouts.data,
            .pushConstantRangeCount = to_u32(config.push_ranges.count),
            .pPushConstantRanges = config.push_ranges.data,
        },
        vk::ShaderCreateInfoEXT{
            .flags = vk::ShaderCreateFlagBitsEXT::eLinkStage,
            .stage = vk::ShaderStageFlagBits::eFragment,
            .nextStage{},
            .codeType = config.code_type,
            .codeSize = fragment_code->count,
            .pCode = fragment_code->data,
            .pName = "main",
            .setLayoutCount = to_u32(config.set_layouts.count),
            .pSetLayouts = config.set_layouts.data,
            .pushConstantRangeCount = to_u32(config.push_ranges.count),
            .pPushConstantRanges = config.push_ranges.data,
        },
    };

    const auto shader_result = vk.device.createShadersEXT(
        to_u32(shader_infos.size()), shader_infos.data(), nullptr, pipeline->m_shaders.data()
    );
    switch (shader_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eIncompatibleShaderBinaryEXT: {
            LOGF_ERROR(
                "Could not create shaders: {} and {}",
                config.vertex_shader_path.string(),
                config.fragment_shader_path.string()
            );
            return Err::ShaderFileInvalid;
        }
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInitializationFailed: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }


    return pipeline;
}

Fence Fence::create(Vk& vk, const Config& config) {
    const auto vk_fence = vk.device.createFence({.flags = config.flags});
    switch (vk_fence.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    Fence fence{};
    fence.m_fence = vk_fence.value;

    ASSERT(fence.m_fence != nullptr);
    return fence;
}

void Fence::wait(Vk& vk) const {
    ASSERT(m_fence != nullptr);

    const auto wait_result = vk.device.waitForFences({m_fence}, vk::True, 1'000'000'000);
    switch (wait_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eTimeout: {
            LOG_WARN("Vulkan timed out waiting for fence");
            break;
        }
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }
}

void Fence::reset(Vk& vk) const {
    ASSERT(m_fence != nullptr);

    const auto reset_result = vk.device.resetFences({m_fence});
    switch (reset_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
}

Semaphore Semaphore::create(Vk& vk) {
    const auto vk_semaphore = vk.device.createSemaphore({});
    switch (vk_semaphore.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    Semaphore semaphore{};
    semaphore.m_semaphore = vk_semaphore.value;

    ASSERT(semaphore.m_semaphore != nullptr);
    return semaphore;
}

void allocate_command_buffers(Vk& vk, const Slice<vk::CommandBuffer> out_cmds) {
    const vk::CommandBufferAllocateInfo alloc_info{
        .commandPool = vk.command_pool,
        .commandBufferCount = to_u32(out_cmds.count),
    };
    const auto cmd_result = vk.device.allocateCommandBuffers(&alloc_info, out_cmds.data);
    switch (cmd_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
}

vk::CommandBuffer begin_single_time_commands(Vk& vk) {
    vk::CommandBufferAllocateInfo alloc_info{
        .commandPool = vk.single_time_command_pool,
        .commandBufferCount = 1,
    };
    vk::CommandBuffer cmd{};
    const auto cmd_result = vk.device.allocateCommandBuffers(&alloc_info, &cmd);
    switch (cmd_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    const auto begin_result = cmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    switch (begin_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    ASSERT(cmd != nullptr);
    return cmd;
}

void end_single_time_commands(Vk& vk, vk::CommandBuffer cmd) {
    ASSERT(cmd != nullptr);

    const auto end_result = cmd.end();
    switch (end_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInvalidVideoStdParametersKHR: ERROR("Vulkan invalid video parameters");
        default: ERROR("Unexpected Vulkan error");
    }

    const auto fence = Fence::create(vk, {});
    defer(fence.destroy(vk));

    vk::SubmitInfo submit_info{
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    const auto submit_result = vk.queue.submit({submit_info}, fence.get());
    switch (submit_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    fence.wait(vk);

    vk.device.freeCommandBuffers(vk.single_time_command_pool, {cmd});
}

Surface Surface::create(Vk& vk, SDL_Window* window) {
    ASSERT(window != nullptr);

    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
    bool sdl_success = SDL_Vulkan_CreateSurface(window, vk.instance, nullptr, &vk_surface);
    if (!sdl_success)
        ERRORF("Could not create Vulkan surface: {}", SDL_GetError());

    Surface surface{};
    surface.m_surface = vk_surface;

    return surface;
}

Result<Swapchain> Swapchain::create(Vk& vk, const vk::SurfaceKHR surface) {
    auto swapchain = ok<Swapchain>();

    const auto create_result = swapchain->resize(vk, surface);
    if (create_result.has_err())
        return create_result.err();

    allocate_command_buffers(vk, swapchain->m_command_buffers);

    for (auto& fence : swapchain->m_frame_finished_fences) {
        fence = Fence::create(vk, {vk::FenceCreateFlagBits::eSignaled});
    }
    for (auto& semaphore : swapchain->m_image_available_semaphores) {
        semaphore = Semaphore::create(vk);
    }
    for (auto& semaphore : swapchain->m_ready_to_present_semaphores) {
        semaphore = Semaphore::create(vk);
    }

    ASSERT(swapchain->m_extent != nullptr);
    ASSERT(swapchain->m_swapchain != nullptr);
    for (usize i = 0; i < swapchain->m_image_count; ++i) {
        ASSERT(swapchain->m_swapchain_images[i] != nullptr);
    }
    for (const auto& cmd : swapchain->m_command_buffers) {
        ASSERT(cmd != nullptr);
    }
    return swapchain;
}

void Swapchain::destroy(Vk& vk) const {
    for (const auto& fence : m_frame_finished_fences) {
        fence.destroy(vk);
    }
    for (const auto& semaphore : m_image_available_semaphores) {
        semaphore.destroy(vk);
    }
    for (const auto& semaphore : m_ready_to_present_semaphores) {
        semaphore.destroy(vk);
    }

    for (const auto& cmd : m_command_buffers) {
        ASSERT(cmd != nullptr);
    }
    vk.device.freeCommandBuffers(vk.command_pool, to_u32(m_command_buffers.size()), m_command_buffers.data());

    ASSERT(m_swapchain != nullptr);
    vk.device.destroySwapchainKHR(m_swapchain);
}

Result<void> Swapchain::resize(Vk& vk, const vk::SurfaceKHR surface) {
    const auto [surface_result, surface_capabilities] = vk.gpu.getSurfaceCapabilitiesKHR(surface);
    switch (surface_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorSurfaceLostKHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }
    if (surface_capabilities.currentExtent.width == 0 || surface_capabilities.currentExtent.height == 0)
        return Err::InvalidWindow;
    if (surface_capabilities.currentExtent.width < surface_capabilities.minImageExtent.width)
        return Err::InvalidWindow;
    if (surface_capabilities.currentExtent.height < surface_capabilities.minImageExtent.height)
        return Err::InvalidWindow;
    if (surface_capabilities.currentExtent.width > surface_capabilities.maxImageExtent.width)
        return Err::InvalidWindow;
    if (surface_capabilities.currentExtent.height > surface_capabilities.maxImageExtent.height)
        return Err::InvalidWindow;

    const auto [present_mode_result, present_modes] = vk.gpu.getSurfacePresentModesKHR(surface);
    switch (surface_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eIncomplete: LOG_WARN("Vulkan get present modes incomplete"); break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorSurfaceLostKHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }

    const auto new_swapchain = vk.device.createSwapchainKHR({
        .surface = surface,
        .minImageCount = surface_capabilities.maxImageCount == 0
                         ? MaxImages
                         : std::min(surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount),
        .imageFormat = SwapchainImageFormat,
        .imageColorSpace = SwapchainColorSpace,
        .imageExtent = surface_capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        .preTransform = surface_capabilities.currentTransform,
        .presentMode = std::ranges::any_of(present_modes, [](const vk::PresentModeKHR mode) { return mode == vk::PresentModeKHR::eMailbox; })
            // ? vk::PresentModeKHR::eMailbox
            ? vk::PresentModeKHR::eFifo
            : vk::PresentModeKHR::eFifo,
        .clipped = vk::True,
        .oldSwapchain = m_swapchain,
    });
    switch (new_swapchain.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        case vk::Result::eErrorSurfaceLostKHR: ERROR("Vulkan surface lost");
        case vk::Result::eErrorNativeWindowInUseKHR: ERROR("Vulkan native window in use");
        case vk::Result::eErrorInitializationFailed: ERROR("Vulkan initialization failed");
        case vk::Result::eErrorCompressionExhaustedEXT: ERROR("Vulkan compression exhausted");
        default: ERROR("Unexpected Vulkan error");
    }

    const auto wait_result = vk.queue.waitIdle();
    switch (wait_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    if (m_swapchain != nullptr) {
        vk.device.destroySwapchainKHR(m_swapchain);
    }
    m_swapchain = new_swapchain.value;
    m_extent = surface_capabilities.currentExtent;

    const auto image_count_result = vk.device.getSwapchainImagesKHR(m_swapchain, &m_image_count, nullptr);
    if (image_count_result != vk::Result::eSuccess)
        ERROR("Could not get swapchain images");
    const auto image_result = vk.device.getSwapchainImagesKHR(m_swapchain, &m_image_count, m_swapchain_images.data());
    if (image_result != vk::Result::eSuccess)
        ERROR("Could not get swapchain images");

    ASSERT(m_swapchain != nullptr);
    for (const auto& image : m_swapchain_images) {
        ASSERT(image != nullptr);
    }
    return ok();
}

Result<Swapchain::DrawInfo> Swapchain::begin_frame(Vk& vk) {
    ASSERT(!m_recording);
    ASSERT(current_cmd() != nullptr);

    m_current_frame_index = (m_current_frame_index + 1) % MaxFramesInFlight;

    is_frame_finished().wait(vk);
    is_frame_finished().reset(vk);

    const auto acquire_result = vk.device.acquireNextImageKHR(
        m_swapchain, 1'000'000'000, is_image_available().get(), nullptr
    );
    switch (acquire_result.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eTimeout: return Err::FrameTimeout;
        case vk::Result::eNotReady: return Err::FrameTimeout;
        case vk::Result::eSuboptimalKHR: return Err::InvalidWindow;
        case vk::Result::eErrorOutOfDateKHR: return Err::InvalidWindow;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        case vk::Result::eErrorSurfaceLostKHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }
    m_current_image_index = acquire_result.value;

    auto cmd = current_cmd();

    const auto begin_result = cmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    switch (begin_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
    m_recording = true;

    const vk::Viewport viewport{
        0.0f, 0.0f,
        static_cast<f32>(m_extent.width), static_cast<f32>(m_extent.height),
        0.0f, 1.0f,
    };
    cmd.setViewportWithCount({viewport});
    const vk::Rect2D scissor{{0, 0}, m_extent};
    cmd.setScissorWithCount({scissor});

    cmd.setRasterizerDiscardEnable(vk::False);
    cmd.setPrimitiveRestartEnable(vk::False);
    cmd.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
    cmd.setPolygonModeEXT(vk::PolygonMode::eFill);
    cmd.setFrontFace(vk::FrontFace::eCounterClockwise);
    cmd.setCullMode(vk::CullModeFlagBits::eNone);
    cmd.setDepthTestEnable(vk::True);
    cmd.setDepthWriteEnable(vk::True);
    cmd.setDepthCompareOp(vk::CompareOp::eLess);
    cmd.setDepthBiasEnable(vk::False);
    cmd.setStencilTestEnable(vk::False);
    cmd.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1);
    cmd.setSampleMaskEXT(vk::SampleCountFlagBits::e1, vk::SampleMask{0xff});
    cmd.setAlphaToCoverageEnableEXT(vk::False);
    cmd.setColorWriteMaskEXT(0, {
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    });
    cmd.setColorBlendEnableEXT(0, {vk::False});

    return ok<DrawInfo>(cmd, current_image(), m_extent);
}

Result<void> Swapchain::end_frame(Vk& vk) {
    ASSERT(m_recording);
    ASSERT(m_swapchain != nullptr);
    ASSERT(current_cmd() != nullptr);

    const auto end_result = current_cmd().end();
    switch (end_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInvalidVideoStdParametersKHR: ERROR("Vulkan invalid video parameters");
        default: ERROR("Unexpected Vulkan error");
    }
    m_recording = false;

    constexpr vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = is_image_available().ptr(),
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &current_cmd(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = is_ready_to_present().ptr(),
    };

    const auto submit_result = vk.queue.submit({submit_info}, is_frame_finished().get());
    switch (submit_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    const vk::PresentInfoKHR present_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = is_ready_to_present().ptr(),
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain,
        .pImageIndices = &m_current_image_index,
    };
    const auto present_result = vk.queue.presentKHR(present_info);
    switch (present_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eSuboptimalKHR: [[fallthrough]];
        case vk::Result::eErrorOutOfDateKHR: {
            return Err::InvalidWindow;
        }
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        case vk::Result::eErrorSurfaceLostKHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }

    return ok();
}

} // namespace hg
