#include "hg_renderer.h"
#include "hg_vulkan.h"
#include <vulkan/vulkan_core.h>

namespace hg {

PbrRenderer create_pbr_renderer(Vk& vk, const PbrRendererConfig& config) {
    PbrRenderer renderer{};

    renderer.buffer_image = create_image_and_view(vk, {
        .extent{config.window.swapchain.extent.width, config.window.swapchain.extent.height, 1},
        .format = config.window.swapchain.format,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sample_count = VK_SAMPLE_COUNT_4_BIT,
    });
    renderer.depth_image = create_image_and_view(vk, {
        .extent{config.window.swapchain.extent.width, config.window.swapchain.extent.height, 1},
        .format = VK_FORMAT_D32_SFLOAT,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT,
        .sample_count = VK_SAMPLE_COUNT_4_BIT,
    });

    renderer.descriptor_layout = create_descriptor_set_layout(vk, {
        std::array<VkDescriptorSetLayoutBinding, 3>{{
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT},
            {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, config.max_textures, VK_SHADER_STAGE_FRAGMENT_BIT},
        }}, std::array<VkDescriptorBindingFlags, 3>{{
            {},
            {},
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        }}
    });

    constexpr VkPushConstantRange skybox_push{
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PbrRenderer::SkyboxPush)
    };
    const auto skybox_pipeline = create_graphics_pipeline(vk, {
        .set_layouts{&renderer.descriptor_layout, 1},
        .push_ranges{&skybox_push, 1},
        .vertex_shader_path = "shaders/skybox.vert.spv",
        .fragment_shader_path = "shaders/skybox.frag.spv",
    });
    if (skybox_pipeline.has_err())
        ERRORF("Could not create skybox shaders: {}", to_string(skybox_pipeline.err()));
    renderer.skybox_pipeline= *skybox_pipeline;

    constexpr VkPushConstantRange model_push{
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PbrRenderer::ModelPush)
    };
    const auto model_pipeline = create_graphics_pipeline(vk, {
        .set_layouts{&renderer.descriptor_layout, 1},
        .push_ranges{&model_push, 1},
        .vertex_shader_path = "shaders/pbr.vert.spv",
        .fragment_shader_path = "shaders/pbr.frag.spv",
    });
    if (model_pipeline.has_err())
        ERROR("Could not find valid pbr shaders");
    renderer.model_pipeline = *model_pipeline;

    renderer.descriptor_pool = create_descriptor_pool(vk, {1, std::array{
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, config.max_textures},
    }});

    renderer.descriptor_set = *allocate_descriptor_set(vk, renderer.descriptor_pool, renderer.descriptor_layout);

    renderer.vp_buffer = create_buffer(vk, {
        sizeof(PbrRenderer::ViewProjectionUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, GpuMemoryType::RandomAccess
    });
    renderer.light_buffer = create_buffer(vk, {
        sizeof(PbrRenderer::LightUniform) * PbrRenderer::MaxLights,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, GpuMemoryType::RandomAccess
    });

    write_uniform_buffer_descriptor(
        vk, {renderer.descriptor_set, 0}, {renderer.vp_buffer.handle, sizeof(PbrRenderer::ViewProjectionUniform)}
    );
    write_uniform_buffer_descriptor(
        vk, {renderer.descriptor_set, 1}, {renderer.light_buffer.handle, sizeof(PbrRenderer::LightUniform)}
    );

    std::array<glm::vec3, 24> positions{
        glm::vec3{-1.0f, -1.0f,  1.0f},
        glm::vec3{-1.0f, -1.0f, -1.0f},
        glm::vec3{ 1.0f, -1.0f, -1.0f},
        glm::vec3{ 1.0f, -1.0f,  1.0f},

        glm::vec3{-1.0f, -1.0f,  1.0f},
        glm::vec3{-1.0f,  1.0f,  1.0f},
        glm::vec3{-1.0f,  1.0f, -1.0f},
        glm::vec3{-1.0f, -1.0f, -1.0f},

        glm::vec3{-1.0f, -1.0f, -1.0f},
        glm::vec3{-1.0f,  1.0f, -1.0f},
        glm::vec3{ 1.0f,  1.0f, -1.0f},
        glm::vec3{ 1.0f, -1.0f, -1.0f},

        glm::vec3{ 1.0f, -1.0f, -1.0f},
        glm::vec3{ 1.0f,  1.0f, -1.0f},
        glm::vec3{ 1.0f,  1.0f,  1.0f},
        glm::vec3{ 1.0f, -1.0f,  1.0f},

        glm::vec3{ 1.0f, -1.0f,  1.0f},
        glm::vec3{ 1.0f,  1.0f,  1.0f},
        glm::vec3{-1.0f,  1.0f,  1.0f},
        glm::vec3{-1.0f, -1.0f,  1.0f},

        glm::vec3{-1.0f,  1.0f, -1.0f},
        glm::vec3{-1.0f,  1.0f,  1.0f},
        glm::vec3{ 1.0f,  1.0f,  1.0f},
        glm::vec3{ 1.0f,  1.0f, -1.0f},

    };
    std::array<u32, 36> indices{
         0,  1,  2,  2,  3,  0,
         4,  5,  6,  6,  7,  4,
         8,  9, 10, 10, 11,  8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20,
    };
    renderer.skybox.vertex_buffer = create_buffer(vk, {
        positions.size() * sizeof(positions[0]),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    renderer.skybox.index_buffer = create_buffer(vk, {
        indices.size() * sizeof(indices[0]),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    write_buffer(vk, renderer.skybox.vertex_buffer, positions.data(), positions.size() * sizeof(positions[0]));
    write_buffer(vk, renderer.skybox.index_buffer, indices.data(), indices.size() * sizeof(indices[0]));

    renderer.textures = malloc_slice<Pool<Texture>::Block>(config.max_textures);
    renderer.models = malloc_slice<Pool<PbrRenderer::Model>::Block>(config.max_models);

    return renderer;
}

void resize_pbr_renderer(Vk& vk, PbrRenderer& renderer, const Window& window) {
    destroy_image_and_view(vk, renderer.buffer_image);
    renderer.buffer_image = create_image_and_view(vk, {
        .extent{window.swapchain.extent.width, window.swapchain.extent.height, 1},
        .format = window.swapchain.format,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sample_count = VK_SAMPLE_COUNT_4_BIT,
    });

    destroy_image_and_view(vk, renderer.depth_image);
    renderer.depth_image = create_image_and_view(vk, {
        .extent{window.swapchain.extent.width, window.swapchain.extent.height, 1},
        .format = VK_FORMAT_D32_SFLOAT,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT,
        .sample_count = VK_SAMPLE_COUNT_4_BIT,
    });
}

void destroy_pbr_renderer(Vk& vk, PbrRenderer& renderer) {
    const auto wait_result = vkQueueWaitIdle(vk.queue);
    switch (wait_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    if (renderer.skybox.cubemap.index != PbrTextureHandle{}.index)
        unload_texture(vk, renderer, renderer.skybox.cubemap);

    destroy_buffer(vk, renderer.skybox.index_buffer);
    destroy_buffer(vk, renderer.skybox.vertex_buffer);

    destroy_buffer(vk, renderer.vp_buffer);
    destroy_buffer(vk, renderer.light_buffer);

    vkDestroyDescriptorPool(vk.device, renderer.descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(vk.device, renderer.descriptor_layout, nullptr);

    destroy_graphics_pipeline(vk, renderer.skybox_pipeline);
    destroy_graphics_pipeline(vk, renderer.model_pipeline);

    destroy_image_and_view(vk, renderer.buffer_image);
    destroy_image_and_view(vk, renderer.depth_image);

    free_slice(renderer.textures.release());
    free_slice(renderer.models.release());
}

static void draw_skybox(VkCommandBuffer cmd, PbrRenderer& renderer) {
    vkCmdSetDepthTestEnable(cmd, VK_FALSE);
    vkCmdSetDepthWriteEnable(cmd, VK_FALSE);
    vkCmdSetCullMode(cmd, VK_CULL_MODE_FRONT_BIT);

    bind_shaders(cmd, renderer.skybox_pipeline);
    vkCmdBindDescriptorSets(cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderer.skybox_pipeline.layout,
        0, 1, &renderer.descriptor_set,
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

    PbrRenderer::SkyboxPush push{
        .cubemap = renderer.skybox.cubemap,
    };
    vkCmdPushConstants(
        cmd, renderer.skybox_pipeline.layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(push), &push
    );

    draw_indexed(cmd, renderer.skybox.vertex_buffer.handle, renderer.skybox.index_buffer.handle, 36);

    vkCmdSetDepthTestEnable(cmd, VK_TRUE);
    vkCmdSetDepthWriteEnable(cmd, VK_TRUE);
    vkCmdSetCullMode(cmd, VK_CULL_MODE_BACK_BIT);
}

static void draw_models(VkCommandBuffer cmd, PbrRenderer& renderer, Slice<const PbrRenderer::ModelTicket> models) {
        constexpr std::array vertex_bindings = {
            VkVertexInputBindingDescription2EXT{
                .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
                .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, .divisor = 1
            }
        };
        constexpr std::array vertex_attributes = {
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

        bind_shaders(cmd, renderer.model_pipeline);
        vkCmdBindDescriptorSets(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer.model_pipeline.layout,
            0, 1, &renderer.descriptor_set,
            0, nullptr
        );

        for (const auto& ticket : models) {
            const auto& model = renderer.models[ticket.model];

            PbrRenderer::ModelPush model_push{
                .model = ticket.transform.matrix(),
                .normal_map_index = to_u32(model.normal_map.index),
                .texture_index = to_u32(model.color_map.index),
                .roughness = model.roughness,
                .metalness = model.metalness
            };
            vkCmdPushConstants(
                cmd, renderer.model_pipeline.layout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(model_push), &model_push
            );

            draw_indexed(cmd, model.vertex_buffer.handle, model.index_buffer.handle, model.index_count);
        }

}

Result<void> draw_pbr(Vk& vk, Window& window, PbrRenderer& renderer, Slice<const PbrRenderer::ModelTicket> models) {
    const auto frame_result = [&]() -> Result<void> {
        const auto begin = begin_frame(vk, window.swapchain);
        if (begin.has_err())
            return begin.err();
        const VkCommandBuffer cmd = *begin;

        BarrierBuilder(vk, {.image_barriers = 2})
            .add_image_barrier(0, renderer.buffer_image.image.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
            .set_image_dst(0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            .add_image_barrier(1, renderer.depth_image.image.image, {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1})
            .set_image_dst(1,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            )
            .build_and_run(vk, cmd);

        const VkRenderingAttachmentInfo color_attachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = renderer.buffer_image.view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };
        const VkRenderingAttachmentInfo depth_attachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = renderer.depth_image.view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue{
                .depthStencil{.depth = 1.0f, .stencil = 0}
            },
        };
        const VkRenderingInfo render_info{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea{{}, window.swapchain.extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment,
            .pDepthAttachment = &depth_attachment,
        };
        vkCmdBeginRendering(cmd, &render_info);

        const VkSampleMask sample_mask = 0xff;
        g_pfn.vkCmdSetSampleMaskEXT(cmd, VK_SAMPLE_COUNT_4_BIT, &sample_mask);
        g_pfn.vkCmdSetRasterizationSamplesEXT(cmd, VK_SAMPLE_COUNT_4_BIT);

        draw_skybox(cmd, renderer);
        draw_models(cmd, renderer, models);

        vkCmdEndRendering(cmd);

        BarrierBuilder(vk, {.image_barriers = 2})
            .add_image_barrier(0, renderer.buffer_image.image.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
            .set_image_src(0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            .set_image_dst(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, 
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            .add_image_barrier(1, window.swapchain.current_image(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
            .set_image_dst(1, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            .build_and_run(vk, cmd);

        resolve_image(cmd, {
            .image = window.swapchain.current_image(),
            .extent = {window.swapchain.extent.width, window.swapchain.extent.height, 1},
        }, {
            .image = renderer.buffer_image.image.image,
            .extent = renderer.buffer_image.image.extent,
        });

        BarrierBuilder(vk, {.image_barriers = 1})
            .add_image_barrier(0, window.swapchain.current_image(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
            .set_image_src(0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            .set_image_dst(0, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            .build_and_run(vk, cmd);

        const auto end = end_frame(vk, window.swapchain);
        if (end.has_err())
            return end.err();
        return ok();
    }();

    if (frame_result.has_err()) {
        switch (frame_result.err()) {
            case Err::FrameTimeout: return frame_result.err();
            case Err::InvalidWindow: {
                // properly handle window minimizing and resizing : TODO

                printf("resized\n");

                const auto window_result = resize_window(vk, window);
                if (window_result.has_err()) {
                    LOGF_ERROR("Could not resize window: {}", to_string(window_result.err()));
                    return window_result.err();
                }

                resize_pbr_renderer(vk, renderer, window);
                break;
            }
            default: ERRORF("Unexpected error: {}", to_string(frame_result.err()));
        }
    }

    return ok();
}

void update_projection(Vk& vk, const PbrRenderer& renderer, const glm::mat4& projection) {
    write_buffer(
        vk,
        renderer.vp_buffer,
        &projection,
        sizeof(projection),
        offsetof(PbrRenderer::ViewProjectionUniform, projection)
    );
}

PbrRenderer::Light make_light(glm::vec3 position, glm::vec3 color, f32 intensity) {
    color *= intensity;
    return {
        .position = glm::vec4{position, 1.0f},
        .color = glm::vec4{color, 1.0f},
    };
}

void update_camera_and_lights(Vk& vk, PbrRenderer& renderer, const PbrLightData& config) {
    const glm::mat4 view = config.camera.view();

    ASSERT(config.lights.count < PbrRenderer::MaxLights);
    PbrRenderer::LightUniform lights{.count = config.lights.count};
    for (usize i = 0; i < config.lights.count; ++i) {
        lights.vals[i] = {
            .position = view * config.lights[i].position,
            .color = config.lights[i].color,
        };
    }

    write_buffer(vk, renderer.light_buffer, &lights, sizeof(lights));
    write_buffer(vk, renderer.vp_buffer, &view, sizeof(view), offsetof(PbrRenderer::ViewProjectionUniform, view));
}

PbrTextureHandle load_texture(Vk& vk, PbrRenderer& renderer, const ImageData& data, const VkFormat format) {
    ASSERT(data.pixels != nullptr);

    PbrTextureHandle texture = renderer.textures.alloc();
    renderer.textures[texture] = create_texture(vk, data, {
        .format = format,
        .sampler_type = SamplerType::Linear
    });
    write_image_sampler_descriptor(vk, {renderer.descriptor_set, 2, to_u32(texture.index)}, renderer.textures[texture]);
    return texture;
}

void unload_texture(Vk& vk, PbrRenderer& renderer, const PbrTextureHandle texture) {
    destroy_texture(vk, renderer.textures[texture]);
    renderer.textures.dealloc(texture);
}

void load_skybox(Vk& vk, PbrRenderer& renderer, const ImageData& cubemap) {
    ASSERT(renderer.skybox.cubemap.index == PbrTextureHandle{}.index);
    ASSERT(cubemap.pixels != nullptr);

    renderer.skybox.cubemap = renderer.textures.alloc();
    renderer.textures[renderer.skybox.cubemap] = create_texture_cubemap(vk, cubemap, {
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .sampler_type = SamplerType::Linear,
    });
    write_image_sampler_descriptor(vk, {
        renderer.descriptor_set, 2, to_u32(renderer.skybox.cubemap.index)
    }, renderer.textures[renderer.skybox.cubemap]);
}

void unload_skybox(Vk& vk, PbrRenderer& renderer) {
    unload_texture(vk, renderer, renderer.skybox.cubemap);
}

PbrModelHandle load_model(Vk& vk, PbrRenderer& renderer, const PbrModelConfig& config) {
    ASSERT(config.data.roughness >= 0.0 && config.data.roughness <= 1.0);
    ASSERT(config.data.metalness >= 0.0 && config.data.metalness <= 1.0);

    const auto index_buffer = create_buffer(vk, {
        config.data.mesh.indices.count * sizeof(config.data.mesh.indices[0]),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    const auto vertex_buffer = create_buffer(vk, {
        config.data.mesh.vertices.count * sizeof(config.data.mesh.vertices[0]),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });

    write_buffer(
        vk,
        index_buffer,
        config.data.mesh.indices.data,
        config.data.mesh.indices.count * sizeof(config.data.mesh.indices[0])
    );
    write_buffer(
        vk,
        vertex_buffer,
        config.data.mesh.vertices.data,
        config.data.mesh.vertices.count * sizeof(config.data.mesh.vertices[0])
    );

    auto model = renderer.models.alloc();
    renderer.models[model] = PbrRenderer::Model{
        .index_count = to_u32(config.data.mesh.indices.count),
        .index_buffer = index_buffer,
        .vertex_buffer = vertex_buffer,
        .normal_map = config.normal_map,
        .color_map = config.color_map,
        .roughness = config.data.roughness,
        .metalness = config.data.metalness,
    };
    return model;
}

void unload_model(Vk& vk, PbrRenderer& renderer, const PbrModelHandle model) {
    auto& data = renderer.models[model];
    destroy_buffer(vk, data.index_buffer);
    destroy_buffer(vk, data.vertex_buffer);
    renderer.models.dealloc(model);
}

} // namespace hg
