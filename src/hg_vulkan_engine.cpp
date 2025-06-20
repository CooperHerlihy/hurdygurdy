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

namespace hg {

#ifdef NDEBUG
inline constexpr std::array<const char*, 0> ValidationLayers = {};
#else
inline constexpr std::array ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
#endif
constexpr std::array DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
};

static vk::Bool32 debug_callback(const vk::DebugUtilsMessageSeverityFlagBitsEXT severity, const vk::DebugUtilsMessageTypeFlagsEXT, const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void*) {
    std::cout << std::format("{}\n", callback_data->pMessage);
    if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        __debugbreak();
    return VK_FALSE;
}

const vk::DebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfo = {
    .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
    .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
    .pfnUserCallback = debug_callback,
};

[[nodiscard]] static Result<std::vector<const char*>> get_required_instance_extensions() {
    u32 glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    if (glfw_extensions == nullptr)
        return Err::GlfwFailure;

    auto required_extensions = ok<std::vector<const char*>>();
    required_extensions->reserve(glfw_extension_count + 1);
    for (usize i = 0; i < glfw_extension_count; ++i)
        required_extensions->emplace_back(glfw_extensions[i]);
#ifndef NDEBUG
    required_extensions->emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    return required_extensions;
}

[[nodiscard]] static Result<bool> check_instance_extension_availability(const std::span<const char* const> required_extensions) {
    debug_assert(!required_extensions.empty());

    const auto extensions = vk::enumerateInstanceExtensionProperties();
    if (extensions.result != vk::Result::eSuccess)
        return Err::VulkanFailure;

    return ok(std::ranges::all_of(required_extensions, [&](const char* required) {
        return std::ranges::any_of(extensions.value, [&](const VkExtensionProperties& extension) {
            return strcmp(required, extension.extensionName) == 0;
        });
    }));
}

