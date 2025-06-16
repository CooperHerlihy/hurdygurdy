#include "hg_pbr_pipeline.h"

#include "hg_load.h"
#include "hg_math.h"
#include "hg_mesh.h"
#include "hg_utils.h"
#include "hg_vulkan_engine.h"

#include <filesystem>

namespace hg {

PbrPipeline PbrPipeline::create(const Engine& engine, const Window& window) {
    debug_assert(engine.device != nullptr);
    debug_assert(window.window != nullptr);

    PbrPipeline pipeline;

    pipeline.m_color_image = GpuImage::create(engine, {.extent = {window.extent.width, window.extent.height, 1},
                                                       .format = window.image_format,
                                                       .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                                                       .sample_count = vk::SampleCountFlagBits::e4});

    pipeline.m_depth_image = GpuImage::create(engine, {.extent = {window.extent.width, window.extent.height, 1},
                                                       .format = vk::Format::eD32Sfloat,
                                                       .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                       .aspect_flags = vk::ImageAspectFlagBits::eDepth,
                                                       .sample_count = vk::SampleCountFlagBits::e4});

    pipeline.m_pbr_set_layouts[0] =
        create_set_layout(engine, std::array{vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                                             vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}});
    pipeline.m_pbr_set_layouts[1] =
        create_set_layout(engine, std::array{vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
                                             vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}});

    std::array push_ranges = {vk::PushConstantRange{vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstant)}};
    const auto pbr_layout = engine.device.createPipelineLayout({
        .setLayoutCount = static_cast<u32>(pipeline.m_pbr_set_layouts.size()),
        .pSetLayouts = pipeline.m_pbr_set_layouts.data(),
        .pushConstantRangeCount = static_cast<u32>(push_ranges.size()),
        .pPushConstantRanges = push_ranges.data(),
    });
    critical_assert(pbr_layout.result == vk::Result::eSuccess);
    pipeline.m_pbr_layout = pbr_layout.value;

    create_linked_shaders(engine, pipeline.m_pbr_shaders,
                          std::array{ShaderConfig{.path = "../shaders/pbr.vert.spv",
                                                  .stage = vk::ShaderStageFlagBits::eVertex,
                                                  .next_stage = vk::ShaderStageFlagBits::eFragment,
                                                  .set_layouts = pipeline.m_pbr_set_layouts,
                                                  .push_ranges = push_ranges},
                                     ShaderConfig{.path = "../shaders/pbr.frag.spv",
                                                  .stage = vk::ShaderStageFlagBits::eFragment,
                                                  .next_stage = {},
                                                  .set_layouts = pipeline.m_pbr_set_layouts,
                                                  .push_ranges = push_ranges}});

    pipeline.m_skybox_set_layouts[0] =
        create_set_layout(engine, std::array{vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                                             vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}});
    pipeline.m_skybox_set_layouts[1] =
        create_set_layout(engine, std::array{vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}});

    const auto skybox_layout = engine.device.createPipelineLayout({
        .setLayoutCount = static_cast<u32>(pipeline.m_skybox_set_layouts.size()),
        .pSetLayouts = pipeline.m_skybox_set_layouts.data(),
    });
    critical_assert(skybox_layout.result == vk::Result::eSuccess);
    pipeline.m_skybox_layout = skybox_layout.value;

    create_linked_shaders(engine, pipeline.m_skybox_shaders,
                          std::array{ShaderConfig{.path = "../shaders/skybox.vert.spv",
                                                  .stage = vk::ShaderStageFlagBits::eVertex,
                                                  .next_stage = vk::ShaderStageFlagBits::eFragment,
                                                  .set_layouts = pipeline.m_skybox_set_layouts,
                                                  .push_ranges = {}},
                                     ShaderConfig{.path = "../shaders/skybox.frag.spv",
                                                  .stage = vk::ShaderStageFlagBits::eFragment,
                                                  .next_stage = {},
                                                  .set_layouts = pipeline.m_skybox_set_layouts,
                                                  .push_ranges = {}}});

    constexpr std::array pool_sizes = {
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 256},
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 256},
    };
    const auto pool = engine.device.createDescriptorPool({.maxSets = 256, .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()), .pPoolSizes = pool_sizes.data()});
    critical_assert(pool.result == vk::Result::eSuccess);
    pipeline.m_descriptor_pool = pool.value;

    pipeline.m_global_set = allocate_descriptor_set(engine, pipeline.m_descriptor_pool, pipeline.m_pbr_set_layouts[0]);

    pipeline.m_vp_buffer = GpuBuffer::create(engine, sizeof(ViewProjectionUniform), vk::BufferUsageFlagBits::eUniformBuffer, GpuBuffer::RandomAccess);
    write_uniform_buffer_descriptor(engine, pipeline.m_global_set, 0, pipeline.m_vp_buffer.buffer, sizeof(ViewProjectionUniform));
    pipeline.m_light_buffer = GpuBuffer::create(engine, sizeof(LightUniform) * MaxLights, vk::BufferUsageFlagBits::eUniformBuffer, GpuBuffer::RandomAccess);
    write_uniform_buffer_descriptor(engine, pipeline.m_global_set, 1, pipeline.m_light_buffer.buffer, sizeof(LightUniform));

    return pipeline;
}

