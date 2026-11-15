#pragma once

#include "hg/inttypes.hpp"
#include "hg/gpu.hpp"
#include "hg/error.hpp"

#include <span>
#include <string_view>

namespace hg {

/**
 * The supported input backends
 */
enum class InputBackend {
    /**
     * A dummy backend for headless or inputless setups
     */
    none,
    /**
     * Wayland for modern Linux
     */
    wayland,
    /**
     * X11 for legacy Linux
     */
    x11,
    /**
     * Win32 for Windows
     */
    win32,
    /**
     * SDL3 for other platforms
     */
    sdl3,
};

/**
 * The event types
 */
enum EventType : u32 {
    EventType_none = 0,
    EventType_quit,

    EventType_text,

    EventType_keyPress,
    EventType_keyRelease,

    EventType_mouseMoved,
    EventType_wheelMoved,

    EventType_windowClosed,
    EventType_windowFocused,
    EventType_windowUnfocused,
    EventType_windowResized,

    EventType_clipboardUpdate,

    EventType_count,
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
    Button_capslock,
    Button_numlock,
    Button_scrolllock,
    Button_pause,
    Button_mouse1,
    Button_mouse2,
    Button_mouse3,
    Button_mouse4,
    Button_mouse5,
    Button_lmouse = Button_mouse1,
    Button_rmouse = Button_mouse2,
    Button_mmouse = Button_mouse3,
    Button_count,
};

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
 * A mouse event
 */
struct MouseEvent {
    /**
     * The change in mouse position relative to the active window
     */
    Vec2 delta;
    /**
     * The current mouse position relative to the active window
     */
    Vec2 pos;
    /**
     * The current global mouse position
     */
    Vec2 globalPos;
};

/**
 * A mouse wheel event
 */
struct WheelEvent {
    /**
     * The change in wheel position
     */
    Vec2 delta;
};

/**
 * Forward declaration of Window
 */
class Window;

/**
 * A window event
 */
struct WindowEvent {
    /**
     * The affected window's data pointer (as a unique identifier)
     */
    void* window;
    /**
     * The width (windowResized)
     */
    u32 width;
    /**
     * The height (windowResized)
     */
    u32 height;
};

/**
 * An event
 */
struct Event {
    /**
     * The type of event
     */
    EventType type;
    /**
     * The timestamp in nanoseconds
     */
    u64 timestamp;
    /**
     * The particular event data
     */
    union {
        /**
         * UTF-8 text input
         */
        char text[32];
        /**
         * The key button pressed or released
         */
        Button button;
        /**
         * A mouse event
         */
        MouseEvent mouse;
        /**
         * A wheel event
         */
        WheelEvent wheel;
        /**
         * A window event
         */
        WindowEvent window;
    };
};

/**
 * Configuration for a window
 */
struct WindowConfig {
    /**
     * The title of the window
     *
     * Note, may be truncated on some platforms
     *
     * An empty title uses the platform default
     */
    std::string_view title = "";
    /**
     * The width of the window in pixels
     *
     * Note, may be ignored depending on platform
     *
     * 0 lets the platform choose a default size
     */
    u32 width = 0;
    /**
     * The height of the window in pixels
     *
     * Note, may be ignored depending on platform
     *
     * 0 lets the platform choose a default size
     */
    u32 height = 0;
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
 * An input device
 */
class InputDevice {
public:
    /**
     * Hint to create to use a specific backend
     *
     * Note, it is not guarunteed that this backend will be used
     */
    static void hintBackend(InputBackend preferredBackend);

    /**
     * Create a new input device
     *
     * Note, the backend should be checked to make sure it is not the dummy
     */
    static std::unique_ptr<InputDevice> create();

    /**
     * Destroy the input device
     */
    virtual ~InputDevice() = default;

    /**
     * Returns the backend in use
     */
    virtual InputBackend getBackend() = 0;

    /**
     * Returns the Vulkan extensions needed by the platform for windowing
     */
    virtual std::span<std::string_view> getVulkanExtensions() = 0;

    /**
     * Returns the display info
     */
    virtual std::span<DisplayInfo> displayInfo() = 0;

    /**
     * Processes all events since startup or the last call to process events
     */
    virtual void processEvents() = 0;

    /**
     * Returns the events processed by processEvents
     */
    virtual std::span<Event> getEvents() = 0;

    /**
     * Returns whether the application was quit
     */
    virtual bool wasQuit() = 0;

    /**
     * Create a new window
     */
    virtual std::unique_ptr<Window> createWindow(const WindowConfig& config) = 0;

    /**
     * Get whether a button is currently down
     */
    virtual bool isButtonDown(Button button) = 0;

    /**
     * Get whether a button was pressed last frame
     */
    virtual bool wasButtonPressed(Button button) = 0;

    /**
     * Get whether a button was released last frame
     */
    virtual bool wasButtonReleased(Button button) = 0;

    /**
     * Get the current mouse position relative to the active window
     */
    virtual Vec2 mousePos() = 0;

    /**
     * Get the change in mouse position
     *
     * Note, if there is only one window, it will be scaled to that window
     */
    virtual Vec2 mouseDelta() = 0;

    /**
     * Get the mouse wheel movement
     */
    virtual Vec2 wheelDelta() = 0;

    /**
     * Set the current cursor
     */
    virtual void setCursor(CursorType type) = 0;

