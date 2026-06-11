#include "hg_audio.hpp"
#include "hg_containers.hpp"
#include "hg_core.hpp"
#include "hg_gpu.hpp"
#include "hg_library.hpp"
#include "hg_memory.hpp"
#include "hg_platform.hpp"
#include "hg_rendering.hpp"
#include "hg_strings.hpp"
#include "hg_templates.hpp"
#include "hg_utils.hpp"
#include "hg_window.hpp"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "SDL3/SDL.h"
#include <SDL3/SDL_vulkan.h>

#include "imgui_impl_vulkan.h"
#include "imgui_impl_sdl3.h"

void hgPlatformInit()
{
    SDL_Init(
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_GAMEPAD |
        SDL_INIT_EVENTS);
}

void hgPlatformDeinit()
{
    SDL_Quit();
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
            hgWarn("Unrecognized Vulkan format\n");
            return 0;
    }
}

static VkFormat formatToVk(HgFormat format)
{
    return (VkFormat)format;
}

u32 hgFormatToSize(HgFormat format)
{
    return vkFormatToSize(formatToVk(format));
}

static VkPipelineStageFlags gpuStageToVk(HgGpuStageFlags stage)
{
    return (VkPipelineStageFlags)stage;
}

static VkAccessFlags gpuAccessToVk(HgGpuAccessFlags access)
{
    return (VkAccessFlags)access;
}

typedef HgHandle Descriptor;

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
            hgError("invalid HgGpuDescriptorType: %d", type);
    }
}

struct HgGpuBuffer {
    VkBuffer buffer;
    VmaAllocation alloc;
    u64 size;
    Descriptor uniformDesc;
    Descriptor storageDesc;
    HgGpuMemoryHostAccess access;
    HgGpuStageFlags lastStage;
    HgGpuAccessFlags lastAccess;
};

static VkBufferUsageFlags gpuBufferUsageToVk(HgGpuBufferUsageFlags usage)
{
    return (VkBufferUsageFlags)usage;
}

static VmaAllocationCreateFlags gpuMemoryUsageToVma(HgGpuMemoryUsage usage)
{
    switch (usage)
    {
        case HgGpuMemoryUsage_deviceOnly:
            return 0;
        case HgGpuMemoryUsage_stagingWrite:
            return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        case HgGpuMemoryUsage_stagingRead:
            return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        case HgGpuMemoryUsage_frequentUpdate:
            return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                   VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        default:
            hgError("Invalid HgGpuMemoryUsage: %d\n", usage);
    }
}

static HgGpuMemoryHostAccess gpuMemoryUsageToHostAccess(HgGpuMemoryUsage usage)
{
    switch (usage)
    {
        case HgGpuMemoryUsage_deviceOnly:
            return HgGpuMemoryHostAccess_none;
        case HgGpuMemoryUsage_stagingWrite:
            return HgGpuMemoryHostAccess_write;
        case HgGpuMemoryUsage_stagingRead:
            return HgGpuMemoryHostAccess_read;
        case HgGpuMemoryUsage_frequentUpdate:
            return HgGpuMemoryHostAccess_none;
        default:
            hgError("Invalid HgGpuMemoryUsage: %d\n", usage);
    }
}

struct HgGpuImage {
    VkImage image;
    VmaAllocation alloc;
    HgGpuImageUsageFlags usage;
    HgFormat format;
    u32 width;
    u32 height;
    u32 depth;
    u8 dimensions;
    u8 mipLevels;
    u8 arrayLayers;
    u8 msaaSamples;
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
            hgError("Invalid image dimensions: %d\n", dimensions);
    }
}

static VkImageUsageFlags gpuImageUsageToVk(HgGpuImageUsageFlags usage)
{
    return (VkImageUsageFlags)usage;
}

static VkImageLayout gpuLayoutToVk(HgGpuLayout layout)
{
    return (VkImageLayout)layout;
}

static VkImageCreateFlags gpuImageConfigFlagsToVk(HgGpuImageConfigFlags flags)
{
    return (VkImageCreateFlags)flags;
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
            hgError("Invalid msaa sample count\n");
    }
}

struct SamplerInfo {
    HgGpuFilter filter;
    HgGpuSamplerEdgeMode mode;
    HgGpuSamplerBorder border;
};

template<>
constexpr u64 hgHash(SamplerInfo info)
{
    return info.border + (info.mode << 4) + (info.filter << 8);
}

constexpr bool operator==(const SamplerInfo& lhs, const SamplerInfo& rhs)
{
    return lhs.filter == rhs.filter
        && lhs.mode == rhs.mode
        && lhs.border == rhs.border;
}

struct HgGpuView {
    HgGpuImage* image;
    VkImageView view;
    VkSampler sampler;
    Descriptor samplerDesc;
    Descriptor storageDesc;
    HgGpuViewType type;
    HgGpuAspectFlags aspectFlags;
    u8 baseMipLevel;
    u8 levelCount;
    u8 baseArrayLayer;
    u8 layerCount;
    HgGpuStageFlags lastStage;
    HgGpuAccessFlags lastAccess;
    HgGpuLayout lastLayout;
};

static HgGpuViewType imageDimensionsToHgView(u32 dimensions)
{
    switch (dimensions)
    {
        case 1:
            return HgGpuViewType_1D;
        case 2:
            return HgGpuViewType_2D;
        case 3:
            return HgGpuViewType_3D;
        default:
            hgError("Invalid image dimensions: %d\n", dimensions);
    }
}

static VkImageViewType gpuViewTypeToVk(HgGpuViewType type)
{
    return (VkImageViewType)type;
}

static VkImageAspectFlags gpuAspectToVk(HgGpuAspectFlags aspect)
{
    return (VkImageAspectFlags)aspect;
}

static VkFilter gpuFilterToVk(HgGpuFilter filter)
{
    return (VkFilter)filter;
}

static VkSamplerAddressMode gpuSamplerAddressModeToVk(HgGpuSamplerEdgeMode mode)
{
    return (VkSamplerAddressMode)mode;
}

static VkBorderColor gpuSamplerBorderToVk(HgGpuSamplerBorder color)
{
    return (VkBorderColor)color;
}

struct HgGpuPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkPipelineBindPoint bindPoint;
};

static VkPrimitiveTopology gpuTopologyToVk(HgGpuTopology topology)
{
    return (VkPrimitiveTopology)topology;
}

static VkPolygonMode gpuPolygonModeToVk(HgGpuPolygonMode mode)
{
    return (VkPolygonMode)mode;
}

static VkCullModeFlags gpuCullModeToVk(HgGpuCullFlags mode)
{
    return (VkCullModeFlags)mode;
}

struct HgWindow {
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    HgArray<HgGpuImage> images;
    HgArray<HgGpuView> views;
    HgArray<VkSemaphore> imageAvailable;
    HgArray<VkSemaphore> readyToPresent;
    u32 imageIdx;
    HgFormat format;
    HgGpuImageUsageFlags imageUsage;
    HgGpuPresentMode presentMode;

    SDL_Window* sdlWindow;
    u32 width;
    u32 height;
    f32 mouseX;
    f32 mouseY;
    bool isKeyDown[HgButton_count]{};
    bool wasClosed = false;

    HgArray<HgWindowEvent> events;
};

struct Frame {
    HgArray<HgWindow*> windows;
    VkCommandPool cmdPool;
    VkFence fence;
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

    HgHandlePool descriptorPools[DescriptorType_count];

    HgPool<HgGpuBuffer> buffers;
    HgPool<HgGpuImage> images;
    HgPool<HgGpuView> views;
    HgPool<HgGpuPipeline> pipelines;

    HgMap<SamplerInfo, VkSampler> samplers;

    Frame* frames = nullptr;
    u32 frameCount = 0;
    u32 currentFrame = 0;
};

static VulkanState vk{};

static void loadVulkan();
static void unloadVulkan();

static void loadVulkanInstanceFuncs(VkInstance instance);
static void loadVulkanDeviceFuncs(VkDevice device);

#ifdef HG_VK_DEBUG_MESSENGER

static VkBool32 debugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    (void)type;
    (void)userData;

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        (void)fprintf(stderr, "Vulkan Error: %s\n", callbackData->pMessage);
        abort();
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        (void)fprintf(stderr, "Vulkan Warning: %s\n", callbackData->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        (void)fprintf(stderr, "Vulkan Info: %s\n", callbackData->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        (void)fprintf(stderr, "Vulkan Verbose: %s\n", callbackData->pMessage);
    } else {
        (void)fprintf(stderr, "Vulkan Unknown: %s\n", callbackData->pMessage);
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

static VkInstance createInstance(HgStringView* extensions, u32 extensionCount)
{
    if (extensionCount > 0)
        hgAssert(extensions != nullptr);

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

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
    instanceInfo.enabledLayerCount = (u32)hgArrayCount(layers);
    instanceInfo.ppEnabledLayerNames = layers;
#endif

    const char** extCStrs = hgArenaAlloc<const char*>(scratch, extensionCount);
    for (u32 i = 0; i < extensionCount; ++i)
    {
        extCStrs[i] = hgCString(scratch, extensions[i]);
    }
    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = extCStrs;

    VkInstance instance = nullptr;
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
    if (instance == nullptr)
        hgError("Failed to create Vulkan instance: %s\n", vkResultToStr(result));

    return instance;
}

#ifdef HG_VK_DEBUG_MESSENGER
static VkDebugUtilsMessengerEXT createDebugUtilsMessenger()
{
    hgAssert(vk.instance != nullptr);

    VkDebugUtilsMessengerEXT messenger = nullptr;
    VkResult result = vkCreateDebugUtilsMessengerEXT(
        vk.instance, &debugUtilsMessengerInfo, nullptr, &messenger);
    if (messenger == nullptr)
        hgError("Failed to create Vulkan debug messenger: %s\n", vkResultToStr(result));

    return messenger;
}
#endif

static bool findQueueFamily(VkPhysicalDevice gpu, u32* queueFamily, VkQueueFlags queueFlags)
{
    hgAssert(gpu != nullptr);
    hgAssert(queueFamily != nullptr);

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    u32 familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
    VkQueueFamilyProperties* families = hgArenaAlloc<VkQueueFamilyProperties>(scratch, familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families);

    for (u32 i = 0; i < familyCount; ++i)
    {
        if (families[i].queueFlags & queueFlags)
        {
            *queueFamily = i;
            return true;
        }
    }
    return false;
}

static const char* const deviceExtensions[]{
    "VK_KHR_swapchain",
};

static VkPhysicalDevice findPhysicalDevice()
{
    hgAssert(vk.instance != nullptr);

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    u32 gpuCount;
    vkEnumeratePhysicalDevices(vk.instance, &gpuCount, nullptr);
    VkPhysicalDevice* gpus = hgArenaAlloc<VkPhysicalDevice>(scratch, gpuCount);
    vkEnumeratePhysicalDevices(vk.instance, &gpuCount, gpus);

    VkExtensionProperties* extProps = nullptr;
    u32 extPropCount = 0;

    for (u32 i = 0; i < gpuCount; ++i)
    {
        VkPhysicalDevice gpu = gpus[i];
        u32 family;

        u32 newPropCount = 0;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &newPropCount, nullptr);
        if (newPropCount > extPropCount)
        {
            extProps = hgArenaRealloc(scratch, extProps, extPropCount, newPropCount);
            extPropCount = newPropCount;
        }
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &newPropCount, extProps);

        for (u32 j = 0; j < (u32)hgArrayCount(deviceExtensions); j++)
        {
            for (u32 k = 0; k < newPropCount; k++)
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

    hgWarn("Could not find a suitable gpu\n");
    return nullptr;
}

static VkDevice createDevice()
{
    hgAssert(vk.physicalDevice != nullptr);
    hgAssert(vk.queueFamily != (u32)-1);

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
    deviceInfo.enabledExtensionCount = (u32)hgArrayCount(deviceExtensions);
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceInfo.pEnabledFeatures = &features;

    VkDevice device = nullptr;
    VkResult result = vkCreateDevice(vk.physicalDevice, &deviceInfo, nullptr, &device);

    if (device == nullptr)
        hgError("Could not create Vulkan device: %s\n", vkResultToStr(result));
    return device;
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
    info.poolSizeCount = (u32)hgArrayCount(sizes);
    info.pPoolSizes = sizes;

    VkDescriptorPool pool = nullptr;
    VkResult result = vkCreateDescriptorPool(vk.device, &info, nullptr, &pool);
    if (pool == nullptr)
        hgError("Could not create VkDescriptorPool: %s\n", vkResultToStr(result));

    return pool;
}

static VkDescriptorSetLayout createBindlessDescriptorLayout()
{
    VkDescriptorSetLayoutBinding bindings[DescriptorType_count]{};
    VkDescriptorBindingFlags flags[DescriptorType_count]{};
    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = descriptorTypeToVk((DescriptorType)i);
        bindings[i].descriptorCount = UINT16_MAX;
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
    flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagsInfo.bindingCount = (u32)hgArrayCount(bindings);
    flagsInfo.pBindingFlags = flags;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = &flagsInfo;
    info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    info.bindingCount = (u32)hgArrayCount(bindings);
    info.pBindings = bindings;

    VkDescriptorSetLayout layout = nullptr;
    VkResult result = vkCreateDescriptorSetLayout(vk.device, &info, nullptr, &layout);
    if (layout == nullptr)
        hgError("Could not create bindless VkDescriptorSetLayout: %s\n", vkResultToStr(result));

    return layout;
}

static Frame createFrame()
{
    Frame frame{};

    frame.windows = hgArrayCreate<HgWindow*>(0, 8);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = vk.queueFamily;

    VkResult poolResult = vkCreateCommandPool(vk.device, &poolInfo, nullptr, &frame.cmdPool);
    if (frame.cmdPool == nullptr)
        hgError("Could not create Vulkan command pool: %s\n", vkResultToStr(poolResult));

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult fenceResult = vkCreateFence(vk.device, &fenceInfo, nullptr, &frame.fence);
    if (frame.fence == nullptr)
        hgError("Could not create Vulkan fence: %s\n", vkResultToStr(fenceResult));

    return frame;
}

void hgGpuInit()
{
    loadVulkan();

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    {
        HgStringView* exts;
        u32 extCount = hgPlatformGetVulkanExtensions(scratch, &exts);
#ifdef HG_VK_DEBUG_MESSENGER
        exts = hgArenaRealloc(scratch, exts, extCount, extCount + 1);
        exts[extCount++] = "VK_EXT_debug_utils";
#endif
        vk.instance = createInstance(exts, extCount);
    }
    loadVulkanInstanceFuncs(vk.instance);

#ifdef HG_VK_DEBUG_MESSENGER
    vk.debugMessenger = createDebugUtilsMessenger();
#endif

    vk.physicalDevice = findPhysicalDevice();
    findQueueFamily(vk.physicalDevice, &vk.queueFamily,
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT);

    vk.device = createDevice();
    loadVulkanDeviceFuncs(vk.device);
    vkGetDeviceQueue(vk.device, vk.queueFamily, 0, &vk.queue);

    {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = vk.physicalDevice;
        allocatorInfo.device = vk.device;
        allocatorInfo.instance = vk.instance;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        VkResult result = vmaCreateAllocator(&allocatorInfo, &vk.vma);
        if (vk.vma == nullptr)
            hgError("Could not create Vulkan memory allocator: %s\n", vkResultToStr(result));
    }

    {
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolInfo.queueFamilyIndex = vk.queueFamily;

        VkResult result = vkCreateCommandPool(vk.device, &cmdPoolInfo, nullptr, &vk.cmdPool);
        if (vk.cmdPool == nullptr)
            hgError("Could not create Vulkan command pool: %s\n", vkResultToStr(result));
    }

    vk.bindlessPool = createBindlessDescriptorPool();
    vk.bindlessLayout = createBindlessDescriptorLayout();
    {
        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = vk.bindlessPool;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &vk.bindlessLayout;

        VkResult result = vkAllocateDescriptorSets(vk.device, &info, &vk.bindlessSet);
        if (vk.bindlessSet == nullptr)
            hgError("Could not allocate bindless VkDescriptorSet: %s\n", vkResultToStr(result));
    }

    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        vk.descriptorPools[i] = hgHandlePoolCreate();
    }

    vk.buffers = hgPoolCreate<HgGpuBuffer>();
    vk.images = hgPoolCreate<HgGpuImage>();
    vk.views = hgPoolCreate<HgGpuView>();
    vk.pipelines = hgPoolCreate<HgGpuPipeline>();

    vk.samplers = hgMapCreate<SamplerInfo, VkSampler>(
        2 *
        HgGpuFilter_count *
        HgGpuSamplerEdgeMode_count *
        HgGpuSamplerBorder_count);

    vk.frameCount = 2;
    vk.currentFrame = 0;
    vk.frames = hgGpaAlloc<Frame>(vk.frameCount);
    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        vk.frames[i] = createFrame();
    }
}

