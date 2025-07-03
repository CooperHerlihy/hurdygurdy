#include "hg_pipeline.h"

#include "hg_utils.h"
#include "hg_math.h"
#include "hg_generate.h"
#include "hg_load.h"
#include "hg_vulkan_engine.h"

#include <filesystem>

namespace hg {

Result<DefaultPipeline> DefaultPipeline::create(const Engine& engine, const vk::Extent2D window_size) {
    ASSERT(window_size.width != 0);
    ASSERT(window_size.height != 0);

    auto pipeline = ok<DefaultPipeline>();

    pipeline->m_color_image = GpuImageAndView::create(engine, {
        .extent = {window_size.width, window_size.height, 1},
        .format = Window::SwapchainImageFormat,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        .sample_count = vk::SampleCountFlagBits::e4,
    });
    pipeline->m_depth_image = GpuImageAndView::create(engine, {
        .extent = {window_size.width, window_size.height, 1},
        .format = vk::Format::eD32Sfloat,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .aspect_flags = vk::ImageAspectFlagBits::eDepth,
        .sample_count = vk::SampleCountFlagBits::e4,
    });

    const auto descriptor_pool = create_descriptor_pool(engine, 1, std::array{
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2}
    });
    if (descriptor_pool.has_err())
        return descriptor_pool.err();
    pipeline->m_descriptor_pool = *descriptor_pool;

    const auto set_layout = create_descriptor_set_layout(engine, std::array{
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
    });
    if (set_layout.has_err())
        return set_layout.err();
    pipeline->m_set_layout = *set_layout;

    const auto global_set = allocate_descriptor_set(engine, pipeline->m_descriptor_pool, pipeline->m_set_layout);
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

    write_uniform_buffer_descriptor(engine, pipeline->m_global_set, 0, pipeline->m_vp_buffer.get(), sizeof(ViewProjectionUniform));
    write_uniform_buffer_descriptor(engine, pipeline->m_global_set, 1, pipeline->m_light_buffer.get(), sizeof(LightUniform));

    ASSERT(pipeline->m_set_layout != nullptr);
    ASSERT(pipeline->m_global_set != nullptr);
    return pipeline;
}

void DefaultPipeline::destroy(const Engine& engine) const {
    ASSERT(engine.device != nullptr);

    ASSERT(m_set_layout != nullptr);
    engine.device.destroyDescriptorSetLayout(m_set_layout);
    ASSERT(m_descriptor_pool != nullptr);
    engine.device.destroyDescriptorPool(m_descriptor_pool);

    m_light_buffer.destroy(engine);
    m_vp_buffer.destroy(engine);

    m_depth_image.destroy(engine);
    m_color_image.destroy(engine);
}