static Result<vk::Instance> init_instance() {
    const vk::ApplicationInfo app_info = {
        .pApplicationName = "Hurdy Gurdy",
        .applicationVersion = 0,
        .pEngineName = "Hurdy Gurdy",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };

    const auto required_extensions = get_required_instance_extensions();
    if (required_extensions.has_err())
        return required_extensions.err();

    const auto extensions_available = check_instance_extension_availability(*required_extensions);
    if (extensions_available.has_err())
        return extensions_available.err();
    if (!*extensions_available)
        return Err::VulkanExtensionsUnavailable;

    const auto instance = vk::createInstance({
        .pNext = &DebugUtilsMessengerCreateInfo,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = to_u32(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = to_u32(required_extensions->size()),
        .ppEnabledExtensionNames = required_extensions->data(),
    });
    if (instance.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkInstance;
    return ok(instance.value);
}

static Result<u32> find_queue_family(const vk::PhysicalDevice gpu) {
    debug_assert(gpu != nullptr);

    const auto queue_families = gpu.getQueueFamilyProperties();
    const auto queue_family = std::find_if(queue_families.begin(), queue_families.end(), [](const vk::QueueFamilyProperties family) {
        return family.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute);
    });
    if (queue_family == queue_families.end())
        return Err::VkQueueFamilyUnavailable;
    return ok(static_cast<u32>(queue_family - queue_families.begin()));
}

static Result<vk::PhysicalDevice> find_gpu(const Engine& engine) {
    debug_assert(engine.instance != nullptr);

    const auto gpus = engine.instance.enumeratePhysicalDevices();
    if (gpus.result != vk::Result::eSuccess)
        return Err::VkPhysicalDevicesUnavailable;

    for (const auto gpu : gpus.value) {
        const auto features = gpu.getFeatures();
        if (features.sampleRateShading != vk::True || features.samplerAnisotropy != vk::True)
            continue;

        const auto extensions = gpu.enumerateDeviceExtensionProperties();
        if (extensions.result != vk::Result::eSuccess)
            return Err::VulkanFailure;

        if (!std::ranges::all_of(DeviceExtensions, [&](const char* required) {
            return std::ranges::any_of(extensions.value, [&](const vk::ExtensionProperties extension) {
                return strcmp(required, extension.extensionName);
            });
        }))
            continue;

        if (find_queue_family(gpu).has_err())
            continue;

        return ok(gpu);
    }

    return Err::VkPhysicalDevicesUnsuitable;
}

static Result<vk::Device> init_device(const Engine& engine) {
    debug_assert(engine.gpu != nullptr);
    debug_assert(engine.queue_family_index != UINT32_MAX);

    vk::PhysicalDeviceShaderObjectFeaturesEXT shader_object_feature = {.pNext = nullptr, .shaderObject = vk::True};
    vk::PhysicalDeviceBufferAddressFeaturesEXT buffer_address_feature = {.pNext = &shader_object_feature, .bufferDeviceAddress = vk::True};
    vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature = {.pNext = &buffer_address_feature, .dynamicRendering = vk::True};
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
    if (device.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkDevice;
    return ok(device.value);
}

static Result<VmaAllocator> init_allocator(const Engine& engine) {
    debug_assert(engine.instance != nullptr);
    debug_assert(engine.gpu != nullptr);
    debug_assert(engine.device != nullptr);

    VmaAllocatorCreateInfo info = {};
    info.physicalDevice = engine.gpu;
    info.device = engine.device;
    info.instance = engine.instance;
    info.vulkanApiVersion = VK_API_VERSION_1_3;

    auto allocator = ok<VmaAllocator>(nullptr);
    const auto result = vmaCreateAllocator(&info, &*allocator);
    if (result != VK_SUCCESS)
        return Err::CouldNotCreateVmaAllocator;
    return allocator;
}

static bool s_engine_initialized = false;

Result<Engine> Engine::create() {
    if (s_engine_initialized)
        critical_error("Cannot initialize more than one engine");

    auto engine = ok<Engine>();

    const auto glfw_success = glfwInit();
    if (glfw_success == GLFW_FALSE)
        return Err::CouldNotInitializeGlfw;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    const auto instance = init_instance();
    if (instance.has_err())
        return instance.err();
    engine->instance = *instance;

    VULKAN_HPP_DEFAULT_DISPATCHER.init(engine->instance);

    const auto messenger = engine->instance.createDebugUtilsMessengerEXT(DebugUtilsMessengerCreateInfo);
    if (messenger.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkDebugUtilsMessenger;
    engine->debug_messenger = messenger.value;

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

    const auto allocator = init_allocator(*engine);
    if (allocator.has_err())
        return allocator.err();
    engine->allocator = *allocator;

    const auto pool = engine->device.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = engine->queue_family_index,
    });
    if (pool.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkCommandPool;
    engine->command_pool = pool.value;

    const auto transient_pool = engine->device.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = engine->queue_family_index,
    });
    if (transient_pool.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkCommandPool;
    engine->single_time_command_pool = transient_pool.value;

    s_engine_initialized = true;
    return engine;
}

void Engine::destroy() const {
    if (!s_engine_initialized)
        critical_error("Cannot destroy uninitialized engine");

    debug_assert(single_time_command_pool != nullptr);
    device.destroyCommandPool(single_time_command_pool);
    debug_assert(command_pool != nullptr);
    device.destroyCommandPool(command_pool);

    debug_assert(allocator != nullptr);
    vmaDestroyAllocator(allocator);

    debug_assert(device != nullptr);
    device.destroy();

    debug_assert(debug_messenger != nullptr);
    instance.destroyDebugUtilsMessengerEXT(debug_messenger);

    debug_assert(instance != nullptr);
    instance.destroy();

    s_engine_initialized = false;
}

Result<Window> Window::create(const Engine& engine, const i32 width, const i32 height) {
    debug_assert(engine.device != nullptr);
    debug_assert(width > 0);
    debug_assert(height > 0);

    auto window = ok<Window>();

    window->window = glfwCreateWindow(width, height, "Hurdy Gurdy", nullptr, nullptr);
    if (window->window == nullptr)
        return Err::GlfwFailure;

    debug_assert(engine.instance != nullptr);
    const auto surface_result = glfwCreateWindowSurface(engine.instance, window->window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&window->surface));
    if (surface_result != VK_SUCCESS)
        return Err::GlfwFailure;

    const auto window_result = window->resize(engine);
    if (window_result.has_err())
        return window_result.err();

    debug_assert(engine.command_pool != nullptr);
    const vk::CommandBufferAllocateInfo cmd_info{
        .commandPool = engine.command_pool,
        .commandBufferCount = MaxFramesInFlight,
    };
    const auto cmd_result = engine.device.allocateCommandBuffers(&cmd_info, window->m_command_buffers.data());
    if (cmd_result != vk::Result::eSuccess)
        return Err::CouldNotAllocateVkCommandBuffers;

    for (auto& fence : window->m_frame_finished_fences) {
        const auto new_fence = engine.device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});
        if (new_fence.result != vk::Result::eSuccess)
            return Err::CouldNotCreateVkFence;
        fence = new_fence.value;
    }
    for (auto& semaphore : window->m_image_available_semaphores) {
        const auto new_semaphore = engine.device.createSemaphore({});
        if (new_semaphore.result != vk::Result::eSuccess)
            return Err::CouldNotCreateVkSemaphore;
        semaphore = new_semaphore.value;
    }
    for (auto& semaphore : window->m_ready_to_present_semaphores) {
        const auto new_semaphore = engine.device.createSemaphore({});
        if (new_semaphore.result != vk::Result::eSuccess)
            return Err::CouldNotCreateVkSemaphore;
        semaphore = new_semaphore.value;
    }

    return window;
}

