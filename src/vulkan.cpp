#include "hurdygurdy.hpp"

static void hgVulkanInit();
static void hgVulkanDeinit();

#ifdef HG_VK_DEBUG_MESSENGER
static VkDebugUtilsMessengerEXT vkDebugMessenger = nullptr;
#endif

static void initBindless();
static void deinitBindless();

static VkDescriptorPool bindlessPool = nullptr;
static VkDescriptorSetLayout bindlessLayout = nullptr;
static VkDescriptorSet bindlessSet = nullptr;

static HgDescriptor* samplerPool = nullptr;
HgDescriptor samplerPoolNext = {0};
static HgDescriptor* combinedImageSamplerPool = nullptr;
HgDescriptor combinedImageSamplerPoolNext = {0};
static HgDescriptor* sampledImagePool = nullptr;
HgDescriptor sampledImagePoolNext = {0};
static HgDescriptor* storageImagePool = nullptr;
HgDescriptor storageImagePoolNext = {0};
static HgDescriptor* uniformTexelBufferPool = nullptr;
HgDescriptor uniformTexelBufferPoolNext = {0};
static HgDescriptor* storageTexelBufferPool = nullptr;
HgDescriptor storageTexelBufferPoolNext = {0};
static HgDescriptor* uniformBufferPool = nullptr;
HgDescriptor uniformBufferPoolNext = {0};
static HgDescriptor* storageBufferPool = nullptr;
HgDescriptor storageBufferPoolNext = {0};

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
        hgVkInstance = hgCreateVkInstance(exts, extCount);
        hgLoadVulkanInstanceFuncs(hgVkInstance);
    }

#ifdef HG_VK_DEBUG_MESSENGER
    if (vkDebugMessenger == nullptr)
        vkDebugMessenger = hgCreateVkDebugUtilsMessenger();
#endif

    if (hgVkPhysicalDevice == nullptr)
    {
        hgVkPhysicalDevice = hgFindVkPhysicalDevice();
        hgFindVkQueueFamily(hgVkPhysicalDevice, &hgVkQueueFamily,
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT);
    }

    if (hgVkDevice == nullptr)
    {
        hgVkDevice = hgCreateVkDevice();
        hgLoadVulkanDeviceFuncs(hgVkDevice);
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

    initBindless();
}

void hgDeinitGraphics()
{
    deinitBindless();

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

static HgDescriptor* createBindlessPool(HgDescriptorType type, HgDescriptor* next)
{
    HgDescriptor* pool = (HgDescriptor*)malloc(UINT16_MAX * sizeof(HgDescriptor));

    for (u32 i = 0; i < UINT16_MAX; ++i)
    {
        pool[i] = {i + 1};
        pool[i].setType(type);
    }
    *next = {0};
    next->setType(type);

    return pool;
}

static void initBindless()
{
    if (bindlessPool == nullptr)
    {
        VkDescriptorPoolSize sizes[]{
            {VK_DESCRIPTOR_TYPE_SAMPLER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, UINT16_MAX},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, UINT16_MAX},
        };

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        info.maxSets = 1;
        info.poolSizeCount = sizeof(sizes) / sizeof(*sizes);
        info.pPoolSizes = sizes;

        VkResult result = vkCreateDescriptorPool(hgVkDevice, &info, nullptr, &bindlessPool);
        if (bindlessPool == nullptr)
            hgError("Could not create bindless VkDescriptorPool: %s\n", hgVkResultToStr(result));
    };

    if (bindlessLayout == nullptr)
    {
        VkDescriptorSetLayoutBinding bindings[HgDescriptorType_count]{};
        VkDescriptorBindingFlags flags[HgDescriptorType_count]{};
        for (u32 i = 0; i < HgDescriptorType_count; ++i)
        {
            bindings[i].descriptorCount = UINT16_MAX;
            bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
            flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        }
        bindings[0].binding = HgDescriptorType_sampler;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        bindings[1].binding = HgDescriptorType_combinedImageSampler;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].binding = HgDescriptorType_sampledImage;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[3].binding = HgDescriptorType_storageImage;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[4].binding = HgDescriptorType_uniformTexelBuffer;
        bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        bindings[5].binding = HgDescriptorType_storageTexelBuffer;
        bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        bindings[6].binding = HgDescriptorType_uniformBuffer;
        bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[7].binding = HgDescriptorType_storageBuffer;
        bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
        flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flagsInfo.bindingCount = sizeof(bindings) / sizeof(*bindings);
        flagsInfo.pBindingFlags = flags;

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.pNext = &flagsInfo;
        info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        info.bindingCount = sizeof(bindings) / sizeof(*bindings);
        info.pBindings = bindings;

        VkResult result = vkCreateDescriptorSetLayout(hgVkDevice, &info, nullptr, &bindlessLayout);
        if (bindlessLayout == nullptr)
            hgError("Could not create bindless VkDescriptorSetLayout: %s\n", hgVkResultToStr(result));
    }

    if (bindlessSet == nullptr)
    {
        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = bindlessPool;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &bindlessLayout;

        VkResult result = vkAllocateDescriptorSets(hgVkDevice, &info, &bindlessSet);
        if (bindlessSet == nullptr)
            hgError("Could not allocate bindless VkDescriptorSet: %s\n", hgVkResultToStr(result));
    }

    if (samplerPool == nullptr)
        samplerPool = createBindlessPool(HgDescriptorType_sampler, &samplerPoolNext);

    if (combinedImageSamplerPool == nullptr)
        combinedImageSamplerPool = createBindlessPool(HgDescriptorType_combinedImageSampler, &combinedImageSamplerPoolNext);

    if (sampledImagePool == nullptr)
        sampledImagePool = createBindlessPool(HgDescriptorType_sampledImage, &sampledImagePoolNext);

    if (storageImagePool == nullptr)
        storageImagePool = createBindlessPool(HgDescriptorType_storageImage, &storageImagePoolNext);

    if (uniformTexelBufferPool == nullptr)
        uniformTexelBufferPool = createBindlessPool(HgDescriptorType_uniformTexelBuffer, &uniformTexelBufferPoolNext);

    if (storageTexelBufferPool == nullptr)
        storageTexelBufferPool = createBindlessPool(HgDescriptorType_storageTexelBuffer, &storageTexelBufferPoolNext);

    if (uniformBufferPool == nullptr)
        uniformBufferPool = createBindlessPool(HgDescriptorType_uniformBuffer, &uniformBufferPoolNext);

    if (storageBufferPool == nullptr)
        storageBufferPool = createBindlessPool(HgDescriptorType_storageBuffer, &storageBufferPoolNext);

}

