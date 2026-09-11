#include "internal.hpp"

#include "sdl/sdl_platform.hpp"

#if defined(HG_PLATFORM_LINUX)
#include "linux/linux_platform.hpp"
#endif

namespace hg {

struct PlatformApi {
    bool (*initPlatform)();
    void (*deinitPlatform)();
    Span<StringView> (*getPlatformVulkanExtensions)(Arena* arena);

    Span<DisplayInfo> (*displayInfo)();
    void (*setCursor)(CursorType type);
    void (*showCursor)(bool show);

    void (*processEvents)();
    Span<Event> (*getEvents)();
    bool (*wasQuit)();

    bool (*isButtonDown)(Button key);
    bool (*wasButtonPressed)(Button key);
    bool (*wasButtonReleased)(Button key);
    Vec2 (*mousePos)();
    Vec2 (*mouseDelta)();
    Vec2 (*wheelDelta)();

    u32 (*connectedGamepadCount)();
    bool (*isGamepadConnected)(u32 gamepad);
    bool (*isGamepadButtonDown)(u32 gamepad, Button key);
    bool (*wasGamepadButtonPressed)(u32 gamepad, GamepadButton key);
    bool (*wasGamepadButtonReleased)(u32 gamepad, GamepadButton key);
    Vec2 (*gamepadLeftStick)(u32 gamepad);
    Vec2 (*gamepadRightStick)(u32 gamepad);
    f32 (*gamepadLeftTrigger)(u32 gamepad);
    f32 (*gamepadRightTrigger)(u32 gamepad);

    void (*setAudioCallback)(AudioCallback callback, void* userData, const AudioConfig& preferredConfig);
    void (*unsetAudioCallback)();

    Window (*windowCreate)(const WindowConfig& config);
    void (*windowDestroy)(void* data);
    GpuSwapchain& (*windowSwapchain)(void* data);
    void (*windowSetTitle)(void* data, StringView title);
    Span<Event> (*windowEvents)(void* data);
    bool (*windowWasClosed)(void* data);
    bool (*windowIsFocused)(void* data);
    bool (*windowWasFocusGained)(void* data);
    bool (*windowWasFocusLost)(void* data);
    bool (*windowWasMoved)(void* data);
    void (*windowGetPos)(void* data, i32* x, i32* y);
    void (*windowSetPos)(void* data, i32 x, i32 y);
    void (*windowSetResizable)(void* data, bool set);
    bool (*windowWasResized)(void* data);
    void (*windowGetSize)(void* data, u32* w, u32* h);
    void (*windowSetSize)(void* data, u32 w, u32 h);
    bool (*windowIsMaximized)(void* data);
    bool (*windowWasMaximized)(void* data);
    void (*windowMaximize)(void* data);
    bool (*windowIsMinimized)(void* data);
    bool (*windowWasMinimized)(void* data);
    void (*windowMinimize)(void* data);
    bool (*windowWasRestored)(void* data);
    void (*windowRestore)(void* data);
    bool (*windowIsFullscreen)(void* data);
    bool (*windowWasMadeFullscreen)(void* data);
    void (*windowSetFullscreen)(void* data, bool set);
    Vec2 (*windowMousePos)(void* data);
    Vec2 (*windowMouseDelta)(void* data);

    StringView (*getClipboardText)();
    void (*setClipboardText)(StringView text);
    void (*openURL)(StringView url);
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
    api.mousePos = sdl::mousePos;
    api.mouseDelta = sdl::mouseDelta;
    api.wheelDelta = sdl::wheelDelta;

    api.connectedGamepadCount = sdl::connectedGamepadCount;
    api.isGamepadConnected = sdl::isGamepadConnected;
    api.isGamepadButtonDown = sdl::isGamepadButtonDown;
    api.wasGamepadButtonPressed = sdl::wasGamepadButtonPressed;
    api.wasGamepadButtonReleased = sdl::wasGamepadButtonReleased;
    api.gamepadLeftStick = sdl::gamepadLeftStick;
    api.gamepadRightStick = sdl::gamepadRightStick;
    api.gamepadLeftTrigger = sdl::gamepadLeftTrigger;
    api.gamepadRightTrigger = sdl::gamepadRightTrigger;

    api.setAudioCallback = sdl::setAudioCallback;
    api.unsetAudioCallback = sdl::unsetAudioCallback;

