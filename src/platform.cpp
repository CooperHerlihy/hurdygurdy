#include "hurdygurdy.hpp"
#include "internal.hpp"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "SDL3/SDL.h"
#include <SDL3/SDL_vulkan.h>

#include "imgui_impl_vulkan.h"
#include "imgui_impl_sdl3.h"

namespace hg {

bool internal::initPlatform()
{
    if (!SDL_Init(
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_GAMEPAD |
        SDL_INIT_EVENTS))
    {
        setError(static_cast<StringView>(SDL_GetError()));
        return false;
    }
    return true;
}

void internal::deinitPlatform()
{
    SDL_Quit();
}

Span<StringView> internal::platformGetVulkanExtensions(Arena* arena)
{
    u32 extCount;
    const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&extCount);

    Span<StringView> extBuffer{arena->alloc<StringView>(extCount), extCount};
    for (u32 i = 0; i < extCount; ++i)
    {
        extBuffer[i] = exts[i];
    }

    return extBuffer;
}

static const char* vkResultToStr(VkResult result)
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

static u32 vkFormatToSize(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_UNDEFINED:
            return 0;

        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_A8_UNORM:
        case VK_FORMAT_R8_BOOL_ARM:
            return 1;

        case VK_FORMAT_R4G4_UNORM_PACK8: return 1;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
        case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
        case VK_FORMAT_A1B5G5R5_UNORM_PACK16:
            return 2;

        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            return 2;

        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
            return 2;

        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SRGB:
            return 3;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return 4;

        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            return 4;

        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
            return 4;

        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
            return 6;

        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 8;

        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;

        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;

        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_SFLOAT:
            return 8;

        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_SFLOAT:
            return 16;

        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
            return 24;

        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return 32;

        case VK_FORMAT_D16_UNORM: return 2;
        case VK_FORMAT_X8_D24_UNORM_PACK32: return 4;
        case VK_FORMAT_D32_SFLOAT: return 4;
        case VK_FORMAT_S8_UINT: return 1;
        case VK_FORMAT_D16_UNORM_S8_UINT: return 3;
        case VK_FORMAT_D24_UNORM_S8_UINT: return 4;
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return 5;

        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
            return 16;

        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            return 8;

        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            return 8;

        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
            return 16;

        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return 16;

        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
            return 8;
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
            return 8;

        case VK_FORMAT_G8B8G8R8_422_UNORM:
        case VK_FORMAT_B8G8R8G8_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        case VK_FORMAT_G16B16G16R16_422_UNORM:
        case VK_FORMAT_B16G16R16G16_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
        case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
        case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
            return 0;

        default:
            HG_WARN("Unrecognized Vulkan format\n");
            return 0;
    }
}

static VkFormat formatToVk(Format format)
{
    return static_cast<VkFormat>(format);
}

u32 formatToSize(Format format)
{
    return vkFormatToSize(formatToVk(format));
}

static VkPipelineStageFlags gpuStageToVk(GpuStageFlags stage)
{
    return static_cast<VkPipelineStageFlags>(stage);
}

static VkAccessFlags gpuAccessToVk(GpuAccessFlags access)
{
    return static_cast<VkAccessFlags>(access);
}

using GpuDescriptor = Handle;

enum DescriptorType : u32 {
    DescriptorType_combinedImageSampler = 0,
    DescriptorType_storageImage = 1,
    DescriptorType_uniformBuffer = 2,
    DescriptorType_storageBuffer = 3,
    DescriptorType_count,
};

static VkDescriptorType descriptorTypeToVk(DescriptorType type)
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
};

static VkBufferUsageFlags gpuBufferUsageToVk(GpuBufferUsageFlags usage)
{
    return static_cast<VkBufferUsageFlags>(usage);
}

static VmaAllocationCreateFlags gpuMemoryUsageToVma(GpuMemoryUsage usage)
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

static GpuMemoryHostAccess gpuMemoryUsageToHostAccess(GpuMemoryUsage usage)
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
};

static VkImageType imageDimensionsToVkImage(u32 dimensions)
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

static VkImageUsageFlags gpuImageUsageToVk(GpuImageUsageFlags usage)
{
    return static_cast<VkImageUsageFlags>(usage);
}

static VkImageLayout gpuLayoutToVk(GpuLayout layout)
{
    return static_cast<VkImageLayout>(layout);
}

static VkImageCreateFlags gpuImageConfigFlagsToVk(GpuImageConfigFlags flags)
{
    return static_cast<VkImageCreateFlags>(flags);
}

static VkSampleCountFlagBits countToMsaaSampleBits(u32 count)
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

struct SamplerInfo {
    GpuFilter filter = GpuFilter_nearest;
    GpuSamplerEdgeMode mode = GpuSamplerEdgeMode_repeat;
    GpuSamplerBorder border = GpuSamplerBorder_floatTransparentBlack;
};

template<>
constexpr u64 hash(SamplerInfo info)
{
    return info.border + (info.mode << 4) + (info.filter << 8);
}

constexpr bool operator==(const SamplerInfo& lhs, const SamplerInfo& rhs)
{
    return lhs.filter == rhs.filter
        && lhs.mode == rhs.mode
        && lhs.border == rhs.border;
}

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
};

static GpuViewType imageDimensionsToHgView(u32 dimensions)
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

static VkImageViewType gpuViewTypeToVk(GpuViewType type)
{
    return static_cast<VkImageViewType>(type);
}

static VkImageAspectFlags gpuAspectToVk(GpuAspectFlags aspect)
{
    return static_cast<VkImageAspectFlags>(aspect);
}

static VkFilter gpuFilterToVk(GpuFilter filter)
{
    return static_cast<VkFilter>(filter);
}

static VkSamplerAddressMode gpuSamplerAddressModeToVk(GpuSamplerEdgeMode mode)
{
    return static_cast<VkSamplerAddressMode>(mode);
}

static VkBorderColor gpuSamplerBorderToVk(GpuSamplerBorder color)
{
    return static_cast<VkBorderColor>(color);
}

struct GpuPipelineData {
    VkPipeline pipeline = nullptr;
    VkPipelineLayout layout = nullptr;
    VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

static VkPrimitiveTopology gpuTopologyToVk(GpuTopology topology)
{
    return static_cast<VkPrimitiveTopology>(topology);
}

static VkPolygonMode gpuPolygonModeToVk(GpuPolygonMode mode)
{
    return static_cast<VkPolygonMode>(mode);
}

static VkCullModeFlags gpuCullModeToVk(GpuCullFlags mode)
{
    return static_cast<VkCullModeFlags>(mode);
}

struct Frame {
    Array<WindowData*> windows = {};
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

static VulkanState vk{};

static bool loadVulkan();
static void unloadVulkan();

static bool loadVulkanInstanceFuncs(VkInstance instance);
static bool loadVulkanDeviceFuncs(VkDevice device);

#ifdef HG_VK_DEBUG_MESSENGER

static VkBool32 debugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    static_cast<void>(type);
    static_cast<void>(userData);

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Error: %s\n", callbackData->pMessage));
        abort();
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Warning: %s\n", callbackData->pMessage));
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Info: %s\n", callbackData->pMessage));
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Verbose: %s\n", callbackData->pMessage));
    } else {
        static_cast<void>(fprintf(stderr, "Vulkan Unknown: %s\n", callbackData->pMessage));
    }
    return VK_FALSE;
}

static const VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerInfo{
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    nullptr,
    0,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    debugCallback,
    nullptr,
};

#endif

static VkInstance createInstance(Span<StringView> extensions)
{
    if (extensions.count > 0)
        HG_ASSERT(extensions.data != nullptr);

    ArenaScope scratch = getScratch();

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hurdy Gurdy Application",
    appInfo.applicationVersion = 0;
    appInfo.pEngineName = "Hurdy Gurdy Engine";
    appInfo.engineVersion = 0;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef HG_VK_DEBUG_MESSENGER
    instanceInfo.pNext = &debugUtilsMessengerInfo;
#endif
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;

#ifdef HG_VK_DEBUG_MESSENGER
    const char* layers[]{
        "VK_LAYER_KHRONOS_validation",
    };
    instanceInfo.enabledLayerCount = static_cast<u32>(std::size(layers));
    instanceInfo.ppEnabledLayerNames = layers;
#endif

    const char** extCStrs = scratch.alloc<const char*>(extensions.count);
    for (u32 i = 0; i < extensions.count; ++i)
    {
        extCStrs[i] = cString(scratch, extensions[i]);
    }
    instanceInfo.enabledExtensionCount = (u32)extensions.count;
    instanceInfo.ppEnabledExtensionNames = extCStrs;

    VkInstance instance = nullptr;
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
    if (instance == nullptr)
    {
        setError("Failed to create Vulkan instance: %s", vkResultToStr(result));
    }

    return instance;
}

#ifdef HG_VK_DEBUG_MESSENGER
static VkDebugUtilsMessengerEXT createDebugUtilsMessenger()
{
    HG_ASSERT(vk.instance != nullptr);

    VkDebugUtilsMessengerEXT messenger = nullptr;
    VkResult result = vkCreateDebugUtilsMessengerEXT(vk.instance, &debugUtilsMessengerInfo, nullptr, &messenger);
    if (messenger == nullptr)
    {
        setError("Failed to create Vulkan debug messenger: %s", vkResultToStr(result));
    }

    return messenger;
}
#endif

static bool findQueueFamily(VkPhysicalDevice gpu, u32* queueFamily, VkQueueFlags queueFlags)
{
    HG_ASSERT(gpu != nullptr);
    HG_ASSERT(queueFamily != nullptr);

    ArenaScope scratch = getScratch();

    u32 familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
    VkQueueFamilyProperties* families = scratch.alloc<VkQueueFamilyProperties>(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families);

    for (u32 i = 0; i < familyCount; ++i)
    {
        if (families[i].queueFlags & queueFlags)
        {
            *queueFamily = i;
            return true;
        }
    }

    setError("Could not find Vulkan queue family");
    *queueFamily = (u32)-1;
    return false;
}

static const char* const deviceExtensions[]{
    "VK_KHR_swapchain",
};

static VkPhysicalDevice findPhysicalDevice()
{
    HG_ASSERT(vk.instance != nullptr);

    ArenaScope scratch = getScratch();

    u32 gpuCount;
    vkEnumeratePhysicalDevices(vk.instance, &gpuCount, nullptr);
    VkPhysicalDevice* gpus = scratch.alloc<VkPhysicalDevice>(gpuCount);
    vkEnumeratePhysicalDevices(vk.instance, &gpuCount, gpus);

    ArrayTemp<VkExtensionProperties> extProps{scratch, 0, 0};

    for (u32 i = 0; i < gpuCount; ++i)
    {
        VkPhysicalDevice gpu = gpus[i];
        u32 family;

        u32 propCount = 0;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &propCount, nullptr);
        extProps.resize(propCount);
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &propCount, extProps.vals);

        for (u32 j = 0; j < static_cast<u32>(std::size(deviceExtensions)); j++)
        {
            for (u32 k = 0; k < propCount; k++)
            {
                if (strcmp(deviceExtensions[j], extProps[k].extensionName) == 0)
                    goto nextExt;
            }
            goto nextGpu;
nextExt:
            continue;
        }

        if (!findQueueFamily(gpu, &family,
                VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT))
            goto nextGpu;

        return gpu;

nextGpu:
        continue;
    }

    setError("Could not find suitable gpu");
    return nullptr;
}

static VkDevice createDevice()
{
    HG_ASSERT(vk.physicalDevice != nullptr);
    HG_ASSERT(vk.queueFamily != (u32)-1);

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeature{};
    descriptorIndexingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeature.pNext = nullptr;
    descriptorIndexingFeature.shaderInputAttachmentArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingPartiallyBound = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptorIndexingFeature.runtimeDescriptorArray = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{};
    dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature.pNext = &descriptorIndexingFeature;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features synchronization2Feature{};
    synchronization2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2Feature.pNext = &dynamicRenderingFeature;
    synchronization2Feature.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures features{};

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = vk.queueFamily;
    queueInfo.queueCount = 1;
    f32 queuePriority = 1.0f;
    queueInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = &synchronization2Feature;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = static_cast<u32>(std::size(deviceExtensions));
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceInfo.pEnabledFeatures = &features;

    VkDevice device = nullptr;
    VkResult result = vkCreateDevice(vk.physicalDevice, &deviceInfo, nullptr, &device);
    if (device == nullptr)
    {
        setError("Could not create VkDevice: %s", vkResultToStr(result));
    }

    return device;
}

static VmaAllocator createVma()
{
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = vk.physicalDevice;
    allocatorInfo.device = vk.device;
    allocatorInfo.instance = vk.instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator vma = nullptr;
    VkResult result = vmaCreateAllocator(&allocatorInfo, &vma);
    if (vma == nullptr)
    {
        setError("Could not create Vulkan memory allocator: %s", vkResultToStr(result));
    }

    return vma;
}

static VkDescriptorPool createBindlessDescriptorPool()
{
    VkDescriptorPoolSize sizes[]{
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, UINT16_MAX},
    };

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    info.maxSets = 1;
    info.poolSizeCount = static_cast<u32>(std::size(sizes));
    info.pPoolSizes = sizes;

    VkDescriptorPool pool = nullptr;
    [[maybe_unused]] VkResult result = vkCreateDescriptorPool(vk.device, &info, nullptr, &pool);
    if (pool == nullptr)
        HG_PANIC("Could not create VkDescriptorPool: %s\n", vkResultToStr(result));

    return pool;
}

static VkDescriptorSetLayout createBindlessDescriptorLayout()
{
    VkDescriptorSetLayoutBinding bindings[DescriptorType_count]{};
    VkDescriptorBindingFlags flags[DescriptorType_count]{};
    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = descriptorTypeToVk(static_cast<DescriptorType>(i));
        bindings[i].descriptorCount = UINT16_MAX;
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
    flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagsInfo.bindingCount = static_cast<u32>(std::size(bindings));
    flagsInfo.pBindingFlags = flags;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = &flagsInfo;
    info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    info.bindingCount = static_cast<u32>(std::size(bindings));
    info.pBindings = bindings;

    VkDescriptorSetLayout layout = nullptr;
    [[maybe_unused]] VkResult result = vkCreateDescriptorSetLayout(vk.device, &info, nullptr, &layout);
    if (layout == nullptr)
        HG_PANIC("Could not create bindless VkDescriptorSetLayout: %s\n", vkResultToStr(result));

    return layout;
}

static Frame createFrame()
{
    Frame frame{};

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = vk.queueFamily;

    [[maybe_unused]]
    VkResult poolResult = vkCreateCommandPool(vk.device, &poolInfo, nullptr, &frame.cmdPool);
    if (frame.cmdPool == nullptr)
        HG_PANIC("Could not create Vulkan command pool: %s\n", vkResultToStr(poolResult));

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    [[maybe_unused]]
    VkResult fenceResult = vkCreateFence(vk.device, &fenceInfo, nullptr, &frame.fence);
    if (frame.fence == nullptr)
        HG_PANIC("Could not create Vulkan fence: %s\n", vkResultToStr(fenceResult));

    return frame;
}

bool internal::initGpu()
{
    ArenaScope scratch = getScratch();

    if (!loadVulkan())
        goto loadFailed;

    {
        Span<StringView> exts = internal::platformGetVulkanExtensions(scratch);
#ifdef HG_VK_DEBUG_MESSENGER
        scratch.extend(exts.data, exts.count, exts.count + 1);
        ++exts.count;
        exts[exts.count - 1] = "VK_EXT_debug_utils";
#endif
        vk.instance = createInstance(exts);
        if (vk.instance == nullptr)
            goto instanceFailed;
    }
    if (!loadVulkanInstanceFuncs(vk.instance))
        goto loadInstanceFailed;

#ifdef HG_VK_DEBUG_MESSENGER
    vk.debugMessenger = createDebugUtilsMessenger();
    if (vk.debugMessenger == nullptr)
        goto debugMessengerFailed;
#endif

    vk.physicalDevice = findPhysicalDevice();
    if (vk.physicalDevice == nullptr)
        goto physicalDeviceFailed;

    findQueueFamily(vk.physicalDevice, &vk.queueFamily,
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT);
    if (vk.queueFamily == (u32)-1)
        goto queueFamilyFailed;

    vk.device = createDevice();
    if (vk.device == nullptr)
        goto deviceFailed;

    if (!loadVulkanDeviceFuncs(vk.device))
        goto loadDeviceFailed;

    vkGetDeviceQueue(vk.device, vk.queueFamily, 0, &vk.queue);
    if (vk.queue == nullptr)
        goto queueFailed;

    vk.vma = createVma();
    if (vk.vma == nullptr)
        goto vmaFailed;

    {
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolInfo.queueFamilyIndex = vk.queueFamily;

        [[maybe_unused]]         VkResult result = vkCreateCommandPool(vk.device, &cmdPoolInfo, nullptr, &vk.cmdPool);
        if (vk.cmdPool == nullptr)
            HG_PANIC("Could not create Vulkan command pool: %s\n", vkResultToStr(result));
    }

    vk.bindlessPool = createBindlessDescriptorPool();
    vk.bindlessLayout = createBindlessDescriptorLayout();
    {
        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = vk.bindlessPool;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &vk.bindlessLayout;

        [[maybe_unused]]         VkResult result = vkAllocateDescriptorSets(vk.device, &info, &vk.bindlessSet);
        if (vk.bindlessSet == nullptr)
            HG_PANIC("Could not allocate bindless VkDescriptorSet: %s\n", vkResultToStr(result));
    }

    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        vk.descriptorPools[i] = handlePoolCreate();
    }

    vk.samplers = Map<SamplerInfo, VkSampler>(
        2 *
        GpuFilter_count *
        GpuSamplerEdgeMode_count *
        GpuSamplerBorder_count);

    vk.frameCount = 2;
    vk.currentFrame = 0;
    vk.frames = heapAlloc<Frame>(vk.frameCount);
    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        new (vk.frames + i) Frame{createFrame()};
    }

    return true;