static void deinitBindless()
{
    if (samplerPool != nullptr)
    {
        free(samplerPool);
        samplerPool = nullptr;
    }
    if (combinedImageSamplerPool != nullptr)
    {
        free(combinedImageSamplerPool);
        combinedImageSamplerPool = nullptr;
    }
    if (sampledImagePool != nullptr)
    {
        free(sampledImagePool);
        sampledImagePool = nullptr;
    }
    if (storageImagePool != nullptr)
    {
        free(storageImagePool);
        storageImagePool = nullptr;
    }
    if (uniformTexelBufferPool != nullptr)
    {
        free(uniformTexelBufferPool);
        uniformTexelBufferPool = nullptr;
    }
    if (storageTexelBufferPool != nullptr)
    {
        free(storageTexelBufferPool);
        storageTexelBufferPool = nullptr;
    }
    if (uniformBufferPool != nullptr)
    {
        free(uniformBufferPool);
        uniformBufferPool = nullptr;
    }
    if (storageBufferPool != nullptr)
    {
        free(storageBufferPool);
        storageBufferPool = nullptr;
    }

    bindlessSet = nullptr;

    if (bindlessLayout != nullptr)
    {
        vkDestroyDescriptorSetLayout(hgVkDevice, bindlessLayout, nullptr);
        bindlessLayout = nullptr;
    }

    if (bindlessPool != nullptr)
    {
        vkDestroyDescriptorPool(hgVkDevice, bindlessPool, nullptr);
        bindlessPool = nullptr;
    }
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

VkInstance hgCreateVkInstance(HgStringView* extensions, u32 extensionCount)
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
    for (u32 i = 0; i < extensionCount; ++i)
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

VkDebugUtilsMessengerEXT hgCreateVkDebugUtilsMessenger()
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

bool hgFindVkQueueFamily(VkPhysicalDevice gpu, u32* queueFamily, VkQueueFlags queueFlags)
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

VkPhysicalDevice hgFindVkPhysicalDevice()
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

        for (u32 j = 0; j < sizeof(vkDeviceExtensions) / sizeof(*vkDeviceExtensions); j++)
        {
            for (u32 k = 0; k < newPropCount; k++)
            {
                if (strcmp(vkDeviceExtensions[j], extProps[k].extensionName) == 0)
                    goto nextExt;
            }
            goto nextGpu;
nextExt:
            continue;
        }

        if (!hgFindVkQueueFamily(gpu, &family,
                VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT))
            goto nextGpu;

        return gpu;

nextGpu:
        continue;
    }

    hgWarn("Could not find a suitable gpu\n");
    return nullptr;
}

