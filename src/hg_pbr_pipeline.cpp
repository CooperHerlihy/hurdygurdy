#include "hg_pbr_pipeline.h"

#include "hg_utils.h"
#include "hg_math.h"
#include "hg_mesh.h"
#include "hg_load.h"
#include "hg_vulkan_engine.h"

#include <concepts>
#include <filesystem>

namespace hg {

Result<DefaultPipeline> DefaultPipeline::create(const Engine& engine, const vk::Extent2D window_size, const vk::DescriptorPool descriptor_pool) {
    ASSERT(window_size.width != 0);
    ASSERT(window_size.height != 0);
    ASSERT(descriptor_pool != nullptr);

    auto pipeline = ok<DefaultPipeline>();

    pipeline->m_color_image = GpuImage::create(engine, {
        .extent = {window_size.width, window_size.height, 1},
        .format = Window::SwapchainImageFormat,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        .sample_count = vk::SampleCountFlagBits::e4,
    });
    pipeline->m_depth_image = GpuImage::create(engine, {
        .extent = {window_size.width, window_size.height, 1},
        .format = vk::Format::eD32Sfloat,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .aspect_flags = vk::ImageAspectFlagBits::eDepth,
        .sample_count = vk::SampleCountFlagBits::e4,
    });

    const auto set_layout = create_descriptor_set_layout(engine, std::array{
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
    });
    if (set_layout.has_err())
        return set_layout.err();
    pipeline->m_set_layout = *set_layout;

    const auto global_set = allocate_descriptor_set(engine, descriptor_pool, pipeline->m_set_layout);
    if (global_set.has_err())
        return global_set.err();
    pipeline->m_global_set = *global_set;

    pipeline->m_vp_buffer = GpuBuffer::create(engine, {
        sizeof(ViewProjectionUniform),
        vk::BufferUsageFlagBits::eUniformBuffer, GpuBuffer::RandomAccess
    });
    pipeline->m_light_buffer = GpuBuffer::create(engine, {
        sizeof(LightUniform) * MaxLights,
        vk::BufferUsageFlagBits::eUniformBuffer, GpuBuffer::RandomAccess
    });

    write_uniform_buffer_descriptor(engine, pipeline->m_global_set, 0, pipeline->m_vp_buffer.buffer, sizeof(ViewProjectionUniform));
    write_uniform_buffer_descriptor(engine, pipeline->m_global_set, 1, pipeline->m_light_buffer.buffer, sizeof(LightUniform));

    ASSERT(pipeline->m_color_image.allocation != nullptr);
    ASSERT(pipeline->m_color_image.image != nullptr);
    ASSERT(pipeline->m_color_image.view != nullptr);
    ASSERT(pipeline->m_depth_image.allocation != nullptr);
    ASSERT(pipeline->m_depth_image.image != nullptr);
    ASSERT(pipeline->m_depth_image.view != nullptr);
    ASSERT(pipeline->m_set_layout != nullptr);
    ASSERT(pipeline->m_global_set != nullptr);
    ASSERT(pipeline->m_vp_buffer.allocation != nullptr);
    ASSERT(pipeline->m_vp_buffer.buffer != nullptr);
    ASSERT(pipeline->m_light_buffer.allocation != nullptr);
    ASSERT(pipeline->m_light_buffer.buffer != nullptr);
    return pipeline;
}

void DefaultPipeline::destroy(const Engine& engine) const {
    ASSERT(engine.device != nullptr);

    ASSERT(m_set_layout != nullptr);
    engine.device.destroyDescriptorSetLayout(m_set_layout);
    ASSERT(m_light_buffer.buffer != nullptr);
    m_light_buffer.destroy(engine);
    ASSERT(m_vp_buffer.buffer != nullptr);
    m_vp_buffer.destroy(engine);

    m_depth_image.destroy(engine);
    m_color_image.destroy(engine);
}

void DefaultPipeline::resize(const Engine& engine, const vk::Extent2D window_size) {
    ASSERT(m_color_image.image != nullptr);
    ASSERT(m_depth_image.image != nullptr);
    ASSERT(window_size.width > 0);
    ASSERT(window_size.height > 0);

    m_color_image.destroy(engine);
    m_color_image = GpuImage::create(engine, {
        .extent = {window_size.width, window_size.height, 1},
        .format = Window::SwapchainImageFormat,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        .sample_count = vk::SampleCountFlagBits::e4,
    });

    m_depth_image.destroy(engine);
    m_depth_image = GpuImage::create(engine, {
        .extent = {window_size.width, window_size.height, 1},
        .format = vk::Format::eD32Sfloat,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .aspect_flags = vk::ImageAspectFlagBits::eDepth,
        .sample_count = vk::SampleCountFlagBits::e4,
    });

    ASSERT(m_color_image.image != nullptr);
    ASSERT(m_depth_image.image != nullptr);
}

void DefaultPipeline::update_camera(const Engine& engine, const Cameraf& camera) {
    const glm::mat4 view = camera.view();

    ASSERT(m_lights.size() < MaxLights);
    LightUniform lights = {.count = m_lights.size()};
    for (usize i = 0; i < m_lights.size(); ++i) {
        lights.vals[i] = {
            .position = view * m_lights[i].position,
            .color = m_lights[i].color,
        };
    }

    m_light_buffer.write(engine, lights);
    m_vp_buffer.write(engine, view, offsetof(ViewProjectionUniform, view));
}

void DefaultPipeline::cmd_draw(const vk::CommandBuffer cmd, const vk::Image render_target, const vk::Extent2D window_size) const {
    ASSERT(cmd != nullptr);
    ASSERT(render_target != nullptr);
    ASSERT(window_size.width > 0);
    ASSERT(window_size.height > 0);

    BarrierBuilder(cmd)
        .add_image_barrier(m_color_image.image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
        .set_image_dst(vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal)
        .add_image_barrier(m_depth_image.image, {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1})
        .set_image_dst(vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .build_and_run();

    const vk::RenderingAttachmentInfo color_attachment = {
        .imageView = m_color_image.view,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eDontCare,
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
        .renderArea = {{0, 0}, window_size},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = &depth_attachment,
    });

    cmd.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e4);
    cmd.setSampleMaskEXT(vk::SampleCountFlagBits::e4, vk::SampleMask{0xff});

    for (const auto& system : m_render_systems) {
        system->cmd_draw(cmd, m_global_set);
    }

    cmd.endRendering();

    BarrierBuilder(cmd)
        .add_image_barrier(m_color_image.image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
        .set_image_src(vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal)
        .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
        .add_image_barrier(render_target, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
        .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
        .build_and_run();

    const vk::ImageResolve2 resolve = {
        .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .extent = vk::Extent3D{window_size.width, window_size.height, 1},
    };
    cmd.resolveImage2({
        .srcImage = m_color_image.image,
        .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
        .dstImage = render_target,
        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
        .regionCount = 1,
        .pRegions = &resolve,
    });

    BarrierBuilder(cmd)
        .add_image_barrier(render_target, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
        .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
        .set_image_dst(vk::PipelineStageFlagBits2::eAllGraphics, vk::AccessFlagBits2::eNone, vk::ImageLayout::ePresentSrcKHR)
        .build_and_run();
}

Result<SkyboxRenderer> SkyboxRenderer::create(const Engine& engine, const DefaultPipeline& pipeline) {
    auto renderer = ok<SkyboxRenderer>();

    const auto set_layout = create_descriptor_set_layout(engine, std::array{
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
    });
    if (set_layout.has_err())
        return set_layout.err();
    renderer->m_set_layout = *set_layout;

    std::array set_layouts = {pipeline.get_global_set_layout(), renderer->m_set_layout};
    const auto pipeline_layout = engine.device.createPipelineLayout({
        .setLayoutCount = to_u32(set_layouts.size()),
        .pSetLayouts = set_layouts.data(),
    });
    if (pipeline_layout.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkPipelineLayout;
    renderer->m_pipeline_layout = pipeline_layout.value;

    const auto skybox_shader_result = create_linked_shaders(engine, renderer->m_shaders, std::array{
        ShaderConfig{
            .path = "../shaders/skybox.vert.spv",
            .stage = vk::ShaderStageFlagBits::eVertex,
            .next_stage = vk::ShaderStageFlagBits::eFragment,
            .set_layouts = set_layouts,
            .push_ranges = {},
        },
        ShaderConfig{
            .path = "../shaders/skybox.frag.spv",
            .stage = vk::ShaderStageFlagBits::eFragment,
            .next_stage = {},
            .set_layouts = set_layouts,
            .push_ranges = {},
        },
    });
    if (skybox_shader_result.has_err())
        return skybox_shader_result.err();

    ASSERT(renderer->m_set_layout != nullptr);
    ASSERT(renderer->m_pipeline_layout != nullptr);
    for (const auto shader : renderer->m_shaders) {
        ASSERT(shader != nullptr);
    }
    return renderer;
}

Result<void> SkyboxRenderer::load_skybox(const Engine& engine, const vk::DescriptorPool pool, const std::filesystem::path path) {
    ASSERT(pool != nullptr);
    ASSERT(!path.empty());

    const auto set = hg::allocate_descriptor_set(engine, pool, m_set_layout);
    const auto cubemap = GpuImage::create_cubemap(engine, path);
    if (set.has_err())
        return set.err();
    if (cubemap.has_err())
        return cubemap.err();
    m_set = *set;
    m_cubemap = *cubemap;
    m_sampler = create_sampler(engine, {.type = SamplerType::Linear});
    hg::write_image_sampler_descriptor(engine, m_set, 0, m_sampler, m_cubemap.view);

    const auto mesh = generate_cube();
    m_index_buffer = GpuBuffer::create(engine, {
        mesh.indices.size() * sizeof(mesh.indices[0]),
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });
    m_vertex_buffer = GpuBuffer::create(engine, {
        mesh.positions.size() * sizeof(mesh.positions[0]),
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });
    m_index_buffer.write(engine, mesh.indices.data(), mesh.indices.size() * sizeof(mesh.indices[0]), 0);
    m_vertex_buffer.write(engine, mesh.positions.data(), mesh.positions.size() * sizeof(mesh.positions[0]), 0);

    ASSERT(m_cubemap.allocation != nullptr);
    ASSERT(m_cubemap.image != nullptr);
    ASSERT(m_cubemap.view != nullptr);
    ASSERT(m_sampler != nullptr);
    ASSERT(m_set != nullptr);
    ASSERT(m_index_buffer.allocation != nullptr);
    ASSERT(m_index_buffer.buffer != nullptr);
    ASSERT(m_vertex_buffer.allocation != nullptr);
    ASSERT(m_vertex_buffer.buffer != nullptr);
    return ok();
}

void SkyboxRenderer::destroy(const Engine& engine) const {
    ASSERT(engine.device != nullptr);

    m_vertex_buffer.destroy(engine);
    m_index_buffer.destroy(engine);
    m_cubemap.destroy(engine);

    ASSERT(m_sampler != nullptr);
    engine.device.destroySampler(m_sampler);

    for (const auto& shader : m_shaders) {
        ASSERT(shader != nullptr);
        engine.device.destroyShaderEXT(shader);
    }

    ASSERT(m_pipeline_layout != nullptr);
    engine.device.destroyPipelineLayout(m_pipeline_layout);

    ASSERT(m_set_layout != nullptr);
    engine.device.destroyDescriptorSetLayout(m_set_layout);
}

void SkyboxRenderer::cmd_draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const {
    ASSERT(m_set != nullptr);
    ASSERT(m_vertex_buffer.buffer != nullptr);
    ASSERT(m_index_buffer.buffer != nullptr);
    ASSERT(cmd != nullptr);
    ASSERT(global_set != nullptr);

    cmd.setDepthTestEnable(vk::False);
    cmd.setDepthWriteEnable(vk::False);
    cmd.setCullMode(vk::CullModeFlagBits::eFront);

    cmd.bindShadersEXT({vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment}, m_shaders);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout, 0, {global_set, m_set}, {});

    cmd.setVertexInputEXT({vk::VertexInputBindingDescription2EXT{.stride = sizeof(glm::vec3), .inputRate = vk::VertexInputRate::eVertex, .divisor = 1}}, {
        vk::VertexInputAttributeDescription2EXT{.location = 0, .format = vk::Format::eR32G32B32Sfloat}
    });
    cmd.bindVertexBuffers(0, {m_vertex_buffer.buffer}, {vk::DeviceSize{0}});
    cmd.bindIndexBuffer(m_index_buffer.buffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(36, 1, 0, 0, 1);

    cmd.setDepthTestEnable(vk::True);
    cmd.setDepthWriteEnable(vk::True);
    cmd.setCullMode(vk::CullModeFlagBits::eNone);
}

Result<PbrRenderer> PbrRenderer::create(const Engine& engine, const DefaultPipeline& pipeline) {
    auto renderer = ok<PbrRenderer>();

    const auto set_layout = create_descriptor_set_layout(engine, std::array{
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
    });
    if (set_layout.has_err())
        return set_layout.err();
    renderer->m_set_layout = *set_layout;

    ASSERT(pipeline.get_global_set_layout() != nullptr);
    std::array set_layouts = {pipeline.get_global_set_layout(), renderer->m_set_layout};
    std::array push_ranges = {vk::PushConstantRange{vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstant)}};
    const auto pipeline_layout = engine.device.createPipelineLayout({
        .setLayoutCount = to_u32(set_layouts.size()),
        .pSetLayouts = set_layouts.data(),
        .pushConstantRangeCount = to_u32(push_ranges.size()),
        .pPushConstantRanges = push_ranges.data(),
    });
    if (pipeline_layout.result != vk::Result::eSuccess)
        return Err::CouldNotCreateVkPipelineLayout;
    renderer->m_pipeline_layout = pipeline_layout.value;

    const auto shader_result = create_linked_shaders(engine, renderer->m_shaders, std::array{
        ShaderConfig{
            .path = "../shaders/pbr.vert.spv",
            .stage = vk::ShaderStageFlagBits::eVertex,
            .next_stage = vk::ShaderStageFlagBits::eFragment,
            .set_layouts = set_layouts,
            .push_ranges = push_ranges,
        },
        ShaderConfig{
            .path = "../shaders/pbr.frag.spv",
            .stage = vk::ShaderStageFlagBits::eFragment,
            .next_stage = {},
            .set_layouts = set_layouts,
            .push_ranges = push_ranges,
        },
    });
    if (shader_result.has_err())
        return shader_result.err();

    ASSERT(renderer->m_set_layout != nullptr);
    ASSERT(renderer->m_pipeline_layout != nullptr);
    for (const auto shader : renderer->m_shaders) {
        ASSERT(shader != nullptr);
    }
    return renderer;
}

void PbrRenderer::destroy(const Engine& engine) const {
    ASSERT(engine.device != nullptr);

    for (const auto& texture : m_textures)
    texture.destroy(engine);
    for (const auto& model : m_models)
    model.destroy(engine);

    for (const auto& shader : m_shaders) {
        ASSERT(shader != nullptr);
        engine.device.destroyShaderEXT(shader);
    }

    ASSERT(m_pipeline_layout != nullptr);
    engine.device.destroyPipelineLayout(m_pipeline_layout);

    ASSERT(m_set_layout != nullptr);
    engine.device.destroyDescriptorSetLayout(m_set_layout);
}

void PbrRenderer::cmd_draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const {
    ASSERT(cmd != nullptr);
    ASSERT(global_set != nullptr);

    cmd.setCullMode(vk::CullModeFlagBits::eBack);

    cmd.setVertexInputEXT({vk::VertexInputBindingDescription2EXT{.stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex, .divisor = 1}}, {
        vk::VertexInputAttributeDescription2EXT{.location = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, position)},
        vk::VertexInputAttributeDescription2EXT{.location = 1, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, normal)},
        vk::VertexInputAttributeDescription2EXT{.location = 2, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, tex_coord)}
    });
    cmd.bindShadersEXT({vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment}, m_shaders);

    for (const auto& ticket : m_render_queue) {
        const auto& model = m_models[ticket.model.index];

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout, 0, {global_set, model.set}, {});
        PushConstant model_push = {ticket.transform.matrix()};
        cmd.pushConstants(m_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(model_push), &model_push);

        cmd.bindIndexBuffer(model.index_buffer.buffer, 0, vk::IndexType::eUint32);
        cmd.bindVertexBuffers(0, {model.vertex_buffer.buffer}, {vk::DeviceSize{0}});
        cmd.drawIndexed(model.index_count, 1, 0, 0, 1);
    }