void PbrPipeline::destroy(const Engine& engine) const {
    debug_assert(engine.device != nullptr);

    for (const auto& texture : m_textures) {
        texture.destroy(engine);
    }
    for (const auto& model : m_models) {
        model.destroy(engine);
    }

    m_skybox.destroy(engine);

    debug_assert(m_light_buffer.buffer != nullptr);
    m_light_buffer.destroy(engine);
    debug_assert(m_vp_buffer.buffer != nullptr);
    m_vp_buffer.destroy(engine);

    debug_assert(m_descriptor_pool != nullptr);
    engine.device.destroyDescriptorPool(m_descriptor_pool);

    for (const auto& set_layout : m_skybox_set_layouts) {
        debug_assert(set_layout != nullptr);
        engine.device.destroyDescriptorSetLayout(set_layout);
    }
    for (const auto& set_layout : m_pbr_set_layouts) {
        debug_assert(set_layout != nullptr);
        engine.device.destroyDescriptorSetLayout(set_layout);
    }

    debug_assert(m_skybox_layout != nullptr);
    engine.device.destroyPipelineLayout(m_skybox_layout);
    debug_assert(m_pbr_layout != nullptr);
    engine.device.destroyPipelineLayout(m_pbr_layout);

    for (const auto& shader : m_skybox_shaders) {
        debug_assert(shader != nullptr);
        engine.device.destroyShaderEXT(shader);
    }
    for (const auto& shader : m_pbr_shaders) {
        debug_assert(shader != nullptr);
        engine.device.destroyShaderEXT(shader);
    }

    debug_assert(m_depth_image.image != nullptr);
    m_depth_image.destroy(engine);
    debug_assert(m_color_image.image != nullptr);
    m_color_image.destroy(engine);
}

