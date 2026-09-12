#include "hg/init.hpp"

#include "internal.hpp"
#include "hg/error.hpp"
#include "hg/macros.hpp"

#if defined(HG_PLATFORM_LINUX)
#include "wayland/wayland_platform.hpp"
#include "x11/x11_platform.hpp"
#include "pipewire/pipewire_platform.hpp"
#endif

#include "sdl/sdl_platform.hpp"

namespace hg {

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
    bool (*windowWasResized)(void* data);
    void (*windowGetSize)(void* data, u32* w, u32* h);
    void (*windowMaximize)(void* data);
    void (*windowMinimize)(void* data);
    void (*windowRestore)(void* data);
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
    api.windowGetSize = sdl::windowGetSize;
    api.windowSetFullscreen = sdl::windowSetFullscreen;
    api.windowIsFocused = sdl::windowIsFocused;
    api.windowWasClosed = sdl::windowWasClosed;
    api.windowWasResized = sdl::windowWasResized;
    api.windowWasFocusGained = sdl::windowWasFocusGained;
    api.windowWasFocusLost = sdl::windowWasFocusLost;
    api.windowMaximize = sdl::windowMaximize;
    api.windowMinimize = sdl::windowMinimize;
    api.windowRestore = sdl::windowRestore;
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
    api.windowGetSize = x11::windowGetSize;
    api.windowSetFullscreen = x11::windowSetFullscreen;
    api.windowIsFocused = x11::windowIsFocused;
    api.windowWasClosed = x11::windowWasClosed;
    api.windowWasResized = x11::windowWasResized;
    api.windowWasFocusGained = x11::windowWasFocusGained;
    api.windowWasFocusLost = x11::windowWasFocusLost;
    api.windowMaximize = x11::windowMaximize;
    api.windowMinimize = x11::windowMinimize;
    api.windowRestore = x11::windowRestore;
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

static void fillWayland()
{
    api.getPlatformVulkanExtensions = wayland::getPlatformVulkanExtensions;

    api.displayInfo = wayland::displayInfo;
    api.setCursor = wayland::setCursor;
    api.showCursor = wayland::showCursor;
    api.getClipboardText = wayland::getClipboardText;
    api.setClipboardText = wayland::setClipboardText;
    api.openURL = wayland::openURL;
    api.processEvents = wayland::processEvents;
    api.getEvents = wayland::getEvents;
    api.wasQuit = wayland::wasQuit;

    api.isButtonDown = wayland::isButtonDown;
    api.wasButtonPressed = wayland::wasButtonPressed;
    api.wasButtonReleased = wayland::wasButtonReleased;
    api.mousePos = wayland::mousePos;
    api.mouseDelta = wayland::mouseDelta;
    api.wheelDelta = wayland::wheelDelta;

    api.connectedGamepadCount = wayland::connectedGamepadCount;
    api.isGamepadConnected = wayland::isGamepadConnected;
    api.isGamepadButtonDown = wayland::isGamepadButtonDown;
    api.wasGamepadButtonPressed = wayland::wasGamepadButtonPressed;
    api.wasGamepadButtonReleased = wayland::wasGamepadButtonReleased;
    api.gamepadLeftStick = wayland::gamepadLeftStick;
    api.gamepadRightStick = wayland::gamepadRightStick;
    api.gamepadLeftTrigger = wayland::gamepadLeftTrigger;
    api.gamepadRightTrigger = wayland::gamepadRightTrigger;

    api.windowCreate = wayland::windowCreate;
    api.windowDestroy = wayland::windowDestroy;
    api.windowSwapchain = wayland::windowSwapchain;
    api.windowSetTitle = wayland::windowSetTitle;
    api.windowGetSize = wayland::windowGetSize;
    api.windowSetFullscreen = wayland::windowSetFullscreen;
    api.windowIsFocused = wayland::windowIsFocused;
    api.windowWasClosed = wayland::windowWasClosed;
    api.windowWasResized = wayland::windowWasResized;
    api.windowWasFocusGained = wayland::windowWasFocusGained;
    api.windowWasFocusLost = wayland::windowWasFocusLost;
    api.windowMaximize = wayland::windowMaximize;
    api.windowMinimize = wayland::windowMinimize;
    api.windowRestore = wayland::windowRestore;
    api.windowEvents = wayland::windowEvents;
    api.windowMousePos = wayland::windowMousePos;
    api.windowMouseDelta = wayland::windowMouseDelta;
}

#endif

enum WindowBackend {
    WindowBackend_none = 0,
    WindowBackend_sdl,
    WindowBackend_x11,
    WindowBackend_wayland,
};

enum AudioBackend {
    AudioBackend_none = 0,
    AudioBackend_sdl,
    AudioBackend_pipewire,
};

static WindowBackend windowBackend{};
static AudioBackend audioBackend{};

static bool initPlatform()
{
#if defined(HG_PLATFORM_LINUX)
    if (pipewire::loadPipeWire())
        audioBackend = AudioBackend_pipewire;

    // Try Wayland first
    if (wayland::loadWayland())
    {
        fillWayland();
        fillPipeWire();
        if (wayland::initWayland())
        {
            windowBackend = WindowBackend_wayland;
            if (audioBackend == AudioBackend_pipewire && pipewire::initPipewire())
            {
                return true;
            }
            wayland::deinitWayland();
            windowBackend = WindowBackend_none;
        }
    }

    // Fall back to X11
    if (x11::loadX11())
    {
        fillX11();
        fillPipeWire();
        if (x11::initX11())
        {
            windowBackend = WindowBackend_x11;
            if (audioBackend == AudioBackend_pipewire && pipewire::initPipewire())
            {
                return true;
            }
            x11::deinitX11();
            windowBackend = WindowBackend_none;
        }
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

static void deinitPlatform()
{
#if defined(HG_PLATFORM_LINUX)
    if (windowBackend == WindowBackend_wayland)
    {
        wayland::deinitWayland();
        windowBackend = WindowBackend_none;
    }

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

static bool initialized = false;
static u32 initCount = 0;

Maybe<HurdyGurdy> init()
{
    if (initialized)
        return some<HurdyGurdy>();

    if (!initPlatform())
        goto platformFailed;

    if (!internal::initGpu())
        goto gpuFailed;

    internal::initRender2D();

    initialized = true;
    return some<HurdyGurdy>();

gpuFailed:
    deinitPlatform();
platformFailed:
    return {};
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

        deinitPlatform();

        initialized = false;
    }
}

Span<StringView> internal::getPlatformVulkanExtensions(Arena* arena)
{
    return api.getPlatformVulkanExtensions(arena);
}

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

bool Window::wasResized() const
{
    return api.windowWasResized(data);
}

void Window::size(u32* width, u32* height) const
{
    api.windowGetSize(data, width, height);
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
