/*
 * =============================================================================
 *
 * Copyright (c) 2025-2026 Cooper Herlihy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 */

#ifndef HG_WINDOW_HPP
#define HG_WINDOW_HPP

#include "hg_core.hpp"
#include "hg_gpu.hpp"

namespace hg {

/**
 * Initialize the windowing subsystem
 */
void windowsInit();

/**
 * Deinitialize the windowing subsystem
 */
void windowsDeinit();

/**
 * The present mode for the swapchain
 */
enum GpuPresentMode : u32 {
    GpuPresentMode_immediate = 0,
    GpuPresentMode_mailbox = 1,
    GpuPresentMode_fifo = 2,
    GpuPresentMode_fifoRelaxed = 3,
};

/**
 * Configuration for a window
 */
struct WindowConfig {
    /**
     * Whether the window can be resized
     */
    bool fixedSize = false;
    /**
     * Whether the window should be windowed or fullscreen
     */
    bool fullscreen = false;
    /**
     * How the swapchain images will be presented
     *
     * Note, will fall back to FIFO if preferred is unavailable
     */
    GpuPresentMode preferredPresentMode = GpuPresentMode_fifo;
    /**
     * How the swapchain images will be used
     */
    GpuImageUsageFlags imageUsage = GpuImageUsage_colorAttachment;
};

/**
 * A window
 */
struct Window;

/**
 * Create a new window
 *
 * Note, width and height are ignored if fullscreen is enabled
 *
 * Returns
 * - The created window, or nullptr on failure
 */
Window* windowCreate(String title, u32 width, u32 height, const WindowConfig* config);

/**
 * Destroy a window
 */
void windowDestroy(Window* window);

/**
 * Acquire an image from each swapchain and begin a command buffer
 *
 * Returns
 * - The command buffer to record this frame
 */
GpuCmd* gpuFrameBegin(Window** windows, u32 windowCount);

/**
 * Finishes recording the command buffer and presents the window images
 *
 * Parameters
 * - cmd The command buffer given from beginFrame
 */
void gpuFrameEnd(GpuCmd* cmd);

/**
 * Returns the window's current image, or nullptr if unavailable this frame
 */
GpuView* windowImageView(Window* window);

/**
 * Returns the window's width in pixels
 */
Format windowImageFormat(Window* window);

/**
 * Processes all events since startup or the last call to process events
 */
void processEvents();

/**
 * The types of events
 */
enum WindowEventType : u32 {
    WindowEventType_none = 0,
    WindowEventType_buttonPress,
    WindowEventType_buttonRelease,
    WindowEventType_count,
};

/**
 * The button inputs
 */
enum Button : u32 {
    Button_none = 0,
    Button_k0,
    Button_k1,
    Button_k2,
    Button_k3,
    Button_k4,
    Button_k5,
    Button_k6,
    Button_k7,
    Button_k8,
    Button_k9,
    Button_q,
    Button_w,
    Button_e,
    Button_r,
    Button_t,
    Button_y,
    Button_u,
    Button_i,
    Button_o,
    Button_p,
    Button_a,
    Button_s,
    Button_d,
    Button_f,
    Button_g,
    Button_h,
    Button_j,
    Button_k,
    Button_l,
    Button_z,
    Button_x,
    Button_c,
    Button_v,
    Button_b,
    Button_n,
    Button_m,
    Button_semicolon,
    Button_colon,
    Button_apostrophe,
    Button_quotation,
    Button_comma,
    Button_period,
    Button_question,
    Button_grave,
    Button_tilde,
    Button_exclamation,
    Button_at,
    Button_hash,
    Button_dollar,
    Button_percent,
    Button_carot,
    Button_ampersand,
    Button_asterisk,
    Button_lparen,
    Button_rparen,
    Button_lbracket,
    Button_rbracket,
    Button_lbrace,
    Button_rbrace,
    Button_equal,
    Button_less,
    Button_greater,
    Button_plus,
    Button_minus,
    Button_slash,
    Button_backslash,
    Button_underscore,
    Button_bar,
    Button_up,
    Button_down,
    Button_left,
    Button_right,
    Button_mouse1,
    Button_mouse2,
    Button_mouse3,
    Button_mouse4,
    Button_mouse5,
    Button_lmouse = Button_mouse1,
    Button_rmouse = Button_mouse2,
    Button_mmouse = Button_mouse3,
    Button_escape,
    Button_space,
    Button_enter,
    Button_backspace,
    Button_kdelete,
    Button_insert,
    Button_tab,
    Button_home,
    Button_end,
    Button_f1,
    Button_f2,
    Button_f3,
    Button_f4,
    Button_f5,
    Button_f6,
    Button_f7,
    Button_f8,
    Button_f9,
    Button_f10,
    Button_f11,
    Button_f12,
    Button_lshift,
    Button_rshift,
    Button_lctrl,
    Button_rctrl,
    Button_lmeta,
    Button_rmeta,
    Button_lalt,
    Button_ralt,
    Button_lsuper,
    Button_rsuper,
    Button_capslock,
    Button_count,
};

/**
 * A button input event
 */
struct WindowButtonEvent {
    /**
     * The type of event
     */
    WindowEventType type;
    /**
     * The button which was pressed or released
     */
    Button button;
};

/**
 * Input event data
 */
union WindowEvent {
    /**
     * The type of event
     */
    WindowEventType type;
    /**
     * The button press or release event
     */
    WindowButtonEvent button;
};

/**
 * Returns whether the application was quit
 */
bool wasQuit();

/**
 * Returns whether the window was closed
 */
bool windowWasClosed(Window* window);

/**
 * Returns whether the mouse is focused on the window
 */
bool windowIsFocused(Window* window);

/**
 * Get the window's width in pixels
 */
u32 windowWidth(Window* window);

/**
 * Get the window's width in pixels
 */
u32 windowHeight(Window* window);

/**
 * Returns the current x position of the mouse relative to the window
 */
f32 mouseX(Window* window);

/**
 * Returns the current y position of the mouse relative to the window
 */
f32 mouseY(Window* window);

/**
 * Returns the change in x position of the mouse relative to the window height
 */
f32 mouseDeltaX(Window* window);

/**
 * Returns the change in y position of the mouse relative to the window height
 */
f32 mouseDeltaY(Window* window);

/**
 * Returns whether the key is currently down
 */
bool isButtonDown(Window* window, Button key);

/**
 * Get the key events since last event processing
 */
WindowEvent* windowEvents(Window* window, u32* count);

} // namespace hg

#endif // WINDOW_HPP
