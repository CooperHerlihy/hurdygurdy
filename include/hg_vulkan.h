#pragma once

#include "hg_utils.h"
#include "hg_load.h"

namespace hg {

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

    [[nodiscard]] static Result<Vk> create();
    void destroy();
};

class GpuBuffer {
public:
    VkBuffer get() const {
        ASSERT(m_buffer != nullptr);
        return m_buffer;
    }

    struct View {
        VkBuffer buffer = nullptr;
        VkDeviceSize range = 0;
        VkDeviceSize offset = 0;
    };

    enum MemoryType {
        DeviceLocal,
        RandomAccess,
        LinearAccess,
    };

    struct Config {
        VkDeviceSize size = 0;
        VkBufferUsageFlags usage{};
        MemoryType memory_type = DeviceLocal;
    };
    [[nodiscard]] static GpuBuffer create(Vk& vk, const Config& config);
    void destroy(Vk& vk) const {
        ASSERT(m_allocation != nullptr);
        ASSERT(m_buffer != nullptr);
        vmaDestroyBuffer(vk.gpu_allocator, m_buffer, m_allocation);
    }

    void write_void(Vk& vk, const void* data, usize size, usize offset) const;

    template <typename T> void write_slice(Vk& vk, const Slice<const T> data, VkDeviceSize offset = 0) const {
        write_void(vk, data.data, data.count * sizeof(T), offset);
    }
    template <typename T> void write(Vk& vk, const T& data, usize offset = 0) const {
        write_void(vk, &data, sizeof(T), offset);
    }

private:
    VmaAllocation m_allocation = nullptr;
    VkBuffer m_buffer{};
    MemoryType m_type = DeviceLocal;
};

class GpuImage {
public:
    VkImage get() const {
        ASSERT(m_image != nullptr);
        return m_image;
    }

    struct Config {
        VkExtent3D extent{};
        VkFormat format{VK_FORMAT_UNDEFINED};
        VkImageUsageFlags usage{};
        u32 mip_levels = 1;
        VkSampleCountFlagBits sample_count{VK_SAMPLE_COUNT_1_BIT};
    };
    [[nodiscard]] static GpuImage create(Vk& vk, const Config& config);

    struct CubemapConfig {
        VkExtent3D face_extent{};
        VkFormat format{VK_FORMAT_UNDEFINED};
        VkImageUsageFlags usage{};
    };
    [[nodiscard]] static GpuImage create_cubemap(Vk& vk, const CubemapConfig& config);

    void destroy(Vk& vk) const {
        ASSERT(m_allocation != nullptr);
        ASSERT(m_image != nullptr);
        vmaDestroyImage(vk.gpu_allocator, m_image, m_allocation);
    }

    struct WriteConfig {
        ImageData data{};
        VkFormat format{VK_FORMAT_R8G8B8A8_SRGB};
        VkImageLayout final_layout{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkImageAspectFlags aspect_flags{VK_IMAGE_ASPECT_COLOR_BIT};
    };
    void write(Vk& vk, const WriteConfig& config) const;
    void write_cubemap(Vk& vk, const WriteConfig& config) const;

private:
    VmaAllocation m_allocation = nullptr;
    VkImage m_image{};
};

class GpuImageView {
public:
    VkImageView get() const {
        ASSERT(m_view != nullptr);
        return m_view;
    }

    GpuImageView() = default;
    GpuImageView(VkImageView view) : m_view{view} {}

    struct Config {
        VkImage image{};
        VkFormat format{};
        VkImageAspectFlags aspect_flags{VK_IMAGE_ASPECT_COLOR_BIT};
    };
    [[nodiscard]] static GpuImageView create(Vk& vk, const Config& config);
    [[nodiscard]] static GpuImageView create_cubemap(Vk& vk, const Config& config);

    void destroy(Vk& vk) const {
        ASSERT(m_view != nullptr);
        vkDestroyImageView(vk.device, m_view, nullptr);
    }

private:
    VkImageView m_view{};
};

class GpuImageAndView {
public:
    VkImage get_image() const { return m_image.get(); }
    VkImageView get_view() const { return m_view.get(); }