void PbrPipeline::resize(const Engine& engine, const Window& window) {
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

void PbrPipeline::render(const vk::CommandBuffer cmd, const Engine& engine, Window& window, const Cameraf& camera) {
    debug_assert(cmd != nullptr);
    debug_assert(window.current_image() != nullptr);
    debug_assert(window.current_view() != nullptr);

    const glm::mat4 view = camera.view();
    m_vp_buffer.write(engine, view, offsetof(ViewProjectionUniform, view));

    LightUniform lights = {
        .vals = {},
        .count = m_lights.size(),
    };
    for (usize i = 0; i < m_lights.size(); ++i) {
        lights.vals[i] = {
            .position = view * m_lights[i].position,
            .color = m_lights[i].color,
        };
    }
    m_light_buffer.write(engine, lights);
    m_lights.clear();

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

    std::array<vk::DescriptorSet, 2> sets = {m_global_set, nullptr};

    cmd.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e4);
    cmd.setSampleMaskEXT(vk::SampleCountFlagBits::e4, vk::SampleMask{0xff});

    cmd.setVertexInputEXT({vk::VertexInputBindingDescription2EXT{.stride = sizeof(glm::vec3), .inputRate = vk::VertexInputRate::eVertex, .divisor = 1}},
                          {vk::VertexInputAttributeDescription2EXT{.location = 0, .format = vk::Format::eR32G32B32Sfloat}});

    cmd.setDepthTestEnable(vk::False);
    cmd.setDepthWriteEnable(vk::False);
    cmd.setCullMode(vk::CullModeFlagBits::eFront);
    cmd.bindShadersEXT({vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment}, m_skybox_shaders);

    sets[1] = m_skybox.set;
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_skybox_layout, 0, sets, {});
    cmd.bindIndexBuffer(m_skybox.index_buffer.buffer, 0, vk::IndexType::eUint32);
    cmd.bindVertexBuffers(0, {m_skybox.vertex_buffer.buffer}, {vk::DeviceSize{0}});
    cmd.drawIndexed(m_skybox.IndexCount, 1, 0, 0, 1);

    cmd.setVertexInputEXT({vk::VertexInputBindingDescription2EXT{.stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex, .divisor = 1}},
                          {vk::VertexInputAttributeDescription2EXT{.location = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, position)},
                           vk::VertexInputAttributeDescription2EXT{.location = 1, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, normal)},
                           vk::VertexInputAttributeDescription2EXT{.location = 2, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, tex_coord)}});

    cmd.setDepthTestEnable(vk::True);
    cmd.setDepthWriteEnable(vk::True);
    cmd.setCullMode(vk::CullModeFlagBits::eBack);
    cmd.bindShadersEXT({vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment}, m_pbr_shaders);

    for (const auto& ticket : m_render_queue) {
        const auto& model = m_models[ticket.model_index];
        sets[1] = model.set;

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pbr_layout, 0, sets, {});
        PushConstant model_push = {ticket.transform.matrix()};
        cmd.pushConstants(m_pbr_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(model_push), &model_push);

        cmd.bindIndexBuffer(model.index_buffer.buffer, 0, vk::IndexType::eUint32);
        cmd.bindVertexBuffers(0, {model.vertex_buffer.buffer}, {vk::DeviceSize{0}});
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

void PbrPipeline::load_texture(const Engine& engine, const std::filesystem::path path) {
    debug_assert(!path.empty());

    const auto texture_data = ImageData::load(path);
    critical_assert(texture_data.has_value());
    defer(texture_data->unload());

    load_texture_from_data(engine, texture_data->pixels, {static_cast<u32>(texture_data->width), static_cast<u32>(texture_data->height), 1}, vk::Format::eR8G8B8A8Srgb, 4);
}

void PbrPipeline::load_texture_from_data(const Engine& engine, const void* data, const vk::Extent3D extent, const vk::Format format, const u32 pixel_alignment) {
    debug_assert(data != nullptr);
    debug_assert(extent.width > 0);
    debug_assert(extent.height > 0);
    debug_assert(extent.depth > 0);
    debug_assert(format != vk::Format::eUndefined);
    debug_assert(pixel_alignment > 0);

    const u32 mips = get_mip_count(extent);

    const auto image = GpuImage::create(engine, {.extent = extent,
                                                 .format = format,
                                                 .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
                                                 .mip_levels = mips});

    image.write(engine, data, extent, pixel_alignment, vk::ImageLayout::eShaderReadOnlyOptimal);
    image.generate_mipmaps(engine, mips, extent, format, vk::ImageLayout::eShaderReadOnlyOptimal);

    const auto sampler = create_sampler(engine, {.type = SamplerType::Linear, .mip_levels = get_mip_count(extent)});

    m_textures.emplace_back(image, sampler);
}

