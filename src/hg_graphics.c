#include "hg_graphics.h"

#include <vulkan/vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

static VkInstance s_instance;
static VkDebugUtilsMessengerEXT s_debug_messenger;

static VkPhysicalDevice s_gpu;
static VkDevice s_device;
static u32 s_queue_family_index;
static VkQueue s_queue;

static VmaAllocator s_allocator;
static VkCommandPool s_command_pool;

static SDL_Window* s_window;
static VkSurfaceKHR s_surface;

#define HG_SWAPCHAIN_MAX_IMAGES 3
#define HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT 2

static VkDescriptorPool s_descriptor_pools[HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT];

static VkSwapchainKHR s_swapchain;
static VkImage s_swapchain_images[HG_SWAPCHAIN_MAX_IMAGES];
static u32 s_swapchain_image_count;
static VkFormat s_swapchain_format;
static u32 s_swapchain_width;
static u32 s_swapchain_height;

static VkCommandBuffer s_command_buffers[HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT];
static VkFence s_frame_finished_fences[HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT];
static VkSemaphore s_image_available_semaphores[HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT];
static VkSemaphore s_ready_to_present_semaphores[HG_SWAPCHAIN_MAX_IMAGES];

static u32 s_current_image_index;
static u32 s_current_frame_index;
static bool s_recording_frame;

static HgTexture* s_previous_target;
static HgTexture* s_previous_depth_buffer;
static bool s_recording_pass;

static VkCommandBuffer s_current_cmd;
static HgShader* s_current_shader;

#ifndef NDEBUG
static const char* const ValidationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};
#endif

static const char* const DeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
};

#ifndef NDEBUG
static VkBool32 debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    (void)type;
    (void)user_data;

    if (severity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                 |  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
        printf("Vulkan Info: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        printf("Vulkan Warning: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        printf("Vulkan Error: %s\n", callback_data->pMessage);
    } else {
        printf("Vulkan Unknown: %s\n", callback_data->pMessage);
    }
    return VK_FALSE;
}

static const VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debug_callback,
};
#endif

static char const** get_instance_extensions(u32* extensions_count) {
    HG_ASSERT(extensions_count != NULL);
    *extensions_count = 0;

    u32 sdl_extensions_count = 0;
    char const* const* sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extensions_count);
    if (sdl_extensions == NULL)
        HG_ERRORF("Failed to get required instance extensions from SDL: %s", SDL_GetError());

#ifndef NDEBUG
    u32 required_extensions_count = sdl_extensions_count + 1;
    const char** required_extensions = hg_heap_alloc(required_extensions_count * sizeof(char*));
    memcpy(required_extensions, sdl_extensions, sdl_extensions_count * sizeof(char*));

    required_extensions[sdl_extensions_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#else
    u32 required_extensions_count = sdl_extensions_count;
    const char** required_extensions = hg_heap_alloc(required_extensions_count * sizeof(char*));
    memcpy(required_extensions, sdl_extensions, sdl_extensions_count * sizeof(char*));
#endif

    u32 extension_properties_count = 0;
    VkResult ext_count_res = vkEnumerateInstanceExtensionProperties(NULL, &extension_properties_count, NULL);
    switch (ext_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan incomplete instance extension enumeration");
            break;
        }
        case VK_ERROR_LAYER_NOT_PRESENT: HG_ERROR("Vulkan layer not present");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkExtensionProperties* extension_properties = hg_heap_alloc(
        extension_properties_count * sizeof(VkExtensionProperties)
    );

    VkResult ext_res = vkEnumerateInstanceExtensionProperties(NULL, &extension_properties_count, extension_properties);
    switch (ext_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan incomplete instance extension enumeration");
            break;
        }
        case VK_ERROR_LAYER_NOT_PRESENT: HG_ERROR("Vulkan layer not present");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    bool found_all = true;
    for (usize i = 0; i < required_extensions_count; ++i) {
        bool found = false;
        for (usize j = 0; j < extension_properties_count; j++) {
            if (strcmp(required_extensions[i], extension_properties[j].extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            found_all = false;
            break;
        }
    }
    if (!found_all)
        HG_ERROR("Vulkan instance missing required extensions");

    hg_heap_free(extension_properties, extension_properties_count * sizeof(VkExtensionProperties));

    *extensions_count = required_extensions_count;
    return required_extensions;
}

static VkInstance hg_create_instance(void) {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hurdy Gurdy",
        .applicationVersion = 0,
        .pEngineName = "Hurdy Gurdy",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };

    u32 extensions_count = 0;
    char const** extensions = get_instance_extensions(&extensions_count);

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifndef NDEBUG
        .pNext = &debug_utils_messenger_create_info,
#endif
        .pApplicationInfo = &app_info,
#ifndef NDEBUG
        .enabledLayerCount = HG_ARRAY_SIZE(ValidationLayers),
        .ppEnabledLayerNames = ValidationLayers,
#endif
        .enabledExtensionCount = extensions_count,
        .ppEnabledExtensionNames = extensions,
    };

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&instance_info, NULL, &instance);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_LAYER_NOT_PRESENT: HG_ERROR("Required Vulkan layer not present");
        case VK_ERROR_EXTENSION_NOT_PRESENT: HG_ERROR("Required Vulkan extension not present");
        case VK_ERROR_INCOMPATIBLE_DRIVER: HG_ERROR("Incompatible Vulkan driver");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan initialization failed");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    hg_heap_free(extensions, extensions_count * sizeof(char*));
    return instance;
}

static VkDebugUtilsMessengerEXT create_debug_messenger(void) {
#ifdef NDEBUG
    return VK_NULL_HANDLE;
#else
    HG_ASSERT(s_instance != VK_NULL_HANDLE);

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_instance, "vkCreateDebugUtilsMessengerEXT");

    VkDebugUtilsMessengerEXT messenger = NULL;
    VkResult result = vkCreateDebugUtilsMessengerEXT(
        s_instance,
        &debug_utils_messenger_create_info,
        NULL,
        &messenger
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return messenger;
#endif
}

static bool find_queue_family(VkPhysicalDevice gpu, u32* queue_family) {
    HG_ASSERT(queue_family != NULL);
    HG_ASSERT(gpu != NULL);

    *queue_family = UINT32_MAX;

    u32 queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_families_count, NULL);
    if (queue_families_count == 0)
        return false;

    VkQueueFamilyProperties* queue_families = hg_heap_alloc(queue_families_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_families_count, queue_families);

    for (u32 i = 0; i < queue_families_count; ++i) {
        if (queue_families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
            *queue_family = i;
            break;
        }
    }
    hg_heap_free(queue_families, queue_families_count * sizeof(VkQueueFamilyProperties));
    return *queue_family != UINT32_MAX;
}

static VkPhysicalDevice* get_gpus(u32* count) {
    HG_ASSERT(s_instance != VK_NULL_HANDLE);
    HG_ASSERT(count != NULL);

    u32 gpu_count = 0;
    VkResult gpu_count_res = vkEnumeratePhysicalDevices(s_instance, &gpu_count, NULL);
    switch (gpu_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan incomplete gpu enumeration");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan initialization failed");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkPhysicalDevice* gpus = hg_heap_alloc(gpu_count * sizeof(VkPhysicalDevice));
    VkResult gpu_result = vkEnumeratePhysicalDevices(s_instance, &gpu_count, gpus);
    switch (gpu_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan incomplete gpu enumeration");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan initialization failed");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    *count = gpu_count;
    return gpus;
}

static VkPhysicalDevice find_gpu(void) {
    HG_ASSERT(s_instance != NULL);

    u32 gpu_count = 0;
    VkPhysicalDevice* gpus = get_gpus(&gpu_count);
    if (gpu_count == 0)
        return VK_NULL_HANDLE;

    u32 extension_properties_count = 0;
    VkExtensionProperties* extension_properties = NULL;;

    VkPhysicalDevice suitable_gpu = VK_NULL_HANDLE;
    for (u32 i = 0; i < gpu_count; ++i) {
        VkPhysicalDevice gpu = gpus[i];

        VkPhysicalDeviceFeatures features = {0};
        vkGetPhysicalDeviceFeatures(gpu, &features);
        if (features.sampleRateShading != VK_TRUE || features.samplerAnisotropy != VK_TRUE)
            continue;

        u32 props_count = 0;
        VkResult ext_count_res = vkEnumerateDeviceExtensionProperties(gpu, NULL, &props_count, NULL);
        switch (ext_count_res) {
            case VK_SUCCESS: break;
            case VK_INCOMPLETE: {
                continue;
            }
            case VK_ERROR_LAYER_NOT_PRESENT: {
                continue;
            }
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            default: HG_ERROR("Unexpected Vulkan error");
        }

        if (props_count > extension_properties_count) {
            hg_heap_free(extension_properties, extension_properties_count * sizeof(VkExtensionProperties));
            extension_properties_count = props_count;
            extension_properties = hg_heap_alloc(extension_properties_count * sizeof(VkExtensionProperties));
        }

        VkResult ext_res = vkEnumerateDeviceExtensionProperties(gpu, NULL, &props_count, extension_properties);
        switch (ext_res) {
            case VK_SUCCESS: break;
            case VK_INCOMPLETE: {
                HG_WARN("Vulkan incomplete gpu extension enumeration");
                continue;
            }
            case VK_ERROR_LAYER_NOT_PRESENT: {
                HG_WARN("Vulkan layer not present");
                continue;
            }
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            default: HG_ERROR("Unexpected Vulkan error");
        }

        bool found_all_extensions = true;
        for (usize j = 0; j < HG_ARRAY_SIZE(DeviceExtensions); j++) {
            bool found_extension = false;
            for (usize k = 0; k < props_count; k++) {
                if (strcmp(DeviceExtensions[j], extension_properties[k].extensionName) == 0) {
                    found_extension = true;
                    break;
                }
            }
            if (!found_extension) {
                found_all_extensions = false;
                break;
            }
        }
        if (!found_all_extensions)
            continue;

        if (!find_queue_family(gpu, &s_queue_family_index))
            continue;

        suitable_gpu = gpu;
        break;
    }

    hg_heap_free(extension_properties, extension_properties_count * sizeof(VkExtensionProperties));
    hg_heap_free(gpus, gpu_count * sizeof(VkPhysicalDevice));
    return suitable_gpu;
}

static VkDevice create_device(void) {
    HG_ASSERT(s_gpu != NULL);
    HG_ASSERT(s_queue_family_index != UINT32_MAX);

    VkPhysicalDeviceBufferAddressFeaturesEXT buffer_address_feature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_ADDRESS_FEATURES_EXT,
        .pNext = NULL,
        .bufferDeviceAddress = VK_TRUE,
        .bufferDeviceAddressCaptureReplay = VK_FALSE,
        .bufferDeviceAddressMultiDevice = VK_FALSE,
    };
    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext = &buffer_address_feature,
        .dynamicRendering = VK_TRUE,
    };
    VkPhysicalDeviceSynchronization2Features synchronization2_feature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = &dynamic_rendering_feature,
        .synchronization2 = VK_TRUE,
    };
    VkPhysicalDeviceFeatures features = {
        .sampleRateShading = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = s_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &synchronization2_feature,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
#ifndef NDEBUG
        .enabledLayerCount = HG_ARRAY_SIZE(ValidationLayers),
        .ppEnabledLayerNames = ValidationLayers,
#endif
        .enabledExtensionCount = HG_ARRAY_SIZE(DeviceExtensions),
        .ppEnabledExtensionNames = DeviceExtensions,
        .pEnabledFeatures = &features,
    };

    VkDevice device = NULL;
    VkResult result = vkCreateDevice(s_gpu, &device_info, NULL, &device);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_EXTENSION_NOT_PRESENT: HG_ERROR("Required Vulkan extension not present");
        case VK_ERROR_FEATURE_NOT_PRESENT: HG_ERROR("Required Vulkan feature not present");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan initialization failed");
        case VK_ERROR_TOO_MANY_OBJECTS: HG_ERROR("Vulkan too many objects");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    return device;
}