    [[nodiscard]] static GpuImageAndView from_parts(GpuImage&& image, GpuImageView&& view) {
        GpuImageAndView image_and_view{};
        image_and_view.m_image = image;
        image_and_view.m_view = view;
        return image_and_view;
    }

    struct Config {
        VkExtent3D extent{};
        VkFormat format{VK_FORMAT_UNDEFINED};
        VkImageUsageFlags usage{};
        VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
        VkImageAspectFlags aspect_flags{VK_IMAGE_ASPECT_COLOR_BIT};
        u32 mip_levels = 1;
        VkSampleCountFlagBits sample_count{VK_SAMPLE_COUNT_1_BIT};
    };
    [[nodiscard]] static GpuImageAndView create(Vk& vk, const Config& config);

    struct CubemapConfig {
        ImageData data{};
        VkFormat format{VK_FORMAT_R8G8B8A8_SRGB};
        VkImageAspectFlags aspect_flags{VK_IMAGE_ASPECT_COLOR_BIT};
    };
    [[nodiscard]] static GpuImageAndView create_cubemap(Vk& vk, const CubemapConfig& config);

    void destroy(Vk& vk) const {
        m_image.destroy(vk);
        m_view.destroy(vk);
    }

    void write(Vk& vk, const GpuImage::WriteConfig& config) const {
        m_image.write(vk, config);
    }

    void generate_mipmaps(Vk& vk, u32 levels, VkExtent3D extent, VkFormat format, VkImageLayout final_layout) const;

private:
    GpuImage m_image{};
    GpuImageView m_view{};
};

inline u32 get_mip_count(const VkExtent3D extent) {
    ASSERT(extent.width > 0 || extent.height > 0 || extent.depth > 0);

    return static_cast<u32>(std::floor(std::log2(std::max(std::max(extent.width, extent.height), extent.depth)))) + 1;
}

class Sampler {
public:
    VkSampler get() const {
        ASSERT(m_sampler != nullptr);
        return m_sampler;
    }

    Sampler() = default;
    Sampler(VkSampler sampler) : m_sampler{sampler} {}

    enum Type {
        Nearest = VK_FILTER_NEAREST,
        Linear = VK_FILTER_LINEAR,
    };

    struct Config {
        Type type = Nearest;
        VkSamplerAddressMode edge_mode{VK_SAMPLER_ADDRESS_MODE_REPEAT};
        u32 mip_levels = 1;
    };
    [[nodiscard]] static Sampler create(Vk& vk, const Config& config);

    void destroy(Vk& vk) const {
        ASSERT(m_sampler != nullptr);
        vkDestroySampler(vk.device, m_sampler, nullptr);
    }

private:
    VkSampler m_sampler{};
};

class Texture {
public:
    VkImage get_image() const { return m_image.get_image(); }
    VkImageView get_view() const { return m_image.get_view(); }
    VkSampler get_sampler() const { return m_sampler.get(); }

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
        VkFormat format{VK_FORMAT_UNDEFINED};
        VkImageAspectFlags aspect_flags{VK_IMAGE_ASPECT_COLOR_BIT};
        bool create_mips = false;
        Sampler::Type sampler_type = Sampler::Nearest;
        VkSamplerAddressMode edge_mode{VK_SAMPLER_ADDRESS_MODE_REPEAT};
    };
    [[nodiscard]] static Texture create(Vk& vk, const ImageData& data, const Config& config);
    [[nodiscard]] static Texture create_cubemap(Vk& vk, const ImageData& data, const Config& config);

    void destroy(Vk& vk) const {
        m_image.destroy(vk);
        m_sampler.destroy(vk);
    }

private:
    GpuImageAndView m_image{};
    Sampler m_sampler{};
};

class DescriptorSetLayout {
public:
    VkDescriptorSetLayout get() const {
        ASSERT(m_descriptor_set_layout != nullptr);
        return m_descriptor_set_layout;
    }