void DefaultPipeline::resize(const Engine& engine, const vk::Extent2D window_size) {
    ASSERT(window_size.width > 0);
    ASSERT(window_size.height > 0);

    m_color_image.destroy(engine);
    m_color_image = GpuImageAndView::create(engine, {
        .extent = {window_size.width, window_size.height, 1},
        .format = Window::SwapchainImageFormat,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        .sample_count = vk::SampleCountFlagBits::e4,
    });

    m_depth_image.destroy(engine);
    m_depth_image = GpuImageAndView::create(engine, {
        .extent = {window_size.width, window_size.height, 1},
        .format = vk::Format::eD32Sfloat,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .aspect_flags = vk::ImageAspectFlagBits::eDepth,
        .sample_count = vk::SampleCountFlagBits::e4,
    });
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

void DefaultPipeline::cmd_draw(
    const vk::CommandBuffer cmd, const vk::Image render_target, const vk::Extent2D window_size
) const {
    ASSERT(cmd != nullptr);
    ASSERT(render_target != nullptr);
    ASSERT(window_size.width > 0);
    ASSERT(window_size.height > 0);

    BarrierBuilder(cmd)
        .add_image_barrier(m_color_image.get_image(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
        .set_image_dst(vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal)
        .add_image_barrier(m_depth_image.get_image(), {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1})
        .set_image_dst(
            vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal
        )
        .build_and_run();

    const vk::RenderingAttachmentInfo color_attachment = {
        .imageView = m_color_image.get_view(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eDontCare,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {{std::array{0.0f, 0.0f, 0.0f, 1.0f}}},
    };
    const vk::RenderingAttachmentInfo depth_attachment = {
        .imageView = m_depth_image.get_view(),
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
        .add_image_barrier(m_color_image.get_image(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
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
        .srcImage = m_color_image.get_image(),
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

    const auto descriptor_pool = create_descriptor_pool(engine, 1, std::array{
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1}
    });
    if (descriptor_pool.has_err())
        return descriptor_pool.err();
    renderer->m_descriptor_pool = *descriptor_pool;

    const auto set = allocate_descriptor_set(engine, renderer->m_descriptor_pool, renderer->m_set_layout);
    if (set.has_err())
        return set.err();
    renderer->m_set = *set;

    ASSERT(renderer->m_set_layout != nullptr);
    ASSERT(renderer->m_pipeline_layout != nullptr);
    ASSERT(renderer->m_descriptor_pool != nullptr);
    ASSERT(renderer->m_set != nullptr);
    for (const auto shader : renderer->m_shaders) {
        ASSERT(shader != nullptr);
    }
    return renderer;
}

Result<void> SkyboxRenderer::load_skybox(const Engine& engine, const std::filesystem::path path) {
    ASSERT(!path.empty());

    const auto cubemap = Texture::create_cubemap(engine, path, {
        .format = vk::Format::eR8G8B8A8Srgb,
        .aspect_flags = vk::ImageAspectFlagBits::eColor,
        .sampler_type = Sampler::Linear,
    });
    if (cubemap.has_err())
        return cubemap.err();
    hg::write_image_sampler_descriptor(engine, m_set, 0, cubemap->get_sampler(), cubemap->get_view());
    m_cubemap = *cubemap;

    const auto mesh = generate_cube();
    std::vector<glm::vec3> positions = {};
    positions.reserve(mesh.vertices.size());
    for (const auto vertex : mesh.vertices) {
        positions.emplace_back(vertex.position);
    }
    m_vertex_buffer = GpuBuffer::create(engine, {
        positions.size() * sizeof(positions[0]),
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });
    m_index_buffer = GpuBuffer::create(engine, {
        mesh.indices.size() * sizeof(mesh.indices[0]),
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });
    m_vertex_buffer.write_span(engine, std::span{positions}, 0);
    m_index_buffer.write_span(engine, std::span{mesh.indices}, 0);

    ASSERT(m_set != nullptr);
    return ok();
}

void SkyboxRenderer::destroy(const Engine& engine) const {
    ASSERT(engine.device != nullptr);

    m_vertex_buffer.destroy(engine);
    m_index_buffer.destroy(engine);
    m_cubemap.destroy(engine);

    for (const auto& shader : m_shaders) {
        ASSERT(shader != nullptr);
        engine.device.destroyShaderEXT(shader);
    }

    ASSERT(m_descriptor_pool != nullptr);
    engine.device.destroyDescriptorPool(m_descriptor_pool);

    ASSERT(m_pipeline_layout != nullptr);
    engine.device.destroyPipelineLayout(m_pipeline_layout);

    ASSERT(m_set_layout != nullptr);
    engine.device.destroyDescriptorSetLayout(m_set_layout);
}

void SkyboxRenderer::cmd_draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const {
    ASSERT(m_set != nullptr);
    ASSERT(cmd != nullptr);
    ASSERT(global_set != nullptr);

    cmd.setDepthTestEnable(vk::False);
    cmd.setDepthWriteEnable(vk::False);
    cmd.setCullMode(vk::CullModeFlagBits::eFront);

    cmd.bindShadersEXT({vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment}, m_shaders);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout, 0, {global_set, m_set}, {});

    cmd.setVertexInputEXT(
        {vk::VertexInputBindingDescription2EXT{.stride = sizeof(glm::vec3), .inputRate = vk::VertexInputRate::eVertex, .divisor = 1}},
        {vk::VertexInputAttributeDescription2EXT{.location = 0, .format = vk::Format::eR32G32B32Sfloat}}
    );
    cmd.bindVertexBuffers(0, {m_vertex_buffer.get()}, {vk::DeviceSize{0}});
    cmd.bindIndexBuffer(m_index_buffer.get(), 0, vk::IndexType::eUint32);
    cmd.drawIndexed(36, 1, 0, 0, 1);

    cmd.setDepthTestEnable(vk::True);
    cmd.setDepthWriteEnable(vk::True);
    cmd.setCullMode(vk::CullModeFlagBits::eNone);
}

Result<PbrRenderer> PbrRenderer::create(const Engine& engine, const DefaultPipeline& pipeline) {
    auto renderer = ok<PbrRenderer>();

    const auto set_layout = create_descriptor_set_layout(
        engine,
        std::array{vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, MaxTextures, vk::ShaderStageFlagBits::eFragment},},
        std::array{vk::DescriptorBindingFlags{vk::DescriptorBindingFlagBits::ePartiallyBound}}
    );
    if (set_layout.has_err())
        return set_layout.err();
    renderer->m_set_layout = *set_layout;

    ASSERT(pipeline.get_global_set_layout() != nullptr);
    std::array set_layouts = {pipeline.get_global_set_layout(), renderer->m_set_layout};
    std::array push_ranges = {vk::PushConstantRange{
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstant)
    }};
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

    const auto descriptor_pool = create_descriptor_pool(engine, 1, std::array{
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, MaxTextures}
    });
    if (descriptor_pool.has_err())
        return descriptor_pool.err();
    renderer->m_descriptor_pool = *descriptor_pool;

    const auto texture_set = allocate_descriptor_set(engine, renderer->m_descriptor_pool, renderer->m_set_layout);
    if (texture_set.has_err())
        return texture_set.err();
    renderer->m_texture_set = *texture_set;

    ASSERT(renderer->m_set_layout != nullptr);
    ASSERT(renderer->m_pipeline_layout != nullptr);
    for (const auto shader : renderer->m_shaders) {
        ASSERT(shader != nullptr);
    }
    ASSERT(renderer->m_descriptor_pool != nullptr);
    ASSERT(renderer->m_texture_set != nullptr);
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

    ASSERT(m_descriptor_pool != nullptr);
    engine.device.destroyDescriptorPool(m_descriptor_pool);

    ASSERT(m_pipeline_layout != nullptr);
    engine.device.destroyPipelineLayout(m_pipeline_layout);

    ASSERT(m_set_layout != nullptr);
    engine.device.destroyDescriptorSetLayout(m_set_layout);
}

void PbrRenderer::cmd_draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const {
    ASSERT(cmd != nullptr);
    ASSERT(global_set != nullptr);

    cmd.setCullMode(vk::CullModeFlagBits::eBack);

    cmd.setVertexInputEXT({
        vk::VertexInputBindingDescription2EXT{.stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex, .divisor = 1}
    }, {
        vk::VertexInputAttributeDescription2EXT{.location = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, position)},
        vk::VertexInputAttributeDescription2EXT{.location = 1, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, normal)},
        vk::VertexInputAttributeDescription2EXT{.location = 2, .format = vk::Format::eR32G32B32A32Sfloat, .offset = offsetof(Vertex, tangent)},
        vk::VertexInputAttributeDescription2EXT{.location = 3, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, tex_coord)}
    });
    cmd.bindShadersEXT({vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment}, m_shaders);

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout, 0, {global_set, m_texture_set}, {});
    for (const auto& ticket : m_render_queue) {
        const auto& model = m_models[ticket.model.index];

        PushConstant model_push = {
            .model = ticket.transform.matrix(),
            .normal_map_index = to_u32(model.normal_map.index),
            .texture_index = to_u32(model.texture.index),
            .roughness = model.roughness,
            .metalness = model.metalness
        };
        cmd.pushConstants(
            m_pipeline_layout,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            0, sizeof(model_push), &model_push
        );

        cmd.bindIndexBuffer(model.index_buffer.get(), 0, vk::IndexType::eUint32);
        cmd.bindVertexBuffers(0, {model.vertex_buffer.get()}, {vk::DeviceSize{0}});
        cmd.drawIndexed(model.index_count, 1, 0, 0, 1);
    }

    cmd.setCullMode(vk::CullModeFlagBits::eNone);
}

