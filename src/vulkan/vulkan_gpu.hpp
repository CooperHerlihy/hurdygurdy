#pragma once

#include "hg/gpu.hpp"
#include "hg/inttypes.hpp"
#include "hg/format.hpp"
#include "hg/hash.hpp"

#include <array>
#include <vector>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace hg {
namespace vk {

using Descriptor = u32;

enum class DescriptorType : u32 {
    combinedImageSampler,
    storageImage,
    uniformBuffer,
    storageBuffer,
    count,
};

struct DescriptorPool {
    std::vector<u32> freed{};
    u32 next = 0;

    u32 alloc();
    void free(u32 desc);
};

struct SamplerInfo {
    GpuFilter filter = GpuFilter::nearest;
    GpuSamplerEdgeMode mode = GpuSamplerEdgeMode::repeat;
    GpuSamplerBorder border = GpuSamplerBorder::floatTransparentBlack;
};

constexpr bool operator==(const SamplerInfo& lhs, const SamplerInfo& rhs)
{
    return lhs.filter == rhs.filter && lhs.mode == rhs.mode && lhs.border == rhs.border;
}

} // namespace vk
template<>
constexpr u64 hash(const vk::SamplerInfo& info)
{
    return (static_cast<u64>(info.border)) + (static_cast<u64>(info.mode) << 4) + (static_cast<u64>(info.filter) << 8);
}
namespace vk {

struct Frame {
    std::vector<GpuSwapchain*> swapchains = {};
    VkCommandPool cmdPool = nullptr;
    VkCommandBuffer cmd = nullptr;
    VkFence fence = nullptr;
};

class GpuVk : public Gpu {
public:
    bool enableDebugMessenger = false;
    bool enableSurfaces = false;
    VkInstance instance = nullptr;
    VkDebugUtilsMessengerEXT debugMessenger = nullptr;
    VkPhysicalDevice physicalDevice = nullptr;
    VkDevice device = nullptr;
    VmaAllocator vma = nullptr;
    VkQueue queue = nullptr;
    u32 queueFamily = static_cast<u32>(-1);
    VkCommandPool cmdPool = nullptr;
    VkDescriptorPool bindlessPool = nullptr;
    VkDescriptorSetLayout bindlessLayout = nullptr;
    VkDescriptorSet bindlessSet = nullptr;
    std::array<DescriptorPool, static_cast<usize>(DescriptorType::count)> descriptorPools;
    HashMap<SamplerInfo, VkSampler> samplers;
    std::vector<Frame> frames{};
    u32 currentFrame = 0;

    static std::unique_ptr<GpuVk> create(std::span<std::string_view> extensions);

    void waitIdle() override;
    std::unique_ptr<GpuBuffer> createBuffer(
        u64 size,
        GpuBufferUsageFlags usageFlags,
        GpuMemoryUsage memoryUsage = GpuMemoryUsage::deviceOnly) override;
    std::unique_ptr<GpuImage> createImage(u32 width, u32 height, Format format, GpuImageUsageFlags usage) override;
    std::unique_ptr<GpuImage> createImage(const GpuImageCreateInfo& config) override;
    std::unique_ptr<GpuPipeline> createGraphicsPipeline(const GpuGraphicsPipelineCreateInfo& config) override;
    std::unique_ptr<GpuPipeline> createComputePipeline(std::span<const byte> shaderCode, u32 pushSize = 0) override;
    std::unique_ptr<GpuCommandBuffer> beginCommands() override;
    void endCommands(std::unique_ptr<GpuCommandBuffer> cmd) override;
    std::unique_ptr<GpuCommandBuffer> beginFrame(std::span<Window*> windows) override;
    void endFrame(std::unique_ptr<GpuCommandBuffer> cmd) override;
    std::unique_ptr<GpuSwapchain> createSwapchain(
        void* surface,
        u32 width,
        u32 height,
        GpuPresentMode presentMode,
        GpuImageUsageFlags imageUsage) override;

    ~GpuVk() noexcept override;

    constexpr GpuVk() noexcept = default;

