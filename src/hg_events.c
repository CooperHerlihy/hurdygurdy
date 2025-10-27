#include "hg_events.h"
#include "hg_graphics.h"

#include <SDL3/SDL.h>

static bool s_window_closed = false;
static bool s_window_resized = false;

static bool s_keys_pressed[HG_KEY_LAST];
static bool s_keys_down[HG_KEY_LAST];
static bool s_keys_up[HG_KEY_LAST];

static f32 s_mouse_pos_x;
static f32 s_mouse_pos_y;
static f32 s_mouse_delta_x;
static f32 s_mouse_delta_y;

void hg_process_events(void) {
    memset(s_keys_down, 0, sizeof(s_keys_down));
    memset(s_keys_up, 0, sizeof(s_keys_up));
    s_window_resized = false;
    s_mouse_delta_x = 0.0f;
    s_mouse_delta_y = 0.0f;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                s_window_closed = true;
            } break;
            case SDL_EVENT_WINDOW_RESIZED: {
                hg_window_update_size();
                s_window_resized = true;
            } break;
            case SDL_EVENT_KEY_DOWN: {
                switch (event.key.key) {
                    case SDLK_ESCAPE: {
                        s_keys_down[HG_KEY_ESCAPE] = true;
                        s_keys_pressed[HG_KEY_ESCAPE] = true;
                    } break;
                    case SDLK_SPACE: {
                        s_keys_down[HG_KEY_SPACE] = true;
                        s_keys_pressed[HG_KEY_SPACE] = true;
                    } break;
                    case SDLK_RETURN: {
                        s_keys_down[HG_KEY_ENTER] = true;
                        s_keys_pressed[HG_KEY_ENTER] = true;
                    } break;
                    case SDLK_TAB: {
                        s_keys_down[HG_KEY_TAB] = true;
                        s_keys_pressed[HG_KEY_TAB] = true;
                    } break;
                    case SDLK_DELETE: {
                        s_keys_down[HG_KEY_DELETE] = true;
                        s_keys_pressed[HG_KEY_DELETE] = true;
                    } break;
                    case SDLK_BACKSPACE: {
                        s_keys_down[HG_KEY_BACKSPACE] = true;
                        s_keys_pressed[HG_KEY_BACKSPACE] = true;
                    } break;
                    case SDLK_LCTRL: {
                        s_keys_down[HG_KEY_LCTRL] = true;
                        s_keys_pressed[HG_KEY_LCTRL] = true;
                    } break;
                    case SDLK_RCTRL: {
                        s_keys_down[HG_KEY_RCTRL] = true;
                        s_keys_pressed[HG_KEY_RCTRL] = true;
                    } break;
                    case SDLK_LALT: {
                        s_keys_down[HG_KEY_LALT] = true;
                        s_keys_pressed[HG_KEY_LALT] = true;
                    } break;
                    case SDLK_RALT: {
                        s_keys_down[HG_KEY_RALT] = true;
                        s_keys_pressed[HG_KEY_RALT] = true;
                    } break;
                    case SDLK_LSHIFT: {
                        s_keys_down[HG_KEY_LSHIFT] = true;
                        s_keys_pressed[HG_KEY_LSHIFT] = true;
                    } break;
                    case SDLK_RSHIFT: {
                        s_keys_down[HG_KEY_RSHIFT] = true;
                        s_keys_pressed[HG_KEY_RSHIFT] = true;
                    } break;
                    case SDLK_UP: {
                        s_keys_down[HG_KEY_UP] = true;
                        s_keys_pressed[HG_KEY_UP] = true;
                    } break;
                    case SDLK_DOWN: {
                        s_keys_down[HG_KEY_DOWN] = true;
                        s_keys_pressed[HG_KEY_DOWN] = true;
                    } break;
                    case SDLK_LEFT: {
                        s_keys_down[HG_KEY_LEFT] = true;
                        s_keys_pressed[HG_KEY_LEFT] = true;
                    } break;
                    case SDLK_RIGHT: {
                        s_keys_down[HG_KEY_RIGHT] = true;
                        s_keys_pressed[HG_KEY_RIGHT] = true;
                    } break;
                    case SDLK_Q: {
                        s_keys_down[HG_KEY_Q] = true;
                        s_keys_pressed[HG_KEY_Q] = true;
                    } break;
                    case SDLK_W: {
                        s_keys_down[HG_KEY_W] = true;
                        s_keys_pressed[HG_KEY_W] = true;
                    } break;
                    case SDLK_E: {
                        s_keys_down[HG_KEY_E] = true;
                        s_keys_pressed[HG_KEY_E] = true;
                    } break;
                    case SDLK_R: {
                        s_keys_down[HG_KEY_R] = true;
                        s_keys_pressed[HG_KEY_R] = true;
                    } break;
                    case SDLK_T: {
                        s_keys_down[HG_KEY_T] = true;
                        s_keys_pressed[HG_KEY_T] = true;
                    } break;
                    case SDLK_Y: {
                        s_keys_down[HG_KEY_Y] = true;
                        s_keys_pressed[HG_KEY_Y] = true;
                    } break;
                    case SDLK_U: {
                        s_keys_down[HG_KEY_U] = true;
                        s_keys_pressed[HG_KEY_U] = true;
                    } break;
                    case SDLK_I: {
                        s_keys_down[HG_KEY_I] = true;
                        s_keys_pressed[HG_KEY_I] = true;
                    } break;
                    case SDLK_O: {
                        s_keys_down[HG_KEY_O] = true;
                        s_keys_pressed[HG_KEY_O] = true;
                    } break;
                    case SDLK_P: {
                        s_keys_down[HG_KEY_P] = true;
                        s_keys_pressed[HG_KEY_P] = true;
                    } break;
                    case SDLK_A: {
                        s_keys_down[HG_KEY_A] = true;
                        s_keys_pressed[HG_KEY_A] = true;
                    } break;
                    case SDLK_S: {
                        s_keys_down[HG_KEY_S] = true;
                        s_keys_pressed[HG_KEY_S] = true;
                    } break;
                    case SDLK_D: {
                        s_keys_down[HG_KEY_D] = true;
                        s_keys_pressed[HG_KEY_D] = true;
                    } break;
                    case SDLK_F: {
                        s_keys_down[HG_KEY_F] = true;
                        s_keys_pressed[HG_KEY_F] = true;
                    } break;
                    case SDLK_G: {
                        s_keys_down[HG_KEY_G] = true;
                        s_keys_pressed[HG_KEY_G] = true;
                    } break;
                    case SDLK_H: {
                        s_keys_down[HG_KEY_H] = true;
                        s_keys_pressed[HG_KEY_H] = true;
                    } break;
                    case SDLK_J: {
                        s_keys_down[HG_KEY_J] = true;
                        s_keys_pressed[HG_KEY_J] = true;
                    } break;
                    case SDLK_K: {
                        s_keys_down[HG_KEY_K] = true;
                        s_keys_pressed[HG_KEY_K] = true;
                    } break;
                    case SDLK_L: {
                        s_keys_down[HG_KEY_L] = true;
                        s_keys_pressed[HG_KEY_L] = true;
                    } break;
                    case SDLK_Z: {
                        s_keys_down[HG_KEY_Z] = true;
                        s_keys_pressed[HG_KEY_Z] = true;
                    } break;
                    case SDLK_X: {
                        s_keys_down[HG_KEY_X] = true;
                        s_keys_pressed[HG_KEY_X] = true;
                    } break;
                    case SDLK_C: {
                        s_keys_down[HG_KEY_C] = true;
                        s_keys_pressed[HG_KEY_C] = true;
                    } break;
                    case SDLK_V: {
                        s_keys_down[HG_KEY_V] = true;
                        s_keys_pressed[HG_KEY_V] = true;
                    } break;
                    case SDLK_B: {
                        s_keys_down[HG_KEY_B] = true;
                        s_keys_pressed[HG_KEY_B] = true;
                    } break;
                    case SDLK_N: {
                        s_keys_down[HG_KEY_N] = true;
                        s_keys_pressed[HG_KEY_N] = true;
                    } break;
                    case SDLK_M: {
                        s_keys_down[HG_KEY_M] = true;
                        s_keys_pressed[HG_KEY_M] = true;
                    } break;
                }
            } break;
            case SDL_EVENT_KEY_UP: {
                switch (event.key.key) {
                    case SDLK_ESCAPE: {
                        s_keys_up[HG_KEY_ESCAPE] = true;
                        s_keys_pressed[HG_KEY_ESCAPE] = false;
                    } break;
                    case SDLK_SPACE: {
                        s_keys_up[HG_KEY_SPACE] = true;
                        s_keys_pressed[HG_KEY_SPACE] = false;
                    } break;
                    case SDLK_RETURN: {
                        s_keys_up[HG_KEY_ENTER] = true;
                        s_keys_pressed[HG_KEY_ENTER] = false;
                    } break;
                    case SDLK_TAB: {
                        s_keys_up[HG_KEY_TAB] = true;
                        s_keys_pressed[HG_KEY_TAB] = false;
                    } break;
                    case SDLK_DELETE: {
                        s_keys_up[HG_KEY_DELETE] = true;
                        s_keys_pressed[HG_KEY_DELETE] = false;
                    } break;
                    case SDLK_BACKSPACE: {
                        s_keys_up[HG_KEY_BACKSPACE] = true;
                        s_keys_pressed[HG_KEY_BACKSPACE] = false;
                    } break;
                    case SDLK_LCTRL: {
                        s_keys_up[HG_KEY_LCTRL] = true;
                        s_keys_pressed[HG_KEY_LCTRL] = false;
                    } break;
                    case SDLK_RCTRL: {
                        s_keys_up[HG_KEY_RCTRL] = true;
                        s_keys_pressed[HG_KEY_RCTRL] = false;
                    } break;
                    case SDLK_LALT: {
                        s_keys_up[HG_KEY_LALT] = true;
                        s_keys_pressed[HG_KEY_LALT] = false;
                    } break;
                    case SDLK_RALT: {
                        s_keys_up[HG_KEY_RALT] = true;
                        s_keys_pressed[HG_KEY_RALT] = false;
                    } break;
                    case SDLK_LSHIFT: {
                        s_keys_up[HG_KEY_LSHIFT] = true;
                        s_keys_pressed[HG_KEY_LSHIFT] = false;
                    } break;
                    case SDLK_RSHIFT: {
                        s_keys_up[HG_KEY_RSHIFT] = true;
                        s_keys_pressed[HG_KEY_RSHIFT] = false;
                    } break;
                    case SDLK_UP: {
                        s_keys_up[HG_KEY_UP] = true;
                        s_keys_pressed[HG_KEY_UP] = false;
                    } break;
                    case SDLK_DOWN: {
                        s_keys_up[HG_KEY_DOWN] = true;
                        s_keys_pressed[HG_KEY_DOWN] = false;
                    } break;
                    case SDLK_LEFT: {
                        s_keys_up[HG_KEY_LEFT] = true;
                        s_keys_pressed[HG_KEY_LEFT] = false;
                    } break;
                    case SDLK_RIGHT: {
                        s_keys_up[HG_KEY_RIGHT] = true;
                        s_keys_pressed[HG_KEY_RIGHT] = false;
                    } break;
                    case SDLK_Q: {
                        s_keys_up[HG_KEY_Q] = true;
                        s_keys_pressed[HG_KEY_Q] = false;
                    } break;
                    case SDLK_W: {
                        s_keys_up[HG_KEY_W] = true;
                        s_keys_pressed[HG_KEY_W] = false;
                    } break;
                    case SDLK_E: {
                        s_keys_up[HG_KEY_E] = true;
                        s_keys_pressed[HG_KEY_E] = false;
                    } break;
                    case SDLK_R: {
                        s_keys_up[HG_KEY_R] = true;
                        s_keys_pressed[HG_KEY_R] = false;
                    } break;
                    case SDLK_T: {
                        s_keys_up[HG_KEY_T] = true;
                        s_keys_pressed[HG_KEY_T] = false;
                    } break;
                    case SDLK_Y: {
                        s_keys_up[HG_KEY_Y] = true;
                        s_keys_pressed[HG_KEY_Y] = false;
                    } break;
                    case SDLK_U: {
                        s_keys_up[HG_KEY_U] = true;
                        s_keys_pressed[HG_KEY_U] = false;
                    } break;
                    case SDLK_I: {
                        s_keys_up[HG_KEY_I] = true;
                        s_keys_pressed[HG_KEY_I] = false;
                    } break;
                    case SDLK_O: {
                        s_keys_up[HG_KEY_O] = true;
                        s_keys_pressed[HG_KEY_O] = false;
                    } break;
                    case SDLK_P: {
                        s_keys_up[HG_KEY_P] = true;
                        s_keys_pressed[HG_KEY_P] = false;
                    } break;
                    case SDLK_A: {
                        s_keys_up[HG_KEY_A] = true;
                        s_keys_pressed[HG_KEY_A] = false;
                    } break;
                    case SDLK_S: {
                        s_keys_up[HG_KEY_S] = true;
                        s_keys_pressed[HG_KEY_S] = false;
                    } break;
                    case SDLK_D: {
                        s_keys_up[HG_KEY_D] = true;
                        s_keys_pressed[HG_KEY_D] = false;
                    } break;
                    case SDLK_F: {
                        s_keys_up[HG_KEY_F] = true;
                        s_keys_pressed[HG_KEY_F] = false;
                    } break;
                    case SDLK_G: {
                        s_keys_up[HG_KEY_G] = true;
                        s_keys_pressed[HG_KEY_G] = false;
                    } break;
                    case SDLK_H: {
                        s_keys_up[HG_KEY_H] = true;
                        s_keys_pressed[HG_KEY_H] = false;
                    } break;
                    case SDLK_J: {
                        s_keys_up[HG_KEY_J] = true;
                        s_keys_pressed[HG_KEY_J] = false;
                    } break;
                    case SDLK_K: {
                        s_keys_up[HG_KEY_K] = true;
                        s_keys_pressed[HG_KEY_K] = false;
                    } break;
                    case SDLK_L: {
                        s_keys_up[HG_KEY_L] = true;
                        s_keys_pressed[HG_KEY_L] = false;
                    } break;
                    case SDLK_Z: {
                        s_keys_up[HG_KEY_Z] = true;
                        s_keys_pressed[HG_KEY_Z] = false;
                    } break;
                    case SDLK_X: {
                        s_keys_up[HG_KEY_X] = true;
                        s_keys_pressed[HG_KEY_X] = false;
                    } break;
                    case SDLK_C: {
                        s_keys_up[HG_KEY_C] = true;
                        s_keys_pressed[HG_KEY_C] = false;
                    } break;
                    case SDLK_V: {
                        s_keys_up[HG_KEY_V] = true;
                        s_keys_pressed[HG_KEY_V] = false;
                    } break;
                    case SDLK_B: {
                        s_keys_up[HG_KEY_B] = true;
                        s_keys_pressed[HG_KEY_B] = false;
                    } break;
                    case SDLK_N: {
                        s_keys_up[HG_KEY_N] = true;
                        s_keys_pressed[HG_KEY_N] = false;
                    } break;
                    case SDLK_M: {
                        s_keys_up[HG_KEY_M] = true;
                        s_keys_pressed[HG_KEY_M] = false;
                    } break;
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT: {
                        s_keys_down[HG_KEY_LMOUSE] = true;
                        s_keys_pressed[HG_KEY_LMOUSE] = true;
                    } break;
                    case SDL_BUTTON_RIGHT: {
                        s_keys_down[HG_KEY_RMOUSE] = true;
                        s_keys_pressed[HG_KEY_RMOUSE] = true;
                    } break;
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT: {
                        s_keys_up[HG_KEY_LMOUSE] = true;
                        s_keys_pressed[HG_KEY_LMOUSE] = false;
                    } break;
                    case SDL_BUTTON_RIGHT: {
                        s_keys_up[HG_KEY_RMOUSE] = true;
                        s_keys_pressed[HG_KEY_RMOUSE] = false;
                    } break;
                }
            } break;
            case SDL_EVENT_MOUSE_MOTION: {
                s_mouse_pos_x = event.motion.x;
                s_mouse_pos_y = event.motion.y;
                s_mouse_delta_x += event.motion.xrel;
                s_mouse_delta_y += event.motion.yrel;
            } break;
            default: break;
        }
    }
}

bool hg_was_window_closed(void) {
    return s_window_closed;
}

bool hg_was_window_resized(void) {
    return s_window_resized;
}

bool hg_get_key_pressed(HgKey key) {
    return s_keys_pressed[key];
}

bool hg_get_key_down(HgKey key) {
    return s_keys_down[key];
}

bool hg_get_key_up(HgKey key) {
    return s_keys_up[key];
}

void hg_get_mouse_pos(f32* x, f32* y) {
    *x = s_mouse_pos_x;
    *y = s_mouse_pos_y;
}

void hg_get_mouse_delta(f32* x, f32* y) {
    *x = s_mouse_delta_x;
    *y = s_mouse_delta_y;
}