VkDevice hgCreateVkDevice()
{
    hgAssert(hgVkPhysicalDevice != nullptr);
    hgAssert(hgVkQueueFamily != (u32)-1);

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
    queueInfo.queueFamilyIndex = hgVkQueueFamily;
    queueInfo.queueCount = 1;
    f32 queuePriority = 1.0f;
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

u32 hgFindVkMemoryTypeIndex(
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

HgBuffer* hgCreateBuffer(
    u64 size,
    VkBufferUsageFlags usage,
    HgBufferMemoryUsage access)
{
    HgBuffer* buffer = (HgBuffer*)malloc(sizeof(*buffer));
    *buffer = {};

    hgAssert(size > 0);
    hgAssert(usage != 0);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    switch (access)
    {
    case HgBufferMemoryUsage_deviceOnly:
        break;
    case HgBufferMemoryUsage_frequentUpdate:
        allocInfo.flags
            = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        break;
    case HgBufferMemoryUsage_stagingWrite:
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        break;
    case HgBufferMemoryUsage_stagingRead:
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        break;
    }

    VkResult result = vmaCreateBuffer(
        hgVkVma,
        &bufferInfo,
        &allocInfo,
        &buffer->buffer,
        &buffer->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        hgError("Could not create VkBuffer: %s", hgVkResultToStr(result));

    buffer->size = size;

    switch (access)
    {
    case HgBufferMemoryUsage_deviceOnly:
        buffer->access = HgBufferMemoryHostAccess_none;
        break;
    case HgBufferMemoryUsage_frequentUpdate:
        VkMemoryPropertyFlags memPropFlags;
        vmaGetAllocationMemoryProperties(hgVkVma, buffer->alloc, &memPropFlags);
        buffer->access = memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            ? HgBufferMemoryHostAccess_write
            : HgBufferMemoryHostAccess_none;
        break;
    case HgBufferMemoryUsage_stagingWrite:
        buffer->access = HgBufferMemoryHostAccess_write;
        break;
    case HgBufferMemoryUsage_stagingRead:
        buffer->access = HgBufferMemoryHostAccess_read;
        break;
    }

    return buffer;
}

void hgDestroyBuffer(HgBuffer *buffer)
{
    if (buffer != nullptr)
    {
        vmaDestroyBuffer(hgVkVma, buffer->buffer, buffer->alloc);
        free(buffer);
    }
}

void hgWriteBuffer(HgBuffer* dst, u64 offset, const void* src, u64 size)
{
    hgAssert(dst != nullptr);
    hgAssert(src != nullptr);

    if (dst->access & HgBufferMemoryHostAccess_write)
    {
        VkResult result = vmaCopyMemoryToAllocation(hgVkVma, src, dst->alloc, offset, size);
        if (result != VK_SUCCESS)
            hgError("Could not write gpu buffer: %s", hgVkResultToStr(result));
        return;
    }

    HgBuffer* stage = hgCreateBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HgBufferMemoryUsage_stagingWrite);
    hgDefer(hgDestroyBuffer(stage));
    hgWriteBuffer(stage, 0, src, size);

    VkCommandBuffer cmd = hgBeginVkCmd();
    {
        VkBufferCopy region{};
        region.dstOffset = offset;
        region.size = size;

        vkCmdCopyBuffer(cmd, stage->buffer, dst->buffer, 1, &region);
    }
    hgEndVkCmd(cmd);
}

void hgReadBuffer(void* dst, HgBuffer* src, u64 offset, u64 size)
{
    hgAssert(dst != nullptr);
    hgAssert(src != nullptr);

    if (src->access & HgBufferMemoryHostAccess_read)
    {
        VkResult result = vmaCopyAllocationToMemory(hgVkVma, src->alloc, offset, dst, size);
        if (result != VK_SUCCESS)
            hgError("Could not read gpu buffer: %s", hgVkResultToStr(result));
        return;
    }

    HgBuffer* stage = hgCreateBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, HgBufferMemoryUsage_stagingRead);
    hgDefer(hgDestroyBuffer(stage));

    VkCommandBuffer cmd = hgBeginVkCmd();
    {
        VkBufferCopy region{};
        region.srcOffset = offset;
        region.size = size;

        vkCmdCopyBuffer(cmd, src->buffer, stage->buffer, 1, &region);
    }
    hgEndVkCmd(cmd);

    hgReadBuffer(dst, stage, 0, size);
}

HgImage* hgCreateImage(u32 width, u32 height, VkFormat format, VkImageUsageFlags usage)
{
    HgCreateImageEx create{};
    create.width = width;
    create.height = height;
    create.format = format;
    create.usage = usage;
    return hgCreateImageEx(create);
}

HgImage* hgCreateImageEx(const HgCreateImageEx& create)
{
    hgAssert(create.format != VK_FORMAT_UNDEFINED);
    hgAssert(create.usage != 0);

    HgImage* image = (HgImage*)malloc(sizeof(*image));
    *image = {};

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = create.type;
    imageInfo.format = create.format;
    imageInfo.extent = {create.width, create.height, create.depth};
    imageInfo.mipLevels = create.mipLevels;
    imageInfo.arrayLayers = create.arrayLayers;
    imageInfo.samples = create.msaaSamples;
    imageInfo.usage = create.usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult result = vmaCreateImage(
        hgVkVma,
        &imageInfo,
        &allocInfo,
        &image->image,
        &image->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        hgError("Could not create VkImage: %s", hgVkResultToStr(result));

    image->type = create.type;
    image->format = create.format;
    image->width = create.width;
    image->height = create.height;
    image->depth = create.depth;
    image->mipLevels = create.mipLevels;
    image->arrayLayers = create.arrayLayers;
    image->msaaSamples = create.msaaSamples;

    return image;
}

void hgDestroyImage(HgImage* image)
{
    if (image != nullptr)
    {
        vmaDestroyImage(hgVkVma, image->image, image->alloc);
        free(image);
    }
}

HgImageView* hgCreateImageView(
    const HgImage* image,
    VkImageSubresourceRange subresource,
    VkImageViewType type)
{
    hgAssert(image != nullptr);
    hgAssert(subresource.aspectMask != 0);

    HgImageView* view = (HgImageView*)malloc(sizeof(*view));
    *view = {};

    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image->image;
    info.viewType = type;
    info.format = image->format;
    info.subresourceRange = subresource;

    VkResult result = vkCreateImageView(hgVkDevice, &info, nullptr, &view->view);
    if (view == nullptr)
        hgError("Could not create VkImageView: %s\n", hgVkResultToStr(result));

    view->image = image;
    view->type = type;
    view->aspectFlags = subresource.aspectMask;
    view->baseMipLevel = subresource.baseMipLevel;
    view->levelCount = subresource.levelCount;
    view->baseArrayLayer = subresource.baseArrayLayer;
    view->layerCount = subresource.layerCount;

    return view;
}

void hgDestroyImageView(HgImageView* view)
{
    if (view != nullptr)
    {
        vkDestroyImageView(hgVkDevice, view->view, nullptr);
        free(view);
    }
}

VkSampler hgCreateVkSampler(VkFilter filter, VkSamplerAddressMode addressMode, VkBorderColor borderColor)
{
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = filter;
    info.minFilter = filter;
    info.mipmapMode = filter == VK_FILTER_LINEAR
        ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = addressMode;
    info.addressModeV = addressMode;
    info.addressModeW = addressMode;
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 1000.0f;
    info.borderColor = borderColor;

    VkSampler sampler = nullptr;
    VkResult result = vkCreateSampler(hgVkDevice, &info, nullptr, &sampler);
    if (sampler == nullptr)
        hgError("Could not create VkSampler: %s", hgVkResultToStr(result));

    return sampler;
}

void hgWriteImage(
    HgImage* dst,
    VkImageSubresourceLayers subresource,
    const void* src,
    VkImageLayout layout)
{
    hgAssert(dst!= nullptr);
    hgAssert(src!= nullptr);

    u64 size = dst->width * dst->height * dst->depth * hgVkFormatToSize(dst->format);

    HgBuffer* stage = hgCreateBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        HgBufferMemoryUsage_stagingWrite);
    hgDefer(hgDestroyBuffer(stage));
    hgWriteBuffer(stage, 0, src, size);

    VkCommandBuffer cmd = hgBeginVkCmd();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarrier.image = dst->image;
    transferBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    transferBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    transferBarrier.subresourceRange.levelCount = 1;
    transferBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(cmd, &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource = subresource;
    region.imageExtent = {dst->width, dst->height, dst->depth};

    vkCmdCopyBufferToImage(cmd, stage->buffer, dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier2 endBarrier{};
    endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    endBarrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    endBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    endBarrier.newLayout = layout;
    endBarrier.image = dst->image;
    endBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    endBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    endBarrier.subresourceRange.levelCount = 1;
    endBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    endBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo endDep{};
    endDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    endDep.imageMemoryBarrierCount = 1;
    endDep.pImageMemoryBarriers = &endBarrier;

    vkCmdPipelineBarrier2(cmd, &endDep);

    hgEndVkCmd(cmd);
}

void hgWriteImageCubemap(
    HgImage* dst,
    VkImageSubresourceLayers subresource,
    const void* src,
    VkImageLayout layout)
{
    hgAssert(dst != nullptr);
    hgAssert(subresource.baseArrayLayer == 0);
    hgAssert(subresource.layerCount == 6);
    hgAssert(src != nullptr);
    hgAssert(dst->depth == 1);

    u64 size = dst->width * dst->height * hgVkFormatToSize(dst->format);

    HgBuffer* buffer = hgCreateBuffer(
        size * 4 * 3,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        HgBufferMemoryUsage_stagingWrite);
    hgDefer(hgDestroyBuffer(buffer));
    hgWriteBuffer(buffer, 0, src, size);

    HgImage* stage = hgCreateImage(
        dst->width * 4,
        dst->height * 3,
        dst->format,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    hgDefer(hgDestroyImage(stage));

    VkCommandBuffer cmd = hgBeginVkCmd();

    VkImageMemoryBarrier2 stageBarrier{};
    stageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    stageBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    stageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    stageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    stageBarrier.image = dst->image;
    stageBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    stageBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    stageBarrier.subresourceRange.levelCount = 1;
    stageBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    stageBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo stageDep{};
    stageDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    stageDep.imageMemoryBarrierCount = 1;
    stageDep.pImageMemoryBarriers = &stageBarrier;

    vkCmdPipelineBarrier2(cmd, &stageDep);

    VkBufferImageCopy stageRegion{};
    stageRegion.imageSubresource = subresource;
    stageRegion.imageExtent = {dst->width, dst->height, 1};

    vkCmdCopyBufferToImage(cmd, buffer->buffer, stage->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &stageRegion);

    VkImageMemoryBarrier2 transferBarriers[2]{};

    transferBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[0].srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[0].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarriers[0].image = stage->image;
    transferBarriers[0].subresourceRange.aspectMask = subresource.aspectMask;
    transferBarriers[0].subresourceRange.baseMipLevel = 0;
    transferBarriers[0].subresourceRange.levelCount = 1;
    transferBarriers[0].subresourceRange.baseArrayLayer = 0;
    transferBarriers[0].subresourceRange.layerCount = 1;

    transferBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[1].image = dst->image;
    transferBarriers[1].subresourceRange.aspectMask = subresource.aspectMask;
    transferBarriers[1].subresourceRange.baseMipLevel = subresource.mipLevel;
    transferBarriers[1].subresourceRange.levelCount = 1;
    transferBarriers[1].subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    transferBarriers[1].subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = sizeof(transferBarriers) / sizeof(*transferBarriers);
    transferDep.pImageMemoryBarriers = transferBarriers;

    vkCmdPipelineBarrier2(cmd, &transferDep);

    VkImageCopy regions[6]{};

    regions[0].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[0].srcOffset = {(int)dst->width * 2, (int)dst->height * 1, 0};
    regions[0].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 0, 1};
    regions[0].dstOffset = {};
    regions[0].extent = {dst->width, dst->height, 1};

    regions[1].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[1].srcOffset = {(int)dst->width * 0, (int)dst->height * 1, 0};
    regions[1].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 1, 1};
    regions[1].dstOffset = {};
    regions[1].extent = {dst->width, dst->height, 1};

    regions[2].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[2].srcOffset = {(int)dst->width * 1, (int)dst->height * 0, 0};
    regions[2].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 2, 1};
    regions[2].dstOffset = {};
    regions[2].extent = {dst->width, dst->height, 1};

    regions[3].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[3].srcOffset = {(int)dst->width * 1, (int)dst->height * 2, 0};
    regions[3].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 3, 1};
    regions[3].dstOffset = {};
    regions[3].extent = {dst->width, dst->height, 1};

    regions[4].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[4].srcOffset = {(int)dst->width * 1, (int)dst->height * 1, 0};
    regions[4].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 4, 1};
    regions[4].dstOffset = {};
    regions[4].extent = {dst->width, dst->height, 1};

    regions[5].srcSubresource = {subresource.aspectMask, 0, 0, 1};
    regions[5].srcOffset = {(int)dst->width * 3, (int)dst->height * 1, 0};
    regions[5].dstSubresource = {subresource.aspectMask, subresource.mipLevel, 5, 1};
    regions[5].dstOffset = {};
    regions[5].extent = {dst->width, dst->height, 1};

    vkCmdCopyImage(
        cmd,
        stage->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        sizeof(regions) / sizeof(*regions),
        regions);

    VkImageMemoryBarrier2 endBarrier{};
    endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    endBarrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    endBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    endBarrier.newLayout = layout;
    endBarrier.image = dst->image;
    endBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    endBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    endBarrier.subresourceRange.levelCount = 1;
    endBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    endBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo endDep{};
    endDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    endDep.imageMemoryBarrierCount = 1;
    endDep.pImageMemoryBarriers = &endBarrier;

    vkCmdPipelineBarrier2(cmd, &endDep);

    hgEndVkCmd(cmd);
}

