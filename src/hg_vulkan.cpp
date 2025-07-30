#include "hg_vulkan.h"

#include "hg_utils.h"
#include "hg_load.h"

namespace hg {

void destroy_buffer(Vk& vk, const GpuBuffer& buffer) {
    ASSERT(buffer.allocation != nullptr);
    ASSERT(buffer.buffer != nullptr);

    vmaDestroyBuffer(vk.gpu_allocator, buffer.buffer, buffer.allocation);
}

GpuBuffer create_buffer(Vk& vk, const GpuBufferConfig& config) {
    ASSERT(config.size != 0);
    ASSERT(config.usage != 0);

    const VkBufferCreateInfo buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = config.size,
        .usage = static_cast<u32>(config.usage),
    };

    VmaAllocationCreateInfo alloc_info{};
    if (config.memory_type == GpuMemoryType::DeviceLocal) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_info.flags = 0;
    } else if (config.memory_type == GpuMemoryType::LinearAccess) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    } else if (config.memory_type == GpuMemoryType::RandomAccess) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    } else {
        ERROR("Invalid memory type");
    }

    GpuBuffer buffer{
        .size = config.size,
        .type = config.memory_type,
    };
    const auto buffer_result = vmaCreateBuffer(
        vk.gpu_allocator,
        &buffer_info,
        &alloc_info,
        &buffer.buffer,
        &buffer.allocation,
        nullptr
    );
    switch (buffer_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    return buffer;
}

void write_buffer(Vk& vk, const GpuBuffer& dst, const void* src, usize size, usize offset) {
    ASSERT(dst.allocation != nullptr);
    ASSERT(dst.buffer != nullptr);
    ASSERT(src != nullptr);
    ASSERT(size != 0);
    if (dst.type == GpuMemoryType::LinearAccess)
        ASSERT(offset == 0);

    if (dst.type == GpuMemoryType::RandomAccess || dst.type == GpuMemoryType::LinearAccess) {
        const auto copy_result = vmaCopyMemoryToAllocation(vk.gpu_allocator, src, dst.allocation, offset, size);
        switch (copy_result) {
            case VK_SUCCESS: return;
            case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
            case VK_ERROR_MEMORY_MAP_FAILED: ERROR("Vulkan memory map failed");
            default: ERROR("Unexpected Vulkan error");
        }
    }
    ASSERT(dst.type == GpuMemoryType::DeviceLocal);

    const auto staging_buffer = create_buffer(vk, {
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, GpuMemoryType::LinearAccess
    });
    defer(destroy_buffer(vk, staging_buffer));
    write_buffer(vk, staging_buffer, src, size, 0);

    submit_single_time_commands(vk, [&](const VkCommandBuffer cmd) {
        copy_to_buffer(cmd, GpuBufferView{&dst, size, offset}, GpuBufferView{&staging_buffer, size});
    });
}

void destroy_image(Vk& vk, const GpuImage& image) {
    ASSERT(image.allocation != nullptr);
    ASSERT(image.image != nullptr);

    vmaDestroyImage(vk.gpu_allocator, image.image, image.allocation);
}

