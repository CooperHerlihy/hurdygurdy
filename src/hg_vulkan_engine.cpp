#include "hg_vulkan_engine.h"

#include "hg_pch.h"
#include "hg_utils.h"
#include "hg_load.h"

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <iostream>
#include <span>
#include <vector>
#include <vulkan/vulkan_enums.hpp>

namespace hg {

#ifdef NDEBUG
inline constexpr std::array<const char*, 0> ValidationLayers = {};
#else
inline constexpr std::array ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
#endif
constexpr std::array DeviceExtensions = {
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
        INFO("{}", callback_data->pMessage);
    } else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        WARN("{}", callback_data->pMessage);
    } else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || true) {
        ERROR("{}", callback_data->pMessage);
    }
    return VK_FALSE;
}

const vk::DebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfo = {
    .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                     | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                     | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
    .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                 | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                 | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
    .pfnUserCallback = debug_callback,
};

static Result<std::vector<const char*>> get_instance_extensions() {
    CONTEXT("Getting required instance extensions");

    u32 glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    if (glfw_extensions == nullptr)
        ERROR("Failed to get required instance extensions from GLFW");

    auto required_extensions = ok<std::vector<const char*>>();
    required_extensions->reserve(glfw_extension_count + 1);
    for (usize i = 0; i < glfw_extension_count; ++i)
        required_extensions->emplace_back(glfw_extensions[i]);
#ifndef NDEBUG
    required_extensions->emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    const auto extensions = vk::enumerateInstanceExtensionProperties();
    switch (extensions.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eIncomplete: WARN("Vulkan incomplete enumeration"); break;
        case vk::Result::eErrorLayerNotPresent: return Err::VulkanLayerUnavailable;
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

static Result<vk::Instance> init_instance() {
    CONTEXT("Initializing Vulkan instance");

    const vk::ApplicationInfo app_info = {
        .pApplicationName = "Hurdy Gurdy",
        .applicationVersion = 0,
        .pEngineName = "Hurdy Gurdy",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };

    const auto extensions = get_instance_extensions();
    if (extensions.has_err())
        return extensions.err();

    const auto instance = vk::createInstance({
        .pNext = &DebugUtilsMessengerCreateInfo,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = to_u32(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = to_u32(extensions->size()),
        .ppEnabledExtensionNames = extensions->data(),
    });
    switch (instance.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorLayerNotPresent: return Err::VulkanLayerUnavailable;
        case vk::Result::eErrorExtensionNotPresent: return Err::VulkanExtensionUnavailable;
        case vk::Result::eErrorIncompatibleDriver: return Err::VulkanIncompatibleDriver;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInitializationFailed: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }

    ASSERT(instance.value != nullptr);
    return ok(instance.value);
}

static vk::DebugUtilsMessengerEXT init_debug_messenger(const Engine& engine) {
    CONTEXT("Initializing Vulkan debug messenger");
    ASSERT(engine.instance != nullptr);

    const auto messenger = engine.instance.createDebugUtilsMessengerEXT(DebugUtilsMessengerCreateInfo);
        switch (messenger.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        default: ERROR("Unexpected Vulkan error");
    }
    return messenger.value;
}

static Result<u32> find_queue_family(const vk::PhysicalDevice gpu) {
    CONTEXT("Finding queue family");
    ASSERT(gpu != nullptr);

    const auto queue_families = gpu.getQueueFamilyProperties();
    const auto queue_family = std::ranges::find_if(queue_families, [](const vk::QueueFamilyProperties family) {
        return static_cast<bool>(family.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute));
    });
    if (queue_family == queue_families.end())
        return Err::VkQueueFamilyUnavailable;

    return ok(static_cast<u32>(queue_family - queue_families.begin()));
}

static Result<vk::PhysicalDevice> find_gpu(const Engine& engine) {
    CONTEXT("Finding suitable GPU");
    ASSERT(engine.instance != nullptr);

    const auto gpus = engine.instance.enumeratePhysicalDevices();
    if (gpus.value.empty())
        return Err::NoCompatibleVkPhysicalDevice;
    switch (gpus.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eIncomplete: WARN("Vulkan incomplete"); break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInitializationFailed: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }

    for (const auto gpu : gpus.value) {
        const auto features = gpu.getFeatures();
        if (features.sampleRateShading != vk::True || features.samplerAnisotropy != vk::True)
            continue;

        const auto extensions = gpu.enumerateDeviceExtensionProperties();
            switch (extensions.result) {
            case vk::Result::eSuccess: break;
            case vk::Result::eIncomplete: WARN("Vulkan incomplete"); break;
            case vk::Result::eErrorLayerNotPresent: return Err::VulkanLayerUnavailable;
            case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
            case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
            default: ERROR("Unexpected Vulkan error");
        }

        if (!std::ranges::all_of(DeviceExtensions, [&](const char* required) {
            return std::ranges::any_of(extensions.value, [&](const vk::ExtensionProperties extension) {
                return strcmp(required, extension.extensionName);
            });
        })) continue;

        if (find_queue_family(gpu).has_err())
            continue;

        ASSERT(gpu != nullptr);
        return ok(gpu);
    }

    return Err::NoCompatibleVkPhysicalDevice;
}

static Result<vk::Device> init_device(const Engine& engine) {
    CONTEXT("Initializing Vulkan device");
    ASSERT(engine.gpu != nullptr);
    ASSERT(engine.queue_family_index != UINT32_MAX);

    vk::PhysicalDeviceBufferAddressFeaturesEXT buffer_address_feature = {.pNext = nullptr, .bufferDeviceAddress = vk::True};
    vk::PhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {
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
    vk::PhysicalDeviceShaderObjectFeaturesEXT shader_object_feature = {.pNext = &descriptor_indexing_features, .shaderObject = vk::True};
    vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature = {.pNext = &shader_object_feature, .dynamicRendering = vk::True};
    vk::PhysicalDeviceSynchronization2Features synchronization2_feature = {.pNext = &dynamic_rendering_feature, .synchronization2 = vk::True};
    constexpr vk::PhysicalDeviceFeatures features = {
        .sampleRateShading = vk::True,
        .samplerAnisotropy = vk::True,
    };

    constexpr float queue_priority = 1.0f;
    const vk::DeviceQueueCreateInfo queue_info{
        .queueFamilyIndex = engine.queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    const auto device = engine.gpu.createDevice({
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
        case vk::Result::eErrorExtensionNotPresent: return Err::VulkanExtensionUnavailable;
        case vk::Result::eErrorFeatureNotPresent: return Err::VulkanFeatureUnavailable;
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

static VmaAllocator init_allocator(const Engine& engine) {
    CONTEXT("Initializing Vulkan memory allocator");
    ASSERT(engine.instance != nullptr);
    ASSERT(engine.gpu != nullptr);
    ASSERT(engine.device != nullptr);

    VmaAllocatorCreateInfo info = {};
    info.physicalDevice = engine.gpu;
    info.device = engine.device;
    info.instance = engine.instance;
    info.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator allocator = nullptr;
    const auto result = vmaCreateAllocator(&info, &allocator);
    if (result != VK_SUCCESS)
        ERROR("Could not create Vma allocator");
    return allocator;
}

Result<Engine> Engine::create() {
    CONTEXT("Creating Vulkan engine");

    auto engine = ok<Engine>();

    const auto glfw_success = glfwInit();
    if (glfw_success == GLFW_FALSE)
        ERROR("Could not initialize GLFW");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    const auto instance = init_instance();
    if (instance.has_err())
        return instance.err();
    engine->instance = *instance;

    VULKAN_HPP_DEFAULT_DISPATCHER.init(engine->instance);

    engine->debug_messenger = init_debug_messenger(*engine);

    const auto gpu = find_gpu(*engine);
    if (gpu.has_err())
        return gpu.err();
    engine->gpu = *gpu;

    const auto queue_family = find_queue_family(engine->gpu);
    if (queue_family.has_err())
        return queue_family.err();
    engine->queue_family_index = *queue_family;

    const auto device = init_device(*engine);
    if (device.has_err())
        return device.err();
    engine->device = *device;

    VULKAN_HPP_DEFAULT_DISPATCHER.init(engine->device);

    engine->queue = engine->device.getQueue(engine->queue_family_index, 0);
    if (engine->queue == nullptr)
        return Err::VkQueueUnavailable;

    engine->gpu_allocator = init_allocator(*engine);

    engine->command_pool = create_command_pool(
        *engine, vk::CommandPoolCreateFlagBits::eResetCommandBuffer
    );
    engine->single_time_command_pool = create_command_pool(
        *engine, vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient
    );

    ASSERT(engine->instance != nullptr);
    ASSERT(engine->debug_messenger != nullptr);
    ASSERT(engine->gpu != nullptr);
    ASSERT(engine->device != nullptr);
    ASSERT(engine->gpu_allocator != nullptr);
    ASSERT(engine->queue_family_index != UINT32_MAX);
    ASSERT(engine->queue != nullptr);
    ASSERT(engine->command_pool != nullptr);
    ASSERT(engine->single_time_command_pool != nullptr);
    return engine;
}

void Engine::destroy() const {
    CONTEXT("Destroying Vulkan engine");
    ASSERT(instance != nullptr);
    ASSERT(debug_messenger != nullptr);
    ASSERT(device != nullptr);
    ASSERT(gpu_allocator != nullptr);
    ASSERT(command_pool != nullptr);
    ASSERT(single_time_command_pool != nullptr);

    device.destroyCommandPool(single_time_command_pool);
    device.destroyCommandPool(command_pool);
    vmaDestroyAllocator(gpu_allocator);
    device.destroy();
    instance.destroyDebugUtilsMessengerEXT(debug_messenger);
    instance.destroy();
}

GpuBuffer GpuBuffer::create(const Engine& engine, const Config& config) {
    CONTEXT("Creating Vulkan buffer");
    ASSERT(engine.gpu_allocator != nullptr);
    ASSERT(config.size != 0);
    ASSERT(config.usage != vk::BufferUsageFlags{});

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = config.size;
    buffer_info.usage = static_cast<VkBufferUsageFlags>(config.usage);

    VmaAllocationCreateInfo alloc_info = {};
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
    const auto buffer_result = vmaCreateBuffer(engine.gpu_allocator, &buffer_info, &alloc_info, &vk_buffer, &vma_allocation, nullptr);
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

void GpuBuffer::write_void(
    const Engine& engine, const void* data, const vk::DeviceSize size, const vk::DeviceSize offset
) const {
    CONTEXT("Writing to Vulkan buffer");
    ASSERT(engine.gpu_allocator != nullptr);
    ASSERT(m_allocation != nullptr);
    ASSERT(m_buffer != nullptr);
    ASSERT(data != nullptr);
    ASSERT(size != 0);
    if (m_type == LinearAccess)
        ASSERT(offset == 0);

    if (m_type == RandomAccess || m_type == LinearAccess) {
        const auto copy_result = vmaCopyMemoryToAllocation(engine.gpu_allocator, data, m_allocation, offset, size);
        switch (copy_result) {
            case VK_SUCCESS: return;
            case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
            case VK_ERROR_MEMORY_MAP_FAILED: ERROR("Vulkan memory map failed");
            default: ERROR("Unexpected Vulkan error");
        }
    }
    ASSERT(m_type == DeviceLocal);

    const auto staging_buffer = create(engine, {size, vk::BufferUsageFlagBits::eTransferSrc, LinearAccess});
    defer(vmaDestroyBuffer(engine.gpu_allocator, staging_buffer.m_buffer, staging_buffer.m_allocation));
    const auto copy_result = vmaCopyMemoryToAllocation(engine.gpu_allocator, data, staging_buffer.m_allocation, 0, size);
    switch (copy_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_MEMORY_MAP_FAILED: ERROR("Vulkan memory map failed");
        default: ERROR("Unexpected Vulkan error");
    }

    submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        cmd.copyBuffer(staging_buffer.m_buffer, m_buffer, {vk::BufferCopy(offset, 0, size)});
    });
}

GpuImage GpuImage::create(const Engine& engine, const Config& config) {
    CONTEXT("Creating Vulkan image");
    ASSERT(engine.device != nullptr);
    ASSERT(engine.gpu_allocator != nullptr);
    ASSERT(config.extent.width > 0);
    ASSERT(config.extent.height > 0);
    ASSERT(config.extent.depth > 0);
    ASSERT(config.format != vk::Format::eUndefined);
    ASSERT(config.usage != vk::ImageUsageFlags{});
    ASSERT(config.sample_count != vk::SampleCountFlagBits{});
    ASSERT(config.mip_levels > 0);

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = static_cast<VkImageType>(config.dimensions);
    image_info.format = static_cast<VkFormat>(config.format);
    image_info.extent = config.extent;
    image_info.mipLevels = config.mip_levels;
    image_info.arrayLayers = 1;
    image_info.samples = static_cast<VkSampleCountFlagBits>(config.sample_count);
    image_info.usage = static_cast<VkImageUsageFlags>(config.usage);

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = 0;

    VkImage vk_image = VK_NULL_HANDLE;
    VmaAllocation vma_allocation = VK_NULL_HANDLE;
    const auto image_result = vmaCreateImage(engine.gpu_allocator, &image_info, &alloc_info, &vk_image, &vma_allocation, nullptr);
    switch (image_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: ERROR("Vulkan compression exhausted");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    GpuImage image = {};
    image.m_image = vk_image;
    image.m_allocation = vma_allocation;

    ASSERT(image.m_image != nullptr);
    ASSERT(image.m_allocation != nullptr);
    return image;
}

GpuImage GpuImage::create_cubemap(const Engine& engine, const CubemapConfig& config) {
    CONTEXT("Creating Vulkan cubemap image");
    ASSERT(engine.device != nullptr);
    ASSERT(engine.gpu_allocator != nullptr);
    ASSERT(config.face_extent.width > 0);
    ASSERT(config.face_extent.height > 0);
    ASSERT(config.face_extent.depth > 0);
    ASSERT(config.format != vk::Format::eUndefined);
    ASSERT(config.usage != vk::ImageUsageFlags{});

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = static_cast<VkFormat>(config.format);
    image_info.extent = config.face_extent;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 6;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.usage = static_cast<VkImageUsageFlags>(config.usage);
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = 0;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    const auto image_result = vmaCreateImage(engine.gpu_allocator, &image_info, &alloc_info, &image, &allocation, nullptr);
    switch (image_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: ERROR("Vulkan compression exhausted");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    GpuImage cubemap = {};
    cubemap.m_allocation = allocation;
    cubemap.m_image = image;

    ASSERT(cubemap.m_image != nullptr);
    ASSERT(cubemap.m_allocation != nullptr);
    return cubemap;
}

void GpuImage::write(const Engine& engine, const WriteConfig& config) const {
    CONTEXT("Writing to Vulkan image");
    ASSERT(engine.gpu_allocator != nullptr);
    ASSERT(m_allocation != nullptr);
    ASSERT(m_image != nullptr);

    const auto& data = config.data;
    ASSERT(data.ptr != nullptr);
    ASSERT(data.alignment > 0);
    ASSERT(data.extent.width > 0);
    ASSERT(data.extent.height > 0);
    ASSERT(data.extent.depth > 0);

    const VkDeviceSize size = data.extent.width * data.extent.height * data.extent.depth * data.alignment;

    const auto staging_buffer = GpuBuffer::create(engine, {
        size, vk::BufferUsageFlagBits::eTransferSrc, GpuBuffer::MemoryType::LinearAccess
    });
    defer(staging_buffer.destroy(engine));
    staging_buffer.write_void(engine, data.ptr, size, 0);

    submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        BarrierBuilder(cmd)
            .add_image_barrier(m_image, config.subresource)
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run();

        const vk::BufferImageCopy2 copy_region = {
            .imageSubresource = {config.subresource.aspectMask, 0, 0, 1},
            .imageExtent = data.extent
        };
        cmd.copyBufferToImage2({
            .srcBuffer = staging_buffer.get(),
            .dstImage = m_image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions = &copy_region
        });

        BarrierBuilder(cmd)
            .add_image_barrier(m_image, config.subresource)
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, config.final_layout)
            .build_and_run();
    });
}

GpuImageView GpuImageView::create(const Engine& engine, const Config& config) {
    CONTEXT("Creating Vulkan image view");
    ASSERT(engine.device != nullptr);
    ASSERT(engine.gpu_allocator != nullptr);
    ASSERT(config.image != nullptr);
    ASSERT(config.format != vk::Format::eUndefined);

    const auto vk_view = engine.device.createImageView({
        .image = config.image,
        .viewType = config.dimensions,
        .format = config.format,
        .subresourceRange = config.subresource,
    });
    switch (vk_view.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInvalidOpaqueCaptureAddress: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    GpuImageView view = {};
    view.m_view = vk_view.value;
    return view;
}

GpuImageAndView GpuImageAndView::create(const Engine& engine, const Config& config) {
    CONTEXT("Creating Vulkan image and view");
    ASSERT(engine.device != nullptr);
    ASSERT(engine.gpu_allocator != nullptr);
    ASSERT(config.extent.width > 0);
    ASSERT(config.extent.height > 0);
    ASSERT(config.extent.depth > 0);
    ASSERT(config.format != vk::Format::eUndefined);
    ASSERT(config.usage != vk::ImageUsageFlags{});
    ASSERT(config.aspect_flags != vk::ImageAspectFlagBits{});
    ASSERT(config.sample_count != vk::SampleCountFlagBits{});
    ASSERT(config.mip_levels > 0);

    vk::ImageType image_dimensions = vk::ImageType::e3D;
    if (config.extent.depth == 1) {
        image_dimensions = static_cast<vk::ImageType>(static_cast<int>(image_dimensions) - 1);
        if (config.extent.height == 1)
            image_dimensions = static_cast<vk::ImageType>(static_cast<int>(image_dimensions) - 1);
    }
    const auto image = GpuImage::create(engine, {
        .extent = config.extent,
        .dimensions = image_dimensions,
        .format = config.format,
        .usage = config.usage,
        .sample_count = config.sample_count,
        .mip_levels = config.mip_levels,
    });

    if (config.layout != vk::ImageLayout::eUndefined) {
        submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
            BarrierBuilder(cmd)
                .add_image_barrier(image.get(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, config.layout)
                .build_and_run();
        });
    }

    vk::ImageViewType view_dimensions = vk::ImageViewType::e3D;
    if (config.extent.depth == 1) {
        view_dimensions = static_cast<vk::ImageViewType>(static_cast<int>(view_dimensions) - 1);
        if (config.extent.height == 1)
            view_dimensions = static_cast<vk::ImageViewType>(static_cast<int>(view_dimensions) - 1);
    }
    const auto view = GpuImageView::create(engine, {
        .image = image.get(),
        .dimensions = view_dimensions,
        .format = config.format,
        .subresource = {.aspectMask = config.aspect_flags, .levelCount = config.mip_levels, .layerCount = 1},
    });

    GpuImageAndView image_and_view = {};
    image_and_view.m_image = image;
    image_and_view.m_view = view;
    return image_and_view;
}

GpuImageAndView GpuImageAndView::create_cubemap(const Engine& engine, const CubemapConfig& config) {
    CONTEXT("Creating Vulkan cubemap image and view");
    ASSERT(engine.device != nullptr);
    ASSERT(engine.gpu_allocator != nullptr);
    ASSERT(config.data.pixels != nullptr);
    ASSERT(config.data.width > 0);
    ASSERT(config.data.height > 0);
    ASSERT(config.data.channels > 0);
    ASSERT(config.format != vk::Format::eUndefined);
    ASSERT(config.aspect_flags != vk::ImageAspectFlagBits{});

    const vk::Extent3D staging_extent = {to_u32(config.data.width), to_u32(config.data.height), 1};

    const auto staging_image = GpuImage::create(engine, {
        .extent = staging_extent, 
        .dimensions = vk::ImageType::e2D,
        .format = config.format,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
    });
    defer(staging_image.destroy(engine));
    staging_image.write(engine, {
        {config.data.pixels.get(), 4, staging_extent},
        vk::ImageLayout::eTransferSrcOptimal
    });

    const vk::Extent3D face_extent = {staging_extent.width / 4, staging_extent.height / 3, 1};

    const auto image = GpuImage::create_cubemap(engine, {
        .face_extent = face_extent,
        .format = config.format,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
    });

    const auto view = GpuImageView::create(engine, {
        .image = image.get(),
        .dimensions = vk::ImageViewType::eCube,
        .format = config.format,
        .subresource = {.aspectMask = config.aspect_flags, .levelCount = 1, .layerCount = 6},
    });

    submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        BarrierBuilder(cmd)
            .add_image_barrier(image.get(), {config.aspect_flags, 0, 1, 0, 6})
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run();

        std::array copies = {
            vk::ImageCopy2{
                .srcSubresource = {config.aspect_flags, 0, 0, 1},
                .srcOffset = {config.data.width * 2 / 4, config.data.height * 1 / 3, 0},
                .dstSubresource = {config.aspect_flags, 0, 0, 1},
                .extent = face_extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {config.aspect_flags, 0, 0, 1},
                .srcOffset = {config.data.width * 0 / 4, config.data.height * 1 / 3, 0},
                .dstSubresource = {config.aspect_flags, 0, 1, 1},
                .extent = face_extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {config.aspect_flags, 0, 0, 1},
                .srcOffset = {config.data.width * 1 / 4, config.data.height * 0 / 3, 0},
                .dstSubresource = {config.aspect_flags, 0, 2, 1},
                .extent = face_extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {config.aspect_flags, 0, 0, 1},
                .srcOffset = {config.data.width * 1 / 4, config.data.height * 2 / 3, 0},
                .dstSubresource = {config.aspect_flags, 0, 3, 1},
                .extent = face_extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {config.aspect_flags, 0, 0, 1},
                .srcOffset = {config.data.width * 1 / 4, config.data.height * 1 / 3, 0},
                .dstSubresource = {config.aspect_flags, 0, 4, 1},
                .extent = face_extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {config.aspect_flags, 0, 0, 1},
                .srcOffset = {config.data.width * 3 / 4, config.data.height * 1 / 3, 0},
                .dstSubresource = {config.aspect_flags, 0, 5, 1},
                .extent = face_extent,
            },
        };
        cmd.copyImage2({
            .srcImage = staging_image.get(),
            .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
            .dstImage = image.get(),
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = to_u32(copies.size()),
            .pRegions = copies.data(),
        });

        BarrierBuilder(cmd)
            .add_image_barrier(image.get(), {config.aspect_flags, 0, 1, 0, 6})
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal)
            .build_and_run();
    });

    GpuImageAndView cubemap = {};
    cubemap.m_image = image;
    cubemap.m_view = view;
    return cubemap;
}

void GpuImageAndView::generate_mipmaps(
    const Engine& engine,
    const u32 mip_levels,
    const vk::Extent3D extent,
    const vk::Format format,
    const vk::ImageLayout final_layout
) const {
    CONTEXT("Generating mipmaps");
    ASSERT(engine.gpu != nullptr);
    ASSERT(mip_levels > 1);
    ASSERT(extent.width > 0);
    ASSERT(extent.height > 0);
    ASSERT(extent.depth > 0);
    ASSERT(format != vk::Format::eUndefined);
    ASSERT(final_layout != vk::ImageLayout::eUndefined);

    const auto format_properties = engine.gpu.getFormatProperties(format);
    if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        ERROR("Format does not support optimal tiling with linear filtering");

    submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        vk::Offset3D mip_offset = {to_i32(extent.width), to_i32(extent.height), to_i32(extent.depth)};

        BarrierBuilder(cmd)
            .add_image_barrier(m_image.get(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
            .build_and_run();

        for (u32 level = 0; level < mip_levels - 1; ++level) {
            BarrierBuilder(cmd)
                .add_image_barrier(m_image.get(), {vk::ImageAspectFlagBits::eColor, level + 1, 1, 0, 1})
                .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
                .build_and_run();

            vk::ImageBlit2 region = {
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, level, 0, 1},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, level + 1, 0, 1},
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

            BarrierBuilder(cmd)
                .add_image_barrier(m_image.get(), {vk::ImageAspectFlagBits::eColor, level + 1, 1, 0, 1})
                .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
                .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
                .build_and_run();
        }

        BarrierBuilder(cmd)
            .add_image_barrier(m_image.get(), {vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1})
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, final_layout)
            .build_and_run();
    });
}

