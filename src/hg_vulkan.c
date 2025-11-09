#include "hg_internal.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

static const char* const DeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static VkBufferUsageFlags hg_buffer_usage_flags_to_vk(HgBufferUsageFlags usage);
static VkImageType hg_dimensions_to_image_type(u32 dimensions);
static VkImageViewType hg_dimensions_to_view_type(u32 dimensions);
static VkFormat hg_format_to_vk(HgFormat format);
static HgFormat hg_format_from_vk(VkFormat format);
static u32 hg_format_size(HgFormat format);
static VkImageAspectFlags hg_format_to_aspect(HgFormat format);
static VkImageUsageFlags hg_texture_usage_flags_to_vk(HgTextureUsageFlags usage);
static VkImageLayout hg_texture_layout_to_vk(HgTextureLayout layout);
static VkSamplerAddressMode hg_sampler_edge_mode_to_vk(HgSamplerEdgeMode edge_mode);
static VkDescriptorType hg_descriptor_type_to_vk(HgDescriptorType type);
static VkVertexInputRate hg_vertex_input_rate_to_vk(HgVertexInputRate rate);
static VkPrimitiveTopology hg_primitive_topology_to_vk(HgPrimitiveTopology topology);
static VkCullModeFlags hg_cull_mode_to_vk(HgCullModeFlagBits cull_mode);

static VkCommandBuffer hg_current_cmd(HgWindow* window);
static VkDescriptorPool hg_current_descriptor_pool(HgWindow* window);
static VkImage hg_current_image(HgWindow* window);
static VkFence hg_is_frame_finished(HgWindow* window);
static VkSemaphore hg_is_image_available(HgWindow* window);

static u32 hg_get_instance_extensions(
    const char** extension_buffer,
    u32 extension_buffer_capacity
);
static u32 hg_get_instance_layers(
    const char** layer_buffer,
    u32 layer_buffer_capacity
);
static VkInstance hg_create_instance(void);

#ifndef NDEBUG
static VkBool32 debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
);
static const VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info;
#endif
static VkDebugUtilsMessengerEXT hg_create_debug_messenger(HurdyGurdy* hg);

static bool find_queue_family(VkPhysicalDevice gpu, u32* queue_family);
static u32 get_gpus(
    HurdyGurdy* hg,
    VkPhysicalDevice* gpu_buffer,
    u32 gpu_buffer_capacity
);
static VkPhysicalDevice hg_find_gpu(HurdyGurdy* hg);

static VkDevice hg_create_device(HurdyGurdy* hg);
static VmaAllocator hg_create_gpu_allocator(HurdyGurdy* hg);

static VkCommandPool hg_create_command_pool(HurdyGurdy* hg);
static void hg_allocate_command_buffers(
    const HurdyGurdy* hg,
    VkCommandBuffer* command_buffers,
    u32 command_buffer_count
);

static VkDescriptorPool hg_create_descriptor_pool(const HurdyGurdy* hg);
static VkDescriptorSet hg_allocate_descriptor_set(
    HgCommands* commands,
    u32 set_index
);

static VkSemaphore hg_create_semaphore(const HurdyGurdy* hg);
static VkFence hg_create_fence(const HurdyGurdy* hg, VkFenceCreateFlags flags);
static void hg_wait_for_fence(const HurdyGurdy* hg, VkFence fence);
static void hg_reset_fence(const HurdyGurdy* hg, VkFence fence);

static VkDescriptorSetLayout hg_create_descriptor_set_layout(
    const HurdyGurdy* hg,
    const VkDescriptorSetLayoutBinding* bindings,
    u32 binding_count
);
static void hg_create_descriptor_set_layouts(
    const HurdyGurdy* hg,
    VkDescriptorSetLayout* layouts,
    HgDescriptorSet* descriptor_sets,
    u32 descriptor_set_count,
    VkPipelineStageFlags stage_flags
);
static VkPipelineLayout hg_create_pipeline_layout(
    const HurdyGurdy* hg,
    const VkDescriptorSetLayout* layouts,
    u32 layout_count,
    const VkPushConstantRange* push_constants,
    u32 push_constant_count
);
static VkShaderModule hg_create_shader_module(
    const HurdyGurdy* hg,
    const byte* code,
    usize size
);

static VkCommandBuffer hg_begin_single_time_cmd(const HurdyGurdy* hg);
static void hg_end_single_time_cmd(const HurdyGurdy* hg, VkCommandBuffer cmd);

static void hg_copy_buffer_to_image(
    VkCommandBuffer cmd,
    HgTexture* dst,
    HgBuffer* src
);
static void hg_copy_image_to_buffer(
    VkCommandBuffer cmd,
    HgBuffer* dst,
    HgTexture* src
);

typedef struct HgBlitConfigInternal {
    VkImage image;
    VkImageAspectFlags aspect;
    VkOffset3D begin;
    VkOffset3D end;
    uint32_t mip_level;
    uint32_t array_layer;
    uint32_t layer_count;
} HgBlitConfigInternal;

static void hg_blit_image_internal(
    VkCommandBuffer cmd,
    const HgBlitConfigInternal* dst,
    const HgBlitConfigInternal* src,
    VkFilter filter
);

static void hg_write_cubemap(
    const HurdyGurdy* hg,
    HgTexture* dst,
    const void* src,
    VkImageLayout layout
);

static VkFormat hg_find_swapchain_format(
    const HurdyGurdy* hg,
    VkSurfaceKHR surface
);
static VkPresentModeKHR hg_find_swapchain_present_mode(
    const HurdyGurdy* hg,
    VkSurfaceKHR surface
);
static VkSwapchainKHR hg_create_new_swapchain(
    const HurdyGurdy* hg,
    VkSwapchainKHR old_swapchain,
    VkSurfaceKHR surface,
    VkFormat format,
    u32* width,
    u32* height
);
static void hg_get_swapchain_images(
    const HurdyGurdy* hg,
    VkSwapchainKHR swapchain,
    VkImage* images,
    u32* image_count
);

static void hg_load_vulkan(void);
static void hg_load_vulkan_instance(VkInstance instance);
static void hg_load_vulkan_device(VkDevice device);

HurdyGurdy* hg_init(void) {
    HurdyGurdy* hg = hg_heap_alloc(sizeof(HurdyGurdy));

    hg_load_vulkan();

    hg_init_platform_internals(hg);

    hg->instance = hg_create_instance();

    hg_load_vulkan_instance(hg->instance);

#ifndef NDEBUG
    hg->debug_messenger = hg_create_debug_messenger(hg);
#endif

    hg->gpu = hg_find_gpu(hg);
    hg->device = hg_create_device(hg);

    hg_load_vulkan_device(hg->device);

    vkGetDeviceQueue(hg->device, hg->queue_family_index, 0, &hg->queue);
    if (hg->queue == VK_NULL_HANDLE)
        HG_ERROR("Vulkan could not get queue");

    hg->allocator = hg_create_gpu_allocator(hg);
    hg->command_pool = hg_create_command_pool(hg);
    hg->descriptor_pool = hg_create_descriptor_pool(hg);

    return hg;
}

void hg_shutdown(HurdyGurdy* hg) {
    HG_ASSERT(hg->instance != VK_NULL_HANDLE);
    HG_ASSERT(hg->debug_messenger != VK_NULL_HANDLE);
    HG_ASSERT(hg->device != VK_NULL_HANDLE);
    HG_ASSERT(hg->queue != VK_NULL_HANDLE);
    HG_ASSERT(hg->allocator != VK_NULL_HANDLE);
    HG_ASSERT(hg->command_pool != VK_NULL_HANDLE);
    HG_ASSERT(hg->platform_internals != NULL);

    hg_shutdown_platform_internals(hg);

    vkDestroyDescriptorPool(hg->device, hg->descriptor_pool, NULL);
    vkDestroyCommandPool(hg->device, hg->command_pool, NULL);
    vmaDestroyAllocator(hg->allocator);
    vkDestroyDevice(hg->device, NULL);

#ifndef NDEBUG
    vkDestroyDebugUtilsMessengerEXT(hg->instance, hg->debug_messenger, NULL);
#endif

    vkDestroyInstance(hg->instance, NULL);

    hg_heap_free(hg, sizeof(HurdyGurdy));
}

void hg_graphics_wait(
    const HurdyGurdy* hg
) {
    HG_ASSERT(hg != NULL);

    VkResult wait_result = vkQueueWaitIdle(hg->queue);
    switch (wait_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }
}

HgBuffer* hg_buffer_create(
    const HurdyGurdy* hg,
    const HgBufferConfig* config
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(config != NULL);
    HG_ASSERT(config->size != 0);
    HG_ASSERT(config->usage != HG_BUFFER_USAGE_NONE);

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = config->size,
        .usage = hg_buffer_usage_flags_to_vk(config->usage),
    };

    VmaAllocationCreateInfo alloc_info = {0};
    if (config->memory_type == HG_GPU_MEMORY_TYPE_DEVICE_LOCAL) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_info.flags = 0;
    } else if (config->memory_type == HG_GPU_MEMORY_TYPE_LINEAR_ACCESS) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    } else if (config->memory_type == HG_GPU_MEMORY_TYPE_RANDOM_ACCESS) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    } else {
        HG_ERROR("Invalid buffer memory type");
    }

    HgBuffer* buffer = hg_heap_alloc(sizeof(HgBuffer));
    *buffer = (HgBuffer){
        .size = config->size,
        .memory_type = config->memory_type,
    };
    VkResult buffer_result = vmaCreateBuffer(
        hg->allocator,
        &buffer_info,
        &alloc_info,
        &buffer->handle,
        &buffer->allocation,
        NULL
    );
    switch (buffer_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: HG_ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: HG_ERROR("Vulkan invalid opaque capture address");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return buffer;
}

void hg_buffer_destroy(
    const HurdyGurdy* hg,
    HgBuffer* buffer
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(buffer != NULL);

    vmaDestroyBuffer(hg->allocator, buffer->handle, buffer->allocation);
    hg_heap_free(buffer, sizeof(HgBuffer));
}

void hg_buffer_write(
    const HurdyGurdy* hg,
    HgBuffer* dst,
    usize offset,
    const void* src,
    usize size
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(src != NULL);
    if (dst->memory_type == HG_GPU_MEMORY_TYPE_LINEAR_ACCESS)
        HG_ASSERT(offset == 0);

    if (size == 0)
        return;
    if (size == SIZE_MAX)
        size = dst->size - offset;

    if (dst->memory_type == HG_GPU_MEMORY_TYPE_RANDOM_ACCESS ||
        dst->memory_type == HG_GPU_MEMORY_TYPE_LINEAR_ACCESS) {
        VkResult copy_result = vmaCopyMemoryToAllocation(
            hg->allocator,
            src,
            dst->allocation,
            offset,
            size
        );
        switch (copy_result) {
            case VK_SUCCESS: return;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_MEMORY_MAP_FAILED: HG_ERROR("Vulkan memory map failed");
            default: HG_ERROR("Unexpected Vulkan error");
        }
    }
    HG_ASSERT(dst->memory_type == HG_GPU_MEMORY_TYPE_DEVICE_LOCAL);

    HgBuffer* staging_buffer = hg_buffer_create(hg, &(HgBufferConfig){
        .size = size,
        .usage = HG_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_type = HG_GPU_MEMORY_TYPE_LINEAR_ACCESS
    });
    hg_buffer_write(hg, staging_buffer, 0, src, size);

    VkCommandBuffer cmd = hg_begin_single_time_cmd(hg);
    vkCmdCopyBuffer(
        cmd,
        staging_buffer->handle,
        dst->handle,
        1,
        &(VkBufferCopy){0, offset, size}
    );
    hg_end_single_time_cmd(hg, cmd);

    hg_buffer_destroy(hg, staging_buffer);
}

void hg_buffer_read(
    const HurdyGurdy* hg,
    const HgBuffer* src,
    usize offset,
    void* dst,
    usize size
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(src != NULL);
    HG_ASSERT(dst != NULL);
    if (src->memory_type == HG_GPU_MEMORY_TYPE_LINEAR_ACCESS)
        HG_ASSERT(offset == 0);

    if (size == 0)
        return;
    if (size == SIZE_MAX)
        size = src->size - offset;

    if (src->memory_type == HG_GPU_MEMORY_TYPE_RANDOM_ACCESS ||
        src->memory_type == HG_GPU_MEMORY_TYPE_LINEAR_ACCESS) {
        VkResult copy_result = vmaCopyAllocationToMemory(
            hg->allocator,
            src->allocation,
            offset,
            dst,
            size
        );
        switch (copy_result) {
            case VK_SUCCESS: return;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_MEMORY_MAP_FAILED: HG_ERROR("Vulkan memory map failed");
            default: HG_ERROR("Unexpected Vulkan error");
        }
    }
    HG_ASSERT(src->memory_type == HG_GPU_MEMORY_TYPE_DEVICE_LOCAL);

    HgBuffer* staging_buffer = hg_buffer_create(hg, &(HgBufferConfig){
        .size = size,
        .usage = HG_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_type = HG_GPU_MEMORY_TYPE_LINEAR_ACCESS
    });

    VkCommandBuffer cmd = hg_begin_single_time_cmd(hg);
    vkCmdCopyBuffer(
        cmd,
        src->handle,
        staging_buffer->handle,
        1,
        &(VkBufferCopy){offset, 0, size}
    );
    hg_end_single_time_cmd(hg, cmd);

    hg_buffer_read(hg, staging_buffer, 0, dst, SIZE_MAX);
    hg_buffer_destroy(hg, staging_buffer);
}

HgTexture* hg_texture_create(
    const HurdyGurdy* hg,
    const HgTextureConfig* config
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(config != NULL);
    HG_ASSERT(config->width > 0);
    HG_ASSERT(config->height > 0);
    HG_ASSERT(config->depth > 0);
    HG_ASSERT(config->format != HG_FORMAT_UNDEFINED);
    HG_ASSERT(config->usage != HG_TEXTURE_USAGE_NONE);
    if (config->make_cubemap) {
        HG_ASSERT(config->width == config->height);
        HG_ASSERT(config->depth == 1);
        HG_ASSERT(config->dimensions != 1 && config->dimensions != 3);
        HG_ASSERT(config->mip_levels <= 1);
    }

    HgTexture* texture = hg_heap_alloc(sizeof(HgTexture));
    *texture = (HgTexture){
        .width = config->width,
        .height = config->height,
        .depth = config->depth,
        .array_layers = config->make_cubemap ? 6 : 1,
        .mip_levels = HG_MAX(1, config->mip_levels),
        .format = hg_format_to_vk(config->format),
        .aspect = hg_format_to_aspect(config->format),
        .layout = VK_IMAGE_LAYOUT_UNDEFINED,
        .view = VK_NULL_HANDLE,
    };

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = config->make_cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
        .imageType = hg_dimensions_to_image_type(config->dimensions),
        .format = texture->format,
        .extent = (VkExtent3D){
            .width = texture->width,
            .height = texture->height,
            .depth = texture->depth,
        },
        .mipLevels = texture->mip_levels,
        .arrayLayers = texture->array_layers,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .usage = hg_texture_usage_flags_to_vk(config->usage),
    };
    VmaAllocationCreateInfo alloc_info = {
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    VkResult image_result = vmaCreateImage(
        hg->allocator,
        &image_info,
        &alloc_info,
        &texture->handle,
        &texture->allocation,
        NULL
    );
    switch (image_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: HG_ERROR("Vulkan compression exhausted");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: HG_ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: HG_ERROR("Vulkan invalid opaque capture address");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    if (config->usage & (
            HG_TEXTURE_USAGE_SAMPLED_BIT |
            HG_TEXTURE_USAGE_STORAGE_BIT |
            HG_TEXTURE_USAGE_RENDER_TARGET_BIT |
            HG_TEXTURE_USAGE_DEPTH_BUFFER_BIT
        )) {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = texture->handle,
            .viewType = config->make_cubemap
                ? VK_IMAGE_VIEW_TYPE_CUBE
                : hg_dimensions_to_view_type(config->dimensions),
            .format = texture->format,
            .subresourceRange = (VkImageSubresourceRange){
                .aspectMask = texture->aspect,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };

        VkResult result = vkCreateImageView(
            hg->device,
            &view_info,
            NULL,
            &texture->view
        );
        switch (result) {
            case VK_SUCCESS: break;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: HG_ERROR("Vulkan invalid opaque capture address");
            default: HG_ERROR("Unexpected Vulkan error");
        }
    }

    if (config->usage & HG_TEXTURE_USAGE_SAMPLED_BIT) {
        VkPhysicalDeviceProperties gpu_properties = {0};
        vkGetPhysicalDeviceProperties(hg->gpu, &gpu_properties);

        VkSamplerCreateInfo sampler_info = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = config->bilinear_filter
                ? VK_FILTER_LINEAR
                : VK_FILTER_NEAREST,
            .minFilter = config->bilinear_filter
                ? VK_FILTER_LINEAR
                : VK_FILTER_NEAREST,
            .mipmapMode = config->bilinear_filter
                ? VK_SAMPLER_MIPMAP_MODE_LINEAR
                : VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = hg_sampler_edge_mode_to_vk(config->edge_mode),
            .addressModeV = hg_sampler_edge_mode_to_vk(config->edge_mode),
            .addressModeW = hg_sampler_edge_mode_to_vk(config->edge_mode),
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = gpu_properties.limits.maxSamplerAnisotropy,
            .maxLod = (f32)config->mip_levels,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        };

        VkResult sampler_result = vkCreateSampler(
            hg->device,
            &sampler_info,
            NULL,
            &texture->sampler
        );
        switch (sampler_result) {
            case VK_SUCCESS: break;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: HG_ERROR("Vulkan invalid device address");
            default: HG_ERROR("Unexpected Vulkan error");
        }
    }

    return texture;
}