GpuImage create_image(Vk& vk, const GpuImageConfig& config) {
    ASSERT(config.extent.width > 0);
    ASSERT(config.extent.height > 0);
    ASSERT(config.extent.depth > 0);
    ASSERT(config.format != VK_FORMAT_UNDEFINED);
    ASSERT(config.usage != 0);
    ASSERT(config.sample_count != 0);
    ASSERT(config.mip_levels > 0);

    const VkImageCreateInfo image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = static_cast<VkFormat>(config.format),
        .extent = config.extent,
        .mipLevels = config.mip_levels,
        .arrayLayers = 1,
        .samples = static_cast<VkSampleCountFlagBits>(config.sample_count),
        .usage = static_cast<VkImageUsageFlags>(config.usage),
    };
    const VmaAllocationCreateInfo alloc_info{
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    GpuImage image{
        .extent = config.extent,
        .format = config.format,
        .mip_levels = config.mip_levels,
    };
    const auto image_result = vmaCreateImage(
            vk.gpu_allocator,
            &image_info,
            &alloc_info,
            &image.image,
            &image.allocation,
            nullptr
    );
    switch (image_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: ERROR("Vulkan compression exhausted");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    return image;
}

GpuImage create_cubemap(Vk& vk, const GpuCubemapConfig& config) {
    ASSERT(config.face_extent.width > 0);
    ASSERT(config.face_extent.height > 0);
    ASSERT(config.face_extent.depth > 0);
    ASSERT(config.format != VK_FORMAT_UNDEFINED);
    ASSERT(config.usage != 0);

    VkImageCreateInfo image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = static_cast<VkFormat>(config.format),
        .extent = config.face_extent,
        .mipLevels = 1,
        .arrayLayers = 6,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .usage = static_cast<VkImageUsageFlags>(config.usage),
    };
    VmaAllocationCreateInfo alloc_info{
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    GpuImage cubemap{
        .extent = config.face_extent,
        .format = config.format,
        .mip_levels = 1,
    };
    const auto image_result = vmaCreateImage(
            vk.gpu_allocator,
            &image_info,
            &alloc_info,
            &cubemap.image,
            &cubemap.allocation,
            nullptr
    );
    switch (image_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: ERROR("Vulkan compression exhausted");
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: ERROR("Vulkan invalid external handle");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    return cubemap;
}

void write_image(Vk& vk, GpuImage& dst, const ImageData& src, const GpuImageWriteConfig& config) {
    ASSERT(dst.allocation != nullptr);
    ASSERT(dst.image != nullptr);
    ASSERT(src.pixels != nullptr);
    ASSERT(src.alignment > 0);
    ASSERT(src.size.x > 0);
    ASSERT(src.size.y > 0);
    ASSERT(src.size.z == 1);

    const VkDeviceSize size = src.size.x * src.size.y * src.size.z * src.alignment;

    const auto staging_buffer = create_buffer(vk, {
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, GpuMemoryType::LinearAccess
    });
    defer(destroy_buffer(vk, staging_buffer));
    write_buffer(vk, staging_buffer, src.pixels, size, 0);

    submit_single_time_commands(vk, [&](const VkCommandBuffer cmd) {
        BarrierBuilder(vk, {.image_barriers = 1})
            .add_image_barrier(0, dst.image, {config.aspect_flags, 0, VK_REMAINING_MIP_LEVELS, 0, 1})
            .set_image_dst(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            .build_and_run(vk, cmd);

        copy_to_image(cmd, dst, staging_buffer, config.aspect_flags);

        BarrierBuilder(vk, {.image_barriers = 1})
            .add_image_barrier(0, dst.image, {config.aspect_flags, 0, VK_REMAINING_MIP_LEVELS, 0, 1})
            .set_image_src(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            .set_image_dst(0, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, config.final_layout)
            .build_and_run(vk, cmd);
    });

    dst.layout = config.final_layout;
}

void write_cubemap(Vk& vk, GpuImage& dst, const ImageData& src, const GpuImageWriteConfig& config) {
    ASSERT(dst.allocation != nullptr);
    ASSERT(dst.image != nullptr);
    ASSERT(src.pixels != nullptr);
    ASSERT(src.alignment > 0);
    ASSERT(src.size.x > 0);
    ASSERT(src.size.y > 0);
    ASSERT(src.size.z == 1);

    const VkExtent3D staging_extent{src.size.x, src.size.y, src.size.z};
    const VkExtent3D face_extent{staging_extent.width / 4, staging_extent.height / 3, 1};

    auto staging_image = create_image(vk, {
        .extent = staging_extent, 
        .format = dst.format,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
    });
    defer(destroy_image(vk, staging_image));
    write_image(vk, staging_image, src, {
        .final_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .aspect_flags = config.aspect_flags,
    });

    submit_single_time_commands(vk, [&](const VkCommandBuffer cmd) {
        BarrierBuilder(vk, {.image_barriers = 1})
            .add_image_barrier(0, dst.image, {config.aspect_flags, 0, 1, 0, 6})
            .set_image_dst(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            .build_and_run(vk, cmd);

        std::array copies{
            VkImageCopy2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(src.size.x) * 2 / 4, to_i32(src.size.y) * 1 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 0, 1},
                .extent = face_extent,
            },
            VkImageCopy2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(src.size.x) * 0 / 4, to_i32(src.size.y) * 1 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 1, 1},
                .extent = face_extent,
            },
            VkImageCopy2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(src.size.x) * 1 / 4, to_i32(src.size.y) * 0 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 2, 1},
                .extent = face_extent,
            },
            VkImageCopy2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(src.size.x) * 1 / 4, to_i32(src.size.y) * 2 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 3, 1},
                .extent = face_extent,
            },
            VkImageCopy2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(src.size.x) * 1 / 4, to_i32(src.size.y) * 1 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 4, 1},
                .extent = face_extent,
            },
            VkImageCopy2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
                .srcSubresource{config.aspect_flags, 0, 0, 1},
                .srcOffset{to_i32(src.size.x) * 3 / 4, to_i32(src.size.y) * 1 / 3, 0},
                .dstSubresource{config.aspect_flags, 0, 5, 1},
                .extent = face_extent,
            },
        };
        VkCopyImageInfo2 copy_region_info{
            .sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
            .srcImage = staging_image.image,
            .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .dstImage = dst.image,
            .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .regionCount = to_u32(copies.size()),
            .pRegions = copies.data(),
        };
        vkCmdCopyImage2(cmd, &copy_region_info);

        BarrierBuilder(vk, {.image_barriers = 1})
            .add_image_barrier(0, dst.image, {config.aspect_flags, 0, 1, 0, 6})
            .set_image_src(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            .set_image_dst(0, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            .build_and_run(vk, cmd);
    });

    dst.layout = config.final_layout;
}

