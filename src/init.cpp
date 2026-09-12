#include "hg/init.hpp"

#include "internal.hpp"

#include "sdl/sdl_platform.hpp"
#include "hg/error.hpp"
#include "hg/macros.hpp"

#if defined(HG_PLATFORM_LINUX)
#include "x11/x11_platform.hpp"
#include "pipewire/pipewire_platform.hpp"
#endif

namespace hg {

static bool initialized = false;
static u32 initCount = 0;

Maybe<HurdyGurdy> init()
{
    if (initialized)
        return some<HurdyGurdy>();

    if (!internal::initPlatform())
        return {};

    if (!internal::initGpu())
    {
        internal::deinitPlatform();
        return {};
    }

    internal::initRender2D();

    initialized = true;
    return some<HurdyGurdy>();
}

HurdyGurdy::HurdyGurdy() noexcept
{
    ++initCount;
}

HurdyGurdy::HurdyGurdy(const HurdyGurdy&)
{
    ++initCount;
}

HurdyGurdy& HurdyGurdy::operator=(const HurdyGurdy&)
{
    ++initCount;
    return *this;
}

HurdyGurdy::HurdyGurdy(HurdyGurdy&&) noexcept
{
    ++initCount;
}

HurdyGurdy& HurdyGurdy::operator=(HurdyGurdy&&) noexcept
{
    ++initCount;
    return *this;
}

HurdyGurdy::~HurdyGurdy() noexcept
{
    if (--initCount == 0 && initialized)
    {
        internal::deinitRender2D();

        internal::deinitGpu();
        internal::deinitPlatform();

        initialized = false;
    }
}

struct PlatformApi {
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

static void fillX11()
{
    api.getPlatformVulkanExtensions = x11::getPlatformVulkanExtensions;

    api.displayInfo = x11::displayInfo;
    api.setCursor = x11::setCursor;
    api.showCursor = x11::showCursor;
    api.getClipboardText = x11::getClipboardText;
    api.setClipboardText = x11::setClipboardText;
    api.openURL = x11::openURL;
    api.processEvents = x11::processEvents;
    api.getEvents = x11::getEvents;
    api.wasQuit = x11::wasQuit;

    api.isButtonDown = x11::isButtonDown;
    api.wasButtonPressed = x11::wasButtonPressed;
    api.wasButtonReleased = x11::wasButtonReleased;
    api.mousePos = x11::mousePos;
    api.mouseDelta = x11::mouseDelta;
    api.wheelDelta = x11::wheelDelta;

    api.connectedGamepadCount = x11::connectedGamepadCount;
    api.isGamepadConnected = x11::isGamepadConnected;
    api.isGamepadButtonDown = x11::isGamepadButtonDown;
    api.wasGamepadButtonPressed = x11::wasGamepadButtonPressed;
    api.wasGamepadButtonReleased = x11::wasGamepadButtonReleased;
    api.gamepadLeftStick = x11::gamepadLeftStick;
    api.gamepadRightStick = x11::gamepadRightStick;
    api.gamepadLeftTrigger = x11::gamepadLeftTrigger;
    api.gamepadRightTrigger = x11::gamepadRightTrigger;

    api.windowCreate = x11::windowCreate;
    api.windowDestroy = x11::windowDestroy;
    api.windowSwapchain = x11::windowSwapchain;
    api.windowSetTitle = x11::windowSetTitle;
    api.windowGetPos = x11::windowGetPos;
    api.windowSetPos = x11::windowSetPos;
    api.windowGetSize = x11::windowGetSize;
    api.windowSetSize = x11::windowSetSize;
    api.windowIsFullscreen = x11::windowIsFullscreen;
    api.windowSetFullscreen = x11::windowSetFullscreen;
    api.windowSetResizable = x11::windowSetResizable;
    api.windowIsFocused = x11::windowIsFocused;
    api.windowWasClosed = x11::windowWasClosed;
    api.windowWasResized = x11::windowWasResized;
    api.windowWasFocusGained = x11::windowWasFocusGained;
    api.windowWasFocusLost = x11::windowWasFocusLost;
    api.windowWasMoved = x11::windowWasMoved;
    api.windowIsMaximized = x11::windowIsMaximized;
    api.windowWasMaximized = x11::windowWasMaximized;
    api.windowIsMinimized = x11::windowIsMinimized;
    api.windowWasMinimized = x11::windowWasMinimized;
    api.windowMaximize = x11::windowMaximize;
    api.windowMinimize = x11::windowMinimize;
    api.windowWasRestored = x11::windowWasRestored;
    api.windowRestore = x11::windowRestore;
    api.windowIsFullscreen = x11::windowIsFullscreen;
    api.windowWasMadeFullscreen = x11::windowWasMadeFullscreen;
    api.windowSetFullscreen = x11::windowSetFullscreen;
    api.windowEvents = x11::windowEvents;
    api.windowMousePos = x11::windowMousePos;
    api.windowMouseDelta = x11::windowMouseDelta;
}

static void fillPipeWire()
{
    api.setAudioCallback = pipewire::setAudioCallback;
    api.unsetAudioCallback = pipewire::unsetAudioCallback;
}

#endif

enum WindowBackend {
    WindowBackend_none = 0,
    WindowBackend_sdl,
    WindowBackend_x11,
};

enum AudioBackend {
    AudioBackend_none = 0,
    AudioBackend_sdl,
    AudioBackend_pipewire,
};

static WindowBackend windowBackend{};
static AudioBackend audioBackend{};

namespace internal {

bool initPlatform()
{
#if defined(HG_PLATFORM_LINUX)
    if (x11::loadX11())
        windowBackend = WindowBackend_x11;

    if (pipewire::loadPipeWire())
        audioBackend = AudioBackend_pipewire;

    if (windowBackend == WindowBackend_x11)
    {
        fillX11();
        if (!x11::initX11())
            return false;
    }

    if (audioBackend == AudioBackend_pipewire)
    {
        fillPipeWire();
        if (!pipewire::initPipewire())
            return false;
    }

    if (x11::loadX11() && pipewire::loadPipeWire())
    {
        fillX11();
        fillPipeWire();
        if (x11::initX11() && pipewire::initPipewire())
        {
            windowBackend = WindowBackend_x11;
            audioBackend = AudioBackend_pipewire;
            return true;
        }
        pipewire::deinitPipewire();
        x11::deinitX11();
    }
    setError("");
#endif

    if (windowBackend == WindowBackend_none || audioBackend == AudioBackend_none)
    {
        if (!sdl::loadSdl())
        {
            setError("Could not load any platform library");
            return false;
        }
        fillSdl();
        if (!sdl::initSdl())
        {
            setError("Could not load any platform library");
            return false;
        }
    }

    return true;
}

void deinitPlatform()
{
#if defined(HG_PLATFORM_LINUX)
    if (windowBackend == WindowBackend_x11)
    {
        x11::deinitX11();
        windowBackend = WindowBackend_none;
    }

    if (audioBackend == AudioBackend_pipewire)
    {
        pipewire::deinitPipewire();
        audioBackend = AudioBackend_none;
    }
#endif

    if (windowBackend == WindowBackend_sdl || audioBackend == AudioBackend_sdl)
    {
        sdl::deinitSdl();
        windowBackend = WindowBackend_none;
        audioBackend = AudioBackend_none;
    }
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
