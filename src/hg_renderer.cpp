#include "hg_renderer.h"
#include "hg_vulkan.h"
#include <vulkan/vulkan_core.h>

namespace hg {

Result<DefaultRenderer> DefaultRenderer::create(Engine& engine, const Config& config) {
    if (!config.fullscreen) {
        ASSERT(config.window_size.x > 0);
        ASSERT(config.window_size.y > 0);
    }

    auto renderer = ok<DefaultRenderer>();

    renderer->m_window = Window::create(config.fullscreen, config.window_size);
    renderer->m_surface = create_surface(engine.vk, renderer->m_window.get());

    const auto swapchain = create_swapchain(engine.vk, renderer->m_surface);
    if (swapchain.has_err())
        return swapchain.err();
    renderer->m_swapchain = *swapchain;

    const glm::ivec2 window_size = renderer->m_window.get_extent();

    renderer->m_color_image = create_image_and_view(engine.vk, {
        .extent{to_u32(window_size.x), to_u32(window_size.y), 1},
        .format = renderer->m_swapchain.format,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sample_count = VK_SAMPLE_COUNT_4_BIT,
    });
    renderer->m_depth_image = create_image_and_view(engine.vk, {
        .extent{to_u32(window_size.x), to_u32(window_size.y), 1},
        .format = VK_FORMAT_D32_SFLOAT,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT,
        .sample_count = VK_SAMPLE_COUNT_4_BIT,
    });

    renderer->m_set_layout = create_descriptor_set_layout(engine.vk, {std::array{
        VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT},
        VkDescriptorSetLayoutBinding{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
    }});

    renderer->m_descriptor_pool = create_descriptor_pool(engine.vk, {1, std::array{
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2}
    }});

    renderer->m_global_set = *allocate_descriptor_set(engine.vk, renderer->m_descriptor_pool, renderer->m_set_layout);