void hg_texture_destroy(
    const HurdyGurdy* hg,
    HgTexture* texture
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(texture != NULL);

    if (texture->sampler != VK_NULL_HANDLE)
        vkDestroySampler(hg->device, texture->sampler, NULL);
    if (texture->view != VK_NULL_HANDLE)
        vkDestroyImageView(hg->device, texture->view, NULL);
    vmaDestroyImage(hg->allocator, texture->handle, texture->allocation);

    hg_heap_free(texture, sizeof(HgTexture));
}

void hg_texture_write(
    const HurdyGurdy* hg,
    HgTexture* dst,
    const void* src,
    HgTextureLayout layout
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(layout != HG_TEXTURE_LAYOUT_UNDEFINED);

    if (dst->is_cubemap) {
        hg_write_cubemap(hg, dst, src, hg_texture_layout_to_vk(layout));
        return;
    }

    if (src == NULL) {
        VkCommandBuffer cmd = hg_begin_single_time_cmd(hg);

        VkImageMemoryBarrier2 barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .newLayout = hg_texture_layout_to_vk(layout),
            .image = dst->handle,
            .subresourceRange = {dst->aspect, 0, VK_REMAINING_MIP_LEVELS, 0, 1},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier,
        });
        dst->layout = hg_texture_layout_to_vk(layout);

        hg_end_single_time_cmd(hg, cmd);
        return;
    }

    u32 pixel_size = hg_format_size(hg_format_from_vk(dst->format));
    HG_ASSERT(pixel_size > 0);

    HgBuffer* staging_buffer = hg_buffer_create(hg, &(HgBufferConfig){
        .size = dst->width * dst->height * dst->depth * pixel_size,
        .usage = HG_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_type = HG_GPU_MEMORY_TYPE_LINEAR_ACCESS,
    });
    hg_buffer_write(hg, staging_buffer, 0, src, SIZE_MAX);

    VkCommandBuffer cmd = hg_begin_single_time_cmd(hg);

    VkImageMemoryBarrier2 pre_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = dst->handle,
        .subresourceRange = {dst->aspect, 0, VK_REMAINING_MIP_LEVELS, 0, 1},
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &pre_barrier,
    });

    hg_copy_buffer_to_image(cmd, dst, staging_buffer);

    VkImageMemoryBarrier2 post_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = hg_texture_layout_to_vk(layout),
        .image = dst->handle,
        .subresourceRange = {dst->aspect, 0, VK_REMAINING_MIP_LEVELS, 0, 1},
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &post_barrier,
    });
    dst->layout = hg_texture_layout_to_vk(layout);

    hg_end_single_time_cmd(hg, cmd);

    hg_buffer_destroy(hg, staging_buffer);
}

void hg_texture_read(
    const HurdyGurdy* hg,
    HgTexture* src,
    void* dst,
    HgTextureLayout layout
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(src != NULL);
    HG_ASSERT(dst != NULL);
    HG_ASSERT(layout != HG_TEXTURE_LAYOUT_UNDEFINED);

    u32 pixel_size = hg_format_size(hg_format_from_vk(src->format));
    HG_ASSERT(pixel_size > 0);

    HgBuffer* staging_buffer = hg_buffer_create(hg, &(HgBufferConfig){
        .size = src->width * src->height * src->depth * pixel_size,
        .usage = HG_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_type = HG_GPU_MEMORY_TYPE_LINEAR_ACCESS,
    });

    VkCommandBuffer cmd = hg_begin_single_time_cmd(hg);

    VkImageMemoryBarrier2 pre_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image = src->handle,
        .subresourceRange = {src->aspect, 0, VK_REMAINING_MIP_LEVELS, 0, 1},
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &pre_barrier,
    });

    hg_copy_image_to_buffer(cmd, staging_buffer, src);

    VkImageMemoryBarrier2 post_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .newLayout = hg_texture_layout_to_vk(layout),
        .image = src->handle,
        .subresourceRange = {src->aspect, 0, VK_REMAINING_MIP_LEVELS, 0, 1},
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &post_barrier,
    });
    src->layout = hg_texture_layout_to_vk(layout);

    hg_end_single_time_cmd(hg, cmd);

    hg_buffer_read(hg, staging_buffer, 0, dst, SIZE_MAX);
    hg_buffer_destroy(hg, staging_buffer);
}

u32 hg_get_max_mip_count(u32 width, u32 height, u32 depth) {
    if (width == 0 && height == 0 && depth == 0)
        return 0;
    return (u32)(log2f(HG_MAX(HG_MAX((f32)width, (f32)height), (f32)depth))) + 1;
}

void hg_texture_generate_mipmaps(
    const HurdyGurdy* hg,
    HgTexture* texture,
    HgTextureLayout layout
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(texture != NULL);
    HG_ASSERT(texture->mip_levels > 1);
    HG_ASSERT(layout != HG_TEXTURE_LAYOUT_UNDEFINED);

    VkFormatProperties format_properties = {0};
    vkGetPhysicalDeviceFormatProperties(
        hg->gpu,
        texture->format,
        &format_properties
    );
    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        HG_ERROR("Format does not support optimal tiling with linear filtering");

    VkCommandBuffer cmd = hg_begin_single_time_cmd(hg);

    VkOffset3D mip_offset = {
        (i32)texture->width,
        (i32)texture->height,
        (i32)texture->depth
    };

    VkImageMemoryBarrier2 initial_read_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image = texture->handle,
        .subresourceRange = {
            texture->aspect,
            0,
            texture->mip_levels,
            0,
            1
        },
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &initial_read_barrier,
    });

    for (u32 level = 0; level < texture->mip_levels - 1; ++level) {
        VkImageMemoryBarrier2 write_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = texture->handle,
            .subresourceRange = {texture->aspect, level + 1, 1, 0, 1},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &write_barrier,
        });

        HgBlitConfigInternal src_view = {
            .image = texture->handle,
            .aspect = texture->aspect,
            .end = mip_offset,
            .mip_level = level,
            .array_layer = 0,
            .layer_count = 1,
        };
        if (mip_offset.x > 1)
            mip_offset.x /= 2;
        if (mip_offset.y > 1)
            mip_offset.y /= 2;
        if (mip_offset.z > 1)
            mip_offset.z /= 2;
        HgBlitConfigInternal dst_view = {
            .image = texture->handle,
            .aspect = texture->aspect,
            .end = mip_offset,
            .mip_level = level + 1,
            .array_layer = 0,
            .layer_count = 1,
        };
        hg_blit_image_internal(cmd, &dst_view, &src_view, VK_FILTER_LINEAR);

        VkImageMemoryBarrier2 read_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .image = texture->handle,
            .subresourceRange = {texture->aspect, level + 1, 1, 0, 1},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &read_barrier,
        });
    }

    VkImageMemoryBarrier2 final_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .newLayout = hg_texture_layout_to_vk(layout),
        .image = texture->handle,
        .subresourceRange = {texture->aspect, 0, texture->mip_levels, 0, 1},
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &final_barrier,
    });
    texture->layout = hg_texture_layout_to_vk(layout);

    hg_end_single_time_cmd(hg, cmd);
}

HgShader* hg_shader_create(
    const HurdyGurdy* hg,
    const HgShaderConfig* config
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(config != NULL);
    HG_ASSERT(config->color_format != HG_FORMAT_UNDEFINED);
    HG_ASSERT(config->vertex_shader != NULL);
    HG_ASSERT(config->vertex_shader_size > 0);
    HG_ASSERT(config->fragment_shader != NULL);
    HG_ASSERT(config->fragment_shader_size > 0);
    if (config->vertex_binding_count > 0)
        HG_ASSERT(config->vertex_bindings != NULL);
    if (config->descriptor_set_count > 0)
        HG_ASSERT(config->descriptor_sets != NULL);

    HgShader* shader = hg_heap_alloc(
        sizeof(HgShader) + config->descriptor_set_count * sizeof(VkDescriptorSetLayout)
    );
    *shader = (HgShader){
        .descriptor_layout_count = config->descriptor_set_count,
        .bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS,
    };

    if (config->descriptor_set_count > 0) {
        hg_create_descriptor_set_layouts(
            hg,
            shader->descriptor_layouts,
            config->descriptor_sets,
            config->descriptor_set_count,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
        );
    }

    VkPushConstantRange push_constant = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = config->push_constant_size,
    };

    shader->layout = hg_create_pipeline_layout(
        hg,
        shader->descriptor_layouts,
        shader->descriptor_layout_count,
        config->push_constant_size > 0 ? &push_constant : NULL,
        config->push_constant_size > 0 ? 1 : 0
    );

    VkShaderModule vertex_shader = hg_create_shader_module(
        hg, config->vertex_shader,
        config->vertex_shader_size
    );
    VkShaderModule fragment_shader = hg_create_shader_module(
        hg, config->fragment_shader,
        config->fragment_shader_size
    );

    VkPipelineShaderStageCreateInfo shader_stages[] = {{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertex_shader,
        .pName = "main",
    }, {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragment_shader,
        .pName = "main",
    }};

#define HG_MAX_VERTEX_BINDINGS 8
#define HG_MAX_VERTEX_ATTRIBUTES 32

    u32 vertex_binding_count = 0;
    VkVertexInputBindingDescription vertex_binding_descriptions[HG_MAX_VERTEX_BINDINGS];
    u32 vertex_attribute_count = 0;
    VkVertexInputAttributeDescription vertex_attribute_descriptions[HG_MAX_VERTEX_ATTRIBUTES];

    for (u32 i = 0; i < config->vertex_binding_count; ++i) {
        HG_ASSERT(config->vertex_bindings[i].attribute_count < HG_MAX_VERTEX_ATTRIBUTES);
        HgVertexBinding* binding = &config->vertex_bindings[i];

        vertex_binding_descriptions[vertex_binding_count] = (VkVertexInputBindingDescription){
            .binding = i,
            .stride = binding->stride,
            .inputRate = hg_vertex_input_rate_to_vk(binding->input_rate),
        };
        for (u32 j = 0; j < binding->attribute_count; ++j) {
            HG_ASSERT(vertex_attribute_count < HG_MAX_VERTEX_ATTRIBUTES);
            HgVertexAttribute* attribute = &binding->attributes[j];

            vertex_attribute_descriptions[vertex_attribute_count] = (VkVertexInputAttributeDescription){
                .location = j,
                .binding = i,
                .format = hg_format_to_vk(attribute->format),
                .offset = attribute->offset,
            };
            ++vertex_attribute_count;
        }
        ++vertex_binding_count;
    }
#undef HG_MAX_VERTEX_BINDINGS
#undef HG_MAX_VERTEX_ATTRIBUTES

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = vertex_binding_count,
        .pVertexBindingDescriptions = vertex_binding_descriptions,
        .vertexAttributeDescriptionCount = vertex_attribute_count,
        .pVertexAttributeDescriptions = vertex_attribute_descriptions,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = hg_primitive_topology_to_vk(config->topology),
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineTessellationStateCreateInfo tessellation_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = hg_cull_mode_to_vk(config->cull_mode),
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = config->depth_format != HG_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE,
        .depthWriteEnable = config->depth_format != HG_FORMAT_UNDEFINED
            ? VK_TRUE
            : VK_FALSE,
        .depthCompareOp = config->enable_color_blend
            ? VK_COMPARE_OP_LESS_OR_EQUAL
            : VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = config->enable_color_blend ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                        | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT
                        | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = {1.0f, 1.0f, 1.0f, 1.0f},
    };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates = dynamic_states,
        .dynamicStateCount = HG_ARRAY_SIZE(dynamic_states),
    };

    VkFormat color_format = hg_format_to_vk(config->color_format);
    VkPipelineRenderingCreateInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = color_format != VK_FORMAT_UNDEFINED ? 1 : 0,
        .pColorAttachmentFormats = &color_format,
        .depthAttachmentFormat = hg_format_to_vk(config->depth_format),
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering_info,
        .stageCount = HG_ARRAY_SIZE(shader_stages),
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState = &tessellation_state,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = shader->layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VkResult result = vkCreateGraphicsPipelines(
        hg->device,
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        NULL,
        &shader->pipeline
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_PIPELINE_COMPILE_REQUIRED_EXT: {
            HG_WARN("Pipeline requires recompilation");
        } break;
        case VK_ERROR_INVALID_SHADER_NV: HG_ERROR("Vulkan invalid shader");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    vkDestroyShaderModule(hg->device, vertex_shader, NULL);
    vkDestroyShaderModule(hg->device, fragment_shader, NULL);
    return shader;
}

HgShader* hg_compute_shader_create(
    const HurdyGurdy* hg,
    const HgComputeShaderConfig* config
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(config != NULL);
    HG_ASSERT(config->shader != NULL);
    HG_ASSERT(config->shader_size > 0);
    if (config->descriptor_set_count > 0)
        HG_ASSERT(config->descriptor_sets != NULL);

    HgShader* shader = hg_heap_alloc(
        sizeof(HgShader) +
        config->descriptor_set_count * sizeof(VkDescriptorSetLayout)
    );
    *shader = (HgShader){
        .descriptor_layout_count = config->descriptor_set_count,
        .bind_point = VK_PIPELINE_BIND_POINT_COMPUTE,
    };

    hg_create_descriptor_set_layouts(
        hg,
        shader->descriptor_layouts,
        config->descriptor_sets,
        config->descriptor_set_count,
        VK_SHADER_STAGE_COMPUTE_BIT
    );

    VkPushConstantRange push_constant = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = config->push_constant_size,
    };

    shader->layout = hg_create_pipeline_layout(
        hg,
        shader->descriptor_layouts,
        shader->descriptor_layout_count,
        config->push_constant_size > 0 ? &push_constant : NULL,
        config->push_constant_size > 0 ? 1 : 0
    );

    VkShaderModule shader_module = hg_create_shader_module(
        hg,
        config->shader,
        config->shader_size
    );

    VkPipelineShaderStageCreateInfo shader_stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader_module,
        .pName = "main",
    };

    VkComputePipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shader_stage,
        .layout = shader->layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VkResult result = vkCreateComputePipelines(
        hg->device,
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        NULL,
        &shader->pipeline
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_PIPELINE_COMPILE_REQUIRED_EXT: {
            HG_WARN("Pipeline requires recompilation");
        } break;
        case VK_ERROR_INVALID_SHADER_NV: HG_ERROR("Vulkan invalid shader");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    vkDestroyShaderModule(hg->device, shader_module, NULL);
    return shader;
}

void hg_shader_destroy(
    const HurdyGurdy* hg,
    HgShader* shader
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(shader != NULL);
    HG_ASSERT(shader->layout != VK_NULL_HANDLE);
    HG_ASSERT(shader->pipeline != VK_NULL_HANDLE);
    for (usize i = 0; i < shader->descriptor_layout_count; ++i) {
        HG_ASSERT(shader->descriptor_layouts[i] != VK_NULL_HANDLE);
    }

    vkDestroyPipeline(hg->device, shader->pipeline, NULL);
    vkDestroyPipelineLayout(hg->device, shader->layout, NULL);
    for (usize i = 0; i < shader->descriptor_layout_count; ++i) {
        vkDestroyDescriptorSetLayout(
            hg->device,
            shader->descriptor_layouts[i],
            NULL
        );
    }
    hg_heap_free(
        shader,
        sizeof(HgShader) +
        shader->descriptor_layout_count * sizeof(VkDescriptorSetLayout)
    );
}

