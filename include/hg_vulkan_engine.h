#pragma once

#include "hg_pch.h"
#include "hg_utils.h"

#include <array>
#include <filesystem>
#include <span>

namespace hg {

struct Engine {
    vk::Instance instance = {};
    vk::DebugUtilsMessengerEXT debug_messenger = {};
    vk::PhysicalDevice gpu = {};
    vk::Device device = {};
    VmaAllocator gpu_allocator = nullptr;
    u32 queue_family_index = UINT32_MAX;
    vk::Queue queue = {};
    vk::CommandPool command_pool = {};
    vk::CommandPool single_time_command_pool = {};

    [[nodiscard]] static Result<Engine> create();
    void destroy() const;
};

class GpuBuffer {
public:
    vk::Buffer get() const {
        ASSERT(m_buffer != nullptr);
        return m_buffer;
    }

    struct View {
        vk::Buffer buffer = nullptr;
        vk::DeviceSize range = 0;
        vk::DeviceSize offset = 0;
    };

    enum MemoryType {
        DeviceLocal,
        RandomAccess,
        LinearAccess,
    };

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
        ASSERT(m_allocation != nullptr);
        ASSERT(m_buffer != nullptr);
        ASSERT(engine.gpu_allocator != nullptr);
        vmaDestroyBuffer(engine.gpu_allocator, m_buffer, m_allocation);
    }

    [[nodiscard]] Result<void> write_result(const Engine& engine, const void* data, vk::DeviceSize size, vk::DeviceSize offset) const;

    void write_void(const Engine& engine, const void* data, vk::DeviceSize size, vk::DeviceSize offset) const {
        const auto result = write_result(engine, data, size, offset);
        if (result.has_err())
            ERROR("Could not write to gpu buffer");
    }

    template <typename T> void write_span(const Engine& engine, const std::span<T> data, vk::DeviceSize offset = 0) const {
        write_void(engine, data.data(), data.size() * sizeof(T), offset);
    }

    template <typename T> void write(const Engine& engine, const T& data, vk::DeviceSize offset = 0) const {
        write_void(engine, &data, sizeof(T), offset);
    }

private:
    VmaAllocation m_allocation = nullptr;
    vk::Buffer m_buffer = {};
    MemoryType m_type = DeviceLocal;
};

class GpuImage {
public:
    vk::Image get() const {
        ASSERT(m_image != nullptr);
        return m_image;
    }

    struct Config {
        vk::Extent3D extent = {};
        vk::ImageType dimensions = vk::ImageType::e1D;
        vk::Format format = vk::Format::eUndefined;
        vk::ImageUsageFlags usage = {};
        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;
        u32 mip_levels = 1;
    };
    [[nodiscard]] static Result<GpuImage> create(const Engine& engine, const Config& config);

    struct CubemapConfig {
        vk::Extent3D face_extent = {};
        vk::Format format = vk::Format::eUndefined;
        vk::ImageUsageFlags usage = {};
    };
    [[nodiscard]] static Result<GpuImage> create_cubemap(const Engine& engine, const CubemapConfig& config);

    void destroy(const Engine& engine) const {
        ASSERT(m_allocation != nullptr);
        ASSERT(m_image != nullptr);
        ASSERT(engine.device != nullptr);
        vmaDestroyImage(engine.gpu_allocator, m_image, m_allocation);
    }

    struct Data {
        const void* ptr = nullptr;
        u64 alignment = 0;
        vk::Extent3D extent = {};
    };

    struct WriteConfig {
        Data data = {};
        vk::ImageLayout final_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        vk::ImageSubresourceRange subresource = {vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1};
    };

    [[nodiscard]] Result<void> write(const Engine& engine, const WriteConfig& config) const;

private:
    VmaAllocation m_allocation = nullptr;
    vk::Image m_image = {};
};

class GpuImageView {
public:
    vk::ImageView get() const {
        ASSERT(m_view != nullptr);
        return m_view;
    }

