#include "hg_vulkan_engine.h"

#include "hg_load.h"
#include "hg_pch.h"
#include "hg_utils.h"

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

static vk::Bool32 debug_callback(const vk::DebugUtilsMessageSeverityFlagBitsEXT severity, const vk::DebugUtilsMessageTypeFlagsEXT,
                                 const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void*) {
    std::cout << std::format("{}\n", callback_data->pMessage);
    debug_assert(!(severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError));
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
    if (glfw_extensions == nullptr) {
        return err(Error::UnknownGLFW, "finding required instance extensions from GLFW");
    }

    auto required_extensions = ok<std::vector<const char*>>();
    required_extensions->reserve(static_cast<usize>(glfw_extension_count) + 1);
    for (usize i = 0; i < glfw_extension_count; ++i) {
        required_extensions->push_back(glfw_extensions[i]);
    }
#ifndef NDEBUG
    required_extensions->push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    return required_extensions;
}

[[nodiscard]] static Result<bool> check_instance_extension_availability(const std::span<const char* const> required_extensions) {
    debug_assert(!required_extensions.empty());

    const auto extensions = vk::enumerateInstanceExtensionProperties();
    if (extensions.result != vk::Result::eSuccess) {
        return err(extensions.result, "enumerating instance extension properties");
    }

    return ok(std::ranges::all_of(required_extensions, [&](const char* required) {
        return std::ranges::any_of(extensions.value, [&](const VkExtensionProperties& extension) { return strcmp(required, extension.extensionName) == 0; });
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

    auto required_extensions = get_required_instance_extensions();
    if (required_extensions.has_err()) {
        return err(required_extensions, "initializing Vulkan instance");
    }
    auto extensions_available = check_instance_extension_availability(*required_extensions);
    if (extensions_available.has_err()) {
        return err(extensions_available, "initializing Vulkan instance");
    }
    if (*extensions_available == false) {
        return err(Error::VulkanResourceNotFound, "initializing Vulkan instance");
    }

    const auto instance = vk::createInstance({
        .pNext = &DebugUtilsMessengerCreateInfo,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<u32>(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = static_cast<u32>(required_extensions->size()),
        .ppEnabledExtensionNames = required_extensions->data(),
    });
    if (instance.result != vk::Result::eSuccess) {
        return err(instance.result, "initializing Vulkan instance");
    }
    return ok(instance.value);
}

static Result<u32> find_queue_family(const vk::PhysicalDevice gpu) {
    debug_assert(gpu != nullptr);

    const auto queue_families = gpu.getQueueFamilyProperties();
    const auto queue_family = std::find_if(queue_families.begin(), queue_families.end(),
                                           [](const vk::QueueFamilyProperties family) { return family.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute); });
    if (queue_family == queue_families.end()) {
        return err(Error::VulkanResourceNotFound, "finding gpu queue family");
    }
    return ok(static_cast<u32>(queue_family - queue_families.begin()));
}

static Result<vk::PhysicalDevice> find_gpu(const Engine& engine) {
    debug_assert(engine.instance != nullptr);

    const auto gpus = engine.instance.enumeratePhysicalDevices();
    if (gpus.result != vk::Result::eSuccess) {
        return err(gpus.result, "enumerating Vulkan physical devices");
    }

    const auto gpu = std::find_if(gpus.value.begin(), gpus.value.end(), [](const vk::PhysicalDevice gpu) {
        const auto features = gpu.getFeatures();
        if (features.sampleRateShading != vk::True || features.samplerAnisotropy != vk::True) {
            return false;
        }

        const auto extensions = gpu.enumerateDeviceExtensionProperties();
        if (extensions.result != vk::Result::eSuccess) {
            return false;
        }
        if (!std::ranges::all_of(DeviceExtensions, [&](const char* required) {
                return std::ranges::any_of(extensions.value, [&](const vk::ExtensionProperties extension) { return strcmp(required, extension.extensionName); });
            })) {
            return false;
        }

        if (!find_queue_family(gpu).has_val()) {
            return false;
        }

        return true;
    });
    if (gpu == gpus.value.end()) {
        return err(Error::VulkanInitFailed, "finding suitable Vulkan physical device");
    }
    return ok(*gpu);
}

static Result<vk::Device> init_device(const Engine& engine) {
    debug_assert(engine.gpu != nullptr);
    debug_assert(engine.queue_family_index != UINT32_MAX);

    float queue_priority = 1.0f;
    vk::DeviceQueueCreateInfo queue_info{.queueFamilyIndex = engine.queue_family_index, .queueCount = 1, .pQueuePriorities = &queue_priority};

    vk::PhysicalDeviceShaderObjectFeaturesEXT shader_object_feature = {.pNext = nullptr, .shaderObject = vk::True};
    vk::PhysicalDeviceBufferAddressFeaturesEXT buffer_address_feature = {.pNext = &shader_object_feature, .bufferDeviceAddress = vk::True};
    vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature = {.pNext = &buffer_address_feature, .dynamicRendering = vk::True};
    vk::PhysicalDeviceSynchronization2Features synchronization2_feature = {.pNext = &dynamic_rendering_feature, .synchronization2 = vk::True};
    constexpr vk::PhysicalDeviceFeatures features = {
        .sampleRateShading = vk::True,
        .samplerAnisotropy = vk::True,
    };

    const auto device = engine.gpu.createDevice({
        .pNext = synchronization2_feature,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledLayerCount = static_cast<u32>(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = static_cast<u32>(DeviceExtensions.size()),
        .ppEnabledExtensionNames = DeviceExtensions.data(),
        .pEnabledFeatures = &features,
    });
    if (device.result != vk::Result::eSuccess) {
        return err(device.result, "initializing Vulkan device");
    }
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
    if (result != VK_SUCCESS) {
        return err(static_cast<vk::Result>(result), "initializing VmaAllocator");
    }
    return allocator;
}

static bool s_engine_initialized = false;

Result<Engine> Engine::create() {
    debug_assert(s_engine_initialized == false);

    auto engine = ok<Engine>();

    const auto glfw_success = glfwInit();
    critical_assert(glfw_success == GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    auto instance = init_instance();
    if (instance.has_err()) {
        return err(instance, "initializing engine");
    }
    engine->instance = instance.get_val();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(engine->instance);

    const auto messenger = engine->instance.createDebugUtilsMessengerEXT(DebugUtilsMessengerCreateInfo);
    if (messenger.result != vk::Result::eSuccess) {
        return err(messenger.result, "initializing engine");
    }
    engine->debug_messenger = messenger.value;

    auto gpu = find_gpu(*engine);
    if (gpu.has_err()) {
        return err(gpu, "initializing engine");
    }
    engine->gpu = *gpu;

    auto queue_family_index = find_queue_family(engine->gpu);
    if (queue_family_index.has_err()) {
        return err(queue_family_index, "initializing engine");
    }
    engine->queue_family_index = *queue_family_index;

    auto device = init_device(*engine);
    if (device.has_err()) {
        return err(device, "initializing engine");
    }
    engine->device = *device;

    VULKAN_HPP_DEFAULT_DISPATCHER.init(engine->device);

    engine->queue = engine->device.getQueue(engine->queue_family_index, 0);
    if (engine->queue == nullptr) {
        return err(Error::VulkanResourceNotFound, "initializing engine Vulkan queue");
    }

    auto allocator = init_allocator(*engine);
    if (allocator.has_err()) {
        return err(allocator, "initializing engine");
    }
    engine->allocator = *allocator;

    const auto pool = engine->device.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = engine->queue_family_index,
    });
    if (pool.result != vk::Result::eSuccess) {
        return err(pool.result, "initializing engine");
    }
    engine->command_pool = pool.value;

    const auto transient_pool = engine->device.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = engine->queue_family_index,
    });
    if (transient_pool.result != vk::Result::eSuccess) {
        return err(transient_pool.result, "initializing engine");
    }
    engine->single_time_command_pool = transient_pool.value;

    s_engine_initialized = true;
    return engine;
}

void Engine::destroy() const {
    debug_assert(s_engine_initialized == true);

    debug_assert(single_time_command_pool != nullptr);
    device.destroyCommandPool(single_time_command_pool);
    debug_assert(command_pool != nullptr);
    device.destroyCommandPool(command_pool);

    debug_assert(allocator != nullptr);
    vmaDestroyAllocator(allocator);

    debug_assert(device != nullptr);
    device.destroy();

    debug_assert(debug_messenger != nullptr);
    const PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    critical_assert(vkDestroyDebugUtilsMessengerEXT != nullptr);
    vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);

    debug_assert(instance != nullptr);
    instance.destroy();

    s_engine_initialized = false;
}