    GpuVk(GpuVk&& other) noexcept
        : enableDebugMessenger{other.enableDebugMessenger}
        , enableSurfaces{other.enableSurfaces}
        , instance{std::exchange(other.instance, nullptr)}
        , debugMessenger{std::exchange(other.debugMessenger, nullptr)}
        , physicalDevice{std::exchange(other.physicalDevice, nullptr)}
        , device{std::exchange(other.device, nullptr)}
        , vma{std::exchange(other.vma, nullptr)}
        , queue{std::exchange(other.queue, nullptr)}
        , queueFamily{std::exchange(other.queueFamily, static_cast<u32>(-1))}
        , cmdPool{std::exchange(other.cmdPool, nullptr)}
        , bindlessPool{std::exchange(other.bindlessPool, nullptr)}
        , bindlessLayout{std::exchange(other.bindlessLayout, nullptr)}
        , bindlessSet{std::exchange(other.bindlessSet, nullptr)}
        , descriptorPools{std::exchange(other.descriptorPools, {})}
        , samplers{std::exchange(other.samplers, {})}
        , frames{std::exchange(other.frames, {})}
        , currentFrame{other.currentFrame}
    {}

    GpuVk& operator=(GpuVk&& other) noexcept
    {
        if (this != &other)
        {
            this->~GpuVk();
            new (this) GpuVk{std::move(other)};
        }
        return *this;
    }

    GpuVk(const GpuVk&) = delete;
    GpuVk& operator=(const GpuVk&) = delete;
};

enum class GpuMemoryHostAccess : u32 {
    none = 0x0,
    write = 0x1,
    read = 0x2,
};

class GpuBufferVk : public GpuBuffer {
public:
    GpuVk* vk = nullptr;
    VkBuffer buffer = nullptr;
    VmaAllocation alloc = nullptr;
    Descriptor uniformDesc = {};
    Descriptor storageDesc = {};

    u64 bufSize = 0;
    GpuBufferUsageFlags usage = 0;
    GpuMemoryHostAccess memoryAccess = GpuMemoryHostAccess::none;
    mutable GpuAccess lastState = GpuAccess::transferDst;

    u64 size() const override;
    u32 uniformDescriptor() const override;
    u32 storageDescriptor() const override;
    void write(std::span<const byte> src, u64 offset) override;
    void read(std::span<byte> dst, u64 offset) const override;

    ~GpuBufferVk() noexcept override;

    constexpr GpuBufferVk() noexcept = default;

    GpuBufferVk(GpuBufferVk&& other) noexcept
        : vk{std::exchange(other.vk, nullptr)}
        , buffer{std::exchange(other.buffer, nullptr)}
        , alloc{std::exchange(other.alloc, nullptr)}
        , uniformDesc{std::exchange(other.uniformDesc, {})}
        , storageDesc{std::exchange(other.storageDesc, {})}
        , bufSize{other.bufSize}
        , usage{other.usage}
        , memoryAccess{other.memoryAccess}
        , lastState{other.lastState}
    {}

    GpuBufferVk& operator=(GpuBufferVk&& other) noexcept
    {
        if (this != &other)
        {
            this->~GpuBufferVk();
            new (this) GpuBufferVk{std::move(other)};
        }
        return *this;
    }

    GpuBufferVk(const GpuBufferVk&) = delete;
    GpuBufferVk& operator=(const GpuBufferVk&) = delete;
};

class GpuImageVk : public GpuImage {
public:
    GpuVk* vk = nullptr;
    VkImage image = nullptr;
    VmaAllocation alloc = nullptr;
    GpuImageUsageFlags usage = 0;
    Format imageFormat = Format::undefined;
    u32 imageWidth = 0;
    u32 imageHeight = 0;
    u32 imageDepth = 0;
    u8 dimensions = 0;
    u8 mipLevels = 0;
    u8 arrayLayers = 0;
    u8 msaaSamples = 0;
    mutable GpuAccess lastState = GpuAccess::transferDst;
    mutable GpuLayout lastLayout = GpuLayout::undefined;

    Format format() const override;
    u32 width() const override;
    u32 height() const override;
    u32 depth() const override;
    u32 mipmapCount() const override;
    u32 layerCount() const override;
    std::unique_ptr<GpuView> createView(GpuFilter filter = GpuFilter::nearest) override;
    std::unique_ptr<GpuView> createView(const GpuViewCreateInfo& config) override;
    void write(std::span<const byte> src, const GpuImageRegion& dst) override;
    virtual void writeWhole(std::span<const byte> src, u32 mipLevel = 0) override;
    void writeCubemap(std::span<const byte> src, u32 mipLevel = 0) override;
    void generateMipmaps(u32 arrayLayer = 0) override;
    void read(std::span<byte> dst, const GpuImageRegion& src) override;
    void readWhole(std::span<byte> dst, u32 mipLevel = 0) override;
    void readCubemap(std::span<byte> dst, u32 mipLevel = 0) override;