vmaFailed:
queueFailed:
    vkDestroyDevice(vk.device, nullptr);
loadDeviceFailed:
deviceFailed:
queueFamilyFailed:
physicalDeviceFailed:
#ifdef HG_VK_DEBUG_MESSENGER
    vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debugMessenger, nullptr);
debugMessengerFailed:
#endif
loadInstanceFailed:
    vkDestroyInstance(vk.instance, nullptr);
instanceFailed:
    unloadVulkan();
loadFailed:
    return false;
}

void internal::deinitGpu()
{
    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        vkDestroyFence(vk.device, vk.frames[i].fence, nullptr);
        vkDestroyCommandPool(vk.device, vk.frames[i].cmdPool, nullptr);
        vk.frames[i].windows = {};
    }
    heapFree(vk.frames, vk.frameCount);

    vk.samplers.forEach([](SamplerInfo*, VkSampler* sampler)
    {
        vkDestroySampler(vk.device, *sampler, nullptr);
    });
    vk.samplers = {};

    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        handlePoolDestroy(&vk.descriptorPools[i]);
    }

    vkDestroyDescriptorSetLayout(vk.device, vk.bindlessLayout, nullptr);
    vkDestroyDescriptorPool(vk.device, vk.bindlessPool, nullptr);

    vkDestroyCommandPool(vk.device, vk.cmdPool, nullptr);

    vmaDestroyAllocator(vk.vma);

    vkDestroyDevice(vk.device, nullptr);

#ifdef HG_VK_DEBUG_MESSENGER
    vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debugMessenger, nullptr);
#endif

    vkDestroyInstance(vk.instance, nullptr);

    unloadVulkan();
}

void gpuWaitIdle()
{
    vkQueueWaitIdle(vk.queue);
}

// static u32 findMemoryTypeIndex(
//     u32 bitmask,
//     VkMemoryPropertyFlags preferredFlags,
//     VkMemoryPropertyFlags unpreferredFlags)
// {
//     HG_ASSERT(vkState.vkPhysicalDevice != nullptr);
//     HG_ASSERT(bitmask != 0);
//
//     VkPhysicalDeviceMemoryProperties memProps;
//     vkGetPhysicalDeviceMemoryProperties(vkState.vkPhysicalDevice, &memProps);
//
//     for (u32 i = 0; i < memProps.memoryTypeCount; ++i)
//     {
//         if ((bitmask & (1 << i)) != 0 &&
//             (memProps.memoryTypes[i].propertyFlags & preferredFlags) != 0 &&
//             (memProps.memoryTypes[i].propertyFlags & unpreferredFlags) == 0)
//         {
//             return i;
//         }
//     }
//     for (u32 i = 0; i < memProps.memoryTypeCount; ++i)
//     {
//         if ((bitmask & (1 << i)) != 0 && (memProps.memoryTypes[i].propertyFlags & preferredFlags) != 0)
//         {
//             HG_WARN("Could not find Vulkan memory type without undesired flags\n");
//             return i;
//         }
//     }
//     for (u32 i = 0; i < memProps.memoryTypeCount; ++i)
//     {
//         if ((bitmask & (1 << i)) != 0)
//         {
//             HG_WARN("Could not find Vulkan memory type with desired flags\n");
//             return i;
//         }
//     }
//     HG_PANIC("Could not find Vulkan memory type in bitmask: %x\n", bitmask);
// }

static GpuDescriptor createBufferDescriptor(
    DescriptorType type,
    const GpuBuffer& buffer,
    u64 offset,
    u64 range)
{
    HG_ASSERT(type < DescriptorType_count);

    ArenaScope scratch = getScratch();

    GpuDescriptor desc = handlePoolAlloc(&vk.descriptorPools[type]);

    VkDescriptorBufferInfo bufferInfo{buffer.data->buffer, offset, range};

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vk.bindlessSet;
    write.dstBinding = type;
    write.dstArrayElement = handleIdx(desc);
    write.descriptorCount = 1;
    write.descriptorType = descriptorTypeToVk(type);
    write.pBufferInfo = &bufferInfo;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk.device, 1, &write, 0, nullptr);

    return desc;
}

static GpuDescriptor createImageDescriptor(
    DescriptorType type,
    const GpuView& imageView,
    GpuLayout imageLayout)
{
    HG_ASSERT(type < DescriptorType_count);

    ArenaScope scratch = getScratch();

    GpuDescriptor desc = handlePoolAlloc(&vk.descriptorPools[type]);

    VkDescriptorImageInfo imageInfo{
        imageView.data->sampler,
        imageView.data->view,
        gpuLayoutToVk(imageLayout)
    };

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vk.bindlessSet;
    write.dstBinding = type;
    write.dstArrayElement = handleIdx(desc);
    write.descriptorCount = 1;
    write.descriptorType = descriptorTypeToVk(type);
    write.pBufferInfo = nullptr;
    write.pImageInfo = &imageInfo;
    write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk.device, 1, &write, 0, nullptr);

    return desc;
}

static void descriptorDestroy(GpuDescriptor desc, DescriptorType type)
{
    if (desc != handleNull)
        handlePoolFree(&vk.descriptorPools[type], desc);
}

GpuBuffer::GpuBuffer(u64 size, GpuBufferUsageFlags usageFlags, GpuMemoryUsage memoryUsage)
    : data{makeUnique<GpuBufferData>()}
{
    HG_ASSERT(size > 0);
    HG_ASSERT(usageFlags != 0);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = gpuBufferUsageToVk(usageFlags);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = gpuMemoryUsageToVma(memoryUsage);

    VkResult result = vmaCreateBuffer(
        vk.vma,
        &bufferInfo,
        &allocInfo,
        &data->buffer,
        &data->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        HG_PANIC("Could not create VkBuffer: %s\n", vkResultToStr(result));

    data->size = size;

    if (usageFlags & GpuBufferUsage_uniformBuffer)
        data->uniformDesc = createBufferDescriptor(DescriptorType_uniformBuffer, *this, 0, size);

    if (usageFlags & GpuBufferUsage_storageBuffer)
        data->storageDesc = createBufferDescriptor(DescriptorType_storageBuffer, *this, 0, size);

    data->usage = usageFlags;

    if (memoryUsage == GpuMemoryUsage_frequentUpdate)
    {
        VkMemoryPropertyFlags memPropFlags;
        vmaGetAllocationMemoryProperties(vk.vma, data->alloc, &memPropFlags);
        data->access = memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            ? GpuMemoryHostAccess_write
            : GpuMemoryHostAccess_none;
    } else {
        data->access = gpuMemoryUsageToHostAccess(memoryUsage);
    }
}

GpuBuffer::~GpuBuffer() noexcept
{
    if (data != nullptr)
    {
        descriptorDestroy(data->storageDesc, DescriptorType_storageBuffer);
        descriptorDestroy(data->uniformDesc, DescriptorType_uniformBuffer);
        vmaDestroyBuffer(vk.vma, data->buffer, data->alloc);
    }
}

GpuBuffer::GpuBuffer() noexcept = default;
GpuBuffer::GpuBuffer(GpuBuffer&&) noexcept = default;
GpuBuffer& GpuBuffer::operator=(GpuBuffer&&) noexcept = default;

u32 GpuBuffer::uniformDescriptor()
{
    HG_ASSERT(data->usage & GpuBufferUsage_uniformBuffer);
    GpuDescriptor desc = data->uniformDesc;
    HG_ASSERT(handlePoolAlive(&vk.descriptorPools[DescriptorType_uniformBuffer], desc));
    return handleIdx(desc);
}

u32 GpuBuffer::storageDescriptor()
{
    HG_ASSERT(data->usage & GpuBufferUsage_storageBuffer);
    GpuDescriptor desc = data->storageDesc;
    HG_ASSERT(handlePoolAlive(&vk.descriptorPools[DescriptorType_storageBuffer], desc));
    return handleIdx(desc);
}

void GpuBuffer::write(const void* src, u64 offset, u64 size)
{
    if (size == 0)
        return;

    HG_ASSERT(src != nullptr);

    if (data->access & GpuMemoryHostAccess_write)
    {
        VkResult result = vmaCopyMemoryToAllocation(vk.vma, src, data->alloc, offset, size);
        if (result != VK_SUCCESS)
            HG_PANIC("Could not write gpu buffer: %s\n", vkResultToStr(result));
        return;
    }

    GpuBuffer stage{size, GpuBufferUsage_transferSrc, GpuMemoryUsage_stagingWrite};
    stage.write(src, 0, size);

    GpuCmd* cmd = gpuCmdBegin();

    VkBufferCopy region{};
    region.dstOffset = offset;
    region.size = size;

    vkCmdCopyBuffer(reinterpret_cast<VkCommandBuffer>(cmd), stage.data->buffer, data->buffer, 1, &region);

    gpuCmdEnd(cmd);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferWrite;
}

void GpuBuffer::read(void* dst, u64 offset, u64 size)
{
    if (size == 0)
        return;

    HG_ASSERT(dst != nullptr);

    if (data->access & GpuMemoryHostAccess_read)
    {
        VkResult result = vmaCopyAllocationToMemory(vk.vma, data->alloc, offset, dst, size);
        if (result != VK_SUCCESS)
            HG_PANIC("Could not read gpu buffer: %s\n", vkResultToStr(result));
        return;
    }

    GpuBuffer stage{size, GpuBufferUsage_transferDst, GpuMemoryUsage_stagingRead};

    GpuCmd* cmd = gpuCmdBegin();

    VkBufferCopy region{};
    region.srcOffset = offset;
    region.size = size;

    vkCmdCopyBuffer(reinterpret_cast<VkCommandBuffer>(cmd), data->buffer, stage.data->buffer, 1, &region);

    gpuCmdEnd(cmd);

    stage.read(dst, 0, size);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferRead;
}

GpuImage::GpuImage(u32 width, u32 height, Format format, GpuImageUsageFlags usage)
{
    GpuImageCreateInfo create{};
    create.width = width;
    create.height = height;
    create.format = format;
    create.usage = usage;
    data = GpuImage{create}.data;
}

GpuImage::GpuImage(const GpuImageCreateInfo& create)
    : data{makeUnique<GpuImageData>()}
{
    HG_ASSERT(create.format != Format_undefined);
    HG_ASSERT(create.usage != 0);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = gpuImageConfigFlagsToVk(create.flags);
    imageInfo.imageType = imageDimensionsToVkImage(create.dimensions);
    imageInfo.format = formatToVk(create.format);
    imageInfo.extent = {create.width, create.height, create.depth};
    imageInfo.mipLevels = create.mipLevels;
    imageInfo.arrayLayers = create.arrayLayers;
    imageInfo.samples = countToMsaaSampleBits(create.msaaSamples);
    imageInfo.usage = gpuImageUsageToVk(create.usage);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult result = vmaCreateImage(
        vk.vma,
        &imageInfo,
        &allocInfo,
        &data->image,
        &data->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        HG_PANIC("Could not create VkImage: %s\n", vkResultToStr(result));

    data->usage = create.usage;
    data->format = create.format;
    data->width = create.width;
    data->height = create.height;
    data->depth = create.depth;
    data->dimensions = static_cast<u8>(create.dimensions);
    data->mipLevels = static_cast<u8>(create.mipLevels);
    data->arrayLayers = static_cast<u8>(create.arrayLayers);
    data->msaaSamples = static_cast<u8>(countToMsaaSampleBits(create.msaaSamples));
}

GpuImage::~GpuImage()
{
    if (data != nullptr)
    {
        vmaDestroyImage(vk.vma, data->image, data->alloc);
    }
}

GpuImage::GpuImage() noexcept = default;
GpuImage::GpuImage(GpuImage&&) noexcept = default;
GpuImage& GpuImage::operator=(GpuImage&&) noexcept = default;

u32 GpuImage::width() const
{
    return data->width;
}

u32 GpuImage::height() const
{
    return data->height;
}

static VkSampler samplerCreate(SamplerInfo* desc)
{
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = gpuFilterToVk(desc->filter);
    info.minFilter = gpuFilterToVk(desc->filter);
    info.mipmapMode = desc->filter == GpuFilter_linear
        ? VK_SAMPLER_MIPMAP_MODE_LINEAR
        : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = gpuSamplerAddressModeToVk(desc->mode);
    info.addressModeV = gpuSamplerAddressModeToVk(desc->mode);
    info.addressModeW = gpuSamplerAddressModeToVk(desc->mode);
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 1000.0f;
    info.borderColor = gpuSamplerBorderToVk(desc->border);

    VkSampler sampler = nullptr;
    [[maybe_unused]]     VkResult result = vkCreateSampler(vk.device, &info, nullptr, &sampler);
    if (sampler == nullptr)
        HG_PANIC("Could not create VkSampler: %s\n", vkResultToStr(result));

    return sampler;
}

static VkSampler samplerGet(
    GpuFilter filter,
    GpuSamplerEdgeMode addressMode = GpuSamplerEdgeMode_repeat,
    GpuSamplerBorder borderColor = GpuSamplerBorder_floatTransparentBlack)
{
    SamplerInfo desc = {filter, addressMode, borderColor};
    VkSampler* sampler = vk.samplers.get(desc);
    if (sampler == nullptr)
    {
        sampler = vk.samplers.add(desc, samplerCreate(&desc));
    }
    return *sampler;
}

GpuView::GpuView(
    GpuImage& image,
    GpuAspectFlags aspectFlags,
    GpuFilter filter)
{
    GpuViewCreateInfo config{};
    config.image = &image;
    config.baseMipLevel = 0;
    config.levelCount = image.data->mipLevels;
    config.baseArrayLayer = 0;
    config.layerCount = image.data->arrayLayers;
    config.aspectFlags = aspectFlags;
    config.type = imageDimensionsToHgView(image.data->dimensions);
    config.filter = filter;
    config.edgeMode = GpuSamplerEdgeMode_repeat;
    config.border = GpuSamplerBorder_floatTransparentBlack;
    new (this) GpuView{config};
}

GpuView::GpuView(const GpuViewCreateInfo& config)
    : data{makeUnique<GpuViewData>()}
{
    HG_ASSERT(config.aspectFlags != 0);

    GpuImageData* image = config.image->data;

    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image->image;
    info.viewType = gpuViewTypeToVk(config.type);
    info.format = formatToVk(image->format);
    info.subresourceRange.aspectMask = gpuAspectToVk(config.aspectFlags);
    info.subresourceRange.baseMipLevel = config.baseMipLevel;
    info.subresourceRange.levelCount = config.levelCount;
    info.subresourceRange.baseArrayLayer = config.baseArrayLayer;
    info.subresourceRange.layerCount = config.layerCount;

    [[maybe_unused]]
    VkResult result = vkCreateImageView(vk.device, &info, nullptr, &data->view);
    if (data->view == nullptr)
        HG_PANIC("Could not create VkImageView: %s\n", vkResultToStr(result));

    if (image->usage & GpuImageUsage_sampled)
    {
        data->sampler = samplerGet(config.filter, config.edgeMode, config.border);
        data->samplerDesc = createImageDescriptor(
            DescriptorType_combinedImageSampler,
            *this,
            GpuLayout_shaderReadOnly);
    }

    if (image->usage & GpuImageUsage_storage)
    {
        data->storageDesc = createImageDescriptor(
            DescriptorType_storageImage,
            *this,
            GpuLayout_general);
    }

    data->image = image;
    data->type = config.type;
    data->aspectFlags = config.aspectFlags;
    data->baseMipLevel = static_cast<u8>(config.baseMipLevel);
    data->levelCount = static_cast<u8>(config.levelCount);
    data->baseArrayLayer = static_cast<u8>(config.baseArrayLayer);
    data->layerCount = static_cast<u8>(config.layerCount);
}

GpuView::~GpuView() noexcept
{
    if (data != nullptr)
    {
        descriptorDestroy(data->storageDesc, DescriptorType_storageImage);
        descriptorDestroy(data->samplerDesc, DescriptorType_combinedImageSampler);
        vkDestroyImageView(vk.device, data->view, nullptr);
    }
}

GpuView::GpuView() noexcept = default;
GpuView::GpuView(GpuView&&) noexcept = default;
GpuView& GpuView::operator=(GpuView&&) noexcept = default;

u32 GpuView::width() const
{
    return data->image->width;
}

u32 GpuView::height() const
{
    return data->image->height;
}

u32 GpuView::samplerDescriptor()
{
    HG_ASSERT(data->image->usage & GpuImageUsage_sampled);
    GpuDescriptor desc = data->samplerDesc;
    HG_ASSERT(handlePoolAlive(&vk.descriptorPools[DescriptorType_combinedImageSampler], desc));
    return handleIdx(desc);
}

u32 GpuView::storageDescriptor()
{
    HG_ASSERT(data->image->usage & GpuImageUsage_storage);
    GpuDescriptor desc = data->storageDesc;
    HG_ASSERT(handlePoolAlive(&vk.descriptorPools[DescriptorType_storageImage], desc));
    return handleIdx(desc);
}

void GpuView::write(const void* src)
{
    HG_ASSERT(src != nullptr);

    u64 size = data->image->width
             * data->image->height
             * data->image->depth
             * formatToSize(data->image->format);

    GpuBuffer stage{size, GpuBufferUsage_transferSrc, GpuMemoryUsage_stagingWrite};
    stage.write(src, 0, size);

    GpuCmd* cmd = gpuCmdBegin();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarrier.image = data->image->image;
    transferBarrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    transferBarrier.subresourceRange.baseMipLevel = data->baseMipLevel;
    transferBarrier.subresourceRange.levelCount = data->levelCount;
    transferBarrier.subresourceRange.baseArrayLayer = data->baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = data->layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = gpuAspectToVk(data->aspectFlags);
    region.imageSubresource.mipLevel = data->baseMipLevel;
    region.imageSubresource.baseArrayLayer = data->baseArrayLayer;
    region.imageSubresource.layerCount = data->layerCount;
    region.imageExtent = {data->image->width, data->image->height, data->image->depth};

    vkCmdCopyBufferToImage(
        reinterpret_cast<VkCommandBuffer>(cmd),
        stage.data->buffer,
        data->image->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    gpuCmdEnd(cmd);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferWrite;
    data->lastLayout = GpuLayout_transferDst;
}

void GpuView::writeCubemap(const void* src)
{
    HG_ASSERT(data->image->depth == 1);
    HG_ASSERT(data->baseArrayLayer == 0);
    HG_ASSERT(data->layerCount >= 6);
    HG_ASSERT(src != nullptr);

    u64 size = data->image->width * data->image->height * formatToSize(data->image->format);

    GpuBuffer buffer{size * 4 * 3, GpuBufferUsage_transferSrc, GpuMemoryUsage_stagingWrite};
    buffer.write(src, 0, size * 4 * 3);

    GpuImage stage{
        data->image->width * 4,
        data->image->height * 3,
        data->image->format,
        GpuImageUsage_transferDst | GpuImageUsage_transferSrc};

    GpuCmd* cmd = gpuCmdBegin();

    VkImageMemoryBarrier2 stageBarrier{};
    stageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    stageBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    stageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    stageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    stageBarrier.image = stage.data->image;
    stageBarrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    stageBarrier.subresourceRange.baseMipLevel = 0;
    stageBarrier.subresourceRange.levelCount = 1;
    stageBarrier.subresourceRange.baseArrayLayer = 0;
    stageBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo stageDep{};
    stageDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    stageDep.imageMemoryBarrierCount = 1;
    stageDep.pImageMemoryBarriers = &stageBarrier;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &stageDep);

    VkBufferImageCopy stageRegion{};
    stageRegion.imageSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    stageRegion.imageExtent = {data->image->width * 4, data->image->height * 3, 1};

    vkCmdCopyBufferToImage(
        reinterpret_cast<VkCommandBuffer>(cmd),
        buffer.data->buffer,
        stage.data->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &stageRegion);

    VkImageMemoryBarrier2 transferBarriers[2]{};

    transferBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[0].srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[0].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarriers[0].image = stage.data->image;
    transferBarriers[0].subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    transferBarriers[0].subresourceRange.baseMipLevel = 0;
    transferBarriers[0].subresourceRange.levelCount = 1;
    transferBarriers[0].subresourceRange.baseArrayLayer = 0;
    transferBarriers[0].subresourceRange.layerCount = 1;

    transferBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[1].image = data->image->image;
    transferBarriers[1].subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    transferBarriers[1].subresourceRange.baseMipLevel = data->baseMipLevel;
    transferBarriers[1].subresourceRange.levelCount = data->levelCount;
    transferBarriers[1].subresourceRange.baseArrayLayer = data->baseArrayLayer;
    transferBarriers[1].subresourceRange.layerCount = data->layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = static_cast<u32>(std::size(transferBarriers));
    transferDep.pImageMemoryBarriers = transferBarriers;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &transferDep);

    VkImageCopy regions[6]{};

    regions[0].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[0].srcOffset = {static_cast<int>(data->image->width) * 2, static_cast<int>(data->image->height) * 1, 0};
    regions[0].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 0, 1};
    regions[0].dstOffset = {};
    regions[0].extent = {data->image->width, data->image->height, 1};

    regions[1].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[1].srcOffset = {static_cast<int>(data->image->width) * 0, static_cast<int>(data->image->height) * 1, 0};
    regions[1].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 1, 1};
    regions[1].dstOffset = {};
    regions[1].extent = {data->image->width, data->image->height, 1};

    regions[2].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[2].srcOffset = {static_cast<int>(data->image->width) * 1, static_cast<int>(data->image->height) * 2, 0};
    regions[2].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 2, 1};
    regions[2].dstOffset = {};
    regions[2].extent = {data->image->width, data->image->height, 1};

    regions[3].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[3].srcOffset = {static_cast<int>(data->image->width) * 1, static_cast<int>(data->image->height) * 0, 0};
    regions[3].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 3, 1};
    regions[3].dstOffset = {};
    regions[3].extent = {data->image->width, data->image->height, 1};

    regions[4].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[4].srcOffset = {static_cast<int>(data->image->width) * 1, static_cast<int>(data->image->height) * 1, 0};
    regions[4].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 4, 1};
    regions[4].dstOffset = {};
    regions[4].extent = {data->image->width, data->image->height, 1};

    regions[5].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[5].srcOffset = {static_cast<int>(data->image->width) * 3, static_cast<int>(data->image->height) * 1, 0};
    regions[5].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 5, 1};
    regions[5].dstOffset = {};
    regions[5].extent = {data->image->width, data->image->height, 1};

    vkCmdCopyImage(
        reinterpret_cast<VkCommandBuffer>(cmd),
        stage.data->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        data->image->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<u32>(std::size(regions)),
        regions);

    gpuCmdEnd(cmd);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferWrite;
    data->lastLayout = GpuLayout_transferDst;
}