    cmd.setCullMode(vk::CullModeFlagBits::eNone);
}

Result<PbrRenderer::TextureHandle> PbrRenderer::load_texture(const Engine& engine, std::filesystem::path path) {
    ASSERT(!path.empty());

    const auto texture_data = ImageData::load(path);
    if (texture_data.has_err())
        return texture_data.err();

    return ok(load_texture_from_data(engine, {
        texture_data->pixels.get(), 4, {to_u32(texture_data->width), to_u32(texture_data->height), 1}
    }));
}

PbrRenderer::TextureHandle PbrRenderer::load_texture_from_data(const Engine& engine, const GpuImage::Data& data, const vk::Format format) {
    ASSERT(data.ptr != nullptr);
    ASSERT(data.alignment > 0);
    ASSERT(data.extent.width > 0);
    ASSERT(data.extent.height > 0);
    ASSERT(data.extent.depth > 0);
    ASSERT(format != vk::Format::eUndefined);

    const u32 mips = get_mip_count(data.extent);
    const auto image = GpuImage::create(engine, {
        .extent = data.extent,
        .format = format,
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
        .mip_levels = mips,
    });
    image.write(engine, data, vk::ImageLayout::eShaderReadOnlyOptimal);
    image.generate_mipmaps(engine, mips, data.extent, format, vk::ImageLayout::eShaderReadOnlyOptimal);
    const auto sampler = create_sampler(engine, {.type = SamplerType::Linear, .mip_levels = mips});

    ASSERT(image.allocation != nullptr);
    ASSERT(image.image != nullptr);
    ASSERT(image.view != nullptr);
    ASSERT(sampler != nullptr);
    m_textures.emplace_back(image, sampler);
    return {m_textures.size() - 1};
}