static VmaAllocator create_gpu_allocator(void) {
    HG_ASSERT(s_instance != NULL);
    HG_ASSERT(s_gpu != NULL);
    HG_ASSERT(s_device != NULL);

    VmaAllocatorCreateInfo info = {
        .physicalDevice = s_gpu,
        .device = s_device,
        .instance = s_instance,
        .vulkanApiVersion = VK_API_VERSION_1_3,
    };
    VmaAllocator allocator = NULL;
    VkResult result = vmaCreateAllocator(&info, &allocator);
    if (result != VK_SUCCESS)
        HG_ERROR("Could not create Vma allocator");
    return allocator;
}

static VkCommandPool create_command_pool(void) {
    HG_ASSERT(s_device != NULL);
    HG_ASSERT(s_queue_family_index != UINT32_MAX);

    VkCommandPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = s_queue_family_index,
    };
    VkCommandPool pool = NULL;
    VkResult result = vkCreateCommandPool(s_device, &info, NULL, &pool);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan failed to initialize");
        default: HG_ERROR("Vulkan failed to create command pool");
    }
    return pool;
}

// Create properly dynamic descriptor pool : TODO
static VkDescriptorPool hg_create_descriptor_pool(void) {
    HG_ASSERT(s_device != VK_NULL_HANDLE);

    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 512 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 512 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 512 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 512 },
    };
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .maxSets = 1024,
        .poolSizeCount = HG_ARRAY_SIZE(pool_sizes),
        .pPoolSizes = pool_sizes
    };
    VkDescriptorPool pool;
    VkResult result = vkCreateDescriptorPool(s_device, &pool_info, NULL, &pool);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_FRAGMENTATION_EXT: HG_ERROR("Vulkan fragmentation error");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED_EXT: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Vulkan failed to create descriptor pool");
    }
    return pool;
}

void hg_graphics_init(void) {
    s_instance = hg_create_instance();
    s_debug_messenger = create_debug_messenger();

    s_gpu = find_gpu();
    s_device = create_device();

    vkGetDeviceQueue(s_device, s_queue_family_index, 0, &s_queue);
    if (s_queue == VK_NULL_HANDLE)
        HG_ERROR("Vulkan could not get queue");

    s_allocator = create_gpu_allocator();
    s_command_pool = create_command_pool();
    for (usize i = 0; i < HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT; ++i) {
        s_descriptor_pools[i] = hg_create_descriptor_pool();
    }

    s_current_cmd = VK_NULL_HANDLE;
}

void hg_graphics_shutdown(void) {
    HG_ASSERT(s_instance != VK_NULL_HANDLE);
    HG_ASSERT(s_debug_messenger != VK_NULL_HANDLE);
    HG_ASSERT(s_device != VK_NULL_HANDLE);
    HG_ASSERT(s_queue != VK_NULL_HANDLE);
    HG_ASSERT(s_allocator != NULL);
    HG_ASSERT(s_command_pool != VK_NULL_HANDLE);

    for (usize i = 0; i < HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT; i++) {
        HG_ASSERT(s_descriptor_pools[i] != VK_NULL_HANDLE);
        vkDestroyDescriptorPool(s_device, s_descriptor_pools[i], NULL);
    }
    vkDestroyCommandPool(s_device, s_command_pool, NULL);
    vmaDestroyAllocator(s_allocator);
    vkDestroyDevice(s_device, NULL);

#ifndef NDEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkDestroyDebugUtilsMessengerEXT == NULL)
        HG_ERROR("vkDestroyDebugUtilsMessengerEXT pfn not found");
    vkDestroyDebugUtilsMessengerEXT(s_instance, s_debug_messenger, NULL);
#endif

    vkDestroyInstance(s_instance, NULL);
}

void hg_graphics_wait(void) {
    HG_ASSERT(s_queue != VK_NULL_HANDLE);

    VkResult wait_result = vkQueueWaitIdle(s_queue);
    switch (wait_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }
}

SDL_Window* hg_create_sdl_window(const char* title, int width, int height, bool windowed) {
    if (windowed)
        HG_ASSERT(width > 0 && height > 0);

    SDL_Window* window;

    if (windowed) {
        window = SDL_CreateWindow(
            title,
            width, height,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
        );
    } else {
        SDL_DisplayID display = SDL_GetPrimaryDisplay();
        if (display == 0)
            HG_ERRORF("Could not get primary display: %s", SDL_GetError());

        int count = 0;
        SDL_DisplayMode** mode = SDL_GetFullscreenDisplayModes(display, &count);
        if (mode == NULL)
            HG_ERRORF("Could not get display modes: %s", SDL_GetError());
        if (count == 0)
            HG_ERROR("No fullscreen modes available");

        window = SDL_CreateWindow(
            title,
            mode[0]->w, mode[0]->h,
            SDL_WINDOW_FULLSCREEN | SDL_WINDOW_VULKAN
        );
    }
    if (window == NULL)
        HG_ERRORF("Could not create window: %s", SDL_GetError());

    return window;
}

static VkFormat hg_find_swapchain_format(VkSurfaceKHR surface) {
    u32 formats_count = 0;
    VkResult formats_count_res = vkGetPhysicalDeviceSurfaceFormatsKHR(s_gpu, surface, &formats_count, NULL);
    switch (formats_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get swapchain formats incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        case VK_ERROR_VALIDATION_FAILED_EXT: HG_ERROR("Vulkan validation failed");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    if (formats_count == 0)
        HG_ERROR("No swapchain formats available");

    VkSurfaceFormatKHR* formats = hg_heap_alloc(formats_count * sizeof(VkSurfaceFormatKHR));

    VkResult format_result = vkGetPhysicalDeviceSurfaceFormatsKHR(s_gpu, surface, &formats_count, formats);
    switch (format_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get swapchain formats incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        case VK_ERROR_VALIDATION_FAILED_EXT: HG_ERROR("Vulkan validation failed");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    VkFormat format = VK_FORMAT_UNDEFINED;
    for (usize i = 0; i < formats_count; ++i) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) {
            format = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        }
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
            format = VK_FORMAT_B8G8R8A8_SRGB;
            break;
        }
    }
    if (format == VK_FORMAT_UNDEFINED)
        HG_ERROR("No supported swapchain formats");

    hg_heap_free(formats, formats_count * sizeof(VkSurfaceFormatKHR));
    return format;
}

static VkPresentModeKHR hg_get_swapchain_present_mode(VkSurfaceKHR surface) {
    u32 present_mode_count = 0;
    VkResult present_count_res = vkGetPhysicalDeviceSurfacePresentModesKHR(
        s_gpu, surface, &present_mode_count, NULL
    );
    switch (present_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get present modes incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkPresentModeKHR* present_modes = hg_heap_alloc(present_mode_count * sizeof(VkPresentModeKHR));

    VkResult present_res = vkGetPhysicalDeviceSurfacePresentModesKHR(
        s_gpu, surface, &present_mode_count, present_modes
    );
    switch (present_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get present modes incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (usize i = 0; i < present_mode_count; ++i) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    hg_heap_free(present_modes, present_mode_count * sizeof(VkPresentModeKHR));
    return present_mode;
}

static VkSwapchainKHR hg_create_new_swapchain(
        VkSwapchainKHR old_swapchain,
        VkSurfaceKHR surface,
        VkFormat format,
        u32* width,
        u32* height
    ) {
    HG_ASSERT(surface != VK_NULL_HANDLE);
    HG_ASSERT(format != VK_FORMAT_UNDEFINED);

    VkPresentModeKHR present_mode = hg_get_swapchain_present_mode(surface);

    VkSurfaceCapabilitiesKHR surface_capabilities = {0};
    VkResult surface_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_gpu, surface, &surface_capabilities);
    switch (surface_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    if (surface_capabilities.currentExtent.width == 0 || surface_capabilities.currentExtent.height == 0)
        return VK_NULL_HANDLE;
    if (surface_capabilities.currentExtent.width < surface_capabilities.minImageExtent.width)
        return VK_NULL_HANDLE;
    if (surface_capabilities.currentExtent.height < surface_capabilities.minImageExtent.height)
        return VK_NULL_HANDLE;
    if (surface_capabilities.currentExtent.width > surface_capabilities.maxImageExtent.width)
        return VK_NULL_HANDLE;
    if (surface_capabilities.currentExtent.height > surface_capabilities.maxImageExtent.height)
        return VK_NULL_HANDLE;
    *width = surface_capabilities.currentExtent.width;
    *height = surface_capabilities.currentExtent.height;

    VkSwapchainCreateInfoKHR new_swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surface_capabilities.maxImageCount == 0
                       ? HG_SWAPCHAIN_MAX_IMAGES
                       : HG_MIN(HG_MIN(
                             surface_capabilities.minImageCount + 1,
                             surface_capabilities.maxImageCount),
                             HG_SWAPCHAIN_MAX_IMAGES),
        .imageFormat = format,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = surface_capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain,
    };

    VkSwapchainKHR new_swapchain = NULL;
    VkResult swapchain_result = vkCreateSwapchainKHR(s_device, &new_swapchain_info, NULL, &new_swapchain);
    switch (swapchain_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: HG_ERROR("Vulkan native window in use");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan initialization failed");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: HG_ERROR("Vulkan compression exhausted");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return new_swapchain;
}

static void hg_get_swapchain_images(VkSwapchainKHR swapchain, VkImage* images, u32* image_count) {
    VkResult image_count_result = vkGetSwapchainImagesKHR(s_device, swapchain, image_count, NULL);
    switch (image_count_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get swapchain images incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkResult image_result = vkGetSwapchainImagesKHR(s_device, swapchain, image_count, images);
    switch (image_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get swapchain images incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }
}

static void hg_allocate_command_buffers(VkCommandBuffer* command_buffers, u32 command_buffer_count) {
    HG_ASSERT(command_buffers != NULL);
    HG_ASSERT(command_buffer_count != 0);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = s_command_pool,
        .commandBufferCount = command_buffer_count,
    };
    const VkResult result = vkAllocateCommandBuffers(s_device, &alloc_info, command_buffers);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }
}

static VkFence hg_create_fence(VkFenceCreateFlags flags) {
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = flags,
    };
    VkFence fence = VK_NULL_HANDLE;
    const VkResult result = vkCreateFence(s_device, &fence_info, NULL, &fence);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    return fence;
}

static VkSemaphore hg_create_semaphore(void) {
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkSemaphore semaphore = VK_NULL_HANDLE;
    const VkResult result = vkCreateSemaphore(s_device, &semaphore_info, NULL, &semaphore);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    return semaphore;
}

void hg_window_open(const HgWindowConfig* config) {
    HG_ASSERT(s_window == NULL);
    HG_ASSERT(s_surface == VK_NULL_HANDLE);

    HG_ASSERT(s_swapchain == VK_NULL_HANDLE);
    for (usize i = 0; i < HG_SWAPCHAIN_MAX_IMAGES; ++i) {
        HG_ASSERT(s_swapchain_images[i] == VK_NULL_HANDLE);
    }
    HG_ASSERT(s_swapchain_image_count == 0);
    HG_ASSERT(s_swapchain_format == VK_FORMAT_UNDEFINED);
    HG_ASSERT(s_swapchain_width == 0);
    HG_ASSERT(s_swapchain_height == 0);

    for (usize i = 0; i < HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT; ++i) {
        HG_ASSERT(s_command_buffers[i] == VK_NULL_HANDLE);
        HG_ASSERT(s_frame_finished_fences[i] == VK_NULL_HANDLE);
        HG_ASSERT(s_image_available_semaphores[i] == VK_NULL_HANDLE);
    }
    for (usize i = 0; i < HG_SWAPCHAIN_MAX_IMAGES; ++i) {
        HG_ASSERT(s_ready_to_present_semaphores[i] == VK_NULL_HANDLE);
    }

    s_window = hg_create_sdl_window(
        config->title,
        (i32)config->width,
        (i32)config->height,
        config->windowed
    );

    bool sdl_success = SDL_Vulkan_CreateSurface(s_window, s_instance, NULL, &s_surface);
    if (!sdl_success)
        HG_ERRORF("Could not create Vulkan surface: %s", SDL_GetError());

    s_swapchain_format = hg_find_swapchain_format(s_surface);

    s_swapchain = hg_create_new_swapchain(
        NULL,
        s_surface,
        s_swapchain_format,
        &s_swapchain_width,
        &s_swapchain_height
    );
    if (s_swapchain == VK_NULL_HANDLE)
        HG_ERROR("Could not create new swapchain");

    hg_get_swapchain_images(s_swapchain, s_swapchain_images, &s_swapchain_image_count);

    hg_allocate_command_buffers(s_command_buffers, HG_ARRAY_SIZE(s_command_buffers));

    for (usize i = 0; i < HG_ARRAY_SIZE(s_frame_finished_fences); ++i) {
        s_frame_finished_fences[i] = hg_create_fence(VK_FENCE_CREATE_SIGNALED_BIT);
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(s_image_available_semaphores); ++i) {
        s_image_available_semaphores[i] = hg_create_semaphore();
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(s_ready_to_present_semaphores); ++i) {
        s_ready_to_present_semaphores[i] = hg_create_semaphore();
    }

    s_current_image_index = 0;
    s_current_frame_index = 0;
    s_recording_frame = false;

    s_previous_target = NULL;
    s_previous_depth_buffer = NULL;
    s_recording_pass = false;

    s_current_shader = NULL;
}