void hgReadImage(
    void* dst,
    HgImage* src,
    VkImageSubresourceLayers subresource,
    VkImageLayout layout)
{
    hgAssert(src != nullptr);
    hgAssert(layout != VK_IMAGE_LAYOUT_UNDEFINED);
    hgAssert(dst != nullptr);

    u64 size = src->width * src->height * src->depth * hgVkFormatToSize(src->format);

    HgBuffer* stage = hgCreateBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        HgBufferMemoryUsage_stagingRead);
    hgDefer(hgDestroyBuffer(stage));

    VkCommandBuffer cmd = hgBeginVkCmd();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarrier.oldLayout = layout;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarrier.image = src->image;
    transferBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    transferBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    transferBarrier.subresourceRange.levelCount = 1;
    transferBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(cmd, &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource = subresource;
    region.imageExtent = {src->width, src->height, src->depth};

    vkCmdCopyImageToBuffer(cmd, src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stage->buffer, 1, &region);

    VkImageMemoryBarrier2 endBarrier{};
    endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    endBarrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    endBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    endBarrier.newLayout = layout;
    endBarrier.image = src->image;
    endBarrier.subresourceRange.aspectMask = subresource.aspectMask;
    endBarrier.subresourceRange.baseMipLevel = subresource.mipLevel;
    endBarrier.subresourceRange.levelCount = 1;
    endBarrier.subresourceRange.baseArrayLayer = subresource.baseArrayLayer;
    endBarrier.subresourceRange.layerCount = subresource.layerCount;

    VkDependencyInfo endDep{};
    endDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    endDep.imageMemoryBarrierCount = 1;
    endDep.pImageMemoryBarriers = &endBarrier;

    vkCmdPipelineBarrier2(cmd, &endDep);

    hgEndVkCmd(cmd);

    hgReadBuffer(dst, stage, 0, size);
}

void hgGenerateMipmaps(
    HgImage* image,
    VkImageAspectFlags aspectFlags,
    VkImageLayout oldLayout,
    VkImageLayout newLayout)
{
    hgAssert(image != nullptr);
    hgAssert(oldLayout != VK_IMAGE_LAYOUT_UNDEFINED);
    hgAssert(newLayout != VK_IMAGE_LAYOUT_UNDEFINED);
    if (image->mipLevels == 1)
        return;

    VkCommandBuffer cmd = hgBeginVkCmd();

    VkOffset3D mipOffset{};
    mipOffset.x = (i32)image->width;
    mipOffset.y = (i32)image->height;
    mipOffset.z = (i32)image->depth;

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = image->image;
    barrier.subresourceRange.aspectMask = aspectFlags;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &dep);

    for (u32 level = 0; level < image->mipLevels - 1; ++level)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.subresourceRange.aspectMask = aspectFlags;
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(cmd, &dep);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = aspectFlags;
        blit.srcSubresource.mipLevel = level;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[1] = mipOffset;
        if (mipOffset.x > 1)
            mipOffset.x /= 2;
        if (mipOffset.y > 1)
            mipOffset.y /= 2;
        if (mipOffset.z > 1)
            mipOffset.z /= 2;
        blit.dstSubresource.aspectMask = aspectFlags;
        blit.dstSubresource.mipLevel = level + 1;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[1] = mipOffset;

        vkCmdBlitImage(cmd,
            image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.subresourceRange.aspectMask = aspectFlags;
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(cmd, &dep);
    }

    barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.dstAccessMask = VK_ACCESS_NONE;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = newLayout;
    barrier.subresourceRange.aspectMask = aspectFlags;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = image->mipLevels;

    vkCmdPipelineBarrier2(cmd, &dep);

    hgEndVkCmd(cmd);
}

