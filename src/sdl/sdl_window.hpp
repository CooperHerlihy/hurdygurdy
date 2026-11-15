#pragma once

#include "hg/window.hpp"
#include "hg/hash.hpp"

#include <span>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

namespace hg {

class WindowSdl3;

class InputDeviceSdl3 : public InputDevice {
private:
    std::vector<DisplayInfo> displays{};

    SDL_Cursor* cursors[CursorType_count]{};
    SDL_Cursor* currentCursor = nullptr;

    std::vector<Event> events{};
    bool quit = false;
    bool isKeyDown[Button_count]{};
    bool wasKeyDown[Button_count]{};
    Vec2 mouseDeltaAccum{};
    Vec2 wheelDeltaAccum{};

    HashMap<SDL_WindowID, WindowSdl3*> windows{};
    WindowSdl3* activeWindow = nullptr;

    WindowSdl3* getWindow(SDL_WindowID id);

public:
    static std::unique_ptr<InputDeviceSdl3> create();
    ~InputDeviceSdl3() override;

    virtual InputBackend getBackend() override
    {
        return InputBackend::sdl3;
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

    friend class WindowSdl3;
};

class WindowSdl3 : public Window {
private:
    mutable GpuSwapchain swap{};

    SDL_Window* sdlWindow = nullptr;

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
    explicit WindowSdl3(InputDevice* device) noexcept
        : Window(device)
    {}
    ~WindowSdl3() override;

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

    friend class InputDeviceSdl3;
};

} // namespace hg