void hgGpuDeinit()
{
    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        vkDestroyFence(vk.device, vk.frames[i].fence, nullptr);
        vkDestroyCommandPool(vk.device, vk.frames[i].cmdPool, nullptr);
        hgArrayDestroy(&vk.frames[i].windows);
    }
    hgGpaFree(vk.frames, vk.frameCount);

    hgMapForEach(&vk.samplers, [](SamplerInfo*, VkSampler* sampler)
    {
        vkDestroySampler(vk.device, *sampler, nullptr);
    });
    hgMapDestroy(&vk.samplers);

    hgPoolDestroy(&vk.pipelines);
    hgPoolDestroy(&vk.views);
    hgPoolDestroy(&vk.images);
    hgPoolDestroy(&vk.buffers);

    for (u32 i = 0; i < DescriptorType_count; ++i)
    {
        hgHandlePoolDestroy(&vk.descriptorPools[i]);
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

void hgGpuWaitIdle()
{
    vkQueueWaitIdle(vk.queue);
}

// static u32 findMemoryTypeIndex(
//     u32 bitmask,
//     VkMemoryPropertyFlags preferredFlags,
//     VkMemoryPropertyFlags unpreferredFlags)
// {
//     hgAssert(vkState.hgVkPhysicalDevice != nullptr);
//     hgAssert(bitmask != 0);
//
//     VkPhysicalDeviceMemoryProperties memProps;
//     vkGetPhysicalDeviceMemoryProperties(vkState.hgVkPhysicalDevice, &memProps);
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
//             hgWarn("Could not find Vulkan memory type without undesired flags\n");
//             return i;
//         }
//     }
//     for (u32 i = 0; i < memProps.memoryTypeCount; ++i)
//     {
//         if ((bitmask & (1 << i)) != 0)
//         {
//             hgWarn("Could not find Vulkan memory type with desired flags\n");
//             return i;
//         }
//     }
//     hgError("Could not find Vulkan memory type in bitmask: %x\n", bitmask);
// }

struct DescriptorBufferInfo {
    HgGpuBuffer* buffer;
    u64 offset;
    u64 range;
};

struct DescriptorImageInfo {
    HgGpuView* imageView;
    HgGpuLayout imageLayout;
};

static Descriptor descriptorCreate(
    DescriptorType type,
    const DescriptorBufferInfo* bufferInfo,
    const DescriptorImageInfo* imageInfo)
{
    hgAssert(type < DescriptorType_count);

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    Descriptor desc = hgHandlePoolAlloc(&vk.descriptorPools[type]);

    VkDescriptorBufferInfo bufferVkInfo = bufferInfo != nullptr
        ? VkDescriptorBufferInfo{bufferInfo->buffer->buffer, bufferInfo->offset, bufferInfo->range}
        : VkDescriptorBufferInfo{};

    VkDescriptorImageInfo imageVkInfo = imageInfo != nullptr
        ? VkDescriptorImageInfo{
            imageInfo->imageView->sampler,
            imageInfo->imageView->view,
            gpuLayoutToVk(imageInfo->imageLayout)
        }
        : VkDescriptorImageInfo{};

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vk.bindlessSet;
    write.dstBinding = type;
    write.dstArrayElement = hgHandleIdx(desc);
    write.descriptorCount = 1;
    write.descriptorType = descriptorTypeToVk(type);
    write.pBufferInfo = bufferInfo != nullptr ? &bufferVkInfo : nullptr;
    write.pImageInfo = imageInfo != nullptr ? &imageVkInfo : nullptr;
    write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk.device, 1, &write, 0, nullptr);

    return desc;
}

static void descriptorDestroy(Descriptor desc, DescriptorType type)
{
    if (desc != hgHandleNull)
    {
        hgHandlePoolFree(&vk.descriptorPools[type], desc);
    }
}

HgGpuBuffer* hgGpuBufferCreate(
    u64 size,
    HgGpuBufferUsageFlags usageFlags,
    HgGpuMemoryUsage memoryUsage)
{
    hgAssert(size > 0);
    hgAssert(usageFlags != 0);

    HgGpuBuffer* buffer = hgPoolAlloc(&vk.buffers);
    *buffer = {};

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
        &buffer->buffer,
        &buffer->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        hgError("Could not create VkBuffer: %s\n", vkResultToStr(result));

    if (usageFlags & HgGpuBufferUsage_uniformBuffer)
    {
        DescriptorBufferInfo descInfo{buffer, 0, size};
        buffer->uniformDesc = descriptorCreate(DescriptorType_uniformBuffer, &descInfo, nullptr);
    }

    if (usageFlags & HgGpuBufferUsage_storageBuffer)
    {
        DescriptorBufferInfo descInfo{buffer, 0, size};
        buffer->storageDesc = descriptorCreate(DescriptorType_storageBuffer, &descInfo, nullptr);
    }

    buffer->size = size;

    if (memoryUsage == HgGpuMemoryUsage_frequentUpdate)
    {
        VkMemoryPropertyFlags memPropFlags;
        vmaGetAllocationMemoryProperties(vk.vma, buffer->alloc, &memPropFlags);
        buffer->access = memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            ? HgGpuMemoryHostAccess_write
            : HgGpuMemoryHostAccess_none;
    } else {
        buffer->access = gpuMemoryUsageToHostAccess(memoryUsage);
    }

    return buffer;
}

void hgGpuBufferDestroy(HgGpuBuffer* buffer)
{
    if (buffer != nullptr)
    {
        descriptorDestroy(buffer->storageDesc, DescriptorType_storageBuffer);
        descriptorDestroy(buffer->uniformDesc, DescriptorType_uniformBuffer);
        vmaDestroyBuffer(vk.vma, buffer->buffer, buffer->alloc);
        hgPoolFree(&vk.buffers, buffer);
    }
}

u32 hgGpuBufferUniformDescriptor(HgGpuBuffer* buffer)
{
    Descriptor desc = buffer->uniformDesc;
    hgAssert(hgHandlePoolAlive(&vk.descriptorPools[DescriptorType_uniformBuffer], desc));
    return hgHandleIdx(desc);
}

u32 hgGpuBufferStorageDescriptor(HgGpuBuffer* buffer)
{
    Descriptor desc = buffer->storageDesc;
    hgAssert(hgHandlePoolAlive(&vk.descriptorPools[DescriptorType_storageBuffer], desc));
    return hgHandleIdx(desc);
}

void hgGpuBufferWrite(HgGpuBuffer* dst, u64 offset, const void* src, u64 size)
{
    if (size == 0)
        return;

    hgAssert(dst != nullptr);
    hgAssert(src != nullptr);

    if (dst->access & HgGpuMemoryHostAccess_write)
    {
        VkResult result = vmaCopyMemoryToAllocation(vk.vma, src, dst->alloc, offset, size);
        if (result != VK_SUCCESS)
            hgError("Could not write gpu buffer: %s\n", vkResultToStr(result));
        return;
    }

    HgGpuBuffer* stage = hgGpuBufferCreate(size, HgGpuBufferUsage_transferSrc, HgGpuMemoryUsage_stagingWrite);
    hgDefer(hgGpuBufferDestroy(stage));
    hgGpuBufferWrite(stage, 0, src, size);

    HgGpuCmd* cmd = hgGpuCmdBegin();

    VkBufferCopy region{};
    region.dstOffset = offset;
    region.size = size;

    vkCmdCopyBuffer((VkCommandBuffer)cmd, stage->buffer, dst->buffer, 1, &region);

    hgGpuCmdEnd(cmd);

    dst->lastStage = HgGpuStage_transfer;
    dst->lastAccess = HgGpuAccess_transferWrite;
}

void hgGpuBufferRead(void* dst, HgGpuBuffer* src, u64 offset, u64 size)
{
    if (size == 0)
        return;

    hgAssert(dst != nullptr);
    hgAssert(src != nullptr);

    if (src->access & HgGpuMemoryHostAccess_read)
    {
        VkResult result = vmaCopyAllocationToMemory(vk.vma, src->alloc, offset, dst, size);
        if (result != VK_SUCCESS)
            hgError("Could not read gpu buffer: %s\n", vkResultToStr(result));
        return;
    }

    HgGpuBuffer* stage = hgGpuBufferCreate(size, HgGpuBufferUsage_transferDst, HgGpuMemoryUsage_stagingRead);
    hgDefer(hgGpuBufferDestroy(stage));

    HgGpuCmd* cmd = hgGpuCmdBegin();

    VkBufferCopy region{};
    region.srcOffset = offset;
    region.size = size;

    vkCmdCopyBuffer((VkCommandBuffer)cmd, src->buffer, stage->buffer, 1, &region);

    hgGpuCmdEnd(cmd);

    hgGpuBufferRead(dst, stage, 0, size);

    src->lastStage = HgGpuStage_transfer;
    src->lastAccess = HgGpuAccess_transferRead;
}

HgGpuImage* hgGpuImageCreate(u32 width, u32 height, HgFormat format, HgGpuImageUsageFlags usage)
{
    HgGpuImageCreateEx create{};
    create.width = width;
    create.height = height;
    create.format = format;
    create.usage = usage;
    return hgGpuImageCreateEx(&create);
}