Result<PbrRenderer::TextureHandle> PbrRenderer::load_texture(
    const Engine& engine, std::filesystem::path path, const vk::Format format
) {
    ASSERT(m_textures.size() < MaxTextures);

    const auto texture = Texture::from_file(engine, path, {.format = format, .create_mips = true, .sampler_type = Sampler::Linear});
    if (texture.has_err())
        return texture.err();

    usize index = m_textures.size();
    write_image_sampler_descriptor(engine, m_texture_set, 0, texture->get_sampler(), texture->get_view(), to_u32(index));

    m_textures.emplace_back(*texture);
    return ok<TextureHandle>(index);
}

PbrRenderer::TextureHandle PbrRenderer::load_texture_from_data(
    const Engine& engine, const GpuImage::Data& data, const vk::Format format
) {
    ASSERT(m_textures.size() < MaxTextures);

    const auto texture = Texture::from_data(engine, data, {.format = format, .create_mips = true, .sampler_type = Sampler::Linear});

    usize index = m_textures.size();
    write_image_sampler_descriptor(engine, m_texture_set, 0, texture.get_sampler(), texture.get_view(), to_u32(index));

    m_textures.emplace_back(texture);
    return {index};
}

Result<PbrRenderer::ModelHandle> PbrRenderer::load_model(
    const Engine& engine, const std::filesystem::path path,
    const TextureHandle normal_map, const TextureHandle texture
) {
    ASSERT(!path.empty());
    ASSERT(texture.index < m_textures.size());

    const auto model = ModelData::load_gltf(path);
    if (model.has_err())
        return model.err();

    return ok(load_model_from_data(engine, model->mesh, normal_map, texture, model->roughness, model->metalness));
}

PbrRenderer::ModelHandle PbrRenderer::load_model_from_data(
    const Engine& engine,
    const Mesh& data,
    const TextureHandle normal_map,
    const TextureHandle texture,
    const float roughness,
    const float metalness
) {
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

    index_buffer.write_span(engine, std::span{data.indices});
    vertex_buffer.write_span(engine, std::span{data.vertices});

    m_models.emplace_back(
        to_u32(data.indices.size()),
        index_buffer, vertex_buffer,
        normal_map, texture,
        roughness, metalness
    );
    return {m_models.size() - 1};
}

} // namespace hg
