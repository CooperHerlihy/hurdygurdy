#pragma once

#include "hg_utils.h"
#include "hg_load.h"

namespace hg {

struct Vk;
[[nodiscard]] Result<Vk> create_vk();
void destroy_vk(Vk& vk);

enum class GpuMemoryType {
    DeviceLocal = 0,
    RandomAccess,
    LinearAccess,
};

struct GpuBuffer {
    VmaAllocation allocation = nullptr;
    VkBuffer handle = nullptr;
    VkDeviceSize size = 0;
    GpuMemoryType type{};
};
void destroy_buffer(Vk& vk, const GpuBuffer& buffer);

struct GpuBufferView {
    VkBuffer handle = nullptr;
    VkDeviceSize range = 0;
    VkDeviceSize offset = 0;
};

struct GpuBufferConfig {
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage{};
    GpuMemoryType memory_type = GpuMemoryType::DeviceLocal;
};
[[nodiscard]] GpuBuffer create_buffer(Vk& vk, const GpuBufferConfig& config);
void write_buffer(Vk& vk, const GpuBuffer& dst, const void* src, usize size, usize offset = 0);

struct GpuImage {
    VmaAllocation allocation = nullptr;
    VkImage image{};
    VkExtent3D extent{};
    VkFormat format{};
    u32 mip_levels = 0;
    VkImageLayout layout{};
};
void destroy_image(Vk& vk, const GpuImage& image);

struct GpuImageConfig {
    VkExtent3D extent{};
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkImageUsageFlags usage{};
    u32 mip_levels = 1;
    VkSampleCountFlagBits sample_count{VK_SAMPLE_COUNT_1_BIT};
};
[[nodiscard]] GpuImage create_image(Vk& vk, const GpuImageConfig& config);

struct GpuCubemapConfig {
    VkExtent3D face_extent{};
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkImageUsageFlags usage{};
};
[[nodiscard]] GpuImage create_cubemap(Vk& vk, const GpuImageConfig& config);

struct GpuImageWriteConfig {
    VkImageLayout final_layout{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkImageAspectFlags aspect_flags{VK_IMAGE_ASPECT_COLOR_BIT};
};
void write_image(Vk& vk, GpuImage& dst, const ImageData& src, const GpuImageWriteConfig& config);
void write_cubemap(Vk& vk, GpuImage& dst, const ImageData& src, const GpuImageWriteConfig& config);

inline u32 get_mip_count(const VkExtent3D extent) {
    ASSERT(extent.width > 0 || extent.height > 0 || extent.depth > 0);
    return static_cast<u32>(std::floor(std::log2(std::max(std::max(extent.width, extent.height), extent.depth)))) + 1;
}
void generate_mipmaps(Vk& vk, GpuImage& image, VkImageLayout final_layout);

struct GpuImageViewConfig {
    VkImage image{};
    VkFormat format{};
    VkImageAspectFlags aspect_flags{VK_IMAGE_ASPECT_COLOR_BIT};
};
[[nodiscard]] VkImageView create_image_view(Vk& vk, const GpuImageViewConfig& config);
[[nodiscard]] VkImageView create_cubemap_view(Vk& vk, const GpuImageViewConfig& config);

struct GpuImageAndView {
    GpuImage image;
    VkImageView view;
};
void destroy_image_and_view(Vk& vk, const GpuImageAndView& image_and_view);

struct GpuImageAndViewConfig {
    VkExtent3D extent{};
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkImageUsageFlags usage{};
    VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
    VkImageAspectFlags aspect_flags{VK_IMAGE_ASPECT_COLOR_BIT};
    u32 mip_levels = 1;
    VkSampleCountFlagBits sample_count{VK_SAMPLE_COUNT_1_BIT};
};
[[nodiscard]] GpuImageAndView create_image_and_view(Vk& vk, const GpuImageAndViewConfig& config);

struct GpuCubemapAndViewConfig {
    ImageData data{};
    VkFormat format{VK_FORMAT_R8G8B8A8_SRGB};
    VkImageAspectFlags aspect_flags{VK_IMAGE_ASPECT_COLOR_BIT};
};
[[nodiscard]] GpuImageAndView create_cubemap_and_view(Vk& vk, const GpuCubemapAndViewConfig& config);

enum class SamplerType {
    Nearest = VK_FILTER_NEAREST,
    Linear = VK_FILTER_LINEAR,
};

struct SamplerConfig {
    SamplerType type = SamplerType::Nearest;
    VkSamplerAddressMode edge_mode{VK_SAMPLER_ADDRESS_MODE_REPEAT};
    u32 mip_levels = 1;
};
[[nodiscard]] VkSampler create_sampler(Vk& vk, const SamplerConfig& config);

struct Texture {
    GpuImageAndView image;
    VkSampler sampler;
};
void destroy_texture(Vk& vk, const Texture& texture);

struct TextureConfig {
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkImageAspectFlags aspect_flags{VK_IMAGE_ASPECT_COLOR_BIT};
    bool create_mips = false;
    SamplerType sampler_type = SamplerType::Nearest;
    VkSamplerAddressMode edge_mode{VK_SAMPLER_ADDRESS_MODE_REPEAT};
};
[[nodiscard]] Texture create_texture(Vk& vk, const ImageData& data, const TextureConfig& config);
[[nodiscard]] Texture create_texture_cubemap(Vk& vk, const ImageData& data, const TextureConfig& config);

struct DescriptorSetLayoutConfig {
    Slice<const VkDescriptorSetLayoutBinding> bindings;
    Slice<const VkDescriptorBindingFlags> flags = {};
};
[[nodiscard]] VkDescriptorSetLayout create_descriptor_set_layout(Vk& vk, const DescriptorSetLayoutConfig& config);

struct DescriptorPoolConfig {
    u32 max_sets = 0;
    Slice<const VkDescriptorPoolSize> descriptors{};
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
    u32 array_index = 0;
};
void write_image_sampler_descriptor(Vk& vk, const DescriptorSetBinding& binding, const Texture& texture);
void write_uniform_buffer_descriptor(Vk& vk, const DescriptorSetBinding& binding, const GpuBufferView& buffer);

struct PipelineLayoutConfig {
    Slice<const VkDescriptorSetLayout> set_layouts{};
    Slice<const VkPushConstantRange> push_ranges{};
};
[[nodiscard]] VkPipelineLayout create_pipeline_layout(Vk& vk, const PipelineLayoutConfig& config);

struct GraphicsPipeline {
    VkPipelineLayout layout{};
    std::array<VkShaderEXT, 2> shaders{};
};
void destroy_graphics_pipeline(Vk& vk, const GraphicsPipeline& pipeline);

struct GraphicsPipelineConfig {
    Slice<const VkDescriptorSetLayout> set_layouts{};
    Slice<const VkPushConstantRange> push_ranges{};
    std::filesystem::path vertex_shader_path{};
    std::filesystem::path fragment_shader_path{};
    VkShaderCodeTypeEXT code_type{VK_SHADER_CODE_TYPE_SPIRV_EXT};
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
        usize memory_barriers = 0;
        usize buffer_barriers = 0;
        usize image_barriers = 0;
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
    VkImage image = nullptr;
    VkOffset3D begin{};
    VkOffset3D end{};
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t mip_level = 0;
    uint32_t array_layer = 0;
    uint32_t layer_count = 1;
};
void blit_image(VkCommandBuffer cmd, const BlitConfig& dst, const BlitConfig& src, VkFilter filter);

struct ResolveConfig {
    VkImage image = nullptr;
    VkExtent3D extent{};
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t mip_level = 0;
    uint32_t array_layer = 0;
    uint32_t layer_count = 1;
};
void resolve_image(VkCommandBuffer cmd, const ResolveConfig& dst, const ResolveConfig& src);

void draw_vertices(VkCommandBuffer cmd, VkBuffer vertex_buffer, u32 vertex_count);
void draw_indexed(VkCommandBuffer cmd, VkBuffer vertex_buffer, VkBuffer index_buffer, u32 index_count);

[[nodiscard]] VkSurfaceKHR create_surface(Vk& vk, SDL_Window* window);

struct Swapchain {
    static constexpr u32 MaxFramesInFlight = 2;
    static constexpr u32 MaxImages = 3;
    static constexpr VkColorSpaceKHR ColorSpace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    VkExtent2D extent{};
    VkSwapchainKHR swapchain{};
    std::array<VkImage, MaxImages> images{};
    u32 image_count = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;