void Window::destroy(const Engine& engine) const {
    debug_assert(engine.device != nullptr);

    for (auto& fence : m_frame_finished_fences) {
        debug_assert(fence != nullptr);
        engine.device.destroyFence(fence);
    }
    for (auto& semaphore : m_image_available_semaphores) {
        debug_assert(semaphore != nullptr);
        engine.device.destroySemaphore(semaphore);
    }
    for (auto& semaphore : m_ready_to_present_semaphores) {
        debug_assert(semaphore != nullptr);
        engine.device.destroySemaphore(semaphore);
    }

    debug_assert(engine.command_pool != nullptr);
    debug_assert(m_command_buffers[0] != nullptr);
    engine.device.freeCommandBuffers(engine.command_pool, to_u32(m_command_buffers.size()), m_command_buffers.data());

    for (usize i = 0; i < image_count; ++i) {
        debug_assert(swapchain_views[i] != nullptr);
        engine.device.destroyImageView(swapchain_views[i]);
    }
    debug_assert(swapchain != nullptr);
    engine.device.destroySwapchainKHR(swapchain);

    debug_assert(surface != nullptr);
    engine.instance.destroySurfaceKHR(surface);

    debug_assert(window != nullptr);
    glfwDestroyWindow(window);
}

Result<void> Window::resize(const Engine& engine) {
    debug_assert(engine.gpu != nullptr);
    debug_assert(engine.device != nullptr);

    const auto [surface_result, surface_capabilities] = engine.gpu.getSurfaceCapabilitiesKHR(surface);
    if (surface_result != vk::Result::eSuccess)
        return Err::VulkanFailure;
    if (surface_capabilities.currentExtent.width <= 0 || surface_capabilities.currentExtent.height <= 0)
        return Err::InvalidWindowSize;

    const auto [present_mode_result, present_modes] = engine.gpu.getSurfacePresentModesKHR(surface);
    if (present_mode_result != vk::Result::eSuccess)
        return Err::VulkanFailure;

    const auto new_swapchain = engine.device.createSwapchainKHR({
        .surface = surface,
        .minImageCount = surface_capabilities.maxImageCount == 0 ? MaxSwapchainImages : std::min(surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount),
        .imageFormat = SwapchainImageFormat,
        .imageColorSpace = SwapchainColorSpace,
        .imageExtent = surface_capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        .preTransform = surface_capabilities.currentTransform,
        .presentMode = std::ranges::any_of(present_modes, [](const vk::PresentModeKHR mode) { return mode == vk::PresentModeKHR::eMailbox; }) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo,
        .clipped = vk::True,
        .oldSwapchain = swapchain,
    });
    if (new_swapchain.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkSwapchain;

    if (swapchain != nullptr) {
        for (usize i = 0; i < image_count; ++i) {
            debug_assert(swapchain_views[i] != nullptr);
            engine.device.destroyImageView(swapchain_views[i]);
        }
        engine.device.destroySwapchainKHR(swapchain);
    }
    swapchain = new_swapchain.value;
    extent = surface_capabilities.currentExtent;

    const vk::Result image_count_result = engine.device.getSwapchainImagesKHR(swapchain, &image_count, nullptr);
    if (image_count_result != vk::Result::eSuccess)
        return Err::VkSwapchainImagesUnavailable;
    const vk::Result image_result = engine.device.getSwapchainImagesKHR(swapchain, &image_count, swapchain_images.data());
    if (image_result != vk::Result::eSuccess)
        return Err::VkSwapchainImagesUnavailable;

    for (size_t i = 0; i < image_count; ++i) {
        const vk::ImageViewCreateInfo view_info = {
            .image = swapchain_images[i],
            .viewType = vk::ImageViewType::e2D,
            .format = SwapchainImageFormat,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1},
        };
        const auto view = engine.device.createImageView(view_info);
        if (view.result != vk::Result::eSuccess)
            return Err::CouldNotCreateGpuImageView;
        swapchain_views[i] = view.value;
    }

    return ok();
}