Result<Window> Window::create(const Engine& engine, i32 width, i32 height) {
    debug_assert(engine.device != nullptr);
    debug_assert(width > 0);
    debug_assert(height > 0);

    auto window = ok<Window>();

    window->window = glfwCreateWindow(width, height, "Hurdy Gurdy", nullptr, nullptr);
    if (window->window == nullptr) {
        return err(Error::UnknownGLFW, "initializing window");
    }

    debug_assert(engine.instance != nullptr);
    const auto surface_result = glfwCreateWindowSurface(engine.instance, window->window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&window->surface));
    if (surface_result != VK_SUCCESS) {
        return err(static_cast<vk::Result>(surface_result), "initializing window");
    }

    auto window_result = window->resize(engine);
    if (window_result.has_err()) {
        return err(window_result, "initializing window");
    }

    debug_assert(engine.command_pool != nullptr);
    const vk::CommandBufferAllocateInfo cmd_info{
        .commandPool = engine.command_pool,
        .commandBufferCount = MaxFramesInFlight,
    };
    const auto cmd_result = engine.device.allocateCommandBuffers(&cmd_info, window->m_command_buffers.data());
    if (cmd_result != vk::Result::eSuccess) {
        return err(cmd_result, "initializing window command buffers");
    }

    for (auto& fence : window->m_frame_finished_fences) {
        const auto new_fence = engine.device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});
        critical_assert(new_fence.result == vk::Result::eSuccess);
        fence = new_fence.value;
    }
    for (auto& semaphore : window->m_image_available_semaphores) {
        const auto new_semaphore = engine.device.createSemaphore({});
        critical_assert(new_semaphore.result == vk::Result::eSuccess);
        semaphore = new_semaphore.value;
    }
    for (auto& semaphore : window->m_ready_to_present_semaphores) {
        const auto new_semaphore = engine.device.createSemaphore({});
        critical_assert(new_semaphore.result == vk::Result::eSuccess);
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
    engine.device.freeCommandBuffers(engine.command_pool, static_cast<u32>(m_command_buffers.size()), m_command_buffers.data());

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
    critical_assert(surface_result == vk::Result::eSuccess);
    critical_assert(surface_capabilities.currentExtent.width > 0);
    critical_assert(surface_capabilities.currentExtent.height > 0);

    const auto [present_mode_result, present_modes] = engine.gpu.getSurfacePresentModesKHR(surface);
    critical_assert(present_mode_result == vk::Result::eSuccess);

    const vk::SwapchainCreateInfoKHR swapchain_info = {
        .surface = surface,
        .minImageCount = surface_capabilities.maxImageCount == 0 ? MaxSwapchainImages : std::min(surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount),
        .imageFormat = vk::Format::eR8G8B8A8Srgb,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = surface_capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        .preTransform = surface_capabilities.currentTransform,
        .presentMode = std::ranges::any_of(present_modes, [](const vk::PresentModeKHR mode) { return mode == vk::PresentModeKHR::eMailbox; }) ? vk::PresentModeKHR::eMailbox
                                                                                                                                              : vk::PresentModeKHR::eFifo,
        .clipped = vk::True,
        .oldSwapchain = swapchain,
    };
    const auto new_swapchain = engine.device.createSwapchainKHR(swapchain_info);
    critical_assert(new_swapchain.result == vk::Result::eSuccess);
    if (swapchain != nullptr) {
        for (usize i = 0; i < image_count; ++i) {
            debug_assert(swapchain_views[i] != nullptr);
            engine.device.destroyImageView(swapchain_views[i]);
        }
        engine.device.destroySwapchainKHR(swapchain);
    }
    swapchain = new_swapchain.value;
    extent = swapchain_info.imageExtent;
    image_format = swapchain_info.imageFormat;

    const vk::Result image_count_result = engine.device.getSwapchainImagesKHR(swapchain, &image_count, nullptr);
    critical_assert(image_count_result == vk::Result::eSuccess);
    const vk::Result image_result = engine.device.getSwapchainImagesKHR(swapchain, &image_count, swapchain_images.data());
    critical_assert(image_result == vk::Result::eSuccess);

    for (size_t i = 0; i < image_count; ++i) {
        const vk::ImageViewCreateInfo view_info = {
            .image = swapchain_images[i],
            .viewType = vk::ImageViewType::e2D,
            .format = image_format,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1},
        };
        const auto view = engine.device.createImageView(view_info);
        critical_assert(view.result == vk::Result::eSuccess);
        swapchain_views[i] = view.value;
    }

    return ok();
}

