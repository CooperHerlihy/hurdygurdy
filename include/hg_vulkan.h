#pragma once

#include "hg_pch.h"
#include "hg_utils.h"
#include "hg_load.h"

#include <array>
#include <filesystem>
#include <span>

namespace hg {

class VulkanAllocator {
public:
    VulkanAllocator() = default;
    VulkanAllocator(Allocator& parent) : m_parent{&parent} {}

    struct Metadata {
        std::byte* ptr = nullptr;
        usize size = 0;
    };

    void* alloc(const usize size, const usize alignment) {
        static_assert(sizeof(Metadata) == 16);
        usize total_size = sizeof(Metadata) + align_size(size, alignment);
        Slice<std::byte> allocation = m_parent->alloc<std::byte>(total_size);

        Metadata* metadata = reinterpret_cast<Metadata*>(allocation.data);
        *metadata = {
            .ptr = allocation.data,
            .size = total_size,
        };
        return metadata->ptr + sizeof(Metadata);
    }

    void* realloc(void* original, const usize size, const usize alignment) {
        static_assert(sizeof(Metadata) == 16);
        std::byte* original_alloc = reinterpret_cast<std::byte*>(original) - sizeof(Metadata);
        Metadata* original_metadata = reinterpret_cast<Metadata*>(original_alloc);
        if (original_alloc != original_metadata->ptr)
            ERROR("Cannot realloc from invalid pointer");

        usize new_size = sizeof(Metadata) + align_size(size, alignment);
        Slice<std::byte> reallocation = m_parent->realloc(Slice<std::byte>{original_metadata->ptr, original_metadata->size}, new_size);
        Metadata* new_metadata = reinterpret_cast<Metadata*>(reallocation.data);
        *new_metadata = {
            .ptr = reallocation.data,
            .size = new_size,
        }; 
        return new_metadata->ptr + sizeof(Metadata);
    }

    void free(void* memory) {
        static_assert(sizeof(Metadata) == 16);
        std::byte* ptr = reinterpret_cast<std::byte*>(memory) - sizeof(Metadata);
        Metadata* metadata = reinterpret_cast<Metadata*>(ptr);
        if (ptr != metadata->ptr)
            ERROR("Cannot dealloc from invalid pointer");
        m_parent->dealloc(Slice<std::byte>(metadata->ptr, metadata->size));
    }

    vk::AllocationCallbacks callbacks() {
        return {
            .pUserData = this,
            .pfnAllocation = [](void* data, usize size, usize alignment, vk::SystemAllocationScope) -> void* {
                auto& allocator = *static_cast<VulkanAllocator*>(data);
                return allocator.alloc(size, alignment);
            },
            .pfnReallocation = [](void* data, void* original, usize size, usize alignment, vk::SystemAllocationScope) -> void* {
                auto& allocator = *static_cast<VulkanAllocator*>(data);
                return allocator.realloc(original, size, alignment);
            },
            .pfnFree = [](void* data, void* memory) {
                auto& allocator = *static_cast<VulkanAllocator*>(data);
                allocator.free(memory);
            },
            .pfnInternalAllocation = nullptr,
            .pfnInternalFree = nullptr,
        };
    }

private:
    Allocator* m_parent = nullptr;
};

struct Vk {
    vk::Instance instance{};
    vk::DebugUtilsMessengerEXT debug_messenger{};

    vk::PhysicalDevice gpu{};
    vk::Device device{};
    VmaAllocator gpu_allocator = nullptr;

    u32 queue_family_index = UINT32_MAX;
    vk::Queue queue{};

    vk::CommandPool command_pool{};
    vk::CommandPool single_time_command_pool{};

    [[nodiscard]] static Result<Vk> create();
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
        vk::DeviceSize size = 0;
        vk::BufferUsageFlags usage{};
        MemoryType memory_type = DeviceLocal;
    };
    [[nodiscard]] static GpuBuffer create(const Vk& vk, const Config& config);

    void destroy(const Vk& vk) const {
        ASSERT(m_allocation != nullptr);
        ASSERT(m_buffer != nullptr);

        vmaDestroyBuffer(vk.gpu_allocator, m_buffer, m_allocation);
    }