static HgDescriptor allocDescriptor(HgDescriptor* pool, HgDescriptor* next)
{
    HgDescriptor desc = *next;

    u32 idx = next->idx();
    *next = pool[idx];

    pool[desc.idx()] = desc;
    return desc;
}

static void deallocDescriptor(HgDescriptor* pool, HgDescriptor* next, HgDescriptor desc)
{
    u32 idx = desc.idx();
    pool[idx] = *next;

    desc.incrementGeneration();
    *next = desc;
}

HgDescriptor hgCreateDescriptor(HgDescriptorType type)
{
    switch (type)
    {
        case HgDescriptorType_sampler:
            hgAssert(samplerPoolNext.idx() != UINT16_MAX);
            return allocDescriptor(samplerPool, &samplerPoolNext);
            break;
        case HgDescriptorType_combinedImageSampler:
            hgAssert(combinedImageSamplerPoolNext.idx() != UINT16_MAX);
            return allocDescriptor(combinedImageSamplerPool, &combinedImageSamplerPoolNext);
            break;
        case HgDescriptorType_sampledImage:
            hgAssert(sampledImagePoolNext.idx() != UINT16_MAX);
            return allocDescriptor(sampledImagePool, &sampledImagePoolNext);
            break;
        case HgDescriptorType_storageImage:
            hgAssert(storageImagePoolNext.idx() != UINT16_MAX);
            return allocDescriptor(storageImagePool, &storageImagePoolNext);
            break;
        case HgDescriptorType_uniformTexelBuffer:
            hgAssert(uniformTexelBufferPoolNext.idx() != UINT16_MAX);
            return allocDescriptor(uniformTexelBufferPool, &uniformTexelBufferPoolNext);
            break;
        case HgDescriptorType_storageTexelBuffer:
            hgAssert(storageTexelBufferPoolNext.idx() != UINT16_MAX);
            return allocDescriptor(storageTexelBufferPool, &storageTexelBufferPoolNext);
            break;
        case HgDescriptorType_uniformBuffer:
            hgAssert(uniformBufferPoolNext.idx() != UINT16_MAX);
            return allocDescriptor(uniformBufferPool, &uniformBufferPoolNext);
            break;
        case HgDescriptorType_storageBuffer:
            hgAssert(storageBufferPoolNext.idx() != UINT16_MAX);
            return allocDescriptor(storageBufferPool, &storageBufferPoolNext);
            break;
        default:
            hgAssert(false);
            break;
    }
}

void hgDestroyDescriptor(HgDescriptor descriptor)
{
    if (descriptor.id == HgDescriptor{}.id)
        return;

    switch (descriptor.type())
    {
        case HgDescriptorType_sampler:
            hgAssert(descriptor.id == samplerPool[descriptor.idx()].id);
            deallocDescriptor(samplerPool, &samplerPoolNext, descriptor);
            break;
        case HgDescriptorType_combinedImageSampler:
            hgAssert(descriptor.id == combinedImageSamplerPool[descriptor.idx()].id);
            deallocDescriptor(combinedImageSamplerPool, &combinedImageSamplerPoolNext, descriptor);
            break;
        case HgDescriptorType_sampledImage:
            hgAssert(descriptor.id == sampledImagePool[descriptor.idx()].id);
            deallocDescriptor(sampledImagePool, &sampledImagePoolNext, descriptor);
            break;
        case HgDescriptorType_storageImage:
            hgAssert(descriptor.id == storageImagePool[descriptor.idx()].id);
            deallocDescriptor(storageImagePool, &storageImagePoolNext, descriptor);
            break;
        case HgDescriptorType_uniformTexelBuffer:
            hgAssert(descriptor.id == uniformTexelBufferPool[descriptor.idx()].id);
            deallocDescriptor(uniformTexelBufferPool, &uniformTexelBufferPoolNext, descriptor);
            break;
        case HgDescriptorType_storageTexelBuffer:
            hgAssert(descriptor.id == storageTexelBufferPool[descriptor.idx()].id);
            deallocDescriptor(storageTexelBufferPool, &storageTexelBufferPoolNext, descriptor);
            break;
        case HgDescriptorType_uniformBuffer:
            hgAssert(descriptor.id == uniformBufferPool[descriptor.idx()].id);
            deallocDescriptor(uniformBufferPool, &uniformBufferPoolNext, descriptor);
            break;
        case HgDescriptorType_storageBuffer:
            hgAssert(descriptor.id == storageBufferPool[descriptor.idx()].id);
            deallocDescriptor(storageBufferPool, &storageBufferPoolNext, descriptor);
            break;
        default:
            hgAssert(false);
            break;
    }
}

VkDescriptorType hgDescriptorTypeToVk(HgDescriptorType type)
{
    switch (type)
    {
        case HgDescriptorType_sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        case HgDescriptorType_combinedImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            break;
        case HgDescriptorType_sampledImage:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            break;
        case HgDescriptorType_storageImage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            break;
        case HgDescriptorType_uniformTexelBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            break;
        case HgDescriptorType_storageTexelBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            break;
        case HgDescriptorType_uniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        case HgDescriptorType_storageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
        default:
            break;
    }
    hgAssert(false);
}

void hgUpdateDescriptor(
    HgDescriptor descriptor,
    const VkDescriptorBufferInfo* bufferInfo,
    const VkDescriptorImageInfo* imageInfo,
    const VkBufferView* texelInfo)
{
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = bindlessSet;
    write.dstBinding = descriptor.type();
    write.dstArrayElement = descriptor.idx();
    write.descriptorCount = 1;
    write.descriptorType = hgDescriptorTypeToVk(descriptor.type());
    write.pBufferInfo = bufferInfo;
    write.pImageInfo = imageInfo;
    write.pTexelBufferView = texelInfo;

    vkUpdateDescriptorSets(hgVkDevice, 1, &write, 0, nullptr);
}

VkPipelineLayout hgCreatePipelineLayout(const VkPushConstantRange* pushRanges, u32 pushRangeCount)
{
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = 1;
    info.pSetLayouts = &bindlessLayout;
    info.pushConstantRangeCount = pushRangeCount;
    info.pPushConstantRanges = pushRanges;

    VkPipelineLayout layout = nullptr;
    VkResult result = vkCreatePipelineLayout(hgVkDevice, &info, nullptr, &layout);
    if (layout == nullptr)
        hgError("Could not create bindless VkPipelineLayout: %s\n", hgVkResultToStr(result));

    return layout;
}