void hg_window_close(void) {
    HG_ASSERT(s_window != NULL);
    HG_ASSERT(s_surface != VK_NULL_HANDLE);
    HG_ASSERT(s_swapchain != VK_NULL_HANDLE);
    HG_ASSERT(!s_recording_frame);

    for (usize i = 0; i < HG_ARRAY_SIZE(s_image_available_semaphores); ++i) {
        HG_ASSERT(s_image_available_semaphores[i] != VK_NULL_HANDLE);
        vkDestroySemaphore(s_device, s_image_available_semaphores[i], NULL);
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(s_ready_to_present_semaphores); ++i) {
        HG_ASSERT(s_ready_to_present_semaphores[i] != VK_NULL_HANDLE);
        vkDestroySemaphore(s_device, s_ready_to_present_semaphores[i], NULL);
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(s_frame_finished_fences); ++i) {
        HG_ASSERT(s_frame_finished_fences[i] != VK_NULL_HANDLE);
        vkDestroyFence(s_device, s_frame_finished_fences[i], NULL);
    }

    for (usize i = 0; i < HG_ARRAY_SIZE(s_command_buffers); ++i) {
        HG_ASSERT(s_command_buffers[i] != VK_NULL_HANDLE);
    }
    vkFreeCommandBuffers(s_device, s_command_pool, HG_ARRAY_SIZE(s_command_buffers), s_command_buffers);

    vkDestroySwapchainKHR(s_device, s_swapchain, NULL);
    vkDestroySurfaceKHR(s_instance, s_surface, NULL);
    SDL_DestroyWindow(s_window);

#ifndef NDEBUG
    s_window = NULL;
    s_surface = VK_NULL_HANDLE;

    s_swapchain = VK_NULL_HANDLE;
    for (usize i = 0; i < HG_SWAPCHAIN_MAX_IMAGES; ++i) {
        s_swapchain_images[i] = VK_NULL_HANDLE;
    }
    s_swapchain_image_count = 0;
    s_swapchain_format = VK_FORMAT_UNDEFINED;

    for (usize i = 0; i < HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT; ++i) {
        s_command_buffers[i] = VK_NULL_HANDLE;
        s_frame_finished_fences[i] = VK_NULL_HANDLE;
        s_image_available_semaphores[i] = VK_NULL_HANDLE;
    }
    for (usize i = 0; i < HG_SWAPCHAIN_MAX_IMAGES; ++i) {
        s_ready_to_present_semaphores[i] = VK_NULL_HANDLE;
    }

    s_current_image_index = 0;
    s_current_frame_index = 0;
#endif

    s_swapchain_width = 0;
    s_swapchain_height = 0;
}

void hg_window_update_size(void) {
    HG_ASSERT(s_window != NULL);

    VkSwapchainKHR new_swapchain = hg_create_new_swapchain(
        s_swapchain,
        s_surface,
        s_swapchain_format,
        &s_swapchain_width,
        &s_swapchain_height
    );
    if (new_swapchain == VK_NULL_HANDLE)
        HG_ERROR("Could not create new swapchain");

    hg_graphics_wait();

    vkDestroySwapchainKHR(s_device, s_swapchain, NULL);
    s_swapchain = new_swapchain;

    hg_get_swapchain_images(s_swapchain, s_swapchain_images, &s_swapchain_image_count);

    for (usize i = 0; i < HG_ARRAY_SIZE(s_image_available_semaphores); ++i) {
        vkDestroySemaphore(s_device, s_image_available_semaphores[i], NULL);
        s_image_available_semaphores[i] = hg_create_semaphore();
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(s_ready_to_present_semaphores); ++i) {
        vkDestroySemaphore(s_device, s_ready_to_present_semaphores[i], NULL);
        s_ready_to_present_semaphores[i] = hg_create_semaphore();
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(s_frame_finished_fences); ++i) {
        vkDestroyFence(s_device, s_frame_finished_fences[i], NULL);
        s_frame_finished_fences[i] = hg_create_fence(VK_FENCE_CREATE_SIGNALED_BIT);
    }

    vkFreeCommandBuffers(s_device, s_command_pool, HG_ARRAY_SIZE(s_command_buffers), s_command_buffers);
    hg_allocate_command_buffers(s_command_buffers, HG_ARRAY_SIZE(s_command_buffers));
}

void hg_window_get_size(u32* width, u32* height) {
    HG_ASSERT(width != NULL);
    HG_ASSERT(height != NULL);
    *width = s_swapchain_width;
    *height = s_swapchain_height;
}

static VkCommandBuffer hg_begin_single_time_cmd(void) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = s_command_pool,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd = NULL;
    VkResult cmd_result = vkAllocateCommandBuffers(s_device, &alloc_info, &cmd);
    switch (cmd_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkResult begin_result = vkBeginCommandBuffer(cmd, &begin_info);
    switch (begin_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    HG_ASSERT(cmd != NULL);
    return cmd;
}

static void hg_wait_for_fence(VkFence fence) {
    HG_ASSERT(fence != VK_NULL_HANDLE);

    VkResult result = vkWaitForFences(s_device, 1, &fence, VK_TRUE, UINT64_MAX);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_TIMEOUT: HG_ERROR("Vulkan timed out waiting for fence");
        case VK_NOT_READY: HG_ERROR("Vulkan not ready");
        case VK_SUBOPTIMAL_KHR: HG_ERROR("Vulkan suboptimal");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }
}

static void hg_end_single_time_cmd(VkCommandBuffer cmd) {
    HG_ASSERT(cmd != NULL);

    VkResult end_result = vkEndCommandBuffer(cmd);
    switch (end_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: HG_ERROR("Vulkan invalid video parameters");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkFence fence = hg_create_fence(0);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    VkResult submit_result = vkQueueSubmit(s_queue, 1, &submit_info, fence);
    switch (submit_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    hg_wait_for_fence(fence);
    vkDestroyFence(s_device, fence, NULL);

    vkFreeCommandBuffers(s_device, s_command_pool, 1, &cmd);
}

typedef struct HgBuffer {
    VmaAllocation allocation;
    VkBuffer handle;
    usize size;
    HgGpuMemoryType memory_type;
} HgBuffer;

static VkBufferUsageFlags hg_buffer_usage_flags_to_vk(HgBufferUsageFlags usage) {
    return (usage & HG_BUFFER_USAGE_READ_WRITE_SRC_BIT ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0)
         | (usage & HG_BUFFER_USAGE_READ_WRITE_DST_BIT ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0)
         | (usage & HG_BUFFER_USAGE_UNIFORM_BUFFER_BIT ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0)
         | (usage & HG_BUFFER_USAGE_STORAGE_BUFFER_BIT ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0)
         | (usage & HG_BUFFER_USAGE_VERTEX_BUFFER_BIT ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0)
         | (usage & HG_BUFFER_USAGE_INDEX_BUFFER_BIT ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0);
}

HgBuffer* hg_buffer_create(const HgBufferConfig* config) {
    HG_ASSERT(config->size != 0);
    HG_ASSERT(config->usage != HG_BUFFER_USAGE_NONE);

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = config->size,
        .usage = hg_buffer_usage_flags_to_vk(config->usage),
    };

    VmaAllocationCreateInfo alloc_info = {0};
    if (config->memory_type == HG_GPU_MEMORY_TYPE_DEVICE_LOCAL) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_info.flags = 0;
    } else if (config->memory_type == HG_GPU_MEMORY_TYPE_LINEAR_ACCESS) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    } else if (config->memory_type == HG_GPU_MEMORY_TYPE_RANDOM_ACCESS) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    } else {
        HG_ERROR("Invalid buffer memory type");
    }

    HgBuffer* buffer = hg_heap_alloc(sizeof(HgBuffer));
    *buffer = (HgBuffer){
        .size = config->size,
        .memory_type = config->memory_type,
    };
    VkResult buffer_result = vmaCreateBuffer(
        s_allocator,
        &buffer_info,
        &alloc_info,
        &buffer->handle,
        &buffer->allocation,
        NULL
    );
    switch (buffer_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: HG_ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: HG_ERROR("Vulkan invalid opaque capture address");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return buffer;
}

void hg_buffer_destroy(HgBuffer* buffer) {
    HG_ASSERT(buffer != NULL);
    HG_ASSERT(buffer->handle != VK_NULL_HANDLE);
    HG_ASSERT(buffer->allocation != VK_NULL_HANDLE);

    vmaDestroyBuffer(s_allocator, buffer->handle, buffer->allocation);
    hg_heap_free(buffer, sizeof(HgBuffer));
}

void hg_buffer_write(HgBuffer* dst, usize offset, const void* src, usize size) {
    HG_ASSERT(dst->allocation != NULL);
    HG_ASSERT(dst->handle != NULL);
    HG_ASSERT(src != NULL);
    HG_ASSERT(size != 0);
    if (dst->memory_type == HG_GPU_MEMORY_TYPE_LINEAR_ACCESS)
        HG_ASSERT(offset == 0);

    if (dst->memory_type == HG_GPU_MEMORY_TYPE_RANDOM_ACCESS|| dst->memory_type == HG_GPU_MEMORY_TYPE_LINEAR_ACCESS) {
        VkResult copy_result = vmaCopyMemoryToAllocation(s_allocator, src, dst->allocation, offset, size);
        switch (copy_result) {
            case VK_SUCCESS: return;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_MEMORY_MAP_FAILED: HG_ERROR("Vulkan memory map failed");
            default: HG_ERROR("Unexpected Vulkan error");
        }
    }
    HG_ASSERT(dst->memory_type == HG_GPU_MEMORY_TYPE_DEVICE_LOCAL);

    HgBuffer* staging_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = size,
        .usage = HG_BUFFER_USAGE_READ_WRITE_SRC_BIT,
        .memory_type = HG_GPU_MEMORY_TYPE_LINEAR_ACCESS
    });
    hg_buffer_write(staging_buffer, 0, src, size);

    VkCommandBuffer cmd = hg_begin_single_time_cmd();
    vkCmdCopyBuffer(cmd, staging_buffer->handle, dst->handle, 1, &(VkBufferCopy){0, offset, size});
    hg_end_single_time_cmd(cmd);

    hg_buffer_destroy(staging_buffer);
}