    renderer->m_vp_buffer = create_buffer(engine.vk, {
        sizeof(ViewProjectionUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, GpuMemoryType::RandomAccess
    });
    renderer->m_light_buffer = create_buffer(engine.vk, {
        sizeof(LightUniform) * MaxLights,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, GpuMemoryType::RandomAccess
    });

    write_uniform_buffer_descriptor(
        engine.vk, {renderer->m_global_set, 0}, {renderer->m_vp_buffer.buffer, sizeof(ViewProjectionUniform)}
    );
    write_uniform_buffer_descriptor(
        engine.vk, {renderer->m_global_set, 1}, {renderer->m_light_buffer.buffer, sizeof(LightUniform)}
    );

    ASSERT(renderer->m_global_set != nullptr);
    return renderer;
}

Result<void> DefaultRenderer::resize(Engine& engine) {
    const auto window_size = m_window.get_extent();

    auto swapchain_result = resize_swapchain(engine.vk, m_swapchain, m_surface);
    if (swapchain_result.has_err())
        return swapchain_result.err();

    destroy_image_and_view(engine.vk, m_color_image);
    m_color_image = create_image_and_view(engine.vk, {
        .extent{to_u32(window_size.x), to_u32(window_size.y), 1},
        .format = m_swapchain.format,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sample_count = VK_SAMPLE_COUNT_4_BIT,
    });

    destroy_image_and_view(engine.vk, m_depth_image);
    m_depth_image = create_image_and_view(engine.vk, {
        .extent{to_u32(window_size.x), to_u32(window_size.y), 1},
        .format = VK_FORMAT_D32_SFLOAT,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT,
        .sample_count = VK_SAMPLE_COUNT_4_BIT,
    });

    return ok();
}

void DefaultRenderer::destroy(Engine& engine) const {
    const auto wait_result = vkQueueWaitIdle(engine.vk.queue);
    switch (wait_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    vkDestroyDescriptorPool(engine.vk.device, m_descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(engine.vk.device, m_set_layout, nullptr);

    destroy_buffer(engine.vk, m_light_buffer);
    destroy_buffer(engine.vk, m_vp_buffer);

    destroy_image_and_view(engine.vk, m_depth_image);
    destroy_image_and_view(engine.vk, m_color_image);

    destroy_swapchain(engine.vk, m_swapchain);
    vkDestroySurfaceKHR(engine.vk.instance, m_surface, nullptr);
    m_window.destroy();
}

[[nodiscard]] Result<void> DefaultRenderer::draw(Engine& engine, const Slice<Pipeline*> pipelines) {
    const auto frame_result = [&]() -> Result<void> {
        const auto begin = begin_frame(engine.vk, m_swapchain);
        if (begin.has_err())
            return begin.err();

        const VkCommandBuffer cmd = *begin;
        const VkImage render_target = m_swapchain.current_image();
        const VkExtent2D extent = m_swapchain.extent;

        BarrierBuilder(engine.vk, {.image_barriers = 2})
            .add_image_barrier(0, m_color_image.image.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
            .set_image_dst(0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            .add_image_barrier(1, m_depth_image.image.image, {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1})
            .set_image_dst(1,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            )
            .build_and_run(engine.vk, cmd);

        const VkRenderingAttachmentInfo color_attachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_color_image.view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue{
                .color{{0.0f, 0.0f, 0.0f, 1.0f}}
            },
        };
        const VkRenderingAttachmentInfo depth_attachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_depth_image.view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue{
                .depthStencil{.depth = 1.0f, .stencil = 0}
            },
        };
        const VkRenderingInfo render_info{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea{{}, extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment,
            .pDepthAttachment = &depth_attachment,
        };
        vkCmdBeginRendering(cmd, &render_info);

        const VkSampleMask sample_mask = 0xff;
        g_pfn.vkCmdSetSampleMaskEXT(cmd, VK_SAMPLE_COUNT_4_BIT, &sample_mask);
        g_pfn.vkCmdSetRasterizationSamplesEXT(cmd, VK_SAMPLE_COUNT_4_BIT);

        for (auto pipeline : pipelines) {
            pipeline->draw(*this, cmd);
        }

        vkCmdEndRendering(cmd);

        BarrierBuilder(engine.vk, {.image_barriers = 2})
            .add_image_barrier(0, m_color_image.image.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
            .set_image_src(0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            .set_image_dst(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            .add_image_barrier(1, render_target, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
            .set_image_dst(1, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            .build_and_run(engine.vk, cmd);

        const VkImageResolve2 resolve{
            .sType = VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2,
            .srcSubresource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
            .dstSubresource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
            .extent{extent.width, extent.height, 1},
        };
        const VkResolveImageInfo2 resolve_info{
            .sType = VK_STRUCTURE_TYPE_RESOLVE_IMAGE_INFO_2,
            .srcImage = m_color_image.image.image,
            .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .dstImage = render_target,
            .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .regionCount = 1,
            .pRegions = &resolve,
        };
        vkCmdResolveImage2(cmd, &resolve_info);

        BarrierBuilder(engine.vk, {.image_barriers = 1})
            .add_image_barrier(0, render_target, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
            .set_image_src(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            .set_image_dst(0, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            .build_and_run(engine.vk, cmd);

        m_light_queue.clear();

        const auto end = end_frame(engine.vk, m_swapchain);
        if (end.has_err())
            return end.err();
        return ok();
    }();

    if (frame_result.has_err()) {
        switch (frame_result.err()) {
            case Err::FrameTimeout: return frame_result.err();
            case Err::InvalidWindow: {
                // TODO: handle window minimizing and resizing

                const auto resize_result = resize(engine);
                if (resize_result.has_err()) {
                    LOGF_ERROR("Could not resize window: {}", to_string(resize_result.err()));
                    return resize_result.err();
                }
                break;
            }
            default: ERRORF("Unexpected error: {}", to_string(frame_result.err()));
        }
    }

    return ok();
}

void DefaultRenderer::update_camera_and_lights(Engine& engine, const Cameraf& camera) {
    const glm::mat4 view{camera.view()};

    ASSERT(m_light_queue.size() < MaxLights);
    LightUniform lights{.count = m_light_queue.size()};
    for (usize i = 0; i < m_light_queue.size(); ++i) {
        lights.vals[i] = {
            .position = view * m_light_queue[i].position,
            .color = m_light_queue[i].color,
        };
    }

    write_buffer(engine.vk, m_light_buffer, &lights, sizeof(lights));
    write_buffer(engine.vk, m_vp_buffer, &view, sizeof(view), offsetof(ViewProjectionUniform, view));
}

SkyboxPipeline SkyboxPipeline::create(Engine& engine, const DefaultRenderer& renderer) {
    ASSERT(renderer.get_global_set_layout() != nullptr);

    SkyboxPipeline pipeline{};

    pipeline.m_set_layout = create_descriptor_set_layout(engine.vk, {{std::array{
        VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
    }}});

    std::array set_layouts{renderer.get_global_set_layout(), pipeline.m_set_layout};

    const auto graphics_pipeline = create_graphics_pipeline(engine.vk, {
        .set_layouts = set_layouts,
        .push_ranges{},
        .vertex_shader_path = "shaders/skybox.vert.spv",
        .fragment_shader_path = "shaders/skybox.frag.spv",
    });
    if (graphics_pipeline.has_err())
        ERRORF("Could not create skybox shaders: {}", to_string(graphics_pipeline.err()));
    pipeline.m_pipeline= *graphics_pipeline;

    pipeline.m_descriptor_pool = create_descriptor_pool(engine.vk, {1, std::array{
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    }});

    pipeline.m_set = *allocate_descriptor_set(engine.vk, pipeline.m_descriptor_pool, pipeline.m_set_layout);

    ASSERT(pipeline.m_set != nullptr);
    return pipeline;
}

void SkyboxPipeline::destroy(Engine& engine) const {
    const VkResult wait_result = vkQueueWaitIdle(engine.vk.queue);
    switch (wait_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    if (m_vertex_buffer.buffer != nullptr)
        destroy_buffer(engine.vk, m_vertex_buffer);
    if (m_index_buffer.buffer != nullptr)
        destroy_buffer(engine.vk, m_index_buffer);
    if (m_cubemap.image.image.image != nullptr)
        destroy_texture(engine.vk, m_cubemap);

    vkDestroyDescriptorPool(engine.vk.device, m_descriptor_pool, nullptr);
    destroy_graphics_pipeline(engine.vk, m_pipeline);
    vkDestroyDescriptorSetLayout(engine.vk.device, m_set_layout, nullptr);
}

void SkyboxPipeline::draw(const DefaultRenderer& renderer, const VkCommandBuffer cmd) {
    ASSERT(m_set != nullptr);
    ASSERT(cmd != nullptr);

    vkCmdSetDepthTestEnable(cmd, VK_FALSE);
    vkCmdSetDepthWriteEnable(cmd, VK_FALSE);
    vkCmdSetCullMode(cmd, VK_CULL_MODE_FRONT_BIT);

    bind_shaders(cmd, m_pipeline);
    const std::array descriptor_sets{renderer.get_global_set(), m_set};
    vkCmdBindDescriptorSets(cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipeline.layout,
        0, to_u32(descriptor_sets.size()), descriptor_sets.data(),
        0, nullptr
    );

    const std::array vertex_bindings = {
        VkVertexInputBindingDescription2EXT{
            .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
            .stride = sizeof(glm::vec3), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, .divisor = 1
        }
    };
    const std::array vertex_attributes = {
        VkVertexInputAttributeDescription2EXT{
            .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
            .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT
        },
    };
    g_pfn.vkCmdSetVertexInputEXT(cmd,
        to_u32(vertex_bindings.size()),
        vertex_bindings.data(),
        to_u32(vertex_attributes.size()),
        vertex_attributes.data()
    );

    const std::array vertex_buffers = {m_vertex_buffer.buffer};
    const std::array offsets = {usize{0}};
    vkCmdBindVertexBuffers(cmd, 0, to_u32(vertex_buffers.size()), vertex_buffers.data(), offsets.data());

    vkCmdBindIndexBuffer(cmd, m_index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 1);

    vkCmdSetDepthTestEnable(cmd, VK_TRUE);
    vkCmdSetDepthWriteEnable(cmd, VK_TRUE);
    vkCmdSetCullMode(cmd, VK_CULL_MODE_NONE);
}

void SkyboxPipeline::load_skybox(Engine& engine, const ImageData& data) {
    ASSERT(data.pixels != nullptr);

    m_cubemap = create_texture_cubemap(engine.vk, data, {
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT,
        .sampler_type = SamplerType::Linear,
    });
    hg::write_image_sampler_descriptor(engine.vk, {m_set, 0}, m_cubemap);

    std::array<glm::vec3, 24> positions{
        glm::vec3{-1.0f, -1.0f,  1.0f}, glm::vec3{-1.0f, -1.0f, -1.0f}, glm::vec3{ 1.0f, -1.0f, -1.0f}, glm::vec3{ 1.0f, -1.0f,  1.0f},
        glm::vec3{-1.0f, -1.0f,  1.0f}, glm::vec3{-1.0f,  1.0f,  1.0f}, glm::vec3{-1.0f,  1.0f, -1.0f}, glm::vec3{-1.0f, -1.0f, -1.0f},
        glm::vec3{-1.0f, -1.0f, -1.0f}, glm::vec3{-1.0f,  1.0f, -1.0f}, glm::vec3{ 1.0f,  1.0f, -1.0f}, glm::vec3{ 1.0f, -1.0f, -1.0f},
        glm::vec3{ 1.0f, -1.0f, -1.0f}, glm::vec3{ 1.0f,  1.0f, -1.0f}, glm::vec3{ 1.0f,  1.0f,  1.0f}, glm::vec3{ 1.0f, -1.0f,  1.0f},
        glm::vec3{ 1.0f, -1.0f,  1.0f}, glm::vec3{ 1.0f,  1.0f,  1.0f}, glm::vec3{-1.0f,  1.0f,  1.0f}, glm::vec3{-1.0f, -1.0f,  1.0f},
        glm::vec3{-1.0f,  1.0f, -1.0f}, glm::vec3{-1.0f,  1.0f,  1.0f}, glm::vec3{ 1.0f,  1.0f,  1.0f}, glm::vec3{ 1.0f,  1.0f, -1.0f},
    };
    std::array<u32, 36> indices{
         0,  1,  2,  2,  3,  0,
         4,  5,  6,  6,  7,  4,
         8,  9, 10, 10, 11,  8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20,
    };
    m_vertex_buffer = create_buffer(engine.vk, {
        positions.size() * sizeof(positions[0]),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    m_index_buffer = create_buffer(engine.vk, {
        indices.size() * sizeof(indices[0]),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    write_buffer(engine.vk, m_vertex_buffer, positions.data(), positions.size() * sizeof(positions[0]));
    write_buffer(engine.vk, m_index_buffer, indices.data(), indices.size() * sizeof(indices[0]));
}

Result<void> SkyboxPipeline::load_skybox(Engine& engine, const std::filesystem::path path) {
    ASSERT(!path.empty());

    auto image = engine.loader.load_image(path);
    if (image.has_err())
        return image.err();
    defer(engine.loader.unload_image(*image));

    load_skybox(engine, engine.loader.get(*image));
    return ok();
}

PbrPipeline PbrPipeline::create(Engine& engine, const DefaultRenderer& renderer) {
    ASSERT(renderer.get_global_set_layout() != nullptr);

    PbrPipeline pipeline{};

    pipeline.m_set_layout = create_descriptor_set_layout(engine.vk, {
        std::array{VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxTextures, VK_SHADER_STAGE_FRAGMENT_BIT}},
        std::array{VkDescriptorBindingFlags{VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT}}
    });

    std::array set_layouts{renderer.get_global_set_layout(), pipeline.m_set_layout};
    std::array push_ranges{VkPushConstantRange{
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant)
    }};

    const auto graphics_pipeline = create_graphics_pipeline(engine.vk, {
        .set_layouts = set_layouts,
        .push_ranges = push_ranges,
        .vertex_shader_path = "shaders/pbr.vert.spv",
        .fragment_shader_path = "shaders/pbr.frag.spv",
    });
    if (graphics_pipeline.has_err())
        ERROR("Could not find valid pbr shaders");
    pipeline.m_pipeline = *graphics_pipeline;

    pipeline.m_descriptor_pool = create_descriptor_pool(engine.vk, {1, std::array{
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxTextures}
    }});
    pipeline.m_texture_set = *allocate_descriptor_set(engine.vk, pipeline.m_descriptor_pool, pipeline.m_set_layout);

    ASSERT(pipeline.m_texture_set != nullptr);
    return pipeline;
}

void PbrPipeline::destroy(Engine& engine) const {
    const auto wait_result = vkQueueWaitIdle(engine.vk.queue);
    switch (wait_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    for (const auto& texture : m_textures) {
        destroy_texture(engine.vk, texture);
    }
    for (const auto& model : m_models) {
        model.destroy(engine);
    }
    vkDestroyDescriptorPool(engine.vk.device, m_descriptor_pool, nullptr);
    destroy_graphics_pipeline(engine.vk, m_pipeline);
    vkDestroyDescriptorSetLayout(engine.vk.device, m_set_layout, nullptr);
}

void PbrPipeline::draw(const DefaultRenderer& renderer, const VkCommandBuffer cmd) {
    ASSERT(cmd != nullptr);

    vkCmdSetCullMode(cmd, VK_CULL_MODE_BACK_BIT);

    std::array vertex_bindings = {
        VkVertexInputBindingDescription2EXT{
            .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
            .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, .divisor = 1
        }
    };
    std::array vertex_attributes = {
        VkVertexInputAttributeDescription2EXT{
           .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
           .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, position)
       },
        VkVertexInputAttributeDescription2EXT{
           .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
           .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal)
       },
        VkVertexInputAttributeDescription2EXT{
           .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
           .location = 2, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Vertex, tangent)
       },
        VkVertexInputAttributeDescription2EXT{
           .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
           .location = 3, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, tex_coord)
       },
    };
    g_pfn.vkCmdSetVertexInputEXT(cmd,
        to_u32(vertex_bindings.size()), vertex_bindings.data(),
        to_u32(vertex_attributes.size()), vertex_attributes.data()
    );

    bind_shaders(cmd, m_pipeline);
    std::array descriptor_sets = {renderer.get_global_set(), m_texture_set};
    vkCmdBindDescriptorSets(cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipeline.layout,
        0, to_u32(descriptor_sets.size()), descriptor_sets.data(),
        0, nullptr
    );

    for (const auto& ticket : m_render_queue) {
        const auto& model = m_models[ticket.model.index];

        PushConstant model_push{
            .model = ticket.transform.matrix(),
            .normal_map_index = to_u32(model.normal_map.index),
            .texture_index = to_u32(model.texture.index),
            .roughness = model.roughness,
            .metalness = model.metalness
        };
        vkCmdPushConstants(
            cmd, m_pipeline.layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(model_push), &model_push
        );

        std::array vertex_buffers = {model.vertex_buffer.buffer};
        std::array offsets = {usize{0}};
        vkCmdBindVertexBuffers(cmd, 0, to_u32(vertex_buffers.size()), vertex_buffers.data(), offsets.data());

        vkCmdBindIndexBuffer(cmd, model.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, model.index_count, 1, 0, 0, 1);
    }

    vkCmdSetCullMode(cmd, VK_CULL_MODE_NONE);

    m_render_queue.clear();
}

PbrPipeline::TextureHandle PbrPipeline::load_texture(
    Engine& engine, const ImageData& data, const VkFormat format
) {
    ASSERT(m_textures.size() < MaxTextures);

    const auto texture = create_texture(engine.vk, data, {
        .format = format,
        .create_mips = true,
        .sampler_type = SamplerType::Linear
    });

    usize index = m_textures.size();
    write_image_sampler_descriptor(engine.vk, {m_texture_set, 0, to_u32(index)}, texture);

    m_textures.emplace_back(texture);
    return {index};
}

Result<PbrPipeline::TextureHandle> PbrPipeline::load_texture(
    Engine& engine, const std::filesystem::path path, const VkFormat format
) {
    ASSERT(!path.empty());

    auto image = engine.loader.load_image(path);
    if (image.has_err())
        return image.err();
    defer(engine.loader.unload_image(*image));

    return ok(load_texture(engine, engine.loader.get(*image), format));
}

PbrPipeline::ModelHandle PbrPipeline::load_model(
    Engine& engine,
    const GltfData& data,
    const TextureHandle normal_map,
    const TextureHandle texture
) {
    ASSERT(texture.index < m_textures.size());
    ASSERT(data.roughness >= 0.0 && data.roughness <= 1.0);
    ASSERT(data.metalness >= 0.0 && data.metalness <= 1.0);

    const auto index_buffer = create_buffer(engine.vk, {
        data.mesh.indices.count * sizeof(data.mesh.indices[0]),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    const auto vertex_buffer = create_buffer(engine.vk, {
        data.mesh.vertices.count * sizeof(data.mesh.vertices[0]),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });

    write_buffer(
        engine.vk, index_buffer, data.mesh.indices.data, data.mesh.indices.count * sizeof(data.mesh.indices[0])
    );
    write_buffer(
        engine.vk, vertex_buffer, data.mesh.vertices.data, data.mesh.vertices.count * sizeof(data.mesh.vertices[0])
    );

    m_models.emplace_back(
        to_u32(data.mesh.indices.count),
        index_buffer, vertex_buffer,
        normal_map, texture,
        data.roughness, data.metalness
    );
    return {m_models.size() - 1};
}

Result<PbrPipeline::ModelHandle> PbrPipeline::load_model(
    Engine& engine,
    const std::filesystem::path path,
    const TextureHandle normal_map,
    const TextureHandle texture
) {
    ASSERT(!path.empty());
    ASSERT(texture.index < m_textures.size());

    auto model = engine.loader.load_gltf(path);
    if (model.has_err())
        return model.err();
    defer(engine.loader.unload_gltf(*model));

    return ok(load_model(engine, engine.loader.get(*model), normal_map, texture));
}

} // namespace hg
