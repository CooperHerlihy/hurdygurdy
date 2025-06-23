#pragma once

#include "hg_pch.h"
#include "hg_utils.h"

#include <array>
#include <filesystem>
#include <span>
#include <vulkan/vulkan_enums.hpp>

namespace hg {

struct Engine {
    vk::Instance instance = {};
    vk::DebugUtilsMessengerEXT debug_messenger = {};
    vk::PhysicalDevice gpu = {};
    vk::Device device = {};
    VmaAllocator allocator = nullptr;

    u32 queue_family_index = UINT32_MAX;
    vk::Queue queue = {};

    vk::CommandPool command_pool = {};
    vk::CommandPool single_time_command_pool = {};

    [[nodiscard]] static Result<Engine> create();
    void destroy() const;
};

constexpr u32 MaxFramesInFlight = 2;
constexpr u32 MaxSwapchainImages = 3;

class Pipeline {
public:
    virtual ~Pipeline() = default;
    virtual void cmd_draw(vk::CommandBuffer cmd, vk::Image render_target, vk::Extent2D window_size) const = 0;
};

class Window {
public:
    static constexpr vk::Format SwapchainImageFormat = vk::Format::eR8G8B8A8Srgb;
    static constexpr vk::ColorSpaceKHR SwapchainColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

    [[nodiscard]] static Result<Window> create(const Engine& engine, bool fullscreen, i32 width, i32 height);
    void destroy(const Engine& engine) const;
    [[nodiscard]] Result<void> resize(const Engine& engine);

    [[nodiscard]] GLFWwindow* window() const { return m_window; }
    [[nodiscard]] vk::Extent2D extent() const { return m_extent; }

    [[nodiscard]] Result<vk::CommandBuffer> begin_frame(const Engine& engine);
    [[nodiscard]] Result<void> end_frame(const Engine& engine);
    [[nodiscard]] Result<void> submit_frame(const Engine& engine, const Pipeline& pipeline) {
        const auto cmd = begin_frame(engine);
        if (cmd.has_err())
            return cmd.err();
        pipeline.cmd_draw(*cmd, current_image(), m_extent);
        return end_frame(engine);
    }

private:
    [[nodiscard]] vk::CommandBuffer& current_cmd() { return m_command_buffers[m_current_frame_index]; }
    [[nodiscard]] vk::Image& current_image() { return m_swapchain_images[m_current_image_index]; }
    [[nodiscard]] vk::Fence& is_frame_finished() { return m_frame_finished_fences[m_current_frame_index]; }
    [[nodiscard]] vk::Semaphore& is_image_available() { return m_image_available_semaphores[m_current_frame_index]; }
    [[nodiscard]] vk::Semaphore& is_ready_to_present() { return m_ready_to_present_semaphores[m_current_image_index]; }
    [[nodiscard]] const vk::CommandBuffer& current_cmd() const { return m_command_buffers[m_current_frame_index]; }
    [[nodiscard]] const vk::Image& current_image() const { return m_swapchain_images[m_current_image_index]; }
    [[nodiscard]] const vk::Fence& is_frame_finished() const { return m_frame_finished_fences[m_current_frame_index]; }
    [[nodiscard]] const vk::Semaphore& is_image_available() const { return m_image_available_semaphores[m_current_frame_index]; }
    [[nodiscard]] const vk::Semaphore& is_ready_to_present() const { return m_ready_to_present_semaphores[m_current_image_index]; }

    GLFWwindow* m_window = nullptr;
    vk::SurfaceKHR m_surface = {};
    vk::Extent2D m_extent = {};
    vk::SwapchainKHR m_swapchain = {};
    std::array<vk::Image, MaxSwapchainImages> m_swapchain_images = {};
    u32 m_image_count = 0;
    u32 m_current_image_index = 0;
    u32 m_current_frame_index = 0;
    bool m_recording = false;
    std::array<vk::CommandBuffer, MaxFramesInFlight> m_command_buffers = {};
    std::array<vk::Fence, MaxFramesInFlight> m_frame_finished_fences = {};
    std::array<vk::Semaphore, MaxFramesInFlight> m_image_available_semaphores = {};
    std::array<vk::Semaphore, MaxSwapchainImages> m_ready_to_present_semaphores = {};
};

struct GpuBuffer {
    enum MemoryType {
        DeviceLocal,
        RandomAccess,
        Staging,
    };

    VmaAllocation allocation = nullptr;
    vk::Buffer buffer = {};
    MemoryType memory_type = DeviceLocal;

    struct Config {
        vk::DeviceSize size;
        vk::BufferUsageFlags usage;
        MemoryType memory_type = DeviceLocal;
    };

    [[nodiscard]] static Result<GpuBuffer> create_result(const Engine& engine, const Config& config);
    [[nodiscard]] static GpuBuffer create(const Engine& engine, const Config& config) {
        const auto buffer = create_result(engine, config);
        if (buffer.has_err())
            ERROR("Could not create gpu buffer");
        return *buffer;
    }
    void destroy(const Engine& engine) const {
        ASSERT(allocation != nullptr);
        ASSERT(buffer != nullptr);
        ASSERT(engine.allocator != nullptr);
        vmaDestroyBuffer(engine.allocator, buffer, allocation);
    }