void hg_buffer_read(void* dst, usize size, const HgBuffer* src, usize offset) {
    HG_ASSERT(dst != NULL);
    HG_ASSERT(src != NULL);
    HG_ASSERT(src->allocation != NULL);
    HG_ASSERT(src->handle != NULL);
    if (src->memory_type == HG_GPU_MEMORY_TYPE_LINEAR_ACCESS)
        HG_ASSERT(offset == 0);

    if (size == 0)
        return;
    if (size == SIZE_MAX)
        size = src->size - offset;

    if (src->memory_type == HG_GPU_MEMORY_TYPE_RANDOM_ACCESS|| src->memory_type == HG_GPU_MEMORY_TYPE_LINEAR_ACCESS) {
        VkResult copy_result = vmaCopyAllocationToMemory(s_allocator, src->allocation, offset, dst, size);
        switch (copy_result) {
            case VK_SUCCESS: return;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_MEMORY_MAP_FAILED: HG_ERROR("Vulkan memory map failed");
            default: HG_ERROR("Unexpected Vulkan error");
        }
    }
    HG_ASSERT(src->memory_type == HG_GPU_MEMORY_TYPE_DEVICE_LOCAL);

    HgBuffer* staging_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = size,
        .usage = HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
        .memory_type = HG_GPU_MEMORY_TYPE_LINEAR_ACCESS
    });

    VkCommandBuffer cmd = hg_begin_single_time_cmd();
    vkCmdCopyBuffer(cmd, src->handle, staging_buffer->handle, 1, &(VkBufferCopy){offset, 0, size});
    hg_end_single_time_cmd(cmd);

    hg_buffer_read(dst, size, staging_buffer, 0);

    hg_buffer_destroy(staging_buffer);
}

typedef struct HgTexture {
    VmaAllocation allocation;
    VkImage handle;
    VkImageView view;
    VkSampler sampler;
    VkImageLayout layout;
    VkImageAspectFlags aspect;
    VkFormat format;
    u32 width;
    u32 height;
    u32 depth;
    u32 array_layers;
    u32 mip_levels;
    bool is_cubemap;
} HgTexture;

static VkFormat hg_format_to_vk(HgFormat format) {
    return (VkFormat)format;
    // const VkFormat texture_formats[] = {
    //     ...
    // };
    // return texture_formats[format];
}

static HgFormat hg_format_from_vk(VkFormat format) {
    return (HgFormat)format;
    // const VkFormat texture_formats[] = {
    //     ...
    // };
    // return texture_formats[format];
}

static u32 hg_format_size(HgFormat format) {
    static const u32 texture_format_sizes[] = {
        [HG_FORMAT_UNDEFINED] = 0,
        [HG_FORMAT_R4G4_UNORM_PACK8 ] = 1,
        [HG_FORMAT_R4G4B4A4_UNORM_PACK16] = 2,
        [HG_FORMAT_B4G4R4A4_UNORM_PACK16] = 2,
        [HG_FORMAT_R5G6B5_UNORM_PACK16] = 2,
        [HG_FORMAT_B5G6R5_UNORM_PACK16] = 2,
        [HG_FORMAT_R5G5B5A1_UNORM_PACK16] = 2,
        [HG_FORMAT_B5G5R5A1_UNORM_PACK16] = 2,
        [HG_FORMAT_A1R5G5B5_UNORM_PACK16] = 2,
        [HG_FORMAT_R8_UNORM] = 1,
        [HG_FORMAT_R8_SNORM] = 1,
        [HG_FORMAT_R8_USCALED] = 1,
        [HG_FORMAT_R8_SSCALED] = 1,
        [HG_FORMAT_R8_UINT] = 1,
        [HG_FORMAT_R8_SINT] = 1,
        [HG_FORMAT_R8_SRGB] = 1,
        [HG_FORMAT_R8G8_UNORM] = 2,
        [HG_FORMAT_R8G8_SNORM] = 2,
        [HG_FORMAT_R8G8_USCALED] = 2,
        [HG_FORMAT_R8G8_SSCALED] = 2,
        [HG_FORMAT_R8G8_UINT] = 2,
        [HG_FORMAT_R8G8_SINT] = 2,
        [HG_FORMAT_R8G8_SRGB] = 2,
        [HG_FORMAT_R8G8B8_UNORM] = 3,
        [HG_FORMAT_R8G8B8_SNORM] = 3,
        [HG_FORMAT_R8G8B8_USCALED] = 3,
        [HG_FORMAT_R8G8B8_SSCALED] = 3,
        [HG_FORMAT_R8G8B8_UINT] = 3,
        [HG_FORMAT_R8G8B8_SINT] = 3,
        [HG_FORMAT_R8G8B8_SRGB] = 3,
        [HG_FORMAT_B8G8R8_UNORM] = 3,
        [HG_FORMAT_B8G8R8_SNORM] = 3,
        [HG_FORMAT_B8G8R8_USCALED] = 3,
        [HG_FORMAT_B8G8R8_SSCALED] = 3,
        [HG_FORMAT_B8G8R8_UINT] = 3,
        [HG_FORMAT_B8G8R8_SINT] = 3,
        [HG_FORMAT_B8G8R8_SRGB] = 3,
        [HG_FORMAT_R8G8B8A8_UNORM] = 4,
        [HG_FORMAT_R8G8B8A8_SNORM] = 4,
        [HG_FORMAT_R8G8B8A8_USCALED] = 4,
        [HG_FORMAT_R8G8B8A8_SSCALED] = 4,
        [HG_FORMAT_R8G8B8A8_UINT] = 4,
        [HG_FORMAT_R8G8B8A8_SINT] = 4,
        [HG_FORMAT_R8G8B8A8_SRGB] = 4,
        [HG_FORMAT_B8G8R8A8_UNORM] = 4,
        [HG_FORMAT_B8G8R8A8_SNORM] = 4,
        [HG_FORMAT_B8G8R8A8_USCALED] = 4,
        [HG_FORMAT_B8G8R8A8_SSCALED] = 4,
        [HG_FORMAT_B8G8R8A8_UINT] = 4,
        [HG_FORMAT_B8G8R8A8_SINT] = 4,
        [HG_FORMAT_B8G8R8A8_SRGB] = 4,
        [HG_FORMAT_A8B8G8R8_UNORM_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_SNORM_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_USCALED_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_SSCALED_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_UINT_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_SINT_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_SRGB_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_UNORM_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_SNORM_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_USCALED_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_SSCALED_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_UINT_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_SINT_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_UNORM_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_SNORM_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_USCALED_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_SSCALED_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_UINT_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_SINT_PACK32] = 4,
        [HG_FORMAT_R16_UNORM] = 2,
        [HG_FORMAT_R16_SNORM] = 2,
        [HG_FORMAT_R16_USCALED] = 2,
        [HG_FORMAT_R16_SSCALED] = 2,
        [HG_FORMAT_R16_UINT] = 2,
        [HG_FORMAT_R16_SINT] = 2,
        [HG_FORMAT_R16_SFLOAT] = 2,
        [HG_FORMAT_R16G16_UNORM] = 4,
        [HG_FORMAT_R16G16_SNORM] = 4,
        [HG_FORMAT_R16G16_USCALED] = 4,
        [HG_FORMAT_R16G16_SSCALED] = 4,
        [HG_FORMAT_R16G16_UINT] = 4,
        [HG_FORMAT_R16G16_SINT] = 4,
        [HG_FORMAT_R16G16_SFLOAT] = 4,
        [HG_FORMAT_R16G16B16_UNORM] = 6,
        [HG_FORMAT_R16G16B16_SNORM] = 6,
        [HG_FORMAT_R16G16B16_USCALED] = 6,
        [HG_FORMAT_R16G16B16_SSCALED] = 6,
        [HG_FORMAT_R16G16B16_UINT] = 6,
        [HG_FORMAT_R16G16B16_SINT] = 6,
        [HG_FORMAT_R16G16B16_SFLOAT] = 6,
        [HG_FORMAT_R16G16B16A16_UNORM] = 8,
        [HG_FORMAT_R16G16B16A16_SNORM] = 8,
        [HG_FORMAT_R16G16B16A16_USCALED] = 8,
        [HG_FORMAT_R16G16B16A16_SSCALED] = 8,
        [HG_FORMAT_R16G16B16A16_UINT] = 8,
        [HG_FORMAT_R16G16B16A16_SINT] = 8,
        [HG_FORMAT_R16G16B16A16_SFLOAT] = 8,
        [HG_FORMAT_R32_UINT] = 4,
        [HG_FORMAT_R32_SINT] = 4,
        [HG_FORMAT_R32_SFLOAT] = 4,
        [HG_FORMAT_R32G32_UINT] = 8,
        [HG_FORMAT_R32G32_SINT] = 8,
        [HG_FORMAT_R32G32_SFLOAT] = 8,
        [HG_FORMAT_R32G32B32_UINT] = 12,
        [HG_FORMAT_R32G32B32_SINT] = 12,
        [HG_FORMAT_R32G32B32_SFLOAT] = 12,
        [HG_FORMAT_R32G32B32A32_UINT] = 16,
        [HG_FORMAT_R32G32B32A32_SINT] = 16,
        [HG_FORMAT_R32G32B32A32_SFLOAT] = 16,
        [HG_FORMAT_R64_UINT] = 8,
        [HG_FORMAT_R64_SINT] = 8,
        [HG_FORMAT_R64_SFLOAT] = 8,
        [HG_FORMAT_R64G64_UINT] = 16,
        [HG_FORMAT_R64G64_SINT] = 16,
        [HG_FORMAT_R64G64_SFLOAT] = 16,
        [HG_FORMAT_R64G64B64_UINT] = 24,
        [HG_FORMAT_R64G64B64_SINT] = 24,
        [HG_FORMAT_R64G64B64_SFLOAT] = 24,
        [HG_FORMAT_R64G64B64A64_UINT] = 32,
        [HG_FORMAT_R64G64B64A64_SINT] = 32,
        [HG_FORMAT_R64G64B64A64_SFLOAT] = 32,
        [HG_FORMAT_B10G11R11_UFLOAT_PACK32] = 4,
        [HG_FORMAT_E5B9G9R9_UFLOAT_PACK32] = 4,
        [HG_FORMAT_D16_UNORM] = 2,
        [HG_FORMAT_X8_D24_UNORM_PACK32] = 4,
        [HG_FORMAT_D32_SFLOAT] = 4,
        [HG_FORMAT_S8_UINT] = 1,
        [HG_FORMAT_D16_UNORM_S8_UINT] = 2,
        [HG_FORMAT_D24_UNORM_S8_UINT] = 4,
        [HG_FORMAT_D32_SFLOAT_S8_UINT] = 4,
    };
    return texture_format_sizes[format];
}

static VkImageUsageFlags hg_texture_usage_flags_to_vk(HgTextureUsageFlags usage) {
    return (usage & HG_TEXTURE_USAGE_TRANSFER_SRC_BIT ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0)
         | (usage & HG_TEXTURE_USAGE_TRANSFER_DST_BIT ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0)
         | (usage & HG_TEXTURE_USAGE_SAMPLED_BIT ? VK_IMAGE_USAGE_SAMPLED_BIT : 0)
         | (usage & HG_TEXTURE_USAGE_STORAGE_BIT ? VK_IMAGE_USAGE_STORAGE_BIT : 0)
         | (usage & HG_TEXTURE_USAGE_RENDER_TARGET_BIT ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0)
         | (usage & HG_TEXTURE_USAGE_DEPTH_BUFFER_BIT ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0);
}

static VkImageAspectFlags hg_texture_aspect_flags_to_vk(HgTextureAspectFlags aspect) {
    return (aspect & HG_TEXTURE_ASPECT_COLOR_BIT ? VK_IMAGE_ASPECT_COLOR_BIT : 0)
         | (aspect & HG_TEXTURE_ASPECT_DEPTH_BIT ? VK_IMAGE_ASPECT_DEPTH_BIT : 0);
}

static HgTextureAspectFlags hg_texture_aspect_flags_from_vk(VkImageAspectFlags aspect) {
    return (aspect & VK_IMAGE_ASPECT_COLOR_BIT ? HG_TEXTURE_ASPECT_COLOR_BIT : 0)
         | (aspect & VK_IMAGE_ASPECT_DEPTH_BIT ? HG_TEXTURE_ASPECT_DEPTH_BIT : 0);
}

static VkImageLayout hg_texture_layout_to_vk(HgTextureLayout layout) {
    return (VkImageLayout)layout;
    // const VkImageLayout image_layouts[] = {
    //     [HG_IMAGE_LAYOUT_UNDEFINED] = VK_IMAGE_LAYOUT_UNDEFINED,
    //     [HG_IMAGE_LAYOUT_GENERAL] = VK_IMAGE_LAYOUT_GENERAL,
    //     [HG_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //     [HG_IMAGE_LAYOUT_DEPTH_BUFFER_OPTIMAL] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    //     [HG_IMAGE_LAYOUT_DEPTH_BUFFER_READ_ONLY_OPTIMAL] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    //     [HG_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //     [HG_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //     [HG_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    // };
    // return image_layouts[layout];
}

