#include "hg_rendering_engine.h"

namespace hg {

#ifdef NDEBUG
inline constexpr std::array<const char*, 0> ValidationLayers = {};
#else
inline constexpr std::array ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
#endif
constexpr std::array DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    // VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
};

static VkBool32 debug_callback(const VkDebugUtilsMessageSeverityFlagBitsEXT severity, const VkDebugUtilsMessageTypeFlagsEXT,
                               const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void*) {
    std::cout << std::format("{}\n", callback_data->pMessage);
    debug_assert(!(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT));
    return VK_FALSE;
}

constexpr VkDebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
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

static vk::DebugUtilsMessengerEXT init_debug_messenger(const Engine& engine) {
    debug_assert(engine.instance != nullptr);

    const PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(engine.instance, "vkCreateDebugUtilsMessengerEXT");
    critical_assert(vkCreateDebugUtilsMessengerEXT != nullptr);

    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    const auto messenger_result = vkCreateDebugUtilsMessengerEXT(engine.instance, &DebugUtilsMessengerCreateInfo, nullptr, &messenger);
    critical_assert(messenger_result == VK_SUCCESS);
    return messenger;
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

    vk::PhysicalDeviceBufferAddressFeaturesEXT buffer_address_feature = {.pNext = nullptr, .bufferDeviceAddress = vk::True};
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

    engine.instance = init_instance();
    debug_assert(engine.instance != nullptr);

    engine.debug_messenger = init_debug_messenger(engine);
    debug_assert(engine.debug_messenger != nullptr);

    engine.gpu = find_gpu(engine);
    critical_assert(engine.gpu != nullptr);

    engine.queue_family_index = *find_queue_family(engine.gpu);
    critical_assert(engine.queue_family_index != UINT32_MAX);

    engine.device = init_device(engine);
    critical_assert(engine.device != nullptr);

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

} // namespace hg