void generate_mipmaps(Vk& vk, GpuImage& image, VkImageLayout final_layout) {
    ASSERT(image.mip_levels > 1);
    ASSERT(image.extent.width > 0);
    ASSERT(image.extent.height > 0);
    ASSERT(image.extent.depth > 0);
    ASSERT(image.format != VK_FORMAT_UNDEFINED);
    ASSERT(final_layout != VK_IMAGE_LAYOUT_UNDEFINED);

    VkFormatProperties format_properties{};
    vkGetPhysicalDeviceFormatProperties(vk.gpu, image.format, &format_properties);
    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        ERROR("Format does not support optimal tiling with linear filtering");

    submit_single_time_commands(vk, [&](const VkCommandBuffer cmd) {
        VkOffset3D mip_offset{to_i32(image.extent.width), to_i32(image.extent.height), to_i32(image.extent.depth)};

        BarrierBuilder(vk, {.image_barriers = 1})
            .add_image_barrier(0, image.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
            .set_image_dst(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            .build_and_run(vk, cmd);

        for (u32 level = 0; level < image.mip_levels - 1; ++level) {
            BarrierBuilder(vk, {.image_barriers = 1})
                .add_image_barrier(0, image.image, {VK_IMAGE_ASPECT_COLOR_BIT, level + 1, 1, 0, 1})
                .set_image_dst(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                .build_and_run(vk, cmd);

            VkImageBlit2 region{
                .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                .srcSubresource{VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1},
                .dstSubresource{VK_IMAGE_ASPECT_COLOR_BIT, level + 1, 0, 1},
            };
            region.srcOffsets[1] = mip_offset;
            if (mip_offset.x > 1)
                mip_offset.x /= 2;
            if (mip_offset.y > 1)
                mip_offset.y /= 2;
            if (mip_offset.z > 1)
                mip_offset.z /= 2;
            region.dstOffsets[1] = mip_offset;

            VkBlitImageInfo2 blit_image_info{
                .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                .srcImage = image.image,
                .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .dstImage = image.image,
                .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .regionCount = 1,
                .pRegions = &region,
                .filter = VK_FILTER_LINEAR,
            };
            vkCmdBlitImage2(cmd, &blit_image_info);

            BarrierBuilder(vk, {.image_barriers = 1})
                .add_image_barrier(0, image.image, {VK_IMAGE_ASPECT_COLOR_BIT, level + 1, 1, 0, 1})
                .set_image_src(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                .set_image_dst(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
                .build_and_run(vk, cmd);
        }

        BarrierBuilder(vk, {.image_barriers = 1})
            .add_image_barrier(0, image.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, image.mip_levels, 0, 1})
            .set_image_src(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            .set_image_dst(0, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, final_layout)
            .build_and_run(vk, cmd);
    });

    image.layout = final_layout;
}

VkImageView create_image_view(Vk& vk, const GpuImageViewConfig& config) {
    ASSERT(config.image != nullptr);
    ASSERT(config.format != VK_FORMAT_UNDEFINED);

    VkImageViewCreateInfo view_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = config.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = static_cast<VkFormat>(config.format),
        .subresourceRange = {static_cast<VkImageAspectFlags>(config.aspect_flags), 0, VK_REMAINING_MIP_LEVELS, 0, 1},
    };

    VkImageView view = nullptr;
    const VkResult result = vkCreateImageView(vk.device, &view_info, nullptr, &view);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    return view;
}

VkImageView create_cubemap_view(Vk& vk, const GpuImageViewConfig& config) {
    ASSERT(config.image != nullptr);
    ASSERT(config.format != VK_FORMAT_UNDEFINED);

    VkImageViewCreateInfo view_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = config.image,
        .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
        .format = static_cast<VkFormat>(config.format),
        .subresourceRange = {static_cast<VkImageAspectFlags>(config.aspect_flags), 0, VK_REMAINING_MIP_LEVELS, 0, 6},
    };

    VkImageView view = nullptr;
    const VkResult result = vkCreateImageView(vk.device, &view_info, nullptr, &view);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: ERROR("Vulkan invalid opaque capture address");
        default: ERROR("Unexpected Vulkan error");
    }

    return view;
}

void destroy_image_and_view(Vk& vk, const GpuImageAndView& image_and_view) {
    vkDestroyImageView(vk.device, image_and_view.view, nullptr);
    destroy_image(vk, image_and_view.image);
}

GpuImageAndView create_image_and_view(Vk& vk, const GpuImageAndViewConfig& config) {
    ASSERT(config.extent.width > 0);
    ASSERT(config.extent.height > 0);
    ASSERT(config.extent.depth > 0);
    ASSERT(config.format != VK_FORMAT_UNDEFINED);
    ASSERT(config.usage != 0);
    ASSERT(config.aspect_flags != 0);
    ASSERT(config.sample_count != 0);
    ASSERT(config.mip_levels > 0);

    GpuImageAndView image_and_view{};

    image_and_view.image = create_image(vk, {
        .extent = config.extent,
        .format = config.format,
        .usage = config.usage,
        .mip_levels = config.mip_levels,
        .sample_count = config.sample_count,
    });
    if (config.layout != VK_IMAGE_LAYOUT_UNDEFINED) {
        submit_single_time_commands(vk, [&](const VkCommandBuffer cmd) {
            BarrierBuilder(vk, {.image_barriers = 1})
                .add_image_barrier(0, image_and_view.image.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
                .set_image_dst(0, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, config.layout)
                .build_and_run(vk, cmd);
        });
        image_and_view.image.layout = config.layout;
    }

    image_and_view.view = create_image_view(vk, {
        .image = image_and_view.image.image,
        .format = config.format,
        .aspect_flags = config.aspect_flags,
    });

    return image_and_view;
}

GpuImageAndView create_cubemap_and_view(Vk& vk, const GpuCubemapAndViewConfig& config) {
    ASSERT(config.data.pixels != nullptr);
    ASSERT(config.data.alignment > 0);
    ASSERT(config.data.size.x > 0);
    ASSERT(config.data.size.y > 0);
    ASSERT(config.data.size.z == 1);
    ASSERT(config.format != VK_FORMAT_UNDEFINED);
    ASSERT(config.aspect_flags != 0);

    GpuImageAndView cubemap{};

    const VkExtent3D face_extent{config.data.size.x / 4, config.data.size.y / 3, 1};
    cubemap.image = create_cubemap(vk, {
        .face_extent = face_extent,
        .format = config.format,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    });
    write_cubemap(vk, cubemap.image, config.data, {
        .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    });

    cubemap.view = create_cubemap_view(vk, {
        .image = cubemap.image.image,
        .format = config.format,
        .aspect_flags = config.aspect_flags,
    });

    return cubemap;
}

VkSampler create_sampler(Vk& vk, const SamplerConfig& config) {
    ASSERT(config.mip_levels >= 1);

    VkPhysicalDeviceProperties gpu_properties{};
    vkGetPhysicalDeviceProperties(vk.gpu, &gpu_properties);
    const auto& limits = gpu_properties.limits;

    VkSamplerCreateInfo sampler_info{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .addressModeU = config.edge_mode,
        .addressModeV = config.edge_mode,
        .addressModeW = config.edge_mode,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = limits.maxSamplerAnisotropy,
        .maxLod = static_cast<f32>(config.mip_levels),
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    };
    if (config.type == SamplerType::Linear) {
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    } else if (config.type == SamplerType::Nearest) {
        sampler_info.magFilter = VK_FILTER_NEAREST;
        sampler_info.minFilter = VK_FILTER_NEAREST;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    } else {
        ERROR("Invalid sampler type");
    }

    VkSampler sampler = nullptr;
    const VkResult result = vkCreateSampler(vk.device, &sampler_info, nullptr, &sampler);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: ERROR("Vulkan invalid device address");
        default: ERROR("Unexpected Vulkan error");
    }

    return sampler;
}

void destroy_texture(Vk& vk, const Texture& texture) {
    ASSERT(texture.sampler != nullptr);

    vkDestroySampler(vk.device, texture.sampler, nullptr);
    destroy_image_and_view(vk, texture.image);
}

Texture create_texture(Vk& vk, const ImageData& data, const TextureConfig& config) {
    ASSERT(data.pixels != nullptr);
    ASSERT(data.alignment > 0);
    ASSERT(data.size.x > 0);
    ASSERT(data.size.y > 0);
    ASSERT(data.size.z == 1);

    Texture texture{};

    VkExtent3D extent{data.size.x, data.size.y, data.size.z};
    const u32 mip_levels = config.create_mips ? get_mip_count(extent) : 1;

    texture.image = create_image_and_view(vk, {
        .extent = extent,
        .format = config.format,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT
               | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
               | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .aspect_flags = config.aspect_flags,
        .mip_levels = mip_levels,
    });
    write_image(vk, texture.image.image, data, {
        .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .aspect_flags = config.aspect_flags,
    });
    if (mip_levels > 1)
        generate_mipmaps(vk, texture.image.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    texture.sampler = create_sampler(vk, {
        .type = config.sampler_type,
        .edge_mode = config.edge_mode,
        .mip_levels = mip_levels,
    });

    return texture;
}

Texture create_texture_cubemap(Vk& vk, const ImageData& data, const TextureConfig& config) {
    ASSERT(config.create_mips == false);

    Texture texture{};
    texture.image = create_cubemap_and_view(vk, {
        .data = data,
        .format = config.format,
        .aspect_flags = config.aspect_flags,
    });
    texture.sampler = create_sampler(vk, {
        .type = config.sampler_type,
        .edge_mode = config.edge_mode,
        .mip_levels = 1,
    });
    return texture;
}

VkDescriptorSetLayout create_descriptor_set_layout(Vk& vk, const DescriptorSetLayoutConfig& config) {
    ASSERT(config.bindings.count > 0);
    if (config.flags.count > 0)
        ASSERT(config.flags.count == config.bindings.count);

    const VkDescriptorSetLayoutBindingFlagsCreateInfo flag_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = to_u32(config.flags.count),
        .pBindingFlags = config.flags.data,
    };
    const VkDescriptorSetLayoutCreateInfo layout_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = config.flags.count == 0 ? nullptr : &flag_info,
        .bindingCount = to_u32(config.bindings.count),
        .pBindings = config.bindings.data,
    };

    VkDescriptorSetLayout layout = nullptr;
    const VkResult result = vkCreateDescriptorSetLayout(vk.device, &layout_info, nullptr, &layout);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    return layout;
}

VkDescriptorPool create_descriptor_pool(Vk& vk, const DescriptorPoolConfig& config) {
    ASSERT(config.max_sets >= 1);
    ASSERT(config.descriptors.count > 0);
    for (const auto& descriptor : config.descriptors) {
        ASSERT(descriptor.descriptorCount > 0);
    }

    VkDescriptorPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = config.max_sets,
        .poolSizeCount = to_u32(config.descriptors.count),
        .pPoolSizes = config.descriptors.data,
    };

    VkDescriptorPool pool = nullptr;
    const VkResult result = vkCreateDescriptorPool(vk.device, &pool_info, nullptr, &pool);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_FRAGMENTATION_EXT: ERROR("Vulkan fragmentation");
        default: ERROR("Unexpected Vulkan error");
    }

    return pool;
}