// static HgTextureLayout hg_texture_layout_from_vk(VkImageLayout layout) {
//     return (HgTextureLayout)layout;
//     // const VkImageLayout texture_layouts[] = {
//     //     [VK_IMAGE_LAYOUT_UNDEFINED] = VK_IMAGE_LAYOUT_UNDEFINED,
//     //     [VK_IMAGE_LAYOUT_GENERAL] = VK_IMAGE_LAYOUT_GENERAL,
//     //     [VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
//     //     [VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
//     //     [VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
//     //     [VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//     //     [VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//     //     [VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//     // };
//     // return texture_layouts[layout];
// }

VkSamplerAddressMode hg_sampler_edge_mode_to_vk(HgSamplerEdgeMode edge_mode) {
    return (VkSamplerAddressMode)edge_mode;
    // const VkSamplerAddressMode texture_edge_modes[] = {
    //     [HG_SAMPLER_EDGE_MODE_REPEAT] = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    //     [HG_SAMPLER_EDGE_MODE_MIRRORED_REPEAT] = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    //     [HG_SAMPLER_EDGE_MODE_CLAMP_TO_EDGE] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    //     [HG_SAMPLER_EDGE_MODE_CLAMP_TO_BORDER] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    //     [HG_SAMPLER_EDGE_MODE_MIRROR_CLAMP_TO_EDGE] = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
    // };
    // return texture_edge_modes[edge_mode];
}

HgTexture* hg_texture_create(const HgTextureConfig* config) {
    HG_ASSERT(config->width > 0);
    HG_ASSERT(config->height > 0);
    HG_ASSERT(config->depth > 0);
    HG_ASSERT(config->format != HG_FORMAT_UNDEFINED);
    HG_ASSERT(config->aspect != HG_TEXTURE_ASPECT_NONE);
    HG_ASSERT(config->usage != HG_TEXTURE_USAGE_NONE);
    if (config->make_cubemap) {
        HG_ASSERT(config->array_layers == 6);
        HG_ASSERT(config->width == config->height);
        HG_ASSERT(config->depth == 1);
    }

    HgTexture* texture = hg_heap_alloc(sizeof(HgTexture));
    *texture = (HgTexture){
        .width = config->width,
        .height = config->height,
        .depth = config->depth,
        .array_layers = config->array_layers > 0 ? config->array_layers : 1,
        .mip_levels = config->mip_levels > 0 ? config->mip_levels : 1,
        .format = hg_format_to_vk(config->format),
        .aspect = hg_texture_aspect_flags_to_vk(config->aspect),
        .layout = VK_IMAGE_LAYOUT_UNDEFINED,
        .view = VK_NULL_HANDLE,
    };

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = config->make_cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
        .imageType = VK_IMAGE_TYPE_3D
                   - (config->depth == 1)
                   - (config->height == 1),
        .format = hg_format_to_vk(config->format),
        .extent = (VkExtent3D){
            .width = config->width,
            .height = config->height,
            .depth = config->depth,
        },
        .mipLevels = config->mip_levels > 0 ? config->mip_levels : 1,
        .arrayLayers = config->array_layers > 0 ? config->array_layers : 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .usage = hg_texture_usage_flags_to_vk(config->usage),
    };
    VmaAllocationCreateInfo alloc_info = {
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    VkResult image_result = vmaCreateImage(
        s_allocator,
        &image_info,
        &alloc_info,
        &texture->handle,
        &texture->allocation,
        NULL
    );
    switch (image_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: HG_ERROR("Vulkan compression exhausted");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: HG_ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: HG_ERROR("Vulkan invalid opaque capture address");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    if (config->usage & (
            HG_TEXTURE_USAGE_SAMPLED_BIT |
            HG_TEXTURE_USAGE_STORAGE_BIT |
            HG_TEXTURE_USAGE_RENDER_TARGET_BIT |
            HG_TEXTURE_USAGE_DEPTH_BUFFER_BIT
        )) {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = texture->handle,
            .viewType = config->make_cubemap
                ? VK_IMAGE_VIEW_TYPE_CUBE
                : VK_IMAGE_VIEW_TYPE_3D
                - (config->depth == 1)
                - (config->height == 1),
            .format = texture->format,
            .subresourceRange = (VkImageSubresourceRange){
                .aspectMask = texture->aspect,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };

        VkResult result = vkCreateImageView(s_device, &view_info, NULL, &texture->view);
        switch (result) {
            case VK_SUCCESS: break;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: HG_ERROR("Vulkan invalid opaque capture address");
            default: HG_ERROR("Unexpected Vulkan error");
        }
    }

    if (config->usage & HG_TEXTURE_USAGE_SAMPLED_BIT) {
        VkPhysicalDeviceProperties gpu_properties = {0};
        vkGetPhysicalDeviceProperties(s_gpu, &gpu_properties);

        VkSamplerCreateInfo sampler_info = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = config->bilinear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
            .minFilter = config->bilinear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
            .addressModeU = hg_sampler_edge_mode_to_vk(config->edge_mode),
            .addressModeV = hg_sampler_edge_mode_to_vk(config->edge_mode),
            .addressModeW = hg_sampler_edge_mode_to_vk(config->edge_mode),
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = gpu_properties.limits.maxSamplerAnisotropy,
            .maxLod = config->mip_levels > 0 ? (f32)config->mip_levels : 1.0f,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        };
        if (config->bilinear_filter)
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        else
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        const VkResult sampler_result = vkCreateSampler(s_device, &sampler_info, NULL, &texture->sampler);
        switch (sampler_result) {
            case VK_SUCCESS: break;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: HG_ERROR("Vulkan invalid device address");
            default: HG_ERROR("Unexpected Vulkan error");
        }
    }

    return texture;
}

void hg_texture_destroy(HgTexture* texture) {
    HG_ASSERT(texture != NULL);
    HG_ASSERT(texture->allocation != VK_NULL_HANDLE);
    HG_ASSERT(texture->handle != VK_NULL_HANDLE);

    if (texture->sampler != VK_NULL_HANDLE)
        vkDestroySampler(s_device, texture->sampler, NULL);
    if (texture->view != VK_NULL_HANDLE)
        vkDestroyImageView(s_device, texture->view, NULL);
    vmaDestroyImage(s_allocator, texture->handle, texture->allocation);
    hg_heap_free(texture, sizeof(HgTexture));
}

static void hg_copy_buffer_to_image(VkCommandBuffer cmd, HgTexture* dst, HgBuffer* src) {
    HG_ASSERT(dst->allocation != NULL);
    HG_ASSERT(dst->handle != NULL);
    HG_ASSERT(src->allocation != NULL);
    HG_ASSERT(src->handle != NULL);

    const VkBufferImageCopy2 copy_region = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
        .imageSubresource = {dst->aspect, 0, 0, 1},
        .imageExtent = {dst->width, dst->height, dst->depth},
    };
    const VkCopyBufferToImageInfo2 copy_region_info = {
        .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
        .srcBuffer = src->handle,
        .dstImage = dst->handle,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &copy_region,
    };
    vkCmdCopyBufferToImage2(cmd, &copy_region_info);
}

static void hg_write_cubemap(HgTexture* dst, const void* src, VkImageLayout layout) {
    HG_ASSERT(dst->allocation != NULL);
    HG_ASSERT(dst->handle != NULL);

    if (src != NULL) {
        VkExtent3D staging_extent = {dst->width * 4, dst->height * 3, dst->depth};

        HgTexture* staging_image = hg_texture_create(&(HgTextureConfig){
            .width = staging_extent.width,
            .height = staging_extent.height,
            .depth = staging_extent.depth,
            .array_layers = 1,
            .mip_levels = 1,
            .format = hg_format_from_vk(dst->format),
            .aspect = hg_texture_aspect_flags_from_vk(dst->aspect),
            .usage = HG_TEXTURE_USAGE_TRANSFER_SRC_BIT | HG_TEXTURE_USAGE_TRANSFER_DST_BIT,
        });
        hg_texture_write(staging_image, src, HG_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        VkCommandBuffer cmd = hg_begin_single_time_cmd();

        VkImageMemoryBarrier2 transfer_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = dst->handle,
            .subresourceRange = {dst->aspect, 0, 1, 0, 6},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &transfer_barrier,
        });

        VkImageCopy2 copies[] = {{
            .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
            .srcSubresource = {dst->aspect, 0, 0, 1},
            .srcOffset = {(i32)dst->width * 2, (i32)dst->height * 1, 0},
            .dstSubresource = {dst->aspect, 0, 0, 1},
            .extent = {dst->width, dst->height, 1},
        }, {
            .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
            .srcSubresource = {dst->aspect, 0, 0, 1},
            .srcOffset = {(i32)dst->width * 0, (i32)dst->height * 1, 0},
            .dstSubresource = {dst->aspect, 0, 1, 1},
            .extent = {dst->width, dst->height, 1},
        }, {
            .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
            .srcSubresource = {dst->aspect, 0, 0, 1},
            .srcOffset = {(i32)dst->width * 1, (i32)dst->height * 0, 0},
            .dstSubresource = {dst->aspect, 0, 2, 1},
            .extent = {dst->width, dst->height, 1},
        }, {
            .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
            .srcSubresource = {dst->aspect, 0, 0, 1},
            .srcOffset = {(i32)dst->width * 1, (i32)dst->height * 2, 0},
            .dstSubresource = {dst->aspect, 0, 3, 1},
            .extent = {dst->width, dst->height, 1},
        }, {
            .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
            .srcSubresource = {dst->aspect, 0, 0, 1},
            .srcOffset = {(i32)dst->width * 1, (i32)dst->height * 1, 0},
            .dstSubresource = {dst->aspect, 0, 4, 1},
            .extent = {dst->width, dst->height, 1},
        }, {
            .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
            .srcSubresource = {dst->aspect, 0, 0, 1},
            .srcOffset = {(i32)dst->width * 3, (i32)dst->height * 1, 0},
            .dstSubresource = {dst->aspect, 0, 5, 1},
            .extent = {dst->width, dst->height, 1},
        }};
        VkCopyImageInfo2 copy_region_info = {
            .sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
            .srcImage = staging_image->handle,
            .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .dstImage = dst->handle,
            .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .regionCount = HG_ARRAY_SIZE(copies),
            .pRegions = copies,
        };
        vkCmdCopyImage2(cmd, &copy_region_info);

        VkImageMemoryBarrier2 final_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = layout,
            .image = dst->handle,
            .subresourceRange = {dst->aspect, 0, 1, 0, 6},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &final_barrier,
        });

        hg_end_single_time_cmd(cmd);

        hg_texture_destroy(staging_image);
    } else {
        VkCommandBuffer cmd = hg_begin_single_time_cmd();

        VkImageMemoryBarrier2 final_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = layout,
            .image = dst->handle,
            .subresourceRange = {dst->aspect, 0, 1, 0, 6},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &final_barrier,
        });

        hg_end_single_time_cmd(cmd);
    }

    dst->layout = layout;
}

static void hg_write_image(HgTexture* dst, const void* src, VkImageLayout layout) {
    HG_ASSERT(dst->allocation != NULL);
    HG_ASSERT(dst->handle != NULL);

    if (src != NULL) {
        u32 pixel_size = hg_format_size(hg_format_from_vk(dst->format));
        HG_ASSERT(pixel_size > 0);

        HgBuffer* staging_buffer = hg_buffer_create(&(HgBufferConfig){
            .size = dst->width * dst->height * dst->depth * pixel_size,
            .usage = HG_BUFFER_USAGE_READ_WRITE_SRC_BIT,
            .memory_type = HG_GPU_MEMORY_TYPE_LINEAR_ACCESS,
        });
        hg_buffer_write(staging_buffer, 0, src, dst->width * dst->height * dst->depth * pixel_size);

        VkCommandBuffer cmd = hg_begin_single_time_cmd();

        VkImageMemoryBarrier2 pre_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = dst->handle,
            .subresourceRange = {dst->aspect, 0, VK_REMAINING_MIP_LEVELS, 0, 1},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &pre_barrier,
        });

        hg_copy_buffer_to_image(cmd, dst, staging_buffer);

        VkImageMemoryBarrier2 post_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = layout,
            .image = dst->handle,
            .subresourceRange = {dst->aspect, 0, VK_REMAINING_MIP_LEVELS, 0, 1},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &post_barrier,
        });

        hg_end_single_time_cmd(cmd);

        hg_buffer_destroy(staging_buffer);
    } else {
        VkCommandBuffer cmd = hg_begin_single_time_cmd();

        VkImageMemoryBarrier2 barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .newLayout = layout,
            .image = dst->handle,
            .subresourceRange = {dst->aspect, 0, VK_REMAINING_MIP_LEVELS, 0, 1},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier,
        });

        hg_end_single_time_cmd(cmd);
    }

    dst->layout = layout;
}