    DescriptorSetLayout() = default;
    DescriptorSetLayout(VkDescriptorSetLayout layout) : m_descriptor_set_layout{layout} {}

    struct Config {
        Slice<const VkDescriptorSetLayoutBinding> bindings;
        Slice<const VkDescriptorBindingFlags> flags = {};
    };
    [[nodiscard]] static DescriptorSetLayout create(Vk& vk, const Config& config);

    void destroy(Vk& vk) const {
        ASSERT(m_descriptor_set_layout != nullptr);

        vkDestroyDescriptorSetLayout(vk.device, m_descriptor_set_layout, nullptr);
    }

private:
    VkDescriptorSetLayout m_descriptor_set_layout{};
};

class DescriptorPool {
public:
    VkDescriptorPool get() const {
        ASSERT(m_pool != nullptr);
        return m_pool;
    }

    DescriptorPool() = default;
    DescriptorPool(VkDescriptorPool pool) : m_pool{pool} {}

    struct Config {
        u32 max_sets = 0;
        Slice<const VkDescriptorPoolSize> descriptors{};
    };
    [[nodiscard]] static DescriptorPool create(Vk& vk, const Config& config);

    void destroy(Vk& vk) const {
        ASSERT(m_pool != nullptr);

        vkDestroyDescriptorPool(vk.device, m_pool, nullptr);
    }

    [[nodiscard]] Result<void> allocate_sets(
        Vk& vk,
        Slice<const VkDescriptorSetLayout> layouts,
        Slice<VkDescriptorSet> out_sets
    );
    [[nodiscard]] inline Result<VkDescriptorSet> allocate_set(Vk& vk, const VkDescriptorSetLayout layout) {
        auto set = ok<VkDescriptorSet>();
        const auto alloc_result = allocate_sets(vk, {&layout, 1}, {&*set, 1});
        if (alloc_result.has_err())
            return alloc_result.err();
        return set;
    }

private:
    VkDescriptorPool m_pool{};
};

void write_uniform_buffer_descriptor(
    Vk& vk, const GpuBuffer::View& buffer,
    VkDescriptorSet set, u32 binding, u32 binding_array_index = 0
);

void write_image_sampler_descriptor(
    Vk& vk, const Texture& texture,
    VkDescriptorSet set, u32 binding, u32 binding_array_index = 0
);

class PipelineLayout {
public:
    VkPipelineLayout get() const {
        ASSERT(m_pipeline_layout != nullptr);
        return m_pipeline_layout;
    }

    PipelineLayout() = default;
    PipelineLayout(VkPipelineLayout layout) : m_pipeline_layout{layout} {}

    struct Config {
        Slice<const VkDescriptorSetLayout> set_layouts{};
        Slice<const VkPushConstantRange> push_ranges{};
    };
    [[nodiscard]] static PipelineLayout create(Vk& vk, const Config& config);

    void destroy(Vk& vk) const {
        ASSERT(m_pipeline_layout != nullptr);
        vkDestroyPipelineLayout(vk.device, m_pipeline_layout, nullptr);
    }

private:
    VkPipelineLayout m_pipeline_layout{};
};

class UnlinkedShader {
public:
    VkShaderEXT get() const {
        ASSERT(m_shader != nullptr);
        return m_shader;
    }

    UnlinkedShader() = default;
    UnlinkedShader(VkShaderEXT shader) : m_shader{shader} {}

    struct Config {
        std::filesystem::path path{};
        VkShaderCodeTypeEXT code_type{VK_SHADER_CODE_TYPE_SPIRV_EXT};
        VkShaderStageFlagBits stage{};
        VkShaderStageFlagBits next_stage{};
        Slice<const VkDescriptorSetLayout> set_layouts{};
        Slice<const VkPushConstantRange> push_ranges{};
    };
    [[nodiscard]] static Result<UnlinkedShader> create(Vk& vk, const Config& config);