HgCommands* hg_commands_begin(
    const HurdyGurdy* hg
) {
    HG_ASSERT(hg != NULL);

    VkResult pool_result = vkResetDescriptorPool(
        hg->device, hg->descriptor_pool, 0
    );
    switch (pool_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    HgCommands* commands = hg_heap_alloc(sizeof(HgCommands));
    *commands = (HgCommands){
        .cmd = hg_begin_single_time_cmd(hg),
        .device = hg->device,
        .descriptor_pool = hg->descriptor_pool,
    };
    return commands;
}

void hg_commands_end(
    const HurdyGurdy* hg,
    HgCommands* commands
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(commands != NULL);

    hg_end_single_time_cmd(hg, commands->cmd);

    hg_heap_free(commands, sizeof(HgCommands));
}

void hg_renderpass_begin(
    HgCommands* commands,
    HgTexture* target,
    HgTexture* depth_buffer,
    bool clear_target,
    bool clear_depth
) {
    HG_ASSERT(commands != NULL);
    HG_ASSERT(target != NULL);

    VkImageMemoryBarrier2 barriers[4];
    u32 barrier_count = 0;

    if (target->layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barriers[barrier_count] = (VkImageMemoryBarrier2){
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = target->layout,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .image = target->handle,
            .subresourceRange = {target->aspect, 0, 1, 0, 1},
        };
        target->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ++barrier_count;
    }

    if (depth_buffer != NULL) {
        if (depth_buffer->layout !=
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barriers[barrier_count] = (VkImageMemoryBarrier2){
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .oldLayout = depth_buffer->layout,
                .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .image = depth_buffer->handle,
                .subresourceRange = {depth_buffer->aspect, 0, 1, 0, 1},
            };
            depth_buffer->layout
                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            ++barrier_count;
        }
    }

    if (commands->previous_target != NULL &&
        commands->previous_target != target) {
        if (commands->previous_target->layout !=
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barriers[barrier_count] = (VkImageMemoryBarrier2){
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = commands->previous_target->layout,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .image = commands->previous_target->handle,
                .subresourceRange = {target->aspect, 0, 1, 0, 1},
            };
            commands->previous_target->layout
                = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ++barrier_count;
        }
    }

    if (commands->previous_depth_buffer != NULL &&
        commands->previous_depth_buffer != depth_buffer) {
        if (commands->previous_depth_buffer->layout !=
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barriers[barrier_count] = (VkImageMemoryBarrier2){
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = commands->previous_depth_buffer->layout,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .image = commands->previous_depth_buffer->handle,
                .subresourceRange = {depth_buffer->aspect, 0, 1, 0, 1},
            };
            commands->previous_depth_buffer->layout
                = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ++barrier_count;
        }
    }

    if (barrier_count > 0) {
        vkCmdPipelineBarrier2(commands->cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = barrier_count,
            .pImageMemoryBarriers = barriers,
        });
    }

    VkRenderingInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, {target->width, target->height}},
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &(VkRenderingAttachmentInfo){
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = target->view,
            .imageLayout = target->layout,
            .loadOp = clear_target
                ? VK_ATTACHMENT_LOAD_OP_CLEAR
                : VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = (VkClearValue){
                .color = {{0.0f, 0.0f, 0.0f, 0.0f}}
            },
        },
        .pDepthAttachment = depth_buffer != NULL
            ? &(VkRenderingAttachmentInfo){
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = depth_buffer->view,
                .imageLayout = depth_buffer->layout,
                .loadOp = clear_depth
                    ? VK_ATTACHMENT_LOAD_OP_CLEAR
                    : VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = (VkClearValue){
                    .depthStencil = {1.0f, 0},
                },
            }
            : NULL,
    };

    vkCmdBeginRendering(commands->cmd, &render_info);

    vkCmdSetViewport(commands->cmd, 0, 1, &(VkViewport){
        0.0f, 0.0f,
        (f32)target->width, (f32)target->height,
        0.0f, 1.0f,
    });
    vkCmdSetScissor(commands->cmd, 0, 1, &(VkRect2D){
        {0, 0},
        {target->width, target->height}
    });

    commands->previous_target = target;
    commands->previous_depth_buffer = depth_buffer;
}

void hg_renderpass_end(
    HgCommands* commands
) {
    HG_ASSERT(commands != NULL);
    vkCmdEndRendering(commands->cmd);
}

void hg_shader_bind(
    HgCommands* commands,
    HgShader* shader
) {
    HG_ASSERT(commands != NULL);
    HG_ASSERT(shader != NULL);

    vkCmdBindPipeline(commands->cmd, shader->bind_point, shader->pipeline);
    commands->shader = shader;
}

void hg_shader_unbind(
    HgCommands* commands
) {
    HG_ASSERT(commands != NULL);
    commands->shader = NULL;
}

void hg_descriptor_set_bind(
    HgCommands* commands,
    u32 set_index,
    HgDescriptor* descriptors,
    u32 descriptor_count
) {
    HG_ASSERT(commands != NULL);
    HG_ASSERT(commands->shader != NULL);
    HG_ASSERT(set_index < commands->shader->descriptor_layout_count);
    HG_ASSERT(descriptors != NULL);
    HG_ASSERT(descriptor_count > 0);

    VkDescriptorSet descriptor_set = hg_allocate_descriptor_set(
        commands,
        set_index
    );
    HG_ASSERT(descriptor_set != VK_NULL_HANDLE);

    // write descriptors more efficiently : TODO
    for (u32 i = 0; i < descriptor_count; ++i) {
        for (u32 j = 0; j < descriptors[i].count; ++j) {
            VkDescriptorBufferInfo buffer_info;
            VkDescriptorImageInfo image_info;

            VkWriteDescriptorSet descriptor_write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_set,
                .dstBinding = i,
                .dstArrayElement = j,
                .descriptorCount = 1,
                .descriptorType = hg_descriptor_type_to_vk(descriptors[i].type),
                .pBufferInfo = &buffer_info,
                .pImageInfo = &image_info,
            };

            switch (descriptors[i].type) {
                case HG_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case HG_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
                    HG_ASSERT(descriptors[i].buffers[j]->handle != VK_NULL_HANDLE);
                    buffer_info = (VkDescriptorBufferInfo){
                        .buffer = descriptors[i].buffers[j]->handle,
                        .offset = 0,
                        .range = descriptors[i].buffers[j]->size,
                    };
                } break;
                case HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE: {
                    HG_ASSERT(descriptors[i].textures[j]->sampler != VK_NULL_HANDLE);
                    HG_ASSERT(descriptors[i].textures[j]->view != VK_NULL_HANDLE);
                    image_info = (VkDescriptorImageInfo){
                        .sampler = descriptors[i].textures[j]->sampler,
                        .imageView = descriptors[i].textures[j]->view,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    };
                } break;
                case HG_DESCRIPTOR_TYPE_STORAGE_TEXTURE: {
                    HG_ASSERT(descriptors[i].textures[j]->view != VK_NULL_HANDLE);
                    image_info = (VkDescriptorImageInfo){
                        .imageView = descriptors[i].textures[j]->view,
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                    };
                } break;
                default: HG_ERROR("Unhandled descriptor type");
            }

            vkUpdateDescriptorSets(commands->device, 1, &descriptor_write, 0, NULL);
        }
    }

    vkCmdBindDescriptorSets(
        commands->cmd,
        commands->shader->bind_point,
        commands->shader->layout,
        set_index,
        1, &descriptor_set,
        0, NULL
    );
}

void hg_push_constant_bind(
    HgCommands* commands,
    void* data,
    u32 size
) {
    HG_ASSERT(commands != NULL);
    HG_ASSERT(data != NULL);
    HG_ASSERT(size > 0);

    vkCmdPushConstants(
        commands->cmd,
        commands->shader->layout,
        commands->shader->bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS
            ? VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
            : VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        size,
        data
    );
}

void hg_draw(
    HgCommands* commands,
    HgBuffer* vertex_buffer,
    HgBuffer* index_buffer,
    u32 vertex_count
) {
    HG_ASSERT(commands != NULL);

    if (vertex_buffer != NULL)
        vkCmdBindVertexBuffers(
            commands->cmd,
            0,
            1,
            &vertex_buffer->handle,
            &(VkDeviceSize){0}
        );
    if (index_buffer != NULL) {
        vkCmdBindIndexBuffer(
            commands->cmd,
            index_buffer->handle,
            0,
            VK_INDEX_TYPE_UINT32
        );
        vkCmdDrawIndexed(
            commands->cmd,
            (u32)(index_buffer->size / sizeof(u32)),
            1,
            0,
            0,
            0
        );
    } else {
        vkCmdDraw(commands->cmd, vertex_count, 1, 0, 0);
    }
}

void hg_compute_dispatch(
    HgCommands* commands,
    u32 group_count_x,
    u32 group_count_y,
    u32 group_count_z
) {
    HG_ASSERT(commands != NULL);
    HG_ASSERT(group_count_x > 0);
    HG_ASSERT(group_count_y > 0);
    HG_ASSERT(group_count_z > 0);

    vkCmdDispatch(commands->cmd, group_count_x, group_count_y, group_count_z);
}

// memory manipulation commands go here : TODO

HgWindow* hg_window_create(
    const HurdyGurdy* hg,
    const HgWindowConfig* config
) {
    HG_ASSERT(hg != NULL);
    HgWindow* window = hg_heap_alloc(sizeof(HgWindow));

    hg_window_create_platform_internals(hg, config, window);

    window->swapchain_format = hg_find_swapchain_format(hg, window->surface);
    window->swapchain = hg_create_new_swapchain(
        hg,
        NULL,
        window->surface,
        window->swapchain_format,
        &window->swapchain_width,
        &window->swapchain_height
    );
    if (window->swapchain == VK_NULL_HANDLE)
        HG_ERROR("Could not create new swapchain");

    hg_get_swapchain_images(
        hg,
        window->swapchain,
        window->swapchain_images,
        &window->swapchain_image_count
    );

    hg_allocate_command_buffers(
        hg,
        window->command_buffers,
        HG_ARRAY_SIZE(window->command_buffers)
    );
    for (usize i = 0; i < HG_ARRAY_SIZE(window->descriptor_pools); ++i) {
        window->descriptor_pools[i] = hg_create_descriptor_pool(hg);
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(window->frame_finished_fences); ++i) {
        window->frame_finished_fences[i] = hg_create_fence(hg, VK_FENCE_CREATE_SIGNALED_BIT);
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(window->image_available_semaphores); ++i) {
        window->image_available_semaphores[i] = hg_create_semaphore(hg);
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(window->ready_to_present_semaphores); ++i) {
        window->ready_to_present_semaphores[i] = hg_create_semaphore(hg);
    }
    window->current_image_index = 0;
    window->current_frame_index = 0;

    memset(window->keys_down, 0, HG_ARRAY_SIZE(window->keys_down));
    memset(window->keys_pressed, 0, HG_ARRAY_SIZE(window->keys_pressed));
    memset(window->keys_released, 0, HG_ARRAY_SIZE(window->keys_released));
    window->was_closed = false;
    window->was_resized = false;

    return window;
}

void hg_window_destroy(
    const HurdyGurdy* hg,
    HgWindow* window
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(window != NULL);
    HG_ASSERT(window->surface != VK_NULL_HANDLE);
    HG_ASSERT(window->swapchain != VK_NULL_HANDLE);

    for (usize i = 0; i < HG_ARRAY_SIZE(window->image_available_semaphores); ++i) {
        HG_ASSERT(window->image_available_semaphores[i] != VK_NULL_HANDLE);
        vkDestroySemaphore(
            hg->device,
            window->image_available_semaphores[i],
            NULL
        );
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(window->ready_to_present_semaphores); ++i) {
        HG_ASSERT(window->ready_to_present_semaphores[i] != VK_NULL_HANDLE);
        vkDestroySemaphore(
            hg->device,
            window->ready_to_present_semaphores[i],
            NULL
        );
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(window->frame_finished_fences); ++i) {
        HG_ASSERT(window->frame_finished_fences[i] != VK_NULL_HANDLE);
        vkDestroyFence(
            hg->device,
            window->frame_finished_fences[i],
            NULL
        );
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(window->descriptor_pools); ++i) {
        HG_ASSERT(window->descriptor_pools[i] != VK_NULL_HANDLE);
        vkDestroyDescriptorPool(
            hg->device,
            window->descriptor_pools[i],
            NULL
        );
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(window->command_buffers); ++i) {
        HG_ASSERT(window->command_buffers[i] != VK_NULL_HANDLE);
    }
    vkFreeCommandBuffers(
        hg->device,
        hg->command_pool,
        HG_ARRAY_SIZE(window->command_buffers),
        window->command_buffers
    );

    vkDestroySwapchainKHR(hg->device, window->swapchain, NULL);
    vkDestroySurfaceKHR(hg->instance, window->surface, NULL);

    hg_window_destroy_platform_internals(hg, window);

    hg_heap_free(window, sizeof(HgWindow));
}

void hg_window_update_swapchain(
    const HurdyGurdy* hg,
    HgWindow* window
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(window != NULL);

    VkSwapchainKHR new_swapchain = hg_create_new_swapchain(
        hg,
        window->swapchain,
        window->surface,
        window->swapchain_format,
        &window->swapchain_width,
        &window->swapchain_height
    );
    if (new_swapchain == VK_NULL_HANDLE) // maybe we can just warn and return : TODO
        HG_ERROR("Could not create new swapchain");

    hg_graphics_wait(hg);

    vkDestroySwapchainKHR(hg->device, window->swapchain, NULL);
    window->swapchain = new_swapchain;

    hg_get_swapchain_images(
        hg,
        window->swapchain,
        window->swapchain_images,
        &window->swapchain_image_count
    );

    vkFreeCommandBuffers(
        hg->device,
        hg->command_pool,
        HG_ARRAY_SIZE(window->command_buffers),
        window->command_buffers
    );
    hg_allocate_command_buffers(
        hg,
        window->command_buffers,
        HG_ARRAY_SIZE(window->command_buffers)
    );

    for (usize i = 0; i < HG_ARRAY_SIZE(window->descriptor_pools); ++i) {
        vkDestroyDescriptorPool(hg->device, window->descriptor_pools[i], NULL);
        window->descriptor_pools[i] = hg_create_descriptor_pool(hg);
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(window->image_available_semaphores); ++i) {
        vkDestroySemaphore(hg->device, window->image_available_semaphores[i], NULL);
        window->image_available_semaphores[i] = hg_create_semaphore(hg);
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(window->ready_to_present_semaphores); ++i) {
        vkDestroySemaphore(hg->device, window->ready_to_present_semaphores[i], NULL);
        window->ready_to_present_semaphores[i] = hg_create_semaphore(hg);
    }
    for (usize i = 0; i < HG_ARRAY_SIZE(window->frame_finished_fences); ++i) {
        vkDestroyFence(hg->device, window->frame_finished_fences[i], NULL);
        window->frame_finished_fences[i] = hg_create_fence(hg, VK_FENCE_CREATE_SIGNALED_BIT);
    }
}