    void write_void(const Vk& vk, const void* data, vk::DeviceSize size, vk::DeviceSize offset) const;

    template <typename T> void write_span(const Vk& vk, const std::span<T> data, vk::DeviceSize offset = 0) const {
        write_void(vk, data.data(), data.size() * sizeof(T), offset);
    }
    template <typename T> void write(const Vk& vk, const T& data, vk::DeviceSize offset = 0) const {
        write_void(vk, &data, sizeof(T), offset);
    }

private:
    VmaAllocation m_allocation = nullptr;
    vk::Buffer m_buffer{};
    MemoryType m_type = DeviceLocal;
};

class GpuImage {
public:
    vk::Image get() const {
        ASSERT(m_image != nullptr);
        return m_image;
    }

    struct Config {
        vk::Extent3D extent{};
        vk::ImageType dimensions{vk::ImageType::e1D};
        vk::Format format{vk::Format::eUndefined};
        vk::ImageUsageFlags usage{};
        vk::SampleCountFlagBits sample_count{vk::SampleCountFlagBits::e1};
        u32 mip_levels = 1;
    };
    [[nodiscard]] static GpuImage create(const Vk& vk, const Config& config);

    struct CubemapConfig {
        vk::Extent3D face_extent{};
        vk::Format format{vk::Format::eUndefined};
        vk::ImageUsageFlags usage{};
    };
    [[nodiscard]] static GpuImage create_cubemap(const Vk& vk, const CubemapConfig& config);

    void destroy(const Vk& vk) const {
        ASSERT(m_allocation != nullptr);
        ASSERT(m_image != nullptr);

        vmaDestroyImage(vk.gpu_allocator, m_image, m_allocation);
    }

    struct Data {
        const void* ptr = nullptr;
        u64 alignment = 0;
        vk::Extent3D extent{};
    };

    struct WriteConfig {
        Data data{};
        vk::ImageLayout final_layout{vk::ImageLayout::eShaderReadOnlyOptimal};
        vk::ImageSubresourceRange subresource{vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1};
    };
    void write(const Vk& vk, const WriteConfig& config) const;

private:
    VmaAllocation m_allocation = nullptr;
    vk::Image m_image{};
};

class GpuImageView {
public:
    vk::ImageView get() const {
        ASSERT(m_view != nullptr);
        return m_view;
    }

    struct Config {
        vk::Image image{};
        vk::ImageViewType dimensions{vk::ImageViewType::e1D};
        vk::Format format{vk::Format::eUndefined};
        vk::ImageSubresourceRange subresource{vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1};
    };
    [[nodiscard]] static GpuImageView create(const Vk& vk, const Config& config);

    void destroy(const Vk& vk) const {
        ASSERT(m_view != nullptr);

        vk.device.destroyImageView(m_view);
    }

private:
    vk::ImageView m_view{};
};

class GpuImageAndView {
public:
    vk::Image get_image() const { return m_image.get(); }
    vk::ImageView get_view() const { return m_view.get(); }

    [[nodiscard]] static GpuImageAndView from_parts(GpuImage&& image, GpuImageView&& view) {
        GpuImageAndView image_and_view{};
        image_and_view.m_image = image;
        image_and_view.m_view = view;
        return image_and_view;
    }

    struct Config {
        vk::Extent3D extent{};
        vk::Format format{vk::Format::eUndefined};
        vk::ImageUsageFlags usage{};
        vk::ImageAspectFlagBits aspect_flags{vk::ImageAspectFlagBits::eColor};
        vk::SampleCountFlagBits sample_count{vk::SampleCountFlagBits::e1};
        vk::ImageLayout layout{vk::ImageLayout::eUndefined};
        u32 mip_levels = 1;
    };
    [[nodiscard]] static GpuImageAndView create(const Vk& vk, const Config& config);

    struct CubemapConfig {
        ImageData data{};
        vk::Format format{vk::Format::eR8G8B8A8Srgb};
        vk::ImageAspectFlagBits aspect_flags{vk::ImageAspectFlagBits::eColor};
    };
    [[nodiscard]] static GpuImageAndView create_cubemap(const Vk& vk, const CubemapConfig& config);

