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
    INPUT_STATE_LMOUSE = 0x10,
    INPUT_STATE_RMOUSE = 0x20,
} InputState;
static i32 s_input_state;

HgTexture* s_target;
HgTexture* s_depth_buffer;

static const u32 s_texture_data[] = {
    0xffff0000, 0xff00ff00,
    0xff0000ff, 0xff00ffff,
};
static HgTexture* s_texture;

static HgVec3 s_camera_position;
static HgTransform2D s_transform;

static HgClock s_clock;
static f64 s_time_elapsed;
static u64 s_frame_count;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    (void)appstate;
    (void)argc;
    (void)argv;

    hg_init();
    hg_2d_renderer_init();

    hg_window_open(&(HgWindowConfig){
        .title = "Hurdy Gurdy",
        .width = 800,
        .height = 600,
        .windowed = true,
    });

    u32 window_width, window_height;
    hg_window_get_size(&window_width, &window_height);

    hg_2d_renderer_target_create(window_width, window_height, &s_target, &s_depth_buffer);
    hg_2d_renderer_update_projection((f32)window_width / (f32)window_height, 0.0f, 8.0f);

    s_texture = hg_2d_sprite_create(s_texture_data, 2, 2, HG_FORMAT_R8G8B8A8_UNORM, false);

    s_camera_position = (HgVec3){0.0f, 0.0f, 0.0f};
    s_transform = (HgTransform2D){
        .position = (HgVec3){-0.2f, -0.2f, 0.0f},
        .scale = (HgVec2){0.4f, 0.4f},
        .rotation = 0.0f,
    };

    (void)hg_clock_tick(&s_clock);

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
                case SDLK_W: {
                    s_input_state |= INPUT_STATE_UP;
                } break;
                case SDLK_S: {
                    s_input_state |= INPUT_STATE_DOWN;
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
                case SDLK_W: {
                    s_input_state &= ~INPUT_STATE_UP;
                } break;
                case SDLK_S: {
                    s_input_state &= ~INPUT_STATE_DOWN;
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
        case SDL_EVENT_WINDOW_RESIZED: {
            u32 window_width, window_height;
            hg_window_update_size();
            hg_window_get_size(&window_width, &window_height);

            hg_texture_destroy(s_target);
            hg_texture_destroy(s_depth_buffer);
            hg_2d_renderer_target_create(window_width, window_height, &s_target, &s_depth_buffer);
            hg_2d_renderer_update_projection((f32)window_width / (f32)window_height, 0.0f, 8.0f);
        } break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    (void)appstate;

    f64 delta = hg_clock_tick(&s_clock);
    s_time_elapsed += delta;
    ++s_frame_count;
    if (s_time_elapsed > 1.0) {
        HG_LOGF("avg: %fms, fps: %" PRIu64, 1.0e3 / (f64)s_frame_count, s_frame_count);
        s_time_elapsed -= 1.0;
        s_frame_count = 0;
    }

    if (s_input_state & INPUT_STATE_UP) {
        s_camera_position.y -= (f32)delta * MOVE_SPEED;
        s_transform.position.y -= (f32)delta * MOVE_SPEED;
    }
    if (s_input_state & INPUT_STATE_DOWN) {
        s_camera_position.y += (f32)delta * MOVE_SPEED;
        s_transform.position.y += (f32)delta * MOVE_SPEED;
    }
    if (s_input_state & INPUT_STATE_LEFT) {
        s_camera_position.x -= (f32)delta * MOVE_SPEED;
        s_transform.position.x -= (f32)delta * MOVE_SPEED;
    }
    if (s_input_state & INPUT_STATE_RIGHT) {
        s_camera_position.x += (f32)delta * MOVE_SPEED;
        s_transform.position.x += (f32)delta * MOVE_SPEED;
    }
    hg_2d_renderer_update_view(s_camera_position, 1.0f, (HgQuat){1.0f, 0.0f, 0.0f, 0.0f});

    if (s_input_state & INPUT_STATE_LMOUSE) {
        s_transform.position.z += (f32)delta * MOVE_SPEED;
    }
    if (s_input_state & INPUT_STATE_RMOUSE) {
        s_transform.position.z -= (f32)delta * MOVE_SPEED;
    }

    HgError begin_result = hg_frame_begin();
    if (begin_result != HG_SUCCESS) {
        HG_DEBUG("Failed to begin frame");
        return SDL_APP_CONTINUE;
    }

    hg_2d_renderer_queue_sprite(s_texture, (HgVec2){0.5f, 0.0f}, (HgVec2){0.5f, 0.5f}, &(HgTransform2D){
        .position = (HgVec3){-0.5f, -0.5f, 1.0f},
        .scale = (HgVec2){1.0f, 1.0f},
        .rotation = 0.0f,
    });

    hg_2d_renderer_queue_sprite(s_texture, (HgVec2){0.0f, 0.0f}, (HgVec2){1.0f, 1.0f}, &s_transform);

    hg_2d_renderer_draw(s_target, s_depth_buffer);

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

    hg_texture_destroy(s_texture);
    hg_texture_destroy(s_depth_buffer);
    hg_texture_destroy(s_target);
    hg_2d_renderer_shutdown();

    hg_window_close();
    hg_shutdown();
#endif
}