Sampler Sampler::create(const Engine& engine, const Config& config) {
    CONTEXT("Creating Vulkan sampler");
    ASSERT(engine.device != nullptr);
    ASSERT(engine.gpu != nullptr);
    ASSERT(config.mip_levels >= 1);
    const auto limits = engine.gpu.getProperties().limits;

    vk::SamplerCreateInfo sampler_info = {
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
    const auto vk_sampler = engine.device.createSampler(sampler_info);
    switch (vk_sampler.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInvalidOpaqueCaptureAddress: ERROR("Vulkan invalid device address");
        default: ERROR("Unexpected Vulkan error");
    }

    Sampler sampler = {};
    sampler.m_sampler = vk_sampler.value;

    ASSERT(sampler.m_sampler != nullptr);
    return sampler;
}

Texture Texture::from_data(const Engine& engine, const GpuImage::Data& data, const Config& config) {
    CONTEXT("Creating Vulkan texture from data");

    const u32 mip_levels = config.create_mips ? get_mip_count(data.extent) : 1;

    const auto image = GpuImageAndView::create(engine, {
        .extent = data.extent,
        .format = config.format,
        .usage = vk::ImageUsageFlagBits::eSampled
               | vk::ImageUsageFlagBits::eTransferSrc
               | vk::ImageUsageFlagBits::eTransferDst,
        .aspect_flags = config.aspect_flags,
        .mip_levels = mip_levels,
    });
    image.write(engine, {
        .data = data,
        .final_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .subresource = {config.aspect_flags, 0, vk::RemainingMipLevels, 0, 1},
    });

    if (mip_levels > 1) {
        image.generate_mipmaps(
            engine,
            mip_levels,
            data.extent,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );
    }

    const auto sampler = Sampler::create(engine, {
        .type = config.sampler_type,
        .edge_mode = config.edge_mode,
        .mip_levels = mip_levels,
    });

    Texture texture = {};
    texture.m_image = image;
    texture.m_sampler = sampler;
    return texture;
}

Result<Texture> Texture::from_file(const Engine& engine, const std::filesystem::path path, const Config& config) {
    CONTEXT("Creating Vulkan texture from file");
    ASSERT(!path.empty());

    const auto data = ImageData::load(path);
    if (data.has_err())
        return data.err();

    return ok(from_data(engine, {
        .ptr = data->pixels.get(),
        .alignment = 4,
        .extent = {to_u32(data->width), to_u32(data->height), 1}
    }, config));
}

Result<Texture> Texture::from_cubemap_file(const Engine& engine, const std::filesystem::path path, const Config& config) {
    CONTEXT("Creating Vulkan cubemap texture from file");
    ASSERT(!path.empty());
    ASSERT(config.create_mips == false);

    auto data = ImageData::load(path);
    if (data.has_err())
        return data.err();

    const auto cubemap = GpuImageAndView::create_cubemap(engine, {
        .data = std::move(*data),
        .format = config.format,
        .aspect_flags = config.aspect_flags,
    });
    const auto sampler = Sampler::create(engine, {
        .type = config.sampler_type,
        .edge_mode = config.edge_mode,
        .mip_levels = 1,
    });

    auto texture = ok<Texture>();
    texture->m_image = cubemap;
    texture->m_sampler = sampler;
    return texture;
}

vk::DescriptorSetLayout create_descriptor_set_layout(
    const Engine& engine,
    const std::span<const vk::DescriptorSetLayoutBinding> bindings,
    const std::span<const vk::DescriptorBindingFlags> flags
) {
    CONTEXT("Creating Vulkan descriptor set layout");
    ASSERT(engine.device != nullptr);
    ASSERT(!bindings.empty());
    if (!flags.empty())
        ASSERT(flags.size() == bindings.size());

    const vk::DescriptorSetLayoutBindingFlagsCreateInfo flag_info = {
        .bindingCount = to_u32(flags.size()),
        .pBindingFlags = flags.data(),
    };
    const auto layout = engine.device.createDescriptorSetLayout({
        .pNext = flags.empty() ? nullptr : &flag_info,
        .bindingCount = to_u32(bindings.size()),
        .pBindings = bindings.data(),
    });
    switch (layout.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    ASSERT(layout.value != nullptr);
    return layout.value;
}

vk::DescriptorPool create_descriptor_pool(
    const Engine& engine, const u32 max_sets,
    const std::span<const vk::DescriptorPoolSize> descriptors
) {
    CONTEXT("Creating Vulkan descriptor pool");
    ASSERT(engine.device != nullptr);
    ASSERT(max_sets >= 1);
    ASSERT(!descriptors.empty());
    for (const auto& descriptor : descriptors) {
        ASSERT(descriptor.descriptorCount > 0);
    }

    const auto pool = engine.device.createDescriptorPool({
        .maxSets = max_sets, 
        .poolSizeCount = to_u32(descriptors.size()), 
        .pPoolSizes = descriptors.data(),
    });
    switch (pool.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorFragmentation: ERROR("Vulkan fragmentation");
        default: ERROR("Unexpected Vulkan error");
    }

    ASSERT(pool.value != nullptr);
    return pool.value;
}

Result<void> allocate_descriptor_sets(
    const Engine& engine, const vk::DescriptorPool pool,
    const std::span<const vk::DescriptorSetLayout> layouts,
    const std::span<vk::DescriptorSet> out_sets
) {
    CONTEXT("Allocating Vulkan descriptor sets");
    ASSERT(engine.device != nullptr);
    ASSERT(pool != nullptr);
    ASSERT(!layouts.empty());
    for (const auto layout : layouts) {
        ASSERT(layout != nullptr);
    }
    ASSERT(!out_sets.empty());
    for (const auto set : out_sets) {
        ASSERT(set == nullptr);
    }
    ASSERT(layouts.size() == out_sets.size());

    const vk::DescriptorSetAllocateInfo alloc_info = {
        .descriptorPool = pool,
        .descriptorSetCount = to_u32(layouts.size()),
        .pSetLayouts = layouts.data(),
    };
    const auto set_result = engine.device.allocateDescriptorSets(&alloc_info, out_sets.data());
    switch (set_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfPoolMemory: return Err::CouldNotAllocateVkDescriptorSets;
        case vk::Result::eErrorFragmentation: return Err::CouldNotAllocateVkDescriptorSets;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    for (const auto set : out_sets) {
        ASSERT(set != nullptr);
    }
    return ok();
}

void write_uniform_buffer_descriptor(
    const Engine& engine, const GpuBuffer::View& buffer,
    const vk::DescriptorSet set, const u32 binding, const u32 binding_array_index
) {
    CONTEXT("Writing Vulkan uniform buffer descriptor");
    ASSERT(engine.device != nullptr);
    ASSERT(set != nullptr);
    ASSERT(buffer.buffer != nullptr);
    ASSERT(buffer.range != 0);

    const vk::DescriptorBufferInfo buffer_info = {buffer.buffer, buffer.offset, buffer.range};
    const vk::WriteDescriptorSet descriptor_write = {
        .dstSet = set,
        .dstBinding = binding,
        .dstArrayElement = binding_array_index,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &buffer_info,
    };
    engine.device.updateDescriptorSets({descriptor_write}, {});
}

void write_image_sampler_descriptor(
    const Engine& engine, const Texture& texture,
    const vk::DescriptorSet set, const u32 binding, const u32 binding_array_index
) {
    CONTEXT("Writing Vulkan combined image and sampler descriptor");
    ASSERT(engine.device != nullptr);
    ASSERT(set != nullptr);

    const vk::DescriptorImageInfo image_info = {
        .sampler = texture.get_sampler(),
        .imageView = texture.get_view(),
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};
    const vk::WriteDescriptorSet descriptor_write = {
        .dstSet = set,
        .dstBinding = binding,
        .dstArrayElement = binding_array_index,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &image_info,
    };
    engine.device.updateDescriptorSets({descriptor_write}, {});
}

PipelineLayout PipelineLayout::create(const Engine& engine, const Config& config) {
    CONTEXT("Creating Vulkan pipeline layout");
    ASSERT(engine.device != nullptr);

    vk::PipelineLayoutCreateInfo layout_info = {
        .setLayoutCount = to_u32(config.set_layouts.size()),
        .pSetLayouts = config.set_layouts.data(),
        .pushConstantRangeCount = to_u32(config.push_ranges.size()),
        .pPushConstantRanges = config.push_ranges.data(),
    };
    const auto vk_layout= engine.device.createPipelineLayout(layout_info, nullptr);
    switch (vk_layout.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    PipelineLayout layout = {};
    layout.m_pipeline_layout = vk_layout.value;

    ASSERT(layout.m_pipeline_layout != nullptr);
    return layout;
}

static Result<std::vector<char>> read_shader(const std::filesystem::path path) {
    CONTEXT("Reading Vulkan shader code");
    ASSERT(!path.empty());

    auto file = std::ifstream{path, std::ios::ate | std::ios::binary};
    if (!file.is_open())
        return Err::ShaderFileNotFound;

    const usize code_size = file.tellg();
    auto code = ok<std::vector<char>>();
    code->resize(code_size);
    file.seekg(0);
    file.read(code->data(), static_cast<std::streamsize>(code->size()));
    file.close();

    if (code->empty())
        return Err::ShaderFileInvalid;

    ASSERT(!code->empty());
    return code;
}

Result<UnlinkedShader> UnlinkedShader::create(const Engine& engine, const Config& config) {
    CONTEXT("Creating Vulkan unlinked shader");
    ASSERT(engine.device != nullptr);
    ASSERT(!config.path.empty());
    ASSERT(config.code_type == vk::ShaderCodeTypeEXT::eSpirv && "binary shader code types untested");
    ASSERT(config.stage != vk::ShaderStageFlagBits{0});

    const auto code = read_shader(config.path);
    if (code.has_err())
        return code.err();

    const auto vk_shader = engine.device.createShaderEXT({
        .stage = config.stage,
        .nextStage = config.next_stage,
        .codeType = config.code_type,
        .codeSize = code->size(),
        .pCode = code->data(),
        .pName = "main",
        .setLayoutCount = to_u32(config.set_layouts.size()),
        .pSetLayouts = config.set_layouts.data(),
        .pushConstantRangeCount = to_u32(config.push_ranges.size()),
        .pPushConstantRanges = config.push_ranges.data(),
    });
    switch (vk_shader.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eIncompatibleShaderBinaryEXT: return Err::ShaderFileInvalid;
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

Result<GraphicsPipeline> GraphicsPipeline::create(const Engine& engine, const Config& config) {
    CONTEXT("Creating Vulkan graphics pipeline");
    ASSERT(engine.device != nullptr);
    ASSERT(!config.vertex_shader_path.empty());
    ASSERT(!config.fragment_shader_path.empty());
    ASSERT(config.code_type == vk::ShaderCodeTypeEXT::eSpirv && "binary shader code types untested");

    const auto layout = PipelineLayout::create(engine, {
        .set_layouts = config.set_layouts,
        .push_ranges = config.push_ranges,
    });

    const auto vertex_code = read_shader(config.vertex_shader_path);
    if (vertex_code.has_err())
        return vertex_code.err();
    const auto fragment_code = read_shader(config.fragment_shader_path);
    if (fragment_code.has_err())
        return fragment_code.err();

    std::array shader_infos = {
        vk::ShaderCreateInfoEXT{
            .flags = vk::ShaderCreateFlagBitsEXT::eLinkStage,
            .stage = vk::ShaderStageFlagBits::eVertex,
            .nextStage = vk::ShaderStageFlagBits::eFragment,
            .codeType = config.code_type,
            .codeSize = vertex_code->size(),
            .pCode = vertex_code->data(),
            .pName = "main",
            .setLayoutCount = to_u32(config.set_layouts.size()),
            .pSetLayouts = config.set_layouts.data(),
            .pushConstantRangeCount = to_u32(config.push_ranges.size()),
            .pPushConstantRanges = config.push_ranges.data(),
        },
        vk::ShaderCreateInfoEXT{
            .flags = vk::ShaderCreateFlagBitsEXT::eLinkStage,
            .stage = vk::ShaderStageFlagBits::eFragment,
            .nextStage = {},
            .codeType = config.code_type,
            .codeSize = fragment_code->size(),
            .pCode = fragment_code->data(),
            .pName = "main",
            .setLayoutCount = to_u32(config.set_layouts.size()),
            .pSetLayouts = config.set_layouts.data(),
            .pushConstantRangeCount = to_u32(config.push_ranges.size()),
            .pPushConstantRanges = config.push_ranges.data(),
        },
    };

    std::array<vk::ShaderEXT, 2> shaders = {};
    const auto shader_result = engine.device.createShadersEXT(to_u32(shader_infos.size()), shader_infos.data(), nullptr, shaders.data());
    switch (shader_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eIncompatibleShaderBinaryEXT: return Err::ShaderFileInvalid;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInitializationFailed: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }

    auto pipeline = ok<GraphicsPipeline>();
    pipeline->m_layout = layout;
    pipeline->m_shaders = shaders;

    return pipeline;
}

Fence Fence::create(const Engine& engine, const Config& config) {
    CONTEXT("Creating Vulkan fence");
    ASSERT(engine.device != nullptr);

    const auto vk_fence = engine.device.createFence({.flags = config.flags});
    switch (vk_fence.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    Fence fence = {};
    fence.m_fence = vk_fence.value;

    ASSERT(fence.m_fence != nullptr);
    return fence;
}

Semaphore Semaphore::create(const Engine& engine) {
    CONTEXT("Creating Vulkan semaphore");
    ASSERT(engine.device != nullptr);

    const auto vk_semaphore = engine.device.createSemaphore({});
    switch (vk_semaphore.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    Semaphore semaphore = {};
    semaphore.m_semaphore = vk_semaphore.value;

    ASSERT(semaphore.m_semaphore != nullptr);
    return semaphore;
}

vk::CommandPool create_command_pool(const Engine& engine, const vk::CommandPoolCreateFlags flags) {
    CONTEXT("Creating Vulkan command pool");
    ASSERT(engine.device != nullptr);
    ASSERT(engine.queue_family_index != UINT32_MAX);

    const auto pool = engine.device.createCommandPool({
        .flags = flags,
        .queueFamilyIndex = engine.queue_family_index,
    });
    switch (pool.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
    return pool.value;
}

void allocate_command_buffers(const Engine& engine, const std::span<vk::CommandBuffer> out_cmds) {
    CONTEXT("Allocating Vulkan command buffers");
    ASSERT(engine.device != nullptr);
    ASSERT(engine.command_pool != nullptr);

    const vk::CommandBufferAllocateInfo alloc_info = {
        .commandPool = engine.command_pool,
        .commandBufferCount = to_u32(out_cmds.size()),
    };
    const auto cmd_result = engine.device.allocateCommandBuffers(&alloc_info, out_cmds.data());
    switch (cmd_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
}

vk::CommandBuffer begin_single_time_commands(const Engine& engine) {
    CONTEXT("Beginning Vulkan single time command buffer");
    ASSERT(engine.device != nullptr);
    ASSERT(engine.single_time_command_pool != nullptr);

    vk::CommandBufferAllocateInfo alloc_info = {
        .commandPool = engine.single_time_command_pool,
        .commandBufferCount = 1,
    };
    vk::CommandBuffer cmd = {};
    const auto cmd_result = engine.device.allocateCommandBuffers(&alloc_info, &cmd);
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

void end_single_time_commands(const Engine& engine, const vk::CommandBuffer cmd) {
    CONTEXT("Ending Vulkan single time command buffer");
    ASSERT(engine.device != nullptr);
    ASSERT(engine.single_time_command_pool != nullptr);
    ASSERT(cmd != nullptr);

    const auto end_result = cmd.end();
    switch (end_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorInvalidVideoStdParametersKHR: ERROR("Vulkan invalid video parameters");
        default: ERROR("Unexpected Vulkan error");
    }

    const auto fence = engine.device.createFence({});
    switch (fence.result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
    defer(engine.device.destroyFence(fence.value));

    vk::SubmitInfo submit_info = {
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    const auto submit_result = engine.queue.submit({submit_info}, fence.value);
    switch (submit_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    const auto wait_result = engine.device.waitForFences({fence.value}, vk::True, 1'000'000'000);
    switch (wait_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eTimeout: WARN("Vulkan fence timed out"); break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    engine.device.freeCommandBuffers(engine.single_time_command_pool, {cmd});
}

Result<Window> Window::create(const Engine& engine, const bool fullscreen, const i32 width, const i32 height) {
    CONTEXT("Creating window");
    ASSERT(engine.instance != nullptr);
    ASSERT(engine.device != nullptr);
    ASSERT(engine.command_pool != nullptr);
    if (!fullscreen) {
        ASSERT(width > 0);
        ASSERT(height > 0);
    }

    auto window = ok<Window>();

    if (fullscreen) {
        const auto monitor = glfwGetPrimaryMonitor();
        if (monitor == nullptr)
            return Err::MonitorUnvailable;

        const auto video_mode = glfwGetVideoMode(monitor);
        if (video_mode == nullptr)
            ERROR("Could not find video mode");

        window->m_window = glfwCreateWindow(video_mode->width, video_mode->width, "Hurdy Gurdy", monitor, nullptr);
        if (window->m_window == nullptr)
            ERROR("Could not create window");

    } else {
        window->m_window = glfwCreateWindow(width, height, "Hurdy Gurdy", nullptr, nullptr);
        if (window->m_window == nullptr)
            ERROR("Could not create window");
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const auto surface_result = glfwCreateWindowSurface(engine.instance, window->m_window, nullptr, &surface);
    switch (surface_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_EXTENSION_NOT_PRESENT: return Err::VulkanExtensionUnavailable;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: ERROR("Window surface creation requires the window to have the client API set to GLFW_NO_API");
        default: ERROR("Unexpected Vulkan error");
    }
    window->m_surface = surface;

    const auto window_result = window->resize(engine);
    if (window_result.has_err())
        return window_result.err();

    allocate_command_buffers(engine, window->m_command_buffers);

    for (auto& fence : window->m_frame_finished_fences) {
        fence = Fence::create(engine, {vk::FenceCreateFlagBits::eSignaled});
    }
    for (auto& semaphore : window->m_image_available_semaphores) {
        semaphore = Semaphore::create(engine);
    }
    for (auto& semaphore : window->m_ready_to_present_semaphores) {
        semaphore = Semaphore::create(engine);
    }

    ASSERT(window->m_window != nullptr);
    ASSERT(window->m_surface != nullptr);
    ASSERT(window->m_extent != nullptr);
    ASSERT(window->m_swapchain != nullptr);
    for (usize i = 0; i < window->m_image_count; ++i) {
        ASSERT(window->m_swapchain_images[i] != nullptr);
    }
    for (const auto cmd : window->m_command_buffers) {
        ASSERT(cmd != nullptr);
    }
    return window;
}

void Window::destroy(const Engine& engine) const {
    CONTEXT("Destroying window");
    ASSERT(engine.device != nullptr);

    for (const auto fence : m_frame_finished_fences) {
        fence.destroy(engine);
    }
    for (const auto semaphore : m_image_available_semaphores) {
        semaphore.destroy(engine);
    }
    for (const auto semaphore : m_ready_to_present_semaphores) {
        semaphore.destroy(engine);
    }

    ASSERT(engine.command_pool != nullptr);
    for (const auto cmd : m_command_buffers) {
        ASSERT(cmd != nullptr);
    }
    engine.device.freeCommandBuffers(engine.command_pool, to_u32(m_command_buffers.size()), m_command_buffers.data());

    ASSERT(m_swapchain != nullptr);
    engine.device.destroySwapchainKHR(m_swapchain);

    ASSERT(m_surface != nullptr);
    engine.instance.destroySurfaceKHR(m_surface);

    ASSERT(m_window != nullptr);
    glfwDestroyWindow(m_window);
}

Result<void> Window::resize(const Engine& engine) {
    CONTEXT("Resizing window");
    ASSERT(engine.gpu != nullptr);
    ASSERT(engine.device != nullptr);

    const auto [surface_result, surface_capabilities] = engine.gpu.getSurfaceCapabilitiesKHR(m_surface);
    switch (surface_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorSurfaceLostKHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }
    if (surface_capabilities.currentExtent.width <= 1 || surface_capabilities.currentExtent.height <= 1)
        return Err::InvalidWindow;

    const auto [present_mode_result, present_modes] = engine.gpu.getSurfacePresentModesKHR(m_surface);
    switch (surface_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eIncomplete: WARN("Vulkan get present modes incomplete"); break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorSurfaceLostKHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }

    const auto new_swapchain = engine.device.createSwapchainKHR({
        .surface = m_surface,
        .minImageCount = surface_capabilities.maxImageCount == 0
                         ? MaxSwapchainImages
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

    const auto wait_result = engine.queue.waitIdle();
    switch (wait_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    if (m_swapchain != nullptr) {
        engine.device.destroySwapchainKHR(m_swapchain);
    }
    m_swapchain = new_swapchain.value;
    m_extent = surface_capabilities.currentExtent;

    const vk::Result image_count_result = engine.device.getSwapchainImagesKHR(m_swapchain, &m_image_count, nullptr);
    if (image_count_result != vk::Result::eSuccess)
        ERROR("Could not get swapchain images");
    const vk::Result image_result = engine.device.getSwapchainImagesKHR(m_swapchain, &m_image_count, m_swapchain_images.data());
    if (image_result != vk::Result::eSuccess)
        ERROR("Could not get swapchain images");

    ASSERT(m_swapchain != nullptr);
    for (const auto image : m_swapchain_images) {
        ASSERT(image != nullptr);
    }
    return ok();
}

Result<vk::CommandBuffer> Window::begin_frame(const Engine& engine) {
    ASSERT(!m_recording);
    ASSERT(current_cmd() != nullptr);
    ASSERT(engine.device != nullptr);

    m_current_frame_index = (m_current_frame_index + 1) % MaxFramesInFlight;

    is_frame_finished().wait(engine);
    is_frame_finished().reset(engine);

    const auto acquire_result = engine.device.acquireNextImageKHR(
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

    const auto begin_result = current_cmd().begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    switch (begin_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
    m_recording = true;

    const vk::Viewport viewport = {
        0.0f, 0.0f,
        static_cast<f32>(m_extent.width), static_cast<f32>(m_extent.height),
        0.0f, 1.0f,
    };
    current_cmd().setViewportWithCount({viewport});
    const vk::Rect2D scissor = {{0, 0}, m_extent};
    current_cmd().setScissorWithCount({scissor});

    current_cmd().setRasterizerDiscardEnable(vk::False);
    current_cmd().setPrimitiveRestartEnable(vk::False);
    current_cmd().setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
    current_cmd().setPolygonModeEXT(vk::PolygonMode::eFill);
    current_cmd().setFrontFace(vk::FrontFace::eCounterClockwise);
    current_cmd().setCullMode(vk::CullModeFlagBits::eNone);
    current_cmd().setDepthTestEnable(vk::True);
    current_cmd().setDepthWriteEnable(vk::True);
    current_cmd().setDepthCompareOp(vk::CompareOp::eLess);
    current_cmd().setDepthBiasEnable(vk::False);
    current_cmd().setStencilTestEnable(vk::False);
    current_cmd().setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1);
    current_cmd().setSampleMaskEXT(vk::SampleCountFlagBits::e1, vk::SampleMask{0xff});
    current_cmd().setAlphaToCoverageEnableEXT(vk::False);
    current_cmd().setColorWriteMaskEXT(0, {
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    });
    current_cmd().setColorBlendEnableEXT(0, {vk::False});

    return ok(current_cmd());
}

Result<void> Window::end_frame(const Engine& engine) {
    ASSERT(m_recording);
    ASSERT(m_swapchain != nullptr);
    ASSERT(current_cmd() != nullptr);
    ASSERT(engine.device != nullptr);

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
    vk::SubmitInfo submit_info = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = is_image_available().ptr(),
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &current_cmd(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = is_ready_to_present().ptr(),
    };

    const auto submit_result = engine.queue.submit({submit_info}, is_frame_finished().get());
    switch (submit_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    const vk::PresentInfoKHR present_info = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = is_ready_to_present().ptr(),
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain,
        .pImageIndices = &m_current_image_index,
    };
    const auto present_result = engine.queue.presentKHR(present_info);
    switch (present_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eSuboptimalKHR: return Err::InvalidWindow;
        case vk::Result::eErrorOutOfDateKHR: return Err::InvalidWindow;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        case vk::Result::eErrorSurfaceLostKHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }

    return ok();
}

} // namespace hg