void PbrPipeline::load_model(const Engine& engine, const std::filesystem::path path, const usize texture_index) {
    debug_assert(!path.empty());
    debug_assert(texture_index < m_textures.size());

    const auto model_result = ModelData::load_gltf(path);
    critical_assert(model_result.has_value());
    const auto model_data = model_result.value();

    const auto vertex_data = VertexData::from_mesh(std::move(model_data.mesh));
    load_model_from_data(engine, vertex_data.indices, vertex_data.vertices, texture_index, model_data.roughness, model_data.metalness);
}

void PbrPipeline::load_model_from_data(const Engine& engine, const std::span<const u32> indices, const std::span<const Vertex> vertices, const usize texture_index, float roughness,
                                       float metalness) {
    debug_assert(!indices.empty());
    debug_assert(!vertices.empty());
    debug_assert(texture_index < m_textures.size());
    debug_assert(roughness >= 0.0 && roughness <= 1.0);
    debug_assert(metalness >= 0.0 && metalness <= 1.0);

    const auto index_buffer = GpuBuffer::create(engine, indices.size() * sizeof(indices[0]), vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    const auto vertex_buffer = GpuBuffer::create(engine, vertices.size() * sizeof(vertices[0]), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    const auto material_buffer = GpuBuffer::create(engine, sizeof(MaterialUniform), vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst);

    index_buffer.write(engine, indices.data(), indices.size() * sizeof(indices[0]), 0);
    vertex_buffer.write(engine, vertices.data(), vertices.size() * sizeof(vertices[0]), 0);
    material_buffer.write(engine, MaterialUniform{roughness, metalness});

    const auto set = hg::allocate_descriptor_set(engine, m_descriptor_pool, m_pbr_set_layouts[1]);
    hg::write_image_sampler_descriptor(engine, set, 0, m_textures[texture_index].sampler, m_textures[texture_index].image.view);
    hg::write_uniform_buffer_descriptor(engine, set, 1, material_buffer.buffer, sizeof(MaterialUniform));

    m_models.emplace_back(static_cast<u32>(indices.size()), index_buffer, vertex_buffer, material_buffer, set);
}

void PbrPipeline::load_skybox(const Engine& engine, const std::filesystem::path path) {
    m_skybox.cubemap = GpuImage::create_cubemap(engine, path);

    m_skybox.sampler = create_sampler(engine, {.type = SamplerType::Linear});

    const auto mesh = generate_cube();

    m_skybox.index_buffer = GpuBuffer::create(engine, mesh.indices.size() * sizeof(mesh.indices[0]), vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    m_skybox.vertex_buffer =
        GpuBuffer::create(engine, mesh.positions.size() * sizeof(mesh.positions[0]), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);

    m_skybox.index_buffer.write(engine, mesh.indices.data(), mesh.indices.size() * sizeof(mesh.indices[0]), 0);
    m_skybox.vertex_buffer.write(engine, mesh.positions.data(), mesh.positions.size() * sizeof(mesh.positions[0]), 0);

    m_skybox.set = hg::allocate_descriptor_set(engine, m_descriptor_pool, m_skybox_set_layouts[1]);
    hg::write_image_sampler_descriptor(engine, m_skybox.set, 0, m_skybox.sampler, m_skybox.cubemap.view);
}

PbrPipeline::VertexData PbrPipeline::VertexData::from_mesh(const Mesh& mesh) {
    debug_assert(mesh.indices.size() > 0);
    debug_assert(mesh.positions.size() > 0);
    debug_assert(mesh.normals.size() > 0);
    debug_assert(mesh.tex_coords.size() > 0);

    VertexData data = {};
    data.indices = std::move(mesh.indices);
    data.vertices.reserve(mesh.positions.size());
    for (usize i = 0; i < mesh.positions.size(); ++i) {
        data.vertices.emplace_back(mesh.positions[i], mesh.normals[i], mesh.tex_coords[i]);
    }

    return data;
}

} // namespace hg