HgGpuImage* hgGpuImageCreateEx(const HgGpuImageCreateEx* create)
{
    hgAssert(create->format != HgFormat_undefined);
    hgAssert(create->usage != 0);

    HgGpuImage* image = hgPoolAlloc(&vk.images);
    *image = {};

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = gpuImageConfigFlagsToVk(create->flags);
    imageInfo.imageType = imageDimensionsToVkImage(create->dimensions);
    imageInfo.format = formatToVk(create->format);
    imageInfo.extent = {create->width, create->height, create->depth};
    imageInfo.mipLevels = create->mipLevels;
    imageInfo.arrayLayers = create->arrayLayers;
    imageInfo.samples = countToMsaaSampleBits(create->msaaSamples);
    imageInfo.usage = gpuImageUsageToVk(create->usage);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult result = vmaCreateImage(
        vk.vma,
        &imageInfo,
        &allocInfo,
        &image->image,
        &image->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        hgError("Could not create VkImage: %s\n", vkResultToStr(result));

    image->usage = create->usage;
    image->format = create->format;
    image->width = create->width;
    image->height = create->height;
    image->depth = create->depth;
    image->dimensions = (u8)create->dimensions;
    image->mipLevels = (u8)create->mipLevels;
    image->arrayLayers = (u8)create->arrayLayers;
    image->msaaSamples = (u8)countToMsaaSampleBits(create->msaaSamples);

    return image;
}

void hgGpuImageDestroy(HgGpuImage* image)
{
    if (image != nullptr)
    {
        vmaDestroyImage(vk.vma, image->image, image->alloc);
        hgPoolFree(&vk.images, image);
    }
}

u32 hgGpuImageWidth(HgGpuImage* image)
{
    return image->width;
}

u32 hgGpuImageHeight(HgGpuImage* image)
{
    return image->height;
}

static VkSampler samplerCreate(SamplerInfo* desc)
{
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = gpuFilterToVk(desc->filter);
    info.minFilter = gpuFilterToVk(desc->filter);
    info.mipmapMode = desc->filter == HgGpuFilter_linear
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
    VkResult result = vkCreateSampler(vk.device, &info, nullptr, &sampler);
    if (sampler == nullptr)
        hgError("Could not create VkSampler: %s\n", vkResultToStr(result));

    return sampler;
}

static VkSampler samplerGet(
    HgGpuFilter filter,
    HgGpuSamplerEdgeMode addressMode = HgGpuSamplerEdgeMode_repeat,
    HgGpuSamplerBorder borderColor = HgGpuSamplerBorder_floatTransparentBlack)
{
    SamplerInfo desc = {filter, addressMode, borderColor};
    VkSampler* sampler = hgMapGet(&vk.samplers, desc);
    if (sampler == nullptr)
    {
        sampler = hgMapAdd(&vk.samplers, desc, samplerCreate(&desc));
    }
    return *sampler;
}

HgGpuView* hgGpuViewCreate(
    HgGpuImage* image,
    HgGpuAspectFlags aspectFlags,
    HgGpuFilter filter)
{
    HgGpuViewCreateEx config{};
    config.image = image;
    config.baseMipLevel = 0;
    config.levelCount = image->mipLevels;
    config.baseArrayLayer = 0;
    config.layerCount = image->arrayLayers;
    config.aspectFlags = aspectFlags;
    config.type = imageDimensionsToHgView(image->dimensions);
    config.filter = filter;
    config.edgeMode = HgGpuSamplerEdgeMode_repeat;
    config.border = HgGpuSamplerBorder_floatTransparentBlack;
    return hgGpuViewCreateEx(&config);
}

HgGpuView* hgGpuViewCreateEx(const HgGpuViewCreateEx* config)
{
    hgAssert(config->aspectFlags != 0);

    HgGpuView* view = hgPoolAlloc(&vk.views);
    *view = {};

    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = config->image->image;
    info.viewType = gpuViewTypeToVk(config->type);
    info.format = formatToVk(config->image->format);
    info.subresourceRange.aspectMask = gpuAspectToVk(config->aspectFlags);
    info.subresourceRange.baseMipLevel = config->baseMipLevel;
    info.subresourceRange.levelCount = config->levelCount;
    info.subresourceRange.baseArrayLayer = config->baseArrayLayer;
    info.subresourceRange.layerCount = config->layerCount;

    VkResult result = vkCreateImageView(vk.device, &info, nullptr, &view->view);
    if (view->view == nullptr)
        hgError("Could not create VkImageView: %s\n", vkResultToStr(result));

    if (config->image->usage & HgGpuImageUsage_sampled)
    {
        view->sampler = samplerGet(config->filter, config->edgeMode, config->border);

        DescriptorImageInfo descInfo{view, HgGpuLayout_shaderReadOnly};
        view->samplerDesc = descriptorCreate(DescriptorType_combinedImageSampler, nullptr, &descInfo);
    }

    if (config->image->usage & HgGpuImageUsage_storage)
    {
        DescriptorImageInfo descInfo{view, HgGpuLayout_general};
        view->storageDesc = descriptorCreate(DescriptorType_storageImage, nullptr, &descInfo);
    }

    view->image = config->image;
    view->type = config->type;
    view->aspectFlags = config->aspectFlags;
    view->baseMipLevel = (u8)config->baseMipLevel;
    view->levelCount = (u8)config->levelCount;
    view->baseArrayLayer = (u8)config->baseArrayLayer;
    view->layerCount = (u8)config->layerCount;

    return view;
}

void hgGpuViewDestroy(HgGpuView* view)
{
    if (view != nullptr)
    {
        descriptorDestroy(view->storageDesc, DescriptorType_storageImage);
        descriptorDestroy(view->samplerDesc, DescriptorType_combinedImageSampler);
        vkDestroyImageView(vk.device, view->view, nullptr);
        hgPoolFree(&vk.views, view);
    }
}

u32 hgGpuViewWidth(HgGpuView* view)
{
    return view->image->width;
}

u32 hgGpuViewHeight(HgGpuView* view)
{
    return view->image->height;
}

u32 hgGpuImageSamplerDescriptor(HgGpuView* view)
{
    Descriptor desc = view->samplerDesc;
    hgAssert(hgHandlePoolAlive(&vk.descriptorPools[DescriptorType_combinedImageSampler], desc));
    return hgHandleIdx(desc);
}

u32 hgGpuImageStorageDescriptor(HgGpuView* view)
{
    Descriptor desc = view->storageDesc;
    hgAssert(hgHandlePoolAlive(&vk.descriptorPools[DescriptorType_storageImage], desc));
    return hgHandleIdx(desc);
}

void hgGpuImageWrite(HgGpuView* dst, const void* src)
{
    hgAssert(dst != nullptr);
    hgAssert(src != nullptr);

    u64 size = dst->image->width
             * dst->image->height
             * dst->image->depth
             * hgFormatToSize(dst->image->format);

    HgGpuBuffer* stage = hgGpuBufferCreate(size, HgGpuBufferUsage_transferSrc, HgGpuMemoryUsage_stagingWrite);
    hgDefer(hgGpuBufferDestroy(stage));
    hgGpuBufferWrite(stage, 0, src, size);

    HgGpuCmd* cmd = hgGpuCmdBegin();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarrier.image = dst->image->image;
    transferBarrier.subresourceRange.aspectMask = gpuAspectToVk(dst->aspectFlags);
    transferBarrier.subresourceRange.baseMipLevel = dst->baseMipLevel;
    transferBarrier.subresourceRange.levelCount = dst->levelCount;
    transferBarrier.subresourceRange.baseArrayLayer = dst->baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = dst->layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2((VkCommandBuffer)cmd, &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = gpuAspectToVk(dst->aspectFlags);
    region.imageSubresource.mipLevel = dst->baseMipLevel;
    region.imageSubresource.baseArrayLayer = dst->baseArrayLayer;
    region.imageSubresource.layerCount = dst->layerCount;
    region.imageExtent = {dst->image->width, dst->image->height, dst->image->depth};

    vkCmdCopyBufferToImage(
        (VkCommandBuffer)cmd,
        stage->buffer,
        dst->image->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    hgGpuCmdEnd(cmd);

    dst->lastStage = HgGpuStage_transfer;
    dst->lastAccess = HgGpuAccess_transferWrite;
    dst->lastLayout = HgGpuLayout_transferDst;
}

void hgGpuImageWriteCubemap(HgGpuView* dst, const void* src)
{
    hgAssert(dst != nullptr);
    hgAssert(dst->image->depth == 1);
    hgAssert(dst->baseArrayLayer == 0);
    hgAssert(dst->layerCount >= 6);
    hgAssert(src != nullptr);

    u64 size = dst->image->width * dst->image->height * hgFormatToSize(dst->image->format);

    HgGpuBuffer* buffer = hgGpuBufferCreate(size * 4 * 3, HgGpuBufferUsage_transferSrc, HgGpuMemoryUsage_stagingWrite);
    hgDefer(hgGpuBufferDestroy(buffer));

    hgGpuBufferWrite(buffer, 0, src, size * 4 * 3);

    HgGpuImage* stage = hgGpuImageCreate(
        dst->image->width * 4,
        dst->image->height * 3,
        dst->image->format,
        HgGpuImageUsage_transferDst | HgGpuImageUsage_transferSrc);
    hgDefer(hgGpuImageDestroy(stage));

    HgGpuCmd* cmd = hgGpuCmdBegin();

    VkImageMemoryBarrier2 stageBarrier{};
    stageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    stageBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    stageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    stageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    stageBarrier.image = stage->image;
    stageBarrier.subresourceRange.aspectMask = gpuAspectToVk(dst->aspectFlags);
    stageBarrier.subresourceRange.baseMipLevel = 0;
    stageBarrier.subresourceRange.levelCount = 1;
    stageBarrier.subresourceRange.baseArrayLayer = 0;
    stageBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo stageDep{};
    stageDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    stageDep.imageMemoryBarrierCount = 1;
    stageDep.pImageMemoryBarriers = &stageBarrier;

    vkCmdPipelineBarrier2((VkCommandBuffer)cmd, &stageDep);

    VkBufferImageCopy stageRegion{};
    stageRegion.imageSubresource = {gpuAspectToVk(dst->aspectFlags), 0, 0, 1};
    stageRegion.imageExtent = {dst->image->width * 4, dst->image->height * 3, 1};

    vkCmdCopyBufferToImage(
        (VkCommandBuffer)cmd,
        buffer->buffer,
        stage->image,
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
    transferBarriers[0].image = stage->image;
    transferBarriers[0].subresourceRange.aspectMask = gpuAspectToVk(dst->aspectFlags);
    transferBarriers[0].subresourceRange.baseMipLevel = 0;
    transferBarriers[0].subresourceRange.levelCount = 1;
    transferBarriers[0].subresourceRange.baseArrayLayer = 0;
    transferBarriers[0].subresourceRange.layerCount = 1;

    transferBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[1].image = dst->image->image;
    transferBarriers[1].subresourceRange.aspectMask = gpuAspectToVk(dst->aspectFlags);
    transferBarriers[1].subresourceRange.baseMipLevel = dst->baseMipLevel;
    transferBarriers[1].subresourceRange.levelCount = dst->levelCount;
    transferBarriers[1].subresourceRange.baseArrayLayer = dst->baseArrayLayer;
    transferBarriers[1].subresourceRange.layerCount = dst->layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = (u32)hgArrayCount(transferBarriers);
    transferDep.pImageMemoryBarriers = transferBarriers;

    vkCmdPipelineBarrier2((VkCommandBuffer)cmd, &transferDep);

    VkImageCopy regions[6]{};

    regions[0].srcSubresource = {gpuAspectToVk(dst->aspectFlags), 0, 0, 1};
    regions[0].srcOffset = {(int)dst->image->width * 2, (int)dst->image->height * 1, 0};
    regions[0].dstSubresource = {gpuAspectToVk(dst->aspectFlags), dst->baseMipLevel, 0, 1};
    regions[0].dstOffset = {};
    regions[0].extent = {dst->image->width, dst->image->height, 1};

    regions[1].srcSubresource = {gpuAspectToVk(dst->aspectFlags), 0, 0, 1};
    regions[1].srcOffset = {(int)dst->image->width * 0, (int)dst->image->height * 1, 0};
    regions[1].dstSubresource = {gpuAspectToVk(dst->aspectFlags), dst->baseMipLevel, 1, 1};
    regions[1].dstOffset = {};
    regions[1].extent = {dst->image->width, dst->image->height, 1};

    regions[2].srcSubresource = {gpuAspectToVk(dst->aspectFlags), 0, 0, 1};
    regions[2].srcOffset = {(int)dst->image->width * 1, (int)dst->image->height * 2, 0};
    regions[2].dstSubresource = {gpuAspectToVk(dst->aspectFlags), dst->baseMipLevel, 2, 1};
    regions[2].dstOffset = {};
    regions[2].extent = {dst->image->width, dst->image->height, 1};

    regions[3].srcSubresource = {gpuAspectToVk(dst->aspectFlags), 0, 0, 1};
    regions[3].srcOffset = {(int)dst->image->width * 1, (int)dst->image->height * 0, 0};
    regions[3].dstSubresource = {gpuAspectToVk(dst->aspectFlags), dst->baseMipLevel, 3, 1};
    regions[3].dstOffset = {};
    regions[3].extent = {dst->image->width, dst->image->height, 1};

    regions[4].srcSubresource = {gpuAspectToVk(dst->aspectFlags), 0, 0, 1};
    regions[4].srcOffset = {(int)dst->image->width * 1, (int)dst->image->height * 1, 0};
    regions[4].dstSubresource = {gpuAspectToVk(dst->aspectFlags), dst->baseMipLevel, 4, 1};
    regions[4].dstOffset = {};
    regions[4].extent = {dst->image->width, dst->image->height, 1};

    regions[5].srcSubresource = {gpuAspectToVk(dst->aspectFlags), 0, 0, 1};
    regions[5].srcOffset = {(int)dst->image->width * 3, (int)dst->image->height * 1, 0};
    regions[5].dstSubresource = {gpuAspectToVk(dst->aspectFlags), dst->baseMipLevel, 5, 1};
    regions[5].dstOffset = {};
    regions[5].extent = {dst->image->width, dst->image->height, 1};

    vkCmdCopyImage(
        (VkCommandBuffer)cmd,
        stage->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst->image->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        (u32)hgArrayCount(regions),
        regions);

    hgGpuCmdEnd(cmd);

    dst->lastStage = HgGpuStage_transfer;
    dst->lastAccess = HgGpuAccess_transferWrite;
    dst->lastLayout = HgGpuLayout_transferDst;
}

void hgGpuImageRead(void* dst, HgGpuView* src)
{
    hgAssert(src != nullptr);
    hgAssert(src->lastLayout != HgGpuLayout_undefined);
    hgAssert(dst != nullptr);

    u64 size = src->image->width
             * src->image->height
             * src->image->depth
             * hgFormatToSize(src->image->format);

    HgGpuBuffer* stage = hgGpuBufferCreate(size, HgGpuBufferUsage_transferDst, HgGpuMemoryUsage_stagingRead);
    hgDefer(hgGpuBufferDestroy(stage));

    HgGpuCmd* cmd = hgGpuCmdBegin();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.srcStageMask = gpuStageToVk(src->lastStage);
    transferBarrier.srcAccessMask = gpuAccessToVk(src->lastAccess);
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarrier.oldLayout = gpuLayoutToVk(src->lastLayout);
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarrier.image = src->image->image;
    transferBarrier.subresourceRange.aspectMask = gpuAspectToVk(src->aspectFlags);
    transferBarrier.subresourceRange.baseMipLevel = src->baseMipLevel;
    transferBarrier.subresourceRange.levelCount = src->levelCount;
    transferBarrier.subresourceRange.baseArrayLayer = src->baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = src->layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2((VkCommandBuffer)cmd, &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = gpuAspectToVk(src->aspectFlags);
    region.imageSubresource.mipLevel = src->baseMipLevel;
    region.imageSubresource.baseArrayLayer = src->baseArrayLayer;
    region.imageSubresource.layerCount = src->layerCount;
    region.imageExtent = {src->image->width, src->image->height, src->image->depth};

    vkCmdCopyImageToBuffer(
        (VkCommandBuffer)cmd,
        src->image->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        stage->buffer,
        1,
        &region);

    hgGpuCmdEnd(cmd);

    hgGpuBufferRead(dst, stage, 0, size);

    src->lastStage = HgGpuStage_transfer;
    src->lastAccess = HgGpuAccess_transferRead;
    src->lastLayout = HgGpuLayout_transferSrc;
}

void hgGpuImageGenMipmaps(HgGpuView* dst)
{
    hgAssert(dst != nullptr);
    hgAssert(dst->lastLayout != HgGpuLayout_undefined);
    if (dst->levelCount == 1)
        return;

    HgGpuCmd* cmd = hgGpuCmdBegin();

    VkOffset3D mipOffset{};
    mipOffset.x = (i32)dst->image->width;
    mipOffset.y = (i32)dst->image->height;
    mipOffset.z = (i32)dst->image->depth;

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = gpuLayoutToVk(dst->lastLayout);
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = dst->image->image;
    barrier.subresourceRange.aspectMask = gpuAspectToVk(dst->aspectFlags);
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2((VkCommandBuffer)cmd, &dep);

    for (u32 level = 0; level < (u32)dst->levelCount - 1; ++level)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.subresourceRange.aspectMask = gpuAspectToVk(dst->aspectFlags);
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2((VkCommandBuffer)cmd, &dep);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = gpuAspectToVk(dst->aspectFlags);
        blit.srcSubresource.mipLevel = level;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[1] = mipOffset;
        if (mipOffset.x > 1)
            mipOffset.x /= 2;
        if (mipOffset.y > 1)
            mipOffset.y /= 2;
        if (mipOffset.z > 1)
            mipOffset.z /= 2;
        blit.dstSubresource.aspectMask = gpuAspectToVk(dst->aspectFlags);
        blit.dstSubresource.mipLevel = level + 1;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[1] = mipOffset;

        vkCmdBlitImage(
            (VkCommandBuffer)cmd,
            dst->image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dst->image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.subresourceRange.aspectMask = gpuAspectToVk(dst->aspectFlags);
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2((VkCommandBuffer)cmd, &dep);
    }

    hgGpuCmdEnd(cmd);

    dst->lastStage = HgGpuStage_transfer;
    dst->lastAccess = HgGpuAccess_transferRead;
    dst->lastLayout = HgGpuLayout_transferSrc;
}

static VkShaderModule createShaderModule(const void* spirvCode, u64 codeSize)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = codeSize;
    info.pCode = (const u32*)spirvCode;

    VkShaderModule shader = nullptr;
    VkResult result = vkCreateShaderModule(vk.device, &info, nullptr, &shader);
    if (shader == nullptr)
        hgError("Could not create VkShaderModule: %s\n", vkResultToStr(result));

    return shader;
}

HgGpuPipeline* hgGpuPipelineCreateGraphics(const HgCreateGpuGraphicsPipeline* config)
{
    hgAssert(config->vertexShader != nullptr);
    hgAssert(config->fragmentShader != nullptr);
    if (config->colorAttachmentCount > 0)
        hgAssert(config->colorAttachmentFormats != nullptr);

    HgGpuPipeline* pipeline = hgPoolAlloc(&vk.pipelines);
    *pipeline = {};

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &vk.bindlessLayout;
    VkPushConstantRange pushRange{VK_SHADER_STAGE_ALL, 0, config->pushConstantSize};
    if (config->pushConstantSize > 0)
    {
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
    }

    VkResult layoutResult = vkCreatePipelineLayout(vk.device, &layoutInfo, nullptr, &pipeline->layout);
    if (pipeline->layout == nullptr)
        hgError("Could not create VkPipelineLayout: %s\n", vkResultToStr(layoutResult));

    VkShaderModule vertexShader = createShaderModule(config->vertexShader, config->vertexShaderSize);
    VkShaderModule fragmentShader = createShaderModule(config->fragmentShader, config->fragmentShaderSize);
    hgDefer(vkDestroyShaderModule(vk.device, vertexShader, nullptr));
    hgDefer(vkDestroyShaderModule(vk.device, fragmentShader, nullptr));

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
    inputAssemblyState.topology = gpuTopologyToVk(config->topology);
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo tessellationState{};
    tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellationState.patchControlPoints = config->tesselationPatchControlPoints;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = gpuPolygonModeToVk(config->polygonMode);
    rasterizationState.cullMode = gpuCullModeToVk(config->cullMode);
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp = 0.0f;
    rasterizationState.depthBiasSlopeFactor = 0.0f;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = countToMsaaSampleBits(config->multisampleCount);
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.minSampleShading = 1.0f;
    multisampleState.pSampleMask = nullptr;
    multisampleState.alphaToCoverageEnable = VK_FALSE;
    multisampleState.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = config->enableDepthRead;
    depthStencilState.depthWriteEnable = config->enableDepthWrite;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable = config->enableDepthRead;
    depthStencilState.stencilTestEnable = VK_FALSE, // : TODO
    depthStencilState.front = {}; // : TODO
    depthStencilState.back = {}; // : TODO
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState* colorBlendAttachments
        = hgArenaAlloc<VkPipelineColorBlendAttachmentState>(scratch, config->colorAttachmentCount);

    for (u32 i = 0; i < config->colorAttachmentCount; ++i)
    {
        colorBlendAttachments[i].blendEnable = config->colorBlendEnables != nullptr
            ? config->colorBlendEnables[i]
            : VK_FALSE;
        colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments[i].colorWriteMask
            = VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = config->colorAttachmentCount;
    colorBlendState.pAttachments = colorBlendAttachments;
    for (float& blendConstant : colorBlendState.blendConstants)
    {
        blendConstant = 1.0f;
    }

    VkDynamicState dynamicStates[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (u32)hgArrayCount(dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;

    VkFormat* colorFormats = hgArenaAlloc<VkFormat>(scratch, config->colorAttachmentCount);
    for (u32 i = 0; i < config->colorAttachmentCount; ++i)
    {
        colorFormats[i] = formatToVk(config->colorAttachmentFormats[i]);
    }
    VkFormat depthFormat = formatToVk(config->depthAttachmentFormat);
    VkFormat stencilFormat = formatToVk(config->stencilAttachmentFormat);

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = (u32)config->colorAttachmentCount;
    renderingInfo.pColorAttachmentFormats = colorFormats;
    renderingInfo.depthAttachmentFormat = depthFormat;
    renderingInfo.stencilAttachmentFormat = stencilFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = (u32)hgArrayCount(shaderStages);
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
    pipelineInfo.layout = pipeline->layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    VkResult pipelineResult = vkCreateGraphicsPipelines(
        vk.device, nullptr, 1, &pipelineInfo, nullptr, &pipeline->pipeline);
    if (pipeline == nullptr)
        hgError("Failed to create Vulkan graphics pipeline: %s\n", vkResultToStr(pipelineResult));

    pipeline->bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    return pipeline;
}

HgGpuPipeline* hgGpuPipelineCreateCompute(u32 pushSize, const u8* shaderCode, u64 shaderCodeSize)
{
    hgAssert(shaderCode != nullptr);
    hgAssert(shaderCodeSize > 0);

    HgGpuPipeline* pipeline = hgPoolAlloc(&vk.pipelines);
    *pipeline = {};

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &vk.bindlessLayout;

    VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, pushSize};
    layoutInfo.pushConstantRangeCount = pushSize > 0 ? 1 : 0;
    layoutInfo.pPushConstantRanges = pushSize > 0 ? &push : nullptr;

    VkResult layoutResult = vkCreatePipelineLayout(vk.device, &layoutInfo, nullptr, &pipeline->layout);
    if (pipeline->layout == nullptr)
        hgError("Could not create VkPipelineLayout: %s\n", vkResultToStr(layoutResult));

    VkShaderModule computeShader = createShaderModule(shaderCode, shaderCodeSize);
    hgDefer(vkDestroyShaderModule(vk.device, computeShader, nullptr));

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = computeShader;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = pipeline->layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    VkResult pipelineResult = vkCreateComputePipelines(
        vk.device, nullptr, 1, &pipelineInfo, nullptr, &pipeline->pipeline);
    if (pipeline == nullptr)
        hgError("Failed to create Vulkan compute pipeline: %s\n", vkResultToStr(pipelineResult));

    pipeline->bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

    return pipeline;
}

void hgGpuPipelineDestroy(HgGpuPipeline* pipeline)
{
    if (pipeline != nullptr)
    {
        vkDestroyPipeline(vk.device, pipeline->pipeline, nullptr);
        vkDestroyPipelineLayout(vk.device, pipeline->layout, nullptr);
        hgPoolFree(&vk.pipelines, pipeline);
    }
}

HgGpuCmd* hgGpuCmdBegin()
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
    return (HgGpuCmd*)cmd;
}

void hgGpuCmdEnd(HgGpuCmd* cmd)
{
    hgAssert(cmd != nullptr);
    vkEndCommandBuffer((VkCommandBuffer)cmd);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;
    vkCreateFence(vk.device, &fenceInfo, nullptr, &fence);
    hgDefer(vkDestroyFence(vk.device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = (VkCommandBuffer*)&cmd;

    vkQueueSubmit(vk.queue, 1, &submit, fence);
    vkWaitForFences(vk.device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkFreeCommandBuffers(vk.device, vk.cmdPool, 1, (VkCommandBuffer*)&cmd);
}

void hgGpuBindPipeline(HgGpuCmd* cmd, HgGpuPipeline* pipeline)
{
    vkCmdBindPipeline(
        (VkCommandBuffer)cmd,
        pipeline->bindPoint,
        pipeline->pipeline);

    vkCmdBindDescriptorSets(
        (VkCommandBuffer)cmd,
        pipeline->bindPoint,
        pipeline->layout,
        0,
        1,
        &vk.bindlessSet,
        0,
        nullptr);
}

void hgGpuPushConstants(HgGpuCmd* cmd, HgGpuPipeline* pipeline, void* push, u32 size)
{
    vkCmdPushConstants(
        (VkCommandBuffer)cmd,
        pipeline->layout,
        pipeline->bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE
            ? VK_SHADER_STAGE_COMPUTE_BIT
            : VK_SHADER_STAGE_ALL,
        0,
        size,
        push);
}

void hgGpuDraw(HgGpuCmd* cmd, u32 vertexBegin, u32 vertexCount, u32 instanceBegin, u32 instanceCount)
{
    vkCmdDraw((VkCommandBuffer)cmd, vertexCount, instanceCount, vertexBegin, instanceBegin);
}

void hgGpuDispatch(HgGpuCmd* cmd, u32 groupCountX, u32 groupCountY, u32 groupCountZ)
{
    vkCmdDispatch((VkCommandBuffer)cmd, groupCountX, groupCountY, groupCountZ);
}

void hgGpuMemoryBarrier(
    HgGpuCmd* cmd,
    const HgGpuBufferBarrier* bufferBarriers,
    u32 bufferBarrierCount,
    const HgGpuImageBarrier* imageBarriers,
    u32 imageBarrierCount)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    VkBufferMemoryBarrier2* vkBufferBarriers = hgArenaAlloc<VkBufferMemoryBarrier2>(scratch, imageBarrierCount);
    VkImageMemoryBarrier2* vkImageBarriers = hgArenaAlloc<VkImageMemoryBarrier2>(scratch, imageBarrierCount);

    for (u32 i = 0; i < bufferBarrierCount; ++i)
    {
        HgGpuBuffer* buffer = bufferBarriers[i].buffer;

        vkBufferBarriers[i] = {};
        vkBufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        vkBufferBarriers[i].srcStageMask = gpuStageToVk(buffer->lastStage);
        vkBufferBarriers[i].srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        vkBufferBarriers[i].dstStageMask = gpuStageToVk(bufferBarriers[i].nextStage);
        vkBufferBarriers[i].dstAccessMask = gpuAccessToVk(bufferBarriers[i].nextAccess);
        vkBufferBarriers[i].buffer = buffer->buffer;
        vkBufferBarriers[i].size = buffer->size;

        buffer->lastStage = bufferBarriers[i].nextStage;
        buffer->lastAccess = bufferBarriers[i].nextAccess;
    }

    for (u32 i = 0; i < imageBarrierCount; ++i)
    {
        HgGpuView* image = imageBarriers[i].image;

        vkImageBarriers[i] = {};
        vkImageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        vkImageBarriers[i].srcStageMask = gpuStageToVk(image->lastStage);
        vkImageBarriers[i].srcAccessMask = gpuAccessToVk(image->lastAccess);
        vkImageBarriers[i].dstStageMask = gpuStageToVk(imageBarriers[i].nextStage);
        vkImageBarriers[i].dstAccessMask = gpuAccessToVk(imageBarriers[i].nextAccess);
        vkImageBarriers[i].oldLayout = gpuLayoutToVk(image->lastLayout);
        vkImageBarriers[i].newLayout = gpuLayoutToVk(imageBarriers[i].nextLayout);
        vkImageBarriers[i].image = image->image->image;
        vkImageBarriers[i].subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = imageBarriers[i].nextStage;
        image->lastAccess = imageBarriers[i].nextAccess;
        image->lastLayout = imageBarriers[i].nextLayout;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = bufferBarrierCount;
    dep.pBufferMemoryBarriers = vkBufferBarriers;
    dep.imageMemoryBarrierCount = imageBarrierCount;
    dep.pImageMemoryBarriers = vkImageBarriers;

    vkCmdPipelineBarrier2((VkCommandBuffer)cmd, &dep);
}

static VkAttachmentLoadOp gpuLoadOpToVk(HgGpuLoadOp op)
{
    return (VkAttachmentLoadOp)op;
}

static VkAttachmentStoreOp gpuStoreOpToVk(HgGpuStoreOp op)
{
    return (VkAttachmentStoreOp)op;
}

void hgGpuComputePass(HgGpuCmd* cmd, const HgGpuComputePass* pass)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    VkBufferMemoryBarrier2* bufferBarriers = nullptr;
    u32 bufferBarrierCount = 0;
    VkImageMemoryBarrier2* imageBarriers = nullptr;
    u32 imageBarrierCount = 0;

    bufferBarriers = hgArenaRealloc<VkBufferMemoryBarrier2>(scratch,
        bufferBarriers, bufferBarrierCount, bufferBarrierCount + pass->uniformBufferCount);

    for (u32 i = bufferBarrierCount; i < bufferBarrierCount + pass->uniformBufferCount; ++i)
    {
        HgGpuBuffer* buffer = &pass->uniformBuffers[i - bufferBarrierCount];

        bufferBarriers[i] = {};
        bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bufferBarriers[i].srcStageMask = gpuStageToVk(buffer->lastStage);
        bufferBarriers[i].srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        bufferBarriers[i].dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        bufferBarriers[i].buffer = buffer->buffer;
        bufferBarriers[i].size = buffer->size;

        buffer->lastStage = HgGpuStage_computeShader;
        buffer->lastAccess = HgGpuAccess_uniformRead;
    }

    bufferBarrierCount += pass->uniformBufferCount;

    bufferBarriers = hgArenaRealloc<VkBufferMemoryBarrier2>(scratch,
        bufferBarriers, bufferBarrierCount, bufferBarrierCount + pass->storageBufferCount);

    for (u32 i = bufferBarrierCount; i < bufferBarrierCount + pass->storageBufferCount; ++i)
    {
        HgGpuBuffer* buffer = &pass->storageBuffers[i - bufferBarrierCount];

        bufferBarriers[i] = {};
        bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bufferBarriers[i].srcStageMask = gpuStageToVk(buffer->lastStage);
        bufferBarriers[i].srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        bufferBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        bufferBarriers[i].buffer = buffer->buffer;
        bufferBarriers[i].size = buffer->size;

        buffer->lastStage = HgGpuStage_computeShader;
        buffer->lastAccess = HgGpuAccess_shaderRead | HgGpuAccess_shaderWrite;
    }

    bufferBarrierCount += pass->storageBufferCount;

    imageBarriers = hgArenaRealloc<VkImageMemoryBarrier2>(scratch,
        imageBarriers, imageBarrierCount, imageBarrierCount + pass->sampledImageCount);

    for (u32 i = imageBarrierCount; i < imageBarrierCount + pass->sampledImageCount; ++i)
    {
        HgGpuView* image = &pass->sampledImages[i - imageBarrierCount];

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = gpuStageToVk(image->lastStage);
        imageBarriers[i].srcAccessMask = gpuAccessToVk(image->lastAccess);
        imageBarriers[i].dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        imageBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageBarriers[i].oldLayout = gpuLayoutToVk(image->lastLayout);
        imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarriers[i].image = image->image->image;
        imageBarriers[i].subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = HgGpuStage_computeShader;
        image->lastAccess = HgGpuAccess_shaderRead;
        image->lastLayout = HgGpuLayout_shaderReadOnly;
    }

    imageBarrierCount += pass->sampledImageCount;

    imageBarriers = hgArenaRealloc<VkImageMemoryBarrier2>(scratch,
        imageBarriers, imageBarrierCount, imageBarrierCount + pass->storageImageCount);

    for (u32 i = imageBarrierCount; i < imageBarrierCount + pass->storageImageCount; ++i)
    {
        HgGpuView* image = &pass->storageImages[i - imageBarrierCount];

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = gpuStageToVk(image->lastStage);
        imageBarriers[i].srcAccessMask = gpuAccessToVk(image->lastAccess);
        imageBarriers[i].dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        imageBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        imageBarriers[i].oldLayout = gpuLayoutToVk(image->lastLayout);
        imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarriers[i].image = image->image->image;
        imageBarriers[i].subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = HgGpuStage_computeShader;
        image->lastAccess = HgGpuAccess_shaderRead | HgGpuAccess_shaderWrite;
        image->lastLayout = HgGpuLayout_general;
    }

    imageBarrierCount += pass->storageImageCount;

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = bufferBarrierCount;
    dep.pBufferMemoryBarriers = bufferBarriers;
    dep.imageMemoryBarrierCount = imageBarrierCount;
    dep.pImageMemoryBarriers = imageBarriers;

    vkCmdPipelineBarrier2((VkCommandBuffer)cmd, &dep);
}

void hgGpuRenderPassBegin(HgGpuCmd* cmd, const HgGpuRenderPass* pass)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    VkBufferMemoryBarrier2* bufferBarriers = nullptr;
    u32 bufferBarrierCount = 0;
    VkImageMemoryBarrier2* imageBarriers = nullptr;
    u32 imageBarrierCount = 0;

    bufferBarriers = hgArenaRealloc<VkBufferMemoryBarrier2>(scratch,
        bufferBarriers, bufferBarrierCount, bufferBarrierCount + pass->uniformBufferCount);

    for (u32 i = bufferBarrierCount; i < bufferBarrierCount + pass->uniformBufferCount; ++i)
    {
        HgGpuBuffer* buffer = &pass->uniformBuffers[i - bufferBarrierCount];

        bufferBarriers[i] = {};
        bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bufferBarriers[i].srcStageMask = gpuStageToVk(buffer->lastStage);
        bufferBarriers[i].srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        bufferBarriers[i].dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        bufferBarriers[i].buffer = buffer->buffer;
        bufferBarriers[i].size = buffer->size;

        buffer->lastStage = HgGpuStage_vertexShader | HgGpuStage_fragmentShader;
        buffer->lastAccess = HgGpuAccess_uniformRead;
    }

    bufferBarrierCount += pass->uniformBufferCount;

    bufferBarriers = hgArenaRealloc<VkBufferMemoryBarrier2>(scratch,
        bufferBarriers, bufferBarrierCount, bufferBarrierCount + pass->storageBufferCount);

    for (u32 i = bufferBarrierCount; i < bufferBarrierCount + pass->storageBufferCount; ++i)
    {
        HgGpuBuffer* buffer = &pass->storageBuffers[i - bufferBarrierCount];

        bufferBarriers[i] = {};
        bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bufferBarriers[i].srcStageMask = gpuStageToVk(buffer->lastStage);
        bufferBarriers[i].srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        bufferBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        bufferBarriers[i].buffer = buffer->buffer;
        bufferBarriers[i].size = buffer->size;

        buffer->lastStage = HgGpuStage_vertexShader | HgGpuStage_fragmentShader;
        buffer->lastAccess = HgGpuAccess_shaderRead | HgGpuAccess_shaderWrite;
    }

    bufferBarrierCount += pass->storageBufferCount;

    imageBarriers = hgArenaRealloc<VkImageMemoryBarrier2>(scratch,
        imageBarriers, imageBarrierCount, imageBarrierCount + pass->sampledImageCount);

    for (u32 i = imageBarrierCount; i < imageBarrierCount + pass->sampledImageCount; ++i)
    {
        HgGpuView* image = &pass->sampledImages[i - imageBarrierCount];

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = gpuStageToVk(image->lastStage);
        imageBarriers[i].srcAccessMask = gpuAccessToVk(image->lastAccess);
        imageBarriers[i].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        imageBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageBarriers[i].oldLayout = gpuLayoutToVk(image->lastLayout);
        imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarriers[i].image = image->image->image;
        imageBarriers[i].subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = HgGpuStage_fragmentShader;
        image->lastAccess = HgGpuAccess_shaderRead;
        image->lastLayout = HgGpuLayout_shaderReadOnly;
    }

    imageBarrierCount += pass->sampledImageCount;

    imageBarriers = hgArenaRealloc<VkImageMemoryBarrier2>(scratch,
        imageBarriers, imageBarrierCount, imageBarrierCount + pass->storageImageCount);

    for (u32 i = imageBarrierCount; i < imageBarrierCount + pass->storageImageCount; ++i)
    {
        HgGpuView* image = &pass->storageImages[i - imageBarrierCount];

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = gpuStageToVk(image->lastStage);
        imageBarriers[i].srcAccessMask = gpuAccessToVk(image->lastAccess);
        imageBarriers[i].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        imageBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        imageBarriers[i].oldLayout = gpuLayoutToVk(image->lastLayout);
        imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarriers[i].image = image->image->image;
        imageBarriers[i].subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = HgGpuStage_fragmentShader;
        image->lastAccess = HgGpuAccess_shaderRead | HgGpuAccess_shaderWrite;
        image->lastLayout = HgGpuLayout_general;
    }

    imageBarrierCount += pass->storageImageCount;

    imageBarriers = hgArenaRealloc<VkImageMemoryBarrier2>(
        scratch, imageBarriers, imageBarrierCount, imageBarrierCount + pass->colorAttachmentCount);

    for (u32 i = imageBarrierCount; i < imageBarrierCount + pass->colorAttachmentCount; ++i)
    {
        HgGpuView* image = pass->colorAttachments[i - imageBarrierCount].image;

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = gpuStageToVk(image->lastStage);
        imageBarriers[i].srcAccessMask = gpuAccessToVk(image->lastAccess);
        imageBarriers[i].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        imageBarriers[i].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if (pass->colorAttachments[i - imageBarrierCount].loadOp == HgGpuLoadOp_load)
            imageBarriers[i].oldLayout = gpuLayoutToVk(image->lastLayout);
        imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imageBarriers[i].image = image->image->image;
        imageBarriers[i].subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = HgGpuStage_colorAttachmentOutput;
        image->lastAccess = HgGpuAccess_colorAttachmentWrite;
        image->lastLayout = HgGpuLayout_colorAttachment;
    }

    imageBarrierCount += pass->colorAttachmentCount;

    if (pass->depthAttachment != nullptr)
    {
        imageBarriers = hgArenaRealloc<VkImageMemoryBarrier2>(
            scratch, imageBarriers, imageBarrierCount, imageBarrierCount + 1);

        HgGpuView* image = pass->depthAttachment->image;

        u32 idx = imageBarrierCount;
        imageBarriers[idx] = {};
        imageBarriers[idx].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[idx].srcStageMask = gpuStageToVk(image->lastStage);
        imageBarriers[idx].srcAccessMask = gpuAccessToVk(image->lastAccess);
        imageBarriers[idx].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        imageBarriers[idx].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                                         | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if (pass->depthAttachment->loadOp == HgGpuLoadOp_load)
            imageBarriers[idx].oldLayout = gpuLayoutToVk(image->lastLayout);
        imageBarriers[idx].newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        imageBarriers[idx].image = image->image->image;
        imageBarriers[idx].subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = HgGpuStage_earlyFragmentTests | HgGpuStage_lateFragmentTests;
        image->lastAccess = HgGpuAccess_depthStencilAttachmentRead | HgGpuAccess_depthStencilAttachmentWrite;
        image->lastLayout = HgGpuLayout_depthStencilAttachment;

        ++imageBarrierCount;
    }

    if (pass->stencilAttachment != nullptr)
    {
        imageBarriers = hgArenaRealloc<VkImageMemoryBarrier2>(
            scratch, imageBarriers, imageBarrierCount, imageBarrierCount + 1);

        HgGpuView* image = pass->stencilAttachment->image;

        u32 idx = imageBarrierCount;
        imageBarriers[idx] = {};
        imageBarriers[idx].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[idx].srcStageMask = gpuStageToVk(image->lastStage);
        imageBarriers[idx].srcAccessMask = gpuAccessToVk(image->lastAccess);
        imageBarriers[idx].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        imageBarriers[idx].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                                         | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if (pass->stencilAttachment->loadOp == HgGpuLoadOp_load)
            imageBarriers[idx].oldLayout = gpuLayoutToVk(image->lastLayout);
        imageBarriers[idx].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        imageBarriers[idx].image = image->image->image;
        imageBarriers[idx].subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = HgGpuStage_earlyFragmentTests | HgGpuStage_lateFragmentTests;
        image->lastAccess = HgGpuAccess_depthStencilAttachmentRead | HgGpuAccess_depthStencilAttachmentWrite;
        image->lastLayout = HgGpuLayout_depthStencilAttachment;

        ++imageBarrierCount;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = bufferBarrierCount;
    dep.pBufferMemoryBarriers = bufferBarriers;
    dep.imageMemoryBarrierCount = imageBarrierCount;
    dep.pImageMemoryBarriers = imageBarriers;

    vkCmdPipelineBarrier2((VkCommandBuffer)cmd, &dep);

    VkRenderingAttachmentInfo* colorAttachments
        = hgArenaAlloc<VkRenderingAttachmentInfo>(scratch, pass->colorAttachmentCount);

    for (u32 i = 0; i < pass->colorAttachmentCount; ++i)
    {
        colorAttachments[i] = {};
        colorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[i].imageView = pass->colorAttachments[i].image->view;
        colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[i].loadOp = gpuLoadOpToVk(pass->colorAttachments[i].loadOp);
        colorAttachments[i].storeOp = gpuStoreOpToVk(pass->colorAttachments[i].storeOp);
        hgMemCopy(&colorAttachments[i].clearValue, &pass->colorAttachments[i].clearValue, sizeof(VkClearValue));
    }

    VkRenderingAttachmentInfo depthAttachment{};
    if (pass->depthAttachment != nullptr)
    {
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = pass->depthAttachment->image->view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = gpuLoadOpToVk(pass->depthAttachment->loadOp);
        depthAttachment.storeOp = gpuStoreOpToVk(pass->depthAttachment->storeOp);
        hgMemCopy(&depthAttachment.clearValue, &pass->depthAttachment->clearValue, sizeof(VkClearValue));
    }

    VkRenderingAttachmentInfo stencilAttachment{};
    if (pass->stencilAttachment != nullptr)
    {
        stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        stencilAttachment.imageView = pass->stencilAttachment->image->view;
        stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        stencilAttachment.loadOp = gpuLoadOpToVk(pass->stencilAttachment->loadOp);
        stencilAttachment.storeOp = gpuStoreOpToVk(pass->stencilAttachment->storeOp);
        hgMemCopy(&stencilAttachment.clearValue, &pass->stencilAttachment->clearValue, sizeof(VkClearValue));
    }

    u32 width = pass->colorAttachments[0].image->image->width;
    u32 height = pass->colorAttachments[0].image->image->height;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = {width, height};
    renderingInfo.layerCount = pass->layerCount;
    renderingInfo.colorAttachmentCount = pass->colorAttachmentCount;
    renderingInfo.pColorAttachments = colorAttachments;
    renderingInfo.pDepthAttachment = pass->depthAttachment != nullptr
        ? &depthAttachment
        : nullptr;
    renderingInfo.pStencilAttachment = pass->stencilAttachment != nullptr
        ? &stencilAttachment
        : nullptr;

    vkCmdBeginRendering((VkCommandBuffer)cmd, &renderingInfo);

    hgGpuSetViewport(cmd, 0, 0, (f32)width, (f32)height);
    hgGpuSetScissor(cmd, 0, 0, width, height);
}

static VkPresentModeKHR hgPresentModeToVk(HgGpuPresentMode mode)
{
    return (VkPresentModeKHR)mode;
}

void hgGpuRenderPassEnd(HgGpuCmd* cmd)
{
    vkCmdEndRendering((VkCommandBuffer)cmd);
}

void hgGpuSetViewport(HgGpuCmd* cmd, f32 x, f32 y, f32 width, f32 height, f32 near, f32 far)
{
    VkViewport viewport{x, y, width, height, near, far};
    vkCmdSetViewport((VkCommandBuffer)cmd, 0, 1, &viewport);
}

void hgGpuSetScissor(HgGpuCmd* cmd, i32 x, i32 y, u32 width, u32 height)
{
    VkRect2D scissor{{x, y}, {width, height}};
    vkCmdSetScissor((VkCommandBuffer)cmd, 0, 1, &scissor);
}

static void resizeWindowSwapchain(HgWindow* window)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    vkQueueWaitIdle(vk.queue);

    for (u32 i = 0; i < window->images.count; ++i)
    {
        vkDestroyImageView(vk.device, window->views[i].view, nullptr);

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
        swapchainInfo.minImageCount = hgMin(capabilities.minImageCount, capabilities.maxImageCount - 1) + 1;
        swapchainInfo.imageFormat = formatToVk(window->format);
        swapchainInfo.imageExtent = {window->width, window->height};
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = window->imageUsage;
        swapchainInfo.preTransform = capabilities.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = hgPresentModeToVk(window->presentMode);
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = window->swapchain;

        VkResult result = vkCreateSwapchainKHR(vk.device, &swapchainInfo, nullptr, &window->swapchain);
        if (window->swapchain == nullptr)
            hgError("Failed to create swapchain: %s\n", vkResultToStr(result));

        u32 swapImageCount;
        vkGetSwapchainImagesKHR(vk.device, window->swapchain, &swapImageCount, nullptr);

        if (window->images.count != swapImageCount)
        {
            hgArrayResize(&window->images, swapImageCount);
            hgArrayResize(&window->views, swapImageCount);
            hgArrayResize(&window->readyToPresent, swapImageCount);
        }

        VkImage* swapImages = hgArenaAlloc<VkImage>(scratch, swapImageCount);
        vkGetSwapchainImagesKHR(vk.device, window->swapchain, &swapImageCount, swapImages);

        for (u32 i = 0; i < window->images.count; ++i)
        {
            window->images[i] = {};
            window->images[i].image = swapImages[i];
            window->images[i].dimensions = 2;
            window->images[i].format = window->format;
            window->images[i].width = window->width;
            window->images[i].height = window->height;
            window->images[i].depth = 1;
            window->images[i].mipLevels = 1;
            window->images[i].arrayLayers = 1;
            window->images[i].msaaSamples = 1;

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = formatToVk(window->format);
            viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            window->views[i] = {};

            VkResult viewResult = vkCreateImageView(vk.device, &viewInfo, nullptr, &window->views[i].view);
            if (window->views[i].view == nullptr)
                hgError("Could not create VkImageView: %s\n", vkResultToStr(viewResult));

            window->views[i].image = &window->images[i];
            window->views[i].type = HgGpuViewType_2D;
            window->views[i].aspectFlags = HgGpuAspect_color;
            window->views[i].baseMipLevel = 0;
            window->views[i].levelCount = 1;
            window->views[i].baseArrayLayer = 0;
            window->views[i].layerCount = 1;

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkResult readyToPresentResult = vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &window->readyToPresent[i]);
            if (window->readyToPresent[i] == nullptr)
                hgError("Could not create VkSemaphore: %s\n", vkResultToStr(readyToPresentResult));
        }

        for (u32 i = 0; i < vk.frameCount; ++i)
        {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkResult imageAvailableResult = vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &window->imageAvailable[i]);
            if (window->imageAvailable[i] == nullptr)
                hgError("Could not create VkSemaphore: %s\n", vkResultToStr(imageAvailableResult));
        }
    }
    else
    {
        window->swapchain = nullptr;
    }

    window->imageIdx = (u32)-1;

    vkDestroySwapchainKHR(vk.device, oldSwapchain, nullptr);
}

HgGpuCmd* hgGpuFrameBegin(HgWindow** windows, u32 windowCount)
{
    Frame* frame = &vk.frames[vk.currentFrame];

    vkWaitForFences(vk.device, 1, &frame->fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &frame->fence);

    frame->windows.count = 0;
    for (u32 i = 0; i < windowCount; ++i)
    {
        if (windows[i]->swapchain == nullptr)
            continue;

        VkResult result = vkAcquireNextImageKHR(
            vk.device,
            windows[i]->swapchain,
            UINT64_MAX,
            windows[i]->imageAvailable[vk.currentFrame],
            nullptr,
            &windows[i]->imageIdx);

        if (result == VK_SUCCESS)
        {
            *hgArrayPush(&frame->windows) = windows[i];
        }
        else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            resizeWindowSwapchain(windows[i]);
            windows[i]->imageIdx = (u32)-1;
        }
        else
        {
            hgError("Could not acquire next image: %s\n", vkResultToStr(result));
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
    return (HgGpuCmd*)cmd;
}

void hgGpuFrameEnd(HgGpuCmd* cmd)
{
    hgAssert(cmd != nullptr);

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    Frame* frame = &vk.frames[vk.currentFrame];

    HgArray<HgGpuImageBarrier> presentBarriers = hgArrayTemp<HgGpuImageBarrier>(scratch, 0, frame->windows.count);
    for (u32 i = 0; i < frame->windows.count; ++i)
    {
        HgGpuImageBarrier* barrier = hgArrayPush(&presentBarriers);
        barrier->image = hgWindowImageView(frame->windows[i]);
        barrier->nextLayout = HgGpuLayout_presentSrc;
    }
    hgGpuMemoryBarrier(cmd, nullptr, 0, presentBarriers.vals, presentBarriers.count);

    vkEndCommandBuffer((VkCommandBuffer)cmd);

    VkPipelineStageFlags* waitStages = hgArenaAlloc<VkPipelineStageFlags>(scratch, frame->windows.count);
    VkSemaphore* imageAvailableSemaphores = hgArenaAlloc<VkSemaphore>(scratch, frame->windows.count);
    VkSemaphore* readyToPresentSemaphores = hgArenaAlloc<VkSemaphore>(scratch, frame->windows.count);

    VkSwapchainKHR* swapchains = hgArenaAlloc<VkSwapchainKHR>(scratch, frame->windows.count);
    u32* imageIndices = hgArenaAlloc<u32>(scratch, frame->windows.count);

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
    submit.pCommandBuffers = (VkCommandBuffer*)&cmd;
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

u32 hgPlatformGetVulkanExtensions(HgArena* arena, HgStringView** extBuffer)
{
    u32 extCount;
    const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&extCount);

    *extBuffer = hgArenaAlloc<HgStringView>(arena, extCount);
    for (u32 i = 0; i < extCount; ++i)
    {
        (*extBuffer)[i] = exts[i];
    }

    return extCount;
}

struct WindowState {
    HgPool<HgWindow> pool = {};
    HgMap<SDL_WindowID, HgWindow*> ids = {};

    f32 mouseDX = 0.0f;
    f32 mouseDY = 0.0f;
    bool wasQuit = false;

    bool imguiInitialized = false;
};

static WindowState windowState{};

void hgWindowsInit()
{
    windowState.pool = hgPoolCreate<HgWindow>();
    windowState.ids = hgMapCreate<SDL_WindowID, HgWindow*>();
}

void hgWindowsDeinit()
{
    hgMapDestroy(&windowState.ids);
    hgPoolDestroy(&windowState.pool);
}

static HgFormat findSwapchainFormat(VkSurfaceKHR surface)
{
    hgAssert(surface != nullptr);

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, surface, &formatCount, nullptr);
    VkSurfaceFormatKHR* formats = hgArenaAlloc<VkSurfaceFormatKHR>(scratch, formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, surface, &formatCount, formats);

    for (u32 i = 0; i < formatCount; ++i)
    {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return HgFormat_r8g8b8a8_srgb;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return HgFormat_b8g8r8a8_srgb;
    }
    hgError("No supported swapchain formats\n");
}

static HgGpuPresentMode findSwapchainPresentMode(
    VkSurfaceKHR surface,
    HgGpuPresentMode desiredMode)
{
    hgAssert(surface != nullptr);

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    if (desiredMode == HgGpuPresentMode_fifo)
        return desiredMode;

    u32 modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, surface, &modeCount, nullptr);
    VkPresentModeKHR* presentModes = hgArenaAlloc<VkPresentModeKHR>(scratch, modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, surface, &modeCount, presentModes);

    for (u32 i = 0; i < modeCount; ++i)
    {
        if (presentModes[i] == hgPresentModeToVk(desiredMode))
            return desiredMode;
    }
    return HgGpuPresentMode_fifo;
}

HgWindow* hgWindowCreate(const char* title, u32 width, u32 height, const HgWindowConfig* config)
{
    static const HgWindowConfig defaultConfig{};
    if (config == nullptr)
        config = &defaultConfig;

    HgWindow* window = hgPoolAlloc(&windowState.pool);
    *window = {};

    if (title == nullptr)
        title = "Hurdy Gurdy";

    u64 flags = SDL_WINDOW_VULKAN;
    if (!config->fixedSize)
        flags |= SDL_WINDOW_RESIZABLE;

    if (config->fullscreen)
    {
        int modeCount = 0;
        SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &modeCount);
        hgDefer(SDL_free(modes));

        width = (u32)modes[0]->w;
        height = (u32)modes[0]->h;
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    window->sdlWindow = SDL_CreateWindow(title, (int)width, (int)height, flags);
    if (window->sdlWindow == nullptr)
        hgError("Failed to create SDL window: %s\n", SDL_GetError());

    SDL_WindowID windowID = SDL_GetWindowID(window->sdlWindow);
    hgMapAdd(&windowState.ids, windowID, window);

    SDL_GetWindowSize(window->sdlWindow, (int*)&window->width, (int*)&window->height);

    if (!SDL_Vulkan_CreateSurface(window->sdlWindow, vk.instance, nullptr, &window->surface))
        hgError("Failed to create Vulkan surface: %s\n", SDL_GetError());

    window->format = findSwapchainFormat(window->surface);
    window->presentMode = findSwapchainPresentMode(window->surface, config->preferredPresentMode);
    window->imageUsage = config->imageUsage;

    window->imageAvailable = hgArrayCreate<VkSemaphore>(vk.frameCount, vk.frameCount);
    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        window->imageAvailable[i] = nullptr;
    }

    resizeWindowSwapchain(window);

    window->events = hgArrayCreate<HgWindowEvent>();

    return window;
}

void hgWindowDestroy(HgWindow* window)
{
    hgArrayDestroy(&window->events);

    for (u32 i = 0; i < window->images.count; ++i)
    {
        vkDestroySemaphore(vk.device, window->readyToPresent[i], nullptr);
        vkDestroyImageView(vk.device, window->views[i].view, nullptr);
    }

    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        vkDestroySemaphore(vk.device, window->imageAvailable[i], nullptr);
    }

    vkDestroySwapchainKHR(vk.device, window->swapchain, nullptr);

    hgArrayDestroy(&window->readyToPresent);
    hgArrayDestroy(&window->imageAvailable);
    hgArrayDestroy(&window->views);
    hgArrayDestroy(&window->images);

    SDL_WindowID windowID = SDL_GetWindowID(window->sdlWindow);
    hgMapRemove(&windowState.ids, windowID);

    vkDestroySurfaceKHR(vk.instance, window->surface, nullptr);
    SDL_DestroyWindow(window->sdlWindow);

    hgPoolFree(&windowState.pool, window);
}

HgGpuView* hgWindowImageView(HgWindow* window)
{
    return window->imageIdx < window->images.count ? &window->views[window->imageIdx] : nullptr;
}

HgFormat hgWindowImageFormat(HgWindow* window)
{
    return window->format;
}

static HgButton sdlKeycodeToHgButton(u32 key)
{
    switch (key)
    {
        case SDLK_0:
            return HgButton_k0;
        case SDLK_1:
            return HgButton_k1;
        case SDLK_2:
            return HgButton_k2;
        case SDLK_3:
            return HgButton_k3;
        case SDLK_4:
            return HgButton_k4;
        case SDLK_5:
            return HgButton_k5;
        case SDLK_6:
            return HgButton_k6;
        case SDLK_7:
            return HgButton_k7;
        case SDLK_8:
            return HgButton_k8;
        case SDLK_9:
            return HgButton_k9;

        case SDLK_Q:
            return HgButton_q;
        case SDLK_W:
            return HgButton_w;
        case SDLK_E:
            return HgButton_e;
        case SDLK_R:
            return HgButton_r;
        case SDLK_T:
            return HgButton_t;
        case SDLK_Y:
            return HgButton_y;
        case SDLK_U:
            return HgButton_u;
        case SDLK_I:
            return HgButton_i;
        case SDLK_O:
            return HgButton_o;
        case SDLK_P:
            return HgButton_p;
        case SDLK_A:
            return HgButton_a;
        case SDLK_S:
            return HgButton_s;
        case SDLK_D:
            return HgButton_d;
        case SDLK_F:
            return HgButton_f;
        case SDLK_G:
            return HgButton_g;
        case SDLK_H:
            return HgButton_h;
        case SDLK_J:
            return HgButton_j;
        case SDLK_K:
            return HgButton_k;
        case SDLK_L:
            return HgButton_l;
        case SDLK_Z:
            return HgButton_z;
        case SDLK_X:
            return HgButton_x;
        case SDLK_C:
            return HgButton_c;
        case SDLK_V:
            return HgButton_v;
        case SDLK_B:
            return HgButton_b;
        case SDLK_N:
            return HgButton_n;
        case SDLK_M:
            return HgButton_m;

        case SDLK_SEMICOLON:
            return HgButton_semicolon;
        case SDLK_COLON:
            return HgButton_colon;
        case SDLK_APOSTROPHE:
            return HgButton_apostrophe;
        case SDLK_DBLAPOSTROPHE:
            return HgButton_quotation;
        case SDLK_COMMA:
            return HgButton_comma;
        case SDLK_PERIOD:
            return HgButton_period;
        case SDLK_QUESTION:
            return HgButton_question;
        case SDLK_GRAVE:
            return HgButton_grave;
        case SDLK_TILDE:
            return HgButton_tilde;
        case SDLK_EXCLAIM:
            return HgButton_exclamation;
        case SDLK_AT:
            return HgButton_at;
        case SDLK_HASH:
            return HgButton_hash;
        case SDLK_DOLLAR:
            return HgButton_dollar;
        case SDLK_PERCENT:
            return HgButton_percent;
        case SDLK_CARET:
            return HgButton_carot;
        case SDLK_AMPERSAND:
            return HgButton_ampersand;
        case SDLK_ASTERISK:
            return HgButton_asterisk;

        case SDLK_LEFTPAREN:
            return HgButton_lparen;
        case SDLK_RIGHTPAREN:
            return HgButton_rparen;
        case SDLK_LEFTBRACKET:
            return HgButton_lbracket;
        case SDLK_RIGHTBRACKET:
            return HgButton_rbracket;
        case SDLK_LEFTBRACE:
            return HgButton_lbrace;
        case SDLK_RIGHTBRACE:
            return HgButton_rbrace;

        case SDLK_EQUALS:
            return HgButton_equal;
        case SDLK_LESS:
            return HgButton_less;
        case SDLK_GREATER:
            return HgButton_greater;
        case SDLK_PLUS:
            return HgButton_plus;
        case SDLK_MINUS:
            return HgButton_minus;
        case SDLK_SLASH:
            return HgButton_slash;
        case SDLK_BACKSLASH:
            return HgButton_backslash;
        case SDLK_UNDERSCORE:
            return HgButton_underscore;
        case SDLK_PIPE:
            return HgButton_bar;

        case SDLK_UP:
            return HgButton_up;
        case SDLK_DOWN:
            return HgButton_down;
        case SDLK_LEFT:
            return HgButton_left;
        case SDLK_RIGHT:
            return HgButton_right;

        case SDLK_ESCAPE:
            return HgButton_escape;
        case SDLK_SPACE:
            return HgButton_space;
        case SDLK_RETURN:
            return HgButton_enter;
        case SDLK_BACKSPACE:
            return HgButton_backspace;
        case SDLK_DELETE:
            return HgButton_kdelete;
        case SDLK_INSERT:
            return HgButton_insert;
        case SDLK_TAB:
            return HgButton_tab;
        case SDLK_HOME:
            return HgButton_home;
        case SDLK_END:
            return HgButton_end;

        case SDLK_F1:
            return HgButton_f1;
        case SDLK_F2:
            return HgButton_f2;
        case SDLK_F3:
            return HgButton_f3;
        case SDLK_F4:
            return HgButton_f4;
        case SDLK_F5:
            return HgButton_f5;
        case SDLK_F6:
            return HgButton_f6;
        case SDLK_F7:
            return HgButton_f7;
        case SDLK_F8:
            return HgButton_f8;
        case SDLK_F9:
            return HgButton_f9;
        case SDLK_F10:
            return HgButton_f10;
        case SDLK_F11:
            return HgButton_f11;
        case SDLK_F12:
            return HgButton_f12;

        case SDLK_LSHIFT:
            return HgButton_lshift;
        case SDLK_RSHIFT:
            return HgButton_rshift;
        case SDLK_LCTRL:
            return HgButton_lctrl;
        case SDLK_RCTRL:
            return HgButton_rctrl;
        case SDLK_LALT:
            return HgButton_lalt;
        case SDLK_RALT:
            return HgButton_ralt;
        case SDLK_LGUI:
            return HgButton_lsuper;
        case SDLK_RGUI:
            return HgButton_rsuper;
        case SDLK_CAPSLOCK:
            return HgButton_capslock;
    }
    return HgButton_none;
}

static HgButton sdlButtonToHgButton(u32 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT:
            return HgButton_mouse1;
        case SDL_BUTTON_RIGHT:
            return HgButton_mouse2;
        case SDL_BUTTON_MIDDLE:
            return HgButton_mouse3;
        case SDL_BUTTON_X1:
            return HgButton_mouse4;
        case SDL_BUTTON_X2:
            return HgButton_mouse5;
    }
    return HgButton_none;
}

