#pragma once

#include "hg_utils.h"

namespace hg {

struct Vk;
[[nodiscard]] Result<Vk> create_vk();
void destroy_vk(Vk& vk);

enum GpuMemoryType {
    GpuMemoryDeviceLocal = 0,
    GpuMemoryRandomAccess,
    GpuMemoryLinearAccess,
};

struct GpuBuffer {
    VmaAllocation allocation;
    VkBuffer handle;
    VkDeviceSize size;
    GpuMemoryType type;
};
void destroy_buffer(Vk& vk, const GpuBuffer& buffer);

struct GpuBufferView {
    VkBuffer handle;
    VkDeviceSize range;
    VkDeviceSize offset;
};

struct GpuBufferConfig {
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    GpuMemoryType memory_type;
};
[[nodiscard]] GpuBuffer create_buffer(Vk& vk, const GpuBufferConfig& config);
void write_buffer(Vk& vk, const GpuBuffer& dst, const void* src, usize size, usize offset = 0);

struct GpuImage {
    VmaAllocation allocation;
    VkImage handle;
    VkExtent3D extent;
    VkFormat format;
    u32 mip_levels;
    VkImageLayout layout;
};
void destroy_gpu_image(Vk& vk, const GpuImage& image);

struct GpuImageConfig {
    VkExtent3D extent;
    VkFormat format;
    VkImageUsageFlags usage;
    u32 mip_levels = 1;
};
[[nodiscard]] GpuImage create_gpu_image(Vk& vk, const GpuImageConfig& config);

struct GpuCubemapConfig {
    VkExtent3D face_extent;
    VkFormat format;
    VkImageUsageFlags usage;
};
[[nodiscard]] GpuImage create_gpu_cubemap(Vk& vk, const GpuCubemapConfig& config);

struct GpuImageWriteConfig {
    Slice<const void> src;
    VkImageLayout final_layout;
    VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
};
void write_gpu_image(Vk& vk, GpuImage& dst, const GpuImageWriteConfig& config);
void write_gpu_cubemap(Vk& vk, GpuImage& dst, const GpuImageWriteConfig& config);

inline u32 get_mip_count(const VkExtent3D extent) {
    ASSERT(extent.width > 0 || extent.height > 0 || extent.depth > 0);
    return static_cast<u32>(std::floor(std::log2(std::max(std::max(extent.width, extent.height), extent.depth)))) + 1;
}
void generate_gpu_image_mipmaps(Vk& vk, GpuImage& image, VkImageLayout final_layout);

struct GpuImageViewConfig {
    VkImage image;
    VkFormat format;
    VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
};
[[nodiscard]] VkImageView create_gpu_image_view(Vk& vk, const GpuImageViewConfig& config);
[[nodiscard]] VkImageView create_gpu_cubemap_view(Vk& vk, const GpuImageViewConfig& config);

enum SamplerType {
    SamplerNearest = VK_FILTER_NEAREST,
    SamplerLinear = VK_FILTER_LINEAR,
};

struct SamplerConfig {
    SamplerType type;
    VkSamplerAddressMode edge_mode;
    u32 mip_levels = 1;
};
[[nodiscard]] VkSampler create_sampler(Vk& vk, const SamplerConfig& config);

struct DescriptorSetLayoutConfig {
    Slice<const VkDescriptorSetLayoutBinding> bindings;
    Slice<const VkDescriptorBindingFlags> flags;
};
[[nodiscard]] VkDescriptorSetLayout create_descriptor_set_layout(Vk& vk, const DescriptorSetLayoutConfig& config);

struct DescriptorPoolConfig {
    u32 max_sets;
    Slice<const VkDescriptorPoolSize> descriptors;
};
[[nodiscard]] VkDescriptorPool create_descriptor_pool(Vk& vk, const DescriptorPoolConfig& config);

[[nodiscard]] Result<void> allocate_descriptor_sets(
    Vk& vk,
    const VkDescriptorPool pool,
    const Slice<const VkDescriptorSetLayout> layouts,
    const Slice<VkDescriptorSet> out_sets
);
[[nodiscard]] inline Result<VkDescriptorSet> allocate_descriptor_set(
    Vk& vk,
    const VkDescriptorPool pool,
    const VkDescriptorSetLayout layout
) {
    auto set = ok<VkDescriptorSet>();
    const auto alloc_result = allocate_descriptor_sets(vk, pool, {&layout, 1}, {&*set, 1});
    if (alloc_result.has_err())
        return alloc_result.err();
    return set;
}

struct DescriptorSetBinding {
    VkDescriptorSet set;
    u32 binding_index;
    u32 array_index;
};
void write_image_sampler_descriptor(Vk& vk, const DescriptorSetBinding& binding, VkImageView image, VkSampler sampler);
void write_uniform_buffer_descriptor(Vk& vk, const DescriptorSetBinding& binding, const GpuBufferView& buffer);

struct PipelineLayoutConfig {
    Slice<const VkDescriptorSetLayout> set_layouts;
    Slice<const VkPushConstantRange> push_ranges;
};
[[nodiscard]] VkPipelineLayout create_pipeline_layout(Vk& vk, const PipelineLayoutConfig& config);

struct GraphicsPipeline {
    VkPipelineLayout layout;
    std::array<VkShaderEXT, 2> shaders;
};
void destroy_graphics_pipeline(Vk& vk, const GraphicsPipeline& pipeline);

struct GraphicsPipelineConfig {
    Slice<const VkDescriptorSetLayout> set_layouts;
    Slice<const VkPushConstantRange> push_ranges;
    std::filesystem::path vertex_shader_path;
    std::filesystem::path fragment_shader_path;
    VkShaderCodeTypeEXT code_type = VK_SHADER_CODE_TYPE_SPIRV_EXT;
};
[[nodiscard]] Result<GraphicsPipeline> create_graphics_pipeline(Vk& vk, const GraphicsPipelineConfig& config);

void bind_shaders(VkCommandBuffer cmd, const GraphicsPipeline& pipeline);

[[nodiscard]] VkFence create_fence(Vk& vk, const VkFenceCreateFlags flags = {});
void wait_for_fence(Vk& vk, const VkFence fence);
void reset_fence(Vk& vk, const VkFence fence);

[[nodiscard]] VkSemaphore create_semaphore(Vk& vk);

void allocate_command_buffers(Vk& vk, const Slice<VkCommandBuffer> out_cmds);

[[nodiscard]] VkCommandBuffer begin_single_time_commands(Vk& vk);
void end_single_time_commands(Vk& vk, VkCommandBuffer cmd);
void submit_single_time_commands(Vk& vk, auto commands) {
    const VkCommandBuffer cmd = begin_single_time_commands(vk);
    commands(cmd);
    end_single_time_commands(vk, cmd);
}

class BarrierBuilder {
public:
    struct Config {
        usize memory_barriers;
        usize buffer_barriers;
        usize image_barriers;
    };
    BarrierBuilder(Vk& vk, const Config& config);
    void build_and_run(Vk& vk, const VkCommandBuffer cmd, const VkDependencyFlags flags = {});

