#include "hg_renderer.h"

#include "hg_utils.h"
#include "hg_math.h"
#include "hg_generate.h"
#include "hg_load.h"
#include "hg_vulkan_engine.h"

#include <filesystem>

namespace hg {

DefaultRenderer DefaultRenderer::create(const Engine& engine, const vk::Extent2D window_size) {
    ASSERT(window_size.width != 0);
    ASSERT(window_size.height != 0);

    DefaultRenderer pipeline{};

    pipeline.m_color_image = GpuImageAndView::create(engine, {
        .extent{window_size.width, window_size.height, 1},
        .format = Window::SwapchainImageFormat,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        .sample_count = vk::SampleCountFlagBits::e4,
    });
    pipeline.m_depth_image = GpuImageAndView::create(engine, {
        .extent{window_size.width, window_size.height, 1},
        .format = vk::Format::eD32Sfloat,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .aspect_flags = vk::ImageAspectFlagBits::eDepth,
        .sample_count = vk::SampleCountFlagBits::e4,
    });

    pipeline.m_set_layout = DescriptorSetLayout::create(engine, {std::array{
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
    }});

    pipeline.m_descriptor_pool = DescriptorPool::create(engine, {1, std::array{
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2}
    }});

    pipeline.m_global_set = *pipeline.m_descriptor_pool.allocate_set(engine, pipeline.m_set_layout.get());;

    pipeline.m_vp_buffer = GpuBuffer::create(engine, {
        sizeof(ViewProjectionUniform),
        vk::BufferUsageFlagBits::eUniformBuffer, GpuBuffer::RandomAccess
    });
    pipeline.m_light_buffer = GpuBuffer::create(engine, {
        sizeof(LightUniform) * MaxLights,
        vk::BufferUsageFlagBits::eUniformBuffer, GpuBuffer::RandomAccess
    });

    write_uniform_buffer_descriptor(engine, {pipeline.m_vp_buffer.get(), sizeof(ViewProjectionUniform)}, pipeline.m_global_set, 0);
    write_uniform_buffer_descriptor(engine, {pipeline.m_light_buffer.get(), sizeof(LightUniform)}, pipeline.m_global_set, 1);

    ASSERT(pipeline.m_global_set != nullptr);
    return pipeline;
}

void DefaultRenderer::destroy(const Engine& engine) const {
    ASSERT(engine.device != nullptr);

    m_descriptor_pool.destroy(engine);
    m_set_layout.destroy(engine);

    m_light_buffer.destroy(engine);
    m_vp_buffer.destroy(engine);

    m_depth_image.destroy(engine);
    m_color_image.destroy(engine);
}

void DefaultRenderer::resize(const Engine& engine, const vk::Extent2D window_size) {
    ASSERT(window_size.width > 0);
    ASSERT(window_size.height > 0);

    m_color_image.destroy(engine);
    m_color_image = GpuImageAndView::create(engine, {
        .extent{window_size.width, window_size.height, 1},
        .format = Window::SwapchainImageFormat,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        .sample_count = vk::SampleCountFlagBits::e4,
    });

    m_depth_image.destroy(engine);
    m_depth_image = GpuImageAndView::create(engine, {
        .extent{window_size.width, window_size.height, 1},
        .format = vk::Format::eD32Sfloat,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .aspect_flags = vk::ImageAspectFlagBits::eDepth,
        .sample_count = vk::SampleCountFlagBits::e4,
    });
}

void DefaultRenderer::update_camera(const Engine& engine, const Cameraf& camera) {
    const glm::mat4 view{camera.view()};

    ASSERT(m_lights.size() < MaxLights);
    LightUniform lights{.count = m_lights.size()};
    for (usize i = 0; i < m_lights.size(); ++i) {
        lights.vals[i] = {
            .position = view * m_lights[i].position,
            .color = m_lights[i].color,
        };
    }

    m_light_buffer.write(engine, lights);
    m_vp_buffer.write(engine, view, offsetof(ViewProjectionUniform, view));
}