vk::CommandBuffer Window::begin_frame(const Engine& engine) {
    debug_assert(engine.device != nullptr);
    debug_assert(!m_recording);

    const auto wait_result = engine.device.waitForFences({is_frame_finished()}, vk::True, UINT64_MAX);
    critical_assert(wait_result == vk::Result::eSuccess);
    const auto reset_result = engine.device.resetFences({is_frame_finished()});
    critical_assert(reset_result == vk::Result::eSuccess);

    const auto acquire_result = engine.device.acquireNextImageKHR(swapchain, UINT64_MAX, is_image_available(), nullptr);
    critical_assert(acquire_result.result == vk::Result::eSuccess);
    current_image_index = acquire_result.value;

    debug_assert(current_cmd() != nullptr);
    const auto begin_result = current_cmd().begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    critical_assert(begin_result == vk::Result::eSuccess);

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
    return current_cmd();
}

bool Window::end_frame(const Engine& engine) {
    debug_assert(engine.device != nullptr);
    debug_assert(m_recording);

    debug_assert(current_cmd() != nullptr);
    const auto end_result = current_cmd().end();
    critical_assert(end_result == vk::Result::eSuccess);
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
    critical_assert(submit_result == vk::Result::eSuccess);

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
    if (present_result == vk::Result::eErrorOutOfDateKHR) {
        return false;
    }
    critical_assert(present_result == vk::Result::eSuccess);

    m_current_frame_index = (m_current_frame_index + 1) % MaxFramesInFlight;
    return true;
}

