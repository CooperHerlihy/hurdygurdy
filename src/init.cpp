#include "hg/init.hpp"

#include "internal.hpp"
#include "hg/window.hpp"
#include "hg/audio.hpp"
#include "hg/error.hpp"
#include "hg/macros.hpp"

#if defined(HG_PLATFORM_LINUX)
#include "wayland/wayland_platform.hpp"
#include "x11/x11_platform.hpp"
#include "pipewire/pipewire_platform.hpp"
#endif

#if defined(HG_PLATFORM_WINDOWS)
#include "win32/win32_platform.hpp"
#endif

#if defined(HG_USE_SDL)
#include "sdl/sdl_platform.hpp"
#endif

namespace hg {

struct PlatformApi {
    Span<StringView> (*getPlatformVulkanExtensions)(Arena* arena);

    Span<DisplayInfo> (*displayInfo)();
    void (*setCursor)(CursorType type);
    void (*showCursor)(bool show);
    StringView (*getClipboardText)();
    void (*setClipboardText)(StringView text);
    void (*openURL)(StringView url);

    void (*processEvents)();
    Span<Event> (*getEvents)();
    bool (*wasQuit)();
    bool (*isButtonDown)(Button key);
    bool (*wasButtonPressed)(Button key);
    bool (*wasButtonReleased)(Button key);
    Vec2 (*mousePos)();
    Vec2 (*mouseDelta)();
    Vec2 (*wheelDelta)();

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
    void (*windowSetFullscreen)(void* data, bool set);
    Vec2 (*windowMousePos)(void* data);
    Vec2 (*windowMouseDelta)(void* data);