    [[nodiscard]] Result<void> write_result(const Engine& engine, const void* data, vk::DeviceSize size, vk::DeviceSize offset) const;
    void write(const Engine& engine, const void* data, vk::DeviceSize size, vk::DeviceSize offset) const {
        const auto result = write_result(engine, data, size, offset);
        if (result.has_err())
            ERROR("Could not write to gpu buffer");
    }
    void write(const Engine& engine, const auto& data, const vk::DeviceSize offset = 0) const {
        write(engine, &data, sizeof(data), offset);
    };
};

struct StagingGpuImage {
    VmaAllocation allocation = nullptr;
    vk::Image image = {};

    struct Config {
        vk::Extent3D extent = {};
        vk::Format format = vk::Format::eUndefined;
        vk::ImageUsageFlags usage = {};
        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;
        u32 mip_levels = 1;
    };

    [[nodiscard]] static Result<StagingGpuImage> create(const Engine& engine, const Config& config);
    void destroy(const Engine& engine) const {
        ASSERT(allocation != nullptr);
        ASSERT(image != nullptr);
        ASSERT(engine.device != nullptr);
        vmaDestroyImage(engine.allocator, image, allocation);
    }

    struct Data {
        const void* ptr;
        u64 alignment;
        vk::Extent3D extent;
    };

    [[nodiscard]] Result<void> write(const Engine& engine, const Data& data, vk::ImageLayout final_layout, const vk::ImageSubresourceRange& subresource = {vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1}) const;
};

struct GpuImage {
    VmaAllocation allocation = nullptr;
    vk::Image image = {};
    vk::ImageView view = {};

    struct Config {
        vk::Extent3D extent = {};
        vk::Format format = vk::Format::eUndefined;
        vk::ImageUsageFlags usage = {};
        vk::ImageAspectFlagBits aspect_flags = vk::ImageAspectFlagBits::eColor;
        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
        u32 mip_levels = 1;
    };

    [[nodiscard]] static Result<GpuImage> create_result(const Engine& engine, const Config& config);
    [[nodiscard]] static GpuImage create(const Engine& engine, const Config& config) {
        const auto image = create_result(engine, config);
        if (image.has_err())
            ERROR("Could not create gpu image");
        return *image;
    }
    [[nodiscard]] static Result<GpuImage> create_cubemap(const Engine& engine, std::filesystem::path path);
    void destroy(const Engine& engine) const {
        ASSERT(view != nullptr);
        ASSERT(engine.device != nullptr);
        engine.device.destroyImageView(view);

        ASSERT(allocation != nullptr);
        ASSERT(image != nullptr);
        ASSERT(engine.allocator != nullptr);
        vmaDestroyImage(engine.allocator, image, allocation);
    }

    using Data = StagingGpuImage::Data;
    [[nodiscard]] Result<void> write_result(const Engine& engine, const Data& data, vk::ImageLayout final_layout,
                                            const vk::ImageSubresourceRange& subresource = {vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1}) const {
        return StagingGpuImage{allocation, image}.write(engine, data, final_layout, subresource);
    }
    void write(const Engine& engine, const Data& data, vk::ImageLayout final_layout,
               const vk::ImageSubresourceRange& subresource = {vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1}) const {
        const auto write_result = StagingGpuImage{allocation, image}.write(engine, data, final_layout, subresource);
        if (write_result.has_err())
            ERROR("Could not write to gpu image");
    }

