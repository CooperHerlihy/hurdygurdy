#include "hg_vulkan_utils.h"

namespace hg {

vk::CommandBuffer begin_single_time_commands(const Engine& engine) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.single_time_command_pool != nullptr);

    vk::CommandBufferAllocateInfo alloc_info = {
        .commandPool = engine.single_time_command_pool,
        .commandBufferCount = 1,
    };
    vk::CommandBuffer cmd = {};
    const auto cmd_result = engine.device.allocateCommandBuffers(&alloc_info, &cmd);
    critical_assert(cmd_result == vk::Result::eSuccess);

    const auto begin_result = cmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    critical_assert(begin_result == vk::Result::eSuccess);

    return cmd;
}

void end_single_time_commands(const Engine& engine, const vk::CommandBuffer cmd) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.single_time_command_pool != nullptr);

    const auto end_result = cmd.end();
    critical_assert(end_result == vk::Result::eSuccess);

    vk::SubmitInfo submit_info = {
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    const auto [fence_result, fence] = engine.device.createFence({});
    critical_assert(fence_result == vk::Result::eSuccess);
    defer(engine.device.destroyFence(fence));

    const auto submit_result = engine.queue.submit({submit_info}, fence);
    critical_assert(submit_result == vk::Result::eSuccess);

    const auto wait_result = engine.device.waitForFences({fence}, vk::True, UINT64_MAX);
    critical_assert(wait_result == vk::Result::eSuccess);

    engine.device.freeCommandBuffers(engine.single_time_command_pool, {cmd});
}

GpuBuffer GpuBuffer::create(const Engine& engine, const vk::DeviceSize size, const vk::BufferUsageFlags usage, const MemoryType memory_type) {
    debug_assert(engine.allocator != nullptr);
    debug_assert(size != 0);
    debug_assert(usage != static_cast<vk::BufferUsageFlags>(0));

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = static_cast<VkBufferUsageFlags>(usage);

    VmaAllocationCreateInfo alloc_info = {};
    if (memory_type == RandomAccess) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    } else if (memory_type == Staging) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    } else if (memory_type == DeviceLocal) {
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_info.flags = 0;
    } else {
        critical_assert(!"Invalid buffer memory type");
    }

    VkBuffer buffer = nullptr;
    VmaAllocation allocation = nullptr;
    const auto buffer_result = vmaCreateBuffer(engine.allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr);
    critical_assert(buffer_result == VK_SUCCESS);
    return {allocation, buffer, memory_type};
}

void GpuBuffer::write(const Engine& engine, const void* data, const vk::DeviceSize size, const vk::DeviceSize offset) const {
    debug_assert(engine.allocator != nullptr);
    debug_assert(allocation != nullptr);
    debug_assert(buffer != nullptr);
    debug_assert(data != nullptr);
    debug_assert(size != 0);
    debug_assert((offset == 0 && memory_type == Staging) || memory_type != Staging);

    if (memory_type == RandomAccess || memory_type == Staging) {
        const auto copy_result = vmaCopyMemoryToAllocation(engine.allocator, data, allocation, offset, size);
        critical_assert(copy_result == VK_SUCCESS);
        return;
    }
    debug_assert(memory_type == DeviceLocal);

    const auto staging_buffer = create(engine, size, vk::BufferUsageFlagBits::eTransferSrc, Staging);
    defer(vmaDestroyBuffer(engine.allocator, staging_buffer.buffer, staging_buffer.allocation));
    const auto copy_result = vmaCopyMemoryToAllocation(engine.allocator, data, staging_buffer.allocation, 0, size);
    critical_assert(copy_result == VK_SUCCESS);

    const auto cmd = begin_single_time_commands(engine);
    cmd.copyBuffer(staging_buffer.buffer, buffer, {vk::BufferCopy(offset, 0, size)});
    end_single_time_commands(engine, cmd);
}

