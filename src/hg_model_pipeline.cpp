#include "hg_model_pipeline.h"

#include "hg_load.h"

namespace hg {

ModelPipeline ModelPipeline::create(const Engine& engine, const Window& window) {
    debug_assert(engine.device != nullptr);
    debug_assert(window.window != nullptr);

    const auto color_image = GpuImage::create(engine, {.extent = {window.extent.width, window.extent.height, 1},
                                                       .format = window.image_format,
                                                       .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                                                       .sample_count = vk::SampleCountFlagBits::e4});

    const auto depth_image = GpuImage::create(engine, {.extent = {window.extent.width, window.extent.height, 1},
                                                       .format = vk::Format::eD32Sfloat,
                                                       .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                       .aspect_flags = vk::ImageAspectFlagBits::eDepth,
                                                       .sample_count = vk::SampleCountFlagBits::e4});

    const auto render_pipeline =
        GraphicsPipelineBuilder()
            .set_shaders("../shaders/model.vert.spv", "../shaders/model.frag.spv")
            .set_render_target(std::array{window.image_format}, vk::Format::eD32Sfloat)
            .add_descriptor_set_layout(std::array{vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}})
            .add_descriptor_set_layout(std::array{vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}})
            .add_push_constant_range(vk::ShaderStageFlagBits::eVertex, sizeof(ModelPushConstant))
            .add_vertex_binding(std::array{VertexAttribute{vk::Format::eR32G32B32Sfloat, offsetof(ModelVertex, position)},
                                           VertexAttribute{vk::Format::eR32G32B32Sfloat, offsetof(ModelVertex, normal)},
                                           VertexAttribute{vk::Format::eR32G32Sfloat, offsetof(ModelVertex, uv)}},
                                sizeof(ModelVertex))
            .set_MSAA(vk::SampleCountFlagBits::e4)
            .enable_depth_buffer()
            .enable_color_blend()
            .enable_culling()
            .build(engine);

    constexpr std::array pool_sizes = {
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 1},
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 255},
    };
    const auto pool = engine.device.createDescriptorPool({.maxSets = 256, .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()), .pPoolSizes = pool_sizes.data()});
    critical_assert(pool.result == vk::Result::eSuccess);

    const auto vp_set = allocate_descriptor_set(engine, pool.value, render_pipeline.descriptor_layouts[0]);
    const auto vp_buffer = GpuBuffer::create(engine, sizeof(ViewProjectionUniform), vk::BufferUsageFlagBits::eUniformBuffer, GpuBuffer::RandomAccess);
    write_uniform_buffer_descriptor(engine, vp_set, 0, vp_buffer.buffer, sizeof(ViewProjectionUniform));

    debug_assert(color_image.image != nullptr);
    debug_assert(depth_image.image != nullptr);
    debug_assert(render_pipeline.pipeline != nullptr);
    debug_assert(pool.value != nullptr);
    debug_assert(vp_set != nullptr);
    return {color_image, depth_image, render_pipeline, pool.value, vp_set, vp_buffer};
}

void ModelPipeline::destroy(const Engine& engine) const {
    debug_assert(engine.device != nullptr);
    for (const auto& texture : m_textures) {
        texture.destroy(engine);
    }
    for (const auto& model : m_models) {
        model.destroy(engine);
    }
    debug_assert(m_vp_buffer.buffer != nullptr);
    m_vp_buffer.destroy(engine);
    debug_assert(m_descriptor_pool != nullptr);
    engine.device.destroyDescriptorPool(m_descriptor_pool);
    debug_assert(m_render_pipeline.pipeline != nullptr);
    m_render_pipeline.destroy(engine);
    debug_assert(m_depth_image.image != nullptr);
    m_depth_image.destroy(engine);
    debug_assert(m_color_image.image != nullptr);
    m_color_image.destroy(engine);
}

void ModelPipeline::resize(const Engine& engine, const Window& window) {
    debug_assert(m_color_image.image != nullptr);
    debug_assert(m_color_image.image != nullptr);
    debug_assert(engine.device != nullptr);
    debug_assert(window.image_format != vk::Format::eUndefined);
    debug_assert(window.extent.width > 0);
    debug_assert(window.extent.height > 0);

    m_color_image.destroy(engine);
    m_color_image = GpuImage::create(engine, {.extent = {window.extent.width, window.extent.height, 1},
                                              .format = window.image_format,
                                              .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                                              .sample_count = vk::SampleCountFlagBits::e4});

    m_depth_image.destroy(engine);
    m_depth_image = GpuImage::create(engine, {.extent = {window.extent.width, window.extent.height, 1},
                                              .format = vk::Format::eD32Sfloat,
                                              .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                              .aspect_flags = vk::ImageAspectFlagBits::eDepth,
                                              .sample_count = vk::SampleCountFlagBits::e4});

    debug_assert(m_color_image.image != nullptr);
    debug_assert(m_color_image.image != nullptr);
}