    BarrierBuilder& add_memory_barrier(const usize index, const VkMemoryBarrier2& barrier) {
        ASSERT(index < m_memories.count);

        m_memories[index] = barrier;
        return *this;
    }
    BarrierBuilder& add_buffer_barrier(const usize index, const VkBufferMemoryBarrier2& barrier) {
        ASSERT(index < m_buffers.count);
        ASSERT(barrier.buffer != nullptr);

        m_buffers[index] = barrier;
        return *this;
    }
    BarrierBuilder& add_image_barrier(const usize index, const VkImageMemoryBarrier2& barrier) {
        ASSERT(index < m_images.count);
        ASSERT(barrier.image != nullptr);
        ASSERT(barrier.newLayout != VK_IMAGE_LAYOUT_UNDEFINED);

        m_images[index] = barrier;
        return *this;
    }
    BarrierBuilder& add_image_barrier(
        const usize index,
        const VkImage image,
        const VkImageSubresourceRange& subresource_range
    ) {
        ASSERT(image != nullptr);

        m_images[index].image = image;
        m_images[index].subresourceRange = subresource_range;
        return *this;
    }
    BarrierBuilder& set_image_src(
        const usize index,
        const VkPipelineStageFlags2 src_stage_mask,
        const VkAccessFlags2 src_access_mask,
        const VkImageLayout old_layout
    ) {
        m_images[index].srcStageMask = src_stage_mask;
        m_images[index].srcAccessMask = src_access_mask;
        m_images[index].oldLayout = old_layout;
        return *this;
    }
    BarrierBuilder& set_image_dst(
        const usize index,
        const VkPipelineStageFlags2 dst_stage_mask,
        const VkAccessFlags2 dst_access_mask,
        const VkImageLayout new_layout
    ) {
        ASSERT(new_layout != VK_IMAGE_LAYOUT_UNDEFINED);

        m_images[index].dstStageMask = dst_stage_mask;
        m_images[index].dstAccessMask = dst_access_mask;
        m_images[index].newLayout = new_layout;
        return *this;
    }

private:
    Slice<VkMemoryBarrier2> m_memories{};
    Slice<VkBufferMemoryBarrier2> m_buffers{};
    Slice<VkImageMemoryBarrier2> m_images{};
};

void copy_to_buffer(VkCommandBuffer cmd, const GpuBufferView& dst, const GpuBufferView& src);
void copy_to_image(VkCommandBuffer cmd, GpuImage& dst, const GpuBuffer& src, VkImageAspectFlags aspect);

struct BlitConfig {
    VkImage image;
    VkOffset3D begin;
    VkOffset3D end;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t mip_level;
    uint32_t array_layer;
    uint32_t layer_count = 1;
};
void blit_image(VkCommandBuffer cmd, const BlitConfig& dst, const BlitConfig& src, VkFilter filter);

struct ResolveConfig {
    VkImage image;
    VkExtent3D extent;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t mip_level;
    uint32_t array_layer;
    uint32_t layer_count = 1;
};
void resolve_image(VkCommandBuffer cmd, const ResolveConfig& dst, const ResolveConfig& src);

void draw_vertices(VkCommandBuffer cmd, VkBuffer vertex_buffer, u32 vertex_count);
void draw_indexed(VkCommandBuffer cmd, VkBuffer vertex_buffer, VkBuffer index_buffer, u32 index_count);

[[nodiscard]] VkSurfaceKHR create_surface(Vk& vk, SDL_Window* window);

static constexpr u32 SwapchainMaxFramesInFlight = 2;
static constexpr u32 SwapchainMaxImages = 3;
static constexpr VkColorSpaceKHR SwapchainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

struct Swapchain {
    VkExtent2D extent;
    VkSwapchainKHR swapchain{};
    std::array<VkImage, SwapchainMaxImages> images;
    std::array<VkImageView, SwapchainMaxImages> image_views;
    u32 image_count;
    VkFormat format;

