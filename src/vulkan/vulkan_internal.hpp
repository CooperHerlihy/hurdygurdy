#pragma once

#include "internal.hpp"
#include "hg/gpu.hpp"
#include "hg/map.hpp"
#include "hg/pool.hpp"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <utility>

namespace hg {

using GpuDescriptor = Handle;

struct GpuBufferData {
    VkBuffer buffer = nullptr;
    VmaAllocation alloc = nullptr;
    u64 size = 0;
    GpuDescriptor uniformDesc = {};
    GpuDescriptor storageDesc = {};
    GpuBufferUsageFlags usage = 0;
    GpuMemoryHostAccess access = GpuMemoryHostAccess_none;
    GpuStageFlags lastStage = 0;
    GpuAccessFlags lastAccess = 0;

    GpuBufferData() = default;
    ~GpuBufferData();

    GpuBufferData(GpuBufferData&& other) noexcept
        : buffer{std::exchange(other.buffer, nullptr)}
        , alloc{std::exchange(other.alloc, nullptr)}
        , size{other.size}
        , uniformDesc{std::exchange(other.uniformDesc, {})}
        , storageDesc{std::exchange(other.storageDesc, {})}
        , usage{other.usage}
        , access{other.access}
        , lastStage{other.lastStage}
        , lastAccess{other.lastAccess}
    {}

    GpuBufferData& operator=(GpuBufferData&& other) noexcept
    {
        if (this != &other)
        {
            this->~GpuBufferData();
            new (this) GpuBufferData{std::move(other)};
        }
        return *this;
    }

    GpuBufferData(const GpuBufferData&) = delete;
    GpuBufferData& operator=(const GpuBufferData&) = delete;
};

struct GpuImageData {
    VkImage image = nullptr;
    VmaAllocation alloc = nullptr;
    GpuImageUsageFlags usage = 0;
    Format format = Format_undefined;
    u32 width = 0;
    u32 height = 0;
    u32 depth = 0;
    u8 dimensions = 0;
    u8 mipLevels = 0;
    u8 arrayLayers = 0;
    u8 msaaSamples = 0;

    GpuImageData() = default;
    ~GpuImageData();

    GpuImageData(GpuImageData&& other) noexcept
        : image{std::exchange(other.image, nullptr)}
        , alloc{std::exchange(other.alloc, nullptr)}
        , usage{other.usage}
        , format{other.format}
        , width{other.width}
        , height{other.height}
        , depth{other.depth}
        , dimensions{other.dimensions}
        , mipLevels{other.mipLevels}
        , arrayLayers{other.arrayLayers}
        , msaaSamples{other.msaaSamples}
    {}

    GpuImageData& operator=(GpuImageData&& other) noexcept
    {
        if (this != &other)
        {
            this->~GpuImageData();
            new (this) GpuImageData{std::move(other)};
        }
        return *this;
    }

    GpuImageData(const GpuImageData&) = delete;
    GpuImageData& operator=(const GpuImageData&) = delete;
};

struct GpuViewData {
    GpuImageData* image = nullptr;
    VkImageView view = nullptr;
    VkSampler sampler = nullptr;
    GpuDescriptor samplerDesc = {};
    GpuDescriptor storageDesc = {};
    GpuViewType type = GpuViewType_1D;
    GpuAspectFlags aspectFlags = 0;
    u8 baseMipLevel = 0;
    u8 levelCount = 0;
    u8 baseArrayLayer = 0;
    u8 layerCount = 0;
    GpuStageFlags lastStage = 0;
    GpuAccessFlags lastAccess = 0;
    GpuLayout lastLayout = GpuLayout_undefined;

    GpuViewData() = default;
    ~GpuViewData();

    GpuViewData(GpuViewData&& other) noexcept
        : image{other.image}
        , view{std::exchange(other.view, nullptr)}
        , sampler{other.sampler}
        , samplerDesc{std::exchange(other.samplerDesc, {})}
        , storageDesc{std::exchange(other.storageDesc, {})}
        , type{other.type}
        , aspectFlags{other.aspectFlags}
        , baseMipLevel{other.baseMipLevel}
        , levelCount{other.levelCount}
        , baseArrayLayer{other.baseArrayLayer}
        , layerCount{other.layerCount}
        , lastStage{other.lastStage}
        , lastAccess{other.lastAccess}
        , lastLayout{other.lastLayout}
    {}

    GpuViewData& operator=(GpuViewData&& other) noexcept
    {
        if (this != &other)
        {
            this->~GpuViewData();
            new (this) GpuViewData{std::move(other)};
        }
        return *this;
    }

    GpuViewData(const GpuViewData&) = delete;
    GpuViewData& operator=(const GpuViewData&) = delete;
};

struct GpuPipelineData {
    VkPipeline pipeline = nullptr;
    VkPipelineLayout layout = nullptr;
    VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    GpuPipelineData() = default;
    ~GpuPipelineData();

