#include "internal.hpp"

#include "sdl/sdl_platform.hpp"

namespace hg {

struct PlatformApi {
    bool (*initPlatform)();
    void (*deinitPlatform)();
    Span<StringView> (*getPlatformVulkanExtensions)(Arena* arena);

    Span<DisplayInfo> (*displayInfo)();
    void (*setCursor)(CursorType type);
    void (*showCursor)(bool show);
    String (*getClipboardText)();
    void (*setClipboardText)(StringView text);
    void (*openURL)(StringView url);
    void (*processEvents)();
    Span<Event> (*getEvents)();
    bool (*wasQuit)();

    bool (*isButtonDown)(Button key);
    bool (*wasButtonPressed)(Button key);
    bool (*wasButtonReleased)(Button key);
    Vec2 (*globalMousePos)();
    Vec2 (*mousePos)();
    Vec2 (*mouseDelta)();
    Vec2 (*wheelDelta)();

    u32 (*gamepadCount)();
    bool (*isGamepadButtonDown)(u32 gamepad, Button key);
    Vec2 (*gamepadLeftStick)(u32 gamepad);
    Vec2 (*gamepadRightStick)(u32 gamepad);
    f32 (*gamepadLeftTrigger)(u32 gamepad);
    f32 (*gamepadRightTrigger)(u32 gamepad);
    bool (*isGamepadActive)(u32 gamepad);

    void (*setAudioCallback)(AudioCallback callback, void* userData, const AudioConfig& preferredConfig);
    void (*unsetAudioCallback)();

