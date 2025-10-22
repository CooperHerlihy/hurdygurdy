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
static i32 s_input_state;

static HgTexture* s_target;
static HgTexture* s_depth_buffer;

static f32 s_camera_fov;
static HgVec3 s_camera_position;
static f32 s_camera_zoom;
static HgQuat s_camera_rotation;

static const HgVertex3D s_vertices[] = {{
    .position = {-0.5f, -0.5f, 0.0f},
    .normal = {0.0f, 0.0f, -1.0f},
    .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
    .uv = {0.0f, 0.0f},
}, {
    .position = {-0.5f, 0.5f, 0.0f},
    .normal = {0.0f, 0.0f, -1.0f},
    .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
    .uv = {0.0f, 1.0f},
}, {
    .position = {0.5f, 0.5f, 0.0f},
    .normal = {0.0f, 0.0f, -1.0f},
    .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
    .uv = {1.0f, 1.0f},
}, {
    .position = {0.5f, -0.5f, 0.0f},
    .normal = {0.0f, 0.0f, -1.0f},
    .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
    .uv = {1.0f, 0.0f},
}};
static HgBuffer* s_vertex_buffer;

static const u32 s_indices[] = {
    0, 1, 2, 2, 3, 0,
};
static HgBuffer* s_index_buffer;

static const u32 s_texture_data[] = {
    0xffff0000,
    0xff00ff00,
    0xff0000ff,
    0xff00ffff,
};
static HgTexture* s_texture;