    ~GpuImageVk() noexcept override;

    constexpr GpuImageVk() noexcept = default;

    GpuImageVk(GpuImageVk&& other) noexcept
        : vk{std::exchange(other.vk, nullptr)}
        , image{std::exchange(other.image, nullptr)}
        , alloc{std::exchange(other.alloc, nullptr)}
        , usage{other.usage}
        , imageFormat{other.imageFormat}
        , imageWidth{other.imageWidth}
        , imageHeight{other.imageHeight}
        , imageDepth{other.imageDepth}
        , dimensions{other.dimensions}
        , mipLevels{other.mipLevels}
        , arrayLayers{other.arrayLayers}
        , msaaSamples{other.msaaSamples}
        , lastState{other.lastState}
        , lastLayout{other.lastLayout}
    {}

    GpuImageVk& operator=(GpuImageVk&& other) noexcept
    {
        if (this != &other)
        {
            this->~GpuImageVk();
            new (this) GpuImageVk{std::move(other)};
        }
        return *this;
    }

    GpuImageVk(const GpuImageVk&) = delete;
    GpuImageVk& operator=(const GpuImageVk&) = delete;
};

class GpuViewVk : public GpuView {
public:
    GpuVk* vk = nullptr;
    GpuImageVk* image = nullptr;
    VkImageView view = nullptr;
    VkSampler sampler = nullptr;
    Descriptor samplerDesc = {};
    Descriptor storageDesc = {};
    GpuImageUsageFlags usage = 0;
    GpuViewType type = GpuViewType::image1D;
    u8 baseMipLevel = 0;
    u8 levelCount = 0;
    u8 baseArrayLayer = 0;
    u8 arrayLayerCount = 0;

    Format format() const override;
    u32 width() const override;
    u32 height() const override;
    u32 depth() const override;
    u32 mipmapCount() const override;
    u32 layerCount() const override;
    u32 samplerDescriptor() const override;
    u32 storageDescriptor() const override;

    ~GpuViewVk() noexcept override;

    constexpr GpuViewVk() noexcept = default;

    GpuViewVk(GpuViewVk&& other) noexcept
        : vk{std::exchange(other.vk, nullptr)}
        , image{other.image}
        , view{std::exchange(other.view, nullptr)}
        , sampler{other.sampler}
        , samplerDesc{std::exchange(other.samplerDesc, {})}
        , storageDesc{std::exchange(other.storageDesc, {})}
        , usage{other.usage}
        , type{other.type}
        , baseMipLevel{other.baseMipLevel}
        , levelCount{other.levelCount}
        , baseArrayLayer{other.baseArrayLayer}
        , arrayLayerCount{other.arrayLayerCount}
    {}

    GpuViewVk& operator=(GpuViewVk&& other) noexcept
    {
        if (this != &other)
        {
            this->~GpuViewVk();
            new (this) GpuViewVk{std::move(other)};
        }
        return *this;
    }

    GpuViewVk(const GpuViewVk&) = delete;
    GpuViewVk& operator=(const GpuViewVk&) = delete;
};

class GpuPipelineVk : public GpuPipeline {
public:
    GpuVk* vk = nullptr;
    VkPipeline pipeline = nullptr;
    VkPipelineLayout layout = nullptr;
    VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    u32 pushSize = 0;

    ~GpuPipelineVk() noexcept override;

    constexpr GpuPipelineVk() noexcept = default;

    GpuPipelineVk(GpuPipelineVk&& other) noexcept
        : vk{std::exchange(other.vk, nullptr)}
        , pipeline{std::exchange(other.pipeline, nullptr)}
        , layout{std::exchange(other.layout, nullptr)}
        , bindPoint{other.bindPoint}
        , pushSize{other.pushSize}
    {}