void GpuView::read(void* dst)
{
    HG_ASSERT(data->lastLayout != GpuLayout_undefined);
    HG_ASSERT(data != nullptr);

    u32 mipW = data->image->width >> data->baseMipLevel;
    u32 mipH = data->image->height >> data->baseMipLevel;
    u32 mipD = data->image->depth >> data->baseMipLevel;
    if (mipW < 1) mipW = 1;
    if (mipH < 1) mipH = 1;
    if (mipD < 1) mipD = 1;

    u64 size = mipW * mipH * mipD * formatToSize(data->image->format);

    GpuBuffer stage{size, GpuBufferUsage_transferDst, GpuMemoryUsage_stagingRead};

    GpuCmd* cmd = gpuCmdBegin();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.srcStageMask = gpuStageToVk(data->lastStage);
    transferBarrier.srcAccessMask = gpuAccessToVk(data->lastAccess);
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarrier.oldLayout = gpuLayoutToVk(data->lastLayout);
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarrier.image = data->image->image;
    transferBarrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    transferBarrier.subresourceRange.baseMipLevel = data->baseMipLevel;
    transferBarrier.subresourceRange.levelCount = data->levelCount;
    transferBarrier.subresourceRange.baseArrayLayer = data->baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = data->layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = gpuAspectToVk(data->aspectFlags);
    region.imageSubresource.mipLevel = data->baseMipLevel;
    region.imageSubresource.baseArrayLayer = data->baseArrayLayer;
    region.imageSubresource.layerCount = data->layerCount;
    region.imageExtent = {mipW, mipH, mipD};

    vkCmdCopyImageToBuffer(
        reinterpret_cast<VkCommandBuffer>(cmd),
        data->image->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        stage.data->buffer,
        1,
        &region);

    gpuCmdEnd(cmd);

    stage.read(dst, 0, size);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferRead;
    data->lastLayout = GpuLayout_transferSrc;
}

void GpuView::genMipmaps()
{
    HG_ASSERT(data->lastLayout != GpuLayout_undefined);
    if (data->levelCount == 1)
        return;

    GpuCmd* cmd = gpuCmdBegin();

    VkOffset3D mipOffset{};
    mipOffset.x = static_cast<i32>(data->image->width);
    mipOffset.y = static_cast<i32>(data->image->height);
    mipOffset.z = static_cast<i32>(data->image->depth);

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = gpuLayoutToVk(data->lastLayout);
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = data->image->image;
    barrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);

    for (u32 level = 0; level < static_cast<u32>(data->levelCount) - 1; ++level)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = gpuAspectToVk(data->aspectFlags);
        blit.srcSubresource.mipLevel = level;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[1] = mipOffset;
        if (mipOffset.x > 1)
            mipOffset.x /= 2;
        if (mipOffset.y > 1)
            mipOffset.y /= 2;
        if (mipOffset.z > 1)
            mipOffset.z /= 2;
        blit.dstSubresource.aspectMask = gpuAspectToVk(data->aspectFlags);
        blit.dstSubresource.mipLevel = level + 1;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[1] = mipOffset;

        vkCmdBlitImage(
            reinterpret_cast<VkCommandBuffer>(cmd),
            data->image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            data->image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);
    }

    gpuCmdEnd(cmd);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferRead;
    data->lastLayout = GpuLayout_transferSrc;
}

u32 getMaxMipmaps(u32 width, u32 height, u32 depth)
{
    u32 max = width > height ? width : height;
    max = max > depth ? max : depth;
    return max == 0 ? 0 : static_cast<u32>(log2(static_cast<f32>(max))) + 1;
}

static VkShaderModule createShaderModule(const void* spirvCode, u64 codeSize)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = codeSize;
    info.pCode = (const u32*)spirvCode;

    VkShaderModule shader = nullptr;
    [[maybe_unused]]     VkResult result = vkCreateShaderModule(vk.device, &info, nullptr, &shader);
    if (shader == nullptr)
        HG_PANIC("Could not create VkShaderModule: %s\n", vkResultToStr(result));

    return shader;
}