void DefaultRenderer::cmd_draw(
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

    const vk::RenderingAttachmentInfo color_attachment{
        .imageView = m_color_image.get_view(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eDontCare,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue{{std::array{0.0f, 0.0f, 0.0f, 1.0f}}},
    };
    const vk::RenderingAttachmentInfo depth_attachment{
        .imageView = m_depth_image.get_view(),
        .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue{.depthStencil{.depth = 1.0f, .stencil = 0}},
    };
    cmd.beginRendering({
        .renderArea{{0, 0}, window_size},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = &depth_attachment,
    });

    cmd.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e4);
    cmd.setSampleMaskEXT(vk::SampleCountFlagBits::e4, vk::SampleMask{0xff});

    for (const auto& system : m_pipelines) {
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

    const vk::ImageResolve2 resolve{
        .srcSubresource{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .dstSubresource{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .extent{window_size.width, window_size.height, 1},
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

SkyboxPipeline SkyboxPipeline::create(const Engine& engine, const DefaultRenderer& renderer) {
    ASSERT(renderer.get_global_set_layout() != nullptr);

    SkyboxPipeline pipeline{};

    pipeline.m_set_layout = DescriptorSetLayout::create(engine, {{std::array{
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
    }}});

    std::array set_layouts{renderer.get_global_set_layout(), pipeline.m_set_layout.get()};

    const auto graphics_pipeline = GraphicsPipeline::create(engine, {
        .set_layouts = set_layouts,
        .push_ranges{},
        .vertex_shader_path = "../shaders/skybox.vert.spv",
        .fragment_shader_path = "../shaders/skybox.frag.spv",
    });
    if (graphics_pipeline.has_err())
        ERROR("Could not find valid skybox shaders");
    pipeline.m_pipeline= *graphics_pipeline;

    pipeline.m_descriptor_pool = DescriptorPool::create(engine, {1, std::array{
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1}
    }});

    pipeline.m_set = *pipeline.m_descriptor_pool.allocate_set(engine, pipeline.m_set_layout.get());;

    ASSERT(pipeline.m_set != nullptr);
    return pipeline;
}

Result<void> SkyboxPipeline::load_skybox(const Engine& engine, const std::filesystem::path path) {
    ASSERT(!path.empty());

    const auto cubemap = Texture::from_cubemap_file(engine, path, {
        .format = vk::Format::eR8G8B8A8Srgb,
        .aspect_flags = vk::ImageAspectFlagBits::eColor,
        .sampler_type = Sampler::Linear,
    });
    if (cubemap.has_err())
        return cubemap.err();
    hg::write_image_sampler_descriptor(engine, *cubemap, m_set, 0);
    m_cubemap = *cubemap;

    const auto mesh = generate_cube();
    std::vector<glm::vec3> positions{};
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

void SkyboxPipeline::destroy(const Engine& engine) const {
    ASSERT(engine.device != nullptr);

    m_vertex_buffer.destroy(engine);
    m_index_buffer.destroy(engine);
    m_cubemap.destroy(engine);

    m_descriptor_pool.destroy(engine);
    m_pipeline.destroy(engine);
    m_set_layout.destroy(engine);
}

void SkyboxPipeline::cmd_draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const {
    ASSERT(m_set != nullptr);
    ASSERT(cmd != nullptr);
    ASSERT(global_set != nullptr);

    cmd.setDepthTestEnable(vk::False);
    cmd.setDepthWriteEnable(vk::False);
    cmd.setCullMode(vk::CullModeFlagBits::eFront);

    cmd.bindShadersEXT({vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment}, m_pipeline.get_shaders());
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.get_layout(), 0, {global_set, m_set}, {});

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

PbrPipeline PbrPipeline::create(const Engine& engine, const DefaultRenderer& renderer) {
    ASSERT(renderer.get_global_set_layout() != nullptr);

    PbrPipeline pipeline{};

    pipeline.m_set_layout = DescriptorSetLayout::create(engine, {
        std::array{vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, MaxTextures, vk::ShaderStageFlagBits::eFragment},},
        std::array{vk::DescriptorBindingFlags{vk::DescriptorBindingFlagBits::ePartiallyBound}}
    });

    std::array set_layouts{renderer.get_global_set_layout(), pipeline.m_set_layout.get()};
    std::array push_ranges{vk::PushConstantRange{
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstant)
    }};

    const auto graphics_pipeline = GraphicsPipeline::create(engine, {
        .set_layouts = set_layouts,
        .push_ranges = push_ranges,
        .vertex_shader_path = "../shaders/pbr.vert.spv",
        .fragment_shader_path = "../shaders/pbr.frag.spv",
    });
    if (graphics_pipeline.has_err())
        ERROR("Could not find valid pbr shaders");
    pipeline.m_pipeline = *graphics_pipeline;

    pipeline.m_descriptor_pool = DescriptorPool::create(engine, {1, std::array{
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, MaxTextures}
    }});
    pipeline.m_texture_set = *pipeline.m_descriptor_pool.allocate_set(engine, pipeline.m_set_layout.get());;

    ASSERT(pipeline.m_texture_set != nullptr);
    return pipeline;
}

void PbrPipeline::destroy(const Engine& engine) const {
    ASSERT(engine.device != nullptr);

    for (const auto& texture : m_textures)
    texture.destroy(engine);
    for (const auto& model : m_models)
    model.destroy(engine);

    m_descriptor_pool.destroy(engine);
    m_pipeline.destroy(engine);
    m_set_layout.destroy(engine);
}

void PbrPipeline::cmd_draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const {
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
    cmd.bindShadersEXT({vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment}, m_pipeline.get_shaders());

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.get_layout(), 0, {global_set, m_texture_set}, {});
    for (const auto& ticket : m_render_queue) {
        const auto& model = m_models[ticket.model.index];

        PushConstant model_push{
            .model = ticket.transform.matrix(),
            .normal_map_index = to_u32(model.normal_map.index),
            .texture_index = to_u32(model.texture.index),
            .roughness = model.roughness,
            .metalness = model.metalness
        };
        cmd.pushConstants(
            m_pipeline.get_layout(),
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            0, sizeof(model_push), &model_push
        );

        cmd.bindIndexBuffer(model.index_buffer.get(), 0, vk::IndexType::eUint32);
        cmd.bindVertexBuffers(0, {model.vertex_buffer.get()}, {vk::DeviceSize{0}});
        cmd.drawIndexed(model.index_count, 1, 0, 0, 1);
    }

    cmd.setCullMode(vk::CullModeFlagBits::eNone);
}

Result<PbrPipeline::TextureHandle> PbrPipeline::load_texture(
    const Engine& engine, std::filesystem::path path, const vk::Format format
) {
    ASSERT(m_textures.size() < MaxTextures);

    const auto texture = Texture::from_file(engine, path, {.format = format, .create_mips = true, .sampler_type = Sampler::Linear});
    if (texture.has_err())
        return texture.err();

    usize index = m_textures.size();
    write_image_sampler_descriptor(engine, *texture, m_texture_set, 0, to_u32(index));

    m_textures.emplace_back(*texture);
    return ok<TextureHandle>(index);
}

PbrPipeline::TextureHandle PbrPipeline::load_texture_from_data(
    const Engine& engine, const GpuImage::Data& data, const vk::Format format
) {
    ASSERT(m_textures.size() < MaxTextures);

    const auto texture = Texture::from_data(engine, data, {.format = format, .create_mips = true, .sampler_type = Sampler::Linear});

    usize index = m_textures.size();
    write_image_sampler_descriptor(engine, texture, m_texture_set, 0, to_u32(index));

    m_textures.emplace_back(texture);
    return {index};
}

Result<PbrPipeline::ModelHandle> PbrPipeline::load_model(
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

PbrPipeline::ModelHandle PbrPipeline::load_model_from_data(
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