    void destroy(const Vk& vk) const {
        m_image.destroy(vk);
        m_view.destroy(vk);
    }

    void write(const Vk& vk, const GpuImage::WriteConfig& config) const {
        m_image.write(vk, config);
    }

    void generate_mipmaps(const Vk& vk, u32 levels, vk::Extent3D extent, vk::Format format, vk::ImageLayout final_layout) const;

private:
    GpuImage m_image{};
    GpuImageView m_view{};
};

inline u32 get_mip_count(const vk::Extent3D extent) {
    ASSERT(extent.width > 0 || extent.height > 0 || extent.depth > 0);

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
        vk::SamplerAddressMode edge_mode{vk::SamplerAddressMode::eRepeat};
        u32 mip_levels = 1;
    };
    [[nodiscard]] static Sampler create(const Vk& vk, const Config& config);

    void destroy(const Vk& vk) const {
        ASSERT(m_sampler != nullptr);

        vk.device.destroySampler(m_sampler);
    }

private:
    vk::Sampler m_sampler{};
};

class Texture {
public:
    vk::Image get_image() const { return m_image.get_image(); }
    vk::ImageView get_view() const { return m_image.get_view(); }
    vk::Sampler get_sampler() const { return m_sampler.get(); }

    [[nodiscard]] static Texture from_parts(GpuImageAndView&& image, Sampler&& sampler) {
        Texture texture{};
        texture.m_image = image;
        texture.m_sampler = sampler;
        return texture;
    }
    [[nodiscard]] static Texture from_parts(GpuImage&& image, GpuImageView&& view, Sampler&& sampler) {
        return from_parts(GpuImageAndView::from_parts(std::move(image), std::move(view)), std::move(sampler));
    }

    struct Config {
        vk::Format format{vk::Format::eUndefined};
        vk::ImageAspectFlagBits aspect_flags{vk::ImageAspectFlagBits::eColor};
        bool create_mips = false;
        Sampler::Type sampler_type = Sampler::Nearest;
        vk::SamplerAddressMode edge_mode{vk::SamplerAddressMode::eRepeat};
    };
    [[nodiscard]] static Texture from_data(const Vk& vk, const GpuImage::Data& data, const Config& config);
    [[nodiscard]] static Result<Texture> from_file(const Vk& vk, std::filesystem::path path, const Config& config);
    [[nodiscard]] static Result<Texture> from_cubemap_file(const Vk& vk, std::filesystem::path path, const Config& config);

    void destroy(const Vk& vk) const {
        m_image.destroy(vk);
        m_sampler.destroy(vk);
    }

private:
    GpuImageAndView m_image{};
    Sampler m_sampler{};
};

class DescriptorSetLayout {
public:
    vk::DescriptorSetLayout get() const {
        ASSERT(m_descriptor_set_layout != nullptr);
        return m_descriptor_set_layout;
    }

    struct Config {
        std::span<const vk::DescriptorSetLayoutBinding> bindings;
        std::span<const vk::DescriptorBindingFlags> flags = {};
    };
    [[nodiscard]] static DescriptorSetLayout create(const Vk& vk, const Config& config);

    void destroy(const Vk& vk) const {
        ASSERT(m_descriptor_set_layout != nullptr);

        vk.device.destroyDescriptorSetLayout(m_descriptor_set_layout);
    }

private:
    vk::DescriptorSetLayout m_descriptor_set_layout{};
};

class DescriptorPool {
public:
    vk::DescriptorPool get() const {
        ASSERT(m_pool != nullptr);
        return m_pool;
    }

    struct Config {
        u32 max_sets = 0;
        std::span<const vk::DescriptorPoolSize> descriptors{};
    };
    [[nodiscard]] static DescriptorPool create(const Vk& vk, const Config& config);

    void destroy(const Vk& vk) const {
        ASSERT(m_pool != nullptr);

        vk.device.destroyDescriptorPool(m_pool);
    }

