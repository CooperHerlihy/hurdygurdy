#include "hg_renderer.h"

#include "hg_load.h"
#include "hg_generate.h"

#include <filesystem>

namespace hg {

Result<DefaultRenderer> DefaultRenderer::create(Vk& vk, const Config& config) {
    if (!config.fullscreen) {
        ASSERT(config.window_size.x > 0);
        ASSERT(config.window_size.y > 0);
    }

    auto renderer = ok<DefaultRenderer>();

    const auto window = Window::create(config.fullscreen, config.window_size.x, config.window_size.y);
    if (window.has_err())
        return window.err();
    renderer->m_window = *window;

    const auto surface = Surface::create(vk, renderer->m_window.get());
    if (surface.has_err())
        return surface.err();
    renderer->m_surface = *surface;

    const auto swapchain = Swapchain::create(vk, renderer->m_surface.get());
    if (swapchain.has_err())
        return swapchain.err();
    renderer->m_swapchain = *swapchain;

    const auto window_size = renderer->m_window.get_extent();

    renderer->m_color_image = GpuImageAndView::create(vk, {
        .extent{window_size.width, window_size.height, 1},
        .format = Swapchain::SwapchainImageFormat,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        .sample_count = vk::SampleCountFlagBits::e4,
    });
    renderer->m_depth_image = GpuImageAndView::create(vk, {
        .extent{window_size.width, window_size.height, 1},
        .format = vk::Format::eD32Sfloat,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .aspect_flags = vk::ImageAspectFlagBits::eDepth,
        .sample_count = vk::SampleCountFlagBits::e4,
    });

    renderer->m_set_layout = DescriptorSetLayout::create(vk, {std::array{
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
    }});

    renderer->m_descriptor_pool = DescriptorPool::create(vk, {1, std::array{
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2}
    }});

    renderer->m_global_set = *renderer->m_descriptor_pool.allocate_set(vk, renderer->m_set_layout.get());;

    renderer->m_vp_buffer = GpuBuffer::create(vk, {
        sizeof(ViewProjectionUniform),
        vk::BufferUsageFlagBits::eUniformBuffer, GpuBuffer::RandomAccess
    });
    renderer->m_light_buffer = GpuBuffer::create(vk, {
        sizeof(LightUniform) * MaxLights,
        vk::BufferUsageFlagBits::eUniformBuffer, GpuBuffer::RandomAccess
    });

    write_uniform_buffer_descriptor(vk, {renderer->m_vp_buffer.get(), sizeof(ViewProjectionUniform)}, renderer->m_global_set, 0);
    write_uniform_buffer_descriptor(vk, {renderer->m_light_buffer.get(), sizeof(LightUniform)}, renderer->m_global_set, 1);

    ASSERT(renderer->m_global_set != nullptr);
    return renderer;
}

Result<void> DefaultRenderer::resize(Vk& vk) {
    const auto window_size = m_window.get_extent();

    auto swapchain_result = m_swapchain.resize(vk, m_surface.get());
    if (swapchain_result.has_err())
        return swapchain_result.err();

    m_color_image.destroy(vk);
    m_color_image = GpuImageAndView::create(vk, {
        .extent{window_size.width, window_size.height, 1},
        .format = Swapchain::SwapchainImageFormat,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        .sample_count = vk::SampleCountFlagBits::e4,
    });

    m_depth_image.destroy(vk);
    m_depth_image = GpuImageAndView::create(vk, {
        .extent{window_size.width, window_size.height, 1},
        .format = vk::Format::eD32Sfloat,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .aspect_flags = vk::ImageAspectFlagBits::eDepth,
        .sample_count = vk::SampleCountFlagBits::e4,
    });

    return ok();
}