HgCommands* hg_window_begin_frame(
    const HurdyGurdy* hg,
    HgWindow* window
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(window != NULL);

    window->current_frame_index
        = (window->current_frame_index + 1) % HG_SWAPCHAIN_MAX_FRAMES_IN_FLIGHT;

    hg_wait_for_fence(hg, hg_is_frame_finished(window));
    hg_reset_fence(hg, hg_is_frame_finished(window));

    HgClock frame_timer;
    (void)hg_clock_tick(&frame_timer);

    const VkResult acquire_result = vkAcquireNextImageKHR(
        hg->device,
        window->swapchain,
        UINT64_MAX,
        hg_is_image_available(window),
        NULL,
        &window->current_image_index
    );
    switch (acquire_result) {
        case VK_SUCCESS: break;
        case VK_SUBOPTIMAL_KHR: {
            HG_WARN("Suboptimal KHR");
            return NULL;
        }
        case VK_ERROR_OUT_OF_DATE_KHR: {
            HG_WARN("Out of date KHR");
            return NULL;
        }
        case VK_TIMEOUT: HG_ERROR("Vulkan timed out waiting for image");
        case VK_NOT_READY: HG_ERROR("Vulkan not ready waiting for image");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkResult pool_result = vkResetDescriptorPool(
        hg->device, hg_current_descriptor_pool(window), 0
    );
    switch (pool_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkResult begin_result = vkBeginCommandBuffer(
        hg_current_cmd(window), &begin_info
    );
    switch (begin_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    window->current_commands = (HgCommands){
        .cmd = hg_current_cmd(window),
        .device = hg->device,
        .descriptor_pool = hg_current_descriptor_pool(window),
    };
    return &window->current_commands;
}

void hg_window_end_frame(
    const HurdyGurdy* hg,
    HgWindow* window,
    HgCommands* commands,
    HgTexture* framebuffer
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(window != NULL);
    HG_ASSERT(commands == &window->current_commands);

    if (framebuffer == NULL) {
        VkResult end_result = vkEndCommandBuffer(commands->cmd);
        switch (end_result) {
            case VK_SUCCESS: break;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: HG_ERROR("Vulkan invalid video parameters");
            default: HG_ERROR("Unexpected Vulkan error");
        }

        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &window->image_available_semaphores[window->current_frame_index],
            .pWaitDstStageMask = &(VkPipelineStageFlags){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
            .commandBufferCount = 1,
            .pCommandBuffers = &window->command_buffers[window->current_frame_index],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &window->ready_to_present_semaphores[window->current_image_index],
        };

        VkResult submit_result = vkQueueSubmit(
            hg->queue,
            1,
            &submit_info,
            hg_is_frame_finished(window)
        );
        switch (submit_result) {
            case VK_SUCCESS: break;
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
            default: HG_ERROR("Unexpected Vulkan error");
        }

        return;
    }

    VkImageMemoryBarrier2 barriers[] = {{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = framebuffer->layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                      ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
                      : framebuffer->layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                      ? VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
                      : framebuffer->layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                      ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
                      : VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask = framebuffer->layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                       ? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
                       : framebuffer->layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                       ? VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                       : framebuffer->layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                       ? VK_ACCESS_2_SHADER_READ_BIT
                       : VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
        .oldLayout = framebuffer->layout,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image = framebuffer->handle,
        .subresourceRange = {framebuffer->aspect, 0, 1, 0, 1},
    }, {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = hg_current_image(window),
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    }};
    framebuffer->layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    vkCmdPipelineBarrier2(commands->cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = HG_ARRAY_SIZE(barriers),
        .pImageMemoryBarriers = barriers,
    });

    HgBlitConfigInternal blit_src = {
        .image = framebuffer->handle,
        .aspect = framebuffer->aspect,
        .begin = {0, 0, 0},
        .end = {(i32)framebuffer->width, (i32)framebuffer->height, 1},
        .mip_level = 0,
        .array_layer = 0,
        .layer_count = 1,
    };
    HgBlitConfigInternal blit_dst = {
        .image = hg_current_image(window),
        .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .begin = {0, 0, 0},
        .end = {(i32)window->swapchain_width, (i32)window->swapchain_height, 1},
        .mip_level = 0,
        .array_layer = 0,
        .layer_count = 1,
    };
    hg_blit_image_internal(commands->cmd, &blit_dst, &blit_src, VK_FILTER_NEAREST);

    commands->previous_target = NULL;
    commands->previous_depth_buffer = NULL;

    VkImageMemoryBarrier2 final_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = hg_current_image(window),
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier2(commands->cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &final_barrier,
    });

    VkResult end_result = vkEndCommandBuffer(commands->cmd);
    switch (end_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: HG_ERROR("Vulkan invalid video parameters");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &window->image_available_semaphores[window->current_frame_index],
        .pWaitDstStageMask = &(VkPipelineStageFlags){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        .commandBufferCount = 1,
        .pCommandBuffers = &window->command_buffers[window->current_frame_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &window->ready_to_present_semaphores[window->current_image_index],
    };

    VkResult submit_result = vkQueueSubmit(
        hg->queue,
        1,
        &submit_info,
        hg_is_frame_finished(window)
    );
    switch (submit_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &window->ready_to_present_semaphores[window->current_image_index],
        .swapchainCount = 1,
        .pSwapchains = &window->swapchain,
        .pImageIndices = &window->current_image_index,
    };
    const VkResult present_result = vkQueuePresentKHR(hg->queue, &present_info);

    switch (present_result) {
        case VK_SUCCESS: break;
        case VK_SUBOPTIMAL_KHR: {
            HG_WARN("Suboptimal KHR");
            break;
        }
        case VK_ERROR_OUT_OF_DATE_KHR: {
            HG_WARN("Out of date KHR");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    window->current_commands.cmd = VK_NULL_HANDLE;
}

static VkBufferUsageFlags hg_buffer_usage_flags_to_vk(HgBufferUsageFlags usage) {
    return (usage & HG_BUFFER_USAGE_TRANSFER_SRC_BIT ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0)
         | (usage & HG_BUFFER_USAGE_TRANSFER_DST_BIT ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0)
         | (usage & HG_BUFFER_USAGE_UNIFORM_BUFFER_BIT ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0)
         | (usage & HG_BUFFER_USAGE_STORAGE_BUFFER_BIT ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0)
         | (usage & HG_BUFFER_USAGE_VERTEX_BUFFER_BIT ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0)
         | (usage & HG_BUFFER_USAGE_INDEX_BUFFER_BIT ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0);
}

static VkImageType hg_dimensions_to_image_type(u32 dimensions) {
    switch (dimensions) {
        case 1: return VK_IMAGE_TYPE_1D;
        case 2: return VK_IMAGE_TYPE_2D;
        case 3: return VK_IMAGE_TYPE_3D;
        default: return VK_IMAGE_TYPE_2D;
    }
}

static VkImageViewType hg_dimensions_to_view_type(u32 dimensions) {
    switch (dimensions) {
        case 1: return VK_IMAGE_VIEW_TYPE_1D;
        case 2: return VK_IMAGE_VIEW_TYPE_2D;
        case 3: return VK_IMAGE_VIEW_TYPE_3D;
        default: return VK_IMAGE_VIEW_TYPE_2D;
    }
}

static VkFormat hg_format_to_vk(HgFormat format) {
    return (VkFormat)format;
    // const VkFormat texture_formats[] = {
    //     ...
    // };
    // return texture_formats[format];
}

static HgFormat hg_format_from_vk(VkFormat format) {
    return (HgFormat)format;
    // const VkFormat texture_formats[] = {
    //     ...
    // };
    // return texture_formats[format];
}

static u32 hg_format_size(HgFormat format) {
    static const u32 texture_format_sizes[] = {
        [HG_FORMAT_UNDEFINED] = 0,
        [HG_FORMAT_R4G4_UNORM_PACK8 ] = 1,
        [HG_FORMAT_R4G4B4A4_UNORM_PACK16] = 2,
        [HG_FORMAT_B4G4R4A4_UNORM_PACK16] = 2,
        [HG_FORMAT_R5G6B5_UNORM_PACK16] = 2,
        [HG_FORMAT_B5G6R5_UNORM_PACK16] = 2,
        [HG_FORMAT_R5G5B5A1_UNORM_PACK16] = 2,
        [HG_FORMAT_B5G5R5A1_UNORM_PACK16] = 2,
        [HG_FORMAT_A1R5G5B5_UNORM_PACK16] = 2,
        [HG_FORMAT_R8_UNORM] = 1,
        [HG_FORMAT_R8_SNORM] = 1,
        [HG_FORMAT_R8_USCALED] = 1,
        [HG_FORMAT_R8_SSCALED] = 1,
        [HG_FORMAT_R8_UINT] = 1,
        [HG_FORMAT_R8_SINT] = 1,
        [HG_FORMAT_R8_SRGB] = 1,
        [HG_FORMAT_R8G8_UNORM] = 2,
        [HG_FORMAT_R8G8_SNORM] = 2,
        [HG_FORMAT_R8G8_USCALED] = 2,
        [HG_FORMAT_R8G8_SSCALED] = 2,
        [HG_FORMAT_R8G8_UINT] = 2,
        [HG_FORMAT_R8G8_SINT] = 2,
        [HG_FORMAT_R8G8_SRGB] = 2,
        [HG_FORMAT_R8G8B8_UNORM] = 3,
        [HG_FORMAT_R8G8B8_SNORM] = 3,
        [HG_FORMAT_R8G8B8_USCALED] = 3,
        [HG_FORMAT_R8G8B8_SSCALED] = 3,
        [HG_FORMAT_R8G8B8_UINT] = 3,
        [HG_FORMAT_R8G8B8_SINT] = 3,
        [HG_FORMAT_R8G8B8_SRGB] = 3,
        [HG_FORMAT_B8G8R8_UNORM] = 3,
        [HG_FORMAT_B8G8R8_SNORM] = 3,
        [HG_FORMAT_B8G8R8_USCALED] = 3,
        [HG_FORMAT_B8G8R8_SSCALED] = 3,
        [HG_FORMAT_B8G8R8_UINT] = 3,
        [HG_FORMAT_B8G8R8_SINT] = 3,
        [HG_FORMAT_B8G8R8_SRGB] = 3,
        [HG_FORMAT_R8G8B8A8_UNORM] = 4,
        [HG_FORMAT_R8G8B8A8_SNORM] = 4,
        [HG_FORMAT_R8G8B8A8_USCALED] = 4,
        [HG_FORMAT_R8G8B8A8_SSCALED] = 4,
        [HG_FORMAT_R8G8B8A8_UINT] = 4,
        [HG_FORMAT_R8G8B8A8_SINT] = 4,
        [HG_FORMAT_R8G8B8A8_SRGB] = 4,
        [HG_FORMAT_B8G8R8A8_UNORM] = 4,
        [HG_FORMAT_B8G8R8A8_SNORM] = 4,
        [HG_FORMAT_B8G8R8A8_USCALED] = 4,
        [HG_FORMAT_B8G8R8A8_SSCALED] = 4,
        [HG_FORMAT_B8G8R8A8_UINT] = 4,
        [HG_FORMAT_B8G8R8A8_SINT] = 4,
        [HG_FORMAT_B8G8R8A8_SRGB] = 4,
        [HG_FORMAT_A8B8G8R8_UNORM_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_SNORM_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_USCALED_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_SSCALED_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_UINT_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_SINT_PACK32] = 4,
        [HG_FORMAT_A8B8G8R8_SRGB_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_UNORM_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_SNORM_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_USCALED_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_SSCALED_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_UINT_PACK32] = 4,
        [HG_FORMAT_A2R10G10B10_SINT_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_UNORM_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_SNORM_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_USCALED_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_SSCALED_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_UINT_PACK32] = 4,
        [HG_FORMAT_A2B10G10R10_SINT_PACK32] = 4,
        [HG_FORMAT_R16_UNORM] = 2,
        [HG_FORMAT_R16_SNORM] = 2,
        [HG_FORMAT_R16_USCALED] = 2,
        [HG_FORMAT_R16_SSCALED] = 2,
        [HG_FORMAT_R16_UINT] = 2,
        [HG_FORMAT_R16_SINT] = 2,
        [HG_FORMAT_R16_SFLOAT] = 2,
        [HG_FORMAT_R16G16_UNORM] = 4,
        [HG_FORMAT_R16G16_SNORM] = 4,
        [HG_FORMAT_R16G16_USCALED] = 4,
        [HG_FORMAT_R16G16_SSCALED] = 4,
        [HG_FORMAT_R16G16_UINT] = 4,
        [HG_FORMAT_R16G16_SINT] = 4,
        [HG_FORMAT_R16G16_SFLOAT] = 4,
        [HG_FORMAT_R16G16B16_UNORM] = 6,
        [HG_FORMAT_R16G16B16_SNORM] = 6,
        [HG_FORMAT_R16G16B16_USCALED] = 6,
        [HG_FORMAT_R16G16B16_SSCALED] = 6,
        [HG_FORMAT_R16G16B16_UINT] = 6,
        [HG_FORMAT_R16G16B16_SINT] = 6,
        [HG_FORMAT_R16G16B16_SFLOAT] = 6,
        [HG_FORMAT_R16G16B16A16_UNORM] = 8,
        [HG_FORMAT_R16G16B16A16_SNORM] = 8,
        [HG_FORMAT_R16G16B16A16_USCALED] = 8,
        [HG_FORMAT_R16G16B16A16_SSCALED] = 8,
        [HG_FORMAT_R16G16B16A16_UINT] = 8,
        [HG_FORMAT_R16G16B16A16_SINT] = 8,
        [HG_FORMAT_R16G16B16A16_SFLOAT] = 8,
        [HG_FORMAT_R32_UINT] = 4,
        [HG_FORMAT_R32_SINT] = 4,
        [HG_FORMAT_R32_SFLOAT] = 4,
        [HG_FORMAT_R32G32_UINT] = 8,
        [HG_FORMAT_R32G32_SINT] = 8,
        [HG_FORMAT_R32G32_SFLOAT] = 8,
        [HG_FORMAT_R32G32B32_UINT] = 12,
        [HG_FORMAT_R32G32B32_SINT] = 12,
        [HG_FORMAT_R32G32B32_SFLOAT] = 12,
        [HG_FORMAT_R32G32B32A32_UINT] = 16,
        [HG_FORMAT_R32G32B32A32_SINT] = 16,
        [HG_FORMAT_R32G32B32A32_SFLOAT] = 16,
        [HG_FORMAT_R64_UINT] = 8,
        [HG_FORMAT_R64_SINT] = 8,
        [HG_FORMAT_R64_SFLOAT] = 8,
        [HG_FORMAT_R64G64_UINT] = 16,
        [HG_FORMAT_R64G64_SINT] = 16,
        [HG_FORMAT_R64G64_SFLOAT] = 16,
        [HG_FORMAT_R64G64B64_UINT] = 24,
        [HG_FORMAT_R64G64B64_SINT] = 24,
        [HG_FORMAT_R64G64B64_SFLOAT] = 24,
        [HG_FORMAT_R64G64B64A64_UINT] = 32,
        [HG_FORMAT_R64G64B64A64_SINT] = 32,
        [HG_FORMAT_R64G64B64A64_SFLOAT] = 32,
        [HG_FORMAT_B10G11R11_UFLOAT_PACK32] = 4,
        [HG_FORMAT_E5B9G9R9_UFLOAT_PACK32] = 4,
        [HG_FORMAT_D16_UNORM] = 2,
        [HG_FORMAT_X8_D24_UNORM_PACK32] = 4,
        [HG_FORMAT_D32_SFLOAT] = 4,
        [HG_FORMAT_S8_UINT] = 1,
        [HG_FORMAT_D16_UNORM_S8_UINT] = 2,
        [HG_FORMAT_D24_UNORM_S8_UINT] = 4,
        [HG_FORMAT_D32_SFLOAT_S8_UINT] = 4,
    };
    return texture_format_sizes[format];
}

static VkImageAspectFlags hg_format_to_aspect(HgFormat format) {
    switch (format) {
        case HG_FORMAT_D16_UNORM:
        case HG_FORMAT_X8_D24_UNORM_PACK32:
        case HG_FORMAT_D32_SFLOAT:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case HG_FORMAT_S8_UINT:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        case HG_FORMAT_D16_UNORM_S8_UINT:
        case HG_FORMAT_D24_UNORM_S8_UINT:
        case HG_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

static VkImageUsageFlags hg_texture_usage_flags_to_vk(HgTextureUsageFlags usage) {
    return (usage & HG_TEXTURE_USAGE_TRANSFER_SRC_BIT ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0)
         | (usage & HG_TEXTURE_USAGE_TRANSFER_DST_BIT ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0)
         | (usage & HG_TEXTURE_USAGE_SAMPLED_BIT ? VK_IMAGE_USAGE_SAMPLED_BIT : 0)
         | (usage & HG_TEXTURE_USAGE_STORAGE_BIT ? VK_IMAGE_USAGE_STORAGE_BIT : 0)
         | (usage & HG_TEXTURE_USAGE_RENDER_TARGET_BIT ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0)
         | (usage & HG_TEXTURE_USAGE_DEPTH_BUFFER_BIT ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0);
}

static VkImageLayout hg_texture_layout_to_vk(HgTextureLayout layout) {
    return (VkImageLayout)layout;
    // const VkImageLayout image_layouts[] = {
    //     [HG_TEXTURE_LAYOUT_UNDEFINED] = VK_IMAGE_LAYOUT_UNDEFINED,
    //     [HG_TEXTURE_LAYOUT_GENERAL] = VK_IMAGE_LAYOUT_GENERAL,
    //     [HG_TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //     [HG_TEXTURE_LAYOUT_DEPTH_BUFFER_OPTIMAL] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    //     [HG_TEXTURE_LAYOUT_DEPTH_BUFFER_READ_ONLY_OPTIMAL] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    //     [HG_TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //     [HG_TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //     [HG_TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    // };
    // return image_layouts[layout];
}

static VkSamplerAddressMode hg_sampler_edge_mode_to_vk(HgSamplerEdgeMode edge_mode) {
    return (VkSamplerAddressMode)edge_mode;
    // const VkSamplerAddressMode texture_edge_modes[] = {
    //     [HG_SAMPLER_EDGE_MODE_REPEAT] = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    //     [HG_SAMPLER_EDGE_MODE_MIRRORED_REPEAT] = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    //     [HG_SAMPLER_EDGE_MODE_CLAMP_TO_EDGE] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    //     [HG_SAMPLER_EDGE_MODE_CLAMP_TO_BORDER] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    //     [HG_SAMPLER_EDGE_MODE_MIRROR_CLAMP_TO_EDGE] = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
    // };
    // return texture_edge_modes[edge_mode];
}

static VkDescriptorType hg_descriptor_type_to_vk(HgDescriptorType type) {
    const VkDescriptorType descriptor_types[] = {
        [HG_DESCRIPTOR_TYPE_UNIFORM_BUFFER] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        [HG_DESCRIPTOR_TYPE_STORAGE_BUFFER] = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        [HG_DESCRIPTOR_TYPE_SAMPLED_TEXTURE] = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        [HG_DESCRIPTOR_TYPE_STORAGE_TEXTURE] = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    };
    return descriptor_types[type];
}

static VkVertexInputRate hg_vertex_input_rate_to_vk(HgVertexInputRate rate) {
    return (VkVertexInputRate)rate;
    // const VkVertexInputRate vertex_input_rates[] = {
    //     [HG_VERTEX_INPUT_RATE_VERTEX] = VK_VERTEX_INPUT_RATE_VERTEX,
    //     [HG_VERTEX_INPUT_RATE_INSTANCE] = VK_VERTEX_INPUT_RATE_INSTANCE,
    // };
    // return vertex_input_rates[rate];
}

static VkPrimitiveTopology hg_primitive_topology_to_vk(HgPrimitiveTopology topology) {
    return (VkPrimitiveTopology)topology;
    // const VkPrimitiveTopology primitive_topologies[] = {
    //     [HG_PRIMITIVE_TOPOLOGY_POINT_LIST] = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
    //     [HG_PRIMITIVE_TOPOLOGY_LINE_LIST] = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
    //     [HG_PRIMITIVE_TOPOLOGY_LINE_STRIP] = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
    //     [HG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    //     [HG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    //     [HG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
    // };
    // return primitive_topologies[topology];
}

static VkCullModeFlags hg_cull_mode_to_vk(HgCullModeFlagBits cull_mode) {
    return (VkCullModeFlags)cull_mode;
    // const VkCullModeFlags cull_modes[] = {
    //     [HG_CULL_MODE_NONE] = VK_CULL_MODE_NONE,
    //     [HG_CULL_MODE_FRONT_BIT] = VK_CULL_MODE_FRONT_BIT,
    //     [HG_CULL_MODE_BACK_BIT] = VK_CULL_MODE_BACK_BIT,
    //     [HG_CULL_MODE_FRONT_AND_BACK] = VK_CULL_MODE_FRONT_AND_BACK,
    // };
    // return cull_modes[cull_mode];
}

static VkCommandBuffer hg_current_cmd(HgWindow* window) {
    return window->command_buffers[window->current_frame_index];
}

static VkDescriptorPool hg_current_descriptor_pool(HgWindow* window) {
    return window->descriptor_pools[window->current_frame_index];
}

static VkImage hg_current_image(HgWindow* window) {
    return window->swapchain_images[window->current_image_index];
}

static VkFence hg_is_frame_finished(HgWindow* window) {
    return window->frame_finished_fences[window->current_frame_index];
}

static VkSemaphore hg_is_image_available(HgWindow* window) {
    return window->image_available_semaphores[window->current_frame_index];
}

static u32 hg_get_instance_extensions(
    const char** extension_buffer,
    u32 extension_buffer_capacity
) {
    HG_ASSERT(extension_buffer != NULL);

    u32 extension_count = hg_platform_get_vulkan_instance_extensions(
        extension_buffer,
        extension_buffer_capacity
    );

#ifndef NDEBUG
    if (extension_count >= extension_buffer_capacity)
        HG_ERROR("Vulkan extension buffer too small");
    extension_buffer[extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    ++extension_count;
#endif

    u32 extension_properties_count = 0;
    VkResult ext_count_res = vkEnumerateInstanceExtensionProperties(
        NULL,
        &extension_properties_count,
        NULL
    );
    switch (ext_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan incomplete instance extension enumeration");
            break;
        }
        case VK_ERROR_LAYER_NOT_PRESENT: HG_ERROR("Vulkan layer not present");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkExtensionProperties* extension_properties = hg_heap_alloc(
        extension_properties_count * sizeof(VkExtensionProperties)
    );

    VkResult ext_res = vkEnumerateInstanceExtensionProperties(
        NULL,
        &extension_properties_count,
        extension_properties
    );
    switch (ext_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan incomplete instance extension enumeration");
            break;
        }
        case VK_ERROR_LAYER_NOT_PRESENT: HG_ERROR("Vulkan layer not present");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    bool found_all = true;
    for (usize i = 0; i < extension_count; ++i) {
        bool found = false;
        for (usize j = 0; j < extension_properties_count; j++) {
            if (strcmp(extension_buffer[i], extension_properties[j].extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            found_all = false;
            break;
        }
    }
    if (!found_all)
        HG_ERROR("Vulkan instance missing required extensions");

    hg_heap_free(
        extension_properties,
        extension_properties_count * sizeof(VkExtensionProperties)
    );
    return extension_count;
}

static u32 hg_get_instance_layers(
    const char** layer_buffer,
    u32 layer_buffer_capacity
) {
#ifndef NDEBUG
    HG_ASSERT(layer_buffer != NULL);

    u32 property_count = 0;
    VkResult count_result = vkEnumerateInstanceLayerProperties(
        &property_count,
        NULL
    );
    switch (count_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan incomplete instance layer enumeration");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkLayerProperties* layer_props = hg_heap_alloc(
        property_count * sizeof(VkLayerProperties)
    );
    VkResult property_result = vkEnumerateInstanceLayerProperties(
        &property_count,
        layer_props
    );
    switch (property_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan incomplete instance layer enumeration");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    const char* desired_layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    u32 layer_count = 0;
    for (usize i = 0; i < HG_ARRAY_SIZE(desired_layers); ++i) {
        for (usize j = 0; j < property_count; j++) {
            if (strcmp(desired_layers[i], layer_props[j].layerName) == 0) {
                if (layer_count == layer_buffer_capacity)
                    HG_ERROR("Vulkan layer buffer capacity insufficient");
                layer_buffer[layer_count] = desired_layers[i];
                ++layer_count;
                break;
            }
        }
    }
    return layer_count;
#endif
    return 0;
}

static VkInstance hg_create_instance(void) {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hurdy Gurdy",
        .applicationVersion = 0,
        .pEngineName = "Hurdy Gurdy",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };

#define HG_MAX_INSTANCE_EXTENSIONS 16
    const char* extensions[HG_MAX_INSTANCE_EXTENSIONS];
    u32 extension_count = hg_get_instance_extensions(
        extensions,
        HG_MAX_INSTANCE_EXTENSIONS
    );
#undef HG_MAX_INSTANCE_EXTENSIONS

#define HG_MAX_INSTANCE_LAYERS 16
    const char* layers[HG_MAX_INSTANCE_LAYERS];
    u32 layer_count = hg_get_instance_layers(layers, HG_MAX_INSTANCE_LAYERS);
#undef HG_MAX_INSTANCE_LAYERS

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifndef NDEBUG
        .pNext = &debug_utils_messenger_create_info,
#endif
        .pApplicationInfo = &app_info,
        .enabledLayerCount = layer_count,
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions,
    };

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&instance_info, NULL, &instance);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_LAYER_NOT_PRESENT: HG_ERROR("Required Vulkan layer not present");
        case VK_ERROR_EXTENSION_NOT_PRESENT: HG_ERROR("Required Vulkan extension not present");
        case VK_ERROR_INCOMPATIBLE_DRIVER: HG_ERROR("Incompatible Vulkan driver");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan initialization failed");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return instance;
}

#ifndef NDEBUG
static VkBool32 debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    (void)type;
    (void)user_data;

    if (severity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                 |  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
        printf("Vulkan Info: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        printf("Vulkan Warning: %s\n", callback_data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        printf("Vulkan Error: %s\n", callback_data->pMessage);
    } else {
        printf("Vulkan Unknown: %s\n", callback_data->pMessage);
    }
    return VK_FALSE;
}

static const VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debug_callback,
};

static VkDebugUtilsMessengerEXT hg_create_debug_messenger(HurdyGurdy* hg) {
    HG_ASSERT(hg->instance != VK_NULL_HANDLE);

    VkDebugUtilsMessengerEXT messenger = NULL;
    VkResult result = vkCreateDebugUtilsMessengerEXT(
        hg->instance,
        &debug_utils_messenger_create_info,
        NULL,
        &messenger
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return messenger;
}
#endif

static bool find_queue_family(VkPhysicalDevice gpu, u32* queue_family) {
    HG_ASSERT(queue_family != NULL);
    HG_ASSERT(gpu != NULL);

    *queue_family = UINT32_MAX;

    u32 queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_families_count, NULL);
    if (queue_families_count == 0)
        return false;

    VkQueueFamilyProperties* queue_families = hg_heap_alloc(
        queue_families_count * sizeof(VkQueueFamilyProperties)
    );
    vkGetPhysicalDeviceQueueFamilyProperties(
        gpu,
        &queue_families_count,
        queue_families
    );

    for (u32 i = 0; i < queue_families_count; ++i) {
        if (queue_families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
            *queue_family = i;
            break;
        }
    }
    hg_heap_free(
        queue_families,
        queue_families_count * sizeof(VkQueueFamilyProperties)
    );
    return *queue_family != UINT32_MAX;
}

static u32 get_gpus(
    HurdyGurdy* hg,
    VkPhysicalDevice* gpu_buffer,
    u32 gpu_buffer_capacity
) {
    HG_ASSERT(hg->instance != VK_NULL_HANDLE);
    HG_ASSERT(gpu_buffer != NULL);

    u32 gpu_count;
    VkResult gpu_count_res = vkEnumeratePhysicalDevices(
        hg->instance,
        &gpu_count,
        NULL
    );
    switch (gpu_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan incomplete gpu enumeration");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan initialization failed");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    if (gpu_count > gpu_buffer_capacity)
        HG_ERROR("Vulkan gpu buffer capacity insufficient");

    VkResult gpu_result = vkEnumeratePhysicalDevices(hg->instance,
        &gpu_count,
        gpu_buffer
    );
    switch (gpu_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan incomplete gpu enumeration");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan initialization failed");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return gpu_count;
}

static VkPhysicalDevice hg_find_gpu(HurdyGurdy* hg) {
    HG_ASSERT(hg->instance != NULL);

#define HG_MAX_GPUS 16
    VkPhysicalDevice* gpus = hg_heap_alloc(
        HG_MAX_GPUS * sizeof(VkPhysicalDevice)
    );
    u32 gpu_count = get_gpus(hg, gpus, HG_MAX_GPUS);
    if (gpu_count == 0)
        HG_ERROR("No GPU available");

    u32 extension_properties_count = 0;
    VkExtensionProperties* extension_properties = NULL;;

    VkPhysicalDevice suitable_gpu = VK_NULL_HANDLE;
    for (u32 i = 0; i < gpu_count; ++i) {
        VkPhysicalDevice gpu = gpus[i];

        VkPhysicalDeviceFeatures features = {0};
        vkGetPhysicalDeviceFeatures(gpu, &features);
        if (features.sampleRateShading != VK_TRUE ||
            features.samplerAnisotropy != VK_TRUE)
            continue;

        u32 props_count = 0;
        VkResult ext_count_res = vkEnumerateDeviceExtensionProperties(
            gpu,
            NULL,
            &props_count,
            NULL
        );
        switch (ext_count_res) {
            case VK_SUCCESS: break;
            case VK_INCOMPLETE: {
                continue;
            }
            case VK_ERROR_LAYER_NOT_PRESENT: {
                continue;
            }
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            default: HG_ERROR("Unexpected Vulkan error");
        }

        if (props_count > extension_properties_count) {
            hg_heap_free(
                extension_properties,
                extension_properties_count * sizeof(VkExtensionProperties)
            );
            extension_properties_count = props_count;
            extension_properties = hg_heap_alloc(
                extension_properties_count * sizeof(VkExtensionProperties)
            );
        }

        VkResult ext_res = vkEnumerateDeviceExtensionProperties(gpu, NULL, &props_count, extension_properties);
        switch (ext_res) {
            case VK_SUCCESS: break;
            case VK_INCOMPLETE: {
                HG_WARN("Vulkan incomplete gpu extension enumeration");
                continue;
            }
            case VK_ERROR_LAYER_NOT_PRESENT: {
                HG_WARN("Vulkan layer not present");
                continue;
            }
            case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
            default: HG_ERROR("Unexpected Vulkan error");
        }

        bool found_all_extensions = true;
        for (usize j = 0; j < HG_ARRAY_SIZE(DeviceExtensions); j++) {
            bool found_extension = false;
            for (usize k = 0; k < props_count; k++) {
                if (strcmp(DeviceExtensions[j], extension_properties[k].extensionName) == 0) {
                    found_extension = true;
                    break;
                }
            }
            if (!found_extension) {
                found_all_extensions = false;
                break;
            }
        }
        if (!found_all_extensions)
            continue;

        if (!find_queue_family(gpu, &hg->queue_family_index))
            continue;

        suitable_gpu = gpu;
        break;
    }
    if (suitable_gpu == VK_NULL_HANDLE)
        HG_ERROR("Could not find suitable gpu");

    hg_heap_free(extension_properties, extension_properties_count * sizeof(VkExtensionProperties));
    hg_heap_free(gpus, HG_MAX_GPUS * sizeof(VkPhysicalDevice));
#undef HG_MAX_GPUS
    return suitable_gpu;
}

static VkDevice hg_create_device(HurdyGurdy* hg) {
    HG_ASSERT(hg->gpu != NULL);
    HG_ASSERT(hg->queue_family_index != UINT32_MAX);

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = VK_TRUE,
    };
    VkPhysicalDeviceSynchronization2Features synchronization2_feature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = &dynamic_rendering_feature,
        .synchronization2 = VK_TRUE,
    };
    VkPhysicalDeviceFeatures features = {
        .sampleRateShading = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = hg->queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &synchronization2_feature,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = HG_ARRAY_SIZE(DeviceExtensions),
        .ppEnabledExtensionNames = DeviceExtensions,
        .pEnabledFeatures = &features,
    };

    VkDevice device = NULL;
    VkResult result = vkCreateDevice(hg->gpu, &device_info, NULL, &device);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_EXTENSION_NOT_PRESENT: HG_ERROR("Required Vulkan extension not present");
        case VK_ERROR_FEATURE_NOT_PRESENT: HG_ERROR("Required Vulkan feature not present");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan initialization failed");
        case VK_ERROR_TOO_MANY_OBJECTS: HG_ERROR("Vulkan too many objects");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    return device;
}

static VmaAllocator hg_create_gpu_allocator(HurdyGurdy* hg) {
    HG_ASSERT(hg->instance != NULL);
    HG_ASSERT(hg->gpu != NULL);
    HG_ASSERT(hg->device != NULL);

    VmaAllocatorCreateInfo info = {
        .physicalDevice = hg->gpu,
        .device = hg->device,
        .instance = hg->instance,
        .vulkanApiVersion = VK_API_VERSION_1_3,
    };
    VmaAllocator allocator = NULL;
    VkResult result = vmaCreateAllocator(&info, &allocator);
    if (result != VK_SUCCESS)
        HG_ERROR("Could not create Vma allocator");
    return allocator;
}

static VkCommandPool hg_create_command_pool(HurdyGurdy* hg) {
    HG_ASSERT(hg->device != NULL);
    HG_ASSERT(hg->queue_family_index != UINT32_MAX);

    VkCommandPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = hg->queue_family_index,
    };
    VkCommandPool pool = NULL;
    VkResult result = vkCreateCommandPool(hg->device, &info, NULL, &pool);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan failed to initialize");
        default: HG_ERROR("Vulkan failed to create command pool");
    }
    return pool;
}

static void hg_allocate_command_buffers(
    const HurdyGurdy* hg,
    VkCommandBuffer* command_buffers,
    u32 command_buffer_count
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(command_buffers != NULL);
    HG_ASSERT(command_buffer_count != 0);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = hg->command_pool,
        .commandBufferCount = command_buffer_count,
    };
    const VkResult result = vkAllocateCommandBuffers(
        hg->device,
        &alloc_info,
        command_buffers
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }
}

// Create properly dynamic descriptor pool : TODO
static VkDescriptorPool hg_create_descriptor_pool(const HurdyGurdy* hg) {
    HG_ASSERT(hg->device != VK_NULL_HANDLE);

    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 512 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 512 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 512 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 512 },
    };
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .maxSets = 1024,
        .poolSizeCount = HG_ARRAY_SIZE(pool_sizes),
        .pPoolSizes = pool_sizes
    };
    VkDescriptorPool pool;
    VkResult result = vkCreateDescriptorPool(
        hg->device,
        &pool_info,
        NULL,
        &pool
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_FRAGMENTATION_EXT: HG_ERROR("Vulkan fragmentation error");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Vulkan failed to create descriptor pool");
    }
    return pool;
}