    struct Config {
        vk::Image image = {};
        vk::ImageViewType dimensions = vk::ImageViewType::e1D;
        vk::Format format = vk::Format::eUndefined;
        vk::ImageSubresourceRange subresource = {vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1};
    };
    [[nodiscard]] static Result<GpuImageView> create(const Engine& engine, const Config& config);

    void destroy(const Engine& engine) const {
        ASSERT(m_view != nullptr);
        engine.device.destroyImageView(m_view);
    }

private:
    vk::ImageView m_view = {};
};

class GpuImageAndView {
public:
    vk::Image get_image() const { return m_image.get(); }
    vk::ImageView get_view() const { return m_view.get(); }

    [[nodiscard]] static GpuImageAndView from_parts(GpuImage&& image, GpuImageView&& view) {
        GpuImageAndView image_and_view = {};
        image_and_view.m_image = image;
        image_and_view.m_view = view;
        return image_and_view;
    }

    struct Config {
        vk::Extent3D extent = {};
        vk::Format format = vk::Format::eUndefined;
        vk::ImageUsageFlags usage = {};
        vk::ImageAspectFlagBits aspect_flags = vk::ImageAspectFlagBits::eColor;
        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
        u32 mip_levels = 1;
    };
    [[nodiscard]] static Result<GpuImageAndView> create_result(const Engine& engine, const Config& config);
    [[nodiscard]] static GpuImageAndView create(const Engine& engine, const Config& config) {
        const auto image = create_result(engine, config);
        if (image.has_err())
            ERROR("Could not create gpu image");
        return *image;
    }

    struct CubemapConfig {
        std::filesystem::path path = {};
        vk::Format format = vk::Format::eR8G8B8A8Srgb;
        vk::ImageAspectFlagBits aspect_flags = vk::ImageAspectFlagBits::eColor;
    };
    [[nodiscard]] static Result<GpuImageAndView> create_cubemap(const Engine& engine, const CubemapConfig& config);

    void destroy(const Engine& engine) const {
        m_image.destroy(engine);
        m_view.destroy(engine);
    }

    [[nodiscard]] Result<void> write_result(const Engine& engine, const GpuImage::WriteConfig& config) const {
        return m_image.write(engine, config);
    }
    void write(const Engine& engine, const GpuImage::WriteConfig& config) const {
        const auto write = write_result(engine, config);
        if (write.has_err())
            ERROR("Could not write to gpu image");
    }

    [[nodiscard]] Result<void> generate_mipmaps_result(
        const Engine& engine, u32 levels, vk::Extent3D extent, vk::Format format, vk::ImageLayout final_layout
    ) const;

    void generate_mipmaps(
        const Engine& engine, u32 levels, vk::Extent3D extent, vk::Format format, vk::ImageLayout final_layout
    ) const {
        const auto mipmap = generate_mipmaps_result(engine, levels, extent, format, final_layout);
        if (mipmap.has_err())
            ERROR("Could not generate mipmaps");
    }

private:
    GpuImage m_image = {};
    GpuImageView m_view = {};
};

inline u32 get_mip_count(const vk::Extent3D extent) {
    return static_cast<u32>(std::floor(std::log2(std::max(std::max(extent.width, extent.height), extent.depth)))) + 1;
}

class Sampler {
public:
    vk::Sampler get() const {
        ASSERT(m_sampler != nullptr);
        return m_sampler;
    }

    enum Type {
        Nearest = VK_FILTER_NEAREST,
        Linear = VK_FILTER_LINEAR,
    };

    struct Config {
        Type type = Nearest;
        vk::SamplerAddressMode edge_mode = vk::SamplerAddressMode::eRepeat;
        u32 mip_levels = 1;
    };
    [[nodiscard]] static Result<Sampler> create_result(const Engine& engine, const Config& config);
    [[nodiscard]] static Sampler create(const Engine& engine, const Config& config) {
        const auto sampler = create_result(engine, config);
        if (sampler.has_err())
            ERROR("Could not create Vulkan sampler");
        return *sampler;
    }

    void destroy(const Engine& engine) const {
        ASSERT(m_sampler != nullptr);
        engine.device.destroySampler(m_sampler);
    }

private:
    vk::Sampler m_sampler = {};
};