GpuPipeline::GpuPipeline(const GpuGraphicsPipelineCreateInfo& config)
    : data{makeUnique<GpuPipelineData>()}
{
    HG_ASSERT(config.vertexShader.data != nullptr);
    HG_ASSERT(config.fragmentShader.data != nullptr);
    if (config.colorAttachmentFormats.count > 0)
        HG_ASSERT(config.colorAttachmentFormats.data != nullptr);

    ArenaScope scratch = getScratch();

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &vk.bindlessLayout;
    VkPushConstantRange pushRange{VK_SHADER_STAGE_ALL, 0, config.pushConstantSize};
    if (config.pushConstantSize > 0)
    {
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
    }

    [[maybe_unused]]
    VkResult layoutResult = vkCreatePipelineLayout(vk.device, &layoutInfo, nullptr, &data->layout);
    if (data->layout == nullptr)
        HG_PANIC("Could not create VkPipelineLayout: %s\n", vkResultToStr(layoutResult));

    VkShaderModule vertexShader = createShaderModule(config.vertexShader.data, config.vertexShader.count);
    VkShaderModule fragmentShader = createShaderModule(config.fragmentShader.data, config.fragmentShader.count);
    HG_DEFER(vkDestroyShaderModule(vk.device, vertexShader, nullptr));
    HG_DEFER(vkDestroyShaderModule(vk.device, fragmentShader, nullptr));

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShader;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentShader;
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputState{};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = gpuTopologyToVk(config.topology);
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo tessellationState{};
    tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellationState.patchControlPoints = config.tesselationPatchControlPoints;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = gpuPolygonModeToVk(config.polygonMode);
    rasterizationState.cullMode = gpuCullModeToVk(config.cullMode);
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp = 0.0f;
    rasterizationState.depthBiasSlopeFactor = 0.0f;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = countToMsaaSampleBits(config.multisampleCount);
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.minSampleShading = 1.0f;
    multisampleState.pSampleMask = nullptr;
    multisampleState.alphaToCoverageEnable = VK_FALSE;
    multisampleState.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = config.enableDepthRead;
    depthStencilState.depthWriteEnable = config.enableDepthWrite;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable = config.enableDepthRead;
    depthStencilState.stencilTestEnable = VK_FALSE, // : TODO
    depthStencilState.front = {}; // : TODO
    depthStencilState.back = {}; // : TODO
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;

    ArrayTemp<VkPipelineColorBlendAttachmentState> colorBlendAttachments{scratch, 0, static_cast<u32>(config.colorAttachmentFormats.count)};

    for (u32 i = 0; i < config.colorAttachmentFormats.count; ++i)
    {
        VkPipelineColorBlendAttachmentState* attachment = colorBlendAttachments.push();
        attachment->blendEnable = config.colorBlendEnables.data != nullptr
            ? config.colorBlendEnables[i]
            : VK_FALSE;
        attachment->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachment->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment->colorBlendOp = VK_BLEND_OP_ADD;
        attachment->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment->alphaBlendOp = VK_BLEND_OP_ADD;
        attachment->colorWriteMask
            = VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = static_cast<u32>(colorBlendAttachments.count);
    colorBlendState.pAttachments = colorBlendAttachments.vals;
    for (float& blendConstant : colorBlendState.blendConstants)
    {
        blendConstant = 1.0f;
    }

    VkDynamicState dynamicStates[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(std::size(dynamicStates));
    dynamicState.pDynamicStates = dynamicStates;

    ArrayTemp<VkFormat> colorFormats{scratch, 0, static_cast<u32>(config.colorAttachmentFormats.count)};
    for (u32 i = 0; i < config.colorAttachmentFormats.count; ++i)
    {
        colorFormats.push(formatToVk(config.colorAttachmentFormats[i]));
    }
    VkFormat depthFormat = formatToVk(config.depthAttachmentFormat);
    VkFormat stencilFormat = formatToVk(config.stencilAttachmentFormat);

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = colorFormats.count;
    renderingInfo.pColorAttachmentFormats = colorFormats.vals;
    renderingInfo.depthAttachmentFormat = depthFormat;
    renderingInfo.stencilAttachmentFormat = stencilFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = static_cast<u32>(std::size(shaderStages));
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputState;
    pipelineInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineInfo.pTessellationState = &tessellationState;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizationState;
    pipelineInfo.pMultisampleState = &multisampleState;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = data->layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    [[maybe_unused]]
    VkResult pipelineResult = vkCreateGraphicsPipelines(
        vk.device, nullptr, 1, &pipelineInfo, nullptr, &data->pipeline);
    if (data->pipeline == nullptr)
        HG_PANIC("Failed to create Vulkan graphics pipeline: %s\n", vkResultToStr(pipelineResult));

    data->bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

GpuPipeline::GpuPipeline(const GpuComputePipelineCreateInfo& config)
    : data{makeUnique<GpuPipelineData>()}
{
    HG_ASSERT(config.shaderCode.data != nullptr);
    HG_ASSERT(config.shaderCode.count > 0);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &vk.bindlessLayout;

    VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, config.pushSize};
    layoutInfo.pushConstantRangeCount = config.pushSize > 0 ? 1 : 0;
    layoutInfo.pPushConstantRanges = config.pushSize > 0 ? &push : nullptr;

    [[maybe_unused]]
    VkResult layoutResult = vkCreatePipelineLayout(vk.device, &layoutInfo, nullptr, &data->layout);
    if (data->layout == nullptr)
        HG_PANIC("Could not create VkPipelineLayout: %s\n", vkResultToStr(layoutResult));

    VkShaderModule computeShader = createShaderModule(config.shaderCode.data, static_cast<u32>(config.shaderCode.count));
    HG_DEFER(vkDestroyShaderModule(vk.device, computeShader, nullptr));

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = computeShader;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = data->layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    [[maybe_unused]]
    VkResult pipelineResult = vkCreateComputePipelines(
        vk.device, nullptr, 1, &pipelineInfo, nullptr, &data->pipeline);
    if (data->pipeline == nullptr)
        HG_PANIC("Failed to create Vulkan compute pipeline: %s\n", vkResultToStr(pipelineResult));

    data->bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
}

GpuPipeline::~GpuPipeline()
{
    if (data != nullptr)
    {
        vkDestroyPipeline(vk.device, data->pipeline, nullptr);
        vkDestroyPipelineLayout(vk.device, data->layout, nullptr);
    }
}

GpuPipeline::GpuPipeline() noexcept = default;
GpuPipeline::GpuPipeline(GpuPipeline&&) noexcept = default;
GpuPipeline& GpuPipeline::operator=(GpuPipeline&&) noexcept = default;

GpuCmd* gpuCmdBegin()
{
    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = vk.cmdPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(vk.device, &cmdInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    return reinterpret_cast<GpuCmd*>(cmd);
}

void gpuCmdEnd(GpuCmd* cmd)
{
    HG_ASSERT(cmd != nullptr);
    vkEndCommandBuffer(reinterpret_cast<VkCommandBuffer>(cmd));

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;
    vkCreateFence(vk.device, &fenceInfo, nullptr, &fence);
    HG_DEFER(vkDestroyFence(vk.device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = reinterpret_cast<VkCommandBuffer*>(&cmd);

    vkQueueSubmit(vk.queue, 1, &submit, fence);
    vkWaitForFences(vk.device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkFreeCommandBuffers(vk.device, vk.cmdPool, 1, reinterpret_cast<VkCommandBuffer*>(&cmd));
}

void gpuBindPipeline(GpuCmd* cmd, const GpuPipeline& pipeline)
{
    vkCmdBindPipeline(
        reinterpret_cast<VkCommandBuffer>(cmd),
        pipeline.data->bindPoint,
        pipeline.data->pipeline);

    vkCmdBindDescriptorSets(
        reinterpret_cast<VkCommandBuffer>(cmd),
        pipeline.data->bindPoint,
        pipeline.data->layout,
        0,
        1,
        &vk.bindlessSet,
        0,
        nullptr);
}

void gpuPushConstants(GpuCmd* cmd, const GpuPipeline& pipeline, void* push, u32 size)
{
    vkCmdPushConstants(
        reinterpret_cast<VkCommandBuffer>(cmd),
        pipeline.data->layout,
        pipeline.data->bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE
            ? VK_SHADER_STAGE_COMPUTE_BIT
            : VK_SHADER_STAGE_ALL,
        0,
        size,
        push);
}

void gpuDraw(GpuCmd* cmd, u32 vertexBegin, u32 vertexCount, u32 instanceBegin, u32 instanceCount)
{
    vkCmdDraw(reinterpret_cast<VkCommandBuffer>(cmd), vertexCount, instanceCount, vertexBegin, instanceBegin);
}

void gpuDispatch(GpuCmd* cmd, u32 groupCountX, u32 groupCountY, u32 groupCountZ)
{
    vkCmdDispatch(reinterpret_cast<VkCommandBuffer>(cmd), groupCountX, groupCountY, groupCountZ);
}

void gpuMemoryBarrier(
    GpuCmd* cmd,
    Span<const GpuBufferBarrier> bufferBarriers,
    Span<const GpuImageBarrier> imageBarriers)
{
    ArenaScope scratch = getScratch();

    ArrayTemp<VkBufferMemoryBarrier2> vkBufferBarriers{scratch, 0, static_cast<u32>(bufferBarriers.count)};
    ArrayTemp<VkImageMemoryBarrier2> vkImageBarriers{scratch, 0, static_cast<u32>(imageBarriers.count)};

    for (const GpuBufferBarrier& barrier : bufferBarriers)
    {
        VkBufferMemoryBarrier2* vkBarrier = vkBufferBarriers.push();

        vkBarrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        vkBarrier->srcStageMask = gpuStageToVk(barrier.buffer->data->lastStage);
        vkBarrier->srcAccessMask = gpuAccessToVk(barrier.buffer->data->lastAccess);
        vkBarrier->dstStageMask = gpuStageToVk(barrier.nextStage);
        vkBarrier->dstAccessMask = gpuAccessToVk(barrier.nextAccess);
        vkBarrier->buffer = barrier.buffer->data->buffer;
        vkBarrier->size = barrier.buffer->data->size;

        barrier.buffer->data->lastStage = barrier.nextStage;
        barrier.buffer->data->lastAccess = barrier.nextAccess;
    }

    for (const GpuImageBarrier& barrier : imageBarriers)
    {
        VkImageMemoryBarrier2* vkBarrier = vkImageBarriers.push();

        vkBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        vkBarrier->srcStageMask = gpuStageToVk(barrier.image->data->lastStage);
        vkBarrier->srcAccessMask = gpuAccessToVk(barrier.image->data->lastAccess);
        vkBarrier->dstStageMask = gpuStageToVk(barrier.nextStage);
        vkBarrier->dstAccessMask = gpuAccessToVk(barrier.nextAccess);
        vkBarrier->oldLayout = gpuLayoutToVk(barrier.image->data->lastLayout);
        vkBarrier->newLayout = gpuLayoutToVk(barrier.nextLayout);
        vkBarrier->image = barrier.image->data->image->image;
        vkBarrier->subresourceRange = {
            gpuAspectToVk(barrier.image->data->aspectFlags),
            barrier.image->data->baseMipLevel,
            barrier.image->data->levelCount,
            barrier.image->data->baseArrayLayer,
            barrier.image->data->layerCount,
        };

        barrier.image->data->lastStage = barrier.nextStage;
        barrier.image->data->lastAccess = barrier.nextAccess;
        barrier.image->data->lastLayout = barrier.nextLayout;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = static_cast<u32>(vkBufferBarriers.count);
    dep.pBufferMemoryBarriers = vkBufferBarriers.vals;
    dep.imageMemoryBarrierCount = static_cast<u32>(vkImageBarriers.count);
    dep.pImageMemoryBarriers = vkImageBarriers.vals;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);
}

static VkAttachmentLoadOp gpuLoadOpToVk(GpuLoadOp op)
{
    return static_cast<VkAttachmentLoadOp>(op);
}

static VkAttachmentStoreOp gpuStoreOpToVk(GpuStoreOp op)
{
    return static_cast<VkAttachmentStoreOp>(op);
}

void gpuComputePass(GpuCmd* cmd, const GpuComputePass& pass)
{
    ArenaScope scratch = getScratch();

    ArrayTemp<VkBufferMemoryBarrier2> bufferBarriers{scratch, 0, 32};
    ArrayTemp<VkImageMemoryBarrier2> imageBarriers{scratch, 0, 32};

    for (u32 i = 0; i < pass.uniformBuffers.count; ++i)
    {
        VkBufferMemoryBarrier2* barrier = bufferBarriers.push();
        GpuBufferData* buffer = pass.uniformBuffers[i]->data;

        barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier->srcStageMask = gpuStageToVk(buffer->lastStage);
        barrier->srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        barrier->dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier->dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        barrier->buffer = buffer->buffer;
        barrier->size = buffer->size;

        buffer->lastStage = GpuStage_computeShader;
        buffer->lastAccess = GpuAccess_uniformRead;
    }

    for (u32 i = 0; i < pass.storageBuffers.count; ++i)
    {
        VkBufferMemoryBarrier2* barrier = bufferBarriers.push();
        GpuBufferData* buffer = pass.storageBuffers[i]->data;

        barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier->srcStageMask = gpuStageToVk(buffer->lastStage);
        barrier->srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        barrier->dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier->buffer = buffer->buffer;
        barrier->size = buffer->size;

        buffer->lastStage = GpuStage_computeShader;
        buffer->lastAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
    }

    for (u32 i = 0; i < pass.sampledImages.count; ++i)
    {
        VkImageMemoryBarrier2* barrier = imageBarriers.push();
        GpuViewData* image = pass.sampledImages[i]->data;

        barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier->srcStageMask = gpuStageToVk(image->lastStage);
        barrier->srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier->dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier->oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier->newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier->image = image->image->image;
        barrier->subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_computeShader;
        image->lastAccess = GpuAccess_shaderRead;
        image->lastLayout = GpuLayout_shaderReadOnly;
    }

    for (u32 i = 0; i < pass.storageImages.count; ++i)
    {
        VkImageMemoryBarrier2* barrier = imageBarriers.push();
        GpuViewData* image = pass.storageImages[i]->data;

        barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier->srcStageMask = gpuStageToVk(image->lastStage);
        barrier->srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier->dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier->oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier->newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier->image = image->image->image;
        barrier->subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_computeShader;
        image->lastAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
        image->lastLayout = GpuLayout_general;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = bufferBarriers.count;
    dep.pBufferMemoryBarriers = bufferBarriers.vals;
    dep.imageMemoryBarrierCount = imageBarriers.count;
    dep.pImageMemoryBarriers = imageBarriers.vals;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);
}

void gpuRenderPassBegin(GpuCmd* cmd, const GpuRenderPass& pass)
{
    ArenaScope scratch = getScratch();

    ArrayTemp<VkBufferMemoryBarrier2> bufferBarriers{scratch, 0, 32};
    ArrayTemp<VkImageMemoryBarrier2> imageBarriers{scratch, 0, 32};

    for (u32 i = 0; i < pass.uniformBuffers.count; ++i)
    {
        VkBufferMemoryBarrier2* barrier = bufferBarriers.push();
        GpuBufferData* buffer = pass.uniformBuffers[i]->data;

        barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier->srcStageMask = gpuStageToVk(buffer->lastStage);
        barrier->srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        barrier->dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier->dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        barrier->buffer = buffer->buffer;
        barrier->size = buffer->size;

        buffer->lastStage = GpuStage_vertexShader | GpuStage_fragmentShader;
        buffer->lastAccess = GpuAccess_uniformRead;
    }

    for (u32 i = 0; i < pass.storageBuffers.count; ++i)
    {
        VkBufferMemoryBarrier2* barrier = bufferBarriers.push();
        GpuBufferData* buffer = pass.storageBuffers[i]->data;

        barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier->srcStageMask = gpuStageToVk(buffer->lastStage);
        barrier->srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        barrier->dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier->buffer = buffer->buffer;
        barrier->size = buffer->size;

        buffer->lastStage = GpuStage_vertexShader | GpuStage_fragmentShader;
        buffer->lastAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
    }

    for (u32 i = 0; i < pass.sampledImages.count; ++i)
    {
        VkImageMemoryBarrier2* barrier = imageBarriers.push();
        GpuViewData* image = pass.sampledImages[i]->data;

        barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier->srcStageMask = gpuStageToVk(image->lastStage);
        barrier->srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier->dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier->oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier->newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier->image = image->image->image;
        barrier->subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_fragmentShader;
        image->lastAccess = GpuAccess_shaderRead;
        image->lastLayout = GpuLayout_shaderReadOnly;
    }

    for (u32 i = 0; i < pass.storageImages.count; ++i)
    {
        VkImageMemoryBarrier2* barrier = imageBarriers.push();
        GpuViewData* image = pass.storageImages[i]->data;

        barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier->srcStageMask = gpuStageToVk(image->lastStage);
        barrier->srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier->dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier->oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier->newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier->image = image->image->image;
        barrier->subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_fragmentShader;
        image->lastAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
        image->lastLayout = GpuLayout_general;
    }

    for (u32 i = 0; i < pass.colorAttachments.count; ++i)
    {
        VkImageMemoryBarrier2* barrier = imageBarriers.push();
        GpuViewData* image = pass.colorAttachments[i].image->data;

        barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier->srcStageMask = gpuStageToVk(image->lastStage);
        barrier->srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier->dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if (pass.colorAttachments[i].loadOp == GpuLoadOp_load)
            barrier->oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier->newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier->image = image->image->image;
        barrier->subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_colorAttachmentOutput;
        image->lastAccess = GpuAccess_colorAttachmentWrite;
        image->lastLayout = GpuLayout_colorAttachment;
    }

    if (pass.depthAttachment != nullptr)
    {
        VkImageMemoryBarrier2* barrier = imageBarriers.push();
        GpuViewData* image = pass.depthAttachment->image->data;

        barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier->srcStageMask = gpuStageToVk(image->lastStage);
        barrier->srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier->dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                              | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        barrier->dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                               | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if (pass.depthAttachment->loadOp == GpuLoadOp_load)
            barrier->oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier->newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        barrier->image = image->image->image;
        barrier->subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_earlyFragmentTests | GpuStage_lateFragmentTests;
        image->lastAccess = GpuAccess_depthStencilAttachmentRead | GpuAccess_depthStencilAttachmentWrite;
        image->lastLayout = GpuLayout_depthStencilAttachment;
    }

    if (pass.stencilAttachment != nullptr)
    {
        VkImageMemoryBarrier2* barrier = imageBarriers.push();
        GpuViewData* image = pass.stencilAttachment->image->data;

        barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier->srcStageMask = gpuStageToVk(image->lastStage);
        barrier->srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier->dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                              | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        barrier->dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                               | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if (pass.stencilAttachment->loadOp == GpuLoadOp_load)
            barrier->oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier->newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier->image = image->image->image;
        barrier->subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_earlyFragmentTests | GpuStage_lateFragmentTests;
        image->lastAccess = GpuAccess_depthStencilAttachmentRead | GpuAccess_depthStencilAttachmentWrite;
        image->lastLayout = GpuLayout_depthStencilAttachment;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = bufferBarriers.count;
    dep.pBufferMemoryBarriers = bufferBarriers.vals;
    dep.imageMemoryBarrierCount = imageBarriers.count;
    dep.pImageMemoryBarriers = imageBarriers.vals;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);

    ArrayTemp<VkRenderingAttachmentInfo> colorAttachments{scratch, 0, static_cast<u32>(pass.colorAttachments.count)};

    for (u32 i = 0; i < pass.colorAttachments.count; ++i)
    {
        VkRenderingAttachmentInfo* attachment = colorAttachments.push();
        attachment->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachment->imageView = pass.colorAttachments[i].image->data->view;
        attachment->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment->loadOp = gpuLoadOpToVk(pass.colorAttachments[i].loadOp);
        attachment->storeOp = gpuStoreOpToVk(pass.colorAttachments[i].storeOp);
        memcpy(&attachment->clearValue, &pass.colorAttachments[i].clearValue, sizeof(VkClearValue));
    }

    VkRenderingAttachmentInfo depthAttachment{};
    if (pass.depthAttachment != nullptr)
    {
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = pass.depthAttachment->image->data->view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = gpuLoadOpToVk(pass.depthAttachment->loadOp);
        depthAttachment.storeOp = gpuStoreOpToVk(pass.depthAttachment->storeOp);
        memcpy(&depthAttachment.clearValue, &pass.depthAttachment->clearValue, sizeof(VkClearValue));
    }

    VkRenderingAttachmentInfo stencilAttachment{};
    if (pass.stencilAttachment != nullptr)
    {
        stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        stencilAttachment.imageView = pass.stencilAttachment->image->data->view;
        stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        stencilAttachment.loadOp = gpuLoadOpToVk(pass.stencilAttachment->loadOp);
        stencilAttachment.storeOp = gpuStoreOpToVk(pass.stencilAttachment->storeOp);
        memcpy(&stencilAttachment.clearValue, &pass.stencilAttachment->clearValue, sizeof(VkClearValue));
    }

    u32 width, height;
    if (pass.colorAttachments.count > 0)
    {
        width = pass.colorAttachments[0].image->data->image->width;
        height = pass.colorAttachments[0].image->data->image->height;
    }
    else if (pass.depthAttachment != nullptr)
    {
        width = pass.depthAttachment->image->data->image->width;
        height = pass.depthAttachment->image->data->image->height;
    }
    else
    {
        HG_ASSERT(pass.stencilAttachment != nullptr);
        width = pass.stencilAttachment->image->data->image->width;
        height = pass.stencilAttachment->image->data->image->height;
    }

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = {width, height};
    renderingInfo.layerCount = pass.layerCount;
    renderingInfo.colorAttachmentCount = static_cast<u32>(pass.colorAttachments.count);
    renderingInfo.pColorAttachments = colorAttachments.vals;
    renderingInfo.pDepthAttachment = pass.depthAttachment != nullptr
        ? &depthAttachment
        : nullptr;
    renderingInfo.pStencilAttachment = pass.stencilAttachment != nullptr
        ? &stencilAttachment
        : nullptr;

    vkCmdBeginRendering(reinterpret_cast<VkCommandBuffer>(cmd), &renderingInfo);

    gpuSetViewport(cmd, 0, 0, static_cast<f32>(width), static_cast<f32>(height));
    gpuSetScissor(cmd, 0, 0, width, height);
}

static VkPresentModeKHR presentModeToVk(GpuPresentMode mode)
{
    return static_cast<VkPresentModeKHR>(mode);
}

void gpuRenderPassEnd(GpuCmd* cmd)
{
    vkCmdEndRendering(reinterpret_cast<VkCommandBuffer>(cmd));
}

void gpuSetViewport(GpuCmd* cmd, f32 x, f32 y, f32 width, f32 height, f32 near, f32 far)
{
    VkViewport viewport{x, y, width, height, near, far};
    vkCmdSetViewport(reinterpret_cast<VkCommandBuffer>(cmd), 0, 1, &viewport);
}

void gpuSetScissor(GpuCmd* cmd, i32 x, i32 y, u32 width, u32 height)
{
    VkRect2D scissor{{x, y}, {width, height}};
    vkCmdSetScissor(reinterpret_cast<VkCommandBuffer>(cmd), 0, 1, &scissor);
}

struct WindowData {
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    Array<GpuImage> images;
    Array<GpuView> views;
    Array<VkSemaphore> imageAvailable;
    Array<VkSemaphore> readyToPresent;
    u32 imageIdx;
    Format format;
    GpuImageUsageFlags imageUsage;
    GpuPresentMode presentMode;

    SDL_Window* sdlWindow;
    u32 width;
    u32 height;
    f32 mouseX;
    f32 mouseY;
    bool isKeyDown[Button_count]{};
    bool wasClosed = false;

    Array<WindowEvent> events;
};

struct WindowState {
    Map<SDL_WindowID, WindowData*> ids{};

    f32 mouseDX = 0.0f;
    f32 mouseDY = 0.0f;
    bool wasQuit = false;

    bool imguiInitialized = false;
};

static WindowState windowState{};

static Button sdlKeycodeToHgButton(u32 key)
{
    switch (key)
    {
        case SDLK_0:
            return Button_k0;
        case SDLK_1:
            return Button_k1;
        case SDLK_2:
            return Button_k2;
        case SDLK_3:
            return Button_k3;
        case SDLK_4:
            return Button_k4;
        case SDLK_5:
            return Button_k5;
        case SDLK_6:
            return Button_k6;
        case SDLK_7:
            return Button_k7;
        case SDLK_8:
            return Button_k8;
        case SDLK_9:
            return Button_k9;

        case SDLK_Q:
            return Button_q;
        case SDLK_W:
            return Button_w;
        case SDLK_E:
            return Button_e;
        case SDLK_R:
            return Button_r;
        case SDLK_T:
            return Button_t;
        case SDLK_Y:
            return Button_y;
        case SDLK_U:
            return Button_u;
        case SDLK_I:
            return Button_i;
        case SDLK_O:
            return Button_o;
        case SDLK_P:
            return Button_p;
        case SDLK_A:
            return Button_a;
        case SDLK_S:
            return Button_s;
        case SDLK_D:
            return Button_d;
        case SDLK_F:
            return Button_f;
        case SDLK_G:
            return Button_g;
        case SDLK_H:
            return Button_h;
        case SDLK_J:
            return Button_j;
        case SDLK_K:
            return Button_k;
        case SDLK_L:
            return Button_l;
        case SDLK_Z:
            return Button_z;
        case SDLK_X:
            return Button_x;
        case SDLK_C:
            return Button_c;
        case SDLK_V:
            return Button_v;
        case SDLK_B:
            return Button_b;
        case SDLK_N:
            return Button_n;
        case SDLK_M:
            return Button_m;

        case SDLK_SEMICOLON:
            return Button_semicolon;
        case SDLK_COLON:
            return Button_colon;
        case SDLK_APOSTROPHE:
            return Button_apostrophe;
        case SDLK_DBLAPOSTROPHE:
            return Button_quotation;
        case SDLK_COMMA:
            return Button_comma;
        case SDLK_PERIOD:
            return Button_period;
        case SDLK_QUESTION:
            return Button_question;
        case SDLK_GRAVE:
            return Button_grave;
        case SDLK_TILDE:
            return Button_tilde;
        case SDLK_EXCLAIM:
            return Button_exclamation;
        case SDLK_AT:
            return Button_at;
        case SDLK_HASH:
            return Button_hash;
        case SDLK_DOLLAR:
            return Button_dollar;
        case SDLK_PERCENT:
            return Button_percent;
        case SDLK_CARET:
            return Button_carot;
        case SDLK_AMPERSAND:
            return Button_ampersand;
        case SDLK_ASTERISK:
            return Button_asterisk;

        case SDLK_LEFTPAREN:
            return Button_lparen;
        case SDLK_RIGHTPAREN:
            return Button_rparen;
        case SDLK_LEFTBRACKET:
            return Button_lbracket;
        case SDLK_RIGHTBRACKET:
            return Button_rbracket;
        case SDLK_LEFTBRACE:
            return Button_lbrace;
        case SDLK_RIGHTBRACE:
            return Button_rbrace;

        case SDLK_EQUALS:
            return Button_equal;
        case SDLK_LESS:
            return Button_less;
        case SDLK_GREATER:
            return Button_greater;
        case SDLK_PLUS:
            return Button_plus;
        case SDLK_MINUS:
            return Button_minus;
        case SDLK_SLASH:
            return Button_slash;
        case SDLK_BACKSLASH:
            return Button_backslash;
        case SDLK_UNDERSCORE:
            return Button_underscore;
        case SDLK_PIPE:
            return Button_bar;

        case SDLK_UP:
            return Button_up;
        case SDLK_DOWN:
            return Button_down;
        case SDLK_LEFT:
            return Button_left;
        case SDLK_RIGHT:
            return Button_right;

        case SDLK_ESCAPE:
            return Button_escape;
        case SDLK_SPACE:
            return Button_space;
        case SDLK_RETURN:
            return Button_enter;
        case SDLK_BACKSPACE:
            return Button_backspace;
        case SDLK_DELETE:
            return Button_kdelete;
        case SDLK_INSERT:
            return Button_insert;
        case SDLK_TAB:
            return Button_tab;
        case SDLK_HOME:
            return Button_home;
        case SDLK_END:
            return Button_end;

        case SDLK_F1:
            return Button_f1;
        case SDLK_F2:
            return Button_f2;
        case SDLK_F3:
            return Button_f3;
        case SDLK_F4:
            return Button_f4;
        case SDLK_F5:
            return Button_f5;
        case SDLK_F6:
            return Button_f6;
        case SDLK_F7:
            return Button_f7;
        case SDLK_F8:
            return Button_f8;
        case SDLK_F9:
            return Button_f9;
        case SDLK_F10:
            return Button_f10;
        case SDLK_F11:
            return Button_f11;
        case SDLK_F12:
            return Button_f12;

        case SDLK_LSHIFT:
            return Button_lshift;
        case SDLK_RSHIFT:
            return Button_rshift;
        case SDLK_LCTRL:
            return Button_lctrl;
        case SDLK_RCTRL:
            return Button_rctrl;
        case SDLK_LALT:
            return Button_lalt;
        case SDLK_RALT:
            return Button_ralt;
        case SDLK_LGUI:
            return Button_lsuper;
        case SDLK_RGUI:
            return Button_rsuper;
        case SDLK_CAPSLOCK:
            return Button_capslock;
    }
    return Button_none;
}

static Button sdlButtonToHgButton(u32 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT:
            return Button_mouse1;
        case SDL_BUTTON_RIGHT:
            return Button_mouse2;
        case SDL_BUTTON_MIDDLE:
            return Button_mouse3;
        case SDL_BUTTON_X1:
            return Button_mouse4;
        case SDL_BUTTON_X2:
            return Button_mouse5;
    }
    return Button_none;
}

static void resizeWindowSwapchain(WindowData* window)
{
    ArenaScope scratch = getScratch();

    vkQueueWaitIdle(vk.queue);

    for (u32 i = 0; i < window->images.count; ++i)
    {
        vkDestroyImageView(vk.device, window->views[i].data->view, nullptr);

        vkDestroySemaphore(vk.device, window->readyToPresent[i], nullptr);
        window->readyToPresent[i] = nullptr;
    }

    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        vkDestroySemaphore(vk.device, window->imageAvailable[i], nullptr);
        window->imageAvailable[i] = nullptr;
    }

    VkSwapchainKHR oldSwapchain = window->swapchain;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physicalDevice, window->surface, &capabilities);

    if (capabilities.currentExtent.width != (u32)-1)
        window->width = capabilities.currentExtent.width;
    if (capabilities.currentExtent.height != (u32)-1)
        window->height = capabilities.currentExtent.height;

    if (window->width != 0 && window->height != 0)
    {
        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = window->surface;
        swapchainInfo.minImageCount = std::min(capabilities.minImageCount, capabilities.maxImageCount - 1) + 1;
        swapchainInfo.imageFormat = formatToVk(window->format);
        swapchainInfo.imageExtent = {window->width, window->height};
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = window->imageUsage;
        swapchainInfo.preTransform = capabilities.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = presentModeToVk(window->presentMode);
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = window->swapchain;

        [[maybe_unused]]         VkResult result = vkCreateSwapchainKHR(vk.device, &swapchainInfo, nullptr, &window->swapchain);
        if (window->swapchain == nullptr)
            HG_PANIC("Failed to create swapchain: %s\n", vkResultToStr(result));

        u32 swapImageCount;
        vkGetSwapchainImagesKHR(vk.device, window->swapchain, &swapImageCount, nullptr);

        if (window->images.count != swapImageCount)
        {
            window->images.resize(swapImageCount);
            window->views.resize(swapImageCount);
            window->readyToPresent.resize(swapImageCount);
        }

        VkImage* swapImages = scratch.alloc<VkImage>(swapImageCount);
        vkGetSwapchainImagesKHR(vk.device, window->swapchain, &swapImageCount, swapImages);

        for (u32 i = 0; i < window->images.count; ++i)
        {
            if (window->images[i].data == nullptr)
                window->images[i].data = makeUnique<GpuImageData>();
            window->images[i].data->image = swapImages[i];
            window->images[i].data->dimensions = 2;
            window->images[i].data->format = window->format;
            window->images[i].data->width = window->width;
            window->images[i].data->height = window->height;
            window->images[i].data->depth = 1;
            window->images[i].data->mipLevels = 1;
            window->images[i].data->arrayLayers = 1;
            window->images[i].data->msaaSamples = 1;

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = formatToVk(window->format);
            viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            if (window->views[i].data == nullptr)
                window->views[i].data = makeUnique<GpuViewData>();

            [[maybe_unused]]
            VkResult viewResult = vkCreateImageView(vk.device, &viewInfo, nullptr, &window->views[i].data->view);
            if (window->views[i].data->view == nullptr)
                HG_PANIC("Could not create VkImageView: %s\n", vkResultToStr(viewResult));

            window->views[i].data->image = window->images[i].data;
            window->views[i].data->type = GpuViewType_2D;
            window->views[i].data->aspectFlags = GpuAspect_color;
            window->views[i].data->baseMipLevel = 0;
            window->views[i].data->levelCount = 1;
            window->views[i].data->baseArrayLayer = 0;
            window->views[i].data->layerCount = 1;

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            [[maybe_unused]]             VkResult readyToPresentResult = vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &window->readyToPresent[i]);
            if (window->readyToPresent[i] == nullptr)
                HG_PANIC("Could not create VkSemaphore: %s\n", vkResultToStr(readyToPresentResult));
        }

        for (u32 i = 0; i < vk.frameCount; ++i)
        {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            [[maybe_unused]]             VkResult imageAvailableResult = vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &window->imageAvailable[i]);
            if (window->imageAvailable[i] == nullptr)
                HG_PANIC("Could not create VkSemaphore: %s\n", vkResultToStr(imageAvailableResult));
        }
    }
    else
    {
        window->swapchain = nullptr;
    }

    window->imageIdx = (u32)-1;

    vkDestroySwapchainKHR(vk.device, oldSwapchain, nullptr);
}

void processEvents()
{
    windowState.mouseDX = 0;
    windowState.mouseDY = 0;

    windowState.ids.forEach([&](SDL_WindowID*, WindowData** window)
    {
        (*window)->events.count = 0;
    });

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (windowState.imguiInitialized)
            ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                windowState.wasQuit = true;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                WindowData** window = windowState.ids.get(event.window.windowID);
                if (window != nullptr)
                    (*window)->wasClosed = true;
            } break;
            case SDL_EVENT_WINDOW_MINIMIZED: [[fallthrough]];
            case SDL_EVENT_WINDOW_RESTORED: [[fallthrough]];
            case SDL_EVENT_WINDOW_RESIZED:
            {
                WindowData** window = windowState.ids.get(event.window.windowID);
                if (window != nullptr)
                {
                    SDL_GetWindowSize((*window)->sdlWindow, reinterpret_cast<int*>(&(*window)->width), reinterpret_cast<int*>(&(*window)->height));
                    resizeWindowSwapchain(*window);
                }
            } break;
            case SDL_EVENT_MOUSE_MOTION:
            {
                WindowData** window = windowState.ids.get(event.button.windowID);
                if (window != nullptr)
                {
                    (*window)->mouseX = event.motion.x;
                    (*window)->mouseY = event.motion.y;
                }
                windowState.mouseDX += event.motion.xrel;
                windowState.mouseDY += event.motion.yrel;
            } break;
            case SDL_EVENT_KEY_DOWN:
            {
                Button key = sdlKeycodeToHgButton(event.key.key);
                WindowData** window = windowState.ids.get(event.key.windowID);
                if (window != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.button.type = WindowEventType_buttonPress;
                    windowEvent.button.button = key;

                    (*window)->events.push(windowEvent);
                    (*window)->isKeyDown[key] = true;
                }
            } break;
            case SDL_EVENT_KEY_UP:
            {
                Button key = sdlKeycodeToHgButton(event.key.key);
                WindowData** window = windowState.ids.get(event.key.windowID);
                if (window != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.button.type = WindowEventType_buttonRelease;
                    windowEvent.button.button = key;

                    (*window)->events.push(windowEvent);
                    (*window)->isKeyDown[key] = false;
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                Button key = sdlButtonToHgButton(event.button.button);
                WindowData** window = windowState.ids.get(event.button.windowID);
                if (window != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.button.type = WindowEventType_buttonPress;
                    windowEvent.button.button = key;

                    (*window)->events.push(windowEvent);
                    (*window)->isKeyDown[key] = true;
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                Button key = sdlButtonToHgButton(event.button.button);
                WindowData** window = windowState.ids.get(event.button.windowID);
                if (window != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.button.type = WindowEventType_buttonRelease;
                    windowEvent.button.button = key;

                    (*window)->events.push(windowEvent);
                    (*window)->isKeyDown[key] = false;
                }
            } break;
        }
    }
}