    void destroy(Vk& vk) const {
        ASSERT(m_shader != nullptr);

        auto vkDestroyShaderEXT = reinterpret_cast<PFN_vkDestroyShaderEXT>(
            vkGetDeviceProcAddr(vk.device, "vkDestroyShaderEXT")
        );
        if (vkDestroyShaderEXT == nullptr)
            ERROR("Could not find vkDestroyShaderEXT");

        vkDestroyShaderEXT(vk.device, m_shader, nullptr);
    }

private:
    VkShaderEXT m_shader{};
};

class GraphicsPipeline {
public:
    VkPipelineLayout get_layout() const { return m_layout.get(); }
    Slice<const VkShaderEXT> get_shaders() const {
        for (const auto shader : m_shaders) {
            ASSERT(shader != nullptr);
        }
        return m_shaders;
    }

    struct Config {
        Slice<const VkDescriptorSetLayout> set_layouts{};
        Slice<const VkPushConstantRange> push_ranges{};
        std::filesystem::path vertex_shader_path{};
        std::filesystem::path fragment_shader_path{};
        VkShaderCodeTypeEXT code_type{VK_SHADER_CODE_TYPE_SPIRV_EXT};
    };
    [[nodiscard]] static Result<GraphicsPipeline> create(Vk& vk, const Config& config);

    void destroy(Vk& vk) const {
        for (const auto shader : m_shaders) {
            ASSERT(shader != nullptr);

            auto vkDestroyShaderEXT = reinterpret_cast<PFN_vkDestroyShaderEXT>(
                vkGetDeviceProcAddr(vk.device, "vkDestroyShaderEXT")
            );
            if (vkDestroyShaderEXT == nullptr)
                ERROR("Could not find vkDestroyShaderEXT");

            vkDestroyShaderEXT(vk.device, shader, nullptr);
        }
        m_layout.destroy(vk);
    }

private:
    PipelineLayout m_layout{};
    std::array<VkShaderEXT, 2> m_shaders{};
};

class Fence {
public:
    VkFence get() const {
        ASSERT(m_fence != nullptr);
        return m_fence;
    }

    Fence() = default;
    Fence(VkFence fence) : m_fence{fence} {}

    struct Config {
        VkFenceCreateFlags flags{};
    };
    [[nodiscard]] static Fence create(Vk& vk, const Config& config);

    void destroy(Vk& vk) const {
        ASSERT(m_fence != nullptr);
        vkDestroyFence(vk.device, m_fence, nullptr);
    }

    void wait(Vk& vk) const;
    void reset(Vk& vk) const;

private:
    VkFence m_fence = nullptr;
};

class Semaphore {
public:
    VkSemaphore get() const {
        ASSERT(m_semaphore != nullptr);
        return m_semaphore;
    }
    VkSemaphore* ptr() {
        ASSERT(m_semaphore != nullptr);
        return &m_semaphore;
    }

    Semaphore() = default;
    Semaphore(VkSemaphore semaphore) : m_semaphore{semaphore} {}

    [[nodiscard]] static Semaphore create(Vk& vk);

    void destroy(Vk& vk) const {
        ASSERT(m_semaphore != nullptr);
        vkDestroySemaphore(vk.device, m_semaphore, nullptr);
    }

private:
    VkSemaphore m_semaphore = nullptr;
};

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
    BarrierBuilder(Vk& vk, const Config& config) {
        if (config.memory_barriers > 0) {
            m_memories = vk.stack.alloc<VkMemoryBarrier2>(config.memory_barriers);
            for (auto& barrier : m_memories) { barrier = {.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2}; }
        }
        if (config.buffer_barriers > 0) {
            m_buffers = vk.stack.alloc<VkBufferMemoryBarrier2>(config.buffer_barriers);
            for (auto& barrier : m_buffers) { barrier = {.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2}; }
        }
        if (config.image_barriers > 0) {
            m_images = vk.stack.alloc<VkImageMemoryBarrier2>(config.image_barriers);
            for (auto& barrier : m_images) { barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2}; }
        }
    }
    void build_and_run(Vk& vk, const VkCommandBuffer cmd, const VkDependencyFlags flags = {}) {
        const VkDependencyInfo dependency_info{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags = flags,
            .memoryBarrierCount = to_u32(m_memories.count),
            .pMemoryBarriers = m_memories.data,
            .bufferMemoryBarrierCount = to_u32(m_buffers.count),
            .pBufferMemoryBarriers = m_buffers.data,
            .imageMemoryBarrierCount = to_u32(m_images.count),
            .pImageMemoryBarriers = m_images.data,
        };
        vkCmdPipelineBarrier2(cmd, &dependency_info);

        if (m_images.data != nullptr)
            vk.stack.dealloc(m_images);
        if (m_buffers.data != nullptr)
            vk.stack.dealloc(m_buffers);
        if (m_memories.data != nullptr)
            vk.stack.dealloc(m_memories);
    }

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

