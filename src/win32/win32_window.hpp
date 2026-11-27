#pragma once

#include "hg/window.hpp"
#include "hg/hash.hpp"

#include <memory>
#include <span>
#include <string_view>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>

namespace hg {

class WindowWin32;

class InputDeviceWin32 : public InputDevice {
private:
    HINSTANCE hInstance = nullptr;
    ATOM windowClass = 0;

    HCURSOR cursors[static_cast<u32>(CursorType::count)]{};
    HCURSOR currentCursor = nullptr;

    std::vector<DisplayInfo> displays{};

    std::vector<Event> events{};
    bool quit = false;
    bool isKeyDown[static_cast<u32>(Button::count)]{};
    bool wasKeyDown[static_cast<u32>(Button::count)]{};
    Vec2 mouseDeltaAccum{};
    Vec2 wheelDeltaAccum{};

    HashMap<u64, WindowWin32*> windows{};
    WindowWin32* activeWindow = nullptr;

    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
    static std::unique_ptr<InputDeviceWin32> create();
    ~InputDeviceWin32() override;

    virtual InputBackend getBackend() override
    {
        return InputBackend::win32;
    }

    virtual std::span<std::string_view> getPlatformGraphicsExtensions() override;
    virtual std::span<DisplayInfo> displayInfo() override;
    virtual void processEvents() override;
    virtual std::span<const Event> getEvents() override;
    virtual bool wasQuit() override;
    virtual std::unique_ptr<Window> createWindow(Gpu* gpu, const WindowConfig& config) override;
    virtual bool isButtonDown(Button button) override;
    virtual bool wasButtonPressed(Button button) override;
    virtual bool wasButtonReleased(Button button) override;
    virtual Vec2 mousePos() override;
    virtual Vec2 mouseDelta() override;
    virtual Vec2 wheelDelta() override;
    virtual void setCursor(CursorType type) override;
    virtual void showCursor(bool show = true) override;

    friend class WindowWin32;
};

class WindowWin32 : public Window {
private:
    mutable std::unique_ptr<GpuSwapchain> swap{};

    HWND hwnd = nullptr;
    WINDOWPLACEMENT savedPlacement{};
    bool fullscreen = false;

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
    explicit WindowWin32(InputDevice* device) noexcept
        : Window(device)
    {}
    ~WindowWin32() override;

    virtual GpuSwapchain* swapchain() const override;
    virtual void setTitle(std::string_view title) override;
    virtual std::span<const Event> getEvents() const override;
    virtual bool wasClosed() const override;
    virtual bool isFocused() const override;
    virtual bool wasFocusGained() const override;
    virtual bool wasFocusLost() const override;
    virtual bool wasResized() const override;
    virtual u32 width() const override;
    virtual u32 height() const override;
    virtual void setFullscreen(bool set = true) override;
    virtual Vec2 mousePos() const override;
    virtual Vec2 mouseDelta() const override;

    friend class InputDeviceWin32;
};

} // namespace hg