    GpuPipelineVk& operator=(GpuPipelineVk&& other) noexcept
    {
        if (this != &other)
        {
            this->~GpuPipelineVk();
            new (this) GpuPipelineVk{std::move(other)};
        }
        return *this;
    }

    GpuPipelineVk(const GpuPipelineVk&) = delete;
    GpuPipelineVk& operator=(const GpuPipelineVk&) = delete;
};

class GpuCommandBufferVk : public GpuCommandBuffer {
public:
    GpuVk* vk = nullptr;
    VkCommandBuffer cmd = nullptr;

    void memoryBarrier (
        std::span<const GpuBufferBarrier> bufferBarriers,
        std::span<const GpuImageBarrier> imageBarriers) override;
    void copyBuffer(
        const GpuBuffer* dst,
        usize dstOffset,
        const GpuBuffer* src,
        usize srcOffset,
        usize size) override;
    void copyImage(
        const GpuImage* dst,
        const GpuImageRegion& dstRegion,
        const GpuImage* src,
        const GpuImageRegion& srcRegion) override;
    void copyImageWhole(
        const GpuImage* dst,
        const GpuImage* src) override;
    void blitImage(
        const GpuImage* dst,
        const GpuImageRegion& dstRegion,
        const GpuImage* src,
        const GpuImageRegion& srcRegion) override;
    void blitImageWhole(
        const GpuImage* dst,
        const GpuImage* src) override;
    void computePass(const GpuPass& pass) override;
    void beginRenderPass(const GpuPass& pass) override;
    void endRenderPass() override;
    void gpuSetViewport(f32 x, f32 y, f32 width, f32 height, f32 near = 0.0f, f32 far = 1.0f) override;
    void gpuSetScissor(i32 x, i32 y, u32 width, u32 height) override;
    void bindPipeline(const GpuPipeline* pipeline) override;
    void pushConstants(const GpuPipeline* pipeline, std::span<const byte> push) override;
    void draw(u32 vertexBegin, u32 vertexCount, u32 instanceBegin, u32 instanceCount) override;
    void dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ) override;

    ~GpuCommandBufferVk() noexcept override;

    constexpr GpuCommandBufferVk() noexcept = default;

    GpuCommandBufferVk(GpuCommandBufferVk&& other) noexcept
        : vk{std::exchange(other.vk, nullptr)}
        , cmd{std::exchange(other.cmd, nullptr)}
    {}

    GpuCommandBufferVk& operator=(GpuCommandBufferVk&& other) noexcept
    {
        if (this != &other)
        {
            this->~GpuCommandBufferVk();
            new (this) GpuCommandBufferVk{std::move(other)};
        }
        return *this;
    }

    GpuCommandBufferVk(const GpuCommandBufferVk&) = delete;
    GpuCommandBufferVk& operator=(const GpuCommandBufferVk&) = delete;
};

class GpuSwapchainVk : public GpuSwapchain {
public:
    GpuVk* vk = nullptr;
    VkSurfaceKHR surface = nullptr;
    VkSwapchainKHR swapchain = nullptr;
    std::vector<GpuImageVk> images{};
    std::vector<GpuViewVk> views{};
    std::vector<VkSemaphore> imageAvailable{};
    std::vector<VkSemaphore> readyToPresent{};
    u32 imageIdx = (u32)-1;
    u32 imageWidth = 0;
    u32 imageHeight = 0;
    Format imageFormat = Format::undefined;
    GpuImageUsageFlags imageUsage{};
    GpuPresentMode presentMode{};

    void resize(u32 width, u32 height) override;
    u32 width() const override;
    u32 height() const override;
    Format format() const override;
    u32 imageCount() const override;
    bool frameAvailable() const override;
    GpuImage* currentImage() const override;
    GpuView* currentView() const override;

    ~GpuSwapchainVk() noexcept override;

    constexpr GpuSwapchainVk() noexcept = default;

    GpuSwapchainVk(GpuSwapchainVk&& other) noexcept
        : vk{std::exchange(other.vk, nullptr)}
        , surface{std::exchange(other.surface, nullptr)}
        , swapchain{std::exchange(other.swapchain, nullptr)}
        , images{std::exchange(other.images, {})}
        , views{std::exchange(other.views, {})}
        , imageAvailable{std::exchange(other.imageAvailable, {})}
        , readyToPresent{std::exchange(other.readyToPresent, {})}
        , imageIdx{std::exchange(other.imageIdx, (u32)-1)}
        , imageWidth{std::exchange(other.imageWidth, 0)}
        , imageHeight{std::exchange(other.imageHeight, 0)}
        , imageFormat{std::exchange(other.imageFormat, Format::undefined)}
        , imageUsage{std::exchange(other.imageUsage, {})}
        , presentMode{std::exchange(other.presentMode, {})}
    {}

