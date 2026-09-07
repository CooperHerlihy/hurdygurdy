#pragma once

#include "hg/inttypes.hpp"
#include "hg/span.hpp"
#include "hg/smart_ptr.hpp"
#include "hg/strings.hpp"
#include "hg/gpu.hpp"

namespace hg {

/**
 * Display enumeration info
 */
struct DisplayInfo {
    i32 posX = 0;
    i32 posY = 0;
    u32 sizeW = 0;
    u32 sizeH = 0;
    i32 workPosX = 0;
    i32 workPosY = 0;
    u32 workSizeW = 0;
    u32 workSizeH = 0;
    f32 dpiScale = 1.0f;
};

/**
 * Returns the display info
 */
Span<DisplayInfo> displayInfo();

/**
 * The types of cursors
 */
enum CursorType : u32 {
    CursorType_arrow,
    CursorType_textInput,
    CursorType_resizeAll,
    CursorType_resizeNS,
    CursorType_resizeEW,
    CursorType_resizeNESW,
    CursorType_resizeNWSE,
    CursorType_hand,
    CursorType_wait,
    CursorType_progress,
    CursorType_notAllowed,
    CursorType_count,
};

/**
 * Set the current cursor
 */
void setCursor(CursorType type);

/**
 * Show or hide the cursor
 */
void showCursor(bool show = true);

/**
 * Returns the platform clipboard text
 */
String getClipboardText();

/**
 * Set the platform clipboard text
 */
void setClipboardText(StringView text);

/**
 * Open a URL in the platform's default handler
 */
void openURL(StringView url);

/**
 * Processes all events since startup or the last call to process events
 */
void processEvents();

/**
 * Returns whether the application was quit
 */
bool wasQuit();

/**
 * The types of events
 */
enum WindowEventType : u32 {
    WindowEventType_none = 0,
    WindowEventType_quit,
    WindowEventType_close,
    WindowEventType_focusGained,
    WindowEventType_focusLost,
    WindowEventType_resize,
    WindowEventType_maximize,
    WindowEventType_minimize,
    WindowEventType_restore,
    WindowEventType_textInput,
    WindowEventType_buttonPress,
    WindowEventType_buttonRelease,
    WindowEventType_gamepadButtonPress,
    WindowEventType_gamepadButtonRelease,
    WindowEventType_gamepadConnected,
    WindowEventType_gamepadDisconnected,
    WindowEventType_count,
};

/**
 * The button inputs
 */
enum Button : u32 {
    Button_none = 0,
    Button_0,
    Button_1,
    Button_2,
    Button_3,
    Button_4,
    Button_5,
    Button_6,
    Button_7,
    Button_8,
    Button_9,
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
    Button_apostrophe,
    Button_comma,
    Button_period,
    Button_grave,
    Button_lbracket,
    Button_rbracket,
    Button_equal,
    Button_minus,
    Button_slash,
    Button_backslash,
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
    Button_pageup,
    Button_pagedown,
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
    Button_printscreen,
    Button_context,
    Button_numpad0,
    Button_numpad1,
    Button_numpad2,
    Button_numpad3,
    Button_numpad4,
    Button_numpad5,
    Button_numpad6,
    Button_numpad7,
    Button_numpad8,
    Button_numpad9,
    Button_numpaddecimal,
    Button_numpaddiv,
    Button_numpadmul,
    Button_numpadminus,
    Button_numpadplus,
    Button_numpadenter,
    Button_lshift,
    Button_rshift,
    Button_lctrl,
    Button_rctrl,
    Button_lalt,
    Button_ralt,
    Button_lsuper,
    Button_rsuper,
    Button_gamepadSouth,
    Button_gamepadEast,
    Button_gamepadWest,
    Button_gamepadNorth,
    Button_gamepadBack,
    Button_gamepadGuide,
    Button_gamepadStart,
    Button_gamepadLeftStick,
    Button_gamepadRightStick,
    Button_gamepadLeftShoulder,
    Button_gamepadRightShoulder,
    Button_gamepadDpadUp,
    Button_gamepadDpadDown,
    Button_gamepadDpadLeft,
    Button_gamepadDpadRight,
    Button_capslock,
    Button_numlock,
    Button_scrolllock,
    Button_pause,
    Button_count,
};

/**
 * Input event data
 */
struct WindowEvent {
    /**
     * The type of event
     */
    WindowEventType type;
    /**
     * The button that was pressed or released (button events)
     */
    Button button;
    /**
     * UTF-8 text input (textInput)
     */
    char text[32];
    /**
     * Gamepad device index, 0-based (gamepad events only)
     */
    u32 gamepad;
    /**
     * Timestamp in nanoseconds
     */
    u64 timestamp;
};

/**
 * Configuration for a window
 */
struct WindowConfig {
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
 * Window implementation data
 */
struct WindowData;

/**
 * A window
 */
struct Window {
    /**
     * Implementation data
     */
    UniquePtr<WindowData> data;