bool wasQuit()
{
    return windowState.wasQuit;
}

static Format findSwapchainFormat(VkSurfaceKHR surface)
{
    HG_ASSERT(surface != nullptr);

    ArenaScope scratch = getScratch();

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, surface, &formatCount, nullptr);
    VkSurfaceFormatKHR* formats = scratch.alloc<VkSurfaceFormatKHR>(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, surface, &formatCount, formats);

    for (u32 i = 0; i < formatCount; ++i)
    {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return Format_r8g8b8a8_srgb;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return Format_b8g8r8a8_srgb;
    }
    HG_PANIC("No supported swapchain formats\n");
}

static GpuPresentMode findSwapchainPresentMode(
    VkSurfaceKHR surface,
    GpuPresentMode desiredMode)
{
    HG_ASSERT(surface != nullptr);

    ArenaScope scratch = getScratch();

    if (desiredMode == GpuPresentMode_fifo)
        return desiredMode;

    u32 modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, surface, &modeCount, nullptr);
    VkPresentModeKHR* presentModes = scratch.alloc<VkPresentModeKHR>(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, surface, &modeCount, presentModes);

    for (u32 i = 0; i < modeCount; ++i)
    {
        if (presentModes[i] == presentModeToVk(desiredMode))
            return desiredMode;
    }
    return GpuPresentMode_fifo;
}

Maybe<Window> Window::create(StringView title, u32 width, u32 height, const WindowConfig& config)
{
    Maybe<Window> window = some<Window>();
    window->data = makeUnique<WindowData>();

    if (title == "")
        title = "Hurdy Gurdy";

    u64 flags = SDL_WINDOW_VULKAN;
    if (!config.fixedSize)
        flags |= SDL_WINDOW_RESIZABLE;

    if (config.fullscreen)
    {
        int modeCount = 0;
        SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &modeCount);
        HG_DEFER(SDL_free(modes));

        width = static_cast<u32>(modes[0]->w);
        height = static_cast<u32>(modes[0]->h);
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    ArenaScope scratch = getScratch();

    window->data->sdlWindow = SDL_CreateWindow(cString(scratch, title), static_cast<int>(width), static_cast<int>(height), flags);
    if (window->data->sdlWindow == nullptr)
    {
        setError(SDL_GetError());
        goto windowFailed;
    }

    if (!SDL_Vulkan_CreateSurface(window->data->sdlWindow, vk.instance, nullptr, &window->data->surface))
    {
        setError(SDL_GetError());
        goto surfaceFailed;
    }

    windowState.ids.add(SDL_GetWindowID(window->data->sdlWindow), window->data);

    SDL_GetWindowSize(window->data->sdlWindow, reinterpret_cast<int*>(&window->data->width), reinterpret_cast<int*>(&window->data->height));

    window->data->format = findSwapchainFormat(window->data->surface);
    window->data->presentMode = findSwapchainPresentMode(window->data->surface, config.preferredPresentMode);
    window->data->imageUsage = config.imageUsage;

    window->data->imageAvailable = Array<VkSemaphore>{vk.frameCount, vk.frameCount};
    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        window->data->imageAvailable[i] = nullptr;
    }

    resizeWindowSwapchain(window->data);

    window->data->events = Array<WindowEvent>(0, 1024);

    return window;

