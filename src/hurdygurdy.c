#include "hg_internal.h"

bool hg_window_was_closed(
    const HgWindow* window
) {
    HG_ASSERT(window != NULL);
    return window->was_closed;
}

bool hg_window_was_resized(
    const HgWindow* window
) {
    HG_ASSERT(window != NULL);
    return window->was_resized;
}

void hg_window_get_size(
    const HgWindow* window,
    u32* width,
    u32* height
) {
    HG_ASSERT(window != NULL);
    HG_ASSERT(width != NULL);
    HG_ASSERT(height != NULL);
    *width = window->swapchain_width;
    *height = window->swapchain_height;
}

bool hg_window_is_key_down(
    const HgWindow* window,
    HgKey key
) {
    HG_ASSERT(window != NULL);
    return window->keys_down[key];
}

bool hg_window_was_key_pressed(
    const HgWindow* window,
    HgKey key
) {
    HG_ASSERT(window != NULL);
    return window->keys_pressed[key];
}

bool hg_window_was_key_released(
    const HgWindow* window,
    HgKey key
) {
    HG_ASSERT(window != NULL);
    return window->keys_released[key];
}

void hg_window_get_mouse_pos(
    const HgWindow* window,
    f32* x,
    f32* y
) {
    HG_ASSERT(window != NULL);
    HG_ASSERT(x != NULL);
    HG_ASSERT(y != NULL);
    *x = window->mouse_pos_x;
    *y = window->mouse_pos_y;
}

void hg_window_get_mouse_delta(
    const HgWindow* window,
    f32* x,
    f32* y
) {
    HG_ASSERT(window != NULL);
    HG_ASSERT(x != NULL);
    HG_ASSERT(y != NULL);
    *x = window->mouse_delta_x;
    *y = window->mouse_delta_y;
}

