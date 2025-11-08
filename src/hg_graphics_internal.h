#ifndef HG_GRAPHICS_INTERNAL_H
#define HG_GRAPHICS_INTERNAL_H

#include "hurdygurdy.h"

#include <vulkan/vulkan.h>

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
    HgShader* current_shader;
    HgTexture* previous_render_target;
    HgTexture* previous_depth_buffer;
};

#endif // HG_GRAPHICS_INTERNAL_H
