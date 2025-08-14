#include "hg_renderer.h"

namespace hg {

Result<Window> create_window(Vk& vk, const WindowConfig& config) {
    if (config.windowed)
        ASSERT(config.size.x > 0 && config.size.y > 0);

    auto window = ok<Window>();

    if (config.windowed) {
        window->window = SDL_CreateWindow(
            "Hurdy Gurdy",
            config.size.x, config.size.y,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
        );
    } else {
        SDL_DisplayID display = SDL_GetPrimaryDisplay();
        if (display == 0)
            ERRORF("Could not get primary display: {}", SDL_GetError());

        int count = 0;
        SDL_DisplayMode** mode = SDL_GetFullscreenDisplayModes(display, &count);
        if (mode == nullptr)
            ERRORF("Could not get display modes: {}", SDL_GetError());
        if (count == 0)
            ERROR("No fullscreen modes available");

        window->window = SDL_CreateWindow(
            "Hurdy Gurdy",
            mode[0]->w, mode[0]->h,
            SDL_WINDOW_FULLSCREEN | SDL_WINDOW_VULKAN
        );
    }
    if (window->window == nullptr)
        ERRORF("Could not create window: {}", SDL_GetError());

    window->surface = create_surface(vk, window->window);

    auto swapchain = create_swapchain(vk, window->surface);
    if (swapchain.has_err())
        ERRORF("Could not create swapchain: {}", to_string(swapchain.err()));
    window->swapchain = *swapchain;

    return window;
}

Result<void> resize_window(Vk& vk, Window& window) {
    ASSERT(window.window != nullptr);
    ASSERT(window.surface != nullptr);
    ASSERT(window.swapchain.swapchain != nullptr);

    auto swapchain = resize_swapchain(vk, window.swapchain, window.surface);
    if (swapchain.has_err())
        return swapchain.err();
    return ok();
}

void destroy_window(Vk& vk, Window& window) {
    ASSERT(window.window != nullptr);
    ASSERT(window.surface != nullptr);
    ASSERT(window.swapchain.swapchain != nullptr);

    destroy_swapchain(vk, window.swapchain);
    vkDestroySurfaceKHR(vk.instance, window.surface, nullptr);
    SDL_DestroyWindow(window.window);
}

struct ViewProjectionUniform {
    glm::mat4 projection;
    glm::mat4 view;
};

struct LightUniform {
    alignas(16) usize count;
    alignas(16) PbrLight vals[PbrMaxLights];
};

struct SkyboxPush {
    u32 cubemap;
};

struct ModelPush {
    glm::mat4 model;
    u32 normal_map_index;
    u32 texture_index;
    float roughness;
    float metalness;
};

struct AntialiasPush {
    glm::vec2 pixel_size;
    u32 input_index;
};

struct TonemapPush {
    u32 input_index;
};

static void create_pbr_renderer_images(PbrRenderer& renderer, VkExtent2D extent) {
    Vk& vk = *renderer.vk;

    GpuImage depth_image = create_gpu_image(vk, {
        .extent{extent.width, extent.height, 1},
        .format = VK_FORMAT_D32_SFLOAT,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    });
    VkImageView depth_image_view = create_gpu_image_view(vk, {
        .image = depth_image.handle,
        .format = VK_FORMAT_D32_SFLOAT,
        .aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT,
    });
    VkSampler depth_sampler = create_sampler(vk, {
        .type = SamplerType::SamplerLinear,
        .edge_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    });
    renderer.textures[renderer.depth_image] = {depth_image, depth_image_view, depth_sampler};

    for (const auto& image : renderer.color_images) {
        GpuImage color_image = create_gpu_image(vk, {
            .extent{extent.width, extent.height, 1},
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                   | VK_IMAGE_USAGE_SAMPLED_BIT
                   | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        });
        VkImageView color_image_view = create_gpu_image_view(vk, {
            .image = color_image.handle,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT,
        });
        VkSampler color_sampler = create_sampler(vk, {
            .type = SamplerType::SamplerLinear,
            .edge_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        });
        renderer.textures[image] = {color_image, color_image_view, color_sampler};

        write_image_sampler_descriptor(vk, {
            renderer.descriptor_set, 2, to_u32(image.index)
        }, renderer.textures[image].view, renderer.textures[image].sampler);
    }
}

static void destroy_pbr_renderer_images(PbrRenderer& renderer) {
    Vk& vk = *renderer.vk;

    for (auto& image : renderer.color_images) {
        destroy_gpu_image(vk, renderer.textures[image].image);
        vkDestroyImageView(vk.device, renderer.textures[image].view, nullptr);
        vkDestroySampler(vk.device, renderer.textures[image].sampler, nullptr);
    }
    destroy_gpu_image(vk, renderer.textures[renderer.depth_image].image);
    vkDestroyImageView(vk.device, renderer.textures[renderer.depth_image].view, nullptr);
    vkDestroySampler(vk.device, renderer.textures[renderer.depth_image].sampler, nullptr);
}

PbrRenderer create_pbr_renderer(Vk& vk, const PbrRendererConfig& config) {
    PbrRenderer renderer;
    renderer.vk = &vk;

    const VkDescriptorSetLayoutBinding bindings[]{
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, config.max_textures, VK_SHADER_STAGE_FRAGMENT_BIT},
    };
    const VkDescriptorBindingFlags flags[]{
        {},
        {},
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
    };
    renderer.descriptor_layout = create_descriptor_set_layout(vk, {
        {bindings, std::size(bindings)},
        {flags, std::size(flags)}
    });