GpuBuffer GpuBuffer::create(const Engine& engine, const vk::DeviceSize size, const vk::BufferUsageFlags usage, const MemoryType memory_type) {
    debug_assert(engine.allocator != nullptr);
    debug_assert(size != 0);
    debug_assert(usage != static_cast<vk::BufferUsageFlags>(0));

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
        critical_assert(!"Invalid buffer memory type");
    }

    VkBuffer buffer = nullptr;
    VmaAllocation allocation = nullptr;
    const auto buffer_result = vmaCreateBuffer(engine.allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr);
    critical_assert(buffer_result == VK_SUCCESS);
    return {allocation, buffer, memory_type};
}

void GpuBuffer::write(const Engine& engine, const void* data, const vk::DeviceSize size, const vk::DeviceSize offset) const {
    debug_assert(engine.allocator != nullptr);
    debug_assert(allocation != nullptr);
    debug_assert(buffer != nullptr);
    debug_assert(data != nullptr);
    debug_assert(size != 0);
    debug_assert((offset == 0 && memory_type == Staging) || memory_type != Staging);

    if (memory_type == RandomAccess || memory_type == Staging) {
        const auto copy_result = vmaCopyMemoryToAllocation(engine.allocator, data, allocation, offset, size);
        critical_assert(copy_result == VK_SUCCESS);
        return;
    }
    debug_assert(memory_type == DeviceLocal);

    const auto staging_buffer = create(engine, size, vk::BufferUsageFlagBits::eTransferSrc, Staging);
    defer(vmaDestroyBuffer(engine.allocator, staging_buffer.buffer, staging_buffer.allocation));
    const auto copy_result = vmaCopyMemoryToAllocation(engine.allocator, data, staging_buffer.allocation, 0, size);
    critical_assert(copy_result == VK_SUCCESS);

    submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) { cmd.copyBuffer(staging_buffer.buffer, buffer, {vk::BufferCopy(offset, 0, size)}); });
}