GpuImage GpuImage::create(const Engine& engine, const Config& config) {
    debug_assert(engine.device != nullptr);
    debug_assert(engine.allocator != nullptr);
    debug_assert(config.extent.width > 0);
    debug_assert(config.extent.height > 0);
    debug_assert(config.extent.depth > 0);
    debug_assert(config.format != vk::Format::eUndefined);
    debug_assert(config.usage != static_cast<vk::ImageUsageFlags>(0));
    debug_assert(config.aspect_flags != static_cast<vk::ImageAspectFlagBits>(0));
    debug_assert(config.sample_count != static_cast<vk::SampleCountFlagBits>(0));
    debug_assert(config.mip_levels > 0);

    VkImageType dimensions = VK_IMAGE_TYPE_3D;
    if (config.extent.depth == 1) {
        dimensions = static_cast<VkImageType>(dimensions - 1);
        if (config.extent.height == 1) {
            dimensions = static_cast<VkImageType>(dimensions - 1);
        }
    }
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = dimensions;
    image_info.format = static_cast<VkFormat>(config.format);
    image_info.extent = config.extent;
    image_info.mipLevels = config.mip_levels;
    image_info.arrayLayers = 1;
    image_info.samples = static_cast<VkSampleCountFlagBits>(config.sample_count);
    image_info.usage = static_cast<VkImageUsageFlags>(config.usage);

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = 0;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    const auto image_result = vmaCreateImage(engine.allocator, &image_info, &alloc_info, &image, &allocation, nullptr);
    critical_assert(image_result == VK_SUCCESS);

    const vk::ImageViewCreateInfo view_info = {
        .image = image,
        .viewType = static_cast<vk::ImageViewType>(dimensions),
        .format = config.format,
        .subresourceRange = {config.aspect_flags, 0, config.mip_levels, 0, 1},
    };
    const auto view = engine.device.createImageView(view_info);
    critical_assert(view.result == vk::Result::eSuccess);

    if (config.layout != vk::ImageLayout::eUndefined) {
        const auto cmd = begin_single_time_commands(engine);
        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, config.layout)
            .build_and_run();
        end_single_time_commands(engine, cmd);
    }

    return {allocation, image, view.value};
}

void GpuImage::write(const Engine& engine, const void* data, const vk::Extent3D extent, const u32 pixel_alignment, const vk::ImageLayout final_layout,
                     const vk::ImageSubresourceRange& subresource) const {
    debug_assert(engine.allocator != nullptr);
    debug_assert(allocation != nullptr);
    debug_assert(image != nullptr);
    debug_assert(view != nullptr);
    debug_assert(data != nullptr);
    debug_assert(extent.width > 0);
    debug_assert(extent.height > 0);
    debug_assert(extent.depth > 0);

    const VkDeviceSize size = static_cast<u64>(extent.width) * static_cast<u64>(extent.height) * static_cast<u64>(extent.depth) * static_cast<u64>(pixel_alignment);

    const auto staging_buffer = GpuBuffer::create(engine, size, vk::BufferUsageFlagBits::eTransferSrc, GpuBuffer::MemoryType::Staging);
    defer(vmaDestroyBuffer(engine.allocator, staging_buffer.buffer, staging_buffer.allocation));
    staging_buffer.write(engine, data, size, 0);

    const auto cmd = begin_single_time_commands(engine);

    BarrierBuilder(cmd)
        .add_image_barrier(image, subresource)
        .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
        .build_and_run();

    const vk::BufferImageCopy2 copy_region = {.imageSubresource = {subresource.aspectMask, 0, 0, 1}, .imageExtent = extent};
    cmd.copyBufferToImage2(
        {.srcBuffer = staging_buffer.buffer, .dstImage = image, .dstImageLayout = vk::ImageLayout::eTransferDstOptimal, .regionCount = 1, .pRegions = &copy_region});

    BarrierBuilder(cmd)
        .add_image_barrier(image, subresource)
        .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
        .set_image_dst(vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead, final_layout)
        .build_and_run();

    end_single_time_commands(engine, cmd);
}