void DefaultRenderer::destroy(Vk& vk) const {
    const auto wait_result = vk.queue.waitIdle();
    switch (wait_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    m_descriptor_pool.destroy(vk);
    m_set_layout.destroy(vk);

    m_light_buffer.destroy(vk);
    m_vp_buffer.destroy(vk);

    m_depth_image.destroy(vk);
    m_color_image.destroy(vk);

    m_swapchain.destroy(vk);
    m_surface.destroy(vk);
    m_window.destroy();
}

[[nodiscard]] Result<void> DefaultRenderer::draw(Vk& vk, const std::span<Pipeline*> pipelines) {
    const auto frame_result = [&]() -> Result<void> {
        const auto begin = m_swapchain.begin_frame(vk);
        if (begin.has_err())
            return begin.err();

        const auto cmd = begin->cmd;
        const auto render_target = begin->render_target;
        const auto extent = begin->extent;

        BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 2})
            .add_image_barrier(0, m_color_image.get_image(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_dst(0, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal)
            .add_image_barrier(1, m_depth_image.get_image(), {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1})
            .set_image_dst(1,
                vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthStencilAttachmentOptimal
            )
            .build_and_run(vk);

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
            .renderArea{{0, 0}, extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment,
            .pDepthAttachment = &depth_attachment,
        });

        cmd.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e4);
        cmd.setSampleMaskEXT(vk::SampleCountFlagBits::e4, vk::SampleMask{0xff});

        for (auto pipeline : pipelines) {
            pipeline->draw(cmd, m_global_set);
        }

        cmd.endRendering();

        BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 2})
            .add_image_barrier(0, m_color_image.get_image(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_src(0, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal)
            .set_image_dst(0, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
            .add_image_barrier(1, render_target, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_dst(1, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run(vk);

        const vk::ImageResolve2 resolve{
            .srcSubresource{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            .dstSubresource{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            .extent{extent.width, extent.height, 1},
        };
        cmd.resolveImage2({
            .srcImage = m_color_image.get_image(),
            .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
            .dstImage = render_target,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions = &resolve,
        });

        BarrierBuilder(vk, {.cmd = cmd, .image_barriers = 1})
            .add_image_barrier(0, render_target, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_src(0, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(0, vk::PipelineStageFlagBits2::eAllGraphics, vk::AccessFlagBits2::eNone, vk::ImageLayout::ePresentSrcKHR)
            .build_and_run(vk);

        m_light_queue.clear();

        const auto end = m_swapchain.end_frame(vk);
        if (end.has_err())
            return end.err();
        return ok();
    }();

    if (frame_result.has_err()) {
        switch (frame_result.err()) {
            case Err::FrameTimeout: return frame_result.err();
            case Err::InvalidWindow: {
                for (int w = 0, h = 0; w <= 1 || h <= 1; glfwGetWindowSize(m_window.get(), &w, &h)) {
                    glfwPollEvents();
                }

                const auto resize_result = resize(vk);
                if (resize_result.has_err()) {
                    LOGF_ERROR("Could not resize window: {}", to_string(resize_result.err()));
                    return resize_result.err();
                }

                resize(vk);
                break;
            }
            default: ERRORF("Unexpected error: {}", to_string(frame_result.err()));
        }
    }

    return ok();
}

void DefaultRenderer::update_camera_and_lights(Vk& vk, const Cameraf& camera) {
    const glm::mat4 view{camera.view()};

    ASSERT(m_light_queue.size() < MaxLights);
    LightUniform lights{.count = m_light_queue.size()};
    for (usize i = 0; i < m_light_queue.size(); ++i) {
        lights.vals[i] = {
            .position = view * m_light_queue[i].position,
            .color = m_light_queue[i].color,
        };
    }

    m_light_buffer.write(vk, lights);
    m_vp_buffer.write(vk, view, offsetof(ViewProjectionUniform, view));
}

SkyboxPipeline SkyboxPipeline::create(Vk& vk, const DefaultRenderer& renderer) {
    ASSERT(renderer.get_global_set_layout() != nullptr);

    SkyboxPipeline pipeline{};

    pipeline.m_set_layout = DescriptorSetLayout::create(vk, {{std::array{
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
    }}});

    std::array set_layouts{renderer.get_global_set_layout(), pipeline.m_set_layout.get()};

    const auto graphics_pipeline = GraphicsPipeline::create(vk, {
        .set_layouts = set_layouts,
        .push_ranges{},
        .vertex_shader_path = "../shaders/skybox.vert.spv",
        .fragment_shader_path = "../shaders/skybox.frag.spv",
    });
    if (graphics_pipeline.has_err())
        ERRORF("Could not create skybox shaders: {}", to_string(graphics_pipeline.err()));
    pipeline.m_pipeline= *graphics_pipeline;

    pipeline.m_descriptor_pool = DescriptorPool::create(vk, {1, std::array{
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1}
    }});

    pipeline.m_set = *pipeline.m_descriptor_pool.allocate_set(vk, pipeline.m_set_layout.get());;

    ASSERT(pipeline.m_set != nullptr);
    return pipeline;
}

Result<void> SkyboxPipeline::load_skybox(Vk& vk, const std::filesystem::path path) {
    ASSERT(!path.empty());

    const auto cubemap = Texture::from_cubemap_file(vk, path, {
        .format = vk::Format::eR8G8B8A8Srgb,
        .aspect_flags = vk::ImageAspectFlagBits::eColor,
        .sampler_type = Sampler::Linear,
    });
    if (cubemap.has_err())
        return cubemap.err();
    hg::write_image_sampler_descriptor(vk, *cubemap, m_set, 0);
    m_cubemap = *cubemap;

    const auto mesh = generate_cube();
    std::vector<glm::vec3> positions{};
    positions.reserve(mesh.vertices.size());
    for (const auto& vertex : mesh.vertices) {
        positions.emplace_back(vertex.position);
    }
    m_vertex_buffer = GpuBuffer::create(vk, {
        positions.size() * sizeof(positions[0]),
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });
    m_index_buffer = GpuBuffer::create(vk, {
        mesh.indices.size() * sizeof(mesh.indices[0]),
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });
    m_vertex_buffer.write_span(vk, std::span{positions}, 0);
    m_index_buffer.write_span(vk, std::span{mesh.indices}, 0);

    ASSERT(m_set != nullptr);
    return ok();
}

void SkyboxPipeline::destroy(Vk& vk) const {
    const auto wait_result = vk.queue.waitIdle();
    switch (wait_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    m_vertex_buffer.destroy(vk);
    m_index_buffer.destroy(vk);
    m_cubemap.destroy(vk);

    m_descriptor_pool.destroy(vk);
    m_pipeline.destroy(vk);
    m_set_layout.destroy(vk);
}

void SkyboxPipeline::draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) {
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

PbrPipeline PbrPipeline::create(Vk& vk, const DefaultRenderer& renderer) {
    ASSERT(renderer.get_global_set_layout() != nullptr);

    PbrPipeline pipeline{};

    pipeline.m_set_layout = DescriptorSetLayout::create(vk, {
        std::array{vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, MaxTextures, vk::ShaderStageFlagBits::eFragment},},
        std::array{vk::DescriptorBindingFlags{vk::DescriptorBindingFlagBits::ePartiallyBound}}
    });

    std::array set_layouts{renderer.get_global_set_layout(), pipeline.m_set_layout.get()};
    std::array push_ranges{vk::PushConstantRange{
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstant)
    }};

    const auto graphics_pipeline = GraphicsPipeline::create(vk, {
        .set_layouts = set_layouts,
        .push_ranges = push_ranges,
        .vertex_shader_path = "../shaders/pbr.vert.spv",
        .fragment_shader_path = "../shaders/pbr.frag.spv",
    });
    if (graphics_pipeline.has_err())
        ERROR("Could not find valid pbr shaders");
    pipeline.m_pipeline = *graphics_pipeline;

    pipeline.m_descriptor_pool = DescriptorPool::create(vk, {1, std::array{
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, MaxTextures}
    }});
    pipeline.m_texture_set = *pipeline.m_descriptor_pool.allocate_set(vk, pipeline.m_set_layout.get());;

    ASSERT(pipeline.m_texture_set != nullptr);
    return pipeline;
}

void PbrPipeline::destroy(Vk& vk) const {
    const auto wait_result = vk.queue.waitIdle();
    switch (wait_result) {
        case vk::Result::eSuccess: break;
        case vk::Result::eErrorOutOfHostMemory: ERROR("Vulkan ran out of host memory");
        case vk::Result::eErrorOutOfDeviceMemory: ERROR("Vulkan ran out of device memory");
        case vk::Result::eErrorDeviceLost: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    for (const auto& texture : m_textures) {
        texture.destroy(vk);
    }
    for (const auto& model : m_models) {
        model.destroy(vk);
    }
    m_descriptor_pool.destroy(vk);
    m_pipeline.destroy(vk);
    m_set_layout.destroy(vk);
}

void PbrPipeline::draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) {
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

    m_render_queue.clear();
}

Result<PbrPipeline::TextureHandle> PbrPipeline::load_texture(
    Vk& vk, std::filesystem::path path, const vk::Format format
) {
    ASSERT(m_textures.size() < MaxTextures);

    const auto texture = Texture::from_file(vk, path, {.format = format, .create_mips = true, .sampler_type = Sampler::Linear});
    if (texture.has_err())
        return texture.err();

    usize index = m_textures.size();
    write_image_sampler_descriptor(vk, *texture, m_texture_set, 0, to_u32(index));

    m_textures.emplace_back(*texture);
    return ok<TextureHandle>(index);
}

PbrPipeline::TextureHandle PbrPipeline::load_texture_from_data(
    Vk& vk, const GpuImage::Data& data, const vk::Format format
) {
    ASSERT(m_textures.size() < MaxTextures);

    const auto texture = Texture::from_data(vk, data, {.format = format, .create_mips = true, .sampler_type = Sampler::Linear});

    usize index = m_textures.size();
    write_image_sampler_descriptor(vk, texture, m_texture_set, 0, to_u32(index));

    m_textures.emplace_back(texture);
    return {index};
}

Result<PbrPipeline::ModelHandle> PbrPipeline::load_model(
    Vk& vk, const std::filesystem::path path,
    const TextureHandle normal_map, const TextureHandle texture
) {
    ASSERT(!path.empty());
    ASSERT(texture.index < m_textures.size());

    const auto model = GltfModelData::load_gltf(path);
    if (model.has_err())
        return model.err();

    return ok(load_model_from_data(vk, model->mesh, normal_map, texture, model->roughness, model->metalness));
}

PbrPipeline::ModelHandle PbrPipeline::load_model_from_data(
    Vk& vk,
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

    const auto index_buffer = GpuBuffer::create(vk, {
        data.indices.size() * sizeof(data.indices[0]),
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });
    const auto vertex_buffer = GpuBuffer::create(vk, {
        data.vertices.size() * sizeof(data.vertices[0]),
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });

    index_buffer.write_span(vk, std::span{data.indices});
    vertex_buffer.write_span(vk, std::span{data.vertices});

    m_models.emplace_back(
        to_u32(data.indices.size()),
        index_buffer, vertex_buffer,
        normal_map, texture,
        roughness, metalness
    );
    return {m_models.size() - 1};
}

} // namespace hg