    GpuSwapchainVk& operator=(GpuSwapchainVk&& other) noexcept
    {
        if (this != &other)
        {
            this->~GpuSwapchainVk();
            new (this) GpuSwapchainVk{std::move(other)};
        }
        return *this;
    }

    GpuSwapchainVk(const GpuSwapchainVk&) = delete;
    GpuSwapchainVk& operator=(const GpuSwapchainVk&) = delete;
};

constexpr const char* vkResultToStr(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_VALIDATION_FAILED:
            return "VK_ERROR_VALIDATION_FAILED";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_NOT_PERMITTED:
            return "VK_ERROR_NOT_PERMITTED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT:
            return "VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
            return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
        case VK_PIPELINE_BINARY_MISSING_KHR:
            return "VK_PIPELINE_BINARY_MISSING_KHR";
        case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
            return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
    }
    return "Unrecognized Vulkan result";
}

constexpr VkFormat formatToVk(Format format)
{
    switch (format)
    {
        case Format::undefined: return VK_FORMAT_UNDEFINED;
        case Format::r8_unorm: return VK_FORMAT_R8_UNORM;
        case Format::rg8_unorm: return VK_FORMAT_R8G8_UNORM;
        case Format::rgba8_unorm: return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::bgra8_unorm: return VK_FORMAT_B8G8R8A8_UNORM;
        case Format::rgba8_srgb: return VK_FORMAT_R8G8B8A8_SRGB;
        case Format::bgra8_srgb: return VK_FORMAT_B8G8R8A8_SRGB;
        case Format::r16_unorm: return VK_FORMAT_R16_UNORM;
        case Format::r16_sfloat: return VK_FORMAT_R16_SFLOAT;
        case Format::rg16_unorm: return VK_FORMAT_R16G16_UNORM;
        case Format::rg16_sfloat: return VK_FORMAT_R16G16_SFLOAT;
        case Format::rgba16_unorm: return VK_FORMAT_R16G16B16A16_UNORM;
        case Format::rgba16_sfloat: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case Format::r32_uint: return VK_FORMAT_R32_UINT;
        case Format::r32_sfloat: return VK_FORMAT_R32_SFLOAT;
        case Format::rg32_sfloat: return VK_FORMAT_R32G32_SFLOAT;
        case Format::rgba32_sfloat: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case Format::a2b10g10r10_unorm_pack32: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case Format::b10g11r11_ufloat_pack32: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case Format::d16_unorm: return VK_FORMAT_D16_UNORM;
        case Format::d32_sfloat: return VK_FORMAT_D32_SFLOAT;
        case Format::s8_uint: return VK_FORMAT_S8_UINT;
        case Format::d16_unorm_s8_uint: return VK_FORMAT_D16_UNORM_S8_UINT;
        case Format::d24_unorm_s8_uint: return VK_FORMAT_D24_UNORM_S8_UINT;
        case Format::d32_sfloat_s8_uint: return VK_FORMAT_D32_SFLOAT_S8_UINT;
    }
    panic("Unknown Vulkan format\n");
    return VK_FORMAT_UNDEFINED;
}

