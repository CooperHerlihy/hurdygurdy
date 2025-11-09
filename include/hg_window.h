#ifndef HG_WINDOW_H
#define HG_WINDOW_H

#include "hg_enums.h"
#include "hg_utils.h"
#include "hg_init.h"
#include "hg_graphics.h"

/**
 * A window
 */
typedef struct HgWindow HgWindow;

/**
 * Configuration for a window
 *
 * title is the title of the window, must not be NULL
 * windowed is whether it is windowed or fullscreen
 * width and height are the size if windowed, otherwise ignored
 */
typedef struct HgWindowConfig {
    const char* title;
    bool windowed;
    u32 width;
    u32 height;
} HgWindowConfig;

/**
 * Creates a window
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param config The window configuration, must not be NULL
 * \return A pointer to the window
 */
HgWindow* hg_window_create(
    const HurdyGurdy* hg,
    const HgWindowConfig* config
);

/**
 * Destroys a window
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param window The window, must not be NULL
 */
void hg_window_destroy(
    const HurdyGurdy* hg,
    HgWindow* window
);

/**
 * Begins a command buffer for rendering
 *
 * Note, only one frame can be recorded at the same time per window
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param window The window to render to, must not be NULL
 * \return the created command buffer, NULL on failure, just skip rendering,
 * process events, and try again
 */
HgCommands* hg_window_begin_frame(
    const HurdyGurdy* hg,
    HgWindow* window
);

/**
 * Ends the command buffer for rendering and presents to the window
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param window The window to render to, must not be NULL
 * \param commands The command buffer
 * \param framebuffer The framebuffer to present, does not present if NULL
 */
void hg_window_end_frame(
    const HurdyGurdy* hg,
    HgWindow* window,
    HgCommands* commands,
    HgTexture* framebuffer
);

/**
 * Processes all events since the last call to hg_process_events() or startup
 *
 * This function must be called every frame before querying input
 * This function processes all events, so all windows must be given
 *
 * \param hg The HurdyGurdy context, must not be NULL
 * \param windows All open windows
 */
void hg_window_process_events(
    const HurdyGurdy* hg,
    HgWindow** windows,
    u32 window_count
);

/**
 * Checks if the window was closed via close button or window manager
 *
 * hg_window_close() is not automatically called when this function returns
 * true, and may be called manually
 *
 * \param window The window, must not be NULL
 * \return whether the window was closed
 */
bool hg_window_was_closed(
    const HgWindow* window
);

/**
 * Checks if the window was resized
 *
 * \param window The window, must not be NULL
 * \return whether the window was resized
 */
bool hg_window_was_resized(
    const HgWindow* window
);

/**
 * Gets the size of the window
 *
 * \param window The window, must not be NULL
 * \param width A pointer to store the width, must not be NULL
 * \param height A pointer to store the height, must not be NULL
 */
void hg_window_get_size(
    const HgWindow* window,
    u32* width,
    u32* height
);

/**
 * Checks if a key is being held down
 *
 * \param window The window, must not be NULL
 * \param key The key to check
 * \return whether the key is being held down
 */
bool hg_window_is_key_down(
    const HgWindow* window,
    HgKey key
);

/**
 * Checks if a key was pressed this frame
 *
 * \param window The window, must not be NULL
 * \param key The key to check
 * \return whether the key was pressed this frame
 */
bool hg_window_was_key_pressed(
    const HgWindow* window,
    HgKey key
);

/**
 * Checks if a key was released this frame
 *
 * \param window The window, must not be NULL
 * \param key The key to check
 * \return whether the key was released this frame
 */
bool hg_window_was_key_released(
    const HgWindow* window,
    HgKey key
);

/**
 * Gets the most recent mouse position
 *
 * \param window The window, must not be NULL
 * \param x A pointer to store the x position, must not be NULL
 * \param y A pointer to store the y position, must not be NULL
 */
void hg_window_get_mouse_pos(
    const HgWindow* window,
    f32* x,
    f32* y
);

/**
 * Gets the most recent mouse delta
 *
 * \param window The window, must not be NULL
 * \param x A pointer to store the x delta, must not be NULL
 * \param y A pointer to store the y delta, must not be NULL
 */
void hg_window_get_mouse_delta(
    const HgWindow* window,
    f32* x,
    f32* y
);

#endif // HG_WINDOW_H