static VkShaderModule createVkShaderModule(const u8* spirvCode, u64 codeSize)
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

VkPipeline hgCreateGraphicsPipeline(const HgCreateGraphicsPipeline& config)
{
    hgAssert(config.layout != nullptr);
    hgAssert(config.vertexShader != nullptr);
    hgAssert(config.fragmentShader != nullptr);
    if (config.colorAttachmentCount > 0)
        hgAssert(config.colorAttachmentFormats != nullptr);
    if (config.vertexBindingCount > 0)
        hgAssert(config.vertexBindings != nullptr);

    VkShaderModule vertexShader = createVkShaderModule(config.vertexShader, config.vertexShaderSize);
    VkShaderModule fragmentShader = createVkShaderModule(config.fragmentShader, config.fragmentShaderSize);
    hgDefer(vkDestroyShaderModule(hgVkDevice, vertexShader, nullptr));
    hgDefer(vkDestroyShaderModule(hgVkDevice, fragmentShader, nullptr));

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

void hgBindGraphicsPipeline(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout pipelineLayout)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &bindlessSet, 0, nullptr);
}

VkPipeline hgCreateComputePipeline(VkPipelineLayout layout, const u8* shaderCode, u64 shaderCodeSize)
{
    hgAssert(layout != nullptr);
    hgAssert(shaderCode != nullptr);
    hgAssert(shaderCodeSize > 0);

    VkShaderModule computeShader = createVkShaderModule(shaderCode, shaderCodeSize);
    hgDefer(vkDestroyShaderModule(hgVkDevice, computeShader, nullptr));

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = computeShader;
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

void hgBindComputePipeline(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout pipelineLayout)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &bindlessSet, 0, nullptr);
}

VkCommandBuffer hgBeginVkCmd()
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

void hgEndVkCmd(VkCommandBuffer cmd)
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

HgRenderer HgRenderer::create(HgArena* arena, u32 maxBuffers, u32 maxImages)
{
    HgRenderer renderer{};
    renderer.buffers = renderer.buffers.create(arena, maxBuffers);
    renderer.images = renderer.images.create(arena, maxImages);
    return renderer;
}

void HgRenderer::reset()
{
    buffers.empty();
    images.empty();
}

void HgRenderer::setBuffer(HgBuffer* buffer, VkPipelineStageFlags lastStage, VkAccessFlags lastAccess)
{
    buffers.add(buffer, {lastStage, lastAccess});
}

