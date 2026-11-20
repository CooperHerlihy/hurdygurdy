#include "hg/window.hpp"

#include "hg/utility.hpp"

#if defined(HG_PLATFORM_LINUX)
#include "wayland/wayland_window.hpp"
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
#if defined(HG_PLATFORM_LINUX)
    case InputBackend::wayland:
        return InputDeviceWayland::create();
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

#if defined(HG_PLATFORM_LINUX)
    if (preferredBackend != InputBackend::wayland)
    {
        d = createBackend(InputBackend::wayland);
        if (d != nullptr)
            return d;
    }

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

} // namespace hg