surfaceFailed:
    SDL_DestroyWindow(window->data->sdlWindow);
windowFailed:
    window->data = {};
    return {};
}

Window::~Window() noexcept
{
    if (data != nullptr)
    {
        for (GpuImage& image : data->images)
        {
            image.data->image = nullptr;
        }
        for (VkSemaphore semaphore : data->readyToPresent)
        {
            vkDestroySemaphore(vk.device, semaphore, nullptr);
        }
        for (VkSemaphore semaphore : data->imageAvailable)
        {
            vkDestroySemaphore(vk.device, semaphore, nullptr);
        }
        vkDestroySwapchainKHR(vk.device, data->swapchain, nullptr);
        vkDestroySurfaceKHR(vk.instance, data->surface, nullptr);

        windowState.ids.remove(SDL_GetWindowID(data->sdlWindow));
        SDL_DestroyWindow(data->sdlWindow);
    }
}

Window::Window() noexcept = default;
Window::Window(Window&& other) noexcept = default;
Window& Window::operator=(Window&& other) noexcept = default;

GpuView* Window::imageView()
{
    return data->imageIdx < data->images.count ? &data->views[data->imageIdx] : nullptr;
}

Format Window::imageFormat()
{
    return data->format;
}

bool Window::wasClosed()
{
    return data->wasClosed;
}

bool Window::isFocused()
{
    return SDL_GetMouseFocus() == data->sdlWindow;
}

u32 Window::width()
{
    return data->width;
}

u32 Window::height()
{
    return data->height;
}

f32 Window::mouseX()
{
    return data->mouseX;
}

f32 Window::mouseY()
{
    return data->mouseY;
}

f32 Window::mouseDX()
{
    return windowState.mouseDX / static_cast<f32>(data->height);
}

f32 Window::mouseDY()
{
    return windowState.mouseDY / static_cast<f32>(data->height);
}

bool Window::isButtonDown(Button key)
{
    return data->isKeyDown[key];
}

Span<WindowEvent> Window::events()
{
    return data->events;
}

GpuCmd* gpuFrameBegin(Span<Window*> windows)
{
    Frame* frame = &vk.frames[vk.currentFrame];

    vkWaitForFences(vk.device, 1, &frame->fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &frame->fence);

    frame->windows.count = 0;
    for (u32 i = 0; i < windows.count; ++i)
    {
        if (windows[i]->data->swapchain == nullptr)
            continue;

        VkResult result = vkAcquireNextImageKHR(
            vk.device,
            windows[i]->data->swapchain,
            UINT64_MAX,
            windows[i]->data->imageAvailable[vk.currentFrame],
            nullptr,
            &windows[i]->data->imageIdx);

        if (result == VK_SUCCESS)
        {
            frame->windows.push(windows[i]->data);
        }
        else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            resizeWindowSwapchain(windows[i]->data);
            windows[i]->data->imageIdx = (u32)-1;
        }
        else
        {
            HG_PANIC("Could not acquire next image: %s\n", vkResultToStr(result));
        }
    }

    vkResetCommandPool(vk.device, frame->cmdPool, 0);

    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = frame->cmdPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(vk.device, &cmdInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    return reinterpret_cast<GpuCmd*>(cmd);
}

void gpuFrameEnd(GpuCmd* cmd)
{
    HG_ASSERT(cmd != nullptr);

    ArenaScope scratch = getScratch();

    Frame* frame = &vk.frames[vk.currentFrame];

    ArrayTemp<GpuImageBarrier> presentBarriers = ArrayTemp<GpuImageBarrier>{scratch, 0, frame->windows.count};
    for (u32 i = 0; i < frame->windows.count; ++i)
    {
        GpuImageBarrier* barrier = presentBarriers.push();
        barrier->image = &frame->windows[i]->views[frame->windows[i]->imageIdx];
        barrier->nextLayout = GpuLayout_presentSrc;
    }
    gpuMemoryBarrier(cmd, {}, presentBarriers);

    vkEndCommandBuffer(reinterpret_cast<VkCommandBuffer>(cmd));

    VkPipelineStageFlags* waitStages = scratch.alloc<VkPipelineStageFlags>(frame->windows.count);
    VkSemaphore* imageAvailableSemaphores = scratch.alloc<VkSemaphore>(frame->windows.count);
    VkSemaphore* readyToPresentSemaphores = scratch.alloc<VkSemaphore>(frame->windows.count);

    VkSwapchainKHR* swapchains = scratch.alloc<VkSwapchainKHR>(frame->windows.count);
    u32* imageIndices = scratch.alloc<u32>(frame->windows.count);

    for (u32 i = 0; i < frame->windows.count; ++i)
    {
        waitStages[i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        imageAvailableSemaphores[i] = frame->windows[i]->imageAvailable[vk.currentFrame];
        readyToPresentSemaphores[i] = frame->windows[i]->readyToPresent[frame->windows[i]->imageIdx];

        swapchains[i] = frame->windows[i]->swapchain;
        imageIndices[i] = frame->windows[i]->imageIdx;
        frame->windows[i]->imageIdx = (u32)-1;
    }

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = frame->windows.count;
    submit.pWaitSemaphores = imageAvailableSemaphores;
    submit.pWaitDstStageMask = waitStages;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = reinterpret_cast<VkCommandBuffer*>(&cmd);
    submit.signalSemaphoreCount = frame->windows.count;
    submit.pSignalSemaphores = readyToPresentSemaphores;

    vkQueueSubmit(vk.queue, 1, &submit, frame->fence);

    if (frame->windows.count > 0)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = frame->windows.count;
        presentInfo.pWaitSemaphores = readyToPresentSemaphores;
        presentInfo.swapchainCount = frame->windows.count;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = imageIndices;

        vkQueuePresentKHR(vk.queue, &presentInfo);
    }

    vk.currentFrame = (vk.currentFrame + 1) % vk.frameCount;
}

void initImGui(
    const Window& window,
    Format colorFormat,
    Format depthFormat,
    Format stencilFormat)
{
    HG_ASSERT(colorFormat != Format_undefined);

    ArenaScope scratch = getScratch();

    ImGui_ImplSDL3_InitForVulkan(window.data->sdlWindow);

    VkFormat colorVkFormat = formatToVk(colorFormat);
    VkFormat depthVkFormat = formatToVk(depthFormat);
    VkFormat stencilVkFormat = formatToVk(stencilFormat);

    ImGui_ImplVulkan_InitInfo imguiInfo{};
    imguiInfo.Instance = vk.instance;
    imguiInfo.PhysicalDevice = vk.physicalDevice;
    imguiInfo.Device = vk.device;
    imguiInfo.QueueFamily = vk.queueFamily;
    imguiInfo.Queue = vk.queue;
    imguiInfo.DescriptorPoolSize = 1000;
    imguiInfo.MinImageCount = window.data->images.count;
    imguiInfo.ImageCount = window.data->images.count;
    imguiInfo.MinAllocationSize = 1 << 20;
    imguiInfo.UseDynamicRendering = true;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorVkFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = depthVkFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = stencilVkFormat;

    ImGui_ImplVulkan_Init(&imguiInfo);

    windowState.imguiInitialized = true;
}

void deinitImGui()
{
    windowState.imguiInitialized = false;

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void* createImGuiTexture(const GpuView& view, GpuLayout layout)
{
    return ImGui_ImplVulkan_AddTexture(view.data->sampler, view.data->view, gpuLayoutToVk(layout));
}

void destroyImGuiTexture(void* texture)
{
    ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(texture));
}

void beginImGuiFrame()
{
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
}

void renderImGui(GpuCmd* cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), reinterpret_cast<VkCommandBuffer>(cmd));
}

struct AudioState {
    SDL_AudioDeviceID device = 0;
    Array<SDL_AudioStream*> streams = {};
};

static AudioState audio{};

bool internal::initAudio()
{
    audio.device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (audio.device == 0)
    {
        setError("SDL could not open audio device: %s", SDL_GetError());
        return false;
    }

    audio.streams = Array<SDL_AudioStream*>(0, 1024);

    return true;
}

void internal::deinitAudio()
{
    for (u32 i = 0; i < audio.streams.count; ++i)
    {
        SDL_DestroyAudioStream(audio.streams[i]);
    }
    audio.streams = {};

    SDL_CloseAudioDevice(audio.device);
}

AudioStream::AudioStream(u32 frequency, u32 channels)
{
    SDL_AudioSpec audioSpec{};
    audioSpec.format = SDL_AUDIO_F32;
    audioSpec.freq = static_cast<int>(frequency);
    audioSpec.channels = static_cast<int>(channels);

    SDL_AudioStream* stream;
    if (audio.streams.count == 0)
    {
        stream = SDL_CreateAudioStream(&audioSpec, nullptr);
        if (stream == nullptr)
        {
            HG_PANIC("Could not create audio stream: %s\n", SDL_GetError());
        }

        if (!SDL_BindAudioStream(audio.device, stream))
        {
            SDL_DestroyAudioStream(stream);
            HG_PANIC("Could not create audio stream: %s\n", SDL_GetError());
        }
    }
    else
    {
        stream = audio.streams.pop();
        if (!SDL_SetAudioStreamFormat(stream, &audioSpec, nullptr))
            HG_PANIC("SDL could not set audio stream format: %s\n", SDL_GetError());
    }

    data = reinterpret_cast<AudioStreamData*>(stream);
}

AudioStream::~AudioStream()
{
    if (data != nullptr)
    {
        SDL_AudioStream* sdlStream = reinterpret_cast<SDL_AudioStream*>(data);

        if (!SDL_ClearAudioStream(sdlStream))
            HG_PANIC("SDL could not clear audio stream: %s\n", SDL_GetError());

        audio.streams.push(sdlStream);
    }
}

void AudioStream::push(Span<f32> samples)
{
    SDL_AudioStream* stream = reinterpret_cast<SDL_AudioStream*>(data);
    if (!SDL_PutAudioStreamData(stream, samples.data, static_cast<int>(samples.count * sizeof(f32))))
        HG_PANIC("SDL could not push audio data: %s\n", SDL_GetError());
}

void AudioStream::clear()
{
    if (!SDL_ClearAudioStream(reinterpret_cast<SDL_AudioStream*>(data)))
        HG_PANIC("SDL could not clear audio stream: %s\n", SDL_GetError());
}

u32 AudioStream::queuedSize()
{
    int size = SDL_GetAudioStreamQueued(reinterpret_cast<SDL_AudioStream*>(data));
    if (size == -1)
        HG_PANIC("SDL could not read audio data: %s\n", SDL_GetError());

    return static_cast<u32>(size) / sizeof(f32);
}

void AudioStream::setGain(f32 gain)
{
    if (!SDL_SetAudioStreamGain(reinterpret_cast<SDL_AudioStream*>(data), gain))
        HG_PANIC("SDL could not clear audio stream: %s\n", SDL_GetError());
}

// template<>
// void serialize(Serializer* s, AudioSource* src)
// {
//     serializeObject(s,
//         &src->audio,
//         &src->position,
//         &src->repeat);
// }

#ifdef HG_PLATFORM_LINUX
#include <dlfcn.h>
#elif defined(HG_PLATFORM_WINDOWS)
#include <windows.h>
#endif

#define HG_MAKE_VULKAN_FUNC(name) PFN_##name name = nullptr

struct VulkanFuncs {
    HG_MAKE_VULKAN_FUNC(vkGetInstanceProcAddr);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceProcAddr);

    HG_MAKE_VULKAN_FUNC(vkCreateInstance);
    HG_MAKE_VULKAN_FUNC(vkDestroyInstance);

    HG_MAKE_VULKAN_FUNC(vkCreateDebugUtilsMessengerEXT);
    HG_MAKE_VULKAN_FUNC(vkDestroyDebugUtilsMessengerEXT);

    HG_MAKE_VULKAN_FUNC(vkEnumeratePhysicalDevices);
    HG_MAKE_VULKAN_FUNC(vkEnumerateDeviceExtensionProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);

    HG_MAKE_VULKAN_FUNC(vkDestroySurfaceKHR);
    HG_MAKE_VULKAN_FUNC(vkCreateDevice);

    HG_MAKE_VULKAN_FUNC(vkDestroyDevice);
    HG_MAKE_VULKAN_FUNC(vkDeviceWaitIdle);

    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    HG_MAKE_VULKAN_FUNC(vkCreateSwapchainKHR);
    HG_MAKE_VULKAN_FUNC(vkDestroySwapchainKHR);
    HG_MAKE_VULKAN_FUNC(vkGetSwapchainImagesKHR);
    HG_MAKE_VULKAN_FUNC(vkAcquireNextImageKHR);

    HG_MAKE_VULKAN_FUNC(vkCreateSemaphore);
    HG_MAKE_VULKAN_FUNC(vkDestroySemaphore);
    HG_MAKE_VULKAN_FUNC(vkCreateFence);
    HG_MAKE_VULKAN_FUNC(vkDestroyFence);
    HG_MAKE_VULKAN_FUNC(vkResetFences);
    HG_MAKE_VULKAN_FUNC(vkWaitForFences);

    HG_MAKE_VULKAN_FUNC(vkGetDeviceQueue);
    HG_MAKE_VULKAN_FUNC(vkQueueWaitIdle);
    HG_MAKE_VULKAN_FUNC(vkQueueSubmit);
    HG_MAKE_VULKAN_FUNC(vkQueuePresentKHR);

    HG_MAKE_VULKAN_FUNC(vkCreateCommandPool);
    HG_MAKE_VULKAN_FUNC(vkDestroyCommandPool);
    HG_MAKE_VULKAN_FUNC(vkResetCommandPool);
    HG_MAKE_VULKAN_FUNC(vkAllocateCommandBuffers);
    HG_MAKE_VULKAN_FUNC(vkFreeCommandBuffers);

    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkResetDescriptorPool);
    HG_MAKE_VULKAN_FUNC(vkAllocateDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkFreeDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkUpdateDescriptorSets);

    HG_MAKE_VULKAN_FUNC(vkCreateDescriptorSetLayout);
    HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorSetLayout);
    HG_MAKE_VULKAN_FUNC(vkCreatePipelineLayout);
    HG_MAKE_VULKAN_FUNC(vkDestroyPipelineLayout);
    HG_MAKE_VULKAN_FUNC(vkCreateShaderModule);
    HG_MAKE_VULKAN_FUNC(vkDestroyShaderModule);
    HG_MAKE_VULKAN_FUNC(vkCreateGraphicsPipelines);
    HG_MAKE_VULKAN_FUNC(vkCreateComputePipelines);
    HG_MAKE_VULKAN_FUNC(vkDestroyPipeline);

    HG_MAKE_VULKAN_FUNC(vkCreateRenderPass);
    HG_MAKE_VULKAN_FUNC(vkDestroyRenderPass);
    HG_MAKE_VULKAN_FUNC(vkCreateFramebuffer);
    HG_MAKE_VULKAN_FUNC(vkDestroyFramebuffer);

    HG_MAKE_VULKAN_FUNC(vkCreateBuffer);
    HG_MAKE_VULKAN_FUNC(vkDestroyBuffer);
    HG_MAKE_VULKAN_FUNC(vkCreateImage);
    HG_MAKE_VULKAN_FUNC(vkDestroyImage);
    HG_MAKE_VULKAN_FUNC(vkCreateImageView);
    HG_MAKE_VULKAN_FUNC(vkDestroyImageView);
    HG_MAKE_VULKAN_FUNC(vkCreateSampler);
    HG_MAKE_VULKAN_FUNC(vkDestroySampler);

    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties);
    HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties2);
    HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements2);
    HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements2);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceBufferMemoryRequirements);
    HG_MAKE_VULKAN_FUNC(vkGetDeviceImageMemoryRequirements);

    HG_MAKE_VULKAN_FUNC(vkAllocateMemory);
    HG_MAKE_VULKAN_FUNC(vkFreeMemory);
    HG_MAKE_VULKAN_FUNC(vkBindBufferMemory);
    HG_MAKE_VULKAN_FUNC(vkBindBufferMemory2);
    HG_MAKE_VULKAN_FUNC(vkBindImageMemory);
    HG_MAKE_VULKAN_FUNC(vkBindImageMemory2);
    HG_MAKE_VULKAN_FUNC(vkMapMemory);
    HG_MAKE_VULKAN_FUNC(vkUnmapMemory);
    HG_MAKE_VULKAN_FUNC(vkFlushMappedMemoryRanges);
    HG_MAKE_VULKAN_FUNC(vkInvalidateMappedMemoryRanges);

    HG_MAKE_VULKAN_FUNC(vkBeginCommandBuffer);
    HG_MAKE_VULKAN_FUNC(vkEndCommandBuffer);
    HG_MAKE_VULKAN_FUNC(vkResetCommandBuffer);

    HG_MAKE_VULKAN_FUNC(vkCmdCopyBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImage);
    HG_MAKE_VULKAN_FUNC(vkCmdBlitImage);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyBufferToImage);
    HG_MAKE_VULKAN_FUNC(vkCmdCopyImageToBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdPipelineBarrier);
    HG_MAKE_VULKAN_FUNC(vkCmdPipelineBarrier2);

    HG_MAKE_VULKAN_FUNC(vkCmdBeginRendering);
    HG_MAKE_VULKAN_FUNC(vkCmdEndRendering);
    HG_MAKE_VULKAN_FUNC(vkCmdBeginRenderPass);
    HG_MAKE_VULKAN_FUNC(vkCmdEndRenderPass);
    HG_MAKE_VULKAN_FUNC(vkCmdSetViewport);
    HG_MAKE_VULKAN_FUNC(vkCmdSetScissor);
    HG_MAKE_VULKAN_FUNC(vkCmdBindPipeline);
    HG_MAKE_VULKAN_FUNC(vkCmdBindDescriptorSets);
    HG_MAKE_VULKAN_FUNC(vkCmdPushConstants);
    HG_MAKE_VULKAN_FUNC(vkCmdBindVertexBuffers);
    HG_MAKE_VULKAN_FUNC(vkCmdBindIndexBuffer);
    HG_MAKE_VULKAN_FUNC(vkCmdDraw);
    HG_MAKE_VULKAN_FUNC(vkCmdDrawIndexed);
    HG_MAKE_VULKAN_FUNC(vkCmdDispatch);
};

