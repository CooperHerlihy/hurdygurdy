#include "hg_vulkan_engine.h"

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

[[nodiscard]] static std::vector<const char*> get_required_instance_extensions() {
    u32 glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    critical_assert(glfw_extensions != nullptr);

    std::vector<const char*> required_extensions = {};
    required_extensions.reserve(static_cast<usize>(glfw_extension_count) + 1);
    for (usize i = 0; i < glfw_extension_count; ++i) {
        required_extensions.push_back(glfw_extensions[i]);
    }
#ifndef NDEBUG
    required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    return required_extensions;
}

[[nodiscard]] static bool check_instance_extension_availability(const std::span<const char* const> required_extensions) {
    debug_assert(!required_extensions.empty());

    const auto [extensions_result, extensions] = vk::enumerateInstanceExtensionProperties();
    critical_assert(extensions_result == vk::Result::eSuccess);

    return std::all_of(required_extensions.begin(), required_extensions.end(), [&](const char* required) {
        return std::any_of(extensions.begin(), extensions.end(), [&](const VkExtensionProperties& extension) { return strcmp(required, extension.extensionName) == 0; });
    });
}

static vk::Instance init_instance() {
    const vk::ApplicationInfo app_info = {
        .pApplicationName = "Hurdy Gurdy",
        .applicationVersion = 0,
        .pEngineName = "Hurdy Gurdy",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };

    const std::vector<const char*> required_extensions = get_required_instance_extensions();
    critical_assert(check_instance_extension_availability(required_extensions));

    const vk::InstanceCreateInfo instance_info = {
        .pNext = &DebugUtilsMessengerCreateInfo,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<u32>(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = static_cast<u32>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data(),
    };

    const auto instance = vk::createInstance(instance_info);
    critical_assert(instance.result == vk::Result::eSuccess);
    return instance.value;
}

static std::optional<u32> find_queue_family(const vk::PhysicalDevice gpu) {
    debug_assert(gpu != nullptr);

    const auto queue_families = gpu.getQueueFamilyProperties();
    const auto queue_family = std::find_if(queue_families.begin(), queue_families.end(),
                                           [](const vk::QueueFamilyProperties family) { return family.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute); });
    if (queue_family == queue_families.end()) {
        return std::nullopt;
    }
    return std::optional{static_cast<u32>(queue_family - queue_families.begin())};
}

static vk::PhysicalDevice find_gpu(const Engine& engine) {
    debug_assert(engine.instance != nullptr);

    const auto [gpus_result, gpus] = engine.instance.enumeratePhysicalDevices();
    critical_assert(gpus_result == vk::Result::eSuccess);

    const auto gpu = std::find_if(gpus.begin(), gpus.end(), [](const vk::PhysicalDevice gpu) {
        const auto features = gpu.getFeatures();
        if (features.sampleRateShading != vk::True || features.samplerAnisotropy != vk::True) {
            return false;
        }

        const auto [extension_result, extensions] = gpu.enumerateDeviceExtensionProperties();
        critical_assert(extension_result == vk::Result::eSuccess);
        if (!std::all_of(DeviceExtensions.begin(), DeviceExtensions.end(), [&](const char* required) {
                return std::any_of(extensions.begin(), extensions.end(), [&](const vk::ExtensionProperties extension) { return strcmp(required, extension.extensionName); });
            })) {
            return false;
        }

        if (!find_queue_family(gpu).has_value()) {
            return false;
        }

        return true;
    });
    critical_assert(gpu != gpus.end());
    return *gpu;
}

static vk::Device init_device(const Engine& engine) {
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

    vk::DeviceCreateInfo device_info = {
        .pNext = synchronization2_feature,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledLayerCount = static_cast<u32>(ValidationLayers.size()),
        .ppEnabledLayerNames = ValidationLayers.data(),
        .enabledExtensionCount = static_cast<u32>(DeviceExtensions.size()),
        .ppEnabledExtensionNames = DeviceExtensions.data(),
        .pEnabledFeatures = &features,
    };
    const auto device = engine.gpu.createDevice(device_info);
    critical_assert(device.result == vk::Result::eSuccess);
    return device.value;
}

static VmaAllocator init_allocator(const Engine& engine) {
    debug_assert(engine.instance != nullptr);
    debug_assert(engine.gpu != nullptr);
    debug_assert(engine.device != nullptr);

    VmaAllocatorCreateInfo info = {};
    info.physicalDevice = engine.gpu;
    info.device = engine.device;
    info.instance = engine.instance;
    info.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator allocator = nullptr;
    const auto result = vmaCreateAllocator(&info, &allocator);
    critical_assert(result == VK_SUCCESS);

    return allocator;
}