Result<vk::CommandBuffer> Window::begin_frame(const Engine& engine) {
    debug_assert(engine.device != nullptr);
    debug_assert(!m_recording);

    const auto wait_result = engine.device.waitForFences({is_frame_finished()}, vk::True, 1'000'000'000);
    if (wait_result != vk::Result::eSuccess)
        return Err::CouldNotWaitForVkFence;
    const auto reset_result = engine.device.resetFences({is_frame_finished()});
    if (reset_result != vk::Result::eSuccess)
        return Err::VulkanFailure;

    const auto acquire_result = engine.device.acquireNextImageKHR(swapchain, UINT64_MAX, is_image_available(), nullptr);
    if (acquire_result.result != vk::Result::eSuccess)
        return Err::CouldNotAcquireVkSwapchainImage;
    current_image_index = acquire_result.value;

    debug_assert(current_cmd() != nullptr);
    const auto begin_result = current_cmd().begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    if (begin_result != vk::Result::eSuccess)
        return Err::CouldNotBeginVkCommandBuffer;

    const vk::Viewport viewport = {0.0f, 0.0f, static_cast<f32>(extent.width), static_cast<f32>(extent.height), 0.0f, 1.0f};
    current_cmd().setViewportWithCount({viewport});
    const vk::Rect2D scissor = {{0, 0}, extent};
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
    current_cmd().setColorWriteMaskEXT(0, {vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA});
    current_cmd().setColorBlendEnableEXT(0, {vk::False});

    m_recording = true;
    return ok(current_cmd());
}

Result<void> Window::end_frame(const Engine& engine) {
    debug_assert(engine.device != nullptr);
    debug_assert(m_recording);

    debug_assert(current_cmd() != nullptr);
    const auto end_result = current_cmd().end();
    if (end_result != vk::Result::eSuccess)
        return Err::CouldNotEndVkCommandBuffer;
    m_recording = false;

    constexpr vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    debug_assert(is_image_available() != nullptr);
    debug_assert(is_ready_to_present() != nullptr);
    vk::SubmitInfo submit_info = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &is_image_available(),
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &current_cmd(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &is_ready_to_present(),
    };

    const auto submit_result = engine.queue.submit({submit_info}, is_frame_finished());
    if (submit_result != vk::Result::eSuccess)
        return Err::CouldNotSubmitVkCommandBuffer;

    debug_assert(is_ready_to_present() != nullptr);
    debug_assert(swapchain != nullptr);
    const vk::PresentInfoKHR present_info = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &is_ready_to_present(),
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &current_image_index,
    };
    const auto present_result = engine.queue.presentKHR(present_info);
    if (present_result == vk::Result::eErrorOutOfDateKHR)
        return Err::InvalidWindowSize;
    if (present_result != vk::Result::eSuccess)
        return Err::CouldNotPresentVkSwapchainImage;

    m_current_frame_index = (m_current_frame_index + 1) % MaxFramesInFlight;
    return ok();
}

Result<GpuBuffer> GpuBuffer::create_result(const Engine& engine, const vk::DeviceSize size, const vk::BufferUsageFlags usage, const MemoryType memory_type) {
    debug_assert(engine.allocator != nullptr);
    debug_assert(size != 0);
    debug_assert(usage != vk::BufferUsageFlags{});

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = static_cast<VkBufferUsageFlags>(usage);

    VmaAllocationCreateInfo alloc_info = {};
    if (memory_type == RandomAccess) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    } else if (memory_type == Staging) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    } else if (memory_type == DeviceLocal) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_info.flags = 0;
    } else {
        debug_assert(!"Invalid buffer memory type");
    }

    VkBuffer buffer = nullptr;
    VmaAllocation allocation = nullptr;
    const auto buffer_result = vmaCreateBuffer(engine.allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr);
    if (buffer_result != VK_SUCCESS) {
        return Err::CouldNotCreateGpuBuffer;
    }
    return ok<GpuBuffer>(allocation, buffer, memory_type);
}