    void (*setAudioCallback)(AudioCallback callback, void* userData, const AudioConfig& preferredConfig);
    void (*unsetAudioCallback)();
};

static PlatformApi platform{};

#if defined(HG_USE_SDL)

static void fillSdl()
{
    platform.getPlatformVulkanExtensions = sdl::getPlatformVulkanExtensions;

    platform.displayInfo = sdl::displayInfo;
    platform.setCursor = sdl::setCursor;
    platform.showCursor = sdl::showCursor;
    platform.getClipboardText = sdl::getClipboardText;
    platform.setClipboardText = sdl::setClipboardText;
    platform.openURL = sdl::openURL;

    platform.processEvents = sdl::processEvents;
    platform.getEvents = sdl::getEvents;
    platform.wasQuit = sdl::wasQuit;
    platform.isButtonDown = sdl::isButtonDown;
    platform.wasButtonPressed = sdl::wasButtonPressed;
    platform.wasButtonReleased = sdl::wasButtonReleased;
    platform.mousePos = sdl::mousePos;
    platform.mouseDelta = sdl::mouseDelta;
    platform.wheelDelta = sdl::wheelDelta;

    platform.windowCreate = sdl::windowCreate;
    platform.windowDestroy = sdl::windowDestroy;
    platform.windowSwapchain = sdl::windowSwapchain;
    platform.windowSetTitle = sdl::windowSetTitle;
    platform.windowGetSize = sdl::windowGetSize;
    platform.windowSetFullscreen = sdl::windowSetFullscreen;
    platform.windowIsFocused = sdl::windowIsFocused;
    platform.windowWasClosed = sdl::windowWasClosed;
    platform.windowWasResized = sdl::windowWasResized;
    platform.windowWasFocusGained = sdl::windowWasFocusGained;
    platform.windowWasFocusLost = sdl::windowWasFocusLost;
    platform.windowSetFullscreen = sdl::windowSetFullscreen;
    platform.windowEvents = sdl::windowEvents;
    platform.windowMousePos = sdl::windowMousePos;
    platform.windowMouseDelta = sdl::windowMouseDelta;

    platform.setAudioCallback = sdl::setAudioCallback;
    platform.unsetAudioCallback = sdl::unsetAudioCallback;
}

#endif

#if defined(HG_PLATFORM_LINUX)

static void fillWayland()
{
    platform.getPlatformVulkanExtensions = wayland::getPlatformVulkanExtensions;

    platform.displayInfo = wayland::displayInfo;
    platform.setCursor = wayland::setCursor;
    platform.showCursor = wayland::showCursor;
    platform.getClipboardText = wayland::getClipboardText;
    platform.setClipboardText = wayland::setClipboardText;
    platform.openURL = wayland::openURL;

    platform.processEvents = wayland::processEvents;
    platform.getEvents = wayland::getEvents;
    platform.wasQuit = wayland::wasQuit;
    platform.isButtonDown = wayland::isButtonDown;
    platform.wasButtonPressed = wayland::wasButtonPressed;
    platform.wasButtonReleased = wayland::wasButtonReleased;
    platform.mousePos = wayland::mousePos;
    platform.mouseDelta = wayland::mouseDelta;
    platform.wheelDelta = wayland::wheelDelta;

    platform.windowCreate = wayland::windowCreate;
    platform.windowDestroy = wayland::windowDestroy;
    platform.windowSwapchain = wayland::windowSwapchain;
    platform.windowSetTitle = wayland::windowSetTitle;
    platform.windowGetSize = wayland::windowGetSize;
    platform.windowSetFullscreen = wayland::windowSetFullscreen;
    platform.windowIsFocused = wayland::windowIsFocused;
    platform.windowWasClosed = wayland::windowWasClosed;
    platform.windowWasResized = wayland::windowWasResized;
    platform.windowWasFocusGained = wayland::windowWasFocusGained;
    platform.windowWasFocusLost = wayland::windowWasFocusLost;
    platform.windowEvents = wayland::windowEvents;
    platform.windowMousePos = wayland::windowMousePos;
    platform.windowMouseDelta = wayland::windowMouseDelta;
}

static void fillX11()
{
    platform.getPlatformVulkanExtensions = x11::getPlatformVulkanExtensions;

    platform.displayInfo = x11::displayInfo;
    platform.setCursor = x11::setCursor;
    platform.showCursor = x11::showCursor;
    platform.getClipboardText = x11::getClipboardText;
    platform.setClipboardText = x11::setClipboardText;
    platform.openURL = x11::openURL;

    platform.processEvents = x11::processEvents;
    platform.getEvents = x11::getEvents;
    platform.wasQuit = x11::wasQuit;
    platform.isButtonDown = x11::isButtonDown;
    platform.wasButtonPressed = x11::wasButtonPressed;
    platform.wasButtonReleased = x11::wasButtonReleased;
    platform.mousePos = x11::mousePos;
    platform.mouseDelta = x11::mouseDelta;
    platform.wheelDelta = x11::wheelDelta;

    platform.windowCreate = x11::windowCreate;
    platform.windowDestroy = x11::windowDestroy;
    platform.windowSwapchain = x11::windowSwapchain;
    platform.windowSetTitle = x11::windowSetTitle;
    platform.windowGetSize = x11::windowGetSize;
    platform.windowSetFullscreen = x11::windowSetFullscreen;
    platform.windowIsFocused = x11::windowIsFocused;
    platform.windowWasClosed = x11::windowWasClosed;
    platform.windowWasResized = x11::windowWasResized;
    platform.windowWasFocusGained = x11::windowWasFocusGained;
    platform.windowWasFocusLost = x11::windowWasFocusLost;
    platform.windowSetFullscreen = x11::windowSetFullscreen;
    platform.windowEvents = x11::windowEvents;
    platform.windowMousePos = x11::windowMousePos;
    platform.windowMouseDelta = x11::windowMouseDelta;
}

static void fillPipeWire()
{
    platform.setAudioCallback = pipewire::setAudioCallback;
    platform.unsetAudioCallback = pipewire::unsetAudioCallback;
}

#endif

#if defined(HG_PLATFORM_WINDOWS)

static void fillWin32()
{
    platform.getPlatformVulkanExtensions = win32::getPlatformVulkanExtensions;

    platform.displayInfo = win32::displayInfo;
    platform.setCursor = win32::setCursor;
    platform.showCursor = win32::showCursor;
    platform.getClipboardText = win32::getClipboardText;
    platform.setClipboardText = win32::setClipboardText;
    platform.openURL = win32::openURL;

    platform.processEvents = win32::processEvents;
    platform.getEvents = win32::getEvents;
    platform.wasQuit = win32::wasQuit;
    platform.isButtonDown = win32::isButtonDown;
    platform.wasButtonPressed = win32::wasButtonPressed;
    platform.wasButtonReleased = win32::wasButtonReleased;
    platform.mousePos = win32::mousePos;
    platform.mouseDelta = win32::mouseDelta;
    platform.wheelDelta = win32::wheelDelta;

    platform.windowCreate = win32::windowCreate;
    platform.windowDestroy = win32::windowDestroy;
    platform.windowSwapchain = win32::windowSwapchain;
    platform.windowSetTitle = win32::windowSetTitle;
    platform.windowGetSize = win32::windowGetSize;
    platform.windowSetFullscreen = win32::windowSetFullscreen;
    platform.windowIsFocused = win32::windowIsFocused;
    platform.windowWasClosed = win32::windowWasClosed;
    platform.windowWasResized = win32::windowWasResized;
    platform.windowWasFocusGained = win32::windowWasFocusGained;
    platform.windowWasFocusLost = win32::windowWasFocusLost;
    platform.windowSetFullscreen = win32::windowSetFullscreen;
    platform.windowEvents = win32::windowEvents;
    platform.windowMousePos = win32::windowMousePos;
    platform.windowMouseDelta = win32::windowMouseDelta;

    platform.setAudioCallback = win32::setAudioCallback;
    platform.unsetAudioCallback = win32::unsetAudioCallback;
}

#endif

enum WindowBackend {
    WindowBackend_none = 0,
    WindowBackend_sdl,
    WindowBackend_wayland,
    WindowBackend_x11,
    WindowBackend_win32,
};

enum AudioBackend {
    AudioBackend_none = 0,
    AudioBackend_sdl,
    AudioBackend_pipewire,
    AudioBackend_win32,
};

static WindowBackend windowBackend{};
static AudioBackend audioBackend{};

static bool initPlatform()
{
#if defined(HG_USE_SDL)
    if (windowBackend == WindowBackend_none || audioBackend == AudioBackend_none)
    {
        fillSdl();
        if (sdl::initSdl())
        {
            windowBackend = WindowBackend_sdl;
            audioBackend = AudioBackend_sdl;
        }
    }
#endif

#if defined(HG_PLATFORM_LINUX)
    if (windowBackend == WindowBackend_none && wayland::loadWayland())
    {
        fillWayland();
        if (wayland::initWayland())
            windowBackend = WindowBackend_wayland;
    }

    if (windowBackend == WindowBackend_none && x11::loadX11())
    {
        fillX11();
        if (x11::initX11())
            windowBackend = WindowBackend_x11;
    }

    if (audioBackend == AudioBackend_none && pipewire::loadPipeWire())
    {
        fillPipeWire();
        if (pipewire::initPipewire())
            audioBackend = AudioBackend_pipewire;
    }
#endif

#if defined(HG_PLATFORM_WINDOWS)
    if (win32::loadWin32())
    {
        fillWin32();
        if (win32::initWin32())
        {
            windowBackend = WindowBackend_win32;
            audioBackend = AudioBackend_win32;
        }
    }
#endif

    if (windowBackend == WindowBackend_none || audioBackend == AudioBackend_none)
    {
        setError("Could not load any platform library");
        return false;
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

#if defined(HG_PLATFORM_WINDOWS)
    if (windowBackend == WindowBackend_win32 || audioBackend == AudioBackend_win32)
    {
        win32::deinitWin32();
        windowBackend = WindowBackend_none;
        audioBackend = AudioBackend_none;
    }
#endif

#if defined(HG_USE_SDL)
    if (windowBackend == WindowBackend_sdl || audioBackend == AudioBackend_sdl)
    {
        sdl::deinitSdl();
        windowBackend = WindowBackend_none;
        audioBackend = AudioBackend_none;
    }
#endif
}

static bool initialized = false;
static u32 initCount = 0;

namespace internal {
    void convertPixelFont();
}

Maybe<HurdyGurdy> init()
{
    if (initialized)
        return some<HurdyGurdy>();

    if (!initPlatform())
        goto platformFailed;

    if (!internal::initGpu())
        goto gpuFailed;

    // internal::convertPixelFont();

    internal::initPixelFont();

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
        internal::deinitPixelFont();

        internal::deinitGpu();

        deinitPlatform();

        initialized = false;
    }
}

Span<StringView> internal::getPlatformVulkanExtensions(Arena* arena)
{
    return platform.getPlatformVulkanExtensions(arena);
}

Span<DisplayInfo> displayInfo()
{
    return platform.displayInfo();
}

void setCursor(CursorType type)
{
    platform.setCursor(type);
}

void showCursor(bool show)
{
    platform.showCursor(show);
}

StringView getClipboardText()
{
    return platform.getClipboardText();
}

void setClipboardText(StringView text)
{
    platform.setClipboardText(text);
}

void openURL(StringView url)
{
    platform.openURL(url);
}

void processEvents()
{
    platform.processEvents();
}

Span<Event> getEvents()
{
    return platform.getEvents();
}

bool wasQuit()
{
    return platform.wasQuit();
}

bool isButtonDown(Button button)
{
    return platform.isButtonDown(button);
}

bool wasButtonPressed(Button button)
{
    return platform.wasButtonPressed(button);
}

bool wasButtonReleased(Button button)
{
    return platform.wasButtonReleased(button);
}

Vec2 mousePos()
{
    return platform.mousePos();
}

Vec2 mouseDelta()
{
    return platform.mouseDelta();
}

Vec2 wheelDelta()
{
    return platform.wheelDelta();
}

Window::Window() noexcept
    : data{nullptr}
{}

Window::~Window() noexcept
{
    if (data != nullptr)
        platform.windowDestroy(data);
}

Window::Window(Window&& other) noexcept
    : data{std::exchange(other.data, nullptr)}
{}

Window& Window::operator=(Window&& other) noexcept
{
    if (this != &other)
    {
        if (data != nullptr)
            platform.windowDestroy(data);
        data = std::exchange(other.data, nullptr);
    }
    return *this;
}

Window Window::create(const WindowConfig& config)
{
    return platform.windowCreate(config);
}

GpuSwapchain& Window::swapchain() const
{
    return platform.windowSwapchain(data);
}

void Window::setTitle(StringView title)
{
    platform.windowSetTitle(data, title);
}

Span<Event> Window::events() const
{
    return platform.windowEvents(data);
}

bool Window::wasClosed() const
{
    return platform.windowWasClosed(data);
}

bool Window::isFocused() const
{
    return platform.windowIsFocused(data);
}

bool Window::wasFocusGained() const
{
    return platform.windowWasFocusGained(data);
}

bool Window::wasFocusLost() const
{
    return platform.windowWasFocusLost(data);
}

bool Window::wasResized() const
{
    return platform.windowWasResized(data);
}

void Window::size(u32* width, u32* height) const
{
    platform.windowGetSize(data, width, height);
}

void Window::setFullscreen(bool set)
{
    platform.windowSetFullscreen(data, set);
}

Vec2 Window::mousePos() const
{
    return platform.windowMousePos(data);
}

Vec2 Window::mouseDelta() const
{
    return platform.windowMouseDelta(data);
}

void setAudioCallback(AudioCallback callback, void* userData, const AudioConfig& preferredConfig)
{
    platform.setAudioCallback(callback, userData, preferredConfig);
}

void unsetAudioCallback()
{
    platform.unsetAudioCallback();
}

} // namespace hg
