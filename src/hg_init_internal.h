#ifndef HG_INIT_INTERNAL
#define HG_INIT_INTERNAL

#include "hg_init.h"

#ifdef __unix__
#include <dlfcn.h>
#elif _WIN32 || _WIN64
#error Windows not supported yet
#endif

#include <vulkan/vulkan.h>

#define HG_SWAPCHAIN_MAX_IMAGES 4
#define HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT 2

struct HurdyGurdy {
    void* libvulkan;

    VkInstance instance;
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

    VkPhysicalDevice gpu;
    VkDevice device;
    u32 queue_family_index;
    VkQueue queue;

    VmaAllocator allocator;
    VkCommandPool command_pool;
    VkDescriptorPool generic_descriptor_pool;
    VkDescriptorPool frame_descriptor_pools[HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT];
};

void hg_init_vulkan_load(HurdyGurdy* hg);
void hg_init_vulkan_load_instance(HurdyGurdy* hg, VkInstance instance);
void hg_init_vulkan_load_device(HurdyGurdy* hg, VkDevice device);
void hg_init_vulkan_close(HurdyGurdy* hg);

#endif // HG_INIT_INTERNAL