    /**
     * Construct empty
     */
    Window() noexcept;

    /**
     * Open a new window
     */
    static Window create(const WindowConfig& config = {});

    /**
     * Close the window
     */
    ~Window() noexcept;

    /**
     * Return the window's swapchain
     */
    GpuSwapchain& swapchain();

    /**
     * Returns the window's current image, or nullptr if unavailable this frame
     */
    GpuView* imageView() const;

    /**
     * Returns the window's pixel format
     */
    Format imageFormat() const;

    /**
     * Set the window title
     */
    void setTitle(StringView title);

    /**
     * Get the position
     */
    void pos(i32* x, i32* y) const;

    /**
     * Set the position
     */
    void setPos(i32 x, i32 y);

    /**
     * Get the width and height
     */
    void size(u32* width, u32* height) const;

    /**
     * Set the width and height
     */
    void setSize(u32 width, u32 height);

    /**
     * Returns whether this window is fullscreen
     */
    bool isFullscreen() const;

    /**
     * Set to fullscreen or disable fullscreen
     */
    void setFullscreen(bool set = true);

    /**
     * Set the window to resizeable or not
     */
    void setResizeable(bool set = true);

    /**
     * Returns whether the mouse is focused on the window
     */
    bool isFocused() const;

    /**
     * Returns whether the window was closed
     */
    bool wasClosed() const;

    /**
     * Returns whether the window was resized since last processEvents
     */
    bool wasResized() const;

    /**
     * Returns whether the window is maximized
     */
    bool isMaximized() const;

    /**
     * Returns whether the window is minimized
     */
    bool isMinimized() const;

    /**
     * Maximize the window
     */
    void maximize();

    /**
     * Minimize the window
     */
    void minimize();

    /**
     * Restore the window from minimized or maximized state
     */
    void restore();

    /**
     * Get the current mouse position in screen coordinates
     */
    Vec2 globalMousePos() const;

    /**
     * Get the current mouse position relative to the window height
     */
    Vec2 mousePos() const;

    /**
     * Get the change in mouse position relative to the window height
     */
    Vec2 mouseDelta() const;

    /**
     * Get the mouse wheel movement
     */
    Vec2 wheelDelta() const;

    /**
     * Get whether the key is currently down
     */
    bool isButtonDown(Button key) const;

    /**
     * Get the key events since last event processing
     */
    Span<WindowEvent> events() const;

    /**
     * Move construct
     */
    Window(Window&& other) noexcept;

    /**
     * Move assign
     */
    Window& operator=(Window&& other) noexcept;

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};

/**
 * Returns the number of connected gamepads
 *
 * Note, any combination of indices 0-3 may be active
 */
u32 gamepadCount();

/**
 * Returns whether a gamepad index is active
 */
bool isGamepadActive(u32 gamepad);

/**
 * Get whether a gamepad button is currently down
 */
bool isGamepadButtonDown(u32 gamepad, Button key);

/**
 * Get the left stick position, each axis in range -1..1
 */
Vec2 gamepadLeftStick(u32 gamepad);

/**
 * Get the right stick position, each axis in range -1..1
 */
Vec2 gamepadRightStick(u32 gamepad);

/**
 * Get the left trigger value, in range 0..1
 */
f32 gamepadLeftTrigger(u32 gamepad);

/**
 * Get the right trigger value, in range 0..1
 */
f32 gamepadRightTrigger(u32 gamepad);

} // namespace hg