    /**
     * Show or hide the cursor
     */
    virtual void showCursor(bool show = true) = 0;
};

/**
 * A dummy input device for headless or inputless setups
 */
class InputDeviceDummy : public InputDevice {
public:
    /**
     * Create a new input device
     */
    static std::unique_ptr<InputDeviceDummy> create()
    {
        return std::make_unique<InputDeviceDummy>();
    }

    /**
     * Returns the backend in use
     */
    virtual InputBackend getBackend() override
    {
        return InputBackend::none;
    }

    /**
     * Returns the Vulkan extensions needed by the platform for windowing
     */
    virtual std::span<std::string_view> getVulkanExtensions() override
    {
        return {};
    }

    /**
     * Returns the display info
     */
    virtual std::span<DisplayInfo> displayInfo() override
    {
        return {};
    }

    /**
     * Processes all events since startup or the last call to process events
     */
    virtual void processEvents() override
    {
    }

    /**
     * Returns the events processed by processEvents
     */
    virtual std::span<Event> getEvents() override
    {
        return {};
    }

    /**
     * Returns whether the application was quit
     */
    virtual bool wasQuit() override
    {
        return false;
    }

    /**
     * Create a new window
     */
    virtual std::unique_ptr<Window> createWindow(const WindowConfig&) override
    {
        setError("Dummy input device cannot create windows");
        return nullptr;
    }

    /**
     * Get whether a button is currently down
     */
    virtual bool isButtonDown(Button) override
    {
        return false;
    }

    /**
     * Get whether a button was pressed last frame
     */
    virtual bool wasButtonPressed(Button) override
    {
        return false;
    }

    /**
     * Get whether a button was released last frame
     */
    virtual bool wasButtonReleased(Button) override
    {
        return false;
    }

    /**
     * Get the current mouse position relative to the active window
     */
    virtual Vec2 mousePos() override
    {
        return {};
    }

    /**
     * Get the change in mouse position
     */
    virtual Vec2 mouseDelta() override
    {
        return {};
    }

    /**
     * Get the mouse wheel movement
     */
    virtual Vec2 wheelDelta() override
    {
        return {};
    }

    /**
     * Set the current cursor
     */
    virtual void setCursor(CursorType) override
    {
    }

    /**
     * Show or hide the cursor
     */
    virtual void showCursor(bool) override
    {
    }
};

/**
 * A window
 */
class Window {
protected:
    /**
     * The input device
     */
    InputDevice* device;

    /**
     * Construct with the input device that created the window
     */
    explicit Window(InputDevice* device) noexcept
        : device{device}
    {}

public:
    /**
     * Close the window
     */
    virtual ~Window() noexcept = default;

    /**
     * Return the window's swapchain for rendering
     */
    virtual GpuSwapchain& swapchain() const = 0;

    /**
     * Get the current swapchain image view
     */
    GpuView* renderTarget() const
    {
        return swapchain().renderTarget();
    }

    /**
     * Get the swapchain image format
     */
    Format swapchainFormat() const
    {
        return swapchain().format();
    }

    /**
     * Set the window title
     */
    virtual void setTitle(std::string_view title) = 0;

    /**
     * Get events relevant to this window
     */
    virtual std::span<Event> events() const = 0;

    /**
     * Returns whether the window was closed
     */
    virtual bool wasClosed() const = 0;

    /**
     * Returns whether this is the active window
     */
    virtual bool isFocused() const = 0;

    /**
     * Returns whether the window gained focus last frame
     */
    virtual bool wasFocusGained() const = 0;

    /**
     * Returns whether the window lost focus last frame
     */
    virtual bool wasFocusLost() const = 0;

    /**
     * Returns whether the window was resized this frame
     */
    virtual bool wasResized() const = 0;

    /**
     * Get the width and height
     */
    virtual void size(u32* width, u32* height) const = 0;

    /**
     * Set to fullscreen or disable fullscreen
     */
    virtual void setFullscreen(bool set = true) = 0;

    /**
     * Get the mouse position relative to this window
     */
    virtual Vec2 mousePos() const = 0;

    /**
     * Get the change in mouse position relative to this window
     */
    virtual Vec2 mouseDelta() const = 0;

    /**
     * Get whether a button is currently down
     */
    bool isButtonDown(Button button)
    {
        return device->isButtonDown(button);
    }

    /**
     * Get whether a button was pressed last frame
     */
    bool wasButtonPressed(Button button)
    {
        return device->wasButtonPressed(button);
    }

    /**
     * Get whether a button was released last frame
     */
    bool wasButtonReleased(Button button)
    {
        return device->wasButtonReleased(button);
    }

    /**
     * Get the current mouse position relative to the active window
     */
    Vec2 mousePos()
    {
        return device->mousePos();
    }

    /**
     * Get the change in mouse position
     *
     * Note, if there is only one window, it will be scaled to that window
     */
    Vec2 mouseDelta()
    {
        return device->mouseDelta();
    }

    /**
     * Get the mouse wheel movement
     */
    Vec2 wheelDelta()
    {
        return device->wheelDelta();
    }

    /**
     * Set the current cursor
     */
    void setCursor(CursorType type)
    {
        return device->setCursor(type);
    }

    /**
     * Show or hide the cursor
     */
    void showCursor(bool show = true)
    {
        return device->showCursor(show);
    }
};

} // namespace hg