    GpuPipelineData(GpuPipelineData&& other) noexcept
        : pipeline{std::exchange(other.pipeline, nullptr)}
        , layout{std::exchange(other.layout, nullptr)}
        , bindPoint{other.bindPoint}
    {}

    GpuPipelineData& operator=(GpuPipelineData&& other) noexcept
    {
        if (this != &other)
        {
            this->~GpuPipelineData();
            new (this) GpuPipelineData{std::move(other)};
        }
        return *this;
    }

    GpuPipelineData(const GpuPipelineData&) = delete;
    GpuPipelineData& operator=(const GpuPipelineData&) = delete;
};

namespace internal {

struct SwapchainData {
    VkSurfaceKHR surface = nullptr;
    VkSwapchainKHR swapchain = nullptr;
    Array<GpuImage> images{};
    Array<GpuView> views{};
    Array<VkSemaphore> imageAvailable{};
    Array<VkSemaphore> readyToPresent{};
    u32 imageIdx = (u32)-1;
    u32 width = 0;
    u32 height = 0;
    Format format = Format_undefined;
    GpuImageUsageFlags imageUsage = {};
    GpuPresentMode presentMode = {};

    SwapchainData() = default;
    ~SwapchainData();

    SwapchainData(SwapchainData&& other) noexcept
        : surface{std::exchange(other.surface, nullptr)}
        , swapchain{std::exchange(other.swapchain, nullptr)}
        , images{std::exchange(other.images, {})}
        , views{std::exchange(other.views, {})}
        , imageAvailable{std::exchange(other.imageAvailable, {})}
        , readyToPresent{std::exchange(other.readyToPresent, {})}
        , imageIdx{std::exchange(other.imageIdx, (u32)-1)}
        , width{std::exchange(other.width, 0)}
        , height{std::exchange(other.height, 0)}
        , format{std::exchange(other.format, Format_undefined)}
        , imageUsage{std::exchange(other.imageUsage, {})}
        , presentMode{std::exchange(other.presentMode, {})}
    {}

    SwapchainData& operator=(SwapchainData&& other) noexcept
    {
        if (this != &other)
        {
            this->~SwapchainData();
            new (this) SwapchainData{std::move(other)};
        }
        return *this;
    }