    [[nodiscard]] Result<void> allocate_sets(
        const Vk& vk,
        std::span<const vk::DescriptorSetLayout> layouts,
        std::span<vk::DescriptorSet> out_sets
    );
    [[nodiscard]] inline Result<vk::DescriptorSet> allocate_set(const Vk& vk, const vk::DescriptorSetLayout layout) {
        auto set = ok<vk::DescriptorSet>();
        const auto alloc_result = allocate_sets(vk, {&layout, 1}, {&*set, 1});
        if (alloc_result.has_err())
            return alloc_result.err();
        return set;
    }

private:
    vk::DescriptorPool m_pool{};
};

void write_uniform_buffer_descriptor(
    const Vk& vk, const GpuBuffer::View& buffer,
    vk::DescriptorSet set, u32 binding, u32 binding_array_index = 0
);

void write_image_sampler_descriptor(
    const Vk& vk, const Texture& texture,
    vk::DescriptorSet set, u32 binding, u32 binding_array_index = 0
);

class PipelineLayout {
public:
    vk::PipelineLayout get() const {
        ASSERT(m_pipeline_layout != nullptr);
        return m_pipeline_layout;
    }

    struct Config {
        std::span<const vk::DescriptorSetLayout> set_layouts{};
        std::span<const vk::PushConstantRange> push_ranges{};
    };
    [[nodiscard]] static PipelineLayout create(const Vk& vk, const Config& config);

    void destroy(const Vk& vk) const {
        ASSERT(m_pipeline_layout != nullptr);

        vk.device.destroyPipelineLayout(m_pipeline_layout);
    }

private:
    vk::PipelineLayout m_pipeline_layout{};
};

class UnlinkedShader {
public:
    vk::ShaderEXT get() const {
        ASSERT(m_shader != nullptr);
        return m_shader;
    }

    struct Config {
        std::filesystem::path path{};
        vk::ShaderCodeTypeEXT code_type{vk::ShaderCodeTypeEXT::eSpirv};
        vk::ShaderStageFlagBits stage{};
        vk::ShaderStageFlagBits next_stage{};
        std::span<const vk::DescriptorSetLayout> set_layouts{};
        std::span<const vk::PushConstantRange> push_ranges{};
    };
    [[nodiscard]] static Result<UnlinkedShader> create(const Vk& vk, const Config& config);

    void destroy(const Vk& vk) const {
        ASSERT(m_shader != nullptr);

        vk.device.destroyShaderEXT(m_shader);
    }

private:
    vk::ShaderEXT m_shader{};
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
        std::span<const vk::DescriptorSetLayout> set_layouts{};
        std::span<const vk::PushConstantRange> push_ranges{};
        std::filesystem::path vertex_shader_path{};
        std::filesystem::path fragment_shader_path{};
        vk::ShaderCodeTypeEXT code_type{vk::ShaderCodeTypeEXT::eSpirv};
    };
    [[nodiscard]] static Result<GraphicsPipeline> create(const Vk& vk, const Config& config);

    void destroy(const Vk& vk) const {
        for (const auto shader : m_shaders) {
            ASSERT(shader != nullptr);
            vk.device.destroyShaderEXT(shader);
        }

        m_layout.destroy(vk);
    }

private:
    PipelineLayout m_layout{};
    std::array<vk::ShaderEXT, 2> m_shaders{};
};

class Fence {
public:
    vk::Fence get() const {
        ASSERT(m_fence != nullptr);
        return m_fence;
    }

    struct Config {
        vk::FenceCreateFlags flags{};
    };
    [[nodiscard]] static Fence create(const Vk& vk, const Config& config);

    void destroy(const Vk& vk) const {
        ASSERT(m_fence != nullptr);

        vk.device.destroyFence(m_fence);
    }

    void wait(const Vk& vk) const;
    void reset(const Vk& vk) const;

private:
    vk::Fence m_fence = nullptr;
};

class Semaphore {
public:
    vk::Semaphore get() const {
        ASSERT(m_semaphore != nullptr);
        return m_semaphore;
    }
    vk::Semaphore* ptr() {
        ASSERT(m_semaphore != nullptr);
        return &m_semaphore;
    }

    [[nodiscard]] static Semaphore create(const Vk& vk);

    void destroy(const Vk& vk) const {
        ASSERT(m_semaphore != nullptr);

        vk.device.destroySemaphore(m_semaphore);
    }

private:
    vk::Semaphore m_semaphore = nullptr;
};