Result<void> allocate_descriptor_sets(
    Vk& vk,
    const VkDescriptorPool pool,
    const Slice<const VkDescriptorSetLayout> layouts,
    const Slice<VkDescriptorSet> out_sets
) {
    ASSERT(pool != nullptr);
    ASSERT(layouts.count > 0);
    for (const auto& layout : layouts) {
        ASSERT(layout != nullptr);
    }
    ASSERT(out_sets.count > 0);
    for (const auto& set : out_sets) {
        ASSERT(set == nullptr);
    }
    ASSERT(layouts.count == out_sets.count);

    const VkDescriptorSetAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = to_u32(layouts.count),
        .pSetLayouts = layouts.data,
    };
    const VkResult result = vkAllocateDescriptorSets(vk.device, &alloc_info, out_sets.data);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_POOL_MEMORY: return Err::OutOfDescriptorSets;
        case VK_ERROR_FRAGMENTATION_EXT: return Err::OutOfDescriptorSets;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    return ok();
}

void write_uniform_buffer_descriptor(Vk& vk, const DescriptorSetBinding& binding, const GpuBufferView& buffer) {
    ASSERT(buffer.buffer != nullptr);
    ASSERT(buffer.buffer->buffer != nullptr);
    ASSERT(buffer.range != 0);
    ASSERT(binding.set != nullptr);

    const VkDescriptorBufferInfo buffer_info{buffer.buffer->buffer, buffer.offset, buffer.range};
    const VkWriteDescriptorSet descriptor_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = binding.set,
        .dstBinding = binding.binding_index,
        .dstArrayElement = binding.array_index,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &buffer_info,
    };
    vkUpdateDescriptorSets(vk.device, 1, &descriptor_write, 0, nullptr);
}