static HgClock s_game_clock;
static f64 s_time_elapsed;
static u64 s_frame_count;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    (void)appstate;
    (void)argc;
    (void)argv;

    hg_init();
    hg_depth_renderer_init();
    hg_3d_renderer_init();

    hg_window_open(&(HgWindowConfig){
        .title = "Hurdy Gurdy",
        // .width = 800,
        // .height = 600,
        // .windowed = true,
    });

    u32 window_width, window_height;
    hg_window_get_size(&window_width, &window_height);

    // hg_3d_renderer_target_create(window_width, window_height, &target, &depth_buffer);
    hg_depth_renderer_target_create(window_width, window_height, &s_target, &s_depth_buffer);

    s_camera_fov = (f32)HG_PI / 3.0f;
    hg_3d_renderer_update_projection(s_camera_fov, (f32)window_width / (f32)window_height, 0.1f, 100.0f);

    s_camera_position = (HgVec3){0.0f, 0.0f, -1.0f};
    s_camera_zoom = 1.0f;
    s_camera_rotation = (HgQuat){1.0f, 0.0f, 0.0f, 0.0f};
    hg_3d_renderer_update_view(s_camera_position, s_camera_zoom, s_camera_rotation);

    s_vertex_buffer = hg_3d_vertex_buffer_create(s_vertices, HG_ARRAY_SIZE(s_vertices));
    s_index_buffer = hg_3d_index_buffer_create(s_indices, HG_ARRAY_SIZE(s_indices));
    s_texture = hg_3d_texture_map_create(s_texture_data, 2, 2, HG_FORMAT_R8G8B8A8_UNORM, false);

    (void)hg_clock_tick(&s_game_clock);

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
                    s_input_state |= INPUT_STATE_UP;
                } break;
                case SDLK_LSHIFT: {
                    s_input_state |= INPUT_STATE_DOWN;
                } break;
                case SDLK_W: {
                    s_input_state |= INPUT_STATE_FORWARD;
                } break;
                case SDLK_S: {
                    s_input_state |= INPUT_STATE_BACKWARD;
                } break;
                case SDLK_A: {
                    s_input_state |= INPUT_STATE_LEFT;
                } break;
                case SDLK_D: {
                    s_input_state |= INPUT_STATE_RIGHT;
                } break;
            }
        } break;
        case SDL_EVENT_KEY_UP: {
            switch (event->key.key) {
                case SDLK_SPACE: {
                    s_input_state &= ~INPUT_STATE_UP;
                } break;
                case SDLK_LSHIFT: {
                    s_input_state &= ~INPUT_STATE_DOWN;
                } break;
                case SDLK_W: {
                    s_input_state &= ~INPUT_STATE_FORWARD;
                } break;
                case SDLK_S: {
                    s_input_state &= ~INPUT_STATE_BACKWARD;
                } break;
                case SDLK_A: {
                    s_input_state &= ~INPUT_STATE_LEFT;
                } break;
                case SDLK_D: {
                    s_input_state &= ~INPUT_STATE_RIGHT;
                } break;
            }
        } break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            switch (event->button.button) {
                case SDL_BUTTON_LEFT: {
                    s_input_state |= INPUT_STATE_LMOUSE;
                } break;
                case SDL_BUTTON_RIGHT: {
                    s_input_state |= INPUT_STATE_RMOUSE;
                } break;
            }
        } break;
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            switch (event->button.button) {
                case SDL_BUTTON_LEFT: {
                    s_input_state &= ~INPUT_STATE_LMOUSE;
                } break;
                case SDL_BUTTON_RIGHT: {
                    s_input_state &= ~INPUT_STATE_RMOUSE;
                } break;
            }
        } break;
        case SDL_EVENT_MOUSE_MOTION: {
            if (s_input_state & INPUT_STATE_LMOUSE) {
                s_camera_rotation = hg_qmul(
                    hg_axis_angle((HgVec3){0.0f, 1.0f, 0.0f}, event->motion.xrel * MOUSE_SPEED),
                    s_camera_rotation
                );
                s_camera_rotation = hg_qmul(
                    s_camera_rotation,
                    hg_axis_angle((HgVec3){-1.0f, 0.0f, 0.0f}, event->motion.yrel * MOUSE_SPEED)
                );
            }
        } break;
        case SDL_EVENT_WINDOW_RESIZED: {
            u32 window_width, window_height;
            hg_window_update_size();
            hg_window_get_size(&window_width, &window_height);

            hg_texture_destroy(s_target);
            hg_texture_destroy(s_depth_buffer);
            hg_depth_renderer_target_create(window_width, window_height, &s_target, &s_depth_buffer);

            f32 aspect = (f32)window_width / (f32)window_height;
            hg_3d_renderer_update_projection(s_camera_fov, aspect, 0.1f, 100.0f);
        } break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    (void)appstate;

    f64 delta = hg_clock_tick(&s_game_clock);
    s_time_elapsed += delta;
    ++s_frame_count;
    if (s_time_elapsed > 1.0) {
        HG_LOGF("avg: %fms, fps: %" PRIu64, 1.0e3 / (f64)s_frame_count, s_frame_count);
        s_time_elapsed -= 1.0;
        s_frame_count = 0;
    }

    if (s_input_state & INPUT_STATE_UP) {
        s_camera_position = hg_move_first_person(
            s_camera_position, s_camera_rotation, (HgVec3){0.0f, -1.0f, 0.0f}, (f32)delta * MOVE_SPEED
        );
    }
    if (s_input_state & INPUT_STATE_DOWN) {
        s_camera_position = hg_move_first_person(
            s_camera_position, s_camera_rotation, (HgVec3){0.0f, 1.0f, 0.0f}, (f32)delta * MOVE_SPEED
        );
    }
    if (s_input_state & INPUT_STATE_FORWARD) {
        s_camera_position = hg_move_first_person(
            s_camera_position, s_camera_rotation, (HgVec3){0.0f, 0.0f, 1.0f}, (f32)delta * MOVE_SPEED
        );
    }
    if (s_input_state & INPUT_STATE_BACKWARD) {
        s_camera_position = hg_move_first_person(
            s_camera_position, s_camera_rotation, (HgVec3){0.0f, 0.0f, -1.0f}, (f32)delta * MOVE_SPEED
        );
    }
    if (s_input_state & INPUT_STATE_LEFT) {
        s_camera_position = hg_move_first_person(
            s_camera_position, s_camera_rotation, (HgVec3){-1.0f, 0.0f, 0.0f}, (f32)delta * MOVE_SPEED
        );
    }
    if (s_input_state & INPUT_STATE_RIGHT) {
        s_camera_position = hg_move_first_person(
            s_camera_position, s_camera_rotation, (HgVec3){1.0f, 0.0f, 0.0f}, (f32)delta * MOVE_SPEED
        );
    }
    hg_3d_renderer_update_view(s_camera_position, s_camera_zoom, s_camera_rotation);

    HgError begin_result = hg_frame_begin();
    if (begin_result != HG_SUCCESS) {
        HG_DEBUG("Failed to begin frame");
        return SDL_APP_CONTINUE;
    }

    static f32 s_time = 0.0f;
    s_time += (f32)delta * 2.0f;
    if (s_time > (f32)HG_TAU) {
        s_time -= (f32)HG_TAU;
    }
    hg_3d_renderer_queue_directional_light((HgVec3){1.0f, 1.0f, 1.0f}, (HgVec3){1.0f, 0.3f, 0.1f}, 0.5f);
    hg_3d_renderer_queue_point_light((HgVec3){cosf(s_time) * 3.0f, -1.0f, -sinf(s_time)}, (HgVec3){1.0f, 1.0f, 1.0f}, 5.0f);

    HgModel3D model = {
        .vertex_buffer = s_vertex_buffer,
        .index_buffer = s_index_buffer,
        .color_map = s_texture,
        .normal_map = NULL,
    };

    hg_3d_renderer_queue_model(&model, &(HgTransform3D){
        .position = (HgVec3){-0.2f, 0.0f, -0.2f},
        .scale = (HgVec3){1.0f, 1.0f, 1.0f},
        .rotation = (HgQuat){1.0f, 0.0f, 0.0f, 0.0f},
    });
    hg_3d_renderer_queue_model(&model, &(HgTransform3D){
        .position = (HgVec3){0.2f, 0.0f, 0.2f},
        .scale = (HgVec3){1.0f, 1.0f, 1.0f},
        .rotation = (HgQuat){1.0f, 0.0f, 0.0f, 0.0f},
    });

    hg_3d_renderer_draw(s_target, s_depth_buffer);

    // hg_depth_render_draw(s_target, s_depth_buffer);

    HgError end_result = hg_frame_end();
    if (end_result != HG_SUCCESS) {
        HG_DEBUG("Failed to end frame");
        return SDL_APP_CONTINUE;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    (void)appstate;
    (void)result;

#if !defined(NDEBUG)
    hg_graphics_wait();

    hg_buffer_destroy(s_index_buffer);
    hg_buffer_destroy(s_vertex_buffer);
    hg_texture_destroy(s_texture);
    hg_texture_destroy(s_depth_buffer);
    hg_texture_destroy(s_target);

    hg_window_close();
    hg_3d_renderer_shutdown();
    hg_depth_renderer_shutdown();
    hg_shutdown();
#endif
}