class Texture {
public:
    vk::Image get_image() const { return m_image.get_image(); }
    vk::ImageView get_view() const { return m_image.get_view(); }
    vk::Sampler get_sampler() const { return m_sampler.get(); }

    [[nodiscard]] static Texture from_parts(GpuImageAndView&& image, Sampler&& sampler) {
        Texture texture = {};
        texture.m_image = image;
        texture.m_sampler = sampler;
        return texture;
    }
    [[nodiscard]] static Texture from_parts(GpuImage&& image, GpuImageView&& view, Sampler&& sampler) {
        return from_parts(GpuImageAndView::from_parts(std::move(image), std::move(view)), std::move(sampler));
    }

    struct Config {
        vk::Format format = vk::Format::eUndefined;
        vk::ImageAspectFlagBits aspect_flags = vk::ImageAspectFlagBits::eColor;
        bool create_mips = false;
        Sampler::Type sampler_type = Sampler::Nearest;
        vk::SamplerAddressMode edge_mode = vk::SamplerAddressMode::eRepeat;
    };
    [[nodiscard]] static Result<Texture> from_data_result(
        const Engine& engine, const GpuImage::Data& data, const Config& config
    );
    [[nodiscard]] static Texture from_data(
        const Engine& engine, const GpuImage::Data& data, const Config& config
    ) {
        const auto texture = from_data_result(engine, data, config);
        if (texture.has_err())
            ERROR("Could not create texture");
        return *texture;
    }
    [[nodiscard]] static Result<Texture> from_file(
        const Engine& engine, std::filesystem::path path, const Config& config
    );
    [[nodiscard]] static Result<Texture> create_cubemap(
        const Engine& engine, std::filesystem::path path, const Config& config
    );

    void destroy(const Engine& engine) const {
        m_image.destroy(engine);
        m_sampler.destroy(engine);
    }

private:
    GpuImageAndView m_image = {};
    Sampler m_sampler = {};
};

[[nodiscard]] Result<vk::DescriptorSetLayout> create_descriptor_set_layout(
    const Engine& engine,
    std::span<const vk::DescriptorSetLayoutBinding> bindings,
    std::span<const vk::DescriptorBindingFlags> flags = {}
);

[[nodiscard]] Result<vk::DescriptorPool> create_descriptor_pool(
    const Engine& engine, u32 max_sets, std::span<const vk::DescriptorPoolSize> descriptors
);

[[nodiscard]] Result<void> allocate_descriptor_sets(
    const Engine& engine, vk::DescriptorPool pool,
    std::span<const vk::DescriptorSetLayout> layouts,
    std::span<vk::DescriptorSet> out_sets
);
[[nodiscard]] inline Result<vk::DescriptorSet> allocate_descriptor_set(
    const Engine& engine, const vk::DescriptorPool pool, const vk::DescriptorSetLayout layout
) {
    auto set = ok<vk::DescriptorSet>();
    const auto alloc_result = allocate_descriptor_sets(engine, pool, {&layout, 1}, {&*set, 1});
    if (alloc_result.has_err())
        return alloc_result.err();
    return set;
}

void write_uniform_buffer_descriptor(
    const Engine& engine, const GpuBuffer::View& buffer,
    vk::DescriptorSet set, u32 binding, u32 binding_array_index = 0
);

void write_image_sampler_descriptor(
    const Engine& engine, const Texture& texture,
    vk::DescriptorSet set, u32 binding, u32 binding_array_index = 0
);

class PipelineLayout {
public:
    vk::PipelineLayout get() const {
        ASSERT(m_pipeline_layout != nullptr);
        return m_pipeline_layout;
    }

    struct Config {
        std::span<const vk::DescriptorSetLayout> set_layouts = {};
        std::span<const vk::PushConstantRange> push_ranges = {};
    };
    [[nodiscard]] static Result<PipelineLayout> create(const Engine& engine, const Config& config);

