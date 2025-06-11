#pragma once

#include "hg_utils.h"

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

    [[nodiscard]] static Engine create();
    void destroy() const;
};

constexpr u32 MaxFramesInFlight = 2;
constexpr u32 MaxSwapchainImages = 3;

class Window {
public:
    GLFWwindow* window = nullptr;
    vk::SurfaceKHR surface = {};

    vk::SwapchainKHR swapchain = {};
    vk::Extent2D extent = {};
    vk::Format image_format = vk::Format::eUndefined;
    u32 image_count = 0;
    u32 current_image_index = 0;
    std::array<vk::Image, MaxSwapchainImages> swapchain_images = {};
    std::array<vk::ImageView, MaxSwapchainImages> swapchain_views = {};

    vk::CommandBuffer& current_cmd() { return m_command_buffers[m_current_frame_index]; }
    vk::Image& current_image() { return swapchain_images[current_image_index]; }
    vk::ImageView& current_view() { return swapchain_views[current_image_index]; }
    vk::Fence& is_frame_finished() { return m_frame_finished_fences[m_current_frame_index]; }
    vk::Semaphore& is_image_available() { return m_image_available_semaphores[m_current_frame_index]; }
    vk::Semaphore& is_ready_to_present() { return m_ready_to_present_semaphores[current_image_index]; }

    [[nodiscard]] static Window create(const Engine& engine, i32 width, i32 height);
    void destroy(const Engine& engine) const;
    void resize(const Engine& engine);

    [[nodiscard]] vk::CommandBuffer begin_frame(const Engine& engine);
    [[nodiscard]] bool end_frame(const Engine& engine);
    [[nodiscard]] bool submit_frame(const Engine& engine, const auto& commands) {
        const auto cmd = begin_frame(engine);
        commands(cmd);
        return end_frame(engine);
    }

private:
    u32 m_current_frame_index = 0;
    std::array<vk::CommandBuffer, MaxFramesInFlight> m_command_buffers = {};
    std::array<vk::Fence, MaxFramesInFlight> m_frame_finished_fences = {};
    std::array<vk::Semaphore, MaxFramesInFlight> m_image_available_semaphores = {};
    std::array<vk::Semaphore, MaxSwapchainImages> m_ready_to_present_semaphores = {};
    bool m_recording = false;
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

    [[nodiscard]] static GpuBuffer create(const Engine& engine, vk::DeviceSize size, vk::BufferUsageFlags usage, MemoryType memory_type = DeviceLocal);
    void destroy(const Engine& engine) const {
        debug_assert(allocation != nullptr);
        debug_assert(buffer != nullptr);
        debug_assert(engine.allocator != nullptr);
        vmaDestroyBuffer(engine.allocator, buffer, allocation);
    }

    void write(const Engine& engine, const void* data, vk::DeviceSize size, vk::DeviceSize offset) const;
    void write(const Engine& engine, const auto& data, const vk::DeviceSize offset = 0) const { write(engine, &data, sizeof(data), offset); };
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

    [[nodiscard]] static GpuImage create(const Engine& engine, const Config& config);
    void destroy(const Engine& engine) const {
        debug_assert(view != nullptr);
        debug_assert(engine.device != nullptr);
        engine.device.destroyImageView(view);

        debug_assert(allocation != nullptr);
        debug_assert(image != nullptr);
        debug_assert(engine.allocator != nullptr);
        vmaDestroyImage(engine.allocator, image, allocation);
    }

    void write(const Engine& engine, const void* data, vk::Extent3D extent, u32 pixel_alignment, vk::ImageLayout final_layout,
               const vk::ImageSubresourceRange& subresource = {vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1}) const;
    void generate_mipmaps(const Engine& engine, u32 levels, vk::Extent3D extent, vk::Format format, vk::ImageLayout final_layout) const;
};

inline u32 get_mip_count(const vk::Extent3D extent) {
    return static_cast<u32>(std::floor(std::log2(std::max(std::max(extent.width, extent.height), extent.depth)))) + 1;
}

void allocate_descriptor_sets(const Engine& engine, vk::DescriptorPool pool, std::span<const vk::DescriptorSetLayout> layouts, std::span<vk::DescriptorSet> sets);
inline vk::DescriptorSet allocate_descriptor_set(const Engine& engine, vk::DescriptorPool pool, vk::DescriptorSetLayout layout) {
    vk::DescriptorSet set = nullptr;
    allocate_descriptor_sets(engine, pool, {&layout, 1}, {&set, 1});
    return set;
}
void write_uniform_buffer_descriptor(const Engine& engine, vk::DescriptorSet set, u32 binding, vk::Buffer buffer, vk::DeviceSize size, vk::DeviceSize offset = 0);
void write_image_sampler_descriptor(const Engine& engine, vk::DescriptorSet set, u32 binding, vk::Sampler sampler, vk::ImageView view);