constexpr VkImageAspectFlags gpuFormatToAspect(Format format)
{
    switch (format)
    {
        case Format::undefined: return VK_IMAGE_ASPECT_NONE;
        case Format::r8_unorm: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::rg8_unorm: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::rgba8_unorm: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::bgra8_unorm: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::rgba8_srgb: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::bgra8_srgb: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::r16_unorm: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::r16_sfloat: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::rg16_unorm: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::rg16_sfloat: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::rgba16_unorm: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::rgba16_sfloat: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::r32_uint: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::r32_sfloat: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::rg32_sfloat: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::rgba32_sfloat: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::a2b10g10r10_unorm_pack32: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::b10g11r11_ufloat_pack32: return VK_IMAGE_ASPECT_COLOR_BIT;
        case Format::d16_unorm: return VK_IMAGE_ASPECT_DEPTH_BIT;
        case Format::d32_sfloat: return VK_IMAGE_ASPECT_DEPTH_BIT;
        case Format::s8_uint: return VK_IMAGE_ASPECT_STENCIL_BIT;
        case Format::d16_unorm_s8_uint: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        case Format::d24_unorm_s8_uint: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        case Format::d32_sfloat_s8_uint: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    panic("Unknown Vulkan format\n");
    return VK_FORMAT_UNDEFINED;
}

constexpr VkPipelineStageFlags gpuStateToVkStage(GpuAccess state)
{
    switch (state)
    {
        case GpuAccess::indirectBuffer: return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        case GpuAccess::uniformBufferCompute: return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case GpuAccess::uniformBufferVertex: return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        case GpuAccess::uniformBufferFragment: return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case GpuAccess::uniformBufferAllGraphics: return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case GpuAccess::storageBufferCompute: return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case GpuAccess::storageBufferVertex: return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        case GpuAccess::storageBufferFragment: return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case GpuAccess::storageBufferAllGraphics: return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case GpuAccess::sampledImageCompute: return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case GpuAccess::sampledImageFragment: return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case GpuAccess::storageImageCompute: return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case GpuAccess::storageImageFragment: return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case GpuAccess::colorAttachment: return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case GpuAccess::depthStencilAttachment: return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case GpuAccess::transferSrc: return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case GpuAccess::transferDst: return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case GpuAccess::hostRead: return VK_PIPELINE_STAGE_HOST_BIT;
    }
    return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
}

constexpr VkAccessFlags gpuStateToVkAccess(GpuAccess state)
{
    switch (state)
    {
        case GpuAccess::indirectBuffer: return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        case GpuAccess::uniformBufferCompute:
        case GpuAccess::uniformBufferVertex:
        case GpuAccess::uniformBufferFragment:
        case GpuAccess::uniformBufferAllGraphics: return VK_ACCESS_UNIFORM_READ_BIT;
        case GpuAccess::storageBufferCompute:
        case GpuAccess::storageBufferVertex:
        case GpuAccess::storageBufferFragment:
        case GpuAccess::storageBufferAllGraphics: return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        case GpuAccess::sampledImageCompute:
        case GpuAccess::sampledImageFragment: return VK_ACCESS_SHADER_READ_BIT;
        case GpuAccess::storageImageCompute:
        case GpuAccess::storageImageFragment: return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        case GpuAccess::colorAttachment: return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case GpuAccess::depthStencilAttachment: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case GpuAccess::transferSrc: return VK_ACCESS_TRANSFER_READ_BIT;
        case GpuAccess::transferDst: return VK_ACCESS_TRANSFER_WRITE_BIT;
        case GpuAccess::hostRead: return VK_ACCESS_HOST_READ_BIT;
    }
    return VK_ACCESS_NONE;
}

constexpr VkBufferUsageFlags gpuBufferUsageToVk(GpuBufferUsageFlags usage)
{
    return static_cast<VkBufferUsageFlags>(usage);
}

constexpr VkImageUsageFlags gpuImageUsageToVk(GpuImageUsageFlags usage)
{
    return static_cast<VkImageUsageFlags>(usage);
}

constexpr VkImageLayout gpuLayoutToVk(GpuLayout layout)
{
    switch (layout)
    {
        case GpuLayout::undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
        case GpuLayout::general: return VK_IMAGE_LAYOUT_GENERAL;
        case GpuLayout::colorAttachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case GpuLayout::depthStencilAttachment: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case GpuLayout::shaderReadOnly: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case GpuLayout::transferSrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case GpuLayout::transferDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case GpuLayout::presentSrc: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    panic("Unknown layout\n");
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

constexpr VkImageViewType gpuViewTypeToVk(GpuViewType type)
{
    return static_cast<VkImageViewType>(type);
}

constexpr VkFilter gpuFilterToVk(GpuFilter filter)
{
    return static_cast<VkFilter>(filter);
}

constexpr VkSamplerAddressMode gpuSamplerAddressModeToVk(GpuSamplerEdgeMode mode)
{
    return static_cast<VkSamplerAddressMode>(mode);
}

constexpr VkBorderColor gpuSamplerBorderToVk(GpuSamplerBorder color)
{
    return static_cast<VkBorderColor>(color);
}

constexpr VkPrimitiveTopology gpuTopologyToVk(GpuTopology topology)
{
    return static_cast<VkPrimitiveTopology>(topology);
}

constexpr VkPolygonMode gpuPolygonModeToVk(GpuPolygonMode mode)
{
    return static_cast<VkPolygonMode>(mode);
}

constexpr VkCullModeFlags gpuCullModeToVk(GpuCullMode mode)
{
    return static_cast<VkCullModeFlags>(mode);
}

constexpr VkAttachmentLoadOp gpuLoadOpToVk(GpuLoadOp op)
{
    return static_cast<VkAttachmentLoadOp>(op);
}

constexpr VkAttachmentStoreOp gpuStoreOpToVk(GpuStoreOp op)
{
    return static_cast<VkAttachmentStoreOp>(op);
}

constexpr VkPresentModeKHR presentModeToVk(GpuPresentMode mode)
{
    return static_cast<VkPresentModeKHR>(mode);
}

constexpr VkDescriptorType descriptorTypeToVk(DescriptorType type)
{
    switch (type)
    {
        case DescriptorType::combinedImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DescriptorType::storageImage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case DescriptorType::uniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescriptorType::storageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        default:
            panic("invalid GpuDescriptorType: {}", static_cast<u32>(type));
    }
}

constexpr VmaAllocationCreateFlags gpuMemoryUsageToVma(GpuMemoryUsage usage)
{
    switch (usage)
    {
        case GpuMemoryUsage::deviceOnly:
            return 0;
        case GpuMemoryUsage::stagingWrite:
            return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        case GpuMemoryUsage::stagingRead:
            return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        case GpuMemoryUsage::frequentUpdate:
            return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                   VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        default:
            panic("Invalid GpuMemoryUsage: {}\n", static_cast<u32>(usage));
    }
}

constexpr GpuMemoryHostAccess gpuMemoryUsageToHostAccess(GpuMemoryUsage usage)
{
    switch (usage)
    {
        case GpuMemoryUsage::deviceOnly:
            return GpuMemoryHostAccess::none;
        case GpuMemoryUsage::stagingWrite:
            return GpuMemoryHostAccess::write;
        case GpuMemoryUsage::stagingRead:
            return GpuMemoryHostAccess::read;
        case GpuMemoryUsage::frequentUpdate:
            return GpuMemoryHostAccess::none;
        default:
            panic("Invalid GpuMemoryUsage: {}\n", static_cast<u32>(usage));
    }
}

constexpr VkImageType imageDimensionsToVkImage(u32 dimensions)
{
    switch (dimensions)
    {
        case 1:
            return VK_IMAGE_TYPE_1D;
        case 2:
            return VK_IMAGE_TYPE_2D;
        case 3:
            return VK_IMAGE_TYPE_3D;
        default:
            panic("Invalid image dimensions: {}\n", dimensions);
    }
}

constexpr VkImageCreateFlags gpuImageConfigFlagsToVk(GpuImageConfigFlags flags)
{
    return static_cast<VkImageCreateFlags>(flags);
}

constexpr VkSampleCountFlagBits countToMsaaSampleBits(u32 count)
{
    switch (count)
    {
        case 1:
            return VK_SAMPLE_COUNT_1_BIT;
        case 2:
            return VK_SAMPLE_COUNT_2_BIT;
        case 4:
            return VK_SAMPLE_COUNT_4_BIT;
        case 8:
            return VK_SAMPLE_COUNT_8_BIT;
        case 16:
            return VK_SAMPLE_COUNT_16_BIT;
        case 32:
            return VK_SAMPLE_COUNT_32_BIT;
        case 64:
            return VK_SAMPLE_COUNT_64_BIT;
        default:
            panic("Invalid msaa sample count\n");
    }
}

constexpr GpuViewType imageDimensionsToHgView(u32 dimensions)
{
    switch (dimensions)
    {
        case 1:
            return GpuViewType::image1D;
        case 2:
            return GpuViewType::image2D;
        case 3:
            return GpuViewType::image3D;
        default:
            panic("Invalid image dimensions: {}\n", dimensions);
    }
}

} // namespace vk
} // namespace hg
