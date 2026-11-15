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

    Cursor cursors[CursorType_count]{};
    Cursor currentCursor = 0;

    std::vector<Event> events{};
    bool quit = false;
    bool isKeyDown[Button_count]{};
    bool wasKeyDown[Button_count]{};
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

    virtual InputBackend getBackend() override
    {
        return InputBackend::x11;
    }

    virtual std::span<std::string_view> getVulkanExtensions() override;
    virtual std::span<DisplayInfo> displayInfo() override;
    virtual void processEvents() override;
    virtual std::span<Event> getEvents() override;
    virtual bool wasQuit() override;
    virtual std::unique_ptr<Window> createWindow(const WindowConfig& config) override;
    virtual bool isButtonDown(Button button) override;
    virtual bool wasButtonPressed(Button button) override;
    virtual bool wasButtonReleased(Button button) override;
    virtual Vec2 mousePos() override;
    virtual Vec2 mouseDelta() override;
    virtual Vec2 wheelDelta() override;
    virtual void setCursor(CursorType type) override;
    virtual void showCursor(bool show = true) override;

    friend class WindowX11;
};

class WindowX11 : public Window {
private:
    mutable GpuSwapchain swap{};

    ::Window x11Window = 0;

    mutable std::vector<Event> eventQueue{};
    Vec2 mouse{};
    u32 width = 0;
    u32 height = 0;
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

    virtual GpuSwapchain& swapchain() const override;
    virtual void setTitle(std::string_view title) override;
    virtual std::span<Event> events() const override;
    virtual bool wasClosed() const override;
    virtual bool isFocused() const override;
    virtual bool wasFocusGained() const override;
    virtual bool wasFocusLost() const override;
    virtual bool wasResized() const override;
    virtual void size(u32* width, u32* height) const override;
    virtual void setFullscreen(bool set = true) override;
    virtual Vec2 mousePos() const override;
    virtual Vec2 mouseDelta() const override;

    friend class InputDeviceX11;
};

} // namespace hg
