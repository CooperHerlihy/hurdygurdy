#pragma once

#include "hg/window.hpp"
#include "hg/hash.hpp"

#include <span>
#include <string_view>
#include <vector>

#include <X11/Xlib.h>

#include <xkbcommon/xkbcommon.h>

namespace hg {

class WindowX11;

class InputDeviceX11 : public InputDevice {
private:
    ::Display* display = nullptr;
    int screen = 0;
    ::Window root = 0;

    Atom wmProtocols = 0;
    Atom wmDeleteMessage = 0;
    Atom wmStateAtom = 0;
    Atom wmStateFullscreen = 0;
    Atom clipboardAtom = 0;
    Atom targetsAtom = 0;
    Atom utf8StringAtom = 0;

    std::vector<DisplayInfo> displays{};

    Cursor cursors[static_cast<u32>(CursorType::count)]{};
    Cursor currentCursor = 0;

    std::vector<Event> events{};
    bool quit = false;
    bool isKeyDown[static_cast<u32>(Button::count)]{};
    bool wasKeyDown[static_cast<u32>(Button::count)]{};
    Vec2 mouseDeltaAccum{};
    Vec2 wheelDeltaAccum{};

    HashMap<::Window, WindowX11*> windows{};
    WindowX11* activeWindow = nullptr;

    std::vector<char> clipboard{};

    struct xkb_context* xkbContext = nullptr;
    struct xkb_keymap* xkbKeymap = nullptr;
    struct xkb_state* xkbState = nullptr;

    void updateDisplayInfo();

public:
    static std::unique_ptr<InputDeviceX11> create();
    ~InputDeviceX11() override;

    InputBackend getBackend() override
    {
        return InputBackend::x11;
    }

    std::span<std::string_view> getPlatformGraphicsExtensions() override;
    std::span<DisplayInfo> displayInfo() override;
    void processEvents() override;
    std::span<const Event> getEvents() override;
    bool wasQuit() override;
    std::unique_ptr<Window> createWindow(Gpu* gpu, const WindowConfig& config) override;
    bool isButtonDown(Button button) override;
    bool wasButtonPressed(Button button) override;
    bool wasButtonReleased(Button button) override;
    Vec2 mousePos() override;
    Vec2 mouseDelta() override;
    Vec2 wheelDelta() override;
    void setCursor(CursorType type) override;
    void showCursor(bool show = true) override;

    friend class WindowX11;
};

class WindowX11 : public Window {
private:
    std::unique_ptr<GpuSwapchain> swap{};

    ::Window x11Window = 0;

    mutable std::vector<Event> eventQueue{};
    Vec2 mouse{};
    u32 winWidth = 0;
    u32 winHeight = 0;
    bool closed = false;
    bool focused = false;
    bool focusGained = false;
    bool focusLost = false;
    bool resized = false;

public:
    explicit WindowX11(InputDevice* device) noexcept
        : Window(device)
    {}
    ~WindowX11() override;

    GpuSwapchain* swapchain() const override;
    void setTitle(std::string_view title) override;
    std::span<const Event> getEvents() const override;
    bool wasClosed() const override;
    bool isFocused() const override;
    bool wasFocusGained() const override;
    bool wasFocusLost() const override;
    bool wasResized() const override;
    u32 width() const override;
    u32 height() const override;
    void setFullscreen(bool set = true) override;
    Vec2 mousePos() const override;
    Vec2 mouseDelta() const override;

    friend class InputDeviceX11;
};

} // namespace hg