void hg_texture_write(HgTexture* dst, const void* src, HgTextureLayout layout) {
    if (dst->is_cubemap) {
        hg_write_cubemap(dst, src, hg_texture_layout_to_vk(layout));
    } else {
        hg_write_image(dst, src, hg_texture_layout_to_vk(layout));
    }
}

static void hg_copy_image_to_buffer(VkCommandBuffer cmd, HgBuffer* dst, HgTexture* src) {
    HG_ASSERT(dst->allocation != NULL);
    HG_ASSERT(dst->handle != NULL);
    HG_ASSERT(src->allocation != NULL);
    HG_ASSERT(src->handle != NULL);

    vkCmdCopyImageToBuffer2(cmd, &(VkCopyImageToBufferInfo2){
        .sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2,
        .srcImage = src->handle,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstBuffer = dst->handle,
        .regionCount = 1,
        .pRegions = &(VkBufferImageCopy2){
            .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
            .imageSubresource = {src->aspect, 0, 0, 1},
            .imageExtent = {src->width, src->height, src->depth},
        },
    });
}

void hg_texture_read(void* dst, HgTexture* src, HgTextureLayout layout) {
    HG_ASSERT(src->handle != VK_NULL_HANDLE);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(layout != HG_IMAGE_LAYOUT_UNDEFINED);

    u32 pixel_size = hg_format_size(hg_format_from_vk(src->format));
    HG_ASSERT(pixel_size > 0);

    HgBuffer* staging_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = src->width * src->height * src->depth * pixel_size,
        .usage = HG_BUFFER_USAGE_READ_WRITE_DST_BIT,
        .memory_type = HG_GPU_MEMORY_TYPE_LINEAR_ACCESS,
    });

    VkCommandBuffer cmd = hg_begin_single_time_cmd();

    VkImageMemoryBarrier2 pre_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image = src->handle,
        .subresourceRange = {src->aspect, 0, VK_REMAINING_MIP_LEVELS, 0, 1},
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &pre_barrier,
    });

    hg_copy_image_to_buffer(cmd, staging_buffer, src);

    VkImageMemoryBarrier2 post_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .newLayout = hg_texture_layout_to_vk(layout),
        .image = src->handle,
        .subresourceRange = {src->aspect, 0, VK_REMAINING_MIP_LEVELS, 0, 1},
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &post_barrier,
    });

    hg_end_single_time_cmd(cmd);

    src->layout = hg_texture_layout_to_vk(layout);

    hg_buffer_read(dst, staging_buffer->size, staging_buffer, 0);
    hg_buffer_destroy(staging_buffer);
}

u32 hg_get_max_mip_count(u32 width, u32 height, u32 depth) {
    if (width == 0 && height == 0 && depth == 0)
        return 0;
    return (u32)(log2f(HG_MAX(HG_MAX((f32)width, (f32)height), (f32)depth))) + 1;
}

typedef struct BlitConfig {
    VkImage image;
    VkImageAspectFlags aspect;
    VkOffset3D begin;
    VkOffset3D end;
    uint32_t mip_level;
    uint32_t array_layer;
    uint32_t layer_count;
} BlitConfig;

void hg_blit_image(VkCommandBuffer cmd, const BlitConfig* dst, const BlitConfig* src, VkFilter filter) {
    HG_ASSERT(dst->image != NULL);
    HG_ASSERT(src->image != NULL);

    VkImageBlit2 region = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .srcSubresource = {src->aspect, src->mip_level, src->array_layer, src->layer_count},
        .srcOffsets = {src->begin, src->end},
        .dstSubresource = {dst->aspect, dst->mip_level, dst->array_layer, dst->layer_count},
        .dstOffsets = {dst->begin, dst->end},
    };
    VkBlitImageInfo2 blit_image_info = {
        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .srcImage = src->image,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage = dst->image,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &region,
        .filter = filter,
    };
    vkCmdBlitImage2(cmd, &blit_image_info);
}

void hg_texture_generate_mipmaps(HgTexture* texture, HgTextureLayout layout) {
    HG_ASSERT(texture->mip_levels > 1);
    HG_ASSERT(texture->width > 0);
    HG_ASSERT(texture->height > 0);
    HG_ASSERT(texture->depth > 0);
    HG_ASSERT(texture->format != VK_FORMAT_UNDEFINED);
    HG_ASSERT(layout != HG_IMAGE_LAYOUT_UNDEFINED);

    VkFormatProperties format_properties = {0};
    vkGetPhysicalDeviceFormatProperties(s_gpu, texture->format, &format_properties);
    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        HG_ERROR("Format does not support optimal tiling with linear filtering");

    VkCommandBuffer cmd = hg_begin_single_time_cmd();

    VkOffset3D mip_offset = {(i32)texture->width, (i32)texture->height, (i32)texture->depth};

    VkImageMemoryBarrier2 initial_read_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image = texture->handle,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, texture->mip_levels, 0, 1},
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &initial_read_barrier,
    });

    for (u32 level = 0; level < texture->mip_levels - 1; ++level) {
        VkImageMemoryBarrier2 write_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = texture->handle,
            .subresourceRange = {texture->aspect, level + 1, 1, 0, 1},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &write_barrier,
        });

        BlitConfig src_view = {
            .image = texture->handle,
            .aspect = texture->aspect,
            .end = mip_offset,
            .mip_level = level,
            .array_layer = 0,
            .layer_count = 1,
        };
        if (mip_offset.x > 1)
            mip_offset.x /= 2;
        if (mip_offset.y > 1)
            mip_offset.y /= 2;
        if (mip_offset.z > 1)
            mip_offset.z /= 2;
        BlitConfig dst_view = {
            .image = texture->handle,
            .aspect = texture->aspect,
            .end = mip_offset,
            .mip_level = level + 1,
            .array_layer = 0,
            .layer_count = 1,
        };
        hg_blit_image(cmd, &dst_view, &src_view, VK_FILTER_LINEAR);

        VkImageMemoryBarrier2 read_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .image = texture->handle,
            .subresourceRange = {texture->aspect, level + 1, 1, 0, 1},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &read_barrier,
        });
    }

    VkImageMemoryBarrier2 final_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .newLayout = hg_texture_layout_to_vk(layout),
        .image = texture->handle,
        .subresourceRange = {texture->aspect, 0, texture->mip_levels, 0, 1},
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &final_barrier,
    });

    hg_end_single_time_cmd(cmd);

    texture->layout = hg_texture_layout_to_vk(layout);
}

typedef struct HgShader {
    VkPipelineLayout layout;
    VkPipeline pipeline;
    u32 descriptor_layout_count;
    VkPipelineBindPoint bind_point;
    VkDescriptorSetLayout descriptor_layouts[];
} HgShader;

static VkDescriptorType hg_descriptor_type_to_vk(HgDescriptorType type) {
    const VkDescriptorType descriptor_types[] = {
        [HG_DESCRIPTOR_TYPE_UNIFORM_BUFFER] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        [HG_DESCRIPTOR_TYPE_STORAGE_BUFFER] = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        [HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE] = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        [HG_DESCRIPTOR_TYPE_STORAGE_TEXTURE] = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    };
    return descriptor_types[type];
}

static VkDescriptorSetLayout hg_create_descriptor_set_layout(
    const VkDescriptorSetLayoutBinding* bindings, u32 binding_count
) {
    HG_ASSERT(bindings != NULL);
    HG_ASSERT(binding_count > 0);

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = binding_count,
        .pBindings = bindings,
    };
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    const VkResult result = vkCreateDescriptorSetLayout(s_device, &layout_info, NULL, &layout);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED_EXT: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return layout;
}

static void hg_create_descriptor_set_layouts(
    VkDescriptorSetLayout* layouts,
    HgDescriptorSet* descriptor_sets,
    u32 descriptor_set_count,
    VkPipelineStageFlags stage_flags
) {
#define HG_MAX_DESCRIPTOR_BINDINGS 32
    for (u32 i = 0; i < descriptor_set_count; ++i) {
        HgDescriptorSet* set = &descriptor_sets[i];
        HG_ASSERT(set->binding_count < HG_MAX_DESCRIPTOR_BINDINGS);

        VkDescriptorSetLayoutBinding bindings[HG_MAX_DESCRIPTOR_BINDINGS];
        for (u32 j = 0; j < set->binding_count; ++j) {
            bindings[j] = (VkDescriptorSetLayoutBinding){
                .binding = j,
                .descriptorType = hg_descriptor_type_to_vk(set->bindings[j].descriptor_type),
                .descriptorCount = set->bindings[j].descriptor_count,
                .stageFlags = stage_flags,
            };
        }

        layouts[i] = hg_create_descriptor_set_layout(
            bindings,
            descriptor_sets[i].binding_count
        );
    }
#undef HG_MAX_DESCRIPTOR_BINDINGS
}

static VkPipelineLayout hg_create_pipeline_layout(
    const VkDescriptorSetLayout* layouts, u32 layout_count,
    const VkPushConstantRange* push_constants, u32 push_constant_count
) {
    if (layout_count > 0)
        HG_ASSERT(layouts != NULL);
    if (push_constant_count > 0)
        HG_ASSERT(push_constants != NULL);

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = layout_count,
        .pSetLayouts = layouts,
        .pushConstantRangeCount = push_constant_count,
        .pPushConstantRanges = push_constants,
    };
    VkPipelineLayout layout = VK_NULL_HANDLE;
    const VkResult result = vkCreatePipelineLayout(s_device, &pipeline_layout_info, NULL, &layout);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED_EXT: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return layout;
}

static VkShaderModule hg_create_shader_module(const byte* code, usize size) {
    HG_ASSERT(code != NULL);
    HG_ASSERT(size > 0);

    VkShaderModuleCreateInfo shader_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = (const u32*)code,
    };
    VkShaderModule shader_module = VK_NULL_HANDLE;
    const VkResult result = vkCreateShaderModule(s_device, &shader_module_info, NULL, &shader_module);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED_EXT: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_INVALID_SHADER_NV: HG_ERROR("Vulkan invalid shader");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return shader_module;
}

// static VkShaderStageFlags hg_shader_stage_to_vk(HgShaderStageFlags stage) {
//     return stage & HG_SHADER_STAGE_ALL_GRAPHICS ? VK_SHADER_STAGE_ALL_GRAPHICS
//          : (stage & HG_SHADER_STAGE_VERTEX_BIT ? VK_SHADER_STAGE_VERTEX_BIT : 0)
//          | (stage & HG_SHADER_STAGE_TESSELLATION_CONTROL_BIT ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : 0)
//          | (stage & HG_SHADER_STAGE_TESSELLATION_EVALUATION_BIT ? VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : 0)
//          | (stage & HG_SHADER_STAGE_GEOMETRY_BIT ? VK_SHADER_STAGE_GEOMETRY_BIT : 0)
//          | (stage & HG_SHADER_STAGE_FRAGMENT_BIT ? VK_SHADER_STAGE_FRAGMENT_BIT : 0)
//          | (stage & HG_SHADER_STAGE_COMPUTE_BIT ? VK_SHADER_STAGE_COMPUTE_BIT : 0);
// }

