#include "hg_graphics.h"
#include "hurdy_gurdy.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#define MOUSE_SPEED 0.003f
#define MOVE_SPEED 1.5f

typedef enum InputState {
    INPUT_STATE_NONE = 0,
    INPUT_STATE_UP = 0x1,
    INPUT_STATE_DOWN = 0x2,
    INPUT_STATE_LEFT = 0x4,
    INPUT_STATE_RIGHT = 0x8,
    INPUT_STATE_FORWARD = 0x10,
    INPUT_STATE_BACKWARD = 0x20,
    INPUT_STATE_LMOUSE = 0x40,
    INPUT_STATE_RMOUSE = 0x80,
} InputState;
static i32 input_state;

static HgTexture* target;
static HgTexture* depth_buffer;

typedef struct Vertex {
    HgVec3 pos;
    HgVec2 uv;
} Vertex;

typedef struct VPUniform {
    HgMat4 view;
    HgMat4 proj;
} VPUniform;

typedef struct Push {
    HgMat4 model;
} Push;

static HgShader* shader;

static HgBuffer* vp_buffer;

static const u32 texture_data[] = {
    0xffff0000,
    0xff00ff00,
    0xff0000ff,
    0xff00ffff,
};
static HgTexture* texture;

static const Vertex vertices[] = {{
    .pos = {-0.5f, -0.5f, 0.0f},
    .uv = {0.0f, 0.0f},
}, {
    .pos = {-0.5f, 0.5f, 0.0f},
    .uv = {0.0f, 2.0f},
}, {
    .pos = {0.5f, 0.5f, 0.0f},
    .uv = {2.0f, 2.0f},
}, {
    .pos = {0.5f, -0.5f, 0.0f},
    .uv = {2.0f, 0.0f},
}};
static HgBuffer* vertex_buffer;

static const u32 indices[] = {
    0, 1, 2, 2, 3, 0,
};
static HgBuffer* index_buffer;

static f32 camera_fov;

static HgVec3 camera_position;
static f32 camera_zoom;
static HgQuat camera_rotation;

static HgVec3 object_position;
static HgVec2 object_scale;
static f32 object_rotation;