Result<void> GpuBuffer::write_result(const Engine& engine, const void* data, const vk::DeviceSize size, const vk::DeviceSize offset) const {
    debug_assert(engine.allocator != nullptr);
    debug_assert(allocation != nullptr);
    debug_assert(buffer != nullptr);
    debug_assert(data != nullptr);
    debug_assert(size != 0);
    debug_assert((offset == 0 && memory_type == Staging) || memory_type != Staging);

    if (memory_type == RandomAccess || memory_type == Staging) {
        const auto copy_result = vmaCopyMemoryToAllocation(engine.allocator, data, allocation, offset, size);
        if (copy_result != VK_SUCCESS) {
            return Err::CouldNotWriteGpuBuffer;
        }
        return ok();
    }
    debug_assert(memory_type == DeviceLocal);

    const auto staging_buffer = create_result(engine, size, vk::BufferUsageFlagBits::eTransferSrc, Staging);
    if (staging_buffer.has_err())
        return staging_buffer.err();
    defer(vmaDestroyBuffer(engine.allocator, staging_buffer->buffer, staging_buffer->allocation));
    const auto copy_result = vmaCopyMemoryToAllocation(engine.allocator, data, staging_buffer->allocation, 0, size);
    if (copy_result != VK_SUCCESS)
        return staging_buffer.err();

    auto submit = submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        cmd.copyBuffer(staging_buffer->buffer, buffer, {vk::BufferCopy(offset, 0, size)});
    });
    if (submit.has_err())
        return Err::CouldNotWriteGpuBuffer;
    return ok();
}

Result<StagingGpuImage> StagingGpuImage::create(const Engine& engine, const Config& config) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.allocator != nullptr);
    debug_assert(config.extent.width > 0);
    debug_assert(config.extent.height > 0);
    debug_assert(config.extent.depth > 0);
    debug_assert(config.format != vk::Format::eUndefined);
    debug_assert(config.usage != vk::ImageUsageFlags{});
    debug_assert(config.sample_count != vk::SampleCountFlagBits{});
    debug_assert(config.mip_levels > 0);

    VkImageType dimensions = VK_IMAGE_TYPE_3D;
    if (config.extent.depth == 1) {
        dimensions = static_cast<VkImageType>(dimensions - 1);
        if (config.extent.height == 1)
            dimensions = static_cast<VkImageType>(dimensions - 1);
    }
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = dimensions;
    image_info.format = static_cast<VkFormat>(config.format);
    image_info.extent = config.extent;
    image_info.mipLevels = config.mip_levels;
    image_info.arrayLayers = 1;
    image_info.samples = static_cast<VkSampleCountFlagBits>(config.sample_count);
    image_info.usage = static_cast<VkImageUsageFlags>(config.usage);

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = 0;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    const auto image_result = vmaCreateImage(engine.allocator, &image_info, &alloc_info, &image, &allocation, nullptr);
    if (image_result != VK_SUCCESS)
        return Err::CouldNotCreateGpuImage;

    return ok<StagingGpuImage>(allocation, image);
}

Result<void> StagingGpuImage::write(const Engine& engine, const void* data, const vk::Extent3D extent, const u64 pixel_alignment, const vk::ImageLayout final_layout, const vk::ImageSubresourceRange& subresource) const {
    debug_assert(engine.allocator != nullptr);
    debug_assert(allocation != nullptr);
    debug_assert(image != nullptr);
    debug_assert(data != nullptr);
    debug_assert(extent.width > 0);
    debug_assert(extent.height > 0);
    debug_assert(extent.depth > 0);

    const VkDeviceSize size = extent.width * extent.height * extent.depth * pixel_alignment;

    const auto staging_buffer = GpuBuffer::create_result(engine, size, vk::BufferUsageFlagBits::eTransferSrc, GpuBuffer::MemoryType::Staging);
    if (staging_buffer.has_err())
        return staging_buffer.err();
    defer(vmaDestroyBuffer(engine.allocator, staging_buffer->buffer, staging_buffer->allocation));
    const auto staging_write = staging_buffer->write_result(engine, data, size, 0);
    if (staging_write.has_err())
        return staging_write.err();

    const auto submit_result = submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        BarrierBuilder(cmd)
            .add_image_barrier(image, subresource)
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run();

        const vk::BufferImageCopy2 copy_region = {.imageSubresource = {subresource.aspectMask, 0, 0, 1}, .imageExtent = extent};
        cmd.copyBufferToImage2({
            .srcBuffer = staging_buffer->buffer,
            .dstImage = image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions = &copy_region
        });

        BarrierBuilder(cmd)
            .add_image_barrier(image, subresource)
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, final_layout)
            .build_and_run();
    });
    if (submit_result.has_err())
        return Err::CouldNotWriteGpuImage;

    return ok();
}