static VkPrimitiveTopology hg_primitive_topology_to_vk(HgPrimitiveTopology topology) {
    return (VkPrimitiveTopology)topology;
    // const VkPrimitiveTopology primitive_topologies[] = {
    //     [HG_PRIMITIVE_TOPOLOGY_POINT_LIST] = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
    //     [HG_PRIMITIVE_TOPOLOGY_LINE_LIST] = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
    //     [HG_PRIMITIVE_TOPOLOGY_LINE_STRIP] = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
    //     [HG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    //     [HG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    //     [HG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
    // };
    // return primitive_topologies[topology];
}

static VkCullModeFlags hg_cull_mode_to_vk(HgCullModeFlagBits cull_mode) {
    return (VkCullModeFlags)cull_mode;
    // const VkCullModeFlags cull_modes[] = {
    //     [HG_CULL_MODE_NONE] = VK_CULL_MODE_NONE,
    //     [HG_CULL_MODE_FRONT_BIT] = VK_CULL_MODE_FRONT_BIT,
    //     [HG_CULL_MODE_BACK_BIT] = VK_CULL_MODE_BACK_BIT,
    //     [HG_CULL_MODE_FRONT_AND_BACK] = VK_CULL_MODE_FRONT_AND_BACK,
    // };
    // return cull_modes[cull_mode];
}

static VkVertexInputRate hg_vertex_input_rate_to_vk(HgVertexInputRate rate) {
    return (VkVertexInputRate)rate;
    // const VkVertexInputRate vertex_input_rates[] = {
    //     [HG_VERTEX_INPUT_RATE_VERTEX] = VK_VERTEX_INPUT_RATE_VERTEX,
    //     [HG_VERTEX_INPUT_RATE_INSTANCE] = VK_VERTEX_INPUT_RATE_INSTANCE,
    // };
    // return vertex_input_rates[rate];
}

HgShader* hg_shader_create(const HgShaderConfig* config) {
    HG_ASSERT(config != NULL);
    HG_ASSERT(config->spirv_vertex_shader != NULL);
    HG_ASSERT(config->spirv_fragment_shader != NULL);
    HG_ASSERT(config->vertex_shader_size > 0);
    HG_ASSERT(config->fragment_shader_size > 0);
    if (config->vertex_binding_count > 0)
        HG_ASSERT(config->vertex_bindings != NULL);
    if (config->descriptor_set_count > 0)
        HG_ASSERT(config->descriptor_sets != NULL);
    if (config->enable_depth_buffer)
        HG_ASSERT(config->depth_format != HG_FORMAT_UNDEFINED);

    HgShader* shader = hg_heap_alloc(
        sizeof(HgShader) + config->descriptor_set_count * sizeof(VkDescriptorSetLayout)
    );
    *shader = (HgShader){
        .descriptor_layout_count = config->descriptor_set_count,
        .bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS,
    };

    hg_create_descriptor_set_layouts(
        shader->descriptor_layouts,
        config->descriptor_sets,
        config->descriptor_set_count,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );

    VkPushConstantRange push_constant = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = config->push_constant_size,
    };

    shader->layout = hg_create_pipeline_layout(
        shader->descriptor_layouts,
        shader->descriptor_layout_count,
        config->push_constant_size > 0 ? &push_constant : NULL,
        config->push_constant_size > 0 ? 1 : 0
    );

    VkShaderModule shader_modules[] = {
        hg_create_shader_module(config->spirv_vertex_shader, config->vertex_shader_size),
        hg_create_shader_module(config->spirv_fragment_shader, config->fragment_shader_size),
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = shader_modules[0],
        .pName = "main",
    }, {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = shader_modules[1],
        .pName = "main",
    }};

#define HG_MAX_VERTEX_BINDINGS 8
#define HG_MAX_VERTEX_ATTRIBUTES 32

    u32 vertex_binding_count = 0;
    VkVertexInputBindingDescription vertex_binding_descriptions[HG_MAX_VERTEX_BINDINGS];
    u32 vertex_attribute_count = 0;
    VkVertexInputAttributeDescription vertex_attribute_descriptions[HG_MAX_VERTEX_ATTRIBUTES];

    for (u32 i = 0; i < config->vertex_binding_count; ++i) {
        HG_ASSERT(config->vertex_bindings[i].attribute_count < HG_MAX_VERTEX_ATTRIBUTES);
        HgVertexBinding* binding = &config->vertex_bindings[i];

        vertex_binding_descriptions[vertex_binding_count] = (VkVertexInputBindingDescription){
            .binding = i,
            .stride = binding->stride,
            .inputRate = hg_vertex_input_rate_to_vk(binding->input_rate),
        };
        for (u32 j = 0; j < binding->attribute_count; ++j) {
            HG_ASSERT(vertex_attribute_count < HG_MAX_VERTEX_ATTRIBUTES);
            HgVertexAttribute* attribute = &binding->attributes[j];

            vertex_attribute_descriptions[vertex_attribute_count] = (VkVertexInputAttributeDescription){
                .location = j,
                .binding = i,
                .format = hg_format_to_vk(attribute->format),
                .offset = attribute->offset,
            };
            ++vertex_attribute_count;
        }
        ++vertex_binding_count;
    }
#undef HG_MAX_VERTEX_BINDINGS
#undef HG_MAX_VERTEX_ATTRIBUTES

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = vertex_binding_count,
        .pVertexBindingDescriptions = vertex_binding_descriptions,
        .vertexAttributeDescriptionCount = vertex_attribute_count,
        .pVertexAttributeDescriptions = vertex_attribute_descriptions,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = hg_primitive_topology_to_vk(config->topology),
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineTessellationStateCreateInfo tessellation_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = hg_cull_mode_to_vk(config->cull_mode),
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = config->enable_depth_buffer ? VK_TRUE : VK_FALSE,
        .depthWriteEnable = config->enable_depth_buffer ? VK_TRUE : VK_FALSE,
        .depthCompareOp = config->enable_color_blend ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = config->enable_color_blend ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                        | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT
                        | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = {1.0f, 1.0f, 1.0f, 1.0f},
    };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates = dynamic_states,
        .dynamicStateCount = HG_ARRAY_SIZE(dynamic_states),
    };

    VkFormat color_format = hg_format_to_vk(config->color_format);
    VkPipelineRenderingCreateInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = color_format != VK_FORMAT_UNDEFINED ? 1 : 0,
        .pColorAttachmentFormats = &color_format,
        .depthAttachmentFormat = hg_format_to_vk(config->depth_format),
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering_info,
        .stageCount = HG_ARRAY_SIZE(shader_stages),
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState = &tessellation_state,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = shader->layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VkResult result = vkCreateGraphicsPipelines(s_device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &shader->pipeline);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_PIPELINE_COMPILE_REQUIRED_EXT: {
            HG_WARN("Pipeline requires recompilation");
        } break;
        case VK_ERROR_INVALID_SHADER_NV: HG_ERROR("Vulkan invalid shader");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED_EXT: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    for (usize i = 0; i < HG_ARRAY_SIZE(shader_modules); ++i) {
        vkDestroyShaderModule(s_device, shader_modules[i], NULL);
    }
    return shader;
}

HgShader* hg_compute_shader_create(const HgComputeShaderConfig* config) {
    HG_ASSERT(config != NULL);
    HG_ASSERT(config->spirv_shader != NULL);
    HG_ASSERT(config->shader_size > 0);
    if (config->descriptor_set_count > 0)
        HG_ASSERT(config->descriptor_sets != NULL);

    HgShader* shader = hg_heap_alloc(
        sizeof(HgShader) + config->descriptor_set_count * sizeof(VkDescriptorSetLayout)
    );
    *shader = (HgShader){
        .descriptor_layout_count = config->descriptor_set_count,
        .bind_point = VK_PIPELINE_BIND_POINT_COMPUTE,
    };

    hg_create_descriptor_set_layouts(
        shader->descriptor_layouts,
        config->descriptor_sets,
        config->descriptor_set_count,
        VK_SHADER_STAGE_COMPUTE_BIT
    );

    VkPushConstantRange push_constant = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = config->push_constant_size,
    };

    shader->layout = hg_create_pipeline_layout(
        shader->descriptor_layouts,
        shader->descriptor_layout_count,
        config->push_constant_size > 0 ? &push_constant : NULL,
        config->push_constant_size > 0 ? 1 : 0
    );

    VkShaderModule shader_module = hg_create_shader_module(config->spirv_shader, config->shader_size);

    VkPipelineShaderStageCreateInfo shader_stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader_module,
        .pName = "main",
    };

    VkComputePipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shader_stage,
        .layout = shader->layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VkResult result = vkCreateComputePipelines(s_device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &shader->pipeline);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_PIPELINE_COMPILE_REQUIRED_EXT: {
            HG_WARN("Pipeline requires recompilation");
        } break;
        case VK_ERROR_INVALID_SHADER_NV: HG_ERROR("Vulkan invalid shader");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED_EXT: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    vkDestroyShaderModule(s_device, shader_module, NULL);
    return shader;
}

void hg_shader_destroy(HgShader* shader) {
    HG_ASSERT(shader != NULL);
    HG_ASSERT(shader->layout != VK_NULL_HANDLE);
    HG_ASSERT(shader->pipeline != VK_NULL_HANDLE);
    for (usize i = 0; i < shader->descriptor_layout_count; ++i) {
        HG_ASSERT(shader->descriptor_layouts[i] != VK_NULL_HANDLE);
    }

    vkDestroyPipeline(s_device, shader->pipeline, NULL);
    vkDestroyPipelineLayout(s_device, shader->layout, NULL);
    for (usize i = 0; i < shader->descriptor_layout_count; ++i) {
        vkDestroyDescriptorSetLayout(s_device, shader->descriptor_layouts[i], NULL);
    }
    hg_heap_free(shader, sizeof(HgShader) + shader->descriptor_layout_count * sizeof(VkDescriptorSetLayout));
}

static VkDescriptorPool hg_current_descriptor_pool(void) {
    return s_descriptor_pools[s_current_frame_index];
}

static VkImage hg_current_image(void) {
    return s_swapchain_images[s_current_image_index];
}

static VkFence hg_is_frame_finished(void) {
    return s_frame_finished_fences[s_current_frame_index];
}

static VkSemaphore hg_is_image_available(void) {
    return s_image_available_semaphores[s_current_frame_index];
}

static void hg_reset_fence(VkFence fence) {
    HG_ASSERT(fence != NULL);

    VkResult reset_result = vkResetFences(s_device, 1, &fence);
    switch (reset_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }
}

void hg_commands_begin(void) {
    HG_ASSERT(!s_recording_frame);
    HG_ASSERT(s_current_cmd == VK_NULL_HANDLE);

    s_current_cmd = hg_begin_single_time_cmd();
}

void hg_commands_end(void) {
    HG_ASSERT(!s_recording_frame);
    HG_ASSERT(s_current_cmd != VK_NULL_HANDLE);

    hg_end_single_time_cmd(s_current_cmd);
    s_current_cmd = VK_NULL_HANDLE;
}

