#include "hurdy_gurdy.h"

#include "hg_ray_marcher.h"

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

static f32 camera_fov;
static f32 camera_aspect;

static HgVec3 camera_position;
static HgVec3 camera_scale;
static HgQuat camera_rotation;

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
        // .width = 800,
        // .height = 600,
        // .windowed = true,
    });

    hg_ray_marcher_init();

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

    camera_fov = (f32)HG_PI / 3.0f;
    camera_aspect = (f32)window_width / (f32)window_height;

    camera_position = (HgVec3){0.0f, 0.0f, 0.0f};
    camera_scale = (HgVec3){1.0f, 1.0f, 1.0f};
    camera_rotation = (HgQuat){1.0f, 0.0f, 0.0f, 0.0f};

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

            camera_aspect = (f32)window_width / (f32)window_height;
            // HgMat4 proj = hg_projection_matrix_perspective(camera_fov, aspect, 0.1f, 100.0f);
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
        camera_position = hg_vadd3(
            camera_position, (HgVec3){0.0f, -(f32)delta * MOVE_SPEED, 0.0f}
        );
    }
    if (input_state & INPUT_STATE_DOWN) {
        camera_position = hg_vadd3(
            camera_position, (HgVec3){0.0f, (f32)delta * MOVE_SPEED, 0.0f}
        );
    }
    if (input_state & INPUT_STATE_FORWARD) {
        camera_position = hg_vadd3(
            camera_position, hg_rotate_vec3(camera_rotation, (HgVec3){0.0f, 0.0f, (f32)delta * MOVE_SPEED})
        );
    }
    if (input_state & INPUT_STATE_BACKWARD) {
        camera_position = hg_vadd3(
            camera_position, hg_rotate_vec3(camera_rotation, (HgVec3){0.0f, 0.0f, -(f32)delta * MOVE_SPEED})
        );
    }
    if (input_state & INPUT_STATE_LEFT) {
        camera_position = hg_vadd3(
            camera_position, hg_rotate_vec3(camera_rotation, (HgVec3){-(f32)delta * MOVE_SPEED, 0.0f, 0.0f})
        );
    }
    if (input_state & INPUT_STATE_RIGHT) {
        camera_position = hg_vadd3(
            camera_position, hg_rotate_vec3(camera_rotation, (HgVec3){(f32)delta * MOVE_SPEED, 0.0f, 0.0f})
        );
    }

    HgError begin_result = hg_frame_begin();
    if (begin_result != HG_SUCCESS) {
        HG_DEBUG("Failed to begin frame");
        return SDL_APP_CONTINUE;
    }

    HgMat4 camera = hg_model_matrix_3d(camera_position, camera_scale, camera_rotation);
    hg_ray_marcher_draw(target, &camera, camera_aspect);

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

    hg_texture_destroy(target);
    hg_ray_marcher_shutdown();

    hg_window_close();
    hg_shutdown();
#endif
}