GpuImage GpuImage::create(const Engine& engine, const Config& config) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.allocator != nullptr);
    debug_assert(config.extent.width > 0);
    debug_assert(config.extent.height > 0);
    debug_assert(config.extent.depth > 0);
    debug_assert(config.format != vk::Format::eUndefined);
    debug_assert(config.usage != static_cast<vk::ImageUsageFlags>(0));
    debug_assert(config.aspect_flags != static_cast<vk::ImageAspectFlagBits>(0));
    debug_assert(config.sample_count != static_cast<vk::SampleCountFlagBits>(0));
    debug_assert(config.mip_levels > 0);

    VkImageType dimensions = VK_IMAGE_TYPE_3D;
    if (config.extent.depth == 1) {
        dimensions = static_cast<VkImageType>(dimensions - 1);
        if (config.extent.height == 1) {
            dimensions = static_cast<VkImageType>(dimensions - 1);
        }
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
    critical_assert(image_result == VK_SUCCESS);

    const vk::ImageViewCreateInfo view_info = {
        .image = image,
        .viewType = static_cast<vk::ImageViewType>(dimensions),
        .format = config.format,
        .subresourceRange = {config.aspect_flags, 0, config.mip_levels, 0, 1},
    };
    const auto view = engine.device.createImageView(view_info);
    critical_assert(view.result == vk::Result::eSuccess);

    if (config.layout != vk::ImageLayout::eUndefined) {
        submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
            BarrierBuilder(cmd)
                .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, config.layout)
                .build_and_run();
        });
    }

    return {allocation, image, view.value};
}

struct StagingImage {
    VmaAllocation allocation = nullptr;
    vk::Image image = {};

    struct Config {
        vk::Extent3D extent = {};
        vk::Format format = vk::Format::eUndefined;
        vk::ImageUsageFlags usage = {};
        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;
        u32 mip_levels = 1;
    };

    void destroy(const Engine& engine) const { vmaDestroyImage(engine.allocator, image, allocation); }
    [[nodiscard]] static StagingImage create(const Engine& engine, const Config& config) {
        debug_assert(engine.device != nullptr);
        debug_assert(engine.allocator != nullptr);
        debug_assert(config.extent.width > 0);
        debug_assert(config.extent.height > 0);
        debug_assert(config.extent.depth > 0);
        debug_assert(config.format != vk::Format::eUndefined);
        debug_assert(config.usage != static_cast<vk::ImageUsageFlags>(0));
        debug_assert(config.sample_count != static_cast<vk::SampleCountFlagBits>(0));
        debug_assert(config.mip_levels > 0);

        VkImageType dimensions = VK_IMAGE_TYPE_3D;
        if (config.extent.depth == 1) {
            dimensions = static_cast<VkImageType>(dimensions - 1);
            if (config.extent.height == 1) {
                dimensions = static_cast<VkImageType>(dimensions - 1);
            }
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
        critical_assert(image_result == VK_SUCCESS);

        return {allocation, image};
    }
};

GpuImage GpuImage::create_cubemap(const Engine& engine, const std::filesystem::path path) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.allocator != nullptr);

    const auto data = ImageData::load(path);
    critical_assert(data.has_val());

    const vk::Extent3D staging_extent = {static_cast<u32>(data->width), static_cast<u32>(data->height), 1};
    const VkDeviceSize staging_size = static_cast<u64>(staging_extent.width) * static_cast<u64>(staging_extent.height) * 4;

    const auto staging_buffer = GpuBuffer::create(engine, staging_size, vk::BufferUsageFlagBits::eTransferSrc, GpuBuffer::MemoryType::Staging);
    defer(staging_buffer.destroy(engine));
    staging_buffer.write(engine, data->pixels.get(), staging_size, 0);

    const auto staging_image = StagingImage::create(
        engine, {.extent = staging_extent, .format = vk::Format::eR8G8B8A8Srgb, .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc});
    defer(staging_image.destroy(engine));

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
    critical_assert(image_result == VK_SUCCESS);

    const vk::ImageViewCreateInfo view_info = {
        .image = image,
        .viewType = vk::ImageViewType::eCube,
        .format = vk::Format::eR8G8B8A8Srgb,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6},
    };
    const auto view = engine.device.createImageView(view_info);
    critical_assert(view.result == vk::Result::eSuccess);

    submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        BarrierBuilder(cmd)
            .add_image_barrier(staging_image.image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run();

        const vk::BufferImageCopy2 copy_region = {.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1}, .imageExtent = staging_extent};
        cmd.copyBufferToImage2({.srcBuffer = staging_buffer.buffer,
                                .dstImage = staging_image.image,
                                .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
                                .regionCount = 1,
                                .pRegions = &copy_region});

        BarrierBuilder(cmd)
            .add_image_barrier(staging_image.image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
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
            .srcImage = staging_image.image,
            .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
            .dstImage = image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = static_cast<u32>(copies.size()),
            .pRegions = copies.data(),
        });

        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6})
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal)
            .build_and_run();
    });

    return {allocation, image, view.value};
}