void write_image_sampler_descriptor(Vk& vk, const DescriptorSetBinding& binding, const Texture& texture) {
    ASSERT(texture.sampler != nullptr);
    ASSERT(texture.image.view != nullptr);
    ASSERT(binding.set != nullptr);

    const VkDescriptorImageInfo image_info{
        .sampler = texture.sampler,
        .imageView = texture.image.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    const VkWriteDescriptorSet descriptor_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = binding.set,
        .dstBinding = binding.binding_index,
        .dstArrayElement = binding.array_index,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &image_info,
    };
    vkUpdateDescriptorSets(vk.device, 1, &descriptor_write, 0, nullptr);
}


VkPipelineLayout create_pipeline_layout(Vk& vk, const PipelineLayoutConfig& config) {
    VkPipelineLayoutCreateInfo layout_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = to_u32(config.set_layouts.count),
        .pSetLayouts = config.set_layouts.data,
        .pushConstantRangeCount = to_u32(config.push_ranges.count),
        .pPushConstantRanges = config.push_ranges.data,
    };

    VkPipelineLayout layout = nullptr;
    const VkResult result = vkCreatePipelineLayout(vk.device, &layout_info, nullptr, &layout);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    return layout;
}

static Result<Slice<char>> read_shader(Vk& vk, const std::filesystem::path path) {
    ASSERT(!path.empty());

    auto file = std::ifstream{path, std::ios::ate | std::ios::binary};
    if (!file.is_open()) {
        LOGF_ERROR("Could not open shader file: {}", path.string());
        return Err::ShaderFileNotFound;
    }

    auto code = ok<Slice<char>>(vk.stack.alloc<char>(file.tellg()));
    file.seekg(0);
    file.read(code->data, static_cast<std::streamsize>(code->count));
    file.close();

    return code;
}

void destroy_graphics_pipeline(Vk& vk, const GraphicsPipeline& pipeline) {
    ASSERT(pipeline.layout != nullptr);
    vkDestroyPipelineLayout(vk.device, pipeline.layout, nullptr);

    for (const auto& shader : pipeline.shaders) {
        ASSERT(shader != nullptr);
        g_pfn.vkDestroyShaderEXT(vk.device, shader, nullptr);
    }
}

Result<GraphicsPipeline> create_graphics_pipeline(Vk& vk, const GraphicsPipelineConfig& config) {
    ASSERT(!config.vertex_shader_path.empty());
    ASSERT(!config.fragment_shader_path.empty());
    ASSERT(config.code_type == VK_SHADER_CODE_TYPE_SPIRV_EXT && "binary shader code types untested");

    auto pipeline = ok<GraphicsPipeline>();

    pipeline->layout = create_pipeline_layout(vk, {
        .set_layouts = config.set_layouts,
        .push_ranges = config.push_ranges,
    });

    const auto vertex_code = read_shader(vk, config.vertex_shader_path);
    defer(vk.stack.dealloc(*vertex_code));
    if (vertex_code.has_err()) {
        LOGF_ERROR("Could not load vertex shader: {}", config.vertex_shader_path.string());
        return vertex_code.err();
    }
    const auto fragment_code = read_shader(vk, config.fragment_shader_path);
    defer(vk.stack.dealloc(*fragment_code));
    if (fragment_code.has_err()) {
        LOGF_ERROR("Could not load fragment shader: {}", config.fragment_shader_path.string());
        return fragment_code.err();
    }

    std::array shader_infos{
        VkShaderCreateInfoEXT{
            .sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
            .flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .nextStage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .codeType = static_cast<VkShaderCodeTypeEXT>(config.code_type),
            .codeSize = vertex_code->count,
            .pCode = vertex_code->data,
            .pName = "main",
            .setLayoutCount = to_u32(config.set_layouts.count),
            .pSetLayouts = config.set_layouts.data,
            .pushConstantRangeCount = to_u32(config.push_ranges.count),
            .pPushConstantRanges = config.push_ranges.data,
        },
        VkShaderCreateInfoEXT{
            .sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
            .flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .nextStage{},
            .codeType = static_cast<VkShaderCodeTypeEXT>(config.code_type),
            .codeSize = fragment_code->count,
            .pCode = fragment_code->data,
            .pName = "main",
            .setLayoutCount = to_u32(config.set_layouts.count),
            .pSetLayouts = config.set_layouts.data,
            .pushConstantRangeCount = to_u32(config.push_ranges.count),
            .pPushConstantRanges = config.push_ranges.data,
        },
    };

    const auto result = g_pfn.vkCreateShadersEXT(vk.device,
        to_u32(shader_infos.size()), shader_infos.data(),
        nullptr, pipeline->shaders.data()
    );
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT: {
            LOGF_ERROR(
                "Could not create shaders: {} and {}",
                config.vertex_shader_path.string(),
                config.fragment_shader_path.string()
            );
            return Err::ShaderFileInvalid;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INITIALIZATION_FAILED: ERROR("Vulkan initialization failed");
        default: ERROR("Unexpected Vulkan error");
    }

    return pipeline;
}

void bind_shaders(VkCommandBuffer cmd, const GraphicsPipeline& pipeline) {
    const std::array shader_stages{VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
    g_pfn.vkCmdBindShadersEXT(cmd, 2, shader_stages.data(), pipeline.shaders.data());
}

VkFence create_fence(Vk& vk, const VkFenceCreateFlags flags) {
    const VkFenceCreateInfo fence_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = static_cast<u32>(flags),
    };
    VkFence fence = VK_NULL_HANDLE;
    const auto result = vkCreateFence(vk.device, &fence_info, nullptr, &fence);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
    return fence;
}