    void destroy(const Engine& engine) const {
        ASSERT(engine.device != nullptr);
        engine.device.destroyPipelineLayout(m_pipeline_layout);
    }

private:
    vk::PipelineLayout m_pipeline_layout = {};
};

class UnlinkedShader {
public:
    vk::ShaderEXT get() const {
        ASSERT(m_shader != nullptr);
        return m_shader;
    }

    struct Config {
        std::filesystem::path path = {};
        vk::ShaderCodeTypeEXT code_type = vk::ShaderCodeTypeEXT::eSpirv;
        vk::ShaderStageFlagBits stage = {};
        vk::ShaderStageFlagBits next_stage = {};
        std::span<const vk::DescriptorSetLayout> set_layouts = {};
        std::span<const vk::PushConstantRange> push_ranges = {};
    };
    [[nodiscard]] static Result<UnlinkedShader> create(const Engine& engine, const Config& config);

    void destroy(const Engine& engine) const {
        ASSERT(engine.device != nullptr);
        engine.device.destroyShaderEXT(m_shader);
    }

private:
    vk::ShaderEXT m_shader = {};
};

class GraphicsPipeline {
public:
    vk::PipelineLayout get_layout() const { return m_layout.get(); }
    std::span<const vk::ShaderEXT> get_shaders() const {
        for (const auto shader : m_shaders) {
            ASSERT(shader != nullptr);
        }
        return m_shaders;
    }

    struct Config {
        std::span<const vk::DescriptorSetLayout> set_layouts = {};
        std::span<const vk::PushConstantRange> push_ranges = {};
        std::filesystem::path vertex_shader_path = {};
        std::filesystem::path fragment_shader_path = {};
        vk::ShaderCodeTypeEXT code_type = vk::ShaderCodeTypeEXT::eSpirv;
    };
    [[nodiscard]] static Result<GraphicsPipeline> create(const Engine& engine, const Config& config);

    void destroy(const Engine& engine) const {
        ASSERT(engine.device != nullptr);
        m_layout.destroy(engine);

        for (const auto shader : m_shaders) {
            ASSERT(shader != nullptr);
            engine.device.destroyShaderEXT(shader);
        }
    }

private:
    PipelineLayout m_layout = {};
    std::array<vk::ShaderEXT, 2> m_shaders = {};
};

[[nodiscard]] vk::CommandPool create_command_pool(const Engine& engine, vk::CommandPoolCreateFlags flags);

[[nodiscard]] Result<vk::CommandBuffer> begin_single_time_commands(const Engine& engine);
[[nodiscard]] Result<void> end_single_time_commands(const Engine& engine, vk::CommandBuffer cmd);
[[nodiscard]] Result<void> submit_single_time_commands(const Engine& engine, const auto& commands) {
    const Result<vk::CommandBuffer> cmd = begin_single_time_commands(engine);
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
    BarrierBuilder& set_image_src(
        const vk::PipelineStageFlags2 src_stage_mask,
        const vk::AccessFlags2 src_access_mask,
        const vk::ImageLayout old_layout
    ) {
        m_images.back().srcStageMask = src_stage_mask;
        m_images.back().srcAccessMask = src_access_mask;
        m_images.back().oldLayout = old_layout;
        return *this;
    }
    BarrierBuilder& set_image_dst(
        const vk::PipelineStageFlags2 dst_stage_mask,
        const vk::AccessFlags2 dst_access_mask,
        const vk::ImageLayout new_layout
    ) {
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

constexpr u32 MaxFramesInFlight = 2;
constexpr u32 MaxSwapchainImages = 3;

class Renderer {
public:
    virtual ~Renderer() = default;
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

    [[nodiscard]] Result<void> draw_frame(const Engine& engine, const Renderer& renderer) {
        const auto cmd = begin_frame(engine);
        if (cmd.has_err())
            return cmd.err();
        renderer.cmd_draw(*cmd, current_image(), m_extent);
        return end_frame(engine);
    }

private:
    [[nodiscard]] Result<vk::CommandBuffer> begin_frame(const Engine& engine);
    [[nodiscard]] Result<void> end_frame(const Engine& engine);

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

} // namespace hg