#undef HG_MAKE_VULKAN_FUNC

static VulkanFuncs vulkanFuncs{};

static Library libvulkan{};

#define HG_LOAD_VULKAN_FUNC(name) \
    hg::vulkanFuncs. name = (PFN_##name)hg::vulkanFuncs.vkGetInstanceProcAddr(nullptr, #name); \
    if (hg::vulkanFuncs. name == nullptr) { \
        setError("Could not load " #name); \
        return false; \
    }

static bool loadVulkan()
{
    Maybe<Library> lib = Library::load(
#if defined(HG_PLATFORM_LINUX)
        "libvulkan.so.1"
#elif defined(HG_PLATFORM_WINDOWS)
        "vulkan-1.dll"
#endif
    );
    if (!lib.has)
    {
        setError("Could not load vulkan");
        return false;
    }
    libvulkan = std::move(*lib);

    *(void**)&hg::vulkanFuncs.vkGetInstanceProcAddr = libvulkan.findFunction( "vkGetInstanceProcAddr").orElse(nullptr);
    if (hg::vulkanFuncs.vkGetInstanceProcAddr == nullptr)
    {
        setError("Could not load vkGetInstanceProcAddr\n");
        return false;
    }

    HG_LOAD_VULKAN_FUNC(vkCreateInstance);

    return true;
}

#undef HG_LOAD_VULKAN_FUNC

static void unloadVulkan()
{
    libvulkan = {};
}

#define HG_LOAD_VULKAN_INSTANCE_FUNC(instance, name) \
    hg::vulkanFuncs. name = (PFN_##name)hg::vulkanFuncs.vkGetInstanceProcAddr(instance, #name); \
    if (hg::vulkanFuncs. name == nullptr) { \
        setError("Could not load " #name); \
        return false; \
    }

static bool loadVulkanInstanceFuncs(VkInstance instance)
{
    HG_ASSERT(instance != nullptr);

    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetDeviceProcAddr);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroyInstance);
#ifdef HG_VK_DEBUG_MESSENGER
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkCreateDebugUtilsMessengerEXT);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroyDebugUtilsMessengerEXT);
#endif
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkEnumeratePhysicalDevices);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkEnumerateDeviceExtensionProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceQueueFamilyProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceMemoryProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceMemoryProperties2);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceSupportKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);

    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkDestroySurfaceKHR)
    HG_LOAD_VULKAN_INSTANCE_FUNC(instance, vkCreateDevice)

    return true;
}

#undef HG_LOAD_VULKAN_INSTANCE_FUNC

#define HG_LOAD_VULKAN_DEVICE_FUNC(device, name) \
    hg::vulkanFuncs. name = (PFN_##name)hg::vulkanFuncs.vkGetDeviceProcAddr(device, #name); \
    if (hg::vulkanFuncs. name == nullptr) { \
        setError("Could not load " #name); \
        return false; \
    }

static bool loadVulkanDeviceFuncs(VkDevice device)
{
    HG_ASSERT(device != nullptr);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDevice)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDeviceWaitIdle)

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSwapchainKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySwapchainKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetSwapchainImagesKHR)
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAcquireNextImageKHR)

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSemaphore);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySemaphore);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateFence);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyFence);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetFences);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkWaitForFences);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceQueue);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueueWaitIdle);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueueSubmit);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkQueuePresentKHR);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateCommandPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyCommandPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetCommandPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateCommandBuffers);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeCommandBuffers);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkUpdateDescriptorSets);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateDescriptorSetLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyDescriptorSetLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreatePipelineLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyPipelineLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateShaderModule);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyShaderModule);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateGraphicsPipelines);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateComputePipelines);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyPipeline);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateRenderPass);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyRenderPass);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateFramebuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyFramebuffer);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateImageView);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroyImageView);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCreateSampler);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkDestroySampler);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetBufferMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetBufferMemoryRequirements2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetImageMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetImageMemoryRequirements2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceBufferMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkGetDeviceImageMemoryRequirements);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkAllocateMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFreeMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindBufferMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindBufferMemory2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindImageMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBindImageMemory2);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkMapMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkUnmapMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkFlushMappedMemoryRanges);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkInvalidateMappedMemoryRanges);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkBeginCommandBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkEndCommandBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkResetCommandBuffer);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBlitImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyBufferToImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdCopyImageToBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdPipelineBarrier);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdPipelineBarrier2);

    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBeginRendering);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdEndRendering);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBeginRenderPass);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdEndRenderPass);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdSetViewport);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdSetScissor);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindPipeline);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdPushConstants);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindVertexBuffers);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdBindIndexBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDraw);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDrawIndexed);
    HG_LOAD_VULKAN_DEVICE_FUNC(device, vkCmdDispatch);

    return true;
}

#undef HG_LOAD_VULKAN_DEVICE_FUNC

} // namespace hg

// ── Vulkan PFN stubs (C-linkage) ──
PFN_vkVoidFunction vkGetInstanceProcAddr(
    VkInstance instance,
    const char* pName)
{
    return hg::vulkanFuncs.vkGetInstanceProcAddr(
        instance,
        pName);
}

PFN_vkVoidFunction vkGetDeviceProcAddr(
    VkDevice device,
    const char* pName)
{
    return hg::vulkanFuncs.vkGetDeviceProcAddr(
        device,
        pName);
}

VkResult vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance)
{
    return hg::vulkanFuncs.vkCreateInstance(
        pCreateInfo,
        pAllocator,
        pInstance);
}

void vkDestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyInstance(
        instance,
        pAllocator);
}

VkResult vkCreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger)
{
    return hg::vulkanFuncs.vkCreateDebugUtilsMessengerEXT(
        instance,
        pCreateInfo,
        pAllocator,
        pMessenger);
}

void vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyDebugUtilsMessengerEXT(
        instance,
        messenger,
        pAllocator);
}

VkResult vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pCount,
    VkPhysicalDevice* pDevices)
{
    return hg::vulkanFuncs.vkEnumeratePhysicalDevices(
        instance,
        pCount,
        pDevices);
}

VkResult vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice device,
    const char* pLayerName,
    uint32_t* pCount,
    VkExtensionProperties* pProps)
{
    return hg::vulkanFuncs.vkEnumerateDeviceExtensionProperties(
        device,
        pLayerName,
        pCount,
        pProps);
}

void vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties)
{
    hg::vulkanFuncs.vkGetPhysicalDeviceProperties(
        physicalDevice,
        pProperties);
}

void vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice device,
    uint32_t* pCount,
    VkQueueFamilyProperties* pProps)
{
    hg::vulkanFuncs.vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        pCount,
        pProps);
}

void vkDestroySurfaceKHR(
    VkInstance instance,
    VkSurfaceKHR surface,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroySurfaceKHR(
        instance,
        surface,
        pAllocator);
}

VkResult vkCreateDevice(
    VkPhysicalDevice device,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice)
{
    return hg::vulkanFuncs.vkCreateDevice(
        device,
        pCreateInfo,
        pAllocator,
        pDevice);
}

void vkDestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyDevice(
        device,
        pAllocator);
}

VkResult vkDeviceWaitIdle(
    VkDevice device)
{
    return hg::vulkanFuncs.vkDeviceWaitIdle(
        device);
}

VkResult vkGetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    VkSurfaceKHR surface,
    VkBool32* pSupported)
{
    return hg::vulkanFuncs.vkGetPhysicalDeviceSurfaceSupportKHR(
        physicalDevice,
        queueFamilyIndex,
        surface,
        pSupported);
}

VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    uint32_t* pCount,
    VkSurfaceFormatKHR* pFormats)
{
    return hg::vulkanFuncs.vkGetPhysicalDeviceSurfaceFormatsKHR(
        device,
        surface,
        pCount,
        pFormats);
}

VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    uint32_t* pCount,
    VkPresentModeKHR* pModes)
{
    return hg::vulkanFuncs.vkGetPhysicalDeviceSurfacePresentModesKHR(
        device,
        surface,
        pCount,
        pModes);
}

VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR* pCaps)
{
    return hg::vulkanFuncs.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device,
        surface,
        pCaps);
}

VkResult vkCreateSwapchainKHR(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain)
{
    return hg::vulkanFuncs.vkCreateSwapchainKHR(
        device,
        pCreateInfo,
        pAllocator,
        pSwapchain);
}

void vkDestroySwapchainKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroySwapchainKHR(
        device,
        swapchain,
        pAllocator);
}

VkResult vkGetSwapchainImagesKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t* pCount,
    VkImage* pImages)
{
    return hg::vulkanFuncs.vkGetSwapchainImagesKHR(
        device,
        swapchain,
        pCount,
        pImages);
}

VkResult vkAcquireNextImageKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint64_t timeout,
    VkSemaphore sem,
    VkFence fence,
    uint32_t* pIndex)
{
    return hg::vulkanFuncs.vkAcquireNextImageKHR(
        device,
        swapchain,
        timeout,
        sem,
        fence,
        pIndex);
}

VkResult vkCreateSemaphore(
    VkDevice device,
    const VkSemaphoreCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSemaphore* pSemaphore)
{
    return hg::vulkanFuncs.vkCreateSemaphore(
        device,
        pCreateInfo,
        pAllocator,
        pSemaphore);
}

void vkDestroySemaphore(
    VkDevice device,
    VkSemaphore sem,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroySemaphore(
        device,
        sem,
        pAllocator);
}

VkResult vkCreateFence(
    VkDevice device,
    const VkFenceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkFence* pFence)
{
    return hg::vulkanFuncs.vkCreateFence(
        device,
        pCreateInfo,
        pAllocator,
        pFence);
}

void vkDestroyFence(
    VkDevice device,
    VkFence fence,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyFence(
        device,
        fence,
        pAllocator);
}

VkResult vkResetFences(
    VkDevice device,
    uint32_t count,
    const VkFence* pFences)
{
    return hg::vulkanFuncs.vkResetFences(
        device,
        count,
        pFences);
}

VkResult vkWaitForFences(
    VkDevice device,
    uint32_t count,
    const VkFence* pFences,
    VkBool32 waitAll,
    uint64_t timeout)
{
    return hg::vulkanFuncs.vkWaitForFences(
        device,
        count,
        pFences,
        waitAll,
        timeout);
}

void vkGetDeviceQueue(
    VkDevice device,
    uint32_t family,
    uint32_t index,
    VkQueue* pQueue)
{
    hg::vulkanFuncs.vkGetDeviceQueue(
        device,
        family,
        index,
        pQueue);
}

VkResult vkQueueWaitIdle(
    VkQueue queue)
{
    return hg::vulkanFuncs.vkQueueWaitIdle(
        queue);
}

VkResult vkQueueSubmit(
    VkQueue queue,
    uint32_t count,
    const VkSubmitInfo* pSubmits,
    VkFence fence)
{
    return hg::vulkanFuncs.vkQueueSubmit(
        queue,
        count,
        pSubmits,
        fence);
}

VkResult vkQueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pInfo)
{
    return hg::vulkanFuncs.vkQueuePresentKHR(
        queue,
        pInfo);
}

VkResult vkCreateCommandPool(
    VkDevice device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkCommandPool* pPool)
{
    return hg::vulkanFuncs.vkCreateCommandPool(
        device,
        pCreateInfo,
        pAllocator,
        pPool);
}

void vkDestroyCommandPool(
    VkDevice device,
    VkCommandPool pool,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyCommandPool(
        device,
        pool,
        pAllocator);
}

VkResult vkResetCommandPool(
    VkDevice device,
    VkCommandPool commandPool,
    VkCommandPoolResetFlags flags)
{
    return hg::vulkanFuncs.vkResetCommandPool(
        device,
        commandPool,
        flags);
}

VkResult vkAllocateCommandBuffers(
    VkDevice device,
    const VkCommandBufferAllocateInfo* pInfo,
    VkCommandBuffer* pBufs)
{
    return hg::vulkanFuncs.vkAllocateCommandBuffers(
        device,
        pInfo,
        pBufs);
}

void vkFreeCommandBuffers(
    VkDevice device,
    VkCommandPool pool,
    uint32_t count,
    const VkCommandBuffer* pBufs)
{
    hg::vulkanFuncs.vkFreeCommandBuffers(
        device,
        pool,
        count,
        pBufs);
}

VkResult vkCreateDescriptorPool(
    VkDevice device,
    const VkDescriptorPoolCreateInfo* pInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorPool* pPool)
{
    return hg::vulkanFuncs.vkCreateDescriptorPool(
        device,
        pInfo,
        pAllocator,
        pPool);
}

void vkDestroyDescriptorPool(
    VkDevice device,
    VkDescriptorPool pool,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyDescriptorPool(
        device,
        pool,
        pAllocator);
}

VkResult vkResetDescriptorPool(
    VkDevice device,
    VkDescriptorPool pool,
    uint32_t flags)
{
    return hg::vulkanFuncs.vkResetDescriptorPool(
        device,
        pool,
        flags);
}

VkResult vkAllocateDescriptorSets(
    VkDevice device,
    const VkDescriptorSetAllocateInfo* pInfo,
    VkDescriptorSet* pSets)
{
    return hg::vulkanFuncs.vkAllocateDescriptorSets(
        device,
        pInfo,
        pSets);
}

VkResult vkFreeDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets)
{
    return hg::vulkanFuncs.vkFreeDescriptorSets(
        device,
        descriptorPool,
        descriptorSetCount,
        pDescriptorSets);
}

void vkUpdateDescriptorSets(
    VkDevice device,
    uint32_t writeCount,
    const VkWriteDescriptorSet* pWrites,
    uint32_t copyCount,
    const VkCopyDescriptorSet* pCopies)
{
    hg::vulkanFuncs.vkUpdateDescriptorSets(
        device,
        writeCount,
        pWrites,
        copyCount,
        pCopies);
}

VkResult vkCreateDescriptorSetLayout(
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* pInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorSetLayout* pLayout)
{
    return hg::vulkanFuncs.vkCreateDescriptorSetLayout(
        device,
        pInfo,
        pAllocator,
        pLayout);
}

void vkDestroyDescriptorSetLayout(
    VkDevice device,
    VkDescriptorSetLayout layout,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyDescriptorSetLayout(
        device,
        layout,
        pAllocator);
}

VkResult vkCreatePipelineLayout(
    VkDevice device,
    const VkPipelineLayoutCreateInfo* pInfo,
    const VkAllocationCallbacks* pAllocator,
    VkPipelineLayout* pLayout)
{
    return hg::vulkanFuncs.vkCreatePipelineLayout(
        device,
        pInfo,
        pAllocator,
        pLayout);
}

void vkDestroyPipelineLayout(
    VkDevice device,
    VkPipelineLayout layout,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyPipelineLayout(
        device,
        layout,
        pAllocator);
}

VkResult vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pModule)
{
    return hg::vulkanFuncs.vkCreateShaderModule(
        device,
        pInfo,
        pAllocator,
        pModule);
}

void vkDestroyShaderModule(
    VkDevice device,
    VkShaderModule module,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyShaderModule(
        device,
        module,
        pAllocator);
}

VkResult vkCreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache cache,
    uint32_t count,
    const VkGraphicsPipelineCreateInfo* pInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines)
{
    return hg::vulkanFuncs.vkCreateGraphicsPipelines(
        device,
        cache,
        count,
        pInfos,
        pAllocator,
        pPipelines);
}