void wait_for_fence(Vk& vk, const VkFence fence) {
    ASSERT(fence != nullptr);

    const auto result = vkWaitForFences(vk.device, 1, &fence, VK_TRUE, 1'000'000'000);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_TIMEOUT: ERROR("Vulkan timed out waiting for fence");
        case VK_NOT_READY: ERROR("Vulkan not ready");
        case VK_SUBOPTIMAL_KHR: ERROR("Vulkan suboptimal");
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }
}

void reset_fence(Vk& vk, const VkFence fence) {
    ASSERT(fence != nullptr);

    const auto reset_result = vkResetFences(vk.device, 1, &fence);
    switch (reset_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
}

VkSemaphore create_semaphore(Vk& vk) {
    const VkSemaphoreCreateInfo semaphore_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkSemaphore semaphore = nullptr;
    const auto result = vkCreateSemaphore(vk.device, &semaphore_info, nullptr, &semaphore);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    return semaphore;
}

void allocate_command_buffers(Vk& vk, const Slice<VkCommandBuffer> out_cmds) {
    const VkCommandBufferAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk.command_pool,
        .commandBufferCount = to_u32(out_cmds.count),
    };

    const VkResult result = vkAllocateCommandBuffers(vk.device, &alloc_info, out_cmds.data);
    switch (result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
}

VkCommandBuffer begin_single_time_commands(Vk& vk) {
    VkCommandBufferAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk.single_time_command_pool,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd = nullptr;
    const auto cmd_result = vkAllocateCommandBuffers(vk.device, &alloc_info, &cmd);
    switch (cmd_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    const auto begin_result = vkBeginCommandBuffer(cmd, &begin_info);
    switch (begin_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    ASSERT(cmd != nullptr);
    return cmd;
}

void end_single_time_commands(Vk& vk, VkCommandBuffer cmdpp) {
    VkCommandBuffer cmd = cmdpp;

    ASSERT(cmd != nullptr);

    const auto end_result = vkEndCommandBuffer(cmdpp);
    switch (end_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: ERROR("Vulkan invalid video parameters");
        default: ERROR("Unexpected Vulkan error");
    }

    const auto fence = create_fence(vk, {});
    defer(vkDestroyFence(vk.device, fence, nullptr));

    VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    const auto submit_result = vkQueueSubmit(vk.queue, 1, &submit_info, fence);
    switch (submit_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    wait_for_fence(vk, fence);

    vkFreeCommandBuffers(vk.device, vk.single_time_command_pool, 1, &cmd);
}

BarrierBuilder::BarrierBuilder(Vk& vk, const Config& config) {
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

void BarrierBuilder::build_and_run(Vk& vk, const VkCommandBuffer cmd, const VkDependencyFlags flags) {
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

void copy_to_buffer(const VkCommandBuffer cmd, const GpuBufferView& dst, const GpuBufferView& src) {
    ASSERT(dst.buffer != nullptr);
    ASSERT(dst.buffer->buffer != nullptr);
    ASSERT(src.buffer != nullptr);
    ASSERT(src.buffer->buffer != nullptr);
    ASSERT(src.range == dst.range);
    if (dst.range == 0)
        return;

    const VkBufferCopy copy_region{src.offset, dst.offset, dst.range};
    vkCmdCopyBuffer(cmd, src.buffer->buffer, dst.buffer->buffer, 1, &copy_region);
}

void copy_to_image(VkCommandBuffer cmd, GpuImage& dst, const GpuBuffer& src, VkImageAspectFlags aspect) {
    const VkBufferImageCopy2 copy_region{
        .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
        .imageSubresource{aspect, 0, 0, 1},
        .imageExtent = dst.extent,
    };
    const VkCopyBufferToImageInfo2 copy_region_info{
        .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
        .srcBuffer = src.buffer,
        .dstImage = dst.image,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &copy_region,
    };
    vkCmdCopyBufferToImage2(cmd, &copy_region_info);
}

void blit_image(VkCommandBuffer cmd, GpuImage& dst, VkRect2D dst_rect, const GpuImage& src, VkRect2D src_rect) {

}

void resolve_image(VkCommandBuffer cmd, GpuImage& dst, const GpuImage& src) {

}

VkSurfaceKHR create_surface(Vk& vk, SDL_Window* window) {
    ASSERT(window != nullptr);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    bool sdl_success = SDL_Vulkan_CreateSurface(window, vk.instance, nullptr, &surface);
    if (!sdl_success)
        ERRORF("Could not create Vulkan surface: {}", SDL_GetError());

    return surface;
}

void destroy_swapchain(Vk& vk, const Swapchain& swapchain) {
    for (const auto& fence : swapchain.frame_finished_fences) {
        ASSERT(fence != nullptr);
        vkDestroyFence(vk.device, fence, nullptr);
    }
    for (const auto& semaphore : swapchain.image_available_semaphores) {
        ASSERT(semaphore != nullptr);
        vkDestroySemaphore(vk.device, semaphore, nullptr);
    }
    for (const auto& semaphore : swapchain.ready_to_present_semaphores) {
        ASSERT(semaphore != nullptr);
        vkDestroySemaphore(vk.device, semaphore, nullptr);
    }

    for (const auto& cmd : swapchain.command_buffers) {
        ASSERT(cmd != nullptr);
    }
    vkFreeCommandBuffers(
        vk.device,
        vk.command_pool,
        to_u32(swapchain.command_buffers.size()),
        swapchain.command_buffers.data()
    );

    ASSERT(swapchain.swapchain != nullptr);
    vkDestroySwapchainKHR(vk.device, swapchain.swapchain, nullptr);
}

[[nodiscard]] static VkFormat get_swapchain_format(Vk& vk, const VkSurfaceKHR surface) {
    u32 format_count = 0;
    const VkResult format_count_res = vkGetPhysicalDeviceSurfaceFormatsKHR(vk.gpu, surface, &format_count, nullptr);
    switch (format_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            LOG_WARN("Vulkan get swapchain formats incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: ERROR("Vulkan surface lost");
        case VK_ERROR_UNKNOWN: ERROR("Vulkan unknown error");
        case VK_ERROR_VALIDATION_FAILED_EXT: ERROR("Vulkan validation failed");
        default: ERROR("Unexpected Vulkan error");
    }
    if (format_count == 0)
        ERROR("No swapchain formats available");

    auto formats = vk.stack.alloc<VkSurfaceFormatKHR>(format_count);
    defer(vk.stack.dealloc(formats));

    const VkResult format_result = vkGetPhysicalDeviceSurfaceFormatsKHR(vk.gpu, surface, &format_count, formats.data);
    switch (format_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            LOG_WARN("Vulkan get swapchain formats incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: ERROR("Vulkan surface lost");
        case VK_ERROR_UNKNOWN: ERROR("Vulkan unknown error");
        case VK_ERROR_VALIDATION_FAILED_EXT: ERROR("Vulkan validation failed");
        default: ERROR("Unexpected Vulkan error");
    }
    if (std::ranges::any_of(formats, [](const VkSurfaceFormatKHR format) {
        return format.format == VK_FORMAT_R8G8B8A8_SRGB;
    }))
        return VK_FORMAT_R8G8B8A8_SRGB;
    else if (std::ranges::any_of(formats, [](const VkSurfaceFormatKHR format) {
        return format.format == VK_FORMAT_B8G8R8A8_SRGB;
    }))
        return VK_FORMAT_B8G8R8A8_SRGB;
    else
        ERROR("No supported swapchain formats");
}

Result<Swapchain> create_swapchain(Vk& vk, const VkSurfaceKHR surface) {
    auto swapchain = ok<Swapchain>();

    swapchain->format = get_swapchain_format(vk, surface);

    const auto result = resize_swapchain(vk, *swapchain, surface);
    if (result.has_err())
        return result.err();

    allocate_command_buffers(vk, swapchain->command_buffers);

    for (auto& fence : swapchain->frame_finished_fences) {
        fence = create_fence(vk, VK_FENCE_CREATE_SIGNALED_BIT);
    }
    for (auto& semaphore : swapchain->image_available_semaphores) {
        semaphore = create_semaphore(vk);
    }
    for (auto& semaphore : swapchain->ready_to_present_semaphores) {
        semaphore = create_semaphore(vk);
    }

    return swapchain;
}

[[nodiscard]] static VkPresentModeKHR get_swapchain_present_mode(Vk& vk, const VkSurfaceKHR surface) {
    u32 present_mode_count = 0;
    const VkResult present_count_res = vkGetPhysicalDeviceSurfacePresentModesKHR(vk.gpu, surface, &present_mode_count, nullptr);
    switch (present_count_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: LOG_WARN("Vulkan get present modes incomplete"); break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }

    auto present_modes = vk.stack.alloc<VkPresentModeKHR>(present_mode_count);
    defer(vk.stack.dealloc(present_modes));

    const VkResult present_res = vkGetPhysicalDeviceSurfacePresentModesKHR(vk.gpu, surface, &present_mode_count, present_modes.data);
    switch (present_res) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: LOG_WARN("Vulkan get present modes incomplete"); break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }

    return std::ranges::any_of(present_modes, [](const VkPresentModeKHR mode) {
        return mode == VK_PRESENT_MODE_MAILBOX_KHR;
    })
        // ? VK_PRESENT_MODE_MAILBOX_KHR
        ? VK_PRESENT_MODE_FIFO_KHR
        : VK_PRESENT_MODE_FIFO_KHR;
}

Result<void> resize_swapchain(Vk& vk, Swapchain& swapchain, const VkSurfaceKHR surface) {
    const VkPresentModeKHR present_mode = get_swapchain_present_mode(vk, surface);

    VkSurfaceCapabilitiesKHR surface_capabilities{};
    const VkResult surface_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.gpu, surface, &surface_capabilities);
    switch (surface_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_SURFACE_LOST_KHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }
    if (surface_capabilities.currentExtent.width == 0 || surface_capabilities.currentExtent.height == 0)
        return Err::InvalidWindow;
    if (surface_capabilities.currentExtent.width < surface_capabilities.minImageExtent.width)
        return Err::InvalidWindow;
    if (surface_capabilities.currentExtent.height < surface_capabilities.minImageExtent.height)
        return Err::InvalidWindow;
    if (surface_capabilities.currentExtent.width > surface_capabilities.maxImageExtent.width)
        return Err::InvalidWindow;
    if (surface_capabilities.currentExtent.height > surface_capabilities.maxImageExtent.height)
        return Err::InvalidWindow;
    swapchain.extent = surface_capabilities.currentExtent;

    const VkSwapchainCreateInfoKHR new_swapchain_info{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surface_capabilities.maxImageCount == 0
                         ? Swapchain::MaxImages
                         : std::min(
                             std::min(surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount),
                             Swapchain::MaxImages
                         ),
        .imageFormat = swapchain.format,
        .imageColorSpace = Swapchain::ColorSpace,
        .imageExtent = swapchain.extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = swapchain.swapchain,
    };

    VkSwapchainKHR new_swapchain = nullptr;
    const VkResult swapchain_result = vkCreateSwapchainKHR(vk.device, &new_swapchain_info, nullptr, &new_swapchain);
    switch (swapchain_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: ERROR("Vulkan surface lost");
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: ERROR("Vulkan native window in use");
        case VK_ERROR_INITIALIZATION_FAILED: ERROR("Vulkan initialization failed");
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: ERROR("Vulkan compression exhausted");
        default: ERROR("Unexpected Vulkan error");
    }

    const auto wait_result = vkQueueWaitIdle(vk.queue);
    switch (wait_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    if (swapchain.swapchain != nullptr) {
        vkDestroySwapchainKHR(vk.device, swapchain.swapchain, nullptr);
    }
    swapchain.swapchain = new_swapchain;

    const auto image_count_result = vkGetSwapchainImagesKHR(
        vk.device,
        swapchain.swapchain,
        &swapchain.image_count,
        nullptr
    );
    switch (image_count_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            LOG_WARN("Vulkan get swapchain images incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    const auto image_result = vkGetSwapchainImagesKHR(
        vk.device,
        swapchain.swapchain,
        &swapchain.image_count,
        swapchain.images.data()
    );
    switch (image_result) {
        case VK_SUCCESS: break;
        case VK_INCOMPLETE: {
            LOG_WARN("Vulkan get swapchain images incomplete");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }

    return ok();
}

Result<VkCommandBuffer> begin_frame(Vk& vk, Swapchain& swapchain) {
    ASSERT(!swapchain.recording);
    ASSERT(swapchain.current_cmd() != nullptr);

    swapchain.current_frame_index = (swapchain.current_frame_index + 1) % Swapchain::MaxFramesInFlight;

    wait_for_fence(vk, swapchain.is_frame_finished());
    reset_fence(vk, swapchain.is_frame_finished());

    const VkResult acquire_result = vkAcquireNextImageKHR(
        vk.device,
        swapchain.swapchain,
        1'000'000'000,
        swapchain.is_image_available(),
        nullptr,
        &swapchain.current_image_index
    );
    switch (acquire_result) {
        case VK_SUCCESS: break;
        case VK_TIMEOUT: return Err::FrameTimeout;
        case VK_NOT_READY: return Err::FrameTimeout;
        case VK_SUBOPTIMAL_KHR: return Err::InvalidWindow;
        case VK_ERROR_OUT_OF_DATE_KHR: return Err::InvalidWindow;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }

    auto cmd = swapchain.current_cmd();

    const VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    const auto begin_result = vkBeginCommandBuffer(cmd, &begin_info);
    switch (begin_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        default: ERROR("Unexpected Vulkan error");
    }
    swapchain.recording = true;

    const VkViewport viewport{
        0.0f, 0.0f,
        static_cast<f32>(swapchain.extent.width), static_cast<f32>(swapchain.extent.height),
        0.0f, 1.0f,
    };
    vkCmdSetViewportWithCount(cmd, 1, &viewport);
    const VkRect2D scissor{{0, 0}, swapchain.extent};
    vkCmdSetScissorWithCount(cmd, 1, &scissor);

    vkCmdSetRasterizerDiscardEnable(cmd, VK_FALSE);
    vkCmdSetDepthTestEnable(cmd, VK_TRUE);
    vkCmdSetDepthWriteEnable(cmd, VK_TRUE);
    vkCmdSetDepthCompareOp(cmd, VK_COMPARE_OP_LESS);
    vkCmdSetDepthBiasEnable(cmd, VK_FALSE);
    vkCmdSetStencilTestEnable(cmd, VK_FALSE);
    vkCmdSetFrontFace(cmd, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    vkCmdSetCullMode(cmd, VK_CULL_MODE_NONE);
    vkCmdSetPrimitiveRestartEnable(cmd, VK_FALSE);
    vkCmdSetPrimitiveTopology(cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    g_pfn.vkCmdSetPolygonModeEXT(cmd, VK_POLYGON_MODE_FILL);
    g_pfn.vkCmdSetAlphaToCoverageEnableEXT(cmd, VK_FALSE);

    const VkSampleMask sample_mask = 0xff;
    g_pfn.vkCmdSetSampleMaskEXT(cmd, VK_SAMPLE_COUNT_1_BIT, &sample_mask);
    g_pfn.vkCmdSetRasterizationSamplesEXT(cmd, VK_SAMPLE_COUNT_1_BIT);

    const VkBool32 color_blend_enable = VK_FALSE;
    g_pfn.vkCmdSetColorBlendEnableEXT(cmd, 0, 1, &color_blend_enable);
    const VkColorComponentFlags color_write_mask
        = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    g_pfn.vkCmdSetColorWriteMaskEXT(cmd, 0, 1, &color_write_mask);

    return ok(cmd);
}

Result<void> end_frame(Vk& vk, Swapchain& swapchain) {
    ASSERT(swapchain.recording);
    ASSERT(swapchain.swapchain != nullptr);
    ASSERT(swapchain.current_cmd() != nullptr);

    const VkResult end_result = vkEndCommandBuffer(swapchain.current_cmd());
    switch (end_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: ERROR("Vulkan invalid video parameters");
        default: ERROR("Unexpected Vulkan error");
    }
    swapchain.recording = false;

    constexpr VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchain.is_image_available(),
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &swapchain.current_cmd(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &swapchain.is_ready_to_present(),
    };

    const auto submit_result = vkQueueSubmit(vk.queue, 1, &submit_info, swapchain.is_frame_finished());
    switch (submit_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    const VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchain.is_ready_to_present(),
        .swapchainCount = 1,
        .pSwapchains = &swapchain.swapchain,
        .pImageIndices = &swapchain.current_image_index,
    };
    const VkResult present_result = vkQueuePresentKHR(vk.queue, &present_info);
    switch (present_result) {
        case VK_SUCCESS: break;
        case VK_SUBOPTIMAL_KHR: [[fallthrough]];
        case VK_ERROR_OUT_OF_DATE_KHR: {
            return Err::InvalidWindow;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        case VK_ERROR_SURFACE_LOST_KHR: ERROR("Vulkan surface lost");
        default: ERROR("Unexpected Vulkan error");
    }

    return ok();
}

} // namespace hg