static HgClock game_clock;
static f64 time_elapsed;
static u64 frame_count;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    (void)appstate;
    (void)argc;
    (void)argv;

    hg_init();
    hg_window_open(&(HgWindowConfig){
        .title = "Hurdy Gurdy",
        .width = 600,
        .height = 400,
        .windowed = true,
    });

    u32 window_width, window_height;
    hg_window_get_size(&window_width, &window_height);

    target = hg_texture_create(&(HgTextureConfig){
        .width = window_width,
        .height = window_height,
        .depth = 1,
        .format = HG_FORMAT_R8G8B8A8_UNORM,
        .aspect = HG_TEXTURE_ASPECT_COLOR_BIT,
        .usage = HG_TEXTURE_USAGE_RENDER_TARGET_BIT | HG_TEXTURE_USAGE_TRANSFER_SRC_BIT,
    });

    depth_buffer = hg_texture_create(&(HgTextureConfig){
        .width = window_width,
        .height = window_height,
        .depth = 1,
        .format = HG_FORMAT_D32_SFLOAT,
        .aspect = HG_TEXTURE_ASPECT_DEPTH_BIT,
        .usage = HG_TEXTURE_USAGE_DEPTH_BUFFER_BIT | HG_TEXTURE_USAGE_TRANSFER_SRC_BIT,
    });

    HgVertexAttribute vertex_attributes[] = {{
        .format = HG_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, pos),
    }, {
        .format = HG_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, uv),
    }};
    HgVertexBinding vertex_bindings[] = {{
        .attributes = vertex_attributes,
        .attribute_count = HG_ARRAY_SIZE(vertex_attributes),
        .stride = sizeof(Vertex),
    }};

    HgDescriptorSetBinding vp_set_bindings[] = {{
        .descriptor_type = HG_DESCRIPTOR_TYPE_BUFFER,
        .descriptor_count = 1,
    }};
    HgDescriptorSetBinding object_set_bindings[] = {{
        .descriptor_type = HG_DESCRIPTOR_TYPE_TEXTURE,
        .descriptor_count = 1,
    }};
    HgDescriptorSet descriptor_sets[] = {{
        .bindings = vp_set_bindings,
        .binding_count = HG_ARRAY_SIZE(vp_set_bindings),
    }, {
        .bindings = object_set_bindings,
        .binding_count = HG_ARRAY_SIZE(object_set_bindings),
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
        .color_format = HG_FORMAT_R8G8B8A8_UNORM,
        .depth_format = HG_FORMAT_D32_SFLOAT,
        .spirv_vertex_shader = vertex_shader,
        .vertex_shader_size = (u32)vertex_shader_size,
        .spirv_fragment_shader = fragment_shader,
        .fragment_shader_size = (u32)fragment_shader_size,
        .vertex_bindings = vertex_bindings,
        .vertex_binding_count = HG_ARRAY_SIZE(vertex_bindings),
        .descriptor_sets = descriptor_sets,
        .descriptor_set_count = HG_ARRAY_SIZE(descriptor_sets),
        .push_constant_size = sizeof(Push),
        .topology = HG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .cull_mode = HG_CULL_MODE_NONE,
        .enable_depth_buffer = true,
        .enable_color_blend = true,
    });

    hg_file_unload_binary(vertex_shader, vertex_shader_size);
    hg_file_unload_binary(fragment_shader, fragment_shader_size);

    vp_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(VPUniform),
        .usage = HG_BUFFER_USAGE_UNIFORM_BUFFER_BIT | HG_BUFFER_USAGE_TRANSFER_DST_BIT,
    });

    texture = hg_texture_create(&(HgTextureConfig){
        .width = 2,
        .height = 2,
        .depth = 1,
        .array_layers = 1,
        .mip_levels = 1,
        .format = HG_FORMAT_R8G8B8A8_UNORM,
        .aspect = HG_TEXTURE_ASPECT_COLOR_BIT,
        .usage = HG_TEXTURE_USAGE_SAMPLED_BIT | HG_TEXTURE_USAGE_TRANSFER_DST_BIT,
        .edge_mode = HG_SAMPLER_EDGE_MODE_REPEAT,
        .bilinear_filter = false,
    });
    hg_texture_write(texture, texture_data, HG_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vertex_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(vertices),
        .usage = HG_BUFFER_USAGE_VERTEX_BUFFER_BIT | HG_BUFFER_USAGE_TRANSFER_DST_BIT,
    });
    hg_buffer_write(vertex_buffer, 0, vertices, sizeof(vertices));

    index_buffer = hg_buffer_create(&(HgBufferConfig){
        .size = sizeof(indices),
        .usage = HG_BUFFER_USAGE_INDEX_BUFFER_BIT | HG_BUFFER_USAGE_TRANSFER_DST_BIT,
    });
    hg_buffer_write(index_buffer, 0, indices, sizeof(indices));

    camera_fov = (f32)HG_PI / 3.0f;
    HgMat4 proj = hg_projection_matrix_perspective(camera_fov, (f32)window_width / (f32)window_height, 0.1f, 100.0f);
    hg_buffer_write(vp_buffer, offsetof(VPUniform, proj), &proj, sizeof(proj));

    camera_position = (HgVec3){0.0f, 0.0f, -1.0f};
    camera_zoom = 1.0f;
    camera_rotation = (HgQuat){1.0f, 0.0f, 0.0f, 0.0f};
    HgMat4 view = hg_view_matrix(camera_position, camera_zoom, camera_rotation);
    hg_buffer_write(vp_buffer, offsetof(VPUniform, view), &view, sizeof(view));

    object_position = (HgVec3){0.0f, 0.0f, 0.0f};
    object_scale = (HgVec2){1.0f, 1.0f};
    object_rotation = 0.0f;

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
                case SDLK_ESCAPE: return SDL_APP_SUCCESS;
                case SDLK_SPACE: {
                    input_state |= INPUT_STATE_UP;
                } break;
                case SDLK_LSHIFT: {
                    input_state |= INPUT_STATE_DOWN;
                } break;
                case SDLK_W: {
                    input_state |= INPUT_STATE_FORWARD;
                } break;
                case SDLK_S: {
                    input_state |= INPUT_STATE_BACKWARD;
                } break;
                case SDLK_A: {
                    input_state |= INPUT_STATE_LEFT;
                } break;
                case SDLK_D: {
                    input_state |= INPUT_STATE_RIGHT;
                } break;
            }
        } break;
        case SDL_EVENT_KEY_UP: {
            switch (event->key.key) {
                case SDLK_SPACE: {
                    input_state &= ~INPUT_STATE_UP;
                } break;
                case SDLK_LSHIFT: {
                    input_state &= ~INPUT_STATE_DOWN;
                } break;
                case SDLK_W: {
                    input_state &= ~INPUT_STATE_FORWARD;
                } break;
                case SDLK_S: {
                    input_state &= ~INPUT_STATE_BACKWARD;
                } break;
                case SDLK_A: {
                    input_state &= ~INPUT_STATE_LEFT;
                } break;
                case SDLK_D: {
                    input_state &= ~INPUT_STATE_RIGHT;
                } break;
            }
        } break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            switch (event->button.button) {
                case SDL_BUTTON_LEFT: {
                    input_state |= INPUT_STATE_LMOUSE;
                } break;
                case SDL_BUTTON_RIGHT: {
                    input_state |= INPUT_STATE_RMOUSE;
                } break;
            }
        } break;
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            switch (event->button.button) {
                case SDL_BUTTON_LEFT: {
                    input_state &= ~INPUT_STATE_LMOUSE;
                } break;
                case SDL_BUTTON_RIGHT: {
                    input_state &= ~INPUT_STATE_RMOUSE;
                } break;
            }
        } break;
        case SDL_EVENT_MOUSE_MOTION: {
            if (input_state & INPUT_STATE_LMOUSE) {
                camera_rotation = hg_qmul(
                    hg_axis_angle((HgVec3){0.0f, 1.0f, 0.0f}, event->motion.xrel * MOUSE_SPEED),
                    camera_rotation
                );
                camera_rotation = hg_qmul(
                    camera_rotation,
                    hg_axis_angle((HgVec3){-1.0f, 0.0f, 0.0f}, event->motion.yrel * MOUSE_SPEED)
                );
            }
        } break;
        case SDL_EVENT_WINDOW_RESIZED: {
            u32 window_width, window_height;
            hg_window_update_size();
            hg_window_get_size(&window_width, &window_height);

            hg_texture_destroy(target);
            target = hg_texture_create(&(HgTextureConfig){
                .width = window_width,
                .height = window_height,
                .depth = 1,
                .array_layers = 1,
                .mip_levels = 1,
                .format = HG_FORMAT_R8G8B8A8_UNORM,
                .aspect = HG_TEXTURE_ASPECT_COLOR_BIT,
                .usage = HG_TEXTURE_USAGE_RENDER_TARGET_BIT | HG_TEXTURE_USAGE_TRANSFER_SRC_BIT,
            });

            hg_texture_destroy(depth_buffer);
            depth_buffer = hg_texture_create(&(HgTextureConfig){
                .width = window_width,
                .height = window_height,
                .depth = 1,
                .array_layers = 1,
                .mip_levels = 1,
                .format = HG_FORMAT_D32_SFLOAT,
                .aspect = HG_TEXTURE_ASPECT_DEPTH_BIT,
                .usage = HG_TEXTURE_USAGE_DEPTH_BUFFER_BIT | HG_TEXTURE_USAGE_TRANSFER_SRC_BIT,
            });

            f32 aspect = (f32)window_width / (f32)window_height;
            HgMat4 proj = hg_projection_matrix_perspective(camera_fov, aspect, 0.1f, 100.0f);
            hg_buffer_write(vp_buffer, offsetof(VPUniform, proj), &proj, sizeof(proj));
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

    if (input_state & INPUT_STATE_UP) {
        camera_position = hg_move_first_person(
            camera_position, camera_rotation, (HgVec3){0.0f, -1.0f, 0.0f}, (f32)delta * MOVE_SPEED
        );
    }
    if (input_state & INPUT_STATE_DOWN) {
        camera_position = hg_move_first_person(
            camera_position, camera_rotation, (HgVec3){0.0f, 1.0f, 0.0f}, (f32)delta * MOVE_SPEED
        );
    }
    if (input_state & INPUT_STATE_FORWARD) {
        camera_position = hg_move_first_person(
            camera_position, camera_rotation, (HgVec3){0.0f, 0.0f, 1.0f}, (f32)delta * MOVE_SPEED
        );
    }
    if (input_state & INPUT_STATE_BACKWARD) {
        camera_position = hg_move_first_person(
            camera_position, camera_rotation, (HgVec3){0.0f, 0.0f, -1.0f}, (f32)delta * MOVE_SPEED
        );
    }
    if (input_state & INPUT_STATE_LEFT) {
        camera_position = hg_move_first_person(
            camera_position, camera_rotation, (HgVec3){-1.0f, 0.0f, 0.0f}, (f32)delta * MOVE_SPEED
        );
    }
    if (input_state & INPUT_STATE_RIGHT) {
        camera_position = hg_move_first_person(
            camera_position, camera_rotation, (HgVec3){1.0f, 0.0f, 0.0f}, (f32)delta * MOVE_SPEED
        );
    }

    HgMat4 view = hg_view_matrix(camera_position, camera_zoom, camera_rotation);
    hg_buffer_write(vp_buffer, offsetof(VPUniform, view), &view, sizeof(view));

    bool success = hg_render_begin();
    if (!success) {
        HG_DEBUG("Failed to render");
        return SDL_APP_CONTINUE;
    }

    hg_renderpass_begin(target, depth_buffer);

    hg_shader_bind(shader);

    HgDescriptor vp_descriptor_set[] = {{
        .type = HG_DESCRIPTOR_TYPE_BUFFER,
        .count = 1,
        .buffers = &vp_buffer,
    }};
    hg_bind_descriptor_set(0, vp_descriptor_set, HG_ARRAY_SIZE(vp_descriptor_set));

    HgDescriptor object_descriptor_set[] = {{
        .type = HG_DESCRIPTOR_TYPE_TEXTURE,
        .count = 1,
        .textures = &texture,
    }};
    hg_bind_descriptor_set(1, object_descriptor_set, HG_ARRAY_SIZE(object_descriptor_set));

    Push push_data = {
        .model = hg_model_matrix_2d(object_position, object_scale, object_rotation),
    };
    hg_draw(vertex_buffer, index_buffer, &push_data, sizeof(push_data));

    hg_shader_unbind();

    hg_renderpass_end();

    success = hg_render_end();
    if (!success) {
        HG_DEBUG("Failed to draw");
        return SDL_APP_CONTINUE;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    (void)appstate;
    (void)result;

#if !defined(NDEBUG)
    hg_graphics_wait();

    hg_buffer_destroy(index_buffer);
    hg_buffer_destroy(vertex_buffer);
    hg_texture_destroy(texture);
    hg_buffer_destroy(vp_buffer);
    hg_shader_destroy(shader);
    hg_texture_destroy(depth_buffer);
    hg_texture_destroy(target);

    hg_window_close();
    hg_shutdown();
#endif
}

