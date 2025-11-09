#ifndef HG_INTERNAL_H
#define HG_INTERNAL_H

#include "hurdygurdy.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct HurdyGurdy {
    void* platform_internals;

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
    VkDescriptorPool descriptor_pool;
};

void hg_init_platform_internals(HurdyGurdy* hg);
void hg_shutdown_platform_internals(HurdyGurdy* hg);

u32 hg_platform_get_vulkan_instance_extensions(
    const char** extension_buffer,
    u32 extension_buffer_capacity
);

struct HgBuffer {
    VmaAllocation allocation;
    VkBuffer handle;
    usize size;
    HgGpuMemoryType memory_type;
};

struct HgTexture {
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
};

struct HgShader {
    VkPipelineLayout layout;
    VkPipeline pipeline;
    u32 descriptor_layout_count;
    VkPipelineBindPoint bind_point;
    VkDescriptorSetLayout descriptor_layouts[];
};

struct HgCommands {
    VkCommandBuffer cmd;
    VkDevice device;
    VkDescriptorPool descriptor_pool;
    HgShader* shader;
    HgTexture* previous_target;
    HgTexture* previous_depth_buffer;
};

#define HG_SWAPCHAIN_MAX_IMAGES 4
#define HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT 2

struct HgWindow {
    void* platform_internals;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkImage swapchain_images[HG_SWAPCHAIN_MAX_IMAGES];
    u32 swapchain_image_count;
    VkFormat swapchain_format;
    u32 swapchain_width;
    u32 swapchain_height;

    VkCommandBuffer command_buffers[HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT];
    VkDescriptorPool descriptor_pools[HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT];
    VkFence frame_finished_fences[HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT];
    VkSemaphore image_available_semaphores[HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT];
    VkSemaphore ready_to_present_semaphores[HG_SWAPCHAIN_MAX_IMAGES];
    u32 current_image_index;
    u32 current_frame_index;

    HgCommands current_commands;

    f32 mouse_pos_x;
    f32 mouse_pos_y;
    f32 mouse_delta_x;
    f32 mouse_delta_y;
    bool keys_down[HG_KEY_LAST];
    bool keys_pressed[HG_KEY_LAST];
    bool keys_released[HG_KEY_LAST];
    bool was_closed;
    bool was_resized;
};

void hg_window_create_platform_internals(
    const HurdyGurdy* hg,
    const HgWindowConfig* config,
    HgWindow* window
);

void hg_window_destroy_platform_internals(
    const HurdyGurdy* hg,
    HgWindow* window
);

void hg_window_update_swapchain(
    const HurdyGurdy* hg,
    HgWindow* window
);

#endif // HG_INTERNAL_H
