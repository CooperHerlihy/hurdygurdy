#include "hurdygurdy.hpp"

void hgVulkanInit();

#ifdef HG_VK_DEBUG_MESSENGER
VkDebugUtilsMessengerEXT vkDebugMessenger = nullptr;
#endif

void hgVkCreateVmaAllocator();

void hgInitGraphics()
{
    hgVulkanInit();

    if (hgVkInstance == nullptr)
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};

        HgStringView* exts;
        u32 extCount = hgVkGetPlatformExtensions(scratch, &exts);
#ifdef HG_VK_DEBUG_MESSENGER
        exts = hgRealloc(scratch, exts, extCount, extCount + 1);
        exts[extCount++] = "VK_EXT_debug_utils";
#endif
        hgVkInstance = hgVkCreateInstance(exts, extCount);
        hgVkLoadInstanceFuncs(hgVkInstance);
    }

#ifdef HG_VK_DEBUG_MESSENGER
    if (vkDebugMessenger == nullptr)
        vkDebugMessenger = hgVkCreateDebugMessenger();
#endif

    if (hgVkPhysicalDevice == nullptr)
    {
        hgVkPhysicalDevice = hgVkFindSingleQueuePhysicalDevice();
        hgVkFindQueueFamily(hgVkPhysicalDevice, &hgVkQueueFamily,
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT);
    }

    if (hgVkDevice == nullptr)
    {
        hgVkDevice = hgVkCreateSingleQueueDevice();
        hgVkLoadDeviceFuncs(hgVkDevice);
        vkGetDeviceQueue(hgVkDevice, hgVkQueueFamily, 0, &hgVkQueue);
    }

    if (hgVkVma == nullptr)
    {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = hgVkPhysicalDevice;
        allocatorInfo.device = hgVkDevice;
        allocatorInfo.instance = hgVkInstance;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        VkResult result = vmaCreateAllocator(&allocatorInfo, &hgVkVma);
        if (hgVkVma == nullptr)
            hgError("Could note create Vulkan memory allocator: %s\n", hgVkResultToStr(result));
    }

    if (hgVkCmdPool == nullptr)
    {
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolInfo.queueFamilyIndex = hgVkQueueFamily;

        VkResult result = vkCreateCommandPool(hgVkDevice, &cmdPoolInfo, nullptr, &hgVkCmdPool);
        if (hgVkCmdPool == nullptr)
            hgError("Could note create Vulkan command pool: %s\n", hgVkResultToStr(result));
    }
}

void hgVulkanDeinit();

void hgDeinitGraphics()
{
    if (hgVkCmdPool != nullptr)
    {
        vkDestroyCommandPool(hgVkDevice, hgVkCmdPool, nullptr);
        hgVkCmdPool = nullptr;
    }

    if (hgVkVma != nullptr)
    {
        vmaDestroyAllocator(hgVkVma);
        hgVkVma = nullptr;
    }

    if (hgVkDevice != nullptr)
    {
        vkDestroyDevice(hgVkDevice, nullptr);
        hgVkDevice = nullptr;
    }

    if (hgVkPhysicalDevice != nullptr)
    {
        hgVkPhysicalDevice = nullptr;
        hgVkQueueFamily = (u32)-1;
    }

#ifdef HG_VK_DEBUG_MESSENGER
    if (vkDebugMessenger != nullptr)
    {
        vkDestroyDebugUtilsMessengerEXT(hgVkInstance, vkDebugMessenger, nullptr);
        vkDebugMessenger = nullptr;
    }
#endif

    if (hgVkInstance != nullptr)
    {
        vkDestroyInstance(hgVkInstance, nullptr);
        hgVkInstance = nullptr;
    }

    hgVulkanDeinit();
}

const char* hgVkResultToStr(VkResult result)
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

u32 hgVkFormatToSize(VkFormat format)
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
            hgWarn("Unrecognized Vulkan format value\n");
            return 0;
    }
}

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

VkInstance hgVkCreateInstance(HgStringView* extensions, u32 extensionCount)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

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
    instanceInfo.enabledLayerCount = sizeof(layers) / sizeof(*layers);
    instanceInfo.ppEnabledLayerNames = layers;
#endif

    const char** extCStrs = hgAlloc<const char*>(scratch, extensionCount);
    for (usize i = 0; i < extensionCount; ++i)
    {
        extCStrs[i] = hgCString(scratch, extensions[i]);
    }
    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = extCStrs;

    VkInstance instance = nullptr;
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
    if (instance == nullptr)
        hgError("Failed to create Vulkan instance: %s\n", hgVkResultToStr(result));

    return instance;
}

VkDebugUtilsMessengerEXT hgVkCreateDebugMessenger()
{
#ifdef HG_VK_DEBUG_MESSENGER
    hgAssert(hgVkInstance != nullptr);

    VkDebugUtilsMessengerEXT messenger = nullptr;
    VkResult result = vkCreateDebugUtilsMessengerEXT(
        hgVkInstance, &debugUtilsMessengerInfo, nullptr, &messenger);
    if (messenger == nullptr)
        hgError("Failed to create Vulkan debug messenger: %s\n", hgVkResultToStr(result));

    return messenger;
#else
    return nullptr;
#endif
}

bool hgVkFindQueueFamily(VkPhysicalDevice gpu, u32* queueFamily, VkQueueFlags queueFlags)
{
    hgAssert(gpu != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    u32 familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
    VkQueueFamilyProperties* families = hgAlloc<VkQueueFamilyProperties>(scratch, familyCount);
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

static const char* const vkDeviceExtensions[]{
    "VK_KHR_swapchain",
};

VkPhysicalDevice hgVkFindSingleQueuePhysicalDevice()
{
    hgAssert(hgVkInstance != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    u32 gpuCount;
    vkEnumeratePhysicalDevices(hgVkInstance, &gpuCount, nullptr);
    VkPhysicalDevice* gpus = hgAlloc<VkPhysicalDevice>(scratch, gpuCount);
    vkEnumeratePhysicalDevices(hgVkInstance, &gpuCount, gpus);

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
            extProps = hgRealloc(scratch, extProps, extPropCount, newPropCount);
            extPropCount = newPropCount;
        }
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &newPropCount, extProps);

        for (usize j = 0; j < sizeof(vkDeviceExtensions) / sizeof(*vkDeviceExtensions); j++)
        {
            for (usize k = 0; k < newPropCount; k++)
            {
                if (strcmp(vkDeviceExtensions[j], extProps[k].extensionName) == 0)
                    goto nextExt;
            }
            goto nextGpu;
nextExt:
            continue;
        }

        if (!hgVkFindQueueFamily(gpu, &family,
                VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT))
            goto nextGpu;

        return gpu;

nextGpu:
        continue;
    }

    hgWarn("Could not find a suitable gpu\n");
    return nullptr;
}

VkDevice hgVkCreateSingleQueueDevice()
{
    hgAssert(hgVkPhysicalDevice != nullptr);
    hgAssert(hgVkQueueFamily != (u32)-1);

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{};
    dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features synchronization2Feature{};
    synchronization2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2Feature.pNext = &dynamicRenderingFeature;
    synchronization2Feature.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures features{};

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = hgVkQueueFamily;
    queueInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = &synchronization2Feature;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = sizeof(vkDeviceExtensions) / sizeof(*vkDeviceExtensions);
    deviceInfo.ppEnabledExtensionNames = vkDeviceExtensions;
    deviceInfo.pEnabledFeatures = &features;

    VkDevice device = nullptr;
    VkResult result = vkCreateDevice(hgVkPhysicalDevice, &deviceInfo, nullptr, &device);

    if (device == nullptr)
        hgError("Could not create Vulkan device: %s\n", hgVkResultToStr(result));
    return device;
}