Result<GpuImage> GpuImage::create_result(const Engine& engine, const Config& config) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.allocator != nullptr);
    debug_assert(config.extent.width > 0);
    debug_assert(config.extent.height > 0);
    debug_assert(config.extent.depth > 0);
    debug_assert(config.format != vk::Format::eUndefined);
    debug_assert(config.usage != vk::ImageUsageFlags{});
    debug_assert(config.aspect_flags != vk::ImageAspectFlagBits{});
    debug_assert(config.sample_count != vk::SampleCountFlagBits{});
    debug_assert(config.mip_levels > 0);

    const auto staging = StagingGpuImage::create(engine, {config.extent, config.format, config.usage, config.sample_count, config.mip_levels});
    if (staging.has_err())
        return Err::CouldNotCreateGpuImage;

    vk::ImageViewType dimensions = vk::ImageViewType::e3D;
    if (config.extent.depth == 1) {
        dimensions = static_cast<vk::ImageViewType>(static_cast<int>(dimensions) - 1);
        if (config.extent.height == 1)
            dimensions = static_cast<vk::ImageViewType>(static_cast<int>(dimensions) - 1);
    }
    const auto view = engine.device.createImageView({
        .image = staging->image,
        .viewType = dimensions,
        .format = config.format,
        .subresourceRange = {config.aspect_flags, 0, config.mip_levels, 0, 1},
    });
    if (view.result != vk::Result::eSuccess)
        return Err::CouldNotCreateGpuImageView;

    if (config.layout != vk::ImageLayout::eUndefined) {
        const auto submit_result = submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
            BarrierBuilder(cmd)
                .add_image_barrier(staging->image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, config.layout)
                .build_and_run();
        });
        if (submit_result.has_err())
            return Err::CouldNotCreateGpuImage;
    }

    return ok<GpuImage>(staging->allocation, staging->image, view.value);
}

Result<GpuImage> GpuImage::create_cubemap(const Engine& engine, const std::filesystem::path path) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.allocator != nullptr);

    const auto data = ImageData::load(path);
    if (data.has_err())
        return data.err();

    const vk::Extent3D staging_extent = {to_u32(data->width), to_u32(data->height), 1};

    const auto staging_image = StagingGpuImage::create(engine, {
        .extent = staging_extent, 
        .format = vk::Format::eR8G8B8A8Srgb, 
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
    });
    if (staging_image.has_err())
        return staging_image.err();
    defer(staging_image->destroy(engine));
    const auto staging_write = staging_image->write(engine, data->pixels.get(), staging_extent, 4, vk::ImageLayout::eTransferSrcOptimal);
    if (staging_write.has_err())
        return staging_write.err();

    const vk::Extent3D extent = {staging_extent.width / 4, staging_extent.height / 3, 1};

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_info.extent = extent;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.arrayLayers = 6;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = 0;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    const auto image_result = vmaCreateImage(engine.allocator, &image_info, &alloc_info, &image, &allocation, nullptr);
    if (image_result != VK_SUCCESS)
        return Err::CouldNotCreateGpuImage;

    const auto view = engine.device.createImageView({
        .image = image,
        .viewType = vk::ImageViewType::eCube,
        .format = vk::Format::eR8G8B8A8Srgb,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6},
    });
    if (view.result != vk::Result::eSuccess)
        return Err::CouldNotCreateGpuImageView;

    const auto submit_result = submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6})
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run();

        std::array copies = {
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 2 / 4, data->height * 1 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .extent = extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 0 / 4, data->height * 1 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 1, 1},
                .extent = extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 1 / 4, data->height * 0 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 2, 1},
                .extent = extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 1 / 4, data->height * 2 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 3, 1},
                .extent = extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 1 / 4, data->height * 1 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 4, 1},
                .extent = extent,
            },
            vk::ImageCopy2{
                .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .srcOffset = {data->width * 3 / 4, data->height * 1 / 3, 0},
                .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 5, 1},
                .extent = extent,
            },
        };
        cmd.copyImage2({
            .srcImage = staging_image->image,
            .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
            .dstImage = image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = to_u32(copies.size()),
            .pRegions = copies.data(),
        });

        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6})
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal)
            .build_and_run();
    });
    if (submit_result.has_err())
        return Err::CouldNotWriteGpuImage;

    return ok<GpuImage>(allocation, image, view.value);
}

