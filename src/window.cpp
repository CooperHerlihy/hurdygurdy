#include "hg/window.hpp"

#include "hg/logging.hpp"

#if defined(HG_USE_WAYLAND)
#include "wayland/wayland_window.hpp"
#endif

#if defined(HG_USE_X11)
#include "x11/x11_window.hpp"
#endif

#if defined(HG_PLATFORM_WINDOWS)
#include "win32/win32_window.hpp"
#endif

#if defined(HG_USE_SDL)
#include "sdl/sdl_window.hpp"
#endif

namespace hg {

#if defined(HG_PLATFORM_LINUX)
static InputBackend preferredBackend = InputBackend::wayland;
#elif defined(HG_PLATFORM_WINDOWS)
static InputBackend preferredBackend = InputBackend::win32;
#else
static InputBackend preferredBackend = InputBackend::sdl3;
#endif

void InputDevice::hintBackend(InputBackend backend)
{
    preferredBackend = backend;
}

static std::unique_ptr<InputDevice> createBackend(InputBackend backend)
{
    switch (backend)
    {
#if defined(HG_USE_WAYLAND)
    case InputBackend::wayland:
        return InputDeviceWayland::create();
#endif
#if defined(HG_USE_X11)
    case InputBackend::x11:
        return InputDeviceX11::create();
#endif
#if defined(HG_PLATFORM_WINDOWS)
    case InputBackend::win32:
        return InputDeviceWin32::create();
#endif
#if defined(HG_USE_SDL)
    case InputBackend::sdl3:
        return InputDeviceSdl3::create();
#endif
    case InputBackend::none:
        return InputDeviceDummy::create();
    default:
        break;
    }

    setError("Could not create input backend: {}", static_cast<u32>(backend));
    return nullptr;
}

std::unique_ptr<InputDevice> InputDevice::create()
{
    std::unique_ptr<InputDevice> d = createBackend(preferredBackend);
    if (d != nullptr)
        return d;

#if defined(HG_USE_WAYLAND)
    if (preferredBackend != InputBackend::wayland)
    {
        d = createBackend(InputBackend::wayland);
        if (d != nullptr)
            return d;
    }
#endif

#if defined(HG_USE_X11)
    if (preferredBackend != InputBackend::x11)
    {
        d = createBackend(InputBackend::x11);
        if (d != nullptr)
            return d;
    }
#endif

#if defined(HG_PLATFORM_WINDOWS)
    if (preferredBackend != InputBackend::win32)
    {
        d = createBackend(InputBackend::win32);
        if (d != nullptr)
            return d;
    }
#endif

#if defined(HG_USE_SDL)
    if (preferredBackend != InputBackend::sdl3)
    {
        d = createBackend(InputBackend::sdl3);
        if (d != nullptr)
            return d;
    }
#endif

    return InputDeviceDummy::create();
}

InputBackend InputDeviceDummy::getBackend()
{
    return InputBackend::none;
}

std::span<std::string_view> InputDeviceDummy::getVulkanExtensions()
{
    return {};
}

std::span<DisplayInfo> InputDeviceDummy::displayInfo()
{
    return {};
}

void InputDeviceDummy::processEvents()
{
}

std::span<const Event> InputDeviceDummy::getEvents()
{
    return {};
}

bool InputDeviceDummy::wasQuit()
{
    return false;
}

std::unique_ptr<Window> InputDeviceDummy::createWindow(Gpu*, const WindowConfig&)
{
    return std::make_unique<WindowDummy>(this);
}

bool InputDeviceDummy::isButtonDown(Button)
{
    return false;
}

bool InputDeviceDummy::wasButtonPressed(Button)
{
    return false;
}

bool InputDeviceDummy::wasButtonReleased(Button)
{
    return false;
}

Vec2 InputDeviceDummy::mousePos()
{
    return {};
}

Vec2 InputDeviceDummy::mouseDelta()
{
    return {};
}

Vec2 InputDeviceDummy::wheelDelta()
{
    return {};
}

void InputDeviceDummy::setCursor(CursorType)
{
}

void InputDeviceDummy::showCursor(bool)
{
}

WindowDummy::WindowDummy(InputDevice* device) noexcept
    : Window{device}
    , swap{std::make_unique<GpuSwapchainDummy>()}
{}

GpuSwapchain* WindowDummy::swapchain() const
{
    return nullptr;
}

void WindowDummy::setTitle(std::string_view)
{
}

std::span<const Event> WindowDummy::getEvents() const
{
    return {};
}

bool WindowDummy::wasClosed() const
{
    return false;
}

bool WindowDummy::isFocused() const
{
    return false;
}

bool WindowDummy::wasFocusGained() const
{
    return false;
}

bool WindowDummy::wasFocusLost() const
{
    return false;
}

bool WindowDummy::wasResized() const
{
    return false;
}

u32 WindowDummy::width() const
{
    return 0;
}

u32 WindowDummy::height() const
{
    return 0;
}

void WindowDummy::setFullscreen(bool)
{
}

Vec2 WindowDummy::mousePos() const
{
    return {};
}

Vec2 WindowDummy::mouseDelta() const
{
    return {};
}

void GpuSwapchainDummy::resize(u32, u32)
{
}

u32 GpuSwapchainDummy::width() const
{
    return 0;
}

u32 GpuSwapchainDummy::height() const
{
    return 0;
}

Format GpuSwapchainDummy::format() const
{
    return Format::undefined;
}

u32 GpuSwapchainDummy::imageCount() const
{
    return 0;
}
bool GpuSwapchainDummy::frameAvailable() const
{
    return false;
}

GpuImage* GpuSwapchainDummy::currentImage() const
{
    return nullptr;
}

GpuView* GpuSwapchainDummy::currentView() const
{
    return nullptr;
}

} // namespace hg