    api.windowCreate = sdl::windowCreate;
    api.windowDestroy = sdl::windowDestroy;
    api.windowSwapchain = sdl::windowSwapchain;
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
    api.windowWasMaximized = sdl::windowWasMaximized;
    api.windowIsMinimized = sdl::windowIsMinimized;
    api.windowWasMinimized = sdl::windowWasMinimized;
    api.windowMaximize = sdl::windowMaximize;
    api.windowMinimize = sdl::windowMinimize;
    api.windowWasRestored = sdl::windowWasRestored;
    api.windowRestore = sdl::windowRestore;
    api.windowIsFullscreen = sdl::windowIsFullscreen;
    api.windowWasMadeFullscreen = sdl::windowwasMadeFullscreen;
    api.windowSetFullscreen = sdl::windowSetFullscreen;
    api.windowEvents = sdl::windowEvents;
    api.windowMousePos = sdl::windowMousePos;
    api.windowMouseDelta = sdl::windowMouseDelta;
}

#if defined(HG_PLATFORM_LINUX)
static void fillLinux()
{
    api.initPlatform = linux_backend::initPlatform;
    api.deinitPlatform = linux_backend::deinitPlatform;
    api.getPlatformVulkanExtensions = linux_backend::getPlatformVulkanExtensions;

    api.displayInfo = linux_backend::displayInfo;
    api.setCursor = linux_backend::setCursor;
    api.showCursor = linux_backend::showCursor;
    api.getClipboardText = linux_backend::getClipboardText;
    api.setClipboardText = linux_backend::setClipboardText;
    api.openURL = linux_backend::openURL;
    api.processEvents = linux_backend::processEvents;
    api.getEvents = linux_backend::getEvents;
    api.wasQuit = linux_backend::wasQuit;

    api.isButtonDown = linux_backend::isButtonDown;
    api.wasButtonPressed = linux_backend::wasButtonPressed;
    api.wasButtonReleased = linux_backend::wasButtonReleased;
    api.mousePos = linux_backend::mousePos;
    api.mouseDelta = linux_backend::mouseDelta;
    api.wheelDelta = linux_backend::wheelDelta;

    api.connectedGamepadCount = linux_backend::connectedGamepadCount;
    api.isGamepadConnected = linux_backend::isGamepadConnected;
    api.isGamepadButtonDown = linux_backend::isGamepadButtonDown;
    api.wasGamepadButtonPressed = linux_backend::wasGamepadButtonPressed;
    api.wasGamepadButtonReleased = linux_backend::wasGamepadButtonReleased;
    api.gamepadLeftStick = linux_backend::gamepadLeftStick;
    api.gamepadRightStick = linux_backend::gamepadRightStick;
    api.gamepadLeftTrigger = linux_backend::gamepadLeftTrigger;
    api.gamepadRightTrigger = linux_backend::gamepadRightTrigger;

    api.setAudioCallback = linux_backend::setAudioCallback;
    api.unsetAudioCallback = linux_backend::unsetAudioCallback;

    api.windowCreate = linux_backend::windowCreate;
    api.windowDestroy = linux_backend::windowDestroy;
    api.windowSwapchain = linux_backend::windowSwapchain;
    api.windowSetTitle = linux_backend::windowSetTitle;
    api.windowGetPos = linux_backend::windowGetPos;
    api.windowSetPos = linux_backend::windowSetPos;
    api.windowGetSize = linux_backend::windowGetSize;
    api.windowSetSize = linux_backend::windowSetSize;
    api.windowIsFullscreen = linux_backend::windowIsFullscreen;
    api.windowSetFullscreen = linux_backend::windowSetFullscreen;
    api.windowSetResizable = linux_backend::windowSetResizable;
    api.windowIsFocused = linux_backend::windowIsFocused;
    api.windowWasClosed = linux_backend::windowWasClosed;
    api.windowWasResized = linux_backend::windowWasResized;
    api.windowWasFocusGained = linux_backend::windowWasFocusGained;
    api.windowWasFocusLost = linux_backend::windowWasFocusLost;
    api.windowWasMoved = linux_backend::windowWasMoved;
    api.windowIsMaximized = linux_backend::windowIsMaximized;
    api.windowWasMaximized = linux_backend::windowWasMaximized;
    api.windowIsMinimized = linux_backend::windowIsMinimized;
    api.windowWasMinimized = linux_backend::windowWasMinimized;
    api.windowMaximize = linux_backend::windowMaximize;
    api.windowMinimize = linux_backend::windowMinimize;
    api.windowWasRestored = linux_backend::windowWasRestored;
    api.windowRestore = linux_backend::windowRestore;
    api.windowIsFullscreen = linux_backend::windowIsFullscreen;
    api.windowWasMadeFullscreen = linux_backend::windowWasMadeFullscreen;
    api.windowSetFullscreen = linux_backend::windowSetFullscreen;
    api.windowEvents = linux_backend::windowEvents;
    api.windowMousePos = linux_backend::windowMousePos;
    api.windowMouseDelta = linux_backend::windowMouseDelta;
}
#endif

static void selectBackend()
{
#if defined(HG_PLATFORM_LINUX)
    // Change this to true for native X11/PipeWire/evdev backend
    bool useNative = true;
    if (useNative)
        fillLinux();
    else
        fillSdl();
    return;
#endif
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

StringView getClipboardText()
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

u32 connectedGamepadCount()
{
    return api.connectedGamepadCount();
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

bool isGamepadConnected(u32 gamepad)
{
    return api.isGamepadConnected(gamepad);
}

bool wasGamepadButtonPressed(u32 gamepad, GamepadButton key)
{
    return api.wasGamepadButtonPressed(gamepad, key);
}

bool wasGamepadButtonReleased(u32 gamepad, GamepadButton key)
{
    return api.wasGamepadButtonReleased(gamepad, key);
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
    return api.windowCreate(config);
}

GpuSwapchain& Window::swapchain() const
{
    return api.windowSwapchain(data);
}

void Window::setTitle(StringView title)
{
    api.windowSetTitle(data, title);
}

Span<Event> Window::events() const
{
    return api.windowEvents(data);
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

bool Window::wasMaximized() const
{
    return api.windowWasMaximized(data);
}

bool Window::isMinimized() const
{
    return api.windowIsMinimized(data);
}

bool Window::wasMinimized() const
{
    return api.windowWasMinimized(data);
}

void Window::maximize()
{
    api.windowMaximize(data);
}

void Window::minimize()
{
    api.windowMinimize(data);
}

bool Window::wasRestored() const
{
    return api.windowWasRestored(data);
}

void Window::restore()
{
    api.windowRestore(data);
}

bool Window::isFullscreen() const
{
    return api.windowIsFullscreen(data);
}

bool Window::wasMadeFullscreen() const
{
    return api.windowWasMadeFullscreen(data);
}

void Window::setFullscreen(bool set)
{
    api.windowSetFullscreen(data, set);
}

Vec2 Window::mousePos() const
{
    return api.windowMousePos(data);
}

Vec2 Window::mouseDelta() const
{
    return api.windowMouseDelta(data);
}

} // namespace hg