void GpuImage::write(const Engine& engine, const void* data, const vk::Extent3D extent, const u32 pixel_alignment, const vk::ImageLayout final_layout,
                     const vk::ImageSubresourceRange& subresource) const {
    debug_assert(engine.allocator != nullptr);
    debug_assert(allocation != nullptr);
    debug_assert(image != nullptr);
    debug_assert(view != nullptr);
    debug_assert(data != nullptr);
    debug_assert(extent.width > 0);
    debug_assert(extent.height > 0);
    debug_assert(extent.depth > 0);

    const VkDeviceSize size = static_cast<u64>(extent.width) * static_cast<u64>(extent.height) * static_cast<u64>(extent.depth) * static_cast<u64>(pixel_alignment);

    const auto staging_buffer = GpuBuffer::create(engine, size, vk::BufferUsageFlagBits::eTransferSrc, GpuBuffer::MemoryType::Staging);
    defer(vmaDestroyBuffer(engine.allocator, staging_buffer.buffer, staging_buffer.allocation));
    staging_buffer.write(engine, data, size, 0);

    submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        BarrierBuilder(cmd)
            .add_image_barrier(image, subresource)
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run();

        const vk::BufferImageCopy2 copy_region = {.imageSubresource = {subresource.aspectMask, 0, 0, 1}, .imageExtent = extent};
        cmd.copyBufferToImage2(
            {.srcBuffer = staging_buffer.buffer, .dstImage = image, .dstImageLayout = vk::ImageLayout::eTransferDstOptimal, .regionCount = 1, .pRegions = &copy_region});

        BarrierBuilder(cmd)
            .add_image_barrier(image, subresource)
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, final_layout)
            .build_and_run();
    });
}

