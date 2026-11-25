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

    SDL_Cursor* cursors[static_cast<u32>(CursorType::count)]{};
    SDL_Cursor* currentCursor = nullptr;

    std::vector<Event> events{};
    bool quit = false;
    bool isKeyDown[static_cast<u32>(Button::count)]{};
    bool wasKeyDown[static_cast<u32>(Button::count)]{};
    Vec2 mouseDeltaAccum{};
    Vec2 wheelDeltaAccum{};

    HashMap<SDL_WindowID, WindowSdl3*> windows{};
    WindowSdl3* activeWindow = nullptr;

    WindowSdl3* getWindow(SDL_WindowID id);

public:
    static std::unique_ptr<InputDeviceSdl3> create();
    ~InputDeviceSdl3() override;

    InputBackend getBackend() override
    {
        return InputBackend::sdl3;
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

    friend class WindowSdl3;
};

class WindowSdl3 : public Window {
private:
    std::unique_ptr<GpuSwapchain> swap{};

    SDL_Window* sdlWindow = nullptr;

    std::vector<Event> eventQueue{};
    Vec2 mouse{};
    u32 winWidth = 0;
    u32 winHeight = 0;
    bool closed = false;
    bool focused = false;
    bool focusGained = false;
    bool focusLost = false;
    bool resized = false;

public:
    explicit WindowSdl3(InputDevice* device) noexcept
        : Window(device)
    {}
    ~WindowSdl3() noexcept override;

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

    friend class InputDeviceSdl3;
};

} // namespace hg