    void* (*windowCreate)(const WindowConfig& config);
    void (*windowDestroy)(void* data);
    GpuSwapchain& (*windowSwapchain)(void* data);
    GpuView* (*windowImageView)(void* data);
    Format (*windowImageFormat)(void* data);
    void (*windowSetTitle)(void* data, StringView title);
    void (*windowGetPos)(void* data, i32* x, i32* y);
    void (*windowSetPos)(void* data, i32 x, i32 y);
    void (*windowGetSize)(void* data, u32* w, u32* h);
    void (*windowSetSize)(void* data, u32 w, u32 h);
    bool (*windowIsFullscreen)(void* data);
    void (*windowSetFullscreen)(void* data, bool set);
    void (*windowSetResizable)(void* data, bool set);
    bool (*windowIsFocused)(void* data);
    bool (*windowWasClosed)(void* data);
    bool (*windowWasResized)(void* data);
    bool (*windowWasFocusGained)(void* data);
    bool (*windowWasFocusLost)(void* data);
    bool (*windowWasMoved)(void* data);
    bool (*windowIsMaximized)(void* data);
    bool (*windowIsMinimized)(void* data);
    void (*windowMaximize)(void* data);
    void (*windowMinimize)(void* data);
    void (*windowRestore)(void* data);
    Vec2 (*windowMousePos)(void* data);
    Span<Event> (*windowEvents)(void* data);
};

static PlatformApi api{};

static void fillSdl()
{
    api.initPlatform = sdl::initPlatform;
    api.deinitPlatform = sdl::deinitPlatform;
    api.getPlatformVulkanExtensions = sdl::getPlatformVulkanExtensions;

    api.displayInfo = sdl::displayInfo;
    api.setCursor = sdl::setCursor;
    api.showCursor = sdl::showCursor;
    api.getClipboardText = sdl::getClipboardText;
    api.setClipboardText = sdl::setClipboardText;
    api.openURL = sdl::openURL;
    api.processEvents = sdl::processEvents;
    api.getEvents = sdl::getEvents;
    api.wasQuit = sdl::wasQuit;

    api.isButtonDown = sdl::isButtonDown;
    api.wasButtonPressed = sdl::wasButtonPressed;
    api.wasButtonReleased = sdl::wasButtonReleased;
    api.globalMousePos = sdl::globalMousePos;
    api.mousePos = sdl::mousePos;
    api.mouseDelta = sdl::mouseDelta;
    api.wheelDelta = sdl::wheelDelta;

    api.gamepadCount = sdl::gamepadCount;
    api.isGamepadButtonDown = sdl::isGamepadButtonDown;
    api.gamepadLeftStick = sdl::gamepadLeftStick;
    api.gamepadRightStick = sdl::gamepadRightStick;
    api.gamepadLeftTrigger = sdl::gamepadLeftTrigger;
    api.gamepadRightTrigger = sdl::gamepadRightTrigger;
    api.isGamepadActive = sdl::isGamepadActive;

    api.setAudioCallback = sdl::setAudioCallback;
    api.unsetAudioCallback = sdl::unsetAudioCallback;

    api.windowCreate = sdl::windowCreate;
    api.windowDestroy = sdl::windowDestroy;
    api.windowSwapchain = sdl::windowSwapchain;
    api.windowImageView = sdl::windowImageView;
    api.windowImageFormat = sdl::windowImageFormat;
    api.windowSetTitle = sdl::windowSetTitle;
    api.windowGetPos = sdl::windowGetPos;
    api.windowSetPos = sdl::windowSetPos;
    api.windowGetSize = sdl::windowGetSize;
    api.windowSetSize = sdl::windowSetSize;
    api.windowIsFullscreen = sdl::windowIsFullscreen;
    api.windowSetFullscreen = sdl::windowSetFullscreen;
    api.windowSetResizable = sdl::windowSetResizable;
    api.windowIsFocused = sdl::windowIsFocused;
    api.windowWasClosed = sdl::windowWasClosed;
    api.windowWasResized = sdl::windowWasResized;
    api.windowWasFocusGained = sdl::windowWasFocusGained;
    api.windowWasFocusLost = sdl::windowWasFocusLost;
    api.windowWasMoved = sdl::windowWasMoved;
    api.windowIsMaximized = sdl::windowIsMaximized;
    api.windowIsMinimized = sdl::windowIsMinimized;
    api.windowMaximize = sdl::windowMaximize;
    api.windowMinimize = sdl::windowMinimize;
    api.windowRestore = sdl::windowRestore;
    api.windowMousePos = sdl::windowMousePos;
    api.windowEvents = sdl::windowEvents;
}

static void selectBackend()
{
    fillSdl();
}

namespace internal {

bool initPlatform()
{
    selectBackend();
    return api.initPlatform();
}

void deinitPlatform()
{
    api.deinitPlatform();
}

Span<StringView> getPlatformVulkanExtensions(Arena* arena)
{
    return api.getPlatformVulkanExtensions(arena);
}

} // namespace internal

Span<DisplayInfo> displayInfo()
{
    return api.displayInfo();
}

void setCursor(CursorType type)
{
    api.setCursor(type);
}

void showCursor(bool show)
{
    api.showCursor(show);
}

String getClipboardText()
{
    return api.getClipboardText();
}

void setClipboardText(StringView text)
{
    api.setClipboardText(text);
}

void openURL(StringView url)
{
    api.openURL(url);
}

void processEvents()
{
    api.processEvents();
}

Span<Event> getEvents()
{
    return api.getEvents();
}

bool wasQuit()
{
    return api.wasQuit();
}

bool isButtonDown(Button button)
{
    return api.isButtonDown(button);
}

bool wasButtonPressed(Button button)
{
    return api.wasButtonPressed(button);
}

bool wasButtonReleased(Button button)
{
    return api.wasButtonReleased(button);
}

Vec2 globalMousePos()
{
    return api.globalMousePos();
}

Vec2 mousePos()
{
    return api.mousePos();
}

Vec2 mouseDelta()
{
    return api.mouseDelta();
}

Vec2 wheelDelta()
{
    return api.wheelDelta();
}

u32 gamepadCount()
{
    return api.gamepadCount();
}

bool isGamepadButtonDown(u32 gamepad, Button key)
{
    return api.isGamepadButtonDown(gamepad, key);
}

Vec2 gamepadLeftStick(u32 gamepad)
{
    return api.gamepadLeftStick(gamepad);
}

Vec2 gamepadRightStick(u32 gamepad)
{
    return api.gamepadRightStick(gamepad);
}

f32 gamepadLeftTrigger(u32 gamepad)
{
    return api.gamepadLeftTrigger(gamepad);
}

f32 gamepadRightTrigger(u32 gamepad)
{
    return api.gamepadRightTrigger(gamepad);
}

bool isGamepadActive(u32 gamepad)
{
    return api.isGamepadActive(gamepad);
}

void setAudioCallback(AudioCallback callback, void* userData, const AudioConfig& preferredConfig)
{
    api.setAudioCallback(callback, userData, preferredConfig);
}

void unsetAudioCallback()
{
    api.unsetAudioCallback();
}

Window::Window() noexcept
    : data{nullptr}
{}

Window::~Window() noexcept
{
    if (data != nullptr)
        api.windowDestroy(data);
}

Window::Window(Window&& other) noexcept
    : data{std::exchange(other.data, nullptr)}
{}

Window& Window::operator=(Window&& other) noexcept
{
    if (this != &other)
    {
        if (data != nullptr)
            api.windowDestroy(data);
        data = std::exchange(other.data, nullptr);
    }
    return *this;
}

Window Window::create(const WindowConfig& config)
{
    Window window{};
    window.data = api.windowCreate(config);
    return window;
}

GpuSwapchain& Window::swapchain() const
{
    return api.windowSwapchain(data);
}

GpuView* Window::imageView() const
{
    return api.windowImageView(data);
}

Format Window::imageFormat() const
{
    return api.windowImageFormat(data);
}

void Window::setTitle(StringView title)
{
    api.windowSetTitle(data, title);
}

Span<Event> Window::events() const
{
    return api.windowEvents(data);
}

Vec2 Window::mousePos() const
{
    return api.windowMousePos(data);
}

bool Window::wasClosed() const
{
    return api.windowWasClosed(data);
}

bool Window::isFocused() const
{
    return api.windowIsFocused(data);
}

bool Window::wasFocusGained() const
{
    return api.windowWasFocusGained(data);
}

bool Window::wasFocusLost() const
{
    return api.windowWasFocusLost(data);
}

bool Window::wasMoved() const
{
    return api.windowWasMoved(data);
}

void Window::pos(i32* x, i32* y) const
{
    api.windowGetPos(data, x, y);
}

void Window::setPos(i32 x, i32 y)
{
    api.windowSetPos(data, x, y);
}

void Window::setResizable(bool set)
{
    api.windowSetResizable(data, set);
}

bool Window::wasResized() const
{
    return api.windowWasResized(data);
}

void Window::size(u32* width, u32* height) const
{
    api.windowGetSize(data, width, height);
}

void Window::setSize(u32 width, u32 height)
{
    api.windowSetSize(data, width, height);
}

bool Window::isMaximized() const
{
    return api.windowIsMaximized(data);
}

bool Window::isMinimized() const
{
    return api.windowIsMinimized(data);
}

void Window::maximize()
{
    api.windowMaximize(data);
}

void Window::minimize()
{
    api.windowMinimize(data);
}

void Window::restore()
{
    api.windowRestore(data);
}

bool Window::isFullscreen() const
{
    return api.windowIsFullscreen(data);
}

void Window::setFullscreen(bool set)
{
    api.windowSetFullscreen(data, set);
}

} // namespace hg