void GpuImage::generate_mipmaps(const Engine& engine, const u32 mip_levels, const vk::Extent3D extent, const vk::Format format, const vk::ImageLayout final_layout) const {
    debug_assert(image != nullptr);
    debug_assert(extent.width > 0);
    debug_assert(extent.height > 0);
    debug_assert(extent.depth > 0);
    debug_assert(format != vk::Format::eUndefined);
    debug_assert(final_layout != vk::ImageLayout::eUndefined);

    debug_assert(engine.gpu != nullptr);
    const auto format_properties = engine.gpu.getFormatProperties(format);
    critical_assert(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear);

    const auto cmd = begin_single_time_commands(engine);

    vk::Offset3D mip_offset = {static_cast<i32>(extent.width), static_cast<i32>(extent.height), static_cast<i32>(extent.depth)};

    BarrierBuilder(cmd)
        .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
        .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
        .build_and_run();

    for (u32 level = 0; level < mip_levels - 1; ++level) {
        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, level + 1, 1, 0, 1})
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run();

        vk::ImageBlit2 region = {
            .srcSubresource = {vk::ImageAspectFlagBits::eColor, level, 0, 1},
            .dstSubresource = {vk::ImageAspectFlagBits::eColor, level + 1, 0, 1},
        };
        region.srcOffsets[1] = mip_offset;
        if (mip_offset.x > 1) {
            mip_offset.x /= 2;
        }
        if (mip_offset.y > 1) {
            mip_offset.y /= 2;
        }
        if (mip_offset.z > 1) {
            mip_offset.z /= 2;
        }
        region.dstOffsets[1] = mip_offset;
        cmd.blitImage2({
            .srcImage = image,
            .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
            .dstImage = image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions = &region,
            .filter = vk::Filter::eLinear,
        });

        BarrierBuilder(cmd)
            .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, level + 1, 1, 0, 1})
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
            .build_and_run();
    }

    BarrierBuilder(cmd)
        .add_image_barrier(image, {vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1})
        .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
        .set_image_dst(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, final_layout)
        .build_and_run();

    end_single_time_commands(engine, cmd);
}

void allocate_descriptor_sets(const Engine& engine, const vk::DescriptorPool pool, const std::span<const vk::DescriptorSetLayout> layouts,
                              const std::span<vk::DescriptorSet> sets) {
    debug_assert(pool != nullptr);
    debug_assert(!layouts.empty());
    debug_assert(layouts[0] != nullptr);
    debug_assert(!sets.empty());
    debug_assert(sets[0] == nullptr);
    debug_assert(layouts.size() == sets.size());

    const vk::DescriptorSetAllocateInfo alloc_info = {
        .descriptorPool = pool,
        .descriptorSetCount = static_cast<u32>(layouts.size()),
        .pSetLayouts = layouts.data(),
    };
    debug_assert(engine.device != nullptr);
    const auto set_result = engine.device.allocateDescriptorSets(&alloc_info, sets.data());
    critical_assert(set_result == vk::Result::eSuccess);
}

void write_uniform_buffer_descriptor(const Engine& engine, const vk::DescriptorSet set, const u32 binding, const vk::Buffer buffer, const vk::DeviceSize size,
                                     const vk::DeviceSize offset) {
    debug_assert(set != nullptr);
    debug_assert(buffer != nullptr);
    debug_assert(size != 0);

    const vk::DescriptorBufferInfo buffer_info = {buffer, offset, size};
    const vk::WriteDescriptorSet descriptor_write = {
        .dstSet = set,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &buffer_info,
    };
    debug_assert(engine.device != nullptr);
    engine.device.updateDescriptorSets({descriptor_write}, {});
}