VkResult vkCreateComputePipelines(
    VkDevice device,
    VkPipelineCache cache,
    uint32_t count,
    const VkComputePipelineCreateInfo* pInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines)
{
    return hg::vulkanFuncs.vkCreateComputePipelines(
        device,
        cache,
        count,
        pInfos,
        pAllocator,
        pPipelines);
}

void vkDestroyPipeline(
    VkDevice device,
    VkPipeline pipeline,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyPipeline(
        device,
        pipeline,
        pAllocator);
}

VkResult vkCreateRenderPass(
    VkDevice device,
    const VkRenderPassCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkRenderPass* pRenderPass)
{
    return hg::vulkanFuncs.vkCreateRenderPass(
        device,
        pCreateInfo,
        pAllocator,
        pRenderPass);
}

void vkDestroyRenderPass(
    VkDevice device,
    VkRenderPass renderPass,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyRenderPass(
        device,
        renderPass,
        pAllocator);
}

VkResult vkCreateFramebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkFramebuffer* pFramebuffer)
{
    return hg::vulkanFuncs.vkCreateFramebuffer(
        device,
        pCreateInfo,
        pAllocator,
        pFramebuffer);
}

void vkDestroyFramebuffer(
    VkDevice device,
    VkFramebuffer framebuffer,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyFramebuffer(
        device,
        framebuffer,
        pAllocator);
}

VkResult vkCreateBuffer(
    VkDevice device,
    const VkBufferCreateInfo* pInfo,
    const VkAllocationCallbacks* pAllocator,
    VkBuffer* pBuf)
{
    return hg::vulkanFuncs.vkCreateBuffer(
        device,
        pInfo,
        pAllocator,
        pBuf);
}

void vkDestroyBuffer(
    VkDevice device,
    VkBuffer buf,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyBuffer(
        device,
        buf,
        pAllocator);
}

VkResult vkCreateImage(
    VkDevice device,
    const VkImageCreateInfo* pInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImage* pImage)
{
    return hg::vulkanFuncs.vkCreateImage(
        device,
        pInfo,
        pAllocator,
        pImage);
}

void vkDestroyImage(
    VkDevice device,
    VkImage img,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyImage(
        device,
        img,
        pAllocator);
}

VkResult vkCreateImageView(
    VkDevice device,
    const VkImageViewCreateInfo* pInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImageView* pView)
{
    return hg::vulkanFuncs.vkCreateImageView(
        device,
        pInfo,
        pAllocator,
        pView);
}

void vkDestroyImageView(
    VkDevice device,
    VkImageView view,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroyImageView(
        device,
        view,
        pAllocator);
}

VkResult vkCreateSampler(
    VkDevice device,
    const VkSamplerCreateInfo* pInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSampler* pSampler)
{
    return hg::vulkanFuncs.vkCreateSampler(
        device,
        pInfo,
        pAllocator,
        pSampler);
}

void vkDestroySampler(
    VkDevice device,
    VkSampler sampler,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkDestroySampler(
        device,
        sampler,
        pAllocator);
}

void vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    hg::vulkanFuncs.vkGetPhysicalDeviceMemoryProperties(
        physicalDevice,
        pMemoryProperties);
}

void vkGetPhysicalDeviceMemoryProperties2(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2*pMemoryProperties)
{
    hg::vulkanFuncs.vkGetPhysicalDeviceMemoryProperties2(
        physicalDevice,
        pMemoryProperties);
}

void vkGetBufferMemoryRequirements(
    VkDevice device,
    VkBuffer buffer,
    VkMemoryRequirements* pMemoryRequirements)
{
    hg::vulkanFuncs.vkGetBufferMemoryRequirements(
        device,
        buffer,
        pMemoryRequirements);
}

void vkGetBufferMemoryRequirements2(
    VkDevice device,
    const VkBufferMemoryRequirementsInfo2* pInfo,
    VkMemoryRequirements2* pMemoryRequirements)
{
    hg::vulkanFuncs.vkGetBufferMemoryRequirements2(
        device,
        pInfo,
        pMemoryRequirements);
}

void vkGetImageMemoryRequirements(
    VkDevice device,
    VkImage image,
    VkMemoryRequirements* pMemoryRequirements)
{
    hg::vulkanFuncs.vkGetImageMemoryRequirements(
        device,
        image,
        pMemoryRequirements);
}

void vkGetImageMemoryRequirements2(
    VkDevice device,
    const VkImageMemoryRequirementsInfo2* pInfo,
    VkMemoryRequirements2* pMemoryRequirements)
{
    hg::vulkanFuncs.vkGetImageMemoryRequirements2(
        device,
        pInfo,
        pMemoryRequirements);
}

void vkGetDeviceBufferMemoryRequirements(
    VkDevice device,
    const VkDeviceBufferMemoryRequirements* pInfo,
    VkMemoryRequirements2* pMemoryRequirements)
{
    hg::vulkanFuncs.vkGetDeviceBufferMemoryRequirements(
        device,
        pInfo,
        pMemoryRequirements);
}

void vkGetDeviceImageMemoryRequirements(
    VkDevice device,
    const VkDeviceImageMemoryRequirements* pInfo,
    VkMemoryRequirements2* pMemoryRequirements)
{
    hg::vulkanFuncs.vkGetDeviceImageMemoryRequirements(
        device,
        pInfo,
        pMemoryRequirements);
}

VkResult vkAllocateMemory(
    VkDevice device,
    const VkMemoryAllocateInfo* pInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDeviceMemory* pMemory)
{
    return hg::vulkanFuncs.vkAllocateMemory(
        device,
        pInfo,
        pAllocator,
        pMemory);
}

void vkFreeMemory(
    VkDevice device,
    VkDeviceMemory mem,
    const VkAllocationCallbacks* pAllocator)
{
    hg::vulkanFuncs.vkFreeMemory(
        device,
        mem,
        pAllocator);
}

VkResult vkBindBufferMemory(
    VkDevice device,
    VkBuffer buf,
    VkDeviceMemory mem,
    VkDeviceSize offset)
{
    return hg::vulkanFuncs.vkBindBufferMemory(
        device,
        buf,
        mem,
        offset);
}

VkResult vkBindBufferMemory2(
    VkDevice device,
    uint32_t bindInfoCount,
    const VkBindBufferMemoryInfo* pBindInfos)
{
    return hg::vulkanFuncs.vkBindBufferMemory2(
        device,
        bindInfoCount,
        pBindInfos);
}

VkResult vkBindImageMemory(
    VkDevice device,
    VkImage img,
    VkDeviceMemory mem,
    VkDeviceSize offset)
{
    return hg::vulkanFuncs.vkBindImageMemory(
        device,
        img,
        mem,
        offset);
}

VkResult vkBindImageMemory2(
    VkDevice device,
    uint32_t bindInfoCount,
    const VkBindImageMemoryInfo* pBindInfos)
{
    return hg::vulkanFuncs.vkBindImageMemory2(
        device,
        bindInfoCount,
        pBindInfos);
}

VkResult vkMapMemory(
    VkDevice device,
    VkDeviceMemory mem,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData)
{
    return hg::vulkanFuncs.vkMapMemory(
        device,
        mem,
        offset, size, flags, ppData);
}

void vkUnmapMemory(
    VkDevice device,
    VkDeviceMemory mem)
{
    hg::vulkanFuncs.vkUnmapMemory(
        device,
        mem);
}

VkResult vkFlushMappedMemoryRanges(
    VkDevice device,
    uint32_t count,
    const VkMappedMemoryRange* pRanges)
{
    return hg::vulkanFuncs.vkFlushMappedMemoryRanges(
        device,
        count,
        pRanges);
}

VkResult vkInvalidateMappedMemoryRanges(
    VkDevice device,
    uint32_t count,
    const VkMappedMemoryRange* pRanges)
{
    return hg::vulkanFuncs.vkInvalidateMappedMemoryRanges(
        device,
        count,
        pRanges);
}

VkResult vkBeginCommandBuffer(
    VkCommandBuffer cmd,
    const VkCommandBufferBeginInfo* pInfo)
{
    return hg::vulkanFuncs.vkBeginCommandBuffer(
        cmd,
        pInfo);
}

VkResult vkEndCommandBuffer(
    VkCommandBuffer cmd)
{
    return hg::vulkanFuncs.vkEndCommandBuffer(
        cmd);
}

VkResult vkResetCommandBuffer(
    VkCommandBuffer cmd,
    VkCommandBufferResetFlags flags)
{
    return hg::vulkanFuncs.vkResetCommandBuffer(
        cmd,
        flags);
}

void vkCmdCopyBuffer(
    VkCommandBuffer cmd,
    VkBuffer src,
    VkBuffer dst,
    uint32_t count,
    const VkBufferCopy* pRegions)
{
    hg::vulkanFuncs.vkCmdCopyBuffer(
        cmd,
        src,
        dst,
        count,
        pRegions);
}

void vkCmdCopyImage(
    VkCommandBuffer cmd,
    VkImage src,
    VkImageLayout srcLayout,
    VkImage dst,
    VkImageLayout dstLayout,
    uint32_t count,
    const VkImageCopy* pRegions)
{
    hg::vulkanFuncs.vkCmdCopyImage(
        cmd,
        src,
        srcLayout,
        dst,
        dstLayout,
        count,
        pRegions);
}

void vkCmdBlitImage(
    VkCommandBuffer cmd,
    VkImage src,
    VkImageLayout srcLayout,
    VkImage dst,
    VkImageLayout dstLayout,
    uint32_t count,
    const VkImageBlit* pRegions,
    VkFilter filter)
{
    hg::vulkanFuncs.vkCmdBlitImage(
        cmd,
        src,
        srcLayout,
        dst,
        dstLayout,
        count,
        pRegions,
        filter);
}

void vkCmdCopyBufferToImage(
    VkCommandBuffer cmd,
    VkBuffer src,
    VkImage dst,
    VkImageLayout dstLayout,
    uint32_t count,
    const VkBufferImageCopy* pRegions)
{
    hg::vulkanFuncs.vkCmdCopyBufferToImage(
        cmd,
        src,
        dst,
        dstLayout,
        count,
        pRegions);
}

void vkCmdCopyImageToBuffer(
    VkCommandBuffer cmd,
    VkImage src,
    VkImageLayout srcLayout,
    VkBuffer dst,
    uint32_t count,
    const VkBufferImageCopy* pRegions)
{
    hg::vulkanFuncs.vkCmdCopyImageToBuffer(
        cmd,
        src,
        srcLayout,
        dst,
        count,
        pRegions);
}

void vkCmdPipelineBarrier2(
    VkCommandBuffer cmd,
    const VkDependencyInfo* pInfo)
{
    hg::vulkanFuncs.vkCmdPipelineBarrier2(
        cmd,
        pInfo);
}

void vkCmdPipelineBarrier(
    VkCommandBuffer commandBuffer,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags,
    uint32_t memoryBarrierCount,
    const VkMemoryBarrier* pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    hg::vulkanFuncs.vkCmdPipelineBarrier(
        commandBuffer,
        srcStageMask,
        dstStageMask,
        dependencyFlags,
        memoryBarrierCount,
        pMemoryBarriers,
        bufferMemoryBarrierCount,
        pBufferMemoryBarriers,
        imageMemoryBarrierCount,
        pImageMemoryBarriers);
}

void vkCmdBeginRendering(
    VkCommandBuffer cmd,
    const VkRenderingInfo* pInfo)
{
    hg::vulkanFuncs.vkCmdBeginRendering(
        cmd,
        pInfo);
}

void vkCmdEndRendering(
    VkCommandBuffer cmd)
{
    hg::vulkanFuncs.vkCmdEndRendering(
        cmd);
}

void vkCmdBeginRenderPass(
    VkCommandBuffer cmd,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkSubpassContents contents)
{
    hg::vulkanFuncs.vkCmdBeginRenderPass(
        cmd,
        pRenderPassBegin,
        contents);
}

void vkCmdEndRenderPass(
    VkCommandBuffer cmd)
{
    hg::vulkanFuncs.vkCmdEndRenderPass(
        cmd);
}

void vkCmdSetViewport(
    VkCommandBuffer cmd,
    uint32_t first,
    uint32_t count,
    const VkViewport* pViewports)
{
    hg::vulkanFuncs.vkCmdSetViewport(
        cmd,
        first,
        count,
        pViewports);
}

void vkCmdSetScissor(
    VkCommandBuffer cmd,
    uint32_t first,
    uint32_t count,
    const VkRect2D* pScissors)
{
    hg::vulkanFuncs.vkCmdSetScissor(
        cmd,
        first,
        count,
        pScissors);
}

void vkCmdBindPipeline(
    VkCommandBuffer cmd,
    VkPipelineBindPoint bindPoint,
    VkPipeline pipeline)
{
    hg::vulkanFuncs.vkCmdBindPipeline(
        cmd,
        bindPoint,
        pipeline);
}

void vkCmdBindDescriptorSets(
    VkCommandBuffer cmd,
    VkPipelineBindPoint bindPoint,
    VkPipelineLayout layout,
    uint32_t firstSet,
    uint32_t count,
    const VkDescriptorSet* pSets,
    uint32_t dynCount,
    const uint32_t* pDyn)
{
    hg::vulkanFuncs.vkCmdBindDescriptorSets(
        cmd,
        bindPoint,
        layout,
        firstSet,
        count,
        pSets,
        dynCount,
        pDyn);
}

void vkCmdPushConstants(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    VkShaderStageFlags stages,
    uint32_t offset,
    uint32_t size,
    const void* pData)
{
    hg::vulkanFuncs.vkCmdPushConstants(
        cmd,
        layout,
        stages,
        offset,
        size,
        pData);
}

void vkCmdBindVertexBuffers(
    VkCommandBuffer cmd,
    uint32_t first,
    uint32_t count,
    const VkBuffer* pBufs,
    const VkDeviceSize* pOffsets)
{
    hg::vulkanFuncs.vkCmdBindVertexBuffers(
        cmd,
        first,
        count,
        pBufs,
        pOffsets);
}

void vkCmdBindIndexBuffer(
    VkCommandBuffer cmd,
    VkBuffer buf,
    VkDeviceSize offset,
    VkIndexType type)
{
    hg::vulkanFuncs.vkCmdBindIndexBuffer(
        cmd,
        buf,
        offset,
        type);
}

void vkCmdDraw(
    VkCommandBuffer cmd,
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t firstVertex,
    uint32_t firstInstance)
{
    hg::vulkanFuncs.vkCmdDraw(
        cmd,
        vertexCount,
        instanceCount,
        firstVertex,
        firstInstance);
}

void vkCmdDrawIndexed(
    VkCommandBuffer cmd,
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t firstIndex,
    int32_t vertexOffset,
    uint32_t firstInstance)
{
    hg::vulkanFuncs.vkCmdDrawIndexed(
        cmd,
        indexCount,
        instanceCount,
        firstIndex,
        vertexOffset,
        firstInstance);
}

void vkCmdDispatch(
    VkCommandBuffer cmd,
    uint32_t x,
    uint32_t y,
    uint32_t z)
{
    hg::vulkanFuncs.vkCmdDispatch(
        cmd,
        x,
        y,
        z);
}

namespace hg {

#if defined(HG_PLATFORM_LINUX)

#include <dlfcn.h>

Maybe<Library> Library::load(StringView path)
{
    ArenaScope scratch = getScratch();
    char* cstr = cString(scratch, path);

    Maybe<Library> lib = some<Library>();
    lib->lib = dlopen(cstr, RTLD_LAZY);
    if (lib->lib == nullptr)
    {
        setError("Could not load dynamic library \"%s\": %s", cstr, dlerror());
        return {};
    }

    return lib;
}

Library::~Library() noexcept
{
    if (lib != nullptr)
        dlclose(lib);
}

Maybe<void*> Library::findFunction(StringView name)
{
    ArenaScope scratch = getScratch();
    char* cstr = cString(scratch, name);

    void* fn = dlsym(lib, cstr);
    if (fn == nullptr)
    {
        setError("Could not load function symbol \"%s\": %s", cstr, dlerror());
        return {};
    }

    return some<void*>(fn);
}

#elif defined(HG_PLATFORM_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

Maybe<Library> Library::load(StringView path)
{
    ArenaScope scratch = getScratch();
    char* cstr = cString(scratch, path);

    Maybe<Library> lib = some<Library>();
    lib->lib = static_cast<void*>(LoadLibraryA(cstr));
    if (lib == nullptr)
    {
        setError("Could not load dynamic library \"%s\"", cstr);
        return {};
    }

    return lib;
}

Library::~Library() noexcept
{
    if (lib != nullptr)
        FreeLibrary(static_cast<HMODULE>(lib));
}

Maybe<void*> Library::findFunction(StringView name)
{
    ArenaScope scratch = getScratch();
    char* cstr = cString(scratch, name);

    void* fn = GetProcAddress(reinterpret_cast<HMODULE>(lib), cstr);
    if (fn == nullptr)
    {
        setError("Could not load function symbol \"%s\"", cstr);
        return {};
    }

    return some<void*>(fn);
}

#endif

} // namespace hg