void hgProcessEvents()
{
    windowState.mouseDX = 0;
    windowState.mouseDY = 0;

    hgMapForEach(&windowState.ids, [&](SDL_WindowID*, HgWindow** window)
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
                HgWindow** window = hgMapGet(&windowState.ids, event.window.windowID);
                if (window != nullptr)
                    (*window)->wasClosed = true;
            } break;
            case SDL_EVENT_WINDOW_MINIMIZED: [[fallthrough]];
            case SDL_EVENT_WINDOW_RESTORED: [[fallthrough]];
            case SDL_EVENT_WINDOW_RESIZED:
            {
                HgWindow** window = hgMapGet(&windowState.ids, event.window.windowID);
                if (window != nullptr)
                {
                    SDL_GetWindowSize((*window)->sdlWindow, (int*)&(*window)->width, (int*)&(*window)->height);
                    resizeWindowSwapchain(*window);
                }
            } break;
            case SDL_EVENT_MOUSE_MOTION:
            {
                HgWindow** window = hgMapGet(&windowState.ids, event.button.windowID);
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
                HgButton key = sdlKeycodeToHgButton(event.key.key);
                HgWindow** window = hgMapGet(&windowState.ids, event.key.windowID);
                if (window != nullptr)
                {
                    HgWindowEvent windowEvent{};
                    windowEvent.button.type = HgWindowEventType_buttonPress;
                    windowEvent.button.button = key;

                    *hgArrayPush(&(*window)->events) = windowEvent;
                    (*window)->isKeyDown[key] = true;
                }
            } break;
            case SDL_EVENT_KEY_UP:
            {
                HgButton key = sdlKeycodeToHgButton(event.key.key);
                HgWindow** window = hgMapGet(&windowState.ids, event.key.windowID);
                if (window != nullptr)
                {
                    HgWindowEvent windowEvent{};
                    windowEvent.button.type = HgWindowEventType_buttonRelease;
                    windowEvent.button.button = key;

                    *hgArrayPush(&(*window)->events) = windowEvent;
                    (*window)->isKeyDown[key] = false;
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                HgButton key = sdlButtonToHgButton(event.button.button);
                HgWindow** window = hgMapGet(&windowState.ids, event.button.windowID);
                if (window != nullptr)
                {
                    HgWindowEvent windowEvent{};
                    windowEvent.button.type = HgWindowEventType_buttonPress;
                    windowEvent.button.button = key;

                    *hgArrayPush(&(*window)->events) = windowEvent;
                    (*window)->isKeyDown[key] = true;
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                HgButton key = sdlButtonToHgButton(event.button.button);
                HgWindow** window = hgMapGet(&windowState.ids, event.button.windowID);
                if (window != nullptr)
                {
                    HgWindowEvent windowEvent{};
                    windowEvent.button.type = HgWindowEventType_buttonRelease;
                    windowEvent.button.button = key;

                    *hgArrayPush(&(*window)->events) = windowEvent;
                    (*window)->isKeyDown[key] = false;
                }
            } break;
        }
    }
}