    const VkDescriptorPoolSize pool_sizes[]{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, config.max_textures},
    };
    renderer.descriptor_pool = create_descriptor_pool(vk, {1, {pool_sizes, std::size(pool_sizes)}});

    renderer.descriptor_set = *allocate_descriptor_set(vk, renderer.descriptor_pool, renderer.descriptor_layout);

    constexpr VkPushConstantRange skybox_push{
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyboxPush)
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
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ModelPush)
    };
    const auto model_pipeline = create_graphics_pipeline(vk, {
        .set_layouts{&renderer.descriptor_layout, 1},
        .push_ranges{&model_push, 1},
        .vertex_shader_path = "shaders/pbr.vert.spv",
        .fragment_shader_path = "shaders/pbr.frag.spv",
    });
    if (model_pipeline.has_err())
        ERRORF("Could not find valid pbr shaders: {}", to_string(model_pipeline.err()));
    renderer.model_pipeline = *model_pipeline;

    constexpr VkPushConstantRange tonemap_push{
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TonemapPush)
    };
    const auto tonemap_pipeline = create_graphics_pipeline(vk, {
        .set_layouts{&renderer.descriptor_layout, 1},
        .push_ranges{&tonemap_push, 1},
        .vertex_shader_path = "shaders/fullscreen.vert.spv",
        .fragment_shader_path = "shaders/tonemap.frag.spv",
    });
    if (tonemap_pipeline.has_err())
        ERRORF("Could not find valid tonemap shaders: {}", to_string(tonemap_pipeline.err()));
    renderer.tonemap_pipeline = *tonemap_pipeline;

    constexpr VkPushConstantRange antialias_push{
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(AntialiasPush)
    };
    const auto antialias_pipeline = create_graphics_pipeline(vk, {
        .set_layouts{&renderer.descriptor_layout, 1},
        .push_ranges{&antialias_push, 1},
        .vertex_shader_path = "shaders/fullscreen.vert.spv",
        .fragment_shader_path = "shaders/antialias.frag.spv",
    });
    if (antialias_pipeline.has_err())
        ERRORF("Could not find valid post process shaders: {}", to_string(antialias_pipeline.err()));
    renderer.antialias_pipeline = *antialias_pipeline;

    renderer.textures = malloc_pool<PbrTexture>(config.max_textures);
    renderer.models = malloc_pool<PbrModel>(config.max_models);

    renderer.depth_image = renderer.textures.alloc();
    for (auto& image : renderer.color_images) {
        image = renderer.textures.alloc();
    }
    create_pbr_renderer_images(renderer, config.window.swapchain.extent);

    renderer.vp_buffer = create_buffer(vk, {
        sizeof(ViewProjectionUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, GpuMemoryRandomAccess
    });
    renderer.light_buffer = create_buffer(vk, {
        sizeof(LightUniform) * PbrMaxLights, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, GpuMemoryRandomAccess
    });

    write_uniform_buffer_descriptor(
        vk, {renderer.descriptor_set, 0}, {renderer.vp_buffer.handle, sizeof(ViewProjectionUniform)}
    );
    write_uniform_buffer_descriptor(
        vk, {renderer.descriptor_set, 1}, {renderer.light_buffer.handle, sizeof(LightUniform)}
    );

    const glm::vec3 positions[]{
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
    const u32 indices[]{
         0,  1,  2,  2,  3,  0,
         4,  5,  6,  6,  7,  4,
         8,  9, 10, 10, 11,  8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20,
    };
    renderer.skybox.vertex_buffer = create_buffer(vk, {
        sizeof(positions), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    renderer.skybox.index_buffer = create_buffer(vk, {
        sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    write_buffer(vk, renderer.skybox.vertex_buffer, positions, sizeof(positions));
    write_buffer(vk, renderer.skybox.index_buffer, indices, sizeof(indices));

    return renderer;
}

void resize_pbr_renderer(PbrRenderer& renderer, VkExtent2D extent) {
    destroy_pbr_renderer_images(renderer);
    create_pbr_renderer_images(renderer, extent);
}

void destroy_pbr_renderer(PbrRenderer& renderer) {
    Vk& vk = *renderer.vk;

    const auto wait_result = vkQueueWaitIdle(vk.queue);
    switch (wait_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: ERROR("Vulkan ran out of device memory");
        case VK_ERROR_DEVICE_LOST: ERROR("Vulkan device lost");
        default: ERROR("Unexpected Vulkan error");
    }

    if (renderer.skybox.cubemap.index != PbrTextureHandle{}.index)
        destroy_texture(renderer, renderer.skybox.cubemap);

    destroy_buffer(vk, renderer.skybox.index_buffer);
    destroy_buffer(vk, renderer.skybox.vertex_buffer);

    destroy_buffer(vk, renderer.vp_buffer);
    destroy_buffer(vk, renderer.light_buffer);

    destroy_pbr_renderer_images(renderer);
    renderer.textures.dealloc(renderer.depth_image);
    for (auto& image : renderer.color_images) {
        renderer.textures.dealloc(image);
    }

    destroy_graphics_pipeline(vk, renderer.skybox_pipeline);
    destroy_graphics_pipeline(vk, renderer.model_pipeline);
    destroy_graphics_pipeline(vk, renderer.tonemap_pipeline);
    destroy_graphics_pipeline(vk, renderer.antialias_pipeline);

    vkDestroyDescriptorPool(vk.device, renderer.descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(vk.device, renderer.descriptor_layout, nullptr);

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

    const VkVertexInputBindingDescription2EXT vertex_bindings[]{
        {
            .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
            .stride = sizeof(glm::vec3), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, .divisor = 1
        }
    };
    const VkVertexInputAttributeDescription2EXT vertex_attributes[]{
        {
            .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
            .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT
        },
    };
    g_pfn.vkCmdSetVertexInputEXT(cmd,
        to_u32(std::size(vertex_bindings)), vertex_bindings,
        to_u32(std::size(vertex_attributes)), vertex_attributes
    );

    SkyboxPush push{
        .cubemap = to_u32(renderer.skybox.cubemap.index),
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

static void draw_models(VkCommandBuffer cmd, PbrRenderer& renderer, Slice<const PbrModelTicket> models) {
    constexpr VkVertexInputBindingDescription2EXT vertex_bindings[]{{
        .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
        .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, .divisor = 1
    }};
    constexpr VkVertexInputAttributeDescription2EXT vertex_attributes[]{{
       .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
       .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, position)
    }, {
       .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
       .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal)
    }, {
       .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
       .location = 2, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Vertex, tangent)
    }, {
       .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
       .location = 3, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, tex_coord)
    }};
    g_pfn.vkCmdSetVertexInputEXT(cmd,
        to_u32(std::size(vertex_bindings)), vertex_bindings,
        to_u32(std::size(vertex_attributes)), vertex_attributes
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

        ModelPush model_push{
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

static void draw_opaque(VkCommandBuffer cmd, PbrRenderer& renderer, VkExtent2D extent, const Scene& scene) {
    Vk& vk = *renderer.vk;
    PbrTexture& depth_image = renderer.textures[renderer.depth_image];
    PbrTexture& color_image = renderer.textures[renderer.color_images[0]];

    BarrierBuilder(vk, {.image_barriers = 2})
        .add_image_barrier(0, depth_image.image.handle, {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1})
        .set_image_dst(0,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        .add_image_barrier(1, color_image.image.handle, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
        .set_image_dst(1,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .build_and_run(vk, cmd);

    const VkRenderingAttachmentInfo color_attachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = color_image.view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    const VkRenderingAttachmentInfo depth_attachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = depth_image.view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue{.depthStencil{1.0f, 0}},
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

    draw_skybox(cmd, renderer);
    draw_models(cmd, renderer, scene.models);

    vkCmdEndRendering(cmd);
}

struct EffectConfig {
    GraphicsPipeline& pipeline;
    Slice<void> push_constant;
    u32 output_image;
    u32 input_image;
};
static void draw_effect(VkCommandBuffer cmd, PbrRenderer& renderer, const EffectConfig& config) {
    Vk& vk = *renderer.vk;
    PbrTexture& output_image = renderer.textures[renderer.color_images[config.output_image]];
    PbrTexture& input_image = renderer.textures[renderer.color_images[config.input_image]];

    BarrierBuilder(vk, {.image_barriers = 2})
        .add_image_barrier(0,
            input_image.image.handle,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        )
        .set_image_src(0,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .set_image_dst(0,
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .add_image_barrier(1,
            output_image.image.handle,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        )
        .set_image_dst(1,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .build_and_run(vk, cmd);

    const VkRenderingAttachmentInfo attachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = output_image.view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    const VkRenderingInfo render_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea{
            {},
            {output_image.image.extent.width, output_image.image.extent.height}
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment,
    };
    vkCmdBeginRendering(cmd, &render_info);

    bind_shaders(cmd, config.pipeline);
    vkCmdBindDescriptorSets(cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        config.pipeline.layout,
        0, 1, &renderer.descriptor_set,
        0, nullptr
    );
    vkCmdPushConstants(cmd, config.pipeline.layout,
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
        0, to_u32(config.push_constant.count), config.push_constant.data
    );
    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRendering(cmd);
}

Result<void> draw_pbr(PbrRenderer& renderer, Window& window, const Scene& scene) {
    ASSERT(scene.lights.count < PbrMaxLights);

    Vk& vk = *renderer.vk;

    const glm::mat4 view = scene.camera->view();
    LightUniform lights{.count = scene.lights.count};
    for (usize i = 0; i < scene.lights.count; ++i) {
        lights.vals[i] = {
            .position = view * scene.lights[i].position,
            .color = scene.lights[i].color,
        };
    }
    write_buffer(vk, renderer.light_buffer, &lights, sizeof(lights));
    write_buffer(vk, renderer.vp_buffer, &view, sizeof(view), offsetof(ViewProjectionUniform, view));

    const auto frame_result = [&]() -> Result<void> {
        const auto begin = begin_frame(vk, window.swapchain);
        if (begin.has_err())
            return begin.err();
        const VkCommandBuffer cmd = *begin;

        draw_opaque(cmd, renderer, window.swapchain.extent, scene);

        BarrierBuilder(vk, {.image_barriers = 1})
            .add_image_barrier(0,
                renderer.textures[renderer.depth_image].image.handle, {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
            )
            .set_image_src(0,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            .set_image_dst(0,
                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            .build_and_run(vk, cmd);

        TonemapPush tonemap_push{
            to_u32(renderer.color_images[0].index)
        };
        draw_effect(cmd, renderer, {
            .pipeline = renderer.tonemap_pipeline,
            .push_constant = {&tonemap_push, sizeof(tonemap_push)},
            .output_image = 1,
            .input_image = 0,
        });

        AntialiasPush antialias_push{
            {1.0f / window.swapchain.extent.width, 1.0f / window.swapchain.extent.height},
            to_u32(renderer.color_images[1].index),
        };
        draw_effect(cmd, renderer, {
            .pipeline = renderer.antialias_pipeline,
            .push_constant = {&antialias_push, sizeof(antialias_push)},
            .output_image = 0,
            .input_image = 1,
        });

        BarrierBuilder(vk, {.image_barriers = 2})
            .add_image_barrier(0,
                renderer.textures[renderer.color_images[0]].image.handle,
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
            )
            .set_image_src(0,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            .set_image_dst(0,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            .add_image_barrier(1, window.swapchain.current_image(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
            .set_image_dst(1,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            .build_and_run(vk, cmd);

        blit_image(cmd, {
            .image = window.swapchain.current_image(),
            .end = {to_i32(window.swapchain.extent.width), to_i32(window.swapchain.extent.height), 1},
        }, {
            .image = renderer.textures[renderer.color_images[0]].image.handle,
            .end = {to_i32(window.swapchain.extent.width), to_i32(window.swapchain.extent.height), 1},
        }, VK_FILTER_NEAREST);

        BarrierBuilder(vk, {.image_barriers = 1})
            .add_image_barrier(0, window.swapchain.current_image(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
            .set_image_src(0,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            .set_image_dst(0,
                VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                VK_ACCESS_2_NONE,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
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

                std::printf("resized\n");

                const auto window_result = resize_window(vk, window);
                if (window_result.has_err()) {
                    LOGF_ERROR("Could not resize window: {}", to_string(window_result.err()));
                    return window_result.err();
                }

                resize_pbr_renderer(renderer, window.swapchain.extent);
                break;
            }
            default: ERRORF("Unexpected error: {}", to_string(frame_result.err()));
        }
    }

    return ok();
}

PbrLight make_light(const glm::vec3 position, const glm::vec3 color, const f32 intensity) {
    return {glm::vec4{position, 1.0f}, glm::vec4{color * intensity, 1.0f}};
}

void update_projection(PbrRenderer& renderer, const glm::mat4& projection) {
    write_buffer(
        *renderer.vk,
        renderer.vp_buffer,
        &projection,
        sizeof(projection),
        offsetof(ViewProjectionUniform, projection)
    );
}

PbrTextureHandle create_texture(PbrRenderer& renderer, AssetManager& assets, const PbrTextureConfig& config) {
    ASSERT(config.format != VK_FORMAT_UNDEFINED);

    Vk& vk = *renderer.vk;
    ImageData& data = assets[config.data];

    GpuImage image = create_gpu_image(vk, {
        .extent = {to_u32(data.size.x), to_u32(data.size.y), 1},
        .format = config.format,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    });
    VkImageView image_view = create_gpu_image_view(vk, {
        .image = image.handle,
        .format = config.format,
    });
    VkSampler sampler = create_sampler(vk, {
        .type = SamplerType::SamplerLinear,
        .edge_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    });
    write_gpu_image(vk, image, {
        .src = {data.pixels, data.size.x * data.size.y * data.alignment},
        .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    });

    PbrTextureHandle texture = renderer.textures.alloc();
    renderer.textures[texture] = {image, image_view, sampler};
    write_image_sampler_descriptor(
        vk,
        {renderer.descriptor_set, 2, to_u32(texture.index)},
        image_view,
        sampler
    );
    return texture;
}

void destroy_texture(PbrRenderer& renderer, const PbrTextureHandle texture) {
    Vk& vk = *renderer.vk;
    destroy_gpu_image(vk, renderer.textures[texture].image);
    vkDestroyImageView(vk.device, renderer.textures[texture].view, nullptr);
    vkDestroySampler(vk.device, renderer.textures[texture].sampler, nullptr);
    renderer.textures.dealloc(texture);
}

void load_skybox(PbrRenderer& renderer, AssetManager& assets, const ImageHandle<u32> cubemap) {
    Vk& vk = *renderer.vk;
    ImageData& data = assets[cubemap];

    GpuImage image = create_gpu_cubemap(vk, {
        .face_extent = {to_u32(data.size.x) / 4, to_u32(data.size.y) / 3, 1},
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    });
    VkImageView view = create_gpu_cubemap_view(vk, {
        .image = image.handle,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
    });
    VkSampler sampler = create_sampler(vk, {
        .type = SamplerType::SamplerLinear,
        .edge_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    });
    write_gpu_cubemap(vk, image, {
        {data.pixels, data.size.x * data.size.y * data.alignment},
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    });

    renderer.skybox.cubemap = renderer.textures.alloc();
    renderer.textures[renderer.skybox.cubemap] = {image, view, sampler};
    write_image_sampler_descriptor(vk, {
        renderer.descriptor_set, 2, to_u32(renderer.skybox.cubemap.index)
    }, view, sampler);
}

void unload_skybox(PbrRenderer& renderer) {
    destroy_texture(renderer, renderer.skybox.cubemap);
}

PbrModelHandle create_model(PbrRenderer& renderer, AssetManager& assets, const PbrModelConfig& config) {
    ASSERT(config.data.roughness >= 0.0 && config.data.roughness <= 1.0);
    ASSERT(config.data.metalness >= 0.0 && config.data.metalness <= 1.0);

    Vk& vk = *renderer.vk;
    MeshData& mesh = assets[config.data.mesh];

    const auto index_buffer = create_buffer(vk, {
        mesh.indices.count * sizeof(mesh.indices[0]),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    const auto vertex_buffer = create_buffer(vk, {
        mesh.vertices.count * sizeof(mesh.vertices[0]),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });

    write_buffer(
        vk,
        index_buffer,
        mesh.indices.data,
        mesh.indices.count * sizeof(mesh.indices[0])
    );
    write_buffer(
        vk,
        vertex_buffer,
        mesh.vertices.data,
        mesh.vertices.count * sizeof(mesh.vertices[0])
    );

    auto model = renderer.models.alloc();
    renderer.models[model] = PbrModel{
        .index_count = to_u32(mesh.indices.count),
        .index_buffer = index_buffer,
        .vertex_buffer = vertex_buffer,
        .normal_map = config.normal_map,
        .color_map = config.color_map,
        .roughness = config.data.roughness,
        .metalness = config.data.metalness,
    };
    return model;
}

void destroy_model(PbrRenderer& renderer, const PbrModelHandle model) {
    Vk& vk = *renderer.vk;
    auto& data = renderer.models[model];
    destroy_buffer(vk, data.index_buffer);
    destroy_buffer(vk, data.vertex_buffer);
    renderer.models.dealloc(model);
}

} // namespace hg
