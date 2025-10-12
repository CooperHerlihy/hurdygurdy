#include "hurdy_gurdy.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

static HgWindow* window;

typedef struct Vertex {
    float pos[3];
    float uv[2];
} Vertex;

static const Vertex vertices[] = {{
    .pos = {-0.5f, -0.5f, 0.0f},
    .uv = {0.0f, 0.0f},
}, {
    .pos = {-0.5f, 0.5f, 0.0f},
    .uv = {0.0f, 1.0f},
}, {
    .pos = {0.5f, 0.5f, 0.0f},
    .uv = {1.0f, 1.0f},
}, {
    .pos = {0.5f, -0.5f, 0.0f},
    .uv = {1.0f, 0.0f},
}};
static HgBuffer* vertex_buffer;

static const u32 indices[] = {
    0, 1, 2, 2, 3, 0,
};
static HgBuffer* index_buffer;

typedef struct Push {
    float offset[2];
} Push;
static Push push_data;

static const u32 texture_data[] = {
    0xff0000ff,
    0xff00ff00,
    0xffff0000,
    0xff00ffff,
};
static HgTexture* texture;
static HgSampler* sampler;

static HgShader* shader;

static HgDescriptorSet* descriptor_set;

static HgTexture* target;
static HgTexture* depth_buffer;