Result<void> GpuImage::generate_mipmaps_result(const Engine& engine, const u32 mip_levels, const vk::Extent3D extent, const vk::Format format, const vk::ImageLayout final_layout) const {
    debug_assert(image != nullptr);
    debug_assert(extent.width > 0);
    debug_assert(extent.height > 0);
    debug_assert(extent.depth > 0);
    debug_assert(format != vk::Format::eUndefined);
    debug_assert(final_layout != vk::ImageLayout::eUndefined);

    debug_assert(engine.gpu != nullptr);
    const auto format_properties = engine.gpu.getFormatProperties(format);
    if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        return Err::CouldNotGenerateMipmaps;

    const auto submit_result = submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        vk::Offset3D mip_offset = {to_i32(extent.width), to_i32(extent.height), to_i32(extent.depth)};

        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
            .build_and_run();

        for (u32 level = 0; level < mip_levels - 1; ++level) {
            BarrierBuilder(cmd)
                .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, level + 1, 1, 0, 1})
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
                .srcImage = image,
                .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
                .dstImage = image,
                .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
                .regionCount = 1,
                .pRegions = &region,
                .filter = vk::Filter::eLinear,
            });

            BarrierBuilder(cmd)
                .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, level + 1, 1, 0, 1})
                .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
                .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
                .build_and_run();
        }

        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1})
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, final_layout)
            .build_and_run();
    });
    if (submit_result.has_err())
        return Err::CouldNotGenerateMipmaps;

    return ok();
}