VkCommandBuffer hgVkBeginCommands()
{
    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = hgVkCmdPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(hgVkDevice, &cmdInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    return cmd;
}

void hgVkEndAndExecute(VkCommandBuffer cmd)
{
    hgAssert(cmd != nullptr);
    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;
    vkCreateFence(hgVkDevice, &fenceInfo, nullptr, &fence);
    hgDefer(vkDestroyFence(hgVkDevice, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(hgVkQueue, 1, &submit, fence);
    vkWaitForFences(hgVkDevice, 1, &fence, VK_TRUE, UINT64_MAX);

    vkFreeCommandBuffers(hgVkDevice, hgVkCmdPool, 1, &cmd);
}

VkDescriptorSetLayout hgVkCreateDescriptorSetLayout(VkDescriptorSetLayoutBinding* bindings, u32 bindingCount)
{
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = bindingCount;
    info.pBindings = bindings;

    VkDescriptorSetLayout layout = nullptr;
    VkResult result = vkCreateDescriptorSetLayout(hgVkDevice, &info, nullptr, &layout);
    if (layout == nullptr)
        hgError("Could not create VkDescriptorSetLayout: %s\n", hgVkResultToStr(result));

    return layout;
}

VkPipelineLayout hgVkCreatePipelineLayout(
    VkDescriptorSetLayout* setLayouts,
    u32 setLayoutCount,
    VkPushConstantRange* pushRanges,
    u32 pushRangeCount)
{
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = setLayoutCount;
    info.pSetLayouts = setLayouts;
    info.pushConstantRangeCount = pushRangeCount;
    info.pPushConstantRanges = pushRanges;

    VkPipelineLayout layout = nullptr;
    VkResult result = vkCreatePipelineLayout(hgVkDevice, &info, nullptr, &layout);
    if (layout == nullptr)
        hgError("Could not create VkPipelineLayout: %s\n", hgVkResultToStr(result));

    return layout;
}

VkShaderModule hgVkCreateShaderModule(const u8* spirvCode, usize codeSize)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = codeSize;
    info.pCode = (const u32*)spirvCode;

    VkShaderModule shader = nullptr;
    VkResult result = vkCreateShaderModule(hgVkDevice, &info, nullptr, &shader);
    if (shader == nullptr)
        hgError("Could not create VkShaderModule: %s\n", hgVkResultToStr(result));

    return shader;
}

VkPipeline hgVkCreateGraphicsPipeline(const HgVkPipelineConfig& config)
{
    if (config.colorAttachmentCount > 0)
        hgAssert(config.colorAttachmentFormats != nullptr);
    hgAssert(config.vertexShader != nullptr);
    hgAssert(config.fragmentShader != nullptr);
    hgAssert(config.layout != nullptr);
    if (config.vertexBindingCount > 0)
        hgAssert(config.vertexBindings != nullptr);

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = config.vertexShader;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = config.fragmentShader;
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputState{};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = (u32)config.vertexBindingCount;
    vertexInputState.pVertexBindingDescriptions = config.vertexBindings;
    vertexInputState.vertexAttributeDescriptionCount = (u32)config.vertexAttributeCount;
    vertexInputState.pVertexAttributeDescriptions = config.vertexAttributes;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = config.topology;
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
    rasterizationState.polygonMode = config.polygonMode;
    rasterizationState.cullMode = config.cullMode;
    rasterizationState.frontFace = config.frontFace;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp = 0.0f;
    rasterizationState.depthBiasSlopeFactor = 0.0f;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = config.multisampleCount != 0
        ? config.multisampleCount
        : VK_SAMPLE_COUNT_1_BIT,
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.minSampleShading = 1.0f;
    multisampleState.pSampleMask = nullptr;
    multisampleState.alphaToCoverageEnable = VK_FALSE;
    multisampleState.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = config.depthAttachmentFormat != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depthStencilState.depthWriteEnable = config.depthAttachmentFormat != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depthStencilState.depthCompareOp = config.enableColorBlend
            ? VK_COMPARE_OP_LESS_OR_EQUAL
            : VK_COMPARE_OP_LESS;
    depthStencilState.depthBoundsTestEnable = config.depthAttachmentFormat != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE;
    depthStencilState.stencilTestEnable = config.stencilAttachmentFormat != VK_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE,
    // depthStencilState.front = {}; : TODO
    // depthStencilState.back = {}; : TODO
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = config.enableColorBlend ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;
    colorBlendState.blendConstants[0] = {1.0f};
    colorBlendState.blendConstants[1] = {1.0f};
    colorBlendState.blendConstants[2] = {1.0f};
    colorBlendState.blendConstants[3] = {1.0f};

    VkDynamicState dynamicStates[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(*dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = (u32)config.colorAttachmentCount;
    renderingInfo.pColorAttachmentFormats = config.colorAttachmentFormats;
    renderingInfo.depthAttachmentFormat = config.depthAttachmentFormat;
    renderingInfo.stencilAttachmentFormat = config.stencilAttachmentFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = sizeof(shaderStages) / sizeof(*shaderStages);
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
    pipelineInfo.layout = config.layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline = nullptr;
    VkResult result = vkCreateGraphicsPipelines(hgVkDevice, nullptr, 1, &pipelineInfo, nullptr, &pipeline);
    if (pipeline == nullptr)
        hgError("Failed to create Vulkan graphics pipeline: %s\n", hgVkResultToStr(result));

    return pipeline;
}

VkPipeline hgVkCreateComputePipeline(VkPipelineLayout layout, const VkShaderModule shader)
{
    hgAssert(layout != nullptr);
    hgAssert(shader != nullptr);

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = shader;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline = nullptr;
    VkResult result = vkCreateComputePipelines(hgVkDevice, nullptr, 1, &pipelineInfo, nullptr, &pipeline);
    if (pipeline == nullptr)
        hgError("Failed to create Vulkan compute pipeline: %s\n", hgVkResultToStr(result));

    return pipeline;
}

VkDescriptorPool hgVkCreateDescriptorPool(
    u32 maxSets,
    VkDescriptorPoolSize* sizes,
    u32 sizeCount,
    VkDescriptorPoolCreateFlags flags)
{
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = flags;
    info.maxSets = maxSets;
    info.poolSizeCount = sizeCount;
    info.pPoolSizes = sizes;

    VkDescriptorPool pool = nullptr;
    VkResult result = vkCreateDescriptorPool(hgVkDevice, &info, nullptr, &pool);
    if (pool == nullptr)
        hgError("Could not create VkDescriptorPool: %s\n", hgVkResultToStr(result));

    return pool;
}

VkDescriptorSet hgVkAllocateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    VkDescriptorSet set = nullptr;
    VkResult result = vkAllocateDescriptorSets(hgVkDevice, &info, &set);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY)
        return nullptr;
    if (set == nullptr)
        hgError("Could not allocate VkDescriptorSet: %s\n", hgVkResultToStr(result));

    return set;
}

void hgVkUpdateDescriptorSetBuffers(
    VkDescriptorSet set,
    u32 binding,
    VkDescriptorType type,
    const VkDescriptorBufferInfo* infos,
    u32 count)
{
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = binding;
    write.descriptorCount = count;
    write.descriptorType = type;
    write.pBufferInfo = infos;

    vkUpdateDescriptorSets(hgVkDevice, 1, &write, 0, nullptr);
}

void hgVkUpdateDescriptorSetImages(
    VkDescriptorSet set,
    u32 binding,
    VkDescriptorType type,
    const VkDescriptorImageInfo* infos,
    u32 count)
{
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = binding;
    write.descriptorCount = count;
    write.descriptorType = type;
    write.pImageInfo = infos;

    vkUpdateDescriptorSets(hgVkDevice, 1, &write, 0, nullptr);
}

u32 hgVkFindMemoryTypeIndex(
    u32 bitmask,
    VkMemoryPropertyFlags preferredFlags,
    VkMemoryPropertyFlags unpreferredFlags)
{
    hgAssert(hgVkPhysicalDevice != nullptr);
    hgAssert(bitmask != 0);

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(hgVkPhysicalDevice, &memProps);

    for (u32 i = 0; i < memProps.memoryTypeCount; ++i)
    {
        if ((bitmask & (1 << i)) != 0 &&
            (memProps.memoryTypes[i].propertyFlags & preferredFlags) != 0 &&
            (memProps.memoryTypes[i].propertyFlags & unpreferredFlags) == 0)
        {
            return i;
        }
    }
    for (u32 i = 0; i < memProps.memoryTypeCount; ++i)
    {
        if ((bitmask & (1 << i)) != 0 && (memProps.memoryTypes[i].propertyFlags & preferredFlags) != 0)
        {
            hgWarn("Could not find Vulkan memory type without undesired flags\n");
            return i;
        }
    }
    for (u32 i = 0; i < memProps.memoryTypeCount; ++i)
    {
        if ((bitmask & (1 << i)) != 0)
        {
            hgWarn("Could not find Vulkan memory type with desired flags\n");
            return i;
        }
    }
    hgError("Could not find Vulkan memory type\n");
}

void hgVkCreateBuffer(
    VkBuffer* buffer,
    VmaAllocation* allocation,
    u64 size,
    VkBufferUsageFlags usage,
    VmaAllocationCreateFlags memory)
{
    hgAssert(buffer != nullptr);
    hgAssert(allocation != nullptr);
    hgAssert(size > 0);
    hgAssert(usage != 0);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.flags = memory;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult result = vmaCreateBuffer(hgVkVma, &bufferInfo, &allocInfo, buffer, allocation, nullptr);
    if (result != VK_SUCCESS)
        hgError("Could not create VkBuffer: %s", hgVkResultToStr(result));
}

void hgVkWriteBufferStaging(
    VkBuffer dst,
    usize offset,
    const void* src,
    usize size)
{
    hgAssert(hgVkDevice != nullptr);
    hgAssert(hgVkVma != nullptr);
    hgAssert(dst != nullptr);
    hgAssert(src != nullptr);

    VkBuffer stage = nullptr;
    VmaAllocation stageAlloc = nullptr;
    hgVkCreateBuffer(&stage, &stageAlloc, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    hgDefer(vmaDestroyBuffer(hgVkVma, stage, stageAlloc));

    vmaCopyMemoryToAllocation(hgVkVma, src, stageAlloc, offset, size);

    VkCommandBuffer cmd = hgVkBeginCommands();

    VkBufferCopy region{};
    region.dstOffset = offset;
    region.size = size;

    vkCmdCopyBuffer(cmd, stage, dst, 1, &region);

    hgVkEndAndExecute(cmd);
}

void hgVkReadBufferStaging(
    void* dst,
    VkBuffer src,
    usize offset,
    usize size)
{
    hgAssert(hgVkDevice != nullptr);
    hgAssert(hgVkVma != nullptr);
    hgAssert(dst != nullptr);
    hgAssert(src != nullptr);

    VkBuffer stage = nullptr;
    VmaAllocation stageAlloc = nullptr;
    hgVkCreateBuffer(&stage, &stageAlloc, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
    hgDefer(vmaDestroyBuffer(hgVkVma, stage, stageAlloc));

    VkCommandBuffer cmd = hgVkBeginCommands();

    VkBufferCopy region{};
    region.srcOffset = offset;
    region.size = size;

    vkCmdCopyBuffer(cmd, src, stage, 1, &region);

    hgVkEndAndExecute(cmd);

    vmaCopyAllocationToMemory(hgVkVma, stageAlloc, offset, dst, size);
}

void hgVkCreateImage(VkImage* image, VmaAllocation* allocation, const HgVkImageConfig& config)
{
    hgAssert(image != nullptr);
    hgAssert(allocation != nullptr);
    hgAssert(config.format != VK_FORMAT_UNDEFINED);
    hgAssert(config.usage != 0);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = config.type;
    imageInfo.format = config.format;
    imageInfo.extent = {config.width, config.height, config.depth};
    imageInfo.mipLevels = config.mipLevels;
    imageInfo.arrayLayers = config.arrayLayers;
    imageInfo.samples = config.samples;
    imageInfo.usage = config.usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult result = vmaCreateImage(hgVkVma, &imageInfo, &allocInfo, image, allocation, nullptr);
    if (result != VK_SUCCESS)
        hgError("Could not create VkImage: %s", hgVkResultToStr(result));
}

VkImageView hgVkCreateImageView(const HgVkImageViewConfig& config)
{
    hgAssert(config.image != nullptr);
    hgAssert(config.format != VK_FORMAT_UNDEFINED);
    hgAssert(config.subresource.aspectMask != 0);

    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = config.image;
    info.viewType = config.type;
    info.format = config.format;
    info.components = config.components;
    info.subresourceRange = config.subresource;

    VkImageView view = nullptr;
    VkResult result = vkCreateImageView(hgVkDevice, &info, nullptr, &view);
    if (view == nullptr)
        hgError("Could not create VkImageView: %s\n", hgVkResultToStr(result));

    return view;
}

void hgWriteVkImageStaging(const HgVkWriteImageStagingConfig& config)
{
    hgAssert(hgVkDevice != nullptr);
    hgAssert(hgVkVma != nullptr);
    hgAssert(config.dstImage != nullptr);
    hgAssert(config.srcData != nullptr);
    hgAssert(config.width > 0);
    hgAssert(config.height > 0);
    hgAssert(config.depth > 0);
    hgAssert(config.format != VK_FORMAT_UNDEFINED);

    usize size = config.width
               * config.height
               * config.depth
               * hgVkFormatToSize(config.format);

    VkBuffer stage = nullptr;
    VmaAllocation stageAlloc = nullptr;
    hgVkCreateBuffer(&stage, &stageAlloc, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    hgDefer(vmaDestroyBuffer(hgVkVma, stage, stageAlloc));

    vmaCopyMemoryToAllocation(hgVkVma, config.srcData, stageAlloc, 0, size);

    VkCommandBuffer cmd = hgVkBeginCommands();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarrier.image = config.dstImage;
    transferBarrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    transferBarrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    transferBarrier.subresourceRange.levelCount = 1;
    transferBarrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(cmd, &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource = config.subresource;
    region.imageExtent = {config.width, config.height, config.depth};

    vkCmdCopyBufferToImage(cmd, stage, config.dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier2 endBarrier{};
    endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    endBarrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    endBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    endBarrier.newLayout = config.layout;
    endBarrier.image = config.dstImage;
    endBarrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    endBarrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    endBarrier.subresourceRange.levelCount = 1;
    endBarrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    endBarrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo endDep{};
    endDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    endDep.imageMemoryBarrierCount = 1;
    endDep.pImageMemoryBarriers = &endBarrier;

    vkCmdPipelineBarrier2(cmd, &endDep);

    hgVkEndAndExecute(cmd);
}

void hgVkWriteCubemapStaging(const HgVkWriteImageStagingConfig& config)
{
    hgAssert(hgVkDevice != nullptr);
    hgAssert(hgVkVma != nullptr);
    hgAssert(config.dstImage != nullptr);
    hgAssert(config.subresource.baseArrayLayer == 0);
    hgAssert(config.subresource.layerCount == 6);
    hgAssert(config.srcData != nullptr);
    hgAssert(config.width > 0);
    hgAssert(config.height > 0);
    hgAssert(config.depth == 0 || config.depth == 1);
    hgAssert(config.format != VK_FORMAT_UNDEFINED);

    usize size = config.width
               * config.height
               * hgVkFormatToSize(config.format);

    VkBuffer buffer = nullptr;
    VmaAllocation bufferAlloc = nullptr;
    hgVkCreateBuffer(&buffer, &bufferAlloc, size * 4 * 3,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    hgDefer(vmaDestroyBuffer(hgVkVma, buffer, bufferAlloc));

    vmaCopyMemoryToAllocation(hgVkVma, config.srcData, bufferAlloc, 0, size);

    HgVkImageConfig stageInfo{};
    stageInfo.width = config.width;
    stageInfo.height = config.height;
    stageInfo.depth = config.depth;
    stageInfo.format = config.format;
    stageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                     | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    VkImage stage = nullptr;
    VmaAllocation stageAlloc = nullptr;
    hgVkCreateImage(&stage, &stageAlloc, stageInfo);
    hgDefer(vmaDestroyImage(hgVkVma, stage, stageAlloc));

    VkCommandBuffer cmd = hgVkBeginCommands();

    VkImageMemoryBarrier2 stageBarrier{};
    stageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    stageBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    stageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    stageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    stageBarrier.image = config.dstImage;
    stageBarrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    stageBarrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    stageBarrier.subresourceRange.levelCount = 1;
    stageBarrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    stageBarrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo stageDep{};
    stageDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    stageDep.imageMemoryBarrierCount = 1;
    stageDep.pImageMemoryBarriers = &stageBarrier;

    vkCmdPipelineBarrier2(cmd, &stageDep);

    VkBufferImageCopy stageRegion{};
    stageRegion.imageSubresource = config.subresource;
    stageRegion.imageExtent = {config.width, config.height, config.depth};

    vkCmdCopyBufferToImage(cmd, buffer, stage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &stageRegion);

    VkImageMemoryBarrier2 transferBarriers[2]{};

    transferBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[0].srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[0].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarriers[0].image = stage;
    transferBarriers[0].subresourceRange.aspectMask = config.subresource.aspectMask;
    transferBarriers[0].subresourceRange.baseMipLevel = 0;
    transferBarriers[0].subresourceRange.levelCount = 1;
    transferBarriers[0].subresourceRange.baseArrayLayer = 0;
    transferBarriers[0].subresourceRange.layerCount = 1;

    transferBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[1].image = config.dstImage;
    transferBarriers[1].subresourceRange.aspectMask = config.subresource.aspectMask;
    transferBarriers[1].subresourceRange.baseMipLevel = config.subresource.mipLevel;
    transferBarriers[1].subresourceRange.levelCount = 1;
    transferBarriers[1].subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    transferBarriers[1].subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = sizeof(transferBarriers) / sizeof(*transferBarriers);
    transferDep.pImageMemoryBarriers = transferBarriers;

    vkCmdPipelineBarrier2(cmd, &transferDep);

    VkImageCopy regions[6]{};

    regions[0].srcSubresource = {config.subresource.aspectMask, 0, 0, 1};
    regions[0].srcOffset = {(int)config.width * 2, (int)config.height * 1, 0};
    regions[0].dstSubresource = {config.subresource.aspectMask, config.subresource.mipLevel, 0, 1};
    regions[0].dstOffset = {};
    regions[0].extent = {config.width, config.height, 1};

    regions[1].srcSubresource = {config.subresource.aspectMask, 0, 0, 1};
    regions[1].srcOffset = {(int)config.width * 0, (int)config.height * 1, 0};
    regions[1].dstSubresource = {config.subresource.aspectMask, config.subresource.mipLevel, 1, 1};
    regions[1].dstOffset = {};
    regions[1].extent = {config.width, config.height, 1};

    regions[2].srcSubresource = {config.subresource.aspectMask, 0, 0, 1};
    regions[2].srcOffset = {(int)config.width * 1, (int)config.height * 0, 0};
    regions[2].dstSubresource = {config.subresource.aspectMask, config.subresource.mipLevel, 2, 1};
    regions[2].dstOffset = {};
    regions[2].extent = {config.width, config.height, 1};

    regions[3].srcSubresource = {config.subresource.aspectMask, 0, 0, 1};
    regions[3].srcOffset = {(int)config.width * 1, (int)config.height * 2, 0};
    regions[3].dstSubresource = {config.subresource.aspectMask, config.subresource.mipLevel, 3, 1};
    regions[3].dstOffset = {};
    regions[3].extent = {config.width, config.height, 1};

    regions[4].srcSubresource = {config.subresource.aspectMask, 0, 0, 1};
    regions[4].srcOffset = {(int)config.width * 1, (int)config.height * 1, 0};
    regions[4].dstSubresource = {config.subresource.aspectMask, config.subresource.mipLevel, 4, 1};
    regions[4].dstOffset = {};
    regions[4].extent = {config.width, config.height, 1};

    regions[5].srcSubresource = {config.subresource.aspectMask, 0, 0, 1};
    regions[5].srcOffset = {(int)config.width * 3, (int)config.height * 1, 0};
    regions[5].dstSubresource = {config.subresource.aspectMask, config.subresource.mipLevel, 5, 1};
    regions[5].dstOffset = {};
    regions[5].extent = {config.width, config.height, 1};

    vkCmdCopyImage(
        cmd,
        stage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        config.dstImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        sizeof(regions) / sizeof(*regions),
        regions);

    VkImageMemoryBarrier2 endBarrier{};
    endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    endBarrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    endBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    endBarrier.newLayout = config.layout;
    endBarrier.image = config.dstImage;
    endBarrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    endBarrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    endBarrier.subresourceRange.levelCount = 1;
    endBarrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    endBarrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo endDep{};
    endDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    endDep.imageMemoryBarrierCount = 1;
    endDep.pImageMemoryBarriers = &endBarrier;

    vkCmdPipelineBarrier2(cmd, &endDep);

    hgVkEndAndExecute(cmd);
}

void hgVkReadImageStaging(const HgVkReadImageStagingConfig& config)
{
    hgAssert(hgVkDevice != nullptr);
    hgAssert(hgVkVma != nullptr);
    hgAssert(config.srcImage != nullptr);
    hgAssert(config.layout != VK_IMAGE_LAYOUT_UNDEFINED);
    hgAssert(config.dst != nullptr);
    hgAssert(config.width > 0);
    hgAssert(config.height > 0);
    hgAssert(config.depth > 0);
    hgAssert(config.format != VK_FORMAT_UNDEFINED);

    usize size = config.width
               * config.height
               * config.depth
               * hgVkFormatToSize(config.format);

    VkBuffer stage = nullptr;
    VmaAllocation stageAlloc = nullptr;
    hgVkCreateBuffer(&stage, &stageAlloc, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    hgDefer(vmaDestroyBuffer(hgVkVma, stage, stageAlloc));

    VkCommandBuffer cmd = hgVkBeginCommands();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarrier.oldLayout = config.layout;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarrier.image = config.srcImage;
    transferBarrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    transferBarrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    transferBarrier.subresourceRange.levelCount = 1;
    transferBarrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(cmd, &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource = config.subresource;
    region.imageExtent = {config.width, config.height, config.depth};

    vkCmdCopyImageToBuffer(cmd, config.srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stage, 1, &region);

    VkImageMemoryBarrier2 endBarrier{};
    endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    endBarrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    endBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    endBarrier.newLayout = config.layout;
    endBarrier.image = config.srcImage;
    endBarrier.subresourceRange.aspectMask = config.subresource.aspectMask;
    endBarrier.subresourceRange.baseMipLevel = config.subresource.mipLevel;
    endBarrier.subresourceRange.levelCount = 1;
    endBarrier.subresourceRange.baseArrayLayer = config.subresource.baseArrayLayer;
    endBarrier.subresourceRange.layerCount = config.subresource.layerCount;

    VkDependencyInfo endDep{};
    endDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    endDep.imageMemoryBarrierCount = 1;
    endDep.pImageMemoryBarriers = &endBarrier;

    vkCmdPipelineBarrier2(cmd, &endDep);

    hgVkEndAndExecute(cmd);

    vmaCopyAllocationToMemory(hgVkVma, stageAlloc, 0, config.dst, size);
}

void hgVkGenerateMipmaps(const HgVkGenerateMipmapsConfig& config)
{
    hgAssert(hgVkDevice != nullptr);
    hgAssert(config.image != nullptr);
    hgAssert(config.oldLayout != VK_IMAGE_LAYOUT_UNDEFINED);
    hgAssert(config.newLayout != VK_IMAGE_LAYOUT_UNDEFINED);
    hgAssert(config.width > 0);
    hgAssert(config.height > 0);
    hgAssert(config.depth > 0);
    hgAssert(config.mipCount > 0);
    if (config.mipCount == 1)
        return;

    VkCommandBuffer cmd = hgVkBeginCommands();

    VkOffset3D mipOffset{};
    mipOffset.x = (i32)config.width;
    mipOffset.y = (i32)config.height;
    mipOffset.z = (i32)config.depth;

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = config.oldLayout;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = config.image;
    barrier.subresourceRange.aspectMask = config.aspectMask;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &dep);

    for (u32 level = 0; level < config.mipCount - 1; ++level)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.subresourceRange.aspectMask = config.aspectMask;
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(cmd, &dep);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = config.aspectMask;
        blit.srcSubresource.mipLevel = level;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[1] = mipOffset;
        if (mipOffset.x > 1)
            mipOffset.x /= 2;
        if (mipOffset.y > 1)
            mipOffset.y /= 2;
        if (mipOffset.z > 1)
            mipOffset.z /= 2;
        blit.dstSubresource.aspectMask = config.aspectMask;
        blit.dstSubresource.mipLevel = level + 1;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[1] = mipOffset;

        vkCmdBlitImage(cmd,
            config.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            config.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.subresourceRange.aspectMask = config.aspectMask;
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(cmd, &dep);
    }

    barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.dstAccessMask = VK_ACCESS_NONE;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = config.newLayout;
    barrier.subresourceRange.aspectMask = config.aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = config.mipCount;

    vkCmdPipelineBarrier2(cmd, &dep);

    hgVkEndAndExecute(cmd);
}

HgRenderer HgRenderer::create(HgArena* arena, u32 maxImages, u32 maxBuffers)
{
    HgRenderer renderer{};
    renderer.buffers = hgAlloc<Buffer>(arena, maxBuffers);
    renderer.bufferCount = 0;
    renderer.bufferCapacity = maxBuffers;
    renderer.images = hgAlloc<Image>(arena, maxImages);
    renderer.imageCount = 0;
    renderer.imageCapacity = maxImages;
    return renderer;
}

void HgRenderer::reset()
{
    bufferCount = 0;
    imageCount = 0;
}

HgBufferRenderID HgRenderer::addBuffer(
    VkBuffer buffer,
    u64 offset,
    u64 size,
    HgRenderUsage previousUsage,
    HgRenderAccess previousAccess)
{
    hgAssert(bufferCount < bufferCapacity);
    hgAssert(buffer != nullptr);

    buffers[bufferCount] = {buffer, offset, size, previousUsage, previousAccess};
    return bufferCount++;
}

HgImageRenderID HgRenderer::addImage(
    VkImage image,
    VkImageView view,
    VkImageSubresourceRange subresource,
    HgRenderUsage previousUsage,
    HgRenderAccess previousAccess)
{
    hgAssert(imageCount < imageCapacity);
    hgAssert(image != nullptr);
    hgAssert(view != nullptr);

    images[imageCount] = {image, view, subresource, previousUsage, previousAccess};
    return imageCount++;
}

static constexpr VkPipelineStageFlags2 getStageMask(HgRenderUsage usage)
{
    switch (usage)
    {
        case HgRenderUsage::none:
            return VK_PIPELINE_STAGE_NONE;
            break;
        case HgRenderUsage::vertexBuffer:
            return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            break;
        case HgRenderUsage::indexBuffer:
            return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            break;
        case HgRenderUsage::graphicsShader: [[fallthrough]];
        case HgRenderUsage::graphicsUniformBuffer:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case HgRenderUsage::computeShader: [[fallthrough]];
        case HgRenderUsage::computeUniformBuffer:
            return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            break;
        case HgRenderUsage::colorAttachment:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case HgRenderUsage::depthAttachment: [[fallthrough]];
        case HgRenderUsage::stencilAttachment:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        case HgRenderUsage::transfer:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case HgRenderUsage::presentSrc:
            return VK_PIPELINE_STAGE_NONE;
            break;
        case HgRenderUsage::count:
            break;
    }
    hgError("Found invalid render usage: %d", (u32)usage);
}

static constexpr VkAccessFlags2 getAccessMask(HgRenderUsage usage, HgRenderAccess access)
{
    switch (usage)
    {
        case HgRenderUsage::none:
            return VK_ACCESS_NONE;
            break;
        case HgRenderUsage::vertexBuffer:
            return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            break;
        case HgRenderUsage::indexBuffer:
            return VK_ACCESS_INDEX_READ_BIT;
            break;
        case HgRenderUsage::graphicsShader:
            return ((u32)access & (u32)HgRenderAccess::read) ? VK_ACCESS_SHADER_READ_BIT : 0
                 | ((u32)access & (u32)HgRenderAccess::write) ? VK_ACCESS_SHADER_WRITE_BIT : 0;
            break;
        case HgRenderUsage::graphicsUniformBuffer:
            return VK_ACCESS_UNIFORM_READ_BIT;
            break;
        case HgRenderUsage::computeShader:
            return ((u32)access & (u32)HgRenderAccess::read) ? VK_ACCESS_SHADER_READ_BIT : 0
                 | ((u32)access & (u32)HgRenderAccess::write) ? VK_ACCESS_SHADER_WRITE_BIT : 0;
            break;
        case HgRenderUsage::computeUniformBuffer:
            return VK_ACCESS_UNIFORM_READ_BIT;
            break;
        case HgRenderUsage::colorAttachment:
            return ((u32)access & (u32)HgRenderAccess::read) ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT : 0
                 | ((u32)access & (u32)HgRenderAccess::write) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0;
            break;
        case HgRenderUsage::depthAttachment: [[fallthrough]];
        case HgRenderUsage::stencilAttachment:
            return ((u32)access & (u32)HgRenderAccess::read) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT : 0
                 | ((u32)access & (u32)HgRenderAccess::write) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0;
            break;
        case HgRenderUsage::transfer:
            return ((u32)access & (u32)HgRenderAccess::read) ? VK_ACCESS_TRANSFER_READ_BIT : 0
                 | ((u32)access & (u32)HgRenderAccess::write) ? VK_ACCESS_TRANSFER_WRITE_BIT : 0;
            break;
        case HgRenderUsage::presentSrc:
            return VK_ACCESS_NONE;
            break;
        case HgRenderUsage::count:
            break;
    }
    hgError("Found invalid render usage: %d", (u32)usage);
}

static constexpr VkImageLayout getLayout(HgRenderUsage usage, HgRenderAccess access)
{
    switch (usage)
    {
        case HgRenderUsage::none:
            return VK_IMAGE_LAYOUT_UNDEFINED;
            break;
        case HgRenderUsage::graphicsShader: [[fallthrough]];
        case HgRenderUsage::computeShader:
            return ((u32)access & (u32)HgRenderAccess::write)
                ? VK_IMAGE_LAYOUT_GENERAL
                : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            break;
        case HgRenderUsage::colorAttachment:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            break;
        case HgRenderUsage::depthAttachment: [[fallthrough]];
        case HgRenderUsage::stencilAttachment:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            break;
        case HgRenderUsage::transfer:
            hgAssert(!(((u32)access & (u32)HgRenderAccess::read) && ((u32)access & (u32)HgRenderAccess::write)));
            return ((u32)access & (u32)HgRenderAccess::read)
                ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            break;
        case HgRenderUsage::presentSrc:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            break;
        case HgRenderUsage::vertexBuffer: [[fallthrough]];
        case HgRenderUsage::indexBuffer: [[fallthrough]];
        case HgRenderUsage::graphicsUniformBuffer: [[fallthrough]];
        case HgRenderUsage::computeUniformBuffer: [[fallthrough]];
        case HgRenderUsage::count:
            break;
    }
    hgError("Found invalid render usage for image: %d", (u32)usage);
}

void HgRenderer::barrier(
    VkCommandBuffer cmd,
    const HgBufferBarrier* bufferBarriers,
    u32 bufferBarrierCount,
    const HgImageBarrier* imageBarriers,
    u32 imageBarrierCount)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    VkBufferMemoryBarrier2* vkBufferBarriers = hgAlloc<VkBufferMemoryBarrier2>(scratch, imageBarrierCount);
    VkImageMemoryBarrier2* vkImageBarriers = hgAlloc<VkImageMemoryBarrier2>(scratch, imageBarrierCount);

    for (usize i = 0; i < bufferBarrierCount; ++i)
    {
        hgAssert(bufferBarriers[i].buffer < bufferCount);
        Buffer& buffer = buffers[bufferBarriers[i].buffer];

        vkBufferBarriers[i] = {};
        vkBufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        vkBufferBarriers[i].srcStageMask = getStageMask(buffer.lastUsage);
        vkBufferBarriers[i].srcAccessMask = getAccessMask(buffer.lastUsage, buffer.lastAccess);
        vkBufferBarriers[i].dstStageMask = getStageMask(bufferBarriers[i].nextUsage);
        vkBufferBarriers[i].dstAccessMask = getAccessMask(bufferBarriers[i].nextUsage, bufferBarriers[i].nextAccess);
        vkBufferBarriers[i].buffer = buffer.handle;
        vkBufferBarriers[i].offset = buffer.offset;
        vkBufferBarriers[i].size = buffer.size;

        buffer.lastUsage = bufferBarriers[i].nextUsage;
        buffer.lastAccess = bufferBarriers[i].nextAccess;
    }

    for (usize i = 0; i < imageBarrierCount; ++i)
    {
        hgAssert(imageBarriers[i].image < imageCount);
        Image& image = images[imageBarriers[i].image];

        vkImageBarriers[i] = {};
        vkImageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        vkImageBarriers[i].srcStageMask = getStageMask(image.lastUsage);
        vkImageBarriers[i].srcAccessMask = getAccessMask(image.lastUsage, image.lastAccess);
        vkImageBarriers[i].dstStageMask = getStageMask(imageBarriers[i].nextUsage);
        vkImageBarriers[i].dstAccessMask = getAccessMask(imageBarriers[i].nextUsage, imageBarriers[i].nextAccess);
        vkImageBarriers[i].oldLayout = getLayout(image.lastUsage, image.lastAccess);
        vkImageBarriers[i].newLayout = getLayout(imageBarriers[i].nextUsage, imageBarriers[i].nextAccess);
        vkImageBarriers[i].image = image.handle;
        vkImageBarriers[i].subresourceRange = image.subresource;

        image.lastUsage = imageBarriers[i].nextUsage;
        image.lastAccess = imageBarriers[i].nextAccess;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = bufferBarrierCount;
    dep.pBufferMemoryBarriers = vkBufferBarriers;
    dep.imageMemoryBarrierCount = imageBarrierCount;
    dep.pImageMemoryBarriers = vkImageBarriers;

    vkCmdPipelineBarrier2(cmd, &dep);
}

void HgRenderer::beginPass(VkCommandBuffer cmd, u32 width, u32 height, const HgRenderPass& pass)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    VkBufferMemoryBarrier2* bufferBarriers = nullptr;
    u32 bufferBarrierCount = 0;
    VkImageMemoryBarrier2* imageBarriers = nullptr;
    u32 imageBarrierCount = 0;

    bufferBarriers = hgRealloc<VkBufferMemoryBarrier2>(scratch, 
        bufferBarriers, bufferBarrierCount, bufferBarrierCount + pass.uniformBufferCount);

    for (usize i = bufferBarrierCount; i < bufferBarrierCount + pass.uniformBufferCount; ++i)
    {
        hgAssert(pass.uniformBuffers[i - bufferBarrierCount] < bufferCount);
        Buffer& buffer = buffers[pass.uniformBuffers[i - bufferBarrierCount]];

        bufferBarriers[i] = {};
        bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bufferBarriers[i].srcStageMask = getStageMask(buffer.lastUsage);
        bufferBarriers[i].srcAccessMask = getAccessMask(buffer.lastUsage, buffer.lastAccess);
        bufferBarriers[i].dstStageMask = getStageMask(HgRenderUsage::graphicsUniformBuffer);
        bufferBarriers[i].dstAccessMask = getAccessMask(HgRenderUsage::graphicsUniformBuffer, HgRenderAccess::read);
        bufferBarriers[i].buffer = buffer.handle;
        bufferBarriers[i].offset = buffer.offset;
        bufferBarriers[i].size = buffer.size;

        buffer.lastUsage = HgRenderUsage::graphicsUniformBuffer;
        buffer.lastAccess = HgRenderAccess::read;
    }

    bufferBarrierCount += pass.uniformBufferCount;

    bufferBarriers = hgRealloc<VkBufferMemoryBarrier2>(scratch, 
        bufferBarriers, bufferBarrierCount, bufferBarrierCount + pass.storageBufferCount);

    for (usize i = bufferBarrierCount; i < bufferBarrierCount + pass.storageBufferCount; ++i)
    {
        hgAssert(pass.storageBuffers[i - bufferBarrierCount] < bufferCount);
        Buffer& buffer = buffers[pass.storageBuffers[i - bufferBarrierCount]];

        bufferBarriers[i] = {};
        bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bufferBarriers[i].srcStageMask = getStageMask(buffer.lastUsage);
        bufferBarriers[i].srcAccessMask = getAccessMask(buffer.lastUsage, buffer.lastAccess);
        bufferBarriers[i].dstStageMask = getStageMask(HgRenderUsage::graphicsShader);
        bufferBarriers[i].dstAccessMask = getAccessMask(HgRenderUsage::graphicsShader, HgRenderAccess::read);
        bufferBarriers[i].buffer = buffer.handle;
        bufferBarriers[i].offset = buffer.offset;
        bufferBarriers[i].size = buffer.size;

        buffer.lastUsage = HgRenderUsage::graphicsShader;
        buffer.lastAccess = HgRenderAccess::read;
    }

    bufferBarrierCount += pass.storageBufferCount;

    imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
        imageBarriers, imageBarrierCount, imageBarrierCount + pass.sampledImageCount);

    for (usize i = imageBarrierCount; i < imageBarrierCount + pass.sampledImageCount; ++i)
    {
        hgAssert(pass.sampledImages[i - imageBarrierCount] < imageCount);
        Image& image = images[pass.sampledImages[i - imageBarrierCount]];

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = getStageMask(image.lastUsage);
        imageBarriers[i].srcAccessMask = getAccessMask(image.lastUsage, image.lastAccess);
        imageBarriers[i].dstStageMask = getStageMask(HgRenderUsage::graphicsShader);
        imageBarriers[i].dstAccessMask = getAccessMask(HgRenderUsage::graphicsShader, HgRenderAccess::read);
        imageBarriers[i].oldLayout = getLayout(image.lastUsage, image.lastAccess);
        imageBarriers[i].newLayout = getLayout(HgRenderUsage::graphicsShader, HgRenderAccess::read);
        imageBarriers[i].image = image.handle;
        imageBarriers[i].subresourceRange = image.subresource;

        image.lastUsage = HgRenderUsage::graphicsShader;
        image.lastAccess = HgRenderAccess::read;
    }

    imageBarrierCount += pass.sampledImageCount;

    imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
        imageBarriers, imageBarrierCount, imageBarrierCount + pass.colorAttachmentCount);

    for (usize i = imageBarrierCount; i < imageBarrierCount + pass.colorAttachmentCount; ++i)
    {
        hgAssert(pass.colorAttachments[i - imageBarrierCount].image < imageCount);
        Image& image = images[pass.colorAttachments[i - imageBarrierCount].image];

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = getStageMask(image.lastUsage);
        imageBarriers[i].srcAccessMask = getAccessMask(image.lastUsage, image.lastAccess);
        imageBarriers[i].dstStageMask = getStageMask(HgRenderUsage::colorAttachment);
        imageBarriers[i].dstAccessMask = getAccessMask(HgRenderUsage::colorAttachment, HgRenderAccess::write);
        imageBarriers[i].oldLayout = getLayout(image.lastUsage, image.lastAccess);
        imageBarriers[i].newLayout = getLayout(HgRenderUsage::colorAttachment, HgRenderAccess::write);
        imageBarriers[i].image = image.handle;
        imageBarriers[i].subresourceRange = image.subresource;

        image.lastUsage = HgRenderUsage::colorAttachment;
        image.lastAccess = HgRenderAccess::write;
    }

    imageBarrierCount += pass.colorAttachmentCount;

    if (pass.depthAttachment.image != (u64)-1)
    {
        hgAssert(pass.depthAttachment.image < imageCount);
        Image& image = images[pass.depthAttachment.image];

        imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
            imageBarriers, imageBarrierCount, imageBarrierCount + 1);

        HgRenderAccess access = (HgRenderAccess)((u32)HgRenderAccess::write |
            (pass.depthAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD ? (u32)HgRenderAccess::read : 0));

        usize idx = imageBarrierCount;
        imageBarriers[idx] = {};
        imageBarriers[idx].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[idx].srcStageMask = getStageMask(image.lastUsage);
        imageBarriers[idx].srcAccessMask = getAccessMask(image.lastUsage, image.lastAccess);
        imageBarriers[idx].dstStageMask = getStageMask(HgRenderUsage::depthAttachment);
        imageBarriers[idx].dstAccessMask = getAccessMask(HgRenderUsage::depthAttachment, access);
        imageBarriers[idx].oldLayout = getLayout(image.lastUsage, image.lastAccess);
        imageBarriers[idx].newLayout = getLayout(HgRenderUsage::depthAttachment, access);
        imageBarriers[idx].image = image.handle;
        imageBarriers[idx].subresourceRange = image.subresource;

        image.lastUsage = HgRenderUsage::depthAttachment;
        image.lastAccess = access;

        ++imageBarrierCount;
    }

    if (pass.stencilAttachment.image != (u64)-1)
    {
        hgAssert(pass.stencilAttachment.image < imageCount);
        Image& image = images[pass.stencilAttachment.image];

        imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
            imageBarriers, imageBarrierCount, imageBarrierCount + 1);

        HgRenderAccess access = (HgRenderAccess)((u32)HgRenderAccess::write |
            (pass.stencilAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD ? (u32)HgRenderAccess::read : 0));

        usize idx = imageBarrierCount;
        imageBarriers[idx] = {};
        imageBarriers[idx].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[idx].srcStageMask = getStageMask(image.lastUsage);
        imageBarriers[idx].srcAccessMask = getAccessMask(image.lastUsage, image.lastAccess);
        imageBarriers[idx].dstStageMask = getStageMask(HgRenderUsage::stencilAttachment);
        imageBarriers[idx].dstAccessMask = getAccessMask(HgRenderUsage::stencilAttachment, access);
        imageBarriers[idx].oldLayout = getLayout(image.lastUsage, image.lastAccess);
        imageBarriers[idx].newLayout = getLayout(HgRenderUsage::stencilAttachment, access);
        imageBarriers[idx].image = image.handle;
        imageBarriers[idx].subresourceRange = image.subresource;

        image.lastUsage = HgRenderUsage::stencilAttachment;
        image.lastAccess = access;

        ++imageBarrierCount;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = bufferBarrierCount;
    dep.pBufferMemoryBarriers = bufferBarriers;
    dep.imageMemoryBarrierCount = imageBarrierCount;
    dep.pImageMemoryBarriers = imageBarriers;

    vkCmdPipelineBarrier2(cmd, &dep);

    VkRenderingAttachmentInfo* colorAttachments
        = hgAlloc<VkRenderingAttachmentInfo>(scratch, pass.colorAttachmentCount);

    for (usize i = 0; i < pass.colorAttachmentCount; ++i)
    {
        hgAssert(pass.colorAttachments[i].image < imageCount);
        Image& image = images[pass.colorAttachments[i].image];

        colorAttachments[i] = {};
        colorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[i].imageView = image.view;
        colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[i].loadOp = pass.colorAttachments[i].loadOp;
        colorAttachments[i].storeOp = pass.colorAttachments[i].storeOp;
        colorAttachments[i].clearValue = pass.colorAttachments[i].clearValue;
    }

    VkRenderingAttachmentInfo depthAttachment{};
    if (pass.depthAttachment.image != (u64)-1)
    {
        hgAssert(pass.depthAttachment.image < imageCount);
        Image& image = images[pass.depthAttachment.image];

        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = image.view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = pass.depthAttachment.loadOp;
        depthAttachment.storeOp = pass.depthAttachment.storeOp;
        depthAttachment.clearValue = pass.depthAttachment.clearValue;
    }

    VkRenderingAttachmentInfo stencilAttachment{};
    if (pass.stencilAttachment.image < imageCount)
    {
        Image& image = images[pass.stencilAttachment.image];

        stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        stencilAttachment.imageView = image.view;
        stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        stencilAttachment.loadOp = pass.stencilAttachment.loadOp;
        stencilAttachment.storeOp = pass.stencilAttachment.storeOp;
        stencilAttachment.clearValue = pass.stencilAttachment.clearValue;
    }

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = {width, height};
    renderingInfo.layerCount = pass.layerCount;
    renderingInfo.colorAttachmentCount = pass.colorAttachmentCount;
    renderingInfo.pColorAttachments = colorAttachments;
    renderingInfo.pDepthAttachment = pass.depthAttachment.image != (u64)-1 ? &depthAttachment : nullptr;
    renderingInfo.pStencilAttachment = pass.stencilAttachment.image != (u64)-1 ? &stencilAttachment : nullptr;

    vkCmdBeginRendering(cmd, &renderingInfo);

    VkViewport viewport{0.0f, 0.0f, (f32)width, (f32)height, 0.0f, 1.0f};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{{0, 0}, {width, height}};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void HgRenderer::endPass(VkCommandBuffer cmd)
{
    vkCmdEndRendering(cmd);
}

void hgInternalResizeWindowSwapchain(HgWindow* window)
{
    if (window->width == 0 || window->height == 0)
        return;

    vkQueueWaitIdle(hgVkQueue);

    if (window->cmds != nullptr)
        vkFreeCommandBuffers(hgVkDevice, hgVkCmdPool, window->imageCount, window->cmds);

    for (usize i = 0; i < window->imageCount; ++i)
    {
        vkDestroyFence(hgVkDevice, window->frameFinished[i], nullptr);
    }
    for (usize i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->imageAvailable[i], nullptr);
    }
    for (usize i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->readyToPresent[i], nullptr);
    }
    for (usize i = 0; i < window->imageCount; ++i)
    {
        vkDestroyImageView(hgVkDevice, window->views[i], nullptr);
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(hgVkPhysicalDevice, window->surface, &surfaceCapabilities);

    if (surfaceCapabilities.currentExtent.width != (u32)-1)
        window->width = surfaceCapabilities.currentExtent.width;
    if (surfaceCapabilities.currentExtent.height != (u32)-1)
        window->height = surfaceCapabilities.currentExtent.height;

    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = window->surface;
    swapchainInfo.minImageCount
        = std::min(surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount - 1) + 1;
    swapchainInfo.imageFormat = window->format;
    swapchainInfo.imageExtent = {window->width, window->height};
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = window->imageUsage;
    swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = window->presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = window->swapchain;

    VkResult result = vkCreateSwapchainKHR(hgVkDevice, &swapchainInfo, nullptr, &window->swapchain);
    if (window->swapchain == nullptr)
        hgError("Failed to create swapchain: %s\n", hgVkResultToStr(result));

    vkGetSwapchainImagesKHR(hgVkDevice, window->swapchain, &window->imageCount, nullptr);
    window->images = (VkImage*)realloc(window->images, sizeof(VkImage) * window->imageCount);
    window->views = (VkImageView*)realloc(window->views, sizeof(VkImageView) * window->imageCount);
    vkGetSwapchainImagesKHR(hgVkDevice, window->swapchain, &window->imageCount, window->images);
    for (usize i = 0; i < window->imageCount; ++i)
    {
        HgVkImageViewConfig viewInfo{};
        viewInfo.image = window->images[i];
        viewInfo.format = window->format;
        viewInfo.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        window->views[i] = hgVkCreateImageView(viewInfo);
    }

    window->cmds = (VkCommandBuffer*)realloc(window->cmds, sizeof(VkCommandBuffer) * window->imageCount);

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.pNext = nullptr;
    cmdAllocInfo.commandPool = hgVkCmdPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = window->imageCount;

    vkAllocateCommandBuffers(hgVkDevice, &cmdAllocInfo, window->cmds);

    window->frameFinished = (VkFence*)realloc(window->frameFinished, sizeof(VkFence) * window->imageCount);
    for (usize i = 0; i < window->imageCount; ++i)
    {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(hgVkDevice, &info, nullptr, &window->frameFinished[i]);
    }

    window->imageAvailable = (VkSemaphore*)realloc(window->imageAvailable, sizeof(VkSemaphore) * window->imageCount);
    for (usize i = 0; i < window->imageCount; ++i)
    {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(hgVkDevice, &info, nullptr, &window->imageAvailable[i]);
    }

    window->readyToPresent = (VkSemaphore*)realloc(window->readyToPresent, sizeof(VkSemaphore) * window->imageCount);
    for (usize i = 0; i < window->imageCount; ++i)
    {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(hgVkDevice, &info, nullptr, &window->readyToPresent[i]);
    }

    vkDestroySwapchainKHR(hgVkDevice, swapchainInfo.oldSwapchain, nullptr);
}

static VkFormat findSwapchainFormat(VkSurfaceKHR surface)
{
    hgAssert(surface != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(hgVkPhysicalDevice, surface, &formatCount, nullptr);
    VkSurfaceFormatKHR* formats = hgAlloc<VkSurfaceFormatKHR>(scratch, formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(hgVkPhysicalDevice, surface, &formatCount, formats);

    for (usize i = 0; i < formatCount; ++i)
    {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return VK_FORMAT_R8G8B8A8_SRGB;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return VK_FORMAT_B8G8R8A8_SRGB;
    }
    hgError("No supported swapchain formats\n");
}

static VkPresentModeKHR findSwapchainPresentMode(
    VkSurfaceKHR surface,
    VkPresentModeKHR desiredMode)
{
    hgAssert(surface != nullptr);

    if (desiredMode == VK_PRESENT_MODE_FIFO_KHR)
        return desiredMode;

    u32 modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(hgVkPhysicalDevice, surface, &modeCount, nullptr);
    VkPresentModeKHR* presentModes = (VkPresentModeKHR*)alloca(modeCount * sizeof(*presentModes));
    vkGetPhysicalDeviceSurfacePresentModesKHR(hgVkPhysicalDevice, surface, &modeCount, presentModes);

    for (usize i = 0; i < modeCount; ++i)
    {
        if (presentModes[i] == desiredMode)
            return desiredMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

void hgInternalCreateWindowSwapchain(HgWindow* window, const HgWindowConfig& config)
{
    window->format = findSwapchainFormat(window->surface);
    window->presentMode = findSwapchainPresentMode(window->surface, config.preferredPresentMode);
    window->imageUsage = config.imageUsage;
    hgInternalResizeWindowSwapchain(window);
}

void hgInternalDestroyWindowSwapchain(HgWindow* window)
{
    vkFreeCommandBuffers(hgVkDevice, hgVkCmdPool, window->imageCount, window->cmds);
    free(window->cmds);

    for (usize i = 0; i < window->imageCount; ++i)
    {
        vkDestroyFence(hgVkDevice, window->frameFinished[i], nullptr);
    }
    free(window->frameFinished);

    for (usize i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->imageAvailable[i], nullptr);
    }
    free(window->imageAvailable);

    for (usize i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->readyToPresent[i], nullptr);
    }
    free(window->readyToPresent);

    for (usize i = 0; i < window->imageCount; ++i)
    {
        vkDestroyImageView(hgVkDevice, window->views[i], nullptr);
    }
    free(window->views);
    free(window->images);

    vkDestroySwapchainKHR(hgVkDevice, window->swapchain, nullptr);
}

VkCommandBuffer HgWindow::beginRecording()
{
    hgAssert(hgVkDevice != nullptr);
    if (width == 0 || height == 0)
        return nullptr;

retry:
    currentFrame = (currentFrame + 1) % imageCount;
    vkWaitForFences(hgVkDevice, 1, &frameFinished[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(hgVkDevice, 1, &frameFinished[currentFrame]);

    VkResult result = vkAcquireNextImageKHR(
        hgVkDevice, swapchain, UINT64_MAX, imageAvailable[currentFrame], nullptr, &currentImage);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        hgInternalResizeWindowSwapchain(this);
        goto retry;
    }
    hgAssert(result == VK_SUCCESS);

    VkCommandBuffer cmd = cmds[currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    return cmd;
}

void HgWindow::endAndPresent(VkCommandBuffer cmd)
{
    hgAssert(cmd == cmds[currentFrame]);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &imageAvailable[currentFrame];
    VkPipelineStageFlags stageFlags{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit.pWaitDstStageMask = &stageFlags;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &readyToPresent[currentFrame];

    vkQueueSubmit(hgVkQueue, 1, &submit, frameFinished[currentFrame]);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &readyToPresent[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImage;

    vkQueuePresentKHR(hgVkQueue, &presentInfo);
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

#define HG_LOAD_VULKAN_INSTANCE_FUNC(instance, name) \
    vulkanFuncs. name = (PFN_##name)vulkanFuncs.vkGetInstanceProcAddr(instance, #name); \
    if (vulkanFuncs. name == nullptr) { hgError("Could not load " #name "\n"); }

void hgVkLoadInstanceFuncs(VkInstance instance)
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

void hgVkLoadDeviceFuncs(VkDevice device)
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

static void* libvulkan = nullptr;

#define HG_LOAD_VULKAN_FUNC(name) \
    vulkanFuncs. name = (PFN_##name)vulkanFuncs.vkGetInstanceProcAddr(nullptr, #name); \
    if (vulkanFuncs. name == nullptr) { hgError("Could not load " #name "\n"); }

#if defined(HG_PLATFORM_LINUX)

#include <dlfcn.h>

void hgVulkanInit()
{
    if (libvulkan == nullptr)
        libvulkan = dlopen("libvulkan.so.1", RTLD_LAZY);
    if (libvulkan == nullptr)
        hgError("Could not load vulkan dynamic lib: %s\n", dlerror());

    *(void**)&vulkanFuncs.vkGetInstanceProcAddr = dlsym(libvulkan, "vkGetInstanceProcAddr");
    if (vulkanFuncs.vkGetInstanceProcAddr == nullptr)
        hgError("Could not load vkGetInstanceProcAddr: %s\n", dlerror());

    HG_LOAD_VULKAN_FUNC(vkCreateInstance);
}

void hgVulkanDeinit()
{
    if (libvulkan != nullptr)
    {
        dlclose(libvulkan);
        libvulkan = nullptr;
    }
}

#elif defined(HG_PLATFORM_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void hgVulkanInit()
{
    if (libvulkan == nullptr)
        libvulkan = (void*)LoadLibraryA("vulkan-1.dll");
    if (libvulkan == nullptr)
        hgError("Could not load vulkan dynamic lib\n");

    vulkanFuncs.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)
        GetProcAddress((HMODULE)libvulkan, "vkGetInstanceProcAddr");
    if (vulkanFuncs.vkGetInstanceProcAddr == nullptr)
        hgError("Could not load vkGetInstanceProcAddr\n");

    HG_LOAD_VULKAN_FUNC(vkCreateInstance);
}

void hgVulkanDeinit()
{
    if (libvulkan != nullptr)
    {
        FreeLibrary((HMODULE)libvulkan);
        libvulkan = nullptr;
    }
}

#endif

#undef HG_LOAD_VULKAN_FUNC