enum class SamplerType {
    Nearest,
    Linear,
};
struct SamplerConfig {
    SamplerType type = SamplerType::Nearest;
    vk::SamplerAddressMode edge_mode = vk::SamplerAddressMode::eRepeat;
    u32 mip_levels = 1;
};
[[nodiscard]] vk::Sampler create_sampler(const Engine& engine, const SamplerConfig& config);

inline vk::DescriptorSetLayout create_set_layout(const Engine& engine, const std::span<const vk::DescriptorSetLayoutBinding> bindings) {
    debug_assert(engine.device != nullptr);
    const auto layout = engine.device.createDescriptorSetLayout({.bindingCount = static_cast<u32>(bindings.size()), .pBindings = bindings.data()});
    critical_assert(layout.result == vk::Result::eSuccess);
    return layout.value;
}
struct ShaderConfig {
    std::filesystem::path path = {};
    vk::ShaderCodeTypeEXT code_type = vk::ShaderCodeTypeEXT::eSpirv;
    vk::ShaderStageFlagBits stage = {};
    vk::ShaderStageFlags next_stage = {};
    std::span<const vk::DescriptorSetLayout> set_layouts = {};
    std::span<const vk::PushConstantRange> push_ranges = {};
    vk::ShaderCreateFlagsEXT flags = {};
};
[[nodiscard]] vk::ShaderEXT create_shader(const Engine& engine, const ShaderConfig& config);

struct Pipeline {
    std::vector<vk::DescriptorSetLayout> descriptor_layouts = {};
    vk::PipelineLayout layout = {};
    vk::Pipeline pipeline = {};

    void destroy(const Engine& engine) const {
        debug_assert(engine.device != nullptr);
        for (const auto descriptor_layout : descriptor_layouts) {
            debug_assert(descriptor_layout != nullptr);
            engine.device.destroyDescriptorSetLayout(descriptor_layout);
        }
        debug_assert(layout != nullptr);
        engine.device.destroyPipelineLayout(layout);
        debug_assert(pipeline != nullptr);
        engine.device.destroyPipeline(pipeline);
    }
};

struct VertexAttribute {
    vk::Format format = vk::Format::eUndefined;
    u32 offset = 0;
};

struct VertexBinding {
    std::span<const VertexAttribute> attributes = {};
    u32 stride = 0;
    vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex;
};

class GraphicsPipelineBuilder {
public:
    explicit constexpr GraphicsPipelineBuilder(const vk::PipelineCache pipeline_cache = nullptr) : m_cache(pipeline_cache) {}

    constexpr GraphicsPipelineBuilder& set_shaders(const std::string_view vertex_shader, const std::string_view fragment_shader) {
        debug_assert(!vertex_shader.empty());
        debug_assert(!fragment_shader.empty());
        m_vertex_shader = vertex_shader;
        m_fragment_shader = fragment_shader;
        return *this;
    }
    constexpr GraphicsPipelineBuilder& add_descriptor_set_layout(std::span<const vk::DescriptorSetLayoutBinding> bindings) {
        debug_assert(!bindings.empty());
        m_descriptor_sets.emplace_back(bindings);
        return *this;
    }
    constexpr GraphicsPipelineBuilder& add_push_constant_range(const vk::ShaderStageFlags stage, const u32 size, const u32 offset = 0) {
        debug_assert(stage != vk::ShaderStageFlagBits{0});
        debug_assert(size > 0);
        m_push_constants.emplace_back(stage, offset, size);
        return *this;
    }
    constexpr GraphicsPipelineBuilder& add_push_constant_range(const vk::PushConstantRange range) {
        debug_assert(range.stageFlags != vk::ShaderStageFlagBits{0});
        debug_assert(range.size > 0);
        m_push_constants.push_back(range);
        return *this;
    }
    constexpr GraphicsPipelineBuilder& add_vertex_binding(const std::span<const VertexAttribute> attributes, const u32 stride,
                                                          const vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex) {
        debug_assert(!attributes.empty());
        debug_assert(stride > 0);
        m_vertex_bindings.emplace_back(attributes, stride, input_rate);
        return *this;
    }
    constexpr GraphicsPipelineBuilder& add_vertex_binding(const VertexBinding& binding) {
        debug_assert(!binding.attributes.empty());
        debug_assert(binding.stride > 0);
        m_vertex_bindings.push_back(binding);
        return *this;
    }
    constexpr GraphicsPipelineBuilder& set_render_target(const std::span<const vk::Format> color_attachments, const vk::Format depth_attachment = vk::Format::eUndefined,
                                                         const vk::Format stencil_attachment = vk::Format::eUndefined) {
        debug_assert(!color_attachments.empty() || depth_attachment != vk::Format::eUndefined || stencil_attachment != vk::Format::eUndefined);
        m_color_formats.clear();
        for (const auto format : color_attachments) {
            m_color_formats.emplace_back(format);
        }
        m_depth_format = depth_attachment;
        m_stencil_format = stencil_attachment;
        return *this;
    }
    constexpr GraphicsPipelineBuilder& enable_culling(const vk::CullModeFlagBits cull_mode = vk::CullModeFlagBits::eBack) {
        m_cull_mode = cull_mode;
        return *this;
    }
    constexpr GraphicsPipelineBuilder& set_MSAA(const vk::SampleCountFlagBits MSAA) {
        debug_assert(MSAA != vk::SampleCountFlagBits{0});
        m_MSAA = MSAA;
        return *this;
    }
    constexpr GraphicsPipelineBuilder& set_topology(const vk::PrimitiveTopology topology) {
        m_topology = topology;
        return *this;
    }
    constexpr GraphicsPipelineBuilder& enable_depth_buffer(const bool enable = true) {
        m_depth_buffer = enable;
        return *this;
    }
    constexpr GraphicsPipelineBuilder& enable_color_blend(const bool enable = true) {
        m_color_blend = enable;
        return *this;
    }