bool hgWasQuit()
{
    return windowState.wasQuit;
}

bool hgWindowWasClosed(HgWindow* window)
{
    hgAssert(window != nullptr);
    return window->wasClosed;
}

bool hgWindowIsFocused(HgWindow* window)
{
    hgAssert(window != nullptr);
    return SDL_GetMouseFocus() == window->sdlWindow;
}

u32 hgWindowWidth(HgWindow* window)
{
    hgAssert(window != nullptr);
    return window->width;
}

u32 hgWindowHeight(HgWindow* window)
{
    hgAssert(window != nullptr);
    return window->height;
}

f32 hgMouseX(HgWindow* window)
{
    hgAssert(window != nullptr);
    return window->mouseX;
}

f32 hgMouseY(HgWindow* window)
{
    hgAssert(window != nullptr);
    return window->mouseY;
}

f32 hgMouseDeltaX(HgWindow* window)
{
    hgAssert(window != nullptr);
    return windowState.mouseDX / (f32)window->height;
}

f32 hgMouseDeltaY(HgWindow* window)
{
    hgAssert(window != nullptr);
    return windowState.mouseDY / (f32)window->height;
}

bool hgIsButtonDown(HgWindow* window, HgButton key)
{
    hgAssert(window != nullptr);
    return window->isKeyDown[key];
}