void write_image_sampler_descriptor(const Engine& engine, const vk::DescriptorSet set, const u32 binding, const vk::Sampler sampler, const vk::ImageView view) {
    debug_assert(set != nullptr);
    debug_assert(sampler != nullptr);
    debug_assert(view != nullptr);

    const vk::DescriptorImageInfo image_info = {sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal};
    const vk::WriteDescriptorSet descriptor_write = {
        .dstSet = set,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &image_info,
    };
    debug_assert(engine.device != nullptr);
    engine.device.updateDescriptorSets({descriptor_write}, {});
}

vk::Sampler create_sampler(const Engine& engine, const SamplerConfig& config) {
    debug_assert(engine.gpu != nullptr);
    const auto limits = engine.gpu.getProperties().limits;

    vk::SamplerCreateInfo sampler_info = {
        .addressModeU = config.edge_mode,
        .addressModeV = config.edge_mode,
        .addressModeW = config.edge_mode,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = limits.maxSamplerAnisotropy,
        .maxLod = static_cast<f32>(config.mip_levels),
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
    };
    if (config.type == SamplerType::Linear) {
        sampler_info.magFilter = vk::Filter::eLinear;
        sampler_info.minFilter = vk::Filter::eLinear;
        sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
    } else if (config.type == SamplerType::Nearest) {
        sampler_info.magFilter = vk::Filter::eNearest;
        sampler_info.minFilter = vk::Filter::eNearest;
        sampler_info.mipmapMode = vk::SamplerMipmapMode::eNearest;
    } else {
        critical_assert(!"Invalid sampler type");
    }

    debug_assert(engine.device != nullptr);
    const auto sampler = engine.device.createSampler(sampler_info);
    critical_assert(sampler.result == vk::Result::eSuccess);
    return sampler.value;
}