void allocate_command_buffers(const Vk& vk, const std::span<vk::CommandBuffer> out_cmds);

[[nodiscard]] vk::CommandBuffer begin_single_time_commands(const Vk& vk);
void end_single_time_commands(const Vk& vk, vk::CommandBuffer cmd);
void submit_single_time_commands(const Vk& vk, auto commands) {
    const vk::CommandBuffer cmd = begin_single_time_commands(vk);
    commands(cmd);
    end_single_time_commands(vk, cmd);
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
    vk::CommandBuffer m_cmd{};
    std::vector<vk::MemoryBarrier2> m_memories{};
    std::vector<vk::BufferMemoryBarrier2> m_buffers{};
    std::vector<vk::ImageMemoryBarrier2> m_images{};
};

class Surface {
public:
    [[nodiscard]] vk::SurfaceKHR get() const {
        ASSERT(m_surface != nullptr);
        return m_surface;
    }

    [[nodiscard]] static Result<Surface> create(const Vk& vk, GLFWwindow* window);
    void destroy(const Vk& vk) const {
        ASSERT(m_surface != nullptr);
        vk.instance.destroySurfaceKHR(m_surface);
    }

private:
    vk::SurfaceKHR m_surface{};
};

class Swapchain {
public:
    static constexpr u32 MaxFramesInFlight = 2;
    static constexpr u32 MaxImages = 3;

    static constexpr vk::Format SwapchainImageFormat{vk::Format::eR8G8B8A8Srgb};
    static constexpr vk::ColorSpaceKHR SwapchainColorSpace{vk::ColorSpaceKHR::eSrgbNonlinear};

    [[nodiscard]] static Result<Swapchain> create(const Vk& vk, vk::SurfaceKHR surface);
    void destroy(const Vk& vk) const;
    [[nodiscard]] Result<void> resize(const Vk& vk, vk::SurfaceKHR surface);

    struct DrawInfo {
        vk::CommandBuffer cmd{};
        vk::Image render_target{};
        vk::Extent2D extent{};
    };
    [[nodiscard]] Result<DrawInfo> begin_frame(const Vk& vk);
    [[nodiscard]] Result<void> end_frame(const Vk& vk);

private:
    [[nodiscard]] vk::CommandBuffer& current_cmd() { return m_command_buffers[m_current_frame_index]; }
    [[nodiscard]] vk::Image& current_image() { return m_swapchain_images[m_current_image_index]; }
    [[nodiscard]] Fence& is_frame_finished() { return m_frame_finished_fences[m_current_frame_index]; }
    [[nodiscard]] Semaphore& is_image_available() { return m_image_available_semaphores[m_current_frame_index]; }
    [[nodiscard]] Semaphore& is_ready_to_present() { return m_ready_to_present_semaphores[m_current_image_index]; }
    [[nodiscard]] const vk::CommandBuffer& current_cmd() const { return m_command_buffers[m_current_frame_index]; }
    [[nodiscard]] const vk::Image& current_image() const { return m_swapchain_images[m_current_image_index]; }
    [[nodiscard]] const Fence& is_frame_finished() const { return m_frame_finished_fences[m_current_frame_index]; }
    [[nodiscard]] const Semaphore& is_image_available() const { return m_image_available_semaphores[m_current_frame_index]; }
    [[nodiscard]] const Semaphore& is_ready_to_present() const { return m_ready_to_present_semaphores[m_current_image_index]; }

    vk::Extent2D m_extent{};
    vk::SwapchainKHR m_swapchain{};
    std::array<vk::Image, MaxImages> m_swapchain_images{};
    u32 m_image_count = 0;
    u32 m_current_image_index = 0;
    u32 m_current_frame_index = 0;
    bool m_recording = false;
    std::array<vk::CommandBuffer, MaxFramesInFlight> m_command_buffers{};
    std::array<Fence, MaxFramesInFlight> m_frame_finished_fences{};
    std::array<Semaphore, MaxFramesInFlight> m_image_available_semaphores{};
    std::array<Semaphore, MaxImages> m_ready_to_present_semaphores{};
};

} // namespace hg