void HgRenderer::setImage(
    HgImageView* image,
    VkPipelineStageFlags lastStage,
    VkAccessFlags lastAccess,
    VkImageLayout lastLayout)
{
    images.add(image, {lastStage, lastAccess, lastLayout});
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

    for (u32 i = 0; i < bufferBarrierCount; ++i)
    {
        BufferState* buffer = buffers.get(bufferBarriers[i].buffer);
        if (buffer == nullptr)
            buffer = buffers.add(bufferBarriers[i].buffer);

        vkBufferBarriers[i] = {};
        vkBufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        vkBufferBarriers[i].srcStageMask = buffer->lastPipelineStage;
        vkBufferBarriers[i].srcAccessMask = buffer->lastAccess;
        vkBufferBarriers[i].dstStageMask = bufferBarriers[i].nextPipelineStage;
        vkBufferBarriers[i].dstAccessMask = bufferBarriers[i].nextAccess;
        vkBufferBarriers[i].buffer = bufferBarriers[i].buffer->buffer;
        vkBufferBarriers[i].size = bufferBarriers[i].buffer->size;

        buffer->lastPipelineStage = bufferBarriers[i].nextPipelineStage;
        buffer->lastAccess = bufferBarriers[i].nextAccess;
    }

    for (u32 i = 0; i < imageBarrierCount; ++i)
    {
        ImageState* image = images.get(imageBarriers[i].image);
        if (image == nullptr)
            image = images.add(imageBarriers[i].image);

        vkImageBarriers[i] = {};
        vkImageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        vkImageBarriers[i].srcStageMask = image->lastPipelineStage;
        vkImageBarriers[i].srcAccessMask = image->lastAccess;
        vkImageBarriers[i].dstStageMask = imageBarriers[i].nextPipelineStage;
        vkImageBarriers[i].dstAccessMask = imageBarriers[i].nextAccess;
        vkImageBarriers[i].oldLayout = image->lastLayout;
        vkImageBarriers[i].newLayout = imageBarriers[i].nextLayout;
        vkImageBarriers[i].image = imageBarriers[i].image->image->image;
        vkImageBarriers[i].subresourceRange = {
            imageBarriers[i].image->aspectFlags,
            imageBarriers[i].image->baseMipLevel,
            imageBarriers[i].image->levelCount,
            imageBarriers[i].image->baseArrayLayer,
            imageBarriers[i].image->layerCount,
        };

        image->lastPipelineStage = imageBarriers[i].nextPipelineStage;
        image->lastAccess = imageBarriers[i].nextAccess;
        image->lastLayout = imageBarriers[i].nextLayout;
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

    for (u32 i = bufferBarrierCount; i < bufferBarrierCount + pass.uniformBufferCount; ++i)
    {
        BufferState* buffer = buffers.get(pass.uniformBuffers[i - bufferBarrierCount]);
        if (buffer == nullptr)
            buffer = buffers.add(pass.uniformBuffers[i - bufferBarrierCount]);

        bufferBarriers[i] = {};
        bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bufferBarriers[i].srcStageMask = buffer->lastPipelineStage;
        bufferBarriers[i].srcAccessMask = buffer->lastAccess;
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                                       | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        bufferBarriers[i].dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        bufferBarriers[i].buffer = pass.uniformBuffers[i - bufferBarrierCount]->buffer;
        bufferBarriers[i].size = pass.uniformBuffers[i - bufferBarrierCount]->size;

        buffer->lastPipelineStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                                 | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        buffer->lastAccess = VK_ACCESS_UNIFORM_READ_BIT;
    }

    bufferBarrierCount += pass.uniformBufferCount;

    bufferBarriers = hgRealloc<VkBufferMemoryBarrier2>(scratch, 
        bufferBarriers, bufferBarrierCount, bufferBarrierCount + pass.storageBufferCount);

    for (u32 i = bufferBarrierCount; i < bufferBarrierCount + pass.storageBufferCount; ++i)
    {
        BufferState* buffer = buffers.get(pass.storageBuffers[i - bufferBarrierCount]);
        if (buffer == nullptr)
            buffer = buffers.add(pass.storageBuffers[i - bufferBarrierCount]);

        bufferBarriers[i] = {};
        bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bufferBarriers[i].srcStageMask = buffer->lastPipelineStage;
        bufferBarriers[i].srcAccessMask = buffer->lastAccess;
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                                       | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        bufferBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT
                                        | VK_ACCESS_SHADER_WRITE_BIT;
        bufferBarriers[i].buffer = pass.uniformBuffers[i - bufferBarrierCount]->buffer;
        bufferBarriers[i].size = pass.uniformBuffers[i - bufferBarrierCount]->size;

        buffer->lastPipelineStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                                  | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        buffer->lastAccess = VK_ACCESS_SHADER_READ_BIT
                           | VK_ACCESS_SHADER_WRITE_BIT;
    }

    bufferBarrierCount += pass.storageBufferCount;

    imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
        imageBarriers, imageBarrierCount, imageBarrierCount + pass.sampledImageCount);

    for (u32 i = imageBarrierCount; i < imageBarrierCount + pass.sampledImageCount; ++i)
    {
        ImageState* image = images.get(pass.sampledImages[i - imageBarrierCount]);
        if (image == nullptr)
            image = images.add(pass.sampledImages[i - imageBarrierCount]);

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = image->lastPipelineStage;
        imageBarriers[i].srcAccessMask = image->lastAccess;
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        imageBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageBarriers[i].oldLayout = image->lastLayout;
        imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarriers[i].image = pass.sampledImages[i - imageBarrierCount]->image->image;
        imageBarriers[i].subresourceRange = {
            pass.sampledImages[i - imageBarrierCount]->aspectFlags,
            pass.sampledImages[i - imageBarrierCount]->baseMipLevel,
            pass.sampledImages[i - imageBarrierCount]->levelCount,
            pass.sampledImages[i - imageBarrierCount]->baseArrayLayer,
            pass.sampledImages[i - imageBarrierCount]->layerCount,
        };

        image->lastPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        image->lastAccess = VK_ACCESS_SHADER_READ_BIT;
        image->lastLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    imageBarrierCount += pass.sampledImageCount;

    imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
        imageBarriers, imageBarrierCount, imageBarrierCount + pass.storageImageCount);

    for (u32 i = imageBarrierCount; i < imageBarrierCount + pass.storageImageCount; ++i)
    {
        ImageState* image = images.get(pass.storageImages[i - imageBarrierCount]);
        if (image == nullptr)
            image = images.add(pass.storageImages[i - imageBarrierCount]);

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = image->lastPipelineStage;
        imageBarriers[i].srcAccessMask = image->lastAccess;
        bufferBarriers[i].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        imageBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT
                                       | VK_ACCESS_SHADER_WRITE_BIT;
        imageBarriers[i].oldLayout = image->lastLayout;
        imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageBarriers[i].image = pass.storageImages[i - imageBarrierCount]->image->image;
        imageBarriers[i].subresourceRange = {
            pass.storageImages[i - imageBarrierCount]->aspectFlags,
            pass.storageImages[i - imageBarrierCount]->baseMipLevel,
            pass.storageImages[i - imageBarrierCount]->levelCount,
            pass.storageImages[i - imageBarrierCount]->baseArrayLayer,
            pass.storageImages[i - imageBarrierCount]->layerCount,
        };

        image->lastPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        image->lastAccess = VK_ACCESS_SHADER_READ_BIT
                          | VK_ACCESS_SHADER_WRITE_BIT;
        image->lastLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    imageBarrierCount += pass.storageImageCount;

    imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
        imageBarriers, imageBarrierCount, imageBarrierCount + pass.colorAttachmentCount);

    for (u32 i = imageBarrierCount; i < imageBarrierCount + pass.colorAttachmentCount; ++i)
    {
        ImageState* image = images.get(pass.colorAttachments[i - imageBarrierCount].image);
        if (image == nullptr)
            image = images.add(pass.colorAttachments[i - imageBarrierCount].image);

        imageBarriers[i] = {};
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[i].srcStageMask = image->lastPipelineStage;
        imageBarriers[i].srcAccessMask = image->lastAccess;
        imageBarriers[i].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        imageBarriers[i].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imageBarriers[i].oldLayout = image->lastLayout;
        imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imageBarriers[i].image = pass.colorAttachments[i - imageBarrierCount].image->image->image;
        imageBarriers[i].subresourceRange = {
            pass.colorAttachments[i - imageBarrierCount].image->aspectFlags,
            pass.colorAttachments[i - imageBarrierCount].image->baseMipLevel,
            pass.colorAttachments[i - imageBarrierCount].image->levelCount,
            pass.colorAttachments[i - imageBarrierCount].image->baseArrayLayer,
            pass.colorAttachments[i - imageBarrierCount].image->layerCount,
        };

        image->lastPipelineStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        image->lastAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        image->lastLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    imageBarrierCount += pass.colorAttachmentCount;

    if (pass.depthAttachment != nullptr)
    {
        ImageState* image = images.get(pass.depthAttachment->image);
        if (image == nullptr)
            image = images.add(pass.depthAttachment->image);

        imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
            imageBarriers, imageBarrierCount, imageBarrierCount + 1);

        u32 idx = imageBarrierCount;
        imageBarriers[idx] = {};
        imageBarriers[idx].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[idx].srcStageMask = image->lastPipelineStage;
        imageBarriers[idx].srcAccessMask = image->lastAccess;
        imageBarriers[idx].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        imageBarriers[idx].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                                         | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        imageBarriers[idx].oldLayout = image->lastLayout;
        imageBarriers[idx].newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        imageBarriers[idx].image = pass.depthAttachment->image->image->image;
        imageBarriers[idx].subresourceRange = {
            pass.depthAttachment->image->aspectFlags,
            pass.depthAttachment->image->baseMipLevel,
            pass.depthAttachment->image->levelCount,
            pass.depthAttachment->image->baseArrayLayer,
            pass.depthAttachment->image->layerCount,
        };

        image->lastPipelineStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        image->lastAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                         | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        image->lastLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

        ++imageBarrierCount;
    }

    if (pass.stencilAttachment != nullptr)
    {
        ImageState* image = images.get(pass.stencilAttachment->image);
        if (image == nullptr)
            image = images.add(pass.stencilAttachment->image);

        imageBarriers = hgRealloc<VkImageMemoryBarrier2>(scratch, 
            imageBarriers, imageBarrierCount, imageBarrierCount + 1);

        u32 idx = imageBarrierCount;
        imageBarriers[idx] = {};
        imageBarriers[idx].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarriers[idx].srcStageMask = image->lastPipelineStage;
        imageBarriers[idx].srcAccessMask = image->lastAccess;
        imageBarriers[idx].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        imageBarriers[idx].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                                         | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        imageBarriers[idx].oldLayout = image->lastLayout;
        imageBarriers[idx].newLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
        imageBarriers[idx].image = pass.stencilAttachment->image->image->image;
        imageBarriers[idx].subresourceRange = {
            pass.stencilAttachment->image->aspectFlags,
            pass.stencilAttachment->image->baseMipLevel,
            pass.stencilAttachment->image->levelCount,
            pass.stencilAttachment->image->baseArrayLayer,
            pass.stencilAttachment->image->layerCount,
        };

        image->lastPipelineStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                                | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        image->lastAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                         | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        image->lastLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;

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

    for (u32 i = 0; i < pass.colorAttachmentCount; ++i)
    {
        colorAttachments[i] = {};
        colorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[i].imageView = pass.colorAttachments[i].image->view;
        colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[i].loadOp = pass.colorAttachments[i].loadOp;
        colorAttachments[i].storeOp = pass.colorAttachments[i].storeOp;
        colorAttachments[i].clearValue = pass.colorAttachments[i].clearValue;
    }

    VkRenderingAttachmentInfo depthAttachment{};
    if (pass.depthAttachment != nullptr)
    {
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = pass.depthAttachment->image->view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = pass.depthAttachment->loadOp;
        depthAttachment.storeOp = pass.depthAttachment->storeOp;
        depthAttachment.clearValue = pass.depthAttachment->clearValue;
    }

    VkRenderingAttachmentInfo stencilAttachment{};
    if (pass.stencilAttachment != nullptr)
    {
        stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        stencilAttachment.imageView = pass.stencilAttachment->image->view;
        stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        stencilAttachment.loadOp = pass.stencilAttachment->loadOp;
        stencilAttachment.storeOp = pass.stencilAttachment->storeOp;
        stencilAttachment.clearValue = pass.stencilAttachment->clearValue;
    }

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = {width, height};
    renderingInfo.layerCount = pass.layerCount;
    renderingInfo.colorAttachmentCount = pass.colorAttachmentCount;
    renderingInfo.pColorAttachments = colorAttachments;
    renderingInfo.pDepthAttachment = pass.depthAttachment != nullptr
        ? &depthAttachment
        : nullptr;
    renderingInfo.pStencilAttachment = pass.stencilAttachment != nullptr
        ? &stencilAttachment
        : nullptr;

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

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    vkQueueWaitIdle(hgVkQueue);

    if (window->cmds != nullptr)
        vkFreeCommandBuffers(hgVkDevice, hgVkCmdPool, window->imageCount, window->cmds);

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroyFence(hgVkDevice, window->frameFinished[i], nullptr);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->imageAvailable[i], nullptr);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->readyToPresent[i], nullptr);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroyImageView(hgVkDevice, window->views[i].view, nullptr);
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

    u32 swapImageCount;
    vkGetSwapchainImagesKHR(hgVkDevice, window->swapchain, &swapImageCount, nullptr);
    VkImage* swapImages = hgAlloc<VkImage>(scratch, swapImageCount);
    vkGetSwapchainImagesKHR(hgVkDevice, window->swapchain, &swapImageCount, swapImages);

    if (window->imageCount != swapImageCount)
    {
        window->images = (HgImage*)realloc(window->images, sizeof(HgImage) * swapImageCount);
        window->views = (HgImageView*)realloc(window->views, sizeof(HgImageView) * swapImageCount);

        window->cmds = (VkCommandBuffer*)realloc(window->cmds, sizeof(VkCommandBuffer) * swapImageCount);
        window->frameFinished = (VkFence*)realloc(window->frameFinished, sizeof(VkFence) * swapImageCount);
        window->imageAvailable = (VkSemaphore*)realloc(window->imageAvailable, sizeof(VkSemaphore) * swapImageCount);
        window->readyToPresent = (VkSemaphore*)realloc(window->readyToPresent, sizeof(VkSemaphore) * swapImageCount);

        window->imageCount = swapImageCount;
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        window->images[i].image = swapImages[i];
        window->images[i].type = VK_IMAGE_TYPE_2D;
        window->images[i].format = window->format;
        window->images[i].width = window->width;
        window->images[i].height = window->height;
        window->images[i].depth = 1;
        window->images[i].mipLevels = 1;
        window->images[i].arrayLayers = 1;
        window->images[i].msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = window->format;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        VkResult viewResult = vkCreateImageView(hgVkDevice, &viewInfo, nullptr, &window->views[i].view);
        if (window->views[i].view == nullptr)
            hgError("Could not create VkImageView: %s\n", hgVkResultToStr(viewResult));

        window->views[i].image = &window->images[i];
        window->views[i].type = VK_IMAGE_VIEW_TYPE_2D;
        window->views[i].aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        window->views[i].baseMipLevel = 0;
        window->views[i].levelCount = 1;
        window->views[i].baseArrayLayer = 0;
        window->views[i].layerCount = 1;
    }

    if (window->imageCount > 0) {
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.pNext = nullptr;
        cmdAllocInfo.commandPool = hgVkCmdPool;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount = window->imageCount;

        vkAllocateCommandBuffers(hgVkDevice, &cmdAllocInfo, window->cmds);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(hgVkDevice, &info, nullptr, &window->frameFinished[i]);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(hgVkDevice, &info, nullptr, &window->imageAvailable[i]);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
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

    for (u32 i = 0; i < formatCount; ++i)
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

    for (u32 i = 0; i < modeCount; ++i)
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

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroyFence(hgVkDevice, window->frameFinished[i], nullptr);
    }
    free(window->frameFinished);

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->imageAvailable[i], nullptr);
    }
    free(window->imageAvailable);

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->readyToPresent[i], nullptr);
    }
    free(window->readyToPresent);

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroyImageView(hgVkDevice, window->views[i].view, nullptr);
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
    if (result != VK_SUCCESS)
        hgError("Could not acquire next image: %s", hgVkResultToStr(result));

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

struct HgVulkanFuncs
{
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

void hgLoadVulkanInstanceFuncs(VkInstance instance)
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

void hgLoadVulkanDeviceFuncs(VkDevice device)
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

static void hgVulkanInit()
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

static void hgVulkanDeinit()
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

static void hgVulkanInit()
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

static void hgVulkanDeinit()
{
    if (libvulkan != nullptr)
    {
        FreeLibrary((HMODULE)libvulkan);
        libvulkan = nullptr;
    }
}

#endif

#undef HG_LOAD_VULKAN_FUNC