void ModelPipeline::render(const vk::CommandBuffer cmd, Window& window) {
    debug_assert(cmd != nullptr);
    debug_assert(window.current_image() != nullptr);
    debug_assert(window.current_view() != nullptr);

    const vk::Viewport viewport = {0.0f, 0.0f, static_cast<f32>(window.extent.width), static_cast<f32>(window.extent.height), 0.0f, 1.0f};
    cmd.setViewport(0, {viewport});
    const vk::Rect2D scissor = {{0, 0}, window.extent};
    cmd.setScissor(0, {scissor});

    BarrierBuilder(cmd)
        .add_image_barrier(m_color_image.image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
        .set_image_dst(vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal)
        .add_image_barrier(m_depth_image.image, {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1})
        .set_image_dst(vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                       vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .build_and_run();

    const vk::RenderingAttachmentInfo color_attachment = {
        .imageView = m_color_image.view,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {{std::array{0.0f, 0.0f, 0.0f, 1.0f}}},
    };
    const vk::RenderingAttachmentInfo depth_attachment = {
        .imageView = m_depth_image.view,
        .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = {.depthStencil = {.depth = 1.0f, .stencil = 0}},
    };
    cmd.beginRendering({
        .renderArea = {{0, 0}, window.extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = &depth_attachment,
    });

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_render_pipeline.pipeline);

    std::array<vk::DescriptorSet, 2> sets = {m_vp_set, nullptr};
    for (const auto& ticket : m_render_queue) {
        const auto& model = m_models[ticket.model_index];
        sets[1] = m_textures[model.texture_index].set;

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_render_pipeline.layout, 0, sets, {});

        ModelPushConstant model_push = {ticket.transform.matrix()};
        cmd.pushConstants(m_render_pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(model_push), &model_push);

        std::array vertex_buffers = {model.vertex_buffer.buffer};
        std::array offsets = {vk::DeviceSize{0}};
        cmd.bindVertexBuffers(0, vertex_buffers, offsets);
        cmd.bindIndexBuffer(model.index_buffer.buffer, 0, vk::IndexType::eUint32);
        cmd.drawIndexed(model.index_count, 1, 0, 0, 1);
    }

    cmd.endRendering();

    BarrierBuilder(cmd)
        .add_image_barrier(m_color_image.image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
        .set_image_src(vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal)
        .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
        .add_image_barrier(window.current_image(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
        .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
        .build_and_run();

    const vk::ImageResolve2 resolve = {
        .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .extent = vk::Extent3D{window.extent.width, window.extent.height, 1},
    };
    cmd.resolveImage2({
        .srcImage = m_color_image.image,
        .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
        .dstImage = window.current_image(),
        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
        .regionCount = 1,
        .pRegions = &resolve,
    });

    BarrierBuilder(cmd)
        .add_image_barrier(window.current_image(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
        .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
        .set_image_dst(vk::PipelineStageFlagBits2::eAllGraphics, vk::AccessFlagBits2::eNone, vk::ImageLayout::ePresentSrcKHR)
        .build_and_run();

    m_render_queue.clear();
}

void ModelPipeline::load_texture(const Engine& engine, const std::filesystem::path path) {
    const auto texture_result = ImageData::load(path);
    critical_assert(texture_result.has_value());
    const auto texture_data = texture_result.value();
    defer(texture_data.unload());

    const vk::Extent3D extent = {static_cast<u32>(texture_data.width), static_cast<u32>(texture_data.height), 1};
    const u32 mips = get_mip_count(extent);

    const auto image = GpuImage::create(engine, {.extent = extent,
                                                 .format = vk::Format::eR8G8B8A8Srgb,
                                                 .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
                                                 .mip_levels = mips});

    image.write(engine, texture_data.pixels, extent, 4, vk::ImageLayout::eShaderReadOnlyOptimal);
    image.generate_mipmaps(engine, mips, extent, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eShaderReadOnlyOptimal);

    const auto sampler = create_sampler(engine, {.type = SamplerType::Linear, .mip_levels = get_mip_count(extent)});
    const auto set = allocate_descriptor_set(engine, m_descriptor_pool, m_render_pipeline.descriptor_layouts[1]);
    write_image_sampler_descriptor(engine, set, 0, sampler, image.view);

    m_textures.emplace_back(image, sampler, set);
}

void ModelPipeline::load_model(const Engine& engine, const std::filesystem::path path, const usize texture_index) {
    const auto model_result = ModelData::load_gltf(path);
    critical_assert(model_result.has_value());
    const auto model_data = model_result.value();

    const auto index_buffer =
        GpuBuffer::create(engine, model_data.indices.size() * sizeof(model_data.indices[0]), vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    const auto vertex_buffer =
        GpuBuffer::create(engine, model_data.vertices.size() * sizeof(model_data.vertices[0]), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);

    index_buffer.write(engine, model_data.indices.data(), model_data.indices.size() * sizeof(model_data.indices[0]), 0);
    vertex_buffer.write(engine, model_data.vertices.data(), model_data.vertices.size() * sizeof(model_data.vertices[0]), 0);

    m_models.emplace_back(static_cast<u32>(model_data.indices.size()), index_buffer, vertex_buffer, texture_index);
}

} // namespace hg