Result<vk::Sampler> create_sampler_result(const Engine& engine, const SamplerConfig& config) {
    debug_assert(engine.gpu != nullptr);
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
    if (config.type == SamplerType::Linear) {
        sampler_info.magFilter = vk::Filter::eLinear;
        sampler_info.minFilter = vk::Filter::eLinear;
        sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
    } else if (config.type == SamplerType::Nearest) {
        sampler_info.magFilter = vk::Filter::eNearest;
        sampler_info.minFilter = vk::Filter::eNearest;
        sampler_info.mipmapMode = vk::SamplerMipmapMode::eNearest;
    } else {
        debug_assert(!"Invalid sampler type");
    }

    debug_assert(engine.device != nullptr);
    const auto sampler = engine.device.createSampler(sampler_info);
    if (sampler.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkSampler;
    return ok(sampler.value);
}

Result<vk::DescriptorSetLayout> create_set_layout(const Engine& engine, const std::span<const vk::DescriptorSetLayoutBinding> bindings) {
    debug_assert(engine.device != nullptr);
    const auto layout = engine.device.createDescriptorSetLayout({
        .bindingCount = to_u32(bindings.size()),
        .pBindings = bindings.data(),
    });
    if (layout.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkDescriptorSetLayout;
    return ok(layout.value);
}

Result<void> allocate_descriptor_sets(const Engine& engine, const vk::DescriptorPool pool, const std::span<const vk::DescriptorSetLayout> layouts, const std::span<vk::DescriptorSet> sets) {
    debug_assert(pool != nullptr);
    debug_assert(!layouts.empty());
    debug_assert(layouts[0] != nullptr);
    debug_assert(!sets.empty());
    debug_assert(sets[0] == nullptr);
    debug_assert(layouts.size() == sets.size());

    const vk::DescriptorSetAllocateInfo alloc_info = {
        .descriptorPool = pool,
        .descriptorSetCount = to_u32(layouts.size()),
        .pSetLayouts = layouts.data(),
    };
    debug_assert(engine.device != nullptr);
    const auto set_result = engine.device.allocateDescriptorSets(&alloc_info, sets.data());
    if (set_result != vk::Result::eSuccess)
        return Err::CouldNotAllocateVkDescriptorSets;
    return ok();
}

void write_uniform_buffer_descriptor(const Engine& engine, const vk::DescriptorSet set, const u32 binding, const vk::Buffer buffer, const vk::DeviceSize size, const vk::DeviceSize offset) {
    debug_assert(set != nullptr);
    debug_assert(buffer != nullptr);
    debug_assert(size != 0);

    const vk::DescriptorBufferInfo buffer_info = {buffer, offset, size};
    const vk::WriteDescriptorSet descriptor_write = {
        .dstSet = set,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &buffer_info,
    };
    debug_assert(engine.device != nullptr);
    engine.device.updateDescriptorSets({descriptor_write}, {});
}

void write_image_sampler_descriptor(const Engine& engine, const vk::DescriptorSet set, const u32 binding, const vk::Sampler sampler, const vk::ImageView view) {
    debug_assert(set != nullptr);
    debug_assert(sampler != nullptr);
    debug_assert(view != nullptr);

    const vk::DescriptorImageInfo image_info = {sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal};
    const vk::WriteDescriptorSet descriptor_write = {
        .dstSet = set,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &image_info,
    };
    debug_assert(engine.device != nullptr);
    engine.device.updateDescriptorSets({descriptor_write}, {});
}

static Result<std::vector<char>> read_shader(const std::filesystem::path path) {
    debug_assert(!path.empty());

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
    return code;
}

Result<vk::ShaderEXT> create_unlinked_shader(const Engine& engine, const ShaderConfig& config) {
    debug_assert(engine.device != nullptr);
    debug_assert(!config.path.empty());
    debug_assert(config.code_type == vk::ShaderCodeTypeEXT::eSpirv && "binary shader code types untested");
    debug_assert(config.stage != vk::ShaderStageFlagBits{0});

    const auto code = read_shader(config.path);
    if (code.has_err())
        return code.err();

    const auto shader = engine.device.createShaderEXT({
        .flags = config.flags,
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
    if (shader.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkShader;
    return ok(shader.value);
}

Result<void> create_linked_shaders(const Engine& engine, const std::span<vk::ShaderEXT> out_shaders, const std::span<const ShaderConfig> configs) {
    debug_assert(engine.device != nullptr);
    debug_assert(configs.size() >= 2);

    std::vector<std::vector<char>> codes = {};
    codes.reserve(configs.size());
    for (const auto& config : configs) {
        debug_assert(!config.path.empty());
        debug_assert(config.code_type == vk::ShaderCodeTypeEXT::eSpirv && "binary shader code types untested");
        debug_assert(config.stage != vk::ShaderStageFlagBits{0});

        const auto code = read_shader(config.path);
        if (code.has_err())
            return code.err();
        codes.emplace_back(std::move(*code));
    }

    std::vector<vk::ShaderCreateInfoEXT> shader_infos = {};
    shader_infos.reserve(configs.size());
    for (usize i = 0; i < codes.size(); ++i) {
        shader_infos.push_back({
            .flags = configs[i].flags | vk::ShaderCreateFlagBitsEXT::eLinkStage,
            .stage = configs[i].stage,
            .nextStage = configs[i].next_stage,
            .codeType = configs[i].code_type,
            .codeSize = codes[i].size(),
            .pCode = codes[i].data(),
            .pName = "main",
            .setLayoutCount = to_u32(configs[i].set_layouts.size()),
            .pSetLayouts = configs[i].set_layouts.data(),
            .pushConstantRangeCount = to_u32(configs[i].push_ranges.size()),
            .pPushConstantRanges = configs[i].push_ranges.data(),
        });
    }

    const auto shader_result = engine.device.createShadersEXT(to_u32(shader_infos.size()), shader_infos.data(), nullptr, out_shaders.data());
    if (shader_result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkShader;
    return ok();
}

Result<vk::CommandBuffer> begin_single_time_commands(const Engine& engine) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.single_time_command_pool != nullptr);

    vk::CommandBufferAllocateInfo alloc_info = {
        .commandPool = engine.single_time_command_pool,
        .commandBufferCount = 1,
    };
    auto cmd = ok<vk::CommandBuffer>();
    const auto cmd_result = engine.device.allocateCommandBuffers(&alloc_info, &*cmd);
    if (cmd_result != vk::Result::eSuccess)
        return Err::CouldNotAllocateVkCommandBuffers;

    const auto begin_result = cmd->begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    if (begin_result != vk::Result::eSuccess)
        return Err::CouldNotBeginVkCommandBuffer;

    return cmd;
}

Result<void> end_single_time_commands(const Engine& engine, const vk::CommandBuffer cmd) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.single_time_command_pool != nullptr);

    const auto end_result = cmd.end();
    if (end_result != vk::Result::eSuccess)
        return Err::CouldNotEndVkCommandBuffer;

    const auto fence = engine.device.createFence({});
    if (fence.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkFence;
    defer(engine.device.destroyFence(fence.value));

    vk::SubmitInfo submit_info = {
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    const auto submit_result = engine.queue.submit({submit_info}, fence.value);
    if (submit_result != vk::Result::eSuccess)
        return Err::CouldNotSubmitVkCommandBuffer;

    const auto wait_result = engine.device.waitForFences({fence.value}, vk::True, UINT64_MAX);
    if (wait_result != vk::Result::eSuccess)
        return Err::CouldNotWaitForVkFence;

    engine.device.freeCommandBuffers(engine.single_time_command_pool, {cmd});
    return ok();
}

} // namespace hg