static bool s_engine_initialized = false;

Engine Engine::create() {
    debug_assert(s_engine_initialized == false);

    Engine engine = {};

    const auto glfw_success = glfwInit();
    critical_assert(glfw_success == GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    engine.instance = init_instance();
    debug_assert(engine.instance != nullptr);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(engine.instance);

    const auto messenger = engine.instance.createDebugUtilsMessengerEXT(DebugUtilsMessengerCreateInfo);
    critical_assert(messenger.result == vk::Result::eSuccess);
    engine.debug_messenger = messenger.value;

    engine.gpu = find_gpu(engine);
    critical_assert(engine.gpu != nullptr);

    engine.queue_family_index = *find_queue_family(engine.gpu);
    critical_assert(engine.queue_family_index != UINT32_MAX);

    engine.device = init_device(engine);
    critical_assert(engine.device != nullptr);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(engine.device);

    engine.queue = engine.device.getQueue(engine.queue_family_index, 0);
    critical_assert(engine.queue != nullptr);

    engine.allocator = init_allocator(engine);
    critical_assert(engine.allocator != nullptr);

    const auto pool = engine.device.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = engine.queue_family_index,
    });
    critical_assert(pool.result == vk::Result::eSuccess);
    engine.command_pool = pool.value;

    const auto transient_pool = engine.device.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = engine.queue_family_index,
    });
    critical_assert(transient_pool.result == vk::Result::eSuccess);
    engine.single_time_command_pool = transient_pool.value;

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

Window Window::create(const Engine& engine, i32 width, i32 height) {
    debug_assert(engine.device != nullptr);
    debug_assert(width > 0);
    debug_assert(height > 0);

    Window window = {};

    window.window = glfwCreateWindow(width, height, "Hurdy Gurdy", nullptr, nullptr);
    debug_assert(window.window != nullptr);

    debug_assert(engine.instance != nullptr);
    const auto surface_result = glfwCreateWindowSurface(engine.instance, window.window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&window.surface));
    critical_assert(surface_result == VK_SUCCESS);

    window.resize(engine);
    debug_assert(window.swapchain != nullptr);

    debug_assert(engine.command_pool != nullptr);
    const vk::CommandBufferAllocateInfo cmd_info{
        .commandPool = engine.command_pool,
        .commandBufferCount = MaxFramesInFlight,
    };
    const auto cmd_result = engine.device.allocateCommandBuffers(&cmd_info, window.m_command_buffers.data());
    critical_assert(cmd_result == vk::Result::eSuccess);

    for (auto& fence : window.m_frame_finished_fences) {
        const auto new_fence = engine.device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});
        critical_assert(new_fence.result == vk::Result::eSuccess);
        fence = new_fence.value;
    }
    for (auto& semaphore : window.m_image_available_semaphores) {
        const auto new_semaphore = engine.device.createSemaphore({});
        critical_assert(new_semaphore.result == vk::Result::eSuccess);
        semaphore = new_semaphore.value;
    }
    for (auto& semaphore : window.m_ready_to_present_semaphores) {
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

void Window::resize(const Engine& engine) {
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
        .presentMode = std::any_of(present_modes.begin(), present_modes.end(), [](const vk::PresentModeKHR mode) { return mode == vk::PresentModeKHR::eMailbox; })
                           ? vk::PresentModeKHR::eMailbox
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

    const auto cmd = begin_single_time_commands(engine);
    cmd.copyBuffer(staging_buffer.buffer, buffer, {vk::BufferCopy(offset, 0, size)});
    end_single_time_commands(engine, cmd);
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
        const auto cmd = begin_single_time_commands(engine);
        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, config.layout)
            .build_and_run();
        end_single_time_commands(engine, cmd);
    }

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

    const auto cmd = begin_single_time_commands(engine);

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
        .set_image_dst(vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead, final_layout)
        .build_and_run();

    end_single_time_commands(engine, cmd);
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

    const auto cmd = begin_single_time_commands(engine);

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

    end_single_time_commands(engine, cmd);
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

    const auto shader_result = engine.device.createShadersEXT(shader_infos.size(), shader_infos.data(), nullptr, out_shaders.data());
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