void GpuImage::generate_mipmaps(const Engine& engine, const u32 mip_levels, const vk::Extent3D extent, const vk::Format format, const vk::ImageLayout final_layout) const {
    debug_assert(image != nullptr);
    debug_assert(extent.width > 0);
    debug_assert(extent.height > 0);
    debug_assert(extent.depth > 0);
    debug_assert(format != vk::Format::eUndefined);
    debug_assert(final_layout != vk::ImageLayout::eUndefined);

    debug_assert(engine.gpu != nullptr);
    const auto format_properties = engine.gpu.getFormatProperties(format);
    critical_assert(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear);

    submit_single_time_commands(engine, [&](const vk::CommandBuffer cmd) {
        vk::Offset3D mip_offset = {static_cast<i32>(extent.width), static_cast<i32>(extent.height), static_cast<i32>(extent.depth)};

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
            if (mip_offset.x > 1) {
                mip_offset.x /= 2;
            }
            if (mip_offset.y > 1) {
                mip_offset.y /= 2;
            }
            if (mip_offset.z > 1) {
                mip_offset.z /= 2;
            }
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
}

void allocate_descriptor_sets(const Engine& engine, const vk::DescriptorPool pool, const std::span<const vk::DescriptorSetLayout> layouts,
                              const std::span<vk::DescriptorSet> sets) {
    debug_assert(pool != nullptr);
    debug_assert(!layouts.empty());
    debug_assert(layouts[0] != nullptr);
    debug_assert(!sets.empty());
    debug_assert(sets[0] == nullptr);
    debug_assert(layouts.size() == sets.size());

    const vk::DescriptorSetAllocateInfo alloc_info = {
        .descriptorPool = pool,
        .descriptorSetCount = static_cast<u32>(layouts.size()),
        .pSetLayouts = layouts.data(),
    };
    debug_assert(engine.device != nullptr);
    const auto set_result = engine.device.allocateDescriptorSets(&alloc_info, sets.data());
    critical_assert(set_result == vk::Result::eSuccess);
}

void write_uniform_buffer_descriptor(const Engine& engine, const vk::DescriptorSet set, const u32 binding, const vk::Buffer buffer, const vk::DeviceSize size,
                                     const vk::DeviceSize offset) {
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

vk::Sampler create_sampler(const Engine& engine, const SamplerConfig& config) {
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
        critical_assert(!"Invalid sampler type");
    }

    debug_assert(engine.device != nullptr);
    const auto sampler = engine.device.createSampler(sampler_info);
    critical_assert(sampler.result == vk::Result::eSuccess);
    return sampler.value;
}

static std::vector<char> read_shader(const std::filesystem::path path) {
    debug_assert(!path.empty());

    auto file = std::ifstream{path, std::ios::ate | std::ios::binary};
    critical_assert(file.is_open());

    const usize code_size = file.tellg();
    std::vector<char> code;
    code.resize(code_size);
    file.seekg(0);
    file.read(code.data(), static_cast<std::streamsize>(code.size()));
    file.close();

    critical_assert(!code.empty());
    return code;
}

vk::ShaderEXT create_unlinked_shader(const Engine& engine, const ShaderConfig& config) {
    debug_assert(engine.device != nullptr);
    debug_assert(!config.path.empty());
    debug_assert(config.code_type == vk::ShaderCodeTypeEXT::eSpirv && "binary shader code types untested");
    debug_assert(config.stage != vk::ShaderStageFlagBits{0});

    const auto code = read_shader(config.path);

    const auto shader = engine.device.createShaderEXT({
        .flags = config.flags,
        .stage = config.stage,
        .nextStage = config.next_stage,
        .codeType = config.code_type,
        .codeSize = code.size(),
        .pCode = code.data(),
        .pName = "main",
        .setLayoutCount = static_cast<u32>(config.set_layouts.size()),
        .pSetLayouts = config.set_layouts.data(),
        .pushConstantRangeCount = static_cast<u32>(config.push_ranges.size()),
        .pPushConstantRanges = config.push_ranges.data(),
    });
    critical_assert(shader.result == vk::Result::eSuccess);
    return shader.value;
}

void create_linked_shaders(const Engine& engine, const std::span<vk::ShaderEXT> out_shaders, const std::span<const ShaderConfig> configs) {
    debug_assert(engine.device != nullptr);
    debug_assert(configs.size() >= 2);

    std::vector<std::vector<char>> codes = {};
    codes.reserve(configs.size());
    for (const auto& config : configs) {
        debug_assert(!config.path.empty());
        debug_assert(config.code_type == vk::ShaderCodeTypeEXT::eSpirv && "binary shader code types untested");
        debug_assert(config.stage != vk::ShaderStageFlagBits{0});

        codes.push_back(read_shader(config.path));
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
            .setLayoutCount = static_cast<u32>(configs[i].set_layouts.size()),
            .pSetLayouts = configs[i].set_layouts.data(),
            .pushConstantRangeCount = static_cast<u32>(configs[i].push_ranges.size()),
            .pPushConstantRanges = configs[i].push_ranges.data(),
        });
    }

    const auto shader_result = engine.device.createShadersEXT(static_cast<u32>(shader_infos.size()), shader_infos.data(), nullptr, out_shaders.data());
    critical_assert(shader_result == vk::Result::eSuccess);
}

vk::CommandBuffer begin_single_time_commands(const Engine& engine) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.single_time_command_pool != nullptr);

    vk::CommandBufferAllocateInfo alloc_info = {
        .commandPool = engine.single_time_command_pool,
        .commandBufferCount = 1,
    };
    vk::CommandBuffer cmd = {};
    const auto cmd_result = engine.device.allocateCommandBuffers(&alloc_info, &cmd);
    critical_assert(cmd_result == vk::Result::eSuccess);

    const auto begin_result = cmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    critical_assert(begin_result == vk::Result::eSuccess);

    return cmd;
}

void end_single_time_commands(const Engine& engine, const vk::CommandBuffer cmd) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.single_time_command_pool != nullptr);

    const auto end_result = cmd.end();
    critical_assert(end_result == vk::Result::eSuccess);

    vk::SubmitInfo submit_info = {
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    const auto [fence_result, fence] = engine.device.createFence({});
    critical_assert(fence_result == vk::Result::eSuccess);
    defer(engine.device.destroyFence(fence));

    const auto submit_result = engine.queue.submit({submit_info}, fence);
    critical_assert(submit_result == vk::Result::eSuccess);

    const auto wait_result = engine.device.waitForFences({fence}, vk::True, UINT64_MAX);
    critical_assert(wait_result == vk::Result::eSuccess);

    engine.device.freeCommandBuffers(engine.single_time_command_pool, {cmd});
}

} // namespace hg