HgWindowEvent* hgWindowEvents(HgWindow* window, u32* count)
{
    hgAssert(window != nullptr);
    hgAssert(count != nullptr);
    *count = window->events.count;
    return window->events.vals;
}

struct AudioState {
    SDL_AudioDeviceID device;
    HgArray<SDL_AudioStream*> streams;
};

static AudioState audio{};

void hgAudioInit()
{
    audio.device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (audio.device == 0)
        hgError("SDL could not open audio device: %s\n", SDL_GetError());

    audio.streams = hgArrayCreate<SDL_AudioStream*>();
}

void hgAudioDeinit()
{
    for (u32 i = 0; i < audio.streams.count; ++i)
    {
        SDL_DestroyAudioStream(audio.streams[i]);
    }
    hgArrayDestroy(&audio.streams);

    SDL_CloseAudioDevice(audio.device);
}

HgAudioStream* hgAudioStreamCreate(u32 frequency, u32 channels)
{
    SDL_AudioSpec audioSpec{};
    audioSpec.format = SDL_AUDIO_F32;
    audioSpec.freq = (int)frequency;
    audioSpec.channels = (int)channels;

    SDL_AudioStream* stream;
    if (audio.streams.count == 0)
    {
        stream = SDL_CreateAudioStream(&audioSpec, nullptr);
        if (stream == nullptr)
            hgError("SDL could not create audio stream: %s\n", SDL_GetError());

        if (!SDL_BindAudioStream(audio.device, stream))
            hgError("SDL could not bind audio stream: %s\n", SDL_GetError());
    }
    else
    {
        stream = hgArrayPop(&audio.streams);
        if (!SDL_SetAudioStreamFormat(stream, &audioSpec, nullptr))
            hgError("SDL could not set audio stream format: %s\n", SDL_GetError());
    }

    return (HgAudioStream*)stream;
}