    u32 current_image_index;
    u32 current_frame_index;
    bool recording;

    VkCommandBuffer command_buffers[SwapchainMaxFramesInFlight];
    VkFence frame_finished_fences[SwapchainMaxFramesInFlight];
    VkSemaphore image_available_semaphores[SwapchainMaxFramesInFlight];
    VkSemaphore ready_to_present_semaphores[SwapchainMaxImages];

    [[nodiscard]] VkCommandBuffer& current_cmd() { return command_buffers[current_frame_index]; }
    [[nodiscard]] VkImage& current_image() { return images[current_image_index]; }
    [[nodiscard]] VkImageView& current_image_view() { return image_views[current_image_index]; }
    [[nodiscard]] VkFence& is_frame_finished() { return frame_finished_fences[current_frame_index]; }
    [[nodiscard]] VkSemaphore& is_image_available() { return image_available_semaphores[current_frame_index]; }
    [[nodiscard]] VkSemaphore& is_ready_to_present() { return ready_to_present_semaphores[current_image_index]; }
    [[nodiscard]] const VkCommandBuffer& current_cmd() const { return command_buffers[current_frame_index]; }
    [[nodiscard]] const VkImage& current_image() const { return images[current_image_index]; }
    [[nodiscard]] const VkImageView& current_image_view() const { return image_views[current_image_index]; }
    [[nodiscard]] const VkFence& is_frame_finished() const { return frame_finished_fences[current_frame_index]; }
    [[nodiscard]] const VkSemaphore& is_image_available() const {
        return image_available_semaphores[current_frame_index];
    }
    [[nodiscard]] const VkSemaphore& is_ready_to_present() const {
        return ready_to_present_semaphores[current_image_index];
    }
};
void destroy_swapchain(Vk& vk, const Swapchain& swapchain);

[[nodiscard]] Result<Swapchain> create_swapchain(Vk& vk, const VkSurfaceKHR surface);
[[nodiscard]] Result<void> resize_swapchain(Vk& vk, Swapchain& swapchain, const VkSurfaceKHR surface);

[[nodiscard]] Result<VkCommandBuffer> begin_frame(Vk& vk, Swapchain& swapchain);
[[nodiscard]] Result<void> end_frame(Vk& vk, Swapchain& swapchain);

struct VulkanPFNs {
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

    PFN_vkCreateShadersEXT vkCreateShadersEXT;
    PFN_vkDestroyShaderEXT vkDestroyShaderEXT;

    PFN_vkCmdSetPolygonModeEXT vkCmdSetPolygonModeEXT;
    PFN_vkCmdSetRasterizationSamplesEXT vkCmdSetRasterizationSamplesEXT;
    PFN_vkCmdSetSampleMaskEXT vkCmdSetSampleMaskEXT;
    PFN_vkCmdSetAlphaToCoverageEnableEXT vkCmdSetAlphaToCoverageEnableEXT;
    PFN_vkCmdSetColorWriteMaskEXT vkCmdSetColorWriteMaskEXT;
    PFN_vkCmdSetColorBlendEnableEXT vkCmdSetColorBlendEnableEXT;
    PFN_vkCmdBindShadersEXT vkCmdBindShadersEXT;
    PFN_vkCmdSetVertexInputEXT vkCmdSetVertexInputEXT;
};

inline VulkanPFNs g_pfn;

void load_instance_procedures(VkInstance instance);
void load_device_procedures(VkDevice device);

struct Vk {
    Arena stack;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    VkPhysicalDevice gpu;
    VkDevice device;
    VmaAllocator gpu_allocator;

    u32 queue_family_index = UINT32_MAX;
    VkQueue queue;

    VkCommandPool command_pool;
    VkCommandPool single_time_command_pool;
};

} // namespace hg
