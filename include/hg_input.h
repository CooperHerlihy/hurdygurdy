#ifndef HG_INPUT_H
#define HG_INPUT_H

#include "hg_utils.h"

typedef enum HgKey {
    HG_KEY_NONE = 0,
    HG_KEY_ESCAPE,
    HG_KEY_SPACE,
    HG_KEY_ENTER,
    HG_KEY_TAB,
    HG_KEY_DELETE,
    HG_KEY_BACKSPACE,
    HG_KEY_LMOUSE,
    HG_KEY_RMOUSE,
    HG_KEY_MMOUSE,
    HG_KEY_UP,
    HG_KEY_DOWN,
    HG_KEY_LEFT,
    HG_KEY_RIGHT,
    HG_KEY_Q,
    HG_KEY_W,
    HG_KEY_E,
    HG_KEY_R,
    HG_KEY_T,
    HG_KEY_Y,
    HG_KEY_U,
    HG_KEY_I,
    HG_KEY_O,
    HG_KEY_P,
    HG_KEY_A,
    HG_KEY_S,
    HG_KEY_D,
    HG_KEY_F,
    HG_KEY_G,
    HG_KEY_H,
    HG_KEY_J,
    HG_KEY_K,
    HG_KEY_L,
    HG_KEY_Z,
    HG_KEY_X,
    HG_KEY_C,
    HG_KEY_V,
    HG_KEY_B,
    HG_KEY_N,
    HG_KEY_M,
    HG_KEY_LSHIFT,
    HG_KEY_RSHIFT,
    HG_KEY_LCTRL,
    HG_KEY_RCTRL,
    HG_KEY_LALT,
    HG_KEY_RALT,
    HG_KEY_LAST,
} HgKey;

void hg_process_events(void);

bool hg_was_window_closed(void);
bool hg_was_window_resized(void);

bool hg_is_key_down(HgKey key);
bool hg_was_key_pressed(HgKey key);
bool hg_was_key_released(HgKey key);

void hg_get_mouse_pos(f32* x, f32* y);
void hg_get_mouse_delta(f32* x, f32* y);

#endif // HG_INPUT_H