    SwapchainData(const SwapchainData&) = delete;
    SwapchainData& operator=(const SwapchainData&) = delete;
};

} // namespace internal

namespace vulkan {

enum DescriptorType : u32 {
    DescriptorType_combinedImageSampler = 0,
    DescriptorType_storageImage = 1,
    DescriptorType_uniformBuffer = 2,
    DescriptorType_storageBuffer = 3,
    DescriptorType_count,
};

struct SamplerInfo {
    GpuFilter filter = GpuFilter_nearest;
    GpuSamplerEdgeMode mode = GpuSamplerEdgeMode_repeat;
    GpuSamplerBorder border = GpuSamplerBorder_floatTransparentBlack;
};

constexpr bool operator==(const SamplerInfo& lhs, const SamplerInfo& rhs)
{
    return lhs.filter == rhs.filter
        && lhs.mode == rhs.mode
        && lhs.border == rhs.border;
}

struct Frame {
    Array<internal::Swapchain*> swapchains = {};
    VkCommandPool cmdPool = nullptr;
    VkFence fence = nullptr;
};

struct VulkanState {
#ifdef HG_VK_DEBUG_MESSENGER
    VkDebugUtilsMessengerEXT debugMessenger = nullptr;
#endif
    VkInstance instance = nullptr;
    VkPhysicalDevice physicalDevice = nullptr;
    VkDevice device = nullptr;
    VmaAllocator vma = nullptr;
    VkQueue queue = nullptr;
    u32 queueFamily = (u32)-1;
    VkCommandPool cmdPool = nullptr;
    VkDescriptorPool bindlessPool = nullptr;
    VkDescriptorSetLayout bindlessLayout = nullptr;
    VkDescriptorSet bindlessSet = nullptr;
    HandlePool descriptorPools[DescriptorType_count];
    Map<SamplerInfo, VkSampler> samplers;
    Frame* frames = nullptr;
    u32 frameCount = 0;
    u32 currentFrame = 0;
};

extern VulkanState vk;

// -- inline conversion helpers --

inline VkFormat formatToVk(Format format)
{
    return static_cast<VkFormat>(format);
}

inline VkPipelineStageFlags gpuStageToVk(GpuStageFlags stage)
{
    return static_cast<VkPipelineStageFlags>(stage);
}

inline VkAccessFlags gpuAccessToVk(GpuAccessFlags access)
{
    return static_cast<VkAccessFlags>(access);
}

inline VkBufferUsageFlags gpuBufferUsageToVk(GpuBufferUsageFlags usage)
{
    return static_cast<VkBufferUsageFlags>(usage);
}

inline VkImageUsageFlags gpuImageUsageToVk(GpuImageUsageFlags usage)
{
    return static_cast<VkImageUsageFlags>(usage);
}

inline VkImageLayout gpuLayoutToVk(GpuLayout layout)
{
    return static_cast<VkImageLayout>(layout);
}

inline VkImageViewType gpuViewTypeToVk(GpuViewType type)
{
    return static_cast<VkImageViewType>(type);
}

inline VkImageAspectFlags gpuAspectToVk(GpuAspectFlags aspect)
{
    return static_cast<VkImageAspectFlags>(aspect);
}

inline VkFilter gpuFilterToVk(GpuFilter filter)
{
    return static_cast<VkFilter>(filter);
}

inline VkSamplerAddressMode gpuSamplerAddressModeToVk(GpuSamplerEdgeMode mode)
{
    return static_cast<VkSamplerAddressMode>(mode);
}

inline VkBorderColor gpuSamplerBorderToVk(GpuSamplerBorder color)
{
    return static_cast<VkBorderColor>(color);
}

inline VkPrimitiveTopology gpuTopologyToVk(GpuTopology topology)
{
    return static_cast<VkPrimitiveTopology>(topology);
}

inline VkPolygonMode gpuPolygonModeToVk(GpuPolygonMode mode)
{
    return static_cast<VkPolygonMode>(mode);
}

inline VkCullModeFlags gpuCullModeToVk(GpuCullFlags mode)
{
    return static_cast<VkCullModeFlags>(mode);
}

inline VkAttachmentLoadOp gpuLoadOpToVk(GpuLoadOp op)
{
    return static_cast<VkAttachmentLoadOp>(op);
}

inline VkAttachmentStoreOp gpuStoreOpToVk(GpuStoreOp op)
{
    return static_cast<VkAttachmentStoreOp>(op);
}

inline VkPresentModeKHR presentModeToVk(GpuPresentMode mode)
{
    return static_cast<VkPresentModeKHR>(mode);
}

inline VkDescriptorType descriptorTypeToVk(DescriptorType type)
{
    switch (type)
    {
        case DescriptorType_combinedImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DescriptorType_storageImage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case DescriptorType_uniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescriptorType_storageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        default:
            HG_PANIC("invalid GpuDescriptorType: %d", type);
    }
}

inline VmaAllocationCreateFlags gpuMemoryUsageToVma(GpuMemoryUsage usage)
{
    switch (usage)
    {
        case GpuMemoryUsage_deviceOnly:
            return 0;
        case GpuMemoryUsage_stagingWrite:
            return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        case GpuMemoryUsage_stagingRead:
            return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        case GpuMemoryUsage_frequentUpdate:
            return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                   VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        default:
            HG_PANIC("Invalid GpuMemoryUsage: %d\n", usage);
    }
}

inline GpuMemoryHostAccess gpuMemoryUsageToHostAccess(GpuMemoryUsage usage)
{
    switch (usage)
    {
        case GpuMemoryUsage_deviceOnly:
            return GpuMemoryHostAccess_none;
        case GpuMemoryUsage_stagingWrite:
            return GpuMemoryHostAccess_write;
        case GpuMemoryUsage_stagingRead:
            return GpuMemoryHostAccess_read;
        case GpuMemoryUsage_frequentUpdate:
            return GpuMemoryHostAccess_none;
        default:
            HG_PANIC("Invalid GpuMemoryUsage: %d\n", usage);
    }
}

inline VkImageType imageDimensionsToVkImage(u32 dimensions)
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
            HG_PANIC("Invalid image dimensions: %d\n", dimensions);
    }
}

inline VkImageCreateFlags gpuImageConfigFlagsToVk(GpuImageConfigFlags flags)
{
    return static_cast<VkImageCreateFlags>(flags);
}

inline VkSampleCountFlagBits countToMsaaSampleBits(u32 count)
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
            HG_PANIC("Invalid msaa sample count\n");
    }
}

inline GpuViewType imageDimensionsToHgView(u32 dimensions)
{
    switch (dimensions)
    {
        case 1:
            return GpuViewType_1D;
        case 2:
            return GpuViewType_2D;
        case 3:
            return GpuViewType_3D;
        default:
            HG_PANIC("Invalid image dimensions: %d\n", dimensions);
    }
}

const char* vkResultToStr(VkResult result);

GpuDescriptor createBufferDescriptor(
    DescriptorType type,
    const GpuBuffer& buffer,
    u64 offset,
    u64 range);

GpuDescriptor createImageDescriptor(
    DescriptorType type,
    const GpuView& imageView,
    GpuLayout imageLayout);

void descriptorDestroy(GpuDescriptor desc, DescriptorType type);

bool loadVulkan();
void unloadVulkan();
bool loadVulkanInstanceFuncs(VkInstance instance);
bool loadVulkanDeviceFuncs(VkDevice device);

VkSampler samplerGet(
    GpuFilter filter,
    GpuSamplerEdgeMode addressMode = GpuSamplerEdgeMode_repeat,
    GpuSamplerBorder borderColor = GpuSamplerBorder_floatTransparentBlack);

} // namespace vulkan

template<>
constexpr u64 hash(vulkan::SamplerInfo info)
{
    return info.border + (info.mode << 4) + (info.filter << 8);
}

} // namespace hg