    [[nodiscard]] Pipeline build(const Engine& engine) const;

private:
    vk::PipelineCache m_cache = {};
    std::string_view m_vertex_shader = {};
    std::string_view m_fragment_shader = {};

    using DescriptorSetLayout = std::span<const vk::DescriptorSetLayoutBinding>;
    std::vector<DescriptorSetLayout> m_descriptor_sets = {};
    std::vector<vk::PushConstantRange> m_push_constants = {};
    std::vector<VertexBinding> m_vertex_bindings = {};

    std::vector<vk::Format> m_color_formats = {};
    vk::Format m_depth_format = vk::Format::eUndefined;
    vk::Format m_stencil_format = vk::Format::eUndefined;

    vk::CullModeFlagBits m_cull_mode = vk::CullModeFlagBits::eNone;
    vk::SampleCountFlagBits m_MSAA = vk::SampleCountFlagBits::e1;
    vk::PrimitiveTopology m_topology = vk::PrimitiveTopology::eTriangleList;
    bool m_depth_buffer = false;
    bool m_color_blend = false;
};

vk::CommandBuffer begin_single_time_commands(const Engine& engine);
void end_single_time_commands(const Engine& engine, vk::CommandBuffer cmd);
void submit_single_time_commands(const Engine& engine, const auto& commands) {
    const auto cmd = begin_single_time_commands(engine);
    commands(cmd);
    end_single_time_commands(engine, cmd);
}

class BarrierBuilder {
public:
    explicit constexpr BarrierBuilder(const vk::CommandBuffer cmd) : m_cmd(cmd) {}

    void build_and_run(const vk::DependencyFlagBits flags = {}) {
        m_cmd.pipelineBarrier2({
            .dependencyFlags = flags,
            .memoryBarrierCount = static_cast<u32>(m_memories.size()),
            .pMemoryBarriers = m_memories.data(),
            .bufferMemoryBarrierCount = static_cast<u32>(m_buffers.size()),
            .pBufferMemoryBarriers = m_buffers.data(),
            .imageMemoryBarrierCount = static_cast<u32>(m_images.size()),
            .pImageMemoryBarriers = m_images.data(),
        });
    }

    BarrierBuilder& add_memory_barrier(const vk::MemoryBarrier2& barrier) {
        m_memories.push_back(barrier);
        return *this;
    }
    BarrierBuilder& add_buffer_barrier(const vk::BufferMemoryBarrier2& barrier) {
        debug_assert(barrier.buffer != nullptr);
        m_buffers.push_back(barrier);
        return *this;
    }
    BarrierBuilder& add_image_barrier(const vk::ImageMemoryBarrier2& barrier) {
        debug_assert(barrier.image != nullptr);
        m_images.push_back(barrier);
        return *this;
    }
    BarrierBuilder& add_image_barrier(const vk::Image image, const vk::ImageSubresourceRange& subresource_range) {
        debug_assert(image != nullptr);
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
        debug_assert(new_layout != vk::ImageLayout::eUndefined);
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