Pipeline GraphicsPipelineBuilder::build(const Engine& engine) const {
    debug_assert(engine.device != nullptr);
    debug_assert(!m_vertex_shader.empty());
    debug_assert(!m_fragment_shader.empty());
    debug_assert(m_MSAA != static_cast<vk::SampleCountFlagBits>(0));

    std::vector<vk::DescriptorSetLayout> descriptor_layouts = {};
    descriptor_layouts.reserve(m_descriptor_sets.size());
    for (const auto& bindings : m_descriptor_sets) {
        const auto layout = engine.device.createDescriptorSetLayout({.bindingCount = static_cast<u32>(bindings.size()), .pBindings = bindings.data()});
        critical_assert(layout.result == vk::Result::eSuccess);
        descriptor_layouts.push_back(layout.value);
    }

    const vk::PipelineLayoutCreateInfo layout_info = {
        .setLayoutCount = static_cast<u32>(descriptor_layouts.size()),
        .pSetLayouts = descriptor_layouts.data(),
        .pushConstantRangeCount = static_cast<u32>(m_push_constants.size()),
        .pPushConstantRanges = m_push_constants.data(),
    };
    const auto layout = engine.device.createPipelineLayout(layout_info);
    critical_assert(layout.result == vk::Result::eSuccess);

    const auto create_shader = [&](const std::string_view path) -> vk::ShaderModule {
        auto file = std::ifstream{path.data(), std::ios::ate | std::ios::binary};
        critical_assert(file.is_open());

        const usize code_size = file.tellg();
        std::vector<char> code;
        code.resize(code_size);
        file.seekg(0);
        file.read(code.data(), static_cast<std::streamsize>(code.size()));
        file.close();

        const auto shader = engine.device.createShaderModule({.codeSize = code.size(), .pCode = reinterpret_cast<u32*>(code.data())});
        critical_assert(shader.result == vk::Result::eSuccess);
        return shader.value;
    };
    const std::array shader_stage_infos = {
        vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eVertex, .module = create_shader(m_vertex_shader), .pName = "main"},
        vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eFragment, .module = create_shader(m_fragment_shader), .pName = "main"},
    };
    defer({
        for (const auto& shader : shader_stage_infos) {
            engine.device.destroyShaderModule(shader.module);
        }
    });

    std::vector<vk::VertexInputBindingDescription> vertex_bindings = {};
    vertex_bindings.reserve(m_vertex_bindings.size());
    std::vector<vk::VertexInputAttributeDescription> vertex_attributes = {};
    vertex_attributes.reserve(m_vertex_bindings.size() * 2);
    for (usize binding = 0; binding < m_vertex_bindings.size(); ++binding) {
        const auto& [attributes, stride, input_rate] = m_vertex_bindings[binding];
        vertex_bindings.emplace_back(binding, stride, input_rate);
        for (usize location = 0; location < attributes.size(); ++location) {
            const auto& [format, offset] = attributes[location];
            vertex_attributes.emplace_back(location, binding, format, offset);
        }
    }
    const vk::PipelineVertexInputStateCreateInfo vertex_input_info = {
        .vertexBindingDescriptionCount = static_cast<u32>(vertex_bindings.size()),
        .pVertexBindingDescriptions = vertex_bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<u32>(vertex_attributes.size()),
        .pVertexAttributeDescriptions = vertex_attributes.data(),
    };
    const vk::PipelineInputAssemblyStateCreateInfo input_assembly_info = {.topology = m_topology};
    constexpr vk::PipelineTessellationStateCreateInfo tessellation_info = {};
    constexpr vk::PipelineViewportStateCreateInfo viewport_info = {.viewportCount = 1, .scissorCount = 1};
    const vk::PipelineRasterizationStateCreateInfo rasterization_info = {
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = m_cull_mode,
        .lineWidth = 1.0f,
    };
    vk::PipelineMultisampleStateCreateInfo multisample_info = {};
    multisample_info.rasterizationSamples = m_MSAA;
    if (m_MSAA > vk::SampleCountFlagBits::e1) {
        multisample_info.sampleShadingEnable = vk::True;
        multisample_info.minSampleShading = 0.2f;
    }
    vk::PipelineDepthStencilStateCreateInfo depth_stencil_info = {};
    if (m_depth_buffer) {
        depth_stencil_info.depthTestEnable = vk::True;
        depth_stencil_info.depthWriteEnable = vk::True;
        depth_stencil_info.depthCompareOp = vk::CompareOp::eLess;
        depth_stencil_info.minDepthBounds = 0.0f;
        depth_stencil_info.maxDepthBounds = 1.0f;
    }

    vk::PipelineColorBlendAttachmentState color_blend_attachment = {.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    if (m_color_blend) {
        color_blend_attachment.blendEnable = vk::True;
        color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
        color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;
    }
    const vk::PipelineColorBlendStateCreateInfo color_blend_info = {
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = std::array{1.0f, 1.0f, 1.0f, 1.0f}
    };

    constexpr std::array dynamic_state = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    const vk::PipelineDynamicStateCreateInfo dynamic_state_info = {.dynamicStateCount = static_cast<u32>(dynamic_state.size()), .pDynamicStates = dynamic_state.data()};

    const vk::PipelineRenderingCreateInfo dynamic_rendering_info = {
        .colorAttachmentCount = static_cast<u32>(m_color_formats.size()),
        .pColorAttachmentFormats = m_color_formats.data(),
        .depthAttachmentFormat = m_depth_format,
        .stencilAttachmentFormat = m_stencil_format,
    };
    const vk::GraphicsPipelineCreateInfo pipeline_info = {
        .pNext = &dynamic_rendering_info,
        .stageCount = static_cast<u32>(shader_stage_infos.size()),
        .pStages = shader_stage_infos.data(),
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pTessellationState = &tessellation_info,
        .pViewportState = &viewport_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState = &multisample_info,
        .pDepthStencilState = &depth_stencil_info,
        .pColorBlendState = &color_blend_info,
        .pDynamicState = &dynamic_state_info,
        .layout = layout.value,
        .basePipelineIndex = -1,
    };
    const auto pipeline = engine.device.createGraphicsPipeline(m_cache, pipeline_info);
    critical_assert(pipeline.result == vk::Result::eSuccess);

    return {descriptor_layouts, layout.value, pipeline.value};
}

} // namespace hg
