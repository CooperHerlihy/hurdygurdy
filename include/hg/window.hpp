#pragma once

#include "hg/inttypes.hpp"
#include "hg/math.hpp"
#include "hg/gpu.hpp"

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
enum class EventType : u32 {
    none = 0,
    quit,

    text,

    keyPress,
    keyRelease,

    mouseMoved,
    wheelMoved,

    windowClosed,
    windowFocused,
    windowUnfocused,
    windowResized,

    clipboardUpdate,

    count,
};

/**
 * The button inputs
 */
enum class Button : u32 {
    none = 0,
    b0,
    b1,
    b2,
    b3,
    b4,
    b5,
    b6,
    b7,
    b8,
    b9,
    q,
    w,
    e,
    r,
    t,
    y,
    u,
    i,
    o,
    p,
    a,
    s,
    d,
    f,
    g,
    h,
    j,
    k,
    l,
    z,
    x,
    c,
    v,
    b,
    n,
    m,
    semicolon,
    apostrophe,
    comma,
    period,
    grave,
    lbracket,
    rbracket,
    equal,
    minus,
    slash,
    backslash,
    up,
    down,
    left,
    right,
    escape,
    space,
    enter,
    backspace,
    kdelete,
    insert,
    tab,
    home,
    end,
    pageup,
    pagedown,
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,
    printscreen,
    context,
    numpad0,
    numpad1,
    numpad2,
    numpad3,
    numpad4,
    numpad5,
    numpad6,
    numpad7,
    numpad8,
    numpad9,
    numpaddecimal,
    numpaddiv,
    numpadmul,
    numpadminus,
    numpadplus,
    numpadenter,
    lshift,
    rshift,
    lctrl,
    rctrl,
    lalt,
    ralt,
    lsuper,
    rsuper,
    capslock,
    numlock,
    scrolllock,
    pause,
    mouse1,
    mouse2,
    mouse3,
    mouse4,
    mouse5,
    lmouse = mouse1,
    rmouse = mouse2,
    mmouse = mouse3,
    count,
};

/**
 * The types of cursors
 */
enum class CursorType : u32 {
    arrow,
    textInput,
    resizeAll,
    resizeNS,
    resizeEW,
    resizeNESW,
    resizeNWSE,
    hand,
    wait,
    progress,
    notAllowed,
    count,
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
    GpuPresentMode preferredPresentMode = GpuPresentMode::fifo;
    /**
     * How the swapchain images will be used
     */
    GpuImageUsageFlags imageUsage = GpuImageUsage::colorAttachment;
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
    virtual std::span<const Event> getEvents() = 0;

    /**
     * Returns whether the application was quit
     */
    virtual bool wasQuit() = 0;

    /**
     * Create a new window
     */
    virtual std::unique_ptr<Window> createWindow(Gpu* gpu, const WindowConfig& config) = 0;

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
    virtual GpuSwapchain* swapchain() const = 0;

    /**
     * Get the current swapchain image view
     */
    GpuImage* currentImage() const
    {
        return swapchain()->currentImage();
    }

    /**
     * Get the current swapchain image view
     */
    GpuView* currentView() const
    {
        return swapchain()->currentView();
    }

    /**
     * Get the swapchain image format
     */
    Format swapchainFormat() const
    {
        return swapchain()->format();
    }

    /**
     * Set the window title
     */
    virtual void setTitle(std::string_view title) = 0;

    /**
     * Get events relevant to this window
     */
    virtual std::span<const Event> getEvents() const = 0;

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
     * Get the width of the window in pixels
     */
    virtual u32 width() const = 0;

    /**
     * Get the height of the window in pixels
     */
    virtual u32 height() const = 0;

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
    InputBackend getBackend() override;

    /**
     * Returns the Vulkan extensions needed by the platform for windowing
     */
    std::span<std::string_view> getVulkanExtensions() override;

    /**
     * Returns the display info
     */
    std::span<DisplayInfo> displayInfo() override;

    /**
     * Processes all events since startup or the last call to process events
     */
    void processEvents() override;

    /**
     * Returns the events processed by processEvents
     */
    std::span<const Event> getEvents() override;

    /**
     * Returns whether the application was quit
     */
    bool wasQuit() override;

    /**
     * Create a new window
     */
    std::unique_ptr<Window> createWindow(Gpu*, const WindowConfig&) override;

    /**
     * Get whether a button is currently down
     */
    bool isButtonDown(Button) override;

    /**
     * Get whether a button was pressed last frame
     */
    bool wasButtonPressed(Button) override;

    /**
     * Get whether a button was released last frame
     */
    bool wasButtonReleased(Button) override;

    /**
     * Get the current mouse position relative to the active window
     */
    Vec2 mousePos() override;

    /**
     * Get the change in mouse position
     */
    Vec2 mouseDelta() override;

    /**
     * Get the mouse wheel movement
     */
    Vec2 wheelDelta() override;

    /**
     * Set the current cursor
     */
    void setCursor(CursorType) override;

    /**
     * Show or hide the cursor
     */
    void showCursor(bool) override;
};

/**
 * A dummy window for headless or windowless setups
 */
class WindowDummy : public Window {
private:
    /**
     * The dummy swapchain
     */
    std::unique_ptr<GpuSwapchain> swap = nullptr;

public:
    /**
     * Construct empty
     */
    explicit WindowDummy(InputDevice* device) noexcept;

    /**
     * The dummy window does not have a swapchain
     */
    GpuSwapchain* swapchain() const override;

    /**
     * Set the window title
     */
    void setTitle(std::string_view) override;

    /**
     * Get events relevant to this window
     */
    std::span<const Event> getEvents() const override;

    /**
     * Returns whether the window was closed
     */
    bool wasClosed() const override;

    /**
     * Returns whether this is the active window
     */
    bool isFocused() const override;

    /**
     * Returns whether the window gained focus last frame
     */
    bool wasFocusGained() const override;

    /**
     * Returns whether the window lost focus last frame
     */
    bool wasFocusLost() const override;

    /**
     * Returns whether the window was resized this frame
     */
    bool wasResized() const override;

    /**
     * Get the width of the window in pixels
     */
    u32 width() const override;

    /**
     * Get the height of the window in pixels
     */
    u32 height() const override;

    /**
     * Set to fullscreen or disable fullscreen
     */
    void setFullscreen(bool = true) override;

    /**
     * Get the mouse position relative to this window
     */
    Vec2 mousePos() const override;

    /**
     * Get the change in mouse position relative to this window
     */
    Vec2 mouseDelta() const override;
};

/**
 * A dummy swapchain for headless or windowless setups
 */
class GpuSwapchainDummy : public GpuSwapchain {
public:
    /**
     * Recreate the swapchain with a new size
     */
    void resize(u32, u32) override;

    /**
     * Get the current swapchain width
     */
    u32 width() const override;

    /**
     * Get the current swapchain height
     */
    u32 height() const override;

    /**
     * Get the current swapchain format
     */
    Format format() const override;

    /**
     * Get the current swapchain image count
     */
    u32 imageCount() const override;

    /**
     * Get the current swapchain image
     */
    GpuImage* currentImage() const override;

    /**
     * Get the current swapchain image view
     */
    GpuView* currentView() const override;
};

} // namespace hg