class Surface {
public:
    [[nodiscard]] VkSurfaceKHR get() const {
        ASSERT(m_surface != nullptr);
        return m_surface;
    }

    [[nodiscard]] static Surface create(Vk& vk, SDL_Window* window);
    void destroy(Vk& vk) const {
        ASSERT(m_surface != nullptr);
        vkDestroySurfaceKHR(vk.instance, m_surface, nullptr);
    }

private:
    VkSurfaceKHR m_surface{};
};

class Swapchain {
public:
    static constexpr u32 MaxFramesInFlight = 2;
    static constexpr u32 MaxImages = 3;

    static constexpr VkColorSpaceKHR SwapchainColorSpace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    [[nodiscard]] static Result<Swapchain> create(Vk& vk, VkSurfaceKHR surface);
    void destroy(Vk& vk) const;
    [[nodiscard]] Result<void> resize(Vk& vk, VkSurfaceKHR surface);

    VkFormat get_format() const { return m_format; }
    struct DrawInfo {
        VkCommandBuffer cmd{};
        VkImage render_target{};
        VkExtent2D extent{};
    };
    [[nodiscard]] Result<DrawInfo> begin_frame(Vk& vk);
    [[nodiscard]] Result<void> end_frame(Vk& vk);

private:
    [[nodiscard]] VkCommandBuffer& current_cmd() { return m_command_buffers[m_current_frame_index]; }
    [[nodiscard]] VkImage& current_image() { return m_images[m_current_image_index]; }
    [[nodiscard]] Fence& is_frame_finished() { return m_frame_finished_fences[m_current_frame_index]; }
    [[nodiscard]] Semaphore& is_image_available() { return m_image_available_semaphores[m_current_frame_index]; }
    [[nodiscard]] Semaphore& is_ready_to_present() { return m_ready_to_present_semaphores[m_current_image_index]; }
    [[nodiscard]] const VkCommandBuffer& current_cmd() const { return m_command_buffers[m_current_frame_index]; }
    [[nodiscard]] const VkImage& current_image() const { return m_images[m_current_image_index]; }
    [[nodiscard]] const Fence& is_frame_finished() const { return m_frame_finished_fences[m_current_frame_index]; }
    [[nodiscard]] const Semaphore& is_image_available() const { return m_image_available_semaphores[m_current_frame_index]; }
    [[nodiscard]] const Semaphore& is_ready_to_present() const { return m_ready_to_present_semaphores[m_current_image_index]; }

    VkExtent2D m_extent{};
    VkSwapchainKHR m_swapchain{};
    std::array<VkImage, MaxImages> m_images{};
    u32 m_image_count = 0;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    u32 m_current_image_index = 0;
    u32 m_current_frame_index = 0;
    bool m_recording = false;
    std::array<VkCommandBuffer, MaxFramesInFlight> m_command_buffers{};
    std::array<Fence, MaxFramesInFlight> m_frame_finished_fences{};
    std::array<Semaphore, MaxFramesInFlight> m_image_available_semaphores{};
    std::array<Semaphore, MaxImages> m_ready_to_present_semaphores{};
};

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

} // namespace hg