HgError hg_frame_begin(void) {
    HG_ASSERT(!s_recording_frame);
    HG_ASSERT(s_current_cmd == VK_NULL_HANDLE);

    s_current_frame_index = (s_current_frame_index + 1) % HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT;

    hg_wait_for_fence(hg_is_frame_finished());
    hg_reset_fence(hg_is_frame_finished());

    const VkResult acquire_result = vkAcquireNextImageKHR(
        s_device,
        s_swapchain,
        UINT64_MAX,
        hg_is_image_available(),
        NULL,
        &s_current_image_index
    );
    switch (acquire_result) {
        case VK_SUCCESS: break;
        case VK_SUBOPTIMAL_KHR: return HG_ERROR_WINDOW_INVALID;
        case VK_ERROR_OUT_OF_DATE_KHR: return HG_ERROR_WINDOW_INVALID;
        case VK_TIMEOUT: HG_ERROR("Vulkan timed out waiting for image");
        case VK_NOT_READY: HG_ERROR("Vulkan not ready waiting for image");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkResult pool_result = vkResetDescriptorPool(s_device, hg_current_descriptor_pool(), 0);
    switch (pool_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_VALIDATION_FAILED_EXT: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    s_current_cmd = s_command_buffers[s_current_frame_index];

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkResult begin_result = vkBeginCommandBuffer(s_current_cmd, &begin_info);
    switch (begin_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    s_recording_frame = true;

    return HG_SUCCESS;
}

HgError hg_frame_end(HgTexture* framebuffer) {
    HG_ASSERT(s_recording_frame);
    HG_ASSERT(s_current_cmd != VK_NULL_HANDLE);
    if (s_recording_pass)
        hg_renderpass_end();

    if (framebuffer == NULL) {
        VkResult end_result = vkEndCommandBuffer(s_current_cmd);
        switch (end_result) {
            case VK_SUCCESS: break;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: HG_ERROR("Vulkan invalid video parameters");
            default: HG_ERROR("Unexpected Vulkan error");
        }
        s_current_cmd = VK_NULL_HANDLE;
        s_recording_frame = false;

        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &s_image_available_semaphores[s_current_frame_index],
            .pWaitDstStageMask = &(VkPipelineStageFlags){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
            .commandBufferCount = 1,
            .pCommandBuffers = &s_command_buffers[s_current_frame_index],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &s_ready_to_present_semaphores[s_current_image_index],
        };

        VkResult submit_result = vkQueueSubmit(s_queue, 1, &submit_info, hg_is_frame_finished());
        switch (submit_result) {
            case VK_SUCCESS: break;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
            default: HG_ERROR("Unexpected Vulkan error");
        }

        return HG_SUCCESS;
    }

    VkImageMemoryBarrier2 barriers[] = {{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = framebuffer->layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                      ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
                      : framebuffer->layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                      ? VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
                      : framebuffer->layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                      ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
                      : VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask = framebuffer->layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                       ? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
                       : framebuffer->layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                       ? VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                       : framebuffer->layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                       ? VK_ACCESS_2_SHADER_READ_BIT
                       : VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .oldLayout = framebuffer->layout,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image = framebuffer->handle,
        .subresourceRange = {framebuffer->aspect, 0, 1, 0, 1},
    }, {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = hg_current_image(),
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    }};
    framebuffer->layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    vkCmdPipelineBarrier2(s_current_cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = HG_ARRAY_SIZE(barriers),
        .pImageMemoryBarriers = barriers,
    });

    BlitConfig blit_src = {
        .image = framebuffer->handle,
        .aspect = framebuffer->aspect,
        .begin = {0, 0, 0},
        .end = {(i32)framebuffer->width, (i32)framebuffer->height, 1},
        .mip_level = 0,
        .array_layer = 0,
        .layer_count = 1,
    };
    BlitConfig blit_dst = {
        .image = hg_current_image(),
        .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .begin = {0, 0, 0},
        .end = {(i32)s_swapchain_width, (i32)s_swapchain_height, 1},
        .mip_level = 0,
        .array_layer = 0,
        .layer_count = 1,
    };
    hg_blit_image(s_current_cmd, &blit_dst, &blit_src, VK_FILTER_NEAREST);

    s_previous_target = NULL;
    s_previous_depth_buffer = NULL;

    VkImageMemoryBarrier2 final_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = hg_current_image(),
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier2(s_current_cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &final_barrier,
    });

    VkResult end_result = vkEndCommandBuffer(s_current_cmd);
    switch (end_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: HG_ERROR("Vulkan invalid video parameters");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    s_current_cmd = VK_NULL_HANDLE;
    s_recording_frame = false;

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &s_image_available_semaphores[s_current_frame_index],
        .pWaitDstStageMask = &(VkPipelineStageFlags){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        .commandBufferCount = 1,
        .pCommandBuffers = &s_command_buffers[s_current_frame_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &s_ready_to_present_semaphores[s_current_image_index],
    };

    VkResult submit_result = vkQueueSubmit(s_queue, 1, &submit_info, hg_is_frame_finished());
    switch (submit_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &s_ready_to_present_semaphores[s_current_image_index],
        .swapchainCount = 1,
        .pSwapchains = &s_swapchain,
        .pImageIndices = &s_current_image_index,
    };
    const VkResult present_result = vkQueuePresentKHR(s_queue, &present_info);

    switch (present_result) {
        case VK_SUCCESS: return HG_SUCCESS;
        case VK_SUBOPTIMAL_KHR: return HG_ERROR_WINDOW_INVALID;
        case VK_ERROR_OUT_OF_DATE_KHR: return HG_ERROR_WINDOW_INVALID;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }
}

void hg_renderpass_begin(HgTexture* target, HgTexture* depth_buffer, bool clear_target, bool clear_depth) {
    if (s_recording_pass)
        hg_renderpass_end();
    s_recording_pass = true;

    HG_ASSERT(target != NULL);

    VkImageMemoryBarrier2 barriers[4];
    u32 barrier_count = 0;

    if (target->layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barriers[barrier_count] = (VkImageMemoryBarrier2){
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = target->layout,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .image = target->handle,
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };
        target->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ++barrier_count;
    }

    if (depth_buffer != NULL) {
        if (depth_buffer->layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barriers[barrier_count] = (VkImageMemoryBarrier2){
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .oldLayout = depth_buffer->layout,
                .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .image = depth_buffer->handle,
                .subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
            };
            depth_buffer->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            ++barrier_count;
        }
    }

    if (s_previous_target != NULL && s_previous_target != target) {
        if (s_previous_target->layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barriers[barrier_count] = (VkImageMemoryBarrier2){
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = s_previous_target->layout,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .image = s_previous_target->handle,
                .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };
            s_previous_target->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ++barrier_count;
        }
    }

    if (s_previous_depth_buffer != NULL && s_previous_depth_buffer != depth_buffer) {
        if (s_previous_depth_buffer->layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barriers[barrier_count] = (VkImageMemoryBarrier2){
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = s_previous_depth_buffer->layout,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .image = s_previous_depth_buffer->handle,
                .subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
            };
            s_previous_depth_buffer->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ++barrier_count;
        }
    }

    if (barrier_count > 0) {
        vkCmdPipelineBarrier2(s_current_cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = barrier_count,
            .pImageMemoryBarriers = barriers,
        });
    }

    VkRenderingInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, {s_swapchain_width, s_swapchain_height}},
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &(VkRenderingAttachmentInfo){
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = target->view,
            .imageLayout = target->layout,
            .loadOp = clear_target ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = (VkClearValue){
                .color = {{0.0f, 0.0f, 0.0f, 0.0f}}
            },
        },
        .pDepthAttachment = depth_buffer != NULL
            ? &(VkRenderingAttachmentInfo){
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = depth_buffer->view,
                .imageLayout = depth_buffer->layout,
                .loadOp = clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = (VkClearValue){
                    .depthStencil = {1.0f, 0},
                },
            }
            : NULL,
    };

    vkCmdBeginRendering(s_current_cmd, &render_info);

    vkCmdSetViewport(s_current_cmd, 0, 1, &(VkViewport){
        0.0f, 0.0f,
        (f32)target->width, (f32)target->height,
        0.0f, 1.0f,
    });
    vkCmdSetScissor(s_current_cmd, 0, 1, &(VkRect2D){
        {0, 0},
        {target->width, target->height}
    });

    s_previous_target = target;
    s_previous_depth_buffer = depth_buffer;
}

void hg_renderpass_end(void) {
    HG_ASSERT(s_recording_pass);

    vkCmdEndRendering(s_current_cmd);
    s_recording_pass = false;
}

void hg_shader_bind(HgShader* shader) {
    HG_ASSERT(shader != NULL);

    vkCmdBindPipeline(s_current_cmd, shader->bind_point, shader->pipeline);
    s_current_shader = shader;
}

void hg_shader_unbind(void) {
    s_current_shader = NULL;
}

static VkDescriptorSet hg_allocate_descriptor_set(HgShader* shader, u32 set_index) {
    HG_ASSERT(shader != NULL);
    HG_ASSERT(set_index < shader->descriptor_layout_count);

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = hg_current_descriptor_pool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &shader->descriptor_layouts[set_index],
    };
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    VkResult result = vkAllocateDescriptorSets(s_device, &alloc_info, &descriptor_set);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_FRAGMENTED_POOL: HG_ERROR("Vulkan fragmented pool"); // : TODO
        case VK_ERROR_OUT_OF_POOL_MEMORY: HG_ERROR("Vulkan ran out of descriptor pool memory"); // : TODO
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED_EXT: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    return descriptor_set;
}

void hg_bind_descriptor_set(u32 set_index, HgDescriptor* descriptors, u32 descriptor_count) {
    HG_ASSERT(set_index < s_current_shader->descriptor_layout_count);
    HG_ASSERT(descriptors != NULL);
    HG_ASSERT(descriptor_count > 0);

    VkDescriptorSet descriptor_set = hg_allocate_descriptor_set(s_current_shader, set_index);
    HG_ASSERT(descriptor_set != VK_NULL_HANDLE);

    // write descriptors more efficiently : TODO
    for (u32 i = 0; i < descriptor_count; ++i) {
        for (u32 j = 0; j < descriptors[i].count; ++j) {
            VkDescriptorBufferInfo buffer_info;
            VkDescriptorImageInfo image_info;

            VkWriteDescriptorSet descriptor_write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_set,
                .dstBinding = i,
                .dstArrayElement = j,
                .descriptorCount = 1,
                .descriptorType = hg_descriptor_type_to_vk(descriptors[i].type),
                .pBufferInfo = &buffer_info,
                .pImageInfo = &image_info,
            };

            switch (descriptors[i].type) {
                case HG_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case HG_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
                    HG_ASSERT(descriptors[i].buffers[j]->handle != VK_NULL_HANDLE);
                    buffer_info = (VkDescriptorBufferInfo){
                        .buffer = descriptors[i].buffers[j]->handle,
                        .offset = 0,
                        .range = descriptors[i].buffers[j]->size,
                    };
                } break;
                case HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE: {
                    HG_ASSERT(descriptors[i].textures[j]->sampler != VK_NULL_HANDLE);
                    HG_ASSERT(descriptors[i].textures[j]->view != VK_NULL_HANDLE);
                    image_info = (VkDescriptorImageInfo){
                        .sampler = descriptors[i].textures[j]->sampler,
                        .imageView = descriptors[i].textures[j]->view,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    };
                } break;
                case HG_DESCRIPTOR_TYPE_STORAGE_TEXTURE: {
                    HG_ASSERT(descriptors[i].textures[j]->view != VK_NULL_HANDLE);
                    image_info = (VkDescriptorImageInfo){
                        .imageView = descriptors[i].textures[j]->view,
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                    };
                } break;
                default: HG_ERROR("Unhandled descriptor type");
            }

            vkUpdateDescriptorSets(s_device, 1, &descriptor_write, 0, NULL);
        }
    }

    vkCmdBindDescriptorSets(
        s_current_cmd,
        s_current_shader->bind_point,
        s_current_shader->layout,
        set_index,
        1, &descriptor_set,
        0, NULL
    );
}

void hg_bind_push_constant(void* data, u32 size) {
    HG_ASSERT(data != NULL);
    HG_ASSERT(size > 0);

    VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    if (s_current_shader->bind_point == VK_PIPELINE_BIND_POINT_COMPUTE)
        stages = VK_SHADER_STAGE_COMPUTE_BIT;
    vkCmdPushConstants(s_current_cmd, s_current_shader->layout, stages, 0, size, data);
}

void hg_draw(HgBuffer* vertex_buffer, HgBuffer* index_buffer, u32 vertex_count) {
    if (vertex_buffer != NULL)
        vkCmdBindVertexBuffers(s_current_cmd, 0, 1, &vertex_buffer->handle, &(VkDeviceSize){0});
    if (index_buffer != NULL) {
        vkCmdBindIndexBuffer(s_current_cmd, index_buffer->handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(s_current_cmd, (u32)(index_buffer->size / sizeof(u32)), 1, 0, 0, 0);
    } else {
        vkCmdDraw(s_current_cmd, vertex_count, 1, 0, 0);
    }
}

void hg_compute_dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z) {
    HG_ASSERT(group_count_x > 0);
    HG_ASSERT(group_count_y > 0);
    HG_ASSERT(group_count_z > 0);

    vkCmdDispatch(s_current_cmd, group_count_x, group_count_y, group_count_z);
}