static VkDescriptorSet hg_allocate_descriptor_set(
    HgCommands* commands,
    u32 set_index
) {
    HG_ASSERT(commands != NULL);
    HG_ASSERT(set_index < commands->shader->descriptor_layout_count);

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = commands->descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &commands->shader->descriptor_layouts[set_index],
    };
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    VkResult result = vkAllocateDescriptorSets(
        commands->device,
        &alloc_info,
        &descriptor_set
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_FRAGMENTED_POOL: HG_ERROR("Vulkan fragmented pool"); // : TODO
        case VK_ERROR_OUT_OF_POOL_MEMORY: HG_ERROR("Vulkan ran out of descriptor pool memory"); // : TODO
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    return descriptor_set;
}

static VkSemaphore hg_create_semaphore(const HurdyGurdy* hg) {
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkSemaphore semaphore = VK_NULL_HANDLE;
    const VkResult result = vkCreateSemaphore(
        hg->device,
        &semaphore_info,
        NULL,
        &semaphore
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    return semaphore;
}

static VkFence hg_create_fence(const HurdyGurdy* hg, VkFenceCreateFlags flags) {
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = flags,
    };
    VkFence fence = VK_NULL_HANDLE;
    const VkResult result = vkCreateFence(
        hg->device,
        &fence_info,
        NULL,
        &fence
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    return fence;
}

static void hg_wait_for_fence(const HurdyGurdy* hg, VkFence fence) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(fence != VK_NULL_HANDLE);

    VkResult result = vkWaitForFences(
        hg->device,
        1,
        &fence,
        VK_TRUE,
        UINT64_MAX
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_TIMEOUT: HG_ERROR("Vulkan timed out waiting for fence");
        case VK_NOT_READY: HG_ERROR("Vulkan not ready");
        case VK_SUBOPTIMAL_KHR: HG_ERROR("Vulkan suboptimal");
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }
}