Result<PbrRenderer::ModelHandle> PbrRenderer::load_model(const Engine& engine, const vk::DescriptorPool descriptor_pool, std::filesystem::path path, TextureHandle texture) {
    ASSERT(descriptor_pool != nullptr);
    ASSERT(!path.empty());
    ASSERT(texture.index < m_textures.size());

    const auto model = ModelData::load_gltf(path);
    if (model.has_err())
        return model.err();

    const auto vertex_data = VertexData::from_mesh(std::move(model->mesh));
    return ok(load_model_from_data(engine, descriptor_pool, vertex_data, model->roughness, model->metalness, texture));
}

PbrRenderer::ModelHandle PbrRenderer::load_model_from_data(const Engine& engine, const vk::DescriptorPool descriptor_pool, const VertexData& data, const float roughness, const float metalness, const TextureHandle texture) {
    ASSERT(descriptor_pool != nullptr);
    ASSERT(!data.indices.empty());
    ASSERT(!data.vertices.empty());
    ASSERT(texture.index < m_textures.size());
    ASSERT(roughness >= 0.0 && roughness <= 1.0);
    ASSERT(metalness >= 0.0 && metalness <= 1.0);

    const auto index_buffer = GpuBuffer::create(engine, {
        data.indices.size() * sizeof(data.indices[0]),
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });
    const auto vertex_buffer = GpuBuffer::create(engine, {
        data.vertices.size() * sizeof(data.vertices[0]),
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });
    const auto material_buffer = GpuBuffer::create(engine, {
        sizeof(MaterialUniform),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst
    });

    index_buffer.write(engine, data.indices.data(), data.indices.size() * sizeof(data.indices[0]), 0);
    vertex_buffer.write(engine, data.vertices.data(), data.vertices.size() * sizeof(data.vertices[0]), 0);
    material_buffer.write(engine, MaterialUniform{roughness, metalness});

    const auto set = hg::allocate_descriptor_set(engine, descriptor_pool, m_set_layout);
    if (set.has_err())
        ERROR("PbrPipeline descriptor pool is empty");
    hg::write_image_sampler_descriptor(engine, *set, 0, m_textures[texture.index].sampler, m_textures[texture.index].image.view);
    hg::write_uniform_buffer_descriptor(engine, *set, 1, material_buffer.buffer, sizeof(MaterialUniform));

    ASSERT(data.indices.size() > 0);
    ASSERT(index_buffer.allocation != nullptr);
    ASSERT(index_buffer.buffer != nullptr);
    ASSERT(vertex_buffer.allocation != nullptr);
    ASSERT(vertex_buffer.buffer != nullptr);
    ASSERT(material_buffer.allocation != nullptr);
    ASSERT(material_buffer.buffer != nullptr);
    ASSERT(*set != nullptr);
    m_models.emplace_back(to_u32(data.indices.size()), index_buffer, vertex_buffer, material_buffer, *set);
    return {m_models.size() - 1};
}

PbrRenderer::VertexData PbrRenderer::VertexData::from_mesh(const Mesh& mesh) {
    ASSERT(!mesh.indices.empty());
    ASSERT(!mesh.positions.empty());
    ASSERT(!mesh.normals.empty());
    ASSERT(!mesh.tex_coords.empty());

    VertexData data = {.indices = std::move(mesh.indices)};
    data.vertices.reserve(mesh.positions.size());
    for (usize i = 0; i < mesh.positions.size(); ++i) {
        data.vertices.emplace_back(mesh.positions[i], mesh.normals[i], mesh.tex_coords[i]);
    }

    ASSERT(!data.indices.empty());
    ASSERT(!data.vertices.empty());
    return data;
}

} // namespace hg