    [[nodiscard]] Result<void> generate_mipmaps_result(const Engine& engine, u32 levels, vk::Extent3D extent, vk::Format format, vk::ImageLayout final_layout) const;
    void generate_mipmaps(const Engine& engine, u32 levels, vk::Extent3D extent, vk::Format format, vk::ImageLayout final_layout) const {
        const auto mipmap = generate_mipmaps_result(engine, levels, extent, format, final_layout);
        if (mipmap.has_err())
            ERROR("Could not generate mipmaps");
    }
};

inline u32 get_mip_count(const vk::Extent3D extent) {
    return static_cast<u32>(std::floor(std::log2(std::max(std::max(extent.width, extent.height), extent.depth)))) + 1;
}

enum class SamplerType {
    Nearest = VK_FILTER_NEAREST,
    Linear = VK_FILTER_LINEAR,
};
struct SamplerConfig {
    SamplerType type = SamplerType::Nearest;
    vk::SamplerAddressMode edge_mode = vk::SamplerAddressMode::eRepeat;
    u32 mip_levels = 1;
};
[[nodiscard]] Result<vk::Sampler> create_sampler_result(const Engine& engine, const SamplerConfig& config);
[[nodiscard]] inline vk::Sampler create_sampler(const Engine& engine, const SamplerConfig& config) {
    const auto sampler = create_sampler_result(engine, config);
    if (sampler.has_err())
        ERROR("Could not create VkSampler");
    return *sampler;
}

[[nodiscard]] Result<vk::DescriptorSetLayout> create_descriptor_set_layout(const Engine& engine, std::span<const vk::DescriptorSetLayoutBinding> bindings);

[[nodiscard]] Result<void> allocate_descriptor_sets(const Engine& engine, vk::DescriptorPool pool, std::span<const vk::DescriptorSetLayout> layouts, std::span<vk::DescriptorSet> out_sets);
[[nodiscard]] inline Result<vk::DescriptorSet> allocate_descriptor_set(const Engine& engine, vk::DescriptorPool pool, vk::DescriptorSetLayout layout) {
    auto set = ok<vk::DescriptorSet>();
    const auto alloc_result = allocate_descriptor_sets(engine, pool, {&layout, 1}, {&*set, 1});
    if (alloc_result.has_err())
        return alloc_result.err();
    return set;
}
void write_uniform_buffer_descriptor(const Engine& engine, vk::DescriptorSet set, u32 binding, vk::Buffer buffer, vk::DeviceSize size, vk::DeviceSize offset = 0);
void write_image_sampler_descriptor(const Engine& engine, vk::DescriptorSet set, u32 binding, vk::Sampler sampler, vk::ImageView view);

struct ShaderConfig {
    std::filesystem::path path = {};
    vk::ShaderCodeTypeEXT code_type = vk::ShaderCodeTypeEXT::eSpirv;
    vk::ShaderStageFlagBits stage = {};
    vk::ShaderStageFlags next_stage = {};
    std::span<const vk::DescriptorSetLayout> set_layouts = {};
    std::span<const vk::PushConstantRange> push_ranges = {};
    vk::ShaderCreateFlagsEXT flags = {};
};
[[nodiscard]] Result<vk::ShaderEXT> create_unlinked_shader(const Engine& engine, const ShaderConfig& config);
[[nodiscard]] Result<void> create_linked_shaders(const Engine& engine, std::span<vk::ShaderEXT> out_shaders, std::span<const ShaderConfig> configs);

[[nodiscard]] Result<vk::CommandBuffer> begin_single_time_commands(const Engine& engine);
[[nodiscard]] Result<void> end_single_time_commands(const Engine& engine, vk::CommandBuffer cmd);
[[nodiscard]] Result<void> submit_single_time_commands(const Engine& engine, const auto& commands) {
    const auto cmd = begin_single_time_commands(engine);
    if (cmd.has_err())
        return cmd.err();
    commands(*cmd);
    return end_single_time_commands(engine, *cmd);
}

class BarrierBuilder {
public:
    explicit constexpr BarrierBuilder(const vk::CommandBuffer cmd) : m_cmd(cmd) {}

    void build_and_run(const vk::DependencyFlagBits flags = {}) {
        m_cmd.pipelineBarrier2({
            .dependencyFlags = flags,
            .memoryBarrierCount = to_u32(m_memories.size()),
            .pMemoryBarriers = m_memories.data(),
            .bufferMemoryBarrierCount = to_u32(m_buffers.size()),
            .pBufferMemoryBarriers = m_buffers.data(),
            .imageMemoryBarrierCount = to_u32(m_images.size()),
            .pImageMemoryBarriers = m_images.data(),
        });
    }

    BarrierBuilder& add_memory_barrier(const vk::MemoryBarrier2& barrier) {
        m_memories.push_back(barrier);
        return *this;
    }
    BarrierBuilder& add_buffer_barrier(const vk::BufferMemoryBarrier2& barrier) {
        ASSERT(barrier.buffer != nullptr);
        m_buffers.push_back(barrier);
        return *this;
    }
    BarrierBuilder& add_image_barrier(const vk::ImageMemoryBarrier2& barrier) {
        ASSERT(barrier.image != nullptr);
        ASSERT(barrier.newLayout != vk::ImageLayout::eUndefined);
        m_images.push_back(barrier);
        return *this;
    }
    BarrierBuilder& add_image_barrier(const vk::Image image, const vk::ImageSubresourceRange& subresource_range) {
        ASSERT(image != nullptr);
        m_images.push_back({
            .image = image,
            .subresourceRange = subresource_range,
        });
        return *this;
    }
    BarrierBuilder& set_image_src(const vk::PipelineStageFlags2 src_stage_mask, const vk::AccessFlags2 src_access_mask, const vk::ImageLayout old_layout) {
        m_images.back().srcStageMask = src_stage_mask;
        m_images.back().srcAccessMask = src_access_mask;
        m_images.back().oldLayout = old_layout;
        return *this;
    }
    BarrierBuilder& set_image_dst(const vk::PipelineStageFlags2 dst_stage_mask, const vk::AccessFlags2 dst_access_mask, const vk::ImageLayout new_layout) {
        ASSERT(new_layout != vk::ImageLayout::eUndefined);
        m_images.back().dstStageMask = dst_stage_mask;
        m_images.back().dstAccessMask = dst_access_mask;
        m_images.back().newLayout = new_layout;
        return *this;
    }

private:
    vk::CommandBuffer m_cmd = {};
    std::vector<vk::MemoryBarrier2> m_memories = {};
    std::vector<vk::BufferMemoryBarrier2> m_buffers = {};
    std::vector<vk::ImageMemoryBarrier2> m_images = {};
};

} // namespace hg