static void hg_reset_fence(const HurdyGurdy* hg, VkFence fence) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(fence != NULL);

    VkResult result = vkResetFences(hg->device, 1, &fence);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan error unknown");
        default: HG_ERROR("Unexpected Vulkan error");
    }
}

static VkDescriptorSetLayout hg_create_descriptor_set_layout(
    const HurdyGurdy* hg,
    const VkDescriptorSetLayoutBinding* bindings,
    u32 binding_count
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(bindings != NULL);
    HG_ASSERT(binding_count > 0);

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = binding_count,
        .pBindings = bindings,
    };
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    const VkResult result = vkCreateDescriptorSetLayout(
        hg->device,
        &layout_info,
        NULL,
        &layout
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return layout;
}

static void hg_create_descriptor_set_layouts(
    const HurdyGurdy* hg,
    VkDescriptorSetLayout* layouts,
    HgDescriptorSet* descriptor_sets,
    u32 descriptor_set_count,
    VkPipelineStageFlags stage_flags
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(layouts != NULL);
    HG_ASSERT(descriptor_sets != NULL);
    HG_ASSERT(descriptor_set_count > 0);
    HG_ASSERT(stage_flags != 0);

#define HG_MAX_DESCRIPTOR_BINDINGS 32
    for (u32 i = 0; i < descriptor_set_count; ++i) {
        HgDescriptorSet* set = &descriptor_sets[i];
        HG_ASSERT(set->binding_count < HG_MAX_DESCRIPTOR_BINDINGS);

        VkDescriptorSetLayoutBinding bindings[HG_MAX_DESCRIPTOR_BINDINGS];
        for (u32 j = 0; j < set->binding_count; ++j) {
            bindings[j] = (VkDescriptorSetLayoutBinding){
                .binding = j,
                .descriptorType = hg_descriptor_type_to_vk(
                    set->bindings[j].descriptor_type
                ),
                .descriptorCount = set->bindings[j].descriptor_count,
                .stageFlags = stage_flags,
            };
        }

        layouts[i] = hg_create_descriptor_set_layout(
            hg,
            bindings,
            descriptor_sets[i].binding_count
        );
    }
#undef HG_MAX_DESCRIPTOR_BINDINGS
}

static VkPipelineLayout hg_create_pipeline_layout(
    const HurdyGurdy* hg,
    const VkDescriptorSetLayout* layouts,
    u32 layout_count,
    const VkPushConstantRange* push_constants,
    u32 push_constant_count
) {
    HG_ASSERT(hg != NULL);
    if (layout_count > 0)
        HG_ASSERT(layouts != NULL);
    if (push_constant_count > 0)
        HG_ASSERT(push_constants != NULL);

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = layout_count,
        .pSetLayouts = layouts,
        .pushConstantRangeCount = push_constant_count,
        .pPushConstantRanges = push_constants,
    };
    VkPipelineLayout layout = VK_NULL_HANDLE;
    const VkResult result = vkCreatePipelineLayout(
        hg->device,
        &pipeline_layout_info,
        NULL,
        &layout
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return layout;
}

static VkShaderModule hg_create_shader_module(
    const HurdyGurdy* hg,
    const byte* code,
    usize size
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(code != NULL);
    HG_ASSERT(size > 0);

    VkShaderModuleCreateInfo shader_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = (const u32*)code,
    };
    VkShaderModule shader_module = VK_NULL_HANDLE;
    const VkResult result = vkCreateShaderModule(
        hg->device,
        &shader_module_info,
        NULL,
        &shader_module
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_INVALID_SHADER_NV: HG_ERROR("Vulkan invalid shader");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return shader_module;
}

static VkCommandBuffer hg_begin_single_time_cmd(const HurdyGurdy* hg) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = hg->command_pool,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd = NULL;
    VkResult cmd_result = vkAllocateCommandBuffers(
        hg->device,
        &alloc_info,
        &cmd
    );
    switch (cmd_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkResult begin_result = vkBeginCommandBuffer(cmd, &begin_info);
    switch (begin_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    HG_ASSERT(cmd != NULL);
    return cmd;
}

static void hg_end_single_time_cmd(const HurdyGurdy* hg, VkCommandBuffer cmd) {
    HG_ASSERT(cmd != NULL);

    VkResult end_result = vkEndCommandBuffer(cmd);
    switch (end_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: HG_ERROR("Vulkan invalid video parameters");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkFence fence = hg_create_fence(hg, 0);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    VkResult submit_result = vkQueueSubmit(hg->queue, 1, &submit_info, fence);
    switch (submit_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    hg_wait_for_fence(hg, fence);
    vkDestroyFence(hg->device, fence, NULL);

    vkFreeCommandBuffers(hg->device, hg->command_pool, 1, &cmd);
}

static void hg_copy_buffer_to_image(
    VkCommandBuffer cmd,
    HgTexture* dst,
    HgBuffer* src
) {
    HG_ASSERT(dst->allocation != NULL);
    HG_ASSERT(dst->handle != NULL);
    HG_ASSERT(src->allocation != NULL);
    HG_ASSERT(src->handle != NULL);

    const VkBufferImageCopy2 copy_region = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
        .imageSubresource = {dst->aspect, 0, 0, 1},
        .imageExtent = {dst->width, dst->height, dst->depth},
    };
    const VkCopyBufferToImageInfo2 copy_region_info = {
        .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
        .srcBuffer = src->handle,
        .dstImage = dst->handle,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &copy_region,
    };
    vkCmdCopyBufferToImage2(cmd, &copy_region_info);
}

static void hg_copy_image_to_buffer(
    VkCommandBuffer cmd,
    HgBuffer* dst,
    HgTexture* src
) {
    HG_ASSERT(dst->allocation != NULL);
    HG_ASSERT(dst->handle != NULL);
    HG_ASSERT(src->allocation != NULL);
    HG_ASSERT(src->handle != NULL);

    vkCmdCopyImageToBuffer2(cmd, &(VkCopyImageToBufferInfo2){
        .sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2,
        .srcImage = src->handle,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstBuffer = dst->handle,
        .regionCount = 1,
        .pRegions = &(VkBufferImageCopy2){
            .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
            .imageSubresource = {src->aspect, 0, 0, 1},
            .imageExtent = {src->width, src->height, src->depth},
        },
    });
}

static void hg_blit_image_internal(
    VkCommandBuffer cmd,
    const HgBlitConfigInternal* dst,
    const HgBlitConfigInternal* src,
    VkFilter filter
) {
    HG_ASSERT(dst->image != NULL);
    HG_ASSERT(src->image != NULL);

    VkImageBlit2 region = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .srcSubresource = {
            src->aspect,
            src->mip_level,
            src->array_layer,
            src->layer_count
        },
        .srcOffsets = {src->begin, src->end},
        .dstSubresource = {
            dst->aspect,
            dst->mip_level,
            dst->array_layer,
            dst->layer_count
        },
        .dstOffsets = {dst->begin, dst->end},
    };
    VkBlitImageInfo2 blit_image_info = {
        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .srcImage = src->image,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage = dst->image,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &region,
        .filter = filter,
    };
    vkCmdBlitImage2(cmd, &blit_image_info);
}

static void hg_write_cubemap(
    const HurdyGurdy* hg,
    HgTexture* dst,
    const void* src,
    VkImageLayout layout
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(dst != VK_NULL_HANDLE);

    if (src == NULL) {
        VkCommandBuffer cmd = hg_begin_single_time_cmd(hg);

        VkImageMemoryBarrier2 final_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = layout,
            .image = dst->handle,
            .subresourceRange = {dst->aspect, 0, 1, 0, 6},
        };
        vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &final_barrier,
        });
        dst->layout = layout;

        hg_end_single_time_cmd(hg, cmd);
        return;
    }

    VkExtent3D staging_extent = {dst->width * 4, dst->height * 3, dst->depth};

    HgTexture* staging_image = hg_texture_create(hg, &(HgTextureConfig){
        .width = staging_extent.width,
        .height = staging_extent.height,
        .depth = staging_extent.depth,
        .format = hg_format_from_vk(dst->format),
        .usage = HG_TEXTURE_USAGE_TRANSFER_SRC_BIT
               | HG_TEXTURE_USAGE_TRANSFER_DST_BIT,
    });
    hg_texture_write(
        hg,
        staging_image,
        src,
        HG_TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );

    VkCommandBuffer cmd = hg_begin_single_time_cmd(hg);

    VkImageMemoryBarrier2 transfer_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = dst->handle,
        .subresourceRange = {dst->aspect, 0, 1, 0, 6},
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &transfer_barrier,
    });

    VkImageCopy2 copies[] = {{
        .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
        .srcSubresource = {dst->aspect, 0, 0, 1},
        .srcOffset = {(i32)dst->width * 2, (i32)dst->height * 1, 0},
        .dstSubresource = {dst->aspect, 0, 0, 1},
        .extent = {dst->width, dst->height, 1},
    }, {
        .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
        .srcSubresource = {dst->aspect, 0, 0, 1},
        .srcOffset = {(i32)dst->width * 0, (i32)dst->height * 1, 0},
        .dstSubresource = {dst->aspect, 0, 1, 1},
        .extent = {dst->width, dst->height, 1},
    }, {
        .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
        .srcSubresource = {dst->aspect, 0, 0, 1},
        .srcOffset = {(i32)dst->width * 1, (i32)dst->height * 0, 0},
        .dstSubresource = {dst->aspect, 0, 2, 1},
        .extent = {dst->width, dst->height, 1},
    }, {
        .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
        .srcSubresource = {dst->aspect, 0, 0, 1},
        .srcOffset = {(i32)dst->width * 1, (i32)dst->height * 2, 0},
        .dstSubresource = {dst->aspect, 0, 3, 1},
        .extent = {dst->width, dst->height, 1},
    }, {
        .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
        .srcSubresource = {dst->aspect, 0, 0, 1},
        .srcOffset = {(i32)dst->width * 1, (i32)dst->height * 1, 0},
        .dstSubresource = {dst->aspect, 0, 4, 1},
        .extent = {dst->width, dst->height, 1},
    }, {
        .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
        .srcSubresource = {dst->aspect, 0, 0, 1},
        .srcOffset = {(i32)dst->width * 3, (i32)dst->height * 1, 0},
        .dstSubresource = {dst->aspect, 0, 5, 1},
        .extent = {dst->width, dst->height, 1},
    }};
    VkCopyImageInfo2 copy_region_info = {
        .sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
        .srcImage = staging_image->handle,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage = dst->handle,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = HG_ARRAY_SIZE(copies),
        .pRegions = copies,
    };
    vkCmdCopyImage2(cmd, &copy_region_info);

    VkImageMemoryBarrier2 final_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = layout,
        .image = dst->handle,
        .subresourceRange = {dst->aspect, 0, 1, 0, 6},
    };
    vkCmdPipelineBarrier2(cmd, &(VkDependencyInfo){
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &final_barrier,
    });
    dst->layout = layout;

    hg_end_single_time_cmd(hg, cmd);

    hg_texture_destroy(hg, staging_image);
}

static VkFormat hg_find_swapchain_format(
    const HurdyGurdy* hg,
    VkSurfaceKHR surface
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(surface != VK_NULL_HANDLE);

    u32 formats_count = 0;
    VkResult formats_count_res = vkGetPhysicalDeviceSurfaceFormatsKHR(
        hg->gpu,
        surface,
        &formats_count,
        NULL
    );
    switch (formats_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get swapchain formats incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    if (formats_count == 0)
        HG_ERROR("No swapchain formats available");

    VkSurfaceFormatKHR* formats = hg_heap_alloc(
        formats_count * sizeof(VkSurfaceFormatKHR)
    );

    VkResult format_result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        hg->gpu,
        surface,
        &formats_count,
        formats
    );
    switch (format_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get swapchain formats incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    VkFormat format = VK_FORMAT_UNDEFINED;
    for (usize i = 0; i < formats_count; ++i) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) {
            format = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        }
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
            format = VK_FORMAT_B8G8R8A8_SRGB;
            break;
        }
    }
    if (format == VK_FORMAT_UNDEFINED)
        HG_ERROR("No supported swapchain formats");

    hg_heap_free(formats, formats_count * sizeof(VkSurfaceFormatKHR));
    return format;
}

