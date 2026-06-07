/*
 * =============================================================================
 *
 * Copyright (c) 2025 Cooper Herlihy
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

/**
 * Initialize the windowing subsystem
 */
void hgWindowsInit();

/**
 * Deinitialize the windowing subsystem
 */
void hgWindowsDeinit();

/**
 * The present mode for the swapchain
 */
enum HgGpuPresentMode : u32 {
    HgGpuPresentMode_immediate = 0,
    HgGpuPresentMode_mailbox = 1,
    HgGpuPresentMode_fifo = 2,
    HgGpuPresentMode_fifoRelaxed = 3,
};

/**
 * Configuration for a window
 */
struct HgWindowConfig {
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
    HgGpuPresentMode preferredPresentMode = HgGpuPresentMode_fifo;
    /**
     * How the swapchain images will be used
     */
    HgGpuImageUsageFlags imageUsage = HgGpuImageUsage_colorAttachment;
};

/**
 * A window
 */
struct HgWindow;

/**
 * Create a new window
 *
 * Note, width and height are ignored if fullscreen is enabled
 */
HgWindow* hgWindowCreate(const char* title, u32 width, u32 height, const HgWindowConfig* config);

/**
 * Destroy a window
 */
void hgWindowDestroy(HgWindow* window);

/**
 * Acquire an image from each swapchain and begin a command buffer
 *
 * Returns
 * - The command buffer to record this frame
 */
HgGpuCmd* hgGpuFrameBegin(HgWindow** windows, u32 windowCount);

/**
 * Finishes recording the command buffer and presents the window images
 *
 * Parameters
 * - cmd The command buffer given from beginFrame
 */
void hgGpuFrameEnd(HgGpuCmd* cmd);

/**
 * Returns the window's current image, or nullptr if it could not be acquired
 */
HgGpuView* hgWindowImageView(HgWindow* window);

/**
 * Get the window's width in pixels
 */
HgFormat hgWindowImageFormat(HgWindow* window);

/**
 * Processes all events since startup or the last call to process events
 */
void hgProcessEvents();

/**
 * The types of events
 */
enum HgWindowEventType : u32 {
    HgWindowEventType_none = 0,
    HgWindowEventType_buttonPress,
    HgWindowEventType_buttonRelease,
    HgWindowEventType_count,
};

/**
 * The button inputs
 */
enum HgButton : u32 {
    HgButton_none = 0,
    HgButton_k0,
    HgButton_k1,
    HgButton_k2,
    HgButton_k3,
    HgButton_k4,
    HgButton_k5,
    HgButton_k6,
    HgButton_k7,
    HgButton_k8,
    HgButton_k9,
    HgButton_q,
    HgButton_w,
    HgButton_e,
    HgButton_r,
    HgButton_t,
    HgButton_y,
    HgButton_u,
    HgButton_i,
    HgButton_o,
    HgButton_p,
    HgButton_a,
    HgButton_s,
    HgButton_d,
    HgButton_f,
    HgButton_g,
    HgButton_h,
    HgButton_j,
    HgButton_k,
    HgButton_l,
    HgButton_z,
    HgButton_x,
    HgButton_c,
    HgButton_v,
    HgButton_b,
    HgButton_n,
    HgButton_m,
    HgButton_semicolon,
    HgButton_colon,
    HgButton_apostrophe,
    HgButton_quotation,
    HgButton_comma,
    HgButton_period,
    HgButton_question,
    HgButton_grave,
    HgButton_tilde,
    HgButton_exclamation,
    HgButton_at,
    HgButton_hash,
    HgButton_dollar,
    HgButton_percent,
    HgButton_carot,
    HgButton_ampersand,
    HgButton_asterisk,
    HgButton_lparen,
    HgButton_rparen,
    HgButton_lbracket,
    HgButton_rbracket,
    HgButton_lbrace,
    HgButton_rbrace,
    HgButton_equal,
    HgButton_less,
    HgButton_greater,
    HgButton_plus,
    HgButton_minus,
    HgButton_slash,
    HgButton_backslash,
    HgButton_underscore,
    HgButton_bar,
    HgButton_up,
    HgButton_down,
    HgButton_left,
    HgButton_right,
    HgButton_mouse1,
    HgButton_mouse2,
    HgButton_mouse3,
    HgButton_mouse4,
    HgButton_mouse5,
    HgButton_lmouse = HgButton_mouse1,
    HgButton_rmouse = HgButton_mouse2,
    HgButton_mmouse = HgButton_mouse3,
    HgButton_escape,
    HgButton_space,
    HgButton_enter,
    HgButton_backspace,
    HgButton_kdelete,
    HgButton_insert,
    HgButton_tab,
    HgButton_home,
    HgButton_end,
    HgButton_f1,
    HgButton_f2,
    HgButton_f3,
    HgButton_f4,
    HgButton_f5,
    HgButton_f6,
    HgButton_f7,
    HgButton_f8,
    HgButton_f9,
    HgButton_f10,
    HgButton_f11,
    HgButton_f12,
    HgButton_lshift,
    HgButton_rshift,
    HgButton_lctrl,
    HgButton_rctrl,
    HgButton_lmeta,
    HgButton_rmeta,
    HgButton_lalt,
    HgButton_ralt,
    HgButton_lsuper,
    HgButton_rsuper,
    HgButton_capslock,
    HgButton_count,
};

/**
 * A button input event
 */
struct HgWindowButtonEvent {
    /**
     * The type of event
     */
    HgWindowEventType type;
    /**
     * The button which was pressed or released
     */
    HgButton button;
};

/**
 * Input event data
 */
union HgWindowEvent {
    /**
     * The type of event
     */
    HgWindowEventType type;
    /**
     * The button press or release event
     */
    HgWindowButtonEvent button;
};

/**
 * Returns whether the application was quit
 */
bool hgWasQuit();

/**
 * Returns whether the window was closed
 */
bool hgWindowWasClosed(HgWindow* window);

/**
 * Returns whether the mouse is focused on the window
 */
bool hgWindowIsFocused(HgWindow* window);

/**
 * Get the window's width in pixels
 */
u32 hgWindowWidth(HgWindow* window);

/**
 * Get the window's width in pixels
 */
u32 hgWindowHeight(HgWindow* window);

/**
 * Returns the current x position of the mouse relative to the window
 */
f32 hgMouseX(HgWindow* window);

/**
 * Returns the current y position of the mouse relative to the window
 */
f32 hgMouseY(HgWindow* window);

/**
 * Returns the change in x position of the mouse relative to the window height
 */
f32 hgMouseDeltaX(HgWindow* window);

/**
 * Returns the change in y position of the mouse relative to the window height
 */
f32 hgMouseDeltaY(HgWindow* window);

/**
 * Returns whether the key is currently down
 */
bool hgIsButtonDown(HgWindow* window, HgButton key);

/**
 * Get the key events since last event processing
 */
HgWindowEvent* hgWindowEvents(HgWindow* window, u32* count);

#endif // HG_WINDOW_HPP