void hgAudioStreamDestroy(HgAudioStream* stream)
{
    if (stream != nullptr)
    {
        SDL_AudioStream* sdlStream = (SDL_AudioStream*)stream;

        // SDL_UnbindAudioStream(sdlStream);
        if (!SDL_ClearAudioStream(sdlStream))
            hgError("SDL could not clear audio stream: %s\n", SDL_GetError());

        *hgArrayPush(&audio.streams) = sdlStream;
    }
}

void hgAudioStreamPush(HgAudioStream* player, const f32* data, u64 size)
{
    SDL_AudioStream* stream = (SDL_AudioStream*)player;
    if (!SDL_PutAudioStreamData(stream, data, (int)size))
        hgError("SDL could not push audio data: %s\n", SDL_GetError());
}

u32 hgAudioStreamQueuedSize(HgAudioStream* stream)
{
    int size = SDL_GetAudioStreamQueued((SDL_AudioStream*)stream);
    if (size == -1)
        hgError("SDL could not read audio data: %s\n", SDL_GetError());

    return (u32)size;
}

void hgAudioStreamClear(HgAudioStream* stream)
{
    if (!SDL_ClearAudioStream((SDL_AudioStream*)stream))
        hgError("SDL could not clear audio stream: %s\n", SDL_GetError());
}

void hgAudioStreamSetGain(HgAudioStream* stream, f32 gain)
{
    if (!SDL_SetAudioStreamGain((SDL_AudioStream*)stream, gain))
        hgError("SDL could not clear audio stream: %s\n", SDL_GetError());
}

void hgImGuiInit(
    HgWindow* window,
    HgFormat colorFormat,
    HgFormat depthFormat,
    HgFormat stencilFormat)
{
    hgAssert(window != nullptr);
    hgAssert(colorFormat != HgFormat_undefined);

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    ImGui_ImplSDL3_InitForVulkan(window->sdlWindow);

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
    imguiInfo.MinImageCount = window->images.count;
    imguiInfo.ImageCount = window->images.count;
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

void hgImGuiDeinit()
{
    windowState.imguiInitialized = false;

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void* hgImGuiTextureCreate(HgGpuView* view, HgGpuLayout layout)
{
    hgAssert(view != nullptr);
    return ImGui_ImplVulkan_AddTexture(view->sampler, view->view, gpuLayoutToVk(layout));
}

void hgImGuiTextureDestroy(void* texture)
{
    ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)texture);
}

void hgImGuiNewFrame()
{
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
}

void hgImGuiDraw(HgGpuCmd* cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), (VkCommandBuffer)cmd);
}

#define HG_MAKE_VULKAN_FUNC(name) PFN_##name name