static VkPresentModeKHR hg_find_swapchain_present_mode(
    const HurdyGurdy* hg,
    VkSurfaceKHR surface
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(surface != VK_NULL_HANDLE);

    u32 present_mode_count = 0;
    VkResult present_count_res = vkGetPhysicalDeviceSurfacePresentModesKHR(
        hg->gpu,
        surface,
        &present_mode_count,
        NULL
    );
    switch (present_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get present modes incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkPresentModeKHR* present_modes = hg_heap_alloc(
        present_mode_count * sizeof(VkPresentModeKHR)
    );

    VkResult present_res = vkGetPhysicalDeviceSurfacePresentModesKHR(
        hg->gpu,
        surface,
        &present_mode_count,
        present_modes
    );
    switch (present_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get present modes incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (usize i = 0; i < present_mode_count; ++i) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    hg_heap_free(present_modes, present_mode_count * sizeof(VkPresentModeKHR));
    return present_mode;
}

static VkSwapchainKHR hg_create_new_swapchain(
    const HurdyGurdy* hg,
    VkSwapchainKHR old_swapchain,
    VkSurfaceKHR surface,
    VkFormat format,
    u32* width,
    u32* height
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(surface != VK_NULL_HANDLE);
    HG_ASSERT(format != VK_FORMAT_UNDEFINED);
    HG_ASSERT(width != NULL);
    HG_ASSERT(height != NULL);

    VkPresentModeKHR present_mode = hg_find_swapchain_present_mode(hg, surface);

    VkSurfaceCapabilitiesKHR surface_capabilities = {0};
    VkResult surface_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        hg->gpu,
        surface,
        &surface_capabilities
    );
    switch (surface_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        default: HG_ERROR("Unexpected Vulkan error");
    }
    if (surface_capabilities.currentExtent.width == 0 || surface_capabilities.currentExtent.height == 0)
        return VK_NULL_HANDLE;
    if (surface_capabilities.currentExtent.width < surface_capabilities.minImageExtent.width)
        return VK_NULL_HANDLE;
    if (surface_capabilities.currentExtent.height < surface_capabilities.minImageExtent.height)
        return VK_NULL_HANDLE;
    if (surface_capabilities.currentExtent.width > surface_capabilities.maxImageExtent.width)
        return VK_NULL_HANDLE;
    if (surface_capabilities.currentExtent.height > surface_capabilities.maxImageExtent.height)
        return VK_NULL_HANDLE;

    VkSwapchainCreateInfoKHR new_swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surface_capabilities.minImageCount,
        .imageFormat = format,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = surface_capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain,
    };

    VkSwapchainKHR new_swapchain = NULL;
    VkResult swapchain_result = vkCreateSwapchainKHR(
        hg->device,
        &new_swapchain_info,
        NULL,
        &new_swapchain
    );
    switch (swapchain_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: HG_ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: HG_ERROR("Vulkan surface lost");
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: HG_ERROR("Vulkan native window in use");
        case VK_ERROR_INITIALIZATION_FAILED: HG_ERROR("Vulkan initialization failed");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: HG_ERROR("Vulkan compression exhausted");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    *width = surface_capabilities.currentExtent.width;
    *height = surface_capabilities.currentExtent.height;
    return new_swapchain;
}

static void hg_get_swapchain_images(
    const HurdyGurdy* hg,
    VkSwapchainKHR swapchain,
    VkImage* images,
    u32* image_count
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(swapchain != NULL);
    HG_ASSERT(images != NULL);
    HG_ASSERT(image_count != NULL);

    VkResult image_count_result = vkGetSwapchainImagesKHR(
        hg->device,
        swapchain,
        image_count,
        NULL
    );
    switch (image_count_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get swapchain images incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    VkResult image_result = vkGetSwapchainImagesKHR(
        hg->device,
        swapchain,
        image_count,
        images
    );
    switch (image_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            HG_WARN("Vulkan get swapchain images incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        default: HG_ERROR("Unexpected Vulkan error");
    }
}

static void hg_stub_fn(void) {
    HG_ERROR("Stub function called\n");
}

#define HG_MAKE_VULKAN_FUNC(name) static PFN_##name s_pfn_##name = (PFN_##name)hg_stub_fn

HG_MAKE_VULKAN_FUNC(vkGetInstanceProcAddr);
HG_MAKE_VULKAN_FUNC(vkEnumerateInstanceExtensionProperties);
HG_MAKE_VULKAN_FUNC(vkEnumerateInstanceLayerProperties);
HG_MAKE_VULKAN_FUNC(vkCreateInstance);
HG_MAKE_VULKAN_FUNC(vkDestroyInstance);
#ifndef NDEBUG
HG_MAKE_VULKAN_FUNC(vkCreateDebugUtilsMessengerEXT);
HG_MAKE_VULKAN_FUNC(vkDestroyDebugUtilsMessengerEXT);
#endif

HG_MAKE_VULKAN_FUNC(vkEnumeratePhysicalDevices);
HG_MAKE_VULKAN_FUNC(vkEnumerateDeviceExtensionProperties);
HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceFeatures);
HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceProperties);
HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceFormatProperties);
HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties);
HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties2);
HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
HG_MAKE_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
HG_MAKE_VULKAN_FUNC(vkDestroySurfaceKHR);

HG_MAKE_VULKAN_FUNC(vkGetDeviceProcAddr);
HG_MAKE_VULKAN_FUNC(vkCreateDevice);
HG_MAKE_VULKAN_FUNC(vkDestroyDevice);

HG_MAKE_VULKAN_FUNC(vkGetDeviceQueue);
HG_MAKE_VULKAN_FUNC(vkQueueWaitIdle);
HG_MAKE_VULKAN_FUNC(vkQueueSubmit);
HG_MAKE_VULKAN_FUNC(vkQueuePresentKHR);

HG_MAKE_VULKAN_FUNC(vkCreateCommandPool);
HG_MAKE_VULKAN_FUNC(vkDestroyCommandPool);
HG_MAKE_VULKAN_FUNC(vkAllocateCommandBuffers);
HG_MAKE_VULKAN_FUNC(vkFreeCommandBuffers);
HG_MAKE_VULKAN_FUNC(vkBeginCommandBuffer);
HG_MAKE_VULKAN_FUNC(vkEndCommandBuffer);
HG_MAKE_VULKAN_FUNC(vkResetCommandBuffer);

HG_MAKE_VULKAN_FUNC(vkCreateDescriptorPool);
HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorPool);
HG_MAKE_VULKAN_FUNC(vkResetDescriptorPool);
HG_MAKE_VULKAN_FUNC(vkAllocateDescriptorSets);
HG_MAKE_VULKAN_FUNC(vkUpdateDescriptorSets);

HG_MAKE_VULKAN_FUNC(vkAllocateMemory);
HG_MAKE_VULKAN_FUNC(vkFreeMemory);
HG_MAKE_VULKAN_FUNC(vkMapMemory);
HG_MAKE_VULKAN_FUNC(vkUnmapMemory);
HG_MAKE_VULKAN_FUNC(vkFlushMappedMemoryRanges);
HG_MAKE_VULKAN_FUNC(vkInvalidateMappedMemoryRanges);

HG_MAKE_VULKAN_FUNC(vkCreateBuffer);
HG_MAKE_VULKAN_FUNC(vkDestroyBuffer);
HG_MAKE_VULKAN_FUNC(vkBindBufferMemory);
HG_MAKE_VULKAN_FUNC(vkBindBufferMemory2);
HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements);
HG_MAKE_VULKAN_FUNC(vkGetBufferMemoryRequirements2);
HG_MAKE_VULKAN_FUNC(vkGetDeviceBufferMemoryRequirements);

HG_MAKE_VULKAN_FUNC(vkCreateImage);
HG_MAKE_VULKAN_FUNC(vkDestroyImage);
HG_MAKE_VULKAN_FUNC(vkBindImageMemory);
HG_MAKE_VULKAN_FUNC(vkBindImageMemory2);
HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements);
HG_MAKE_VULKAN_FUNC(vkGetImageMemoryRequirements2);
HG_MAKE_VULKAN_FUNC(vkGetDeviceImageMemoryRequirements);

HG_MAKE_VULKAN_FUNC(vkCreateImageView);
HG_MAKE_VULKAN_FUNC(vkDestroyImageView);
HG_MAKE_VULKAN_FUNC(vkCreateSampler);
HG_MAKE_VULKAN_FUNC(vkDestroySampler);

HG_MAKE_VULKAN_FUNC(vkCreateShaderModule);
HG_MAKE_VULKAN_FUNC(vkDestroyShaderModule);
HG_MAKE_VULKAN_FUNC(vkCreateDescriptorSetLayout);
HG_MAKE_VULKAN_FUNC(vkDestroyDescriptorSetLayout);
HG_MAKE_VULKAN_FUNC(vkCreatePipelineLayout);
HG_MAKE_VULKAN_FUNC(vkDestroyPipelineLayout);
HG_MAKE_VULKAN_FUNC(vkCreateGraphicsPipelines);
HG_MAKE_VULKAN_FUNC(vkCreateComputePipelines);
HG_MAKE_VULKAN_FUNC(vkDestroyPipeline);

HG_MAKE_VULKAN_FUNC(vkCreateSwapchainKHR);
HG_MAKE_VULKAN_FUNC(vkDestroySwapchainKHR);
HG_MAKE_VULKAN_FUNC(vkGetSwapchainImagesKHR);
HG_MAKE_VULKAN_FUNC(vkAcquireNextImageKHR);

HG_MAKE_VULKAN_FUNC(vkCreateFence);
HG_MAKE_VULKAN_FUNC(vkDestroyFence);
HG_MAKE_VULKAN_FUNC(vkResetFences);
HG_MAKE_VULKAN_FUNC(vkWaitForFences);
HG_MAKE_VULKAN_FUNC(vkCreateSemaphore);
HG_MAKE_VULKAN_FUNC(vkDestroySemaphore);

HG_MAKE_VULKAN_FUNC(vkCmdCopyBuffer);
HG_MAKE_VULKAN_FUNC(vkCmdCopyBuffer2);
HG_MAKE_VULKAN_FUNC(vkCmdCopyImage);
HG_MAKE_VULKAN_FUNC(vkCmdCopyImage2);
HG_MAKE_VULKAN_FUNC(vkCmdBlitImage2);
HG_MAKE_VULKAN_FUNC(vkCmdCopyBufferToImage2);
HG_MAKE_VULKAN_FUNC(vkCmdCopyImageToBuffer2);
HG_MAKE_VULKAN_FUNC(vkCmdPipelineBarrier2);

HG_MAKE_VULKAN_FUNC(vkCmdBeginRendering);
HG_MAKE_VULKAN_FUNC(vkCmdEndRendering);
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

#undef HG_MAKE_VULKAN_FUNC

#define HG_LOAD_VULKAN_FUNC(name) s_pfn_##name = (PFN_##name)vkGetInstanceProcAddr(NULL, #name); \
    if (s_pfn_##name == NULL) { HG_ERROR("Could not load " #name); }

static void* s_libvulkan = NULL;

static bool s_is_vulkan_loaded = false;

#ifdef __unix__
#include <dlfcn.h>
#elif _WIN32 || _WIN64
#error "Windows not supported yet" // : TODO
#else
#error "unsupported platform"
#endif

static void hg_load_vulkan(void) {
    if (s_is_vulkan_loaded)
        return;

#ifdef __unix__
    s_libvulkan = dlopen("libvulkan.so.1", RTLD_LAZY);
    if (s_libvulkan == NULL)
        HG_ERRORF("Could not load libvulkan.so.1: %s", dlerror());

    *(void**)&s_pfn_vkGetInstanceProcAddr = dlsym(s_libvulkan, "vkGetInstanceProcAddr");
    if (s_pfn_vkGetInstanceProcAddr == NULL)
        HG_ERRORF("Could not load vkGetInstanceProcAddr: %s", dlerror());
#elif _WIN32 || _WIN64
#error "Windows not supported yet" // : TODO
#else
#error "unsupported platform"
#endif

    HG_LOAD_VULKAN_FUNC(vkEnumerateInstanceExtensionProperties);
    HG_LOAD_VULKAN_FUNC(vkEnumerateInstanceLayerProperties);
    HG_LOAD_VULKAN_FUNC(vkCreateInstance);

    s_is_vulkan_loaded = true;
}

#undef HG_LOAD_VULKAN_FUNC

#define HG_LOAD_VULKAN_INSTANCE_FUNC(name) s_pfn_##name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
    if (s_pfn_##name == NULL) { HG_ERROR("Could not load " #name); }

static bool s_is_vulkan_instance_loaded = false;

static void hg_load_vulkan_instance(VkInstance instance) {
    if (s_is_vulkan_instance_loaded)
        return;

    HG_LOAD_VULKAN_INSTANCE_FUNC(vkDestroyInstance);
#ifndef NDEBUG
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkCreateDebugUtilsMessengerEXT);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkDestroyDebugUtilsMessengerEXT);
#endif

    HG_LOAD_VULKAN_INSTANCE_FUNC(vkEnumeratePhysicalDevices);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkEnumerateDeviceExtensionProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceFeatures);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceFormatProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceMemoryProperties);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceMemoryProperties2);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);

    HG_LOAD_VULKAN_INSTANCE_FUNC(vkGetDeviceProcAddr);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkCreateDevice);
    HG_LOAD_VULKAN_INSTANCE_FUNC(vkDestroyDevice);

    HG_LOAD_VULKAN_INSTANCE_FUNC(vkDestroySurfaceKHR);

    s_is_vulkan_instance_loaded = true;
}

#undef HG_LOAD_VULKAN_INSTANCE_FUNC

#define HG_LOAD_VULKAN_DEVICE_FUNC(name) s_pfn_##name = (PFN_##name)vkGetDeviceProcAddr(device, #name); \
    if (s_pfn_##name == NULL) { HG_ERROR("Could not load " #name); }

static bool s_is_vulkan_device_loaded = false;

static void hg_load_vulkan_device(VkDevice device) {
    if (s_is_vulkan_device_loaded)
        return;

    HG_LOAD_VULKAN_DEVICE_FUNC(vkGetDeviceQueue);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkQueueWaitIdle);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkQueueSubmit);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkQueuePresentKHR);

    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateCommandPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroyCommandPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkAllocateCommandBuffers);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkFreeCommandBuffers);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkBeginCommandBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkEndCommandBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkResetCommandBuffer);

    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroyDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkResetDescriptorPool);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkAllocateDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkUpdateDescriptorSets);

    HG_LOAD_VULKAN_DEVICE_FUNC(vkAllocateMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkFreeMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkMapMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkUnmapMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkFlushMappedMemoryRanges);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkInvalidateMappedMemoryRanges);

    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroyBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkBindBufferMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkBindBufferMemory2);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkGetBufferMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkGetBufferMemoryRequirements2);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkGetDeviceBufferMemoryRequirements);

    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroyImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkBindImageMemory);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkBindImageMemory2);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkGetImageMemoryRequirements);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkGetImageMemoryRequirements2);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkGetDeviceImageMemoryRequirements);

    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateImageView);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroyImageView);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateSampler);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroySampler);

    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateShaderModule);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroyShaderModule);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateDescriptorSetLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroyDescriptorSetLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreatePipelineLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroyPipelineLayout);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateGraphicsPipelines);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateComputePipelines);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroyPipeline);

    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateSwapchainKHR);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroySwapchainKHR);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkGetSwapchainImagesKHR);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkAcquireNextImageKHR);

    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateFence);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroyFence);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkResetFences);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkWaitForFences);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCreateSemaphore);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkDestroySemaphore);

    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdCopyBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdCopyBuffer2);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdCopyImage);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdCopyImage2);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdBlitImage2);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdCopyBufferToImage2);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdCopyImageToBuffer2);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdPipelineBarrier2);

    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdBeginRendering);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdEndRendering);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdSetViewport);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdSetScissor);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdBindPipeline);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdBindDescriptorSets);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdPushConstants);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdBindVertexBuffers);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdBindIndexBuffer);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdDraw);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdDrawIndexed);
    HG_LOAD_VULKAN_DEVICE_FUNC(vkCmdDispatch);

    s_is_vulkan_device_loaded = true;
}

PFN_vkVoidFunction vkGetInstanceProcAddr(
    VkInstance instance,
    const char* pName
) {
    return s_pfn_vkGetInstanceProcAddr(instance, pName);
}

VkResult vkEnumerateInstanceExtensionProperties(
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties
) {
    return s_pfn_vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}

VkResult vkEnumerateInstanceLayerProperties(
    uint32_t* pPropertyCount,
    VkLayerProperties* pProperties
) {
    return s_pfn_vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

VkResult vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance
) {
    return s_pfn_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

void vkDestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyInstance(instance, pAllocator);
}