    u32 current_image_index = 0;
    u32 current_frame_index = 0;
    bool recording = false;

    std::array<VkCommandBuffer, MaxFramesInFlight> command_buffers{};
    std::array<VkFence, MaxFramesInFlight> frame_finished_fences{};
    std::array<VkSemaphore, MaxFramesInFlight> image_available_semaphores{};
    std::array<VkSemaphore, MaxImages> ready_to_present_semaphores{};

    [[nodiscard]] VkCommandBuffer& current_cmd() { return command_buffers[current_frame_index]; }
    [[nodiscard]] VkImage& current_image() { return images[current_image_index]; }
    [[nodiscard]] VkFence& is_frame_finished() { return frame_finished_fences[current_frame_index]; }
    [[nodiscard]] VkSemaphore& is_image_available() { return image_available_semaphores[current_frame_index]; }
    [[nodiscard]] VkSemaphore& is_ready_to_present() { return ready_to_present_semaphores[current_image_index]; }
    [[nodiscard]] const VkCommandBuffer& current_cmd() const { return command_buffers[current_frame_index]; }
    [[nodiscard]] const VkImage& current_image() const { return images[current_image_index]; }
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
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT{};
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{};

    PFN_vkCreateShadersEXT vkCreateShadersEXT{};
    PFN_vkDestroyShaderEXT vkDestroyShaderEXT{};

    PFN_vkCmdSetPolygonModeEXT vkCmdSetPolygonModeEXT{};
    PFN_vkCmdSetRasterizationSamplesEXT vkCmdSetRasterizationSamplesEXT{};
    PFN_vkCmdSetSampleMaskEXT vkCmdSetSampleMaskEXT{};
    PFN_vkCmdSetAlphaToCoverageEnableEXT vkCmdSetAlphaToCoverageEnableEXT{};
    PFN_vkCmdSetColorWriteMaskEXT vkCmdSetColorWriteMaskEXT{};
    PFN_vkCmdSetColorBlendEnableEXT vkCmdSetColorBlendEnableEXT{};
    PFN_vkCmdBindShadersEXT vkCmdBindShadersEXT{};
    PFN_vkCmdSetVertexInputEXT vkCmdSetVertexInputEXT{};
};

inline VulkanPFNs g_pfn;

void load_instance_procedures(VkInstance instance);
void load_device_procedures(VkDevice device);

struct Vk {
    Arena stack{};

    VkInstance instance{};
    VkDebugUtilsMessengerEXT debug_messenger{};

    VkPhysicalDevice gpu{};
    VkDevice device{};
    VmaAllocator gpu_allocator = nullptr;

    u32 queue_family_index = UINT32_MAX;
    VkQueue queue{};

    VkCommandPool command_pool{};
    VkCommandPool single_time_command_pool{};
};

} // namespace hg