struct HgVulkanFuncs {
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

static HgVulkanFuncs vulkanFuncs{};

static HgLibrary* libvulkan = nullptr;

#define HG_LOAD_VULKAN_FUNC(name) \
    vulkanFuncs. name = (PFN_##name)vulkanFuncs.vkGetInstanceProcAddr(nullptr, #name); \
    if (vulkanFuncs. name == nullptr) { hgError("Could not load " #name "\n"); }

static void loadVulkan()
{
    if (libvulkan == nullptr)
        libvulkan = hgLibraryLoad(
#if defined(HG_PLATFORM_LINUX)
            "libvulkan.so.1"
#elif defined(HG_PLATFORM_WINDOWS)
            "vulkan-1.dll"
#endif
        );

    if (libvulkan == nullptr)
        hgError("Could not load vulkan\n");

    *(void**)&vulkanFuncs.vkGetInstanceProcAddr = hgLibraryFindFunction(libvulkan, "vkGetInstanceProcAddr");
    if (vulkanFuncs.vkGetInstanceProcAddr == nullptr)
        hgError("Could not load vkGetInstanceProcAddr\n");

    HG_LOAD_VULKAN_FUNC(vkCreateInstance);
}

#undef HG_LOAD_VULKAN_FUNC

static void unloadVulkan()
{
    if (libvulkan != nullptr)
    {
        hgLibraryUnload(libvulkan);
        libvulkan = nullptr;
    }
}

#define HG_LOAD_VULKAN_INSTANCE_FUNC(instance, name) \
    vulkanFuncs. name = (PFN_##name)vulkanFuncs.vkGetInstanceProcAddr(instance, #name); \
    if (vulkanFuncs. name == nullptr) { hgError("Could not load " #name "\n"); }

static void loadVulkanInstanceFuncs(VkInstance instance)
{
    hgAssert(instance != nullptr);

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
}

#undef HG_LOAD_VULKAN_INSTANCE_FUNC

#define HG_LOAD_VULKAN_DEVICE_FUNC(device, name) \
    vulkanFuncs. name = (PFN_##name)vulkanFuncs.vkGetDeviceProcAddr(device, #name); \
    if (vulkanFuncs. name == nullptr) { hgError("Could not load " #name "\n"); }

static void loadVulkanDeviceFuncs(VkDevice device)
{
    hgAssert(device != nullptr);

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
}

#undef HG_LOAD_VULKAN_DEVICE_FUNC

PFN_vkVoidFunction vkGetInstanceProcAddr(
    VkInstance instance,
    const char* pName)
{
    return vulkanFuncs.vkGetInstanceProcAddr(
        instance,
        pName);
}

PFN_vkVoidFunction vkGetDeviceProcAddr(
    VkDevice device,
    const char* pName)
{
    return vulkanFuncs.vkGetDeviceProcAddr(
        device,
        pName);
}

VkResult vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance)
{
    return vulkanFuncs.vkCreateInstance(
        pCreateInfo,
        pAllocator,
        pInstance);
}

void vkDestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator)
{
    vulkanFuncs.vkDestroyInstance(
        instance,
        pAllocator);
}

VkResult vkCreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger)
{
    return vulkanFuncs.vkCreateDebugUtilsMessengerEXT(
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
    vulkanFuncs.vkDestroyDebugUtilsMessengerEXT(
        instance,
        messenger,
        pAllocator);
}

VkResult vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pCount,
    VkPhysicalDevice* pDevices)
{
    return vulkanFuncs.vkEnumeratePhysicalDevices(
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
    return vulkanFuncs.vkEnumerateDeviceExtensionProperties(
        device,
        pLayerName,
        pCount,
        pProps);
}

void vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties)
{
    vulkanFuncs.vkGetPhysicalDeviceProperties(
        physicalDevice,
        pProperties);
}

void vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice device,
    uint32_t* pCount,
    VkQueueFamilyProperties* pProps)
{
    vulkanFuncs.vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        pCount,
        pProps);
}

void vkDestroySurfaceKHR(
    VkInstance instance,
    VkSurfaceKHR surface,
    const VkAllocationCallbacks* pAllocator)
{
    vulkanFuncs.vkDestroySurfaceKHR(
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
    return vulkanFuncs.vkCreateDevice(
        device,
        pCreateInfo,
        pAllocator,
        pDevice);
}

void vkDestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator)
{
    vulkanFuncs.vkDestroyDevice(
        device,
        pAllocator);
}

VkResult vkDeviceWaitIdle(
    VkDevice device)
{
    return vulkanFuncs.vkDeviceWaitIdle(
        device);
}

VkResult vkGetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    VkSurfaceKHR surface,
    VkBool32* pSupported)
{
    return vulkanFuncs.vkGetPhysicalDeviceSurfaceSupportKHR(
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
    return vulkanFuncs.vkGetPhysicalDeviceSurfaceFormatsKHR(
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
    return vulkanFuncs.vkGetPhysicalDeviceSurfacePresentModesKHR(
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
    return vulkanFuncs.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
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
    return vulkanFuncs.vkCreateSwapchainKHR(
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
    vulkanFuncs.vkDestroySwapchainKHR(
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
    return vulkanFuncs.vkGetSwapchainImagesKHR(
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
    return vulkanFuncs.vkAcquireNextImageKHR(
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
    return vulkanFuncs.vkCreateSemaphore(
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
    vulkanFuncs.vkDestroySemaphore(
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
    return vulkanFuncs.vkCreateFence(
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
    vulkanFuncs.vkDestroyFence(
        device,
        fence,
        pAllocator);
}

VkResult vkResetFences(
    VkDevice device,
    uint32_t count,
    const VkFence* pFences)
{
    return vulkanFuncs.vkResetFences(
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
    return vulkanFuncs.vkWaitForFences(
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
    vulkanFuncs.vkGetDeviceQueue(
        device,
        family,
        index,
        pQueue);
}

VkResult vkQueueWaitIdle(
    VkQueue queue)
{
    return vulkanFuncs.vkQueueWaitIdle(
        queue);
}

VkResult vkQueueSubmit(
    VkQueue queue,
    uint32_t count,
    const VkSubmitInfo* pSubmits,
    VkFence fence)
{
    return vulkanFuncs.vkQueueSubmit(
        queue,
        count,
        pSubmits,
        fence);
}

VkResult vkQueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pInfo)
{
    return vulkanFuncs.vkQueuePresentKHR(
        queue,
        pInfo);
}

VkResult vkCreateCommandPool(
    VkDevice device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkCommandPool* pPool)
{
    return vulkanFuncs.vkCreateCommandPool(
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
    vulkanFuncs.vkDestroyCommandPool(
        device,
        pool,
        pAllocator);
}

VkResult vkResetCommandPool(
    VkDevice device,
    VkCommandPool commandPool,
    VkCommandPoolResetFlags flags)
{
    return vulkanFuncs.vkResetCommandPool(
        device,
        commandPool,
        flags);
}

VkResult vkAllocateCommandBuffers(
    VkDevice device,
    const VkCommandBufferAllocateInfo* pInfo,
    VkCommandBuffer* pBufs)
{
    return vulkanFuncs.vkAllocateCommandBuffers(
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
    vulkanFuncs.vkFreeCommandBuffers(
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
    return vulkanFuncs.vkCreateDescriptorPool(
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
    vulkanFuncs.vkDestroyDescriptorPool(
        device,
        pool,
        pAllocator);
}

VkResult vkResetDescriptorPool(
    VkDevice device,
    VkDescriptorPool pool,
    uint32_t flags)
{
    return vulkanFuncs.vkResetDescriptorPool(
        device,
        pool,
        flags);
}

VkResult vkAllocateDescriptorSets(
    VkDevice device,
    const VkDescriptorSetAllocateInfo* pInfo,
    VkDescriptorSet* pSets)
{
    return vulkanFuncs.vkAllocateDescriptorSets(
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
    return vulkanFuncs.vkFreeDescriptorSets(
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
    vulkanFuncs.vkUpdateDescriptorSets(
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
    return vulkanFuncs.vkCreateDescriptorSetLayout(
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
    vulkanFuncs.vkDestroyDescriptorSetLayout(
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
    return vulkanFuncs.vkCreatePipelineLayout(
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
    vulkanFuncs.vkDestroyPipelineLayout(
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
    return vulkanFuncs.vkCreateShaderModule(
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
    vulkanFuncs.vkDestroyShaderModule(
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
    return vulkanFuncs.vkCreateGraphicsPipelines(
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
    return vulkanFuncs.vkCreateComputePipelines(
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
    vulkanFuncs.vkDestroyPipeline(
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
    return vulkanFuncs.vkCreateRenderPass(
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
    vulkanFuncs.vkDestroyRenderPass(
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
    return vulkanFuncs.vkCreateFramebuffer(
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
    vulkanFuncs.vkDestroyFramebuffer(
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
    return vulkanFuncs.vkCreateBuffer(
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
    vulkanFuncs.vkDestroyBuffer(
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
    return vulkanFuncs.vkCreateImage(
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
    vulkanFuncs.vkDestroyImage(
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
    return vulkanFuncs.vkCreateImageView(
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
    vulkanFuncs.vkDestroyImageView(
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
    return vulkanFuncs.vkCreateSampler(
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
    vulkanFuncs.vkDestroySampler(
        device,
        sampler,
        pAllocator);
}

void vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    vulkanFuncs.vkGetPhysicalDeviceMemoryProperties(
        physicalDevice,
        pMemoryProperties);
}

void vkGetPhysicalDeviceMemoryProperties2(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2*pMemoryProperties)
{
    vulkanFuncs.vkGetPhysicalDeviceMemoryProperties2(
        physicalDevice,
        pMemoryProperties);
}

void vkGetBufferMemoryRequirements(
    VkDevice device,
    VkBuffer buffer,
    VkMemoryRequirements* pMemoryRequirements)
{
    vulkanFuncs.vkGetBufferMemoryRequirements(
        device,
        buffer,
        pMemoryRequirements);
}

void vkGetBufferMemoryRequirements2(
    VkDevice device,
    const VkBufferMemoryRequirementsInfo2* pInfo,
    VkMemoryRequirements2* pMemoryRequirements)
{
    vulkanFuncs.vkGetBufferMemoryRequirements2(
        device,
        pInfo,
        pMemoryRequirements);
}

void vkGetImageMemoryRequirements(
    VkDevice device,
    VkImage image,
    VkMemoryRequirements* pMemoryRequirements)
{
    vulkanFuncs.vkGetImageMemoryRequirements(
        device,
        image,
        pMemoryRequirements);
}

void vkGetImageMemoryRequirements2(
    VkDevice device,
    const VkImageMemoryRequirementsInfo2* pInfo,
    VkMemoryRequirements2* pMemoryRequirements)
{
    vulkanFuncs.vkGetImageMemoryRequirements2(
        device,
        pInfo,
        pMemoryRequirements);
}

void vkGetDeviceBufferMemoryRequirements(
    VkDevice device,
    const VkDeviceBufferMemoryRequirements* pInfo,
    VkMemoryRequirements2* pMemoryRequirements)
{
    vulkanFuncs.vkGetDeviceBufferMemoryRequirements(
        device,
        pInfo,
        pMemoryRequirements);
}

void vkGetDeviceImageMemoryRequirements(
    VkDevice device,
    const VkDeviceImageMemoryRequirements* pInfo,
    VkMemoryRequirements2* pMemoryRequirements)
{
    vulkanFuncs.vkGetDeviceImageMemoryRequirements(
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
    return vulkanFuncs.vkAllocateMemory(
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
    vulkanFuncs.vkFreeMemory(
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
    return vulkanFuncs.vkBindBufferMemory(
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
    return vulkanFuncs.vkBindBufferMemory2(
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
    return vulkanFuncs.vkBindImageMemory(
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
    return vulkanFuncs.vkBindImageMemory2(
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
    return vulkanFuncs.vkMapMemory(
        device,
        mem,
        offset, size, flags, ppData);
}

void vkUnmapMemory(
    VkDevice device,
    VkDeviceMemory mem)
{
    vulkanFuncs.vkUnmapMemory(
        device,
        mem);
}

VkResult vkFlushMappedMemoryRanges(
    VkDevice device,
    uint32_t count,
    const VkMappedMemoryRange* pRanges)
{
    return vulkanFuncs.vkFlushMappedMemoryRanges(
        device,
        count,
        pRanges);
}

VkResult vkInvalidateMappedMemoryRanges(
    VkDevice device,
    uint32_t count,
    const VkMappedMemoryRange* pRanges)
{
    return vulkanFuncs.vkInvalidateMappedMemoryRanges(
        device,
        count,
        pRanges);
}

VkResult vkBeginCommandBuffer(
    VkCommandBuffer cmd,
    const VkCommandBufferBeginInfo* pInfo)
{
    return vulkanFuncs.vkBeginCommandBuffer(
        cmd,
        pInfo);
}

VkResult vkEndCommandBuffer(
    VkCommandBuffer cmd)
{
    return vulkanFuncs.vkEndCommandBuffer(
        cmd);
}

VkResult vkResetCommandBuffer(
    VkCommandBuffer cmd,
    VkCommandBufferResetFlags flags)
{
    return vulkanFuncs.vkResetCommandBuffer(
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
    vulkanFuncs.vkCmdCopyBuffer(
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
    vulkanFuncs.vkCmdCopyImage(
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
    vulkanFuncs.vkCmdBlitImage(
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
    vulkanFuncs.vkCmdCopyBufferToImage(
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
    vulkanFuncs.vkCmdCopyImageToBuffer(
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
    vulkanFuncs.vkCmdPipelineBarrier2(
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
    vulkanFuncs.vkCmdPipelineBarrier(
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
    vulkanFuncs.vkCmdBeginRendering(
        cmd,
        pInfo);
}

void vkCmdEndRendering(
    VkCommandBuffer cmd)
{
    vulkanFuncs.vkCmdEndRendering(
        cmd);
}

void vkCmdBeginRenderPass(
    VkCommandBuffer cmd,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkSubpassContents contents)
{
    vulkanFuncs.vkCmdBeginRenderPass(
        cmd,
        pRenderPassBegin,
        contents);
}

void vkCmdEndRenderPass(
    VkCommandBuffer cmd)
{
    vulkanFuncs.vkCmdEndRenderPass(
        cmd);
}

void vkCmdSetViewport(
    VkCommandBuffer cmd,
    uint32_t first,
    uint32_t count,
    const VkViewport* pViewports)
{
    vulkanFuncs.vkCmdSetViewport(
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
    vulkanFuncs.vkCmdSetScissor(
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
    vulkanFuncs.vkCmdBindPipeline(
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
    vulkanFuncs.vkCmdBindDescriptorSets(
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
    vulkanFuncs.vkCmdPushConstants(
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
    vulkanFuncs.vkCmdBindVertexBuffers(
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
    vulkanFuncs.vkCmdBindIndexBuffer(
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
    vulkanFuncs.vkCmdDraw(
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
    vulkanFuncs.vkCmdDrawIndexed(
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
    vulkanFuncs.vkCmdDispatch(
        cmd,
        x,
        y,
        z);
}

#if defined(HG_PLATFORM_LINUX)

#include <dlfcn.h>

HgLibrary* hgLibraryLoad(HgStringView path)
{
    char* cstr = hgCString(hgScratch(), path);

    HgLibrary* lib = (HgLibrary*)dlopen(cstr, RTLD_LAZY);
    if (lib == nullptr)
        hgWarn("Could not load dynamic library \"%s\": %s\n", cstr, dlerror());

    return lib;
}

void hgLibraryUnload(HgLibrary* lib)
{
    if (lib != nullptr)
        dlclose(lib);
}

void* hgLibraryFindFunction(HgLibrary* lib, HgStringView symbol)
{
    char* cstr = hgCString(hgScratch(), symbol);

    void* fn = dlsym(lib, cstr);
    if (lib == nullptr)
        hgWarn("Could not load function symbol \"%s\": %s\n", cstr, dlerror());

    return fn;
}

#elif defined(HG_PLATFORM_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

HgLibrary* hgLibraryLoad(HgStringView path)
{
    char* cstr = hgCString(hgScratch(), path);

    HgLibrary* lib = (HgLibrary*)LoadLibraryA(cstr);
    if (lib == nullptr)
        hgWarn("Could not load dynamic library \"%s\"\n", cstr);

    return lib;
}

void hgLibraryUnload(HgLibrary* lib)
{
    if (lib != nullptr)
        FreeLibrary((HMODULE)lib);
}

void* hgLibraryFindFunction(HgLibrary* lib, HgStringView symbol)
{
    char* cstr = hgCString(hgScratch(), symbol);

    void* fn = GetProcAddress((HMODULE)lib, cstr);
    if (lib == nullptr)
        hgWarn("Could not load function symbol \"%s\"\n", cstr);

    return fn;
}

#endif