#ifndef NDEBUG
VkResult vkCreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger
) {
    return s_pfn_vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

void vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}
#endif

VkResult vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices
) {
    return s_pfn_vkEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}

VkResult vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties
) {
    return s_pfn_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

void vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pQueueFamilyPropertyCount,
    VkQueueFamilyProperties* pQueueFamilyProperties
) {
    s_pfn_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void vkGetPhysicalDeviceFeatures(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures* pFeatures
) {
    s_pfn_vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

void vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties
) {
    s_pfn_vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void vkGetPhysicalDeviceFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties* pFormatProperties
) {
    s_pfn_vkGetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}

void vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties
) {
    s_pfn_vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

void vkGetPhysicalDeviceMemoryProperties2(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2* pMemoryProperties
) {
    s_pfn_vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
}

VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    uint32_t* pSurfaceFormatCount,
    VkSurfaceFormatKHR* pSurfaceFormats
) {
    return s_pfn_vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    uint32_t* pPresentModeCount,
    VkPresentModeKHR* pPresentModes
) {
    return s_pfn_vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
}

VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR* pSurfaceCapabilities
) {
    return s_pfn_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
}

void vkDestroySurfaceKHR(
    VkInstance instance,
    VkSurfaceKHR surface,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroySurfaceKHR(instance, surface, pAllocator);
}

PFN_vkVoidFunction vkGetDeviceProcAddr(
    VkDevice device,
    const char* pName
) {
    return s_pfn_vkGetDeviceProcAddr(device, pName);
}

VkResult vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice
) {
    return s_pfn_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

void vkDestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyDevice(device, pAllocator);
}

void vkGetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue
) {
    s_pfn_vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

VkResult vkQueueWaitIdle(
    VkQueue queue
) {
    return s_pfn_vkQueueWaitIdle(queue);
}

VkResult vkQueueSubmit(
    VkQueue queue,
    uint32_t submitCount,
    const VkSubmitInfo* pSubmits,
    VkFence fence
) {
    return s_pfn_vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

VkResult vkQueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo
) {
    return s_pfn_vkQueuePresentKHR(queue, pPresentInfo);
}

VkResult vkCreateCommandPool(
    VkDevice device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkCommandPool* pCommandPool
) {
    return s_pfn_vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}

void vkDestroyCommandPool(
    VkDevice device,
    VkCommandPool commandPool,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyCommandPool(device, commandPool, pAllocator);
}

VkResult vkAllocateCommandBuffers(
    VkDevice device,
    const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer* pCommandBuffers
) {
    return s_pfn_vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}

void vkFreeCommandBuffers(
    VkDevice device,
    VkCommandPool commandPool,
    uint32_t commandBufferCount,
    const VkCommandBuffer* pCommandBuffers
) {
    s_pfn_vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

VkResult vkBeginCommandBuffer(
    VkCommandBuffer commandBuffer,
    const VkCommandBufferBeginInfo* pBeginInfo
) {
    return s_pfn_vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}

VkResult vkEndCommandBuffer(
    VkCommandBuffer commandBuffer
) {
    return s_pfn_vkEndCommandBuffer(commandBuffer);
}

VkResult vkResetCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkCommandBufferResetFlags flags
) {
    return s_pfn_vkResetCommandBuffer(commandBuffer, flags);
}

VkResult vkCreateDescriptorPool(
    VkDevice device,
    const VkDescriptorPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorPool* pDescriptorPool
) {
    return s_pfn_vkCreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
}

void vkDestroyDescriptorPool(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyDescriptorPool(device, descriptorPool, pAllocator);
}

VkResult vkResetDescriptorPool(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorPoolResetFlags flags
) {
    return s_pfn_vkResetDescriptorPool(device, descriptorPool, flags);
}

VkResult vkAllocateDescriptorSets(
    VkDevice device,
    const VkDescriptorSetAllocateInfo* pAllocateInfo,
    VkDescriptorSet* pDescriptorSets
) {
    return s_pfn_vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}

void vkUpdateDescriptorSets(
    VkDevice device,
    uint32_t descriptorWriteCount,
    const VkWriteDescriptorSet* pDescriptorWrites,
    uint32_t descriptorCopyCount,
    const VkCopyDescriptorSet* pDescriptorCopies
) {
    s_pfn_vkUpdateDescriptorSets(
        device,
        descriptorWriteCount,
        pDescriptorWrites,
        descriptorCopyCount,
        pDescriptorCopies
    );
}

VkResult vkAllocateMemory(
    VkDevice device,
    const VkMemoryAllocateInfo* pAllocateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDeviceMemory* pMemory
) {
    return s_pfn_vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}

void vkFreeMemory(
    VkDevice device,
    VkDeviceMemory memory,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkFreeMemory(device, memory, pAllocator);
}

VkResult vkMapMemory(
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData
) {
    return s_pfn_vkMapMemory(device, memory, offset, size, flags, ppData);
}

void vkUnmapMemory(
    VkDevice device,
    VkDeviceMemory memory
) {
    s_pfn_vkUnmapMemory(device, memory);
}

VkResult vkFlushMappedMemoryRanges(
    VkDevice device,
    uint32_t memoryRangeCount,
    const VkMappedMemoryRange* pMemoryRanges
) {
    return s_pfn_vkFlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

VkResult vkInvalidateMappedMemoryRanges(
    VkDevice device,
    uint32_t memoryRangeCount,
    const VkMappedMemoryRange* pMemoryRanges
) {
    return s_pfn_vkInvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

VkResult vkCreateBuffer(
    VkDevice device,
    const VkBufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkBuffer* pBuffer
) {
    return s_pfn_vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

void vkDestroyBuffer(
    VkDevice device,
    VkBuffer buffer,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyBuffer(device, buffer, pAllocator);
}

VkResult vkBindBufferMemory(
    VkDevice device,
    VkBuffer buffer,
    VkDeviceMemory memory,
    VkDeviceSize memoryOffset
) {
    return s_pfn_vkBindBufferMemory(device, buffer, memory, memoryOffset);
}

VkResult vkBindBufferMemory2(
    VkDevice device,
    uint32_t bindInfoCount,
    const VkBindBufferMemoryInfo* pBindInfos
) {
    return s_pfn_vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
}

void vkGetBufferMemoryRequirements(
    VkDevice device,
    VkBuffer buffer,
    VkMemoryRequirements* pMemoryRequirements
) {
    s_pfn_vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

void vkGetBufferMemoryRequirements2(
    VkDevice device,
    const VkBufferMemoryRequirementsInfo2* pInfo,
    VkMemoryRequirements2* pMemoryRequirements
) {
    s_pfn_vkGetBufferMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void vkGetDeviceBufferMemoryRequirements(
    VkDevice device,
    const VkDeviceBufferMemoryRequirements* pInfo,
    VkMemoryRequirements2* pMemoryRequirements
) {
    s_pfn_vkGetDeviceBufferMemoryRequirements(device, pInfo, pMemoryRequirements);
}

VkResult vkCreateImage(
        VkDevice device,
        const VkImageCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkImage* pImage
        ) {
    return s_pfn_vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}

void vkDestroyImage(
    VkDevice device,
    VkImage image,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyImage(device, image, pAllocator);
}

VkResult vkBindImageMemory(
    VkDevice device,
    VkImage image,
    VkDeviceMemory memory,
    VkDeviceSize memoryOffset
) {
    return s_pfn_vkBindImageMemory(device, image, memory, memoryOffset);
}

VkResult vkBindImageMemory2(
    VkDevice device,
    uint32_t bindInfoCount,
    const VkBindImageMemoryInfo* pBindInfos
) {
    return s_pfn_vkBindImageMemory2(device, bindInfoCount, pBindInfos);
}

void vkGetImageMemoryRequirements(
    VkDevice device,
    VkImage image,
    VkMemoryRequirements* pMemoryRequirements
) {
    s_pfn_vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}

void vkGetImageMemoryRequirements2(
    VkDevice device,
    const VkImageMemoryRequirementsInfo2* pInfo,
    VkMemoryRequirements2* pMemoryRequirements
) {
    s_pfn_vkGetImageMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void vkGetDeviceImageMemoryRequirements(
    VkDevice device,
    const VkDeviceImageMemoryRequirements* pInfo,
    VkMemoryRequirements2* pMemoryRequirements
) {
    s_pfn_vkGetDeviceImageMemoryRequirements(device, pInfo, pMemoryRequirements);
}

VkResult vkCreateImageView(
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImageView* pView
) {
    return s_pfn_vkCreateImageView(device, pCreateInfo, pAllocator, pView);
}

void vkDestroyImageView(
    VkDevice device,
    VkImageView imageView,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyImageView(device, imageView, pAllocator);
}

VkResult vkCreateSampler(
    VkDevice device,
    const VkSamplerCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSampler* pSampler
) {
    return s_pfn_vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
}

void vkDestroySampler(
    VkDevice device,
    VkSampler sampler,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroySampler(device, sampler, pAllocator);
}

VkResult vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule
) {
    return s_pfn_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}

void vkDestroyShaderModule(
    VkDevice device,
    VkShaderModule shaderModule,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyShaderModule(device, shaderModule, pAllocator);
}

VkResult vkCreateDescriptorSetLayout(
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorSetLayout* pSetLayout
) {
    return s_pfn_vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
}

void vkDestroyDescriptorSetLayout(
    VkDevice device,
    VkDescriptorSetLayout descriptorSetLayout,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

VkResult vkCreatePipelineLayout(
    VkDevice device,
    const VkPipelineLayoutCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkPipelineLayout* pPipelineLayout
) {
    return s_pfn_vkCreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
}

void vkDestroyPipelineLayout(
    VkDevice device,
    VkPipelineLayout pipelineLayout,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

VkResult vkCreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkGraphicsPipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines
) {
    return s_pfn_vkCreateGraphicsPipelines(
        device,
        pipelineCache,
        createInfoCount,
        pCreateInfos,
        pAllocator,
        pPipelines
    );
}

VkResult vkCreateComputePipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkComputePipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines
) {
    return s_pfn_vkCreateComputePipelines(
        device,
        pipelineCache,
        createInfoCount,
        pCreateInfos,
        pAllocator,
        pPipelines
    );
}

void vkDestroyPipeline(
    VkDevice device,
    VkPipeline pipeline,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyPipeline(device, pipeline, pAllocator);
}

VkResult vkCreateSwapchainKHR(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain
) {
    return s_pfn_vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}

void vkDestroySwapchainKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

VkResult vkGetSwapchainImagesKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t* pSwapchainImageCount,
    VkImage* pSwapchainImages
) {
    return s_pfn_vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

VkResult vkAcquireNextImageKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint64_t timeout,
    VkSemaphore semaphore,
    VkFence fence,
    uint32_t* pImageIndex
) {
    return s_pfn_vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VkResult vkCreateFence(
    VkDevice device,
    const VkFenceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkFence* pFence
) {
    return s_pfn_vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

void vkDestroyFence(
    VkDevice device,
    VkFence fence,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroyFence(device, fence, pAllocator);
}

VkResult vkResetFences(
    VkDevice device,
    uint32_t fenceCount,
    const VkFence* pFences
) {
    return s_pfn_vkResetFences(device, fenceCount, pFences);
}

VkResult vkWaitForFences(
    VkDevice device,
    uint32_t fenceCount,
    const VkFence* pFences,
    VkBool32 waitAll,
    uint64_t timeout
) {
    return s_pfn_vkWaitForFences(device, fenceCount, pFences, waitAll, timeout);
}

VkResult vkCreateSemaphore(
    VkDevice device,
    const VkSemaphoreCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSemaphore* pSemaphore
) {
    return s_pfn_vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

void vkDestroySemaphore(
    VkDevice device,
    VkSemaphore semaphore,
    const VkAllocationCallbacks* pAllocator
) {
    s_pfn_vkDestroySemaphore(device, semaphore, pAllocator);
}

void vkCmdCopyBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    uint32_t regionCount,
    const VkBufferCopy* pRegions
) {
    s_pfn_vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

void vkCmdCopyBuffer2(
    VkCommandBuffer commandBuffer,
    const VkCopyBufferInfo2* pCopyBufferInfo
) {
    s_pfn_vkCmdCopyBuffer2(commandBuffer, pCopyBufferInfo);
}

void vkCmdCopyImage(
    VkCommandBuffer commandBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage dstImage,
    VkImageLayout dstImageLayout,
    uint32_t regionCount,
    const VkImageCopy* pRegions
) {
    s_pfn_vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

void vkCmdCopyImage2(
    VkCommandBuffer commandBuffer,
    const VkCopyImageInfo2* pCopyImageInfo
) {
    s_pfn_vkCmdCopyImage2(commandBuffer, pCopyImageInfo);
}

void vkCmdBlitImage2(
    VkCommandBuffer commandBuffer,
    const VkBlitImageInfo2* pBlitImageInfo
) {
    s_pfn_vkCmdBlitImage2(commandBuffer, pBlitImageInfo);
}

void vkCmdCopyImageToBuffer2(
    VkCommandBuffer commandBuffer,
    const VkCopyImageToBufferInfo2* pCopyImageToBufferInfo
) {
    s_pfn_vkCmdCopyImageToBuffer2(commandBuffer, pCopyImageToBufferInfo);
}

void vkCmdCopyBufferToImage2(
    VkCommandBuffer commandBuffer,
    const VkCopyBufferToImageInfo2* pCopyBufferToImageInfo
) {
    s_pfn_vkCmdCopyBufferToImage2(commandBuffer, pCopyBufferToImageInfo);
}

void vkCmdPipelineBarrier2(
    VkCommandBuffer commandBuffer,
    const VkDependencyInfo* pDependencyInfo
) {
    s_pfn_vkCmdPipelineBarrier2(commandBuffer, pDependencyInfo);
}

void vkCmdBeginRendering(
    VkCommandBuffer commandBuffer,
    const VkRenderingInfo* pRenderingInfo
) {
    s_pfn_vkCmdBeginRendering(commandBuffer, pRenderingInfo);
}

void vkCmdEndRendering(
    VkCommandBuffer commandBuffer
) {
    s_pfn_vkCmdEndRendering(commandBuffer);
}

void vkCmdSetViewport(
    VkCommandBuffer commandBuffer,
    uint32_t firstViewport,
    uint32_t viewportCount,
    const VkViewport* pViewports
) {
    s_pfn_vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

void vkCmdSetScissor(
    VkCommandBuffer commandBuffer,
    uint32_t firstScissor,
    uint32_t scissorCount,
    const VkRect2D* pScissors
) {
    s_pfn_vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}

void vkCmdBindPipeline(
    VkCommandBuffer commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline pipeline
) {
    s_pfn_vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

void vkCmdBindDescriptorSets(
    VkCommandBuffer commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout,
    uint32_t firstSet,
    uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets,
    uint32_t dynamicOffsetCount,
    const uint32_t* pDynamicOffsets
) {
    s_pfn_vkCmdBindDescriptorSets(
        commandBuffer,
        pipelineBindPoint,
        layout,
        firstSet,
        descriptorSetCount,
        pDescriptorSets,
        dynamicOffsetCount,
        pDynamicOffsets
    );
}

void vkCmdPushConstants(
    VkCommandBuffer commandBuffer,
    VkPipelineLayout layout,
    VkShaderStageFlags stageFlags,
    uint32_t offset,
    uint32_t size,
    const void* pValues
) {
    s_pfn_vkCmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}

void vkCmdBindVertexBuffers(
    VkCommandBuffer commandBuffer,
    uint32_t firstBinding,
    uint32_t bindingCount,
    const VkBuffer* pBuffers,
    const VkDeviceSize* pOffsets
) {
    s_pfn_vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

void vkCmdBindIndexBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkIndexType indexType
) {
    s_pfn_vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

void vkCmdDraw(
    VkCommandBuffer commandBuffer,
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t firstVertex,
    uint32_t firstInstance
) {
    s_pfn_vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void vkCmdDrawIndexed(
    VkCommandBuffer commandBuffer,
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t firstIndex,
    int32_t vertexOffset,
    uint32_t firstInstance
) {
    s_pfn_vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void vkCmdDispatch(
    VkCommandBuffer commandBuffer,
    uint32_t groupCountX,
    uint32_t groupCountY,
    uint32_t groupCountZ
) {
    s_pfn_vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