static HgClock game_clock;
static f64 time_elapsed;
static u64 frame_count;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    (void)appstate;
    (void)argc;
    (void)argv;

    hg_init();

    window = hg_window_create(&(HgWindowConfig){
        .title = "Hurdy Gurdy",
    });

    vertex_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(vertices),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    });
    hg_buffer_write(vertex_buffer, 0, vertices, sizeof(vertices));

    index_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(indices),
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    });
    hg_buffer_write(index_buffer, 0, indices, sizeof(indices));

    push_data = (Push){
        .offset = {0.0f, 0.0f},
    };

    texture = hg_texture_create(&(HgTextureConfig){
        .width = 2,
        .height = 2,
        .depth = 1,
        .array_layers = 1,
        .mip_levels = 1,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    });
    hg_texture_write(texture, texture_data, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    sampler = hg_sampler_create(&(HgSamplerConfig){
        .type = VK_FILTER_LINEAR,
        .edge_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    });

    VkVertexInputBindingDescription vertex_bindings[] = {{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    }};
    VkVertexInputAttributeDescription vertex_attributes[] = {{
        .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, pos),
    }, {
        .binding = 0,
        .location = 1,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, uv),
    }};

    HgDescriptorSetBinding descriptor_set_bindings[] = {{
        .set = 0,
        .binding = 0,
        .descriptor_type = HG_DESCRIPTOR_TYPE_TEXTURE,
        .descriptor_count = 1,
    }, {
        .set = 0,
        .binding = 1,
        .descriptor_type = HG_DESCRIPTOR_TYPE_SAMPLER,
        .descriptor_count = 1,
    }};

    VkPushConstantRange push_constants[] = {{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .size = sizeof(Push),
    }};

    byte* vertex_shader;
    usize vertex_shader_size;
    HgError vertex_shader_error =
        hg_file_load_binary("build/shaders/test.vert.spv", &vertex_shader, &vertex_shader_size);
    switch (vertex_shader_error) {
        case HG_SUCCESS: break;
        case HG_FILE_NOT_FOUND: HG_ERROR("vertex shader not found");
        case HG_FILE_READ_FAILURE: HG_ERROR("vertex shader not readable");
        default: HG_ERROR("unknown error");
    }

    byte* fragment_shader;
    usize fragment_shader_size;
    HgError fragment_shader_error =
        hg_file_load_binary("build/shaders/test.frag.spv", &fragment_shader, &fragment_shader_size);
    switch (fragment_shader_error) {
        case HG_SUCCESS: break;
        case HG_FILE_NOT_FOUND: HG_ERROR("fragment shader not found");
        case HG_FILE_READ_FAILURE: HG_ERROR("fragment shader not readable");
        default: HG_ERROR("unknown error");
    }

    shader = hg_shader_create(&(HgShaderConfig){
        .color_format = VK_FORMAT_R8G8B8A8_UNORM,
        .depth_format = VK_FORMAT_D32_SFLOAT,
        .spirv_vertex_shader = vertex_shader,
        .vertex_shader_size = (u32)vertex_shader_size,
        .spirv_fragment_shader = fragment_shader,
        .fragment_shader_size = (u32)fragment_shader_size,
        .vertex_bindings = vertex_bindings,
        .vertex_attributes = vertex_attributes,
        .vertex_binding_count = HG_ARRAY_SIZE(vertex_bindings),
        .vertex_attribute_count = HG_ARRAY_SIZE(vertex_attributes),
        .descriptor_set_bindings = descriptor_set_bindings,
        .push_constants = push_constants,
        .descriptor_binding_count = HG_ARRAY_SIZE(descriptor_set_bindings),
        .push_constant_count = HG_ARRAY_SIZE(push_constants),
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .cull_mode = VK_CULL_MODE_BACK_BIT,
        .enable_depth_buffer = true,
        .enable_color_blend = true,
    });

    hg_file_unload_binary(vertex_shader, vertex_shader_size);
    hg_file_unload_binary(fragment_shader, fragment_shader_size);

    descriptor_set = hg_allocate_descriptor_set(shader, 0);
    hg_update_descriptor_set(descriptor_set, 0, 0, HG_DESCRIPTOR_TYPE_TEXTURE, NULL, texture, NULL);
    hg_update_descriptor_set(descriptor_set, 1, 0, HG_DESCRIPTOR_TYPE_SAMPLER, NULL, NULL, sampler);

    u32 window_width, window_height;
    hg_window_get_size(window, &window_width, &window_height);

    target = hg_texture_create(&(HgTextureConfig){
        .width = window_width,
        .height = window_height,
        .depth = 1,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    });

    depth_buffer = hg_texture_create(&(HgTextureConfig){
        .width = window_width,
        .height = window_height,
        .depth = 1,
        .format = VK_FORMAT_D32_SFLOAT,
        .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    });

    (void)hg_clock_tick(&game_clock);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    (void)appstate;

    switch (event->type) {
        case SDL_EVENT_QUIT: {
            return SDL_APP_SUCCESS;
        } break;
        case SDL_EVENT_KEY_DOWN: {
            switch (event->key.key) {
                case SDLK_ESCAPE: {
                    return SDL_APP_SUCCESS;
                } break;
                case SDLK_UP: {
                    push_data.offset[1] -= 0.1f;
                } break;
                case SDLK_DOWN: {
                    push_data.offset[1] += 0.1f;
                } break;
                case SDLK_LEFT: {
                    push_data.offset[0] -= 0.1f;
                } break;
                case SDLK_RIGHT: {
                    push_data.offset[0] += 0.1f;
                } break;
            }
        } break;
        case SDL_EVENT_WINDOW_RESIZED: {
            u32 window_width, window_height;
            hg_window_update_size(window);
            hg_window_get_size(window, &window_width, &window_height);

            hg_texture_destroy(target);
            target = hg_texture_create(&(HgTextureConfig){
                .width = window_width,
                .height = window_height,
                .depth = 1,
                .array_layers = 1,
                .mip_levels = 1,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            });
        } break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    (void)appstate;

    f64 delta = hg_clock_tick(&game_clock);
    time_elapsed += delta;
    ++frame_count;
    if (time_elapsed > 1.0) {
        HG_LOGF("avg: %fms, fps: %" PRIu64, 1.0e3 / (f64)frame_count, frame_count);
        time_elapsed -= 1.0;
        frame_count = 0;
    }

    HgObject objects[] = {{
        .vertex_buffer = vertex_buffer,
        .index_buffer = index_buffer,
        .descriptor_sets = &descriptor_set,
        .descriptor_set_count = 1,
        .push_data = &push_data,
        .push_size = sizeof(Push),
    }};

    HgPipeline pipeline = {
        .shader = shader,
        .objects = objects,
        .object_count = HG_ARRAY_SIZE(objects),
    };

    HgRenderPass render_pass = {
        .target = target,
        .depth_buffer = depth_buffer,
        .pipelines = &pipeline,
        .pipeline_count = 1,
    };

    HgRenderDescription scene = {
        .render_passes = &render_pass,
        .render_pass_count = 1,
    };

    bool success = hg_draw(window, &scene);
    if (!success) {
        HG_DEBUG("Failed to draw");
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    (void)appstate;
    (void)result;

#if !defined(NDEBUG)
    hg_wait_graphics();

    hg_texture_destroy(depth_buffer);
    hg_texture_destroy(target);
    hg_shader_destroy(shader);
    hg_sampler_destroy(sampler);
    hg_texture_destroy(texture);
    hg_buffer_destroy(index_buffer);
    hg_buffer_destroy(vertex_buffer);

    hg_window_destroy(window);
    hg_shutdown();
#endif
}

