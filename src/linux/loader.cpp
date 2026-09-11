#include "linux_internal.hpp"

#include "hg/error.hpp"
#include "hg/dynlib.hpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <pipewire/stream.h>
#include <pipewire/thread-loop.h>
#include <pipewire/context.h>
#include <pipewire/core.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/raw.h>

#include <xkbcommon/xkbcommon.h>

#include <linux/input.h>
#include <libevdev/libevdev.h>

namespace hg::linux_backend {

static Library libX11{};
XlibFuncs xlibFuncs{};

static Library libXrandr{};
XrandrFuncs xrandrFuncs{};

static Library libpipewire{};
PipeWireFuncs pwFuncs{};

static Library libxkb{};
XkbFuncs xkbFuncs{};

static Library libevdevLib{};
EvdevFuncs evdevFuncs{};

static bool loadXlib()
{
    Maybe<Library> lib = Library::load("libX11.so.6");
    if (!lib.has)
    {
        setError("Could not load libX11");
        return false;
    }
    libX11 = std::move(*lib);

#define HG_LOAD_XLIB(name) \
    *(void**)&xlibFuncs.name = libX11.findFunction(#name).orElse(nullptr); \
    if (xlibFuncs.name == nullptr) { setError("Could not load " #name); return false; }

    HG_LOAD_XLIB(XOpenDisplay);
    HG_LOAD_XLIB(XCloseDisplay);
    HG_LOAD_XLIB(XCreateWindow);
    HG_LOAD_XLIB(XDestroyWindow);
    HG_LOAD_XLIB(XMapWindow);
    HG_LOAD_XLIB(XUnmapWindow);
    HG_LOAD_XLIB(XMapRaised);
    HG_LOAD_XLIB(XWithdrawWindow);
    HG_LOAD_XLIB(XResizeWindow);
    HG_LOAD_XLIB(XMoveResizeWindow);
    HG_LOAD_XLIB(XMoveWindow);
    HG_LOAD_XLIB(XGetWindowAttributes);
    HG_LOAD_XLIB(XSelectInput);
    HG_LOAD_XLIB(XInternAtom);
    HG_LOAD_XLIB(XSetWMProtocols);
    HG_LOAD_XLIB(XStoreName);
    HG_LOAD_XLIB(XFree);
    HG_LOAD_XLIB(XPending);
    HG_LOAD_XLIB(XNextEvent);
    HG_LOAD_XLIB(XSendEvent);
    HG_LOAD_XLIB(XDefaultScreen);
    HG_LOAD_XLIB(XRootWindow);
    HG_LOAD_XLIB(XCreatePixmapCursor);
    HG_LOAD_XLIB(XCreatePixmap);
    HG_LOAD_XLIB(XFreePixmap);
    HG_LOAD_XLIB(XCreateFontCursor);
    HG_LOAD_XLIB(XFreeCursor);
    HG_LOAD_XLIB(XDefineCursor);
    HG_LOAD_XLIB(XUndefineCursor);
    HG_LOAD_XLIB(XChangeProperty);
    HG_LOAD_XLIB(XGetWindowProperty);
    HG_LOAD_XLIB(XSetSelectionOwner);
    *(void**)&xlibFuncs.XKeycodeToKeysym = libX11.findFunction("XKeycodeToKeysym").orElse(nullptr);
    if (xlibFuncs.XKeycodeToKeysym == nullptr) { setError("Could not load XKeycodeToKeysym"); return false; }
    HG_LOAD_XLIB(XKeysymToKeycode);
    HG_LOAD_XLIB(XQueryPointer);

#undef HG_LOAD_XLIB

    return true;
}

static bool loadXrandr()
{
    Maybe<Library> lib = Library::load("libXrandr.so.2");
    if (!lib.has)
    {
        setError("Could not load libXrandr");
        return false;
    }
    libXrandr = std::move(*lib);

#define HG_LOAD_XRANDR(name) \
    *(void**)&xrandrFuncs.name = libXrandr.findFunction(#name).orElse(nullptr); \
    if (xrandrFuncs.name == nullptr) { setError("Could not load " #name); return false; }

    HG_LOAD_XRANDR(XRRGetScreenResourcesCurrent);
    HG_LOAD_XRANDR(XRRFreeScreenResources);
    HG_LOAD_XRANDR(XRRGetCrtcInfo);
    HG_LOAD_XRANDR(XRRFreeCrtcInfo);

#undef HG_LOAD_XRANDR

    return true;
}

static bool loadPipeWire()
{
    Maybe<Library> lib = Library::load("libpipewire-0.3.so.0");
    if (!lib.has)
    {
        setError("Could not load libpipewire");
        return false;
    }
    libpipewire = std::move(*lib);

#define HG_LOAD_PW(name) \
    *(void**)&pwFuncs.name = libpipewire.findFunction(#name).orElse(nullptr); \
    if (pwFuncs.name == nullptr) { setError("Could not load " #name); return false; }

    HG_LOAD_PW(pw_init);
    HG_LOAD_PW(pw_deinit);
    HG_LOAD_PW(pw_thread_loop_new);
    HG_LOAD_PW(pw_thread_loop_destroy);
    HG_LOAD_PW(pw_thread_loop_start);
    HG_LOAD_PW(pw_thread_loop_stop);
    HG_LOAD_PW(pw_thread_loop_lock);
    HG_LOAD_PW(pw_thread_loop_unlock);
    HG_LOAD_PW(pw_thread_loop_get_loop);
    HG_LOAD_PW(pw_context_new);
    HG_LOAD_PW(pw_context_connect);
    HG_LOAD_PW(pw_context_destroy);
    HG_LOAD_PW(pw_core_disconnect);
    HG_LOAD_PW(pw_stream_new);
    HG_LOAD_PW(pw_stream_destroy);
    HG_LOAD_PW(pw_stream_connect);
    HG_LOAD_PW(pw_stream_disconnect);
    HG_LOAD_PW(pw_stream_dequeue_buffer);
    HG_LOAD_PW(pw_stream_queue_buffer);
    HG_LOAD_PW(pw_stream_add_listener);
    HG_LOAD_PW(pw_stream_update_params);

#undef HG_LOAD_PW

    return true;
}

static bool loadXkb()
{
    Maybe<Library> lib = Library::load("libxkbcommon.so.0");
    if (!lib.has)
    {
        setError("Could not load libxkbcommon");
        return false;
    }
    libxkb = std::move(*lib);

#define HG_LOAD_XKB(name) \
    *(void**)&xkbFuncs.name = libxkb.findFunction(#name).orElse(nullptr); \
    if (xkbFuncs.name == nullptr) { setError("Could not load " #name); return false; }

    HG_LOAD_XKB(xkb_context_new);
    HG_LOAD_XKB(xkb_context_unref);
    HG_LOAD_XKB(xkb_keymap_new_from_names);
    HG_LOAD_XKB(xkb_keymap_unref);
    HG_LOAD_XKB(xkb_state_new);
    HG_LOAD_XKB(xkb_state_unref);
    HG_LOAD_XKB(xkb_state_key_get_utf8);
    HG_LOAD_XKB(xkb_state_update_mask);

#undef HG_LOAD_XKB

    return true;
}

static bool loadEvdev()
{
    Maybe<Library> lib = Library::load("libevdev.so.2");
    if (!lib.has)
    {
        setError("Could not load libevdev");
        return false;
    }
    libevdevLib = std::move(*lib);

#define HG_LOAD_EVDEV(name) \
    *(void**)&evdevFuncs.name = libevdevLib.findFunction(#name).orElse(nullptr); \
    if (evdevFuncs.name == nullptr) { setError("Could not load " #name); return false; }

    HG_LOAD_EVDEV(libevdev_new_from_fd);
    HG_LOAD_EVDEV(libevdev_free);
    HG_LOAD_EVDEV(libevdev_get_name);
    HG_LOAD_EVDEV(libevdev_get_id_vendor);
    HG_LOAD_EVDEV(libevdev_get_id_product);
    HG_LOAD_EVDEV(libevdev_has_event_type);
    HG_LOAD_EVDEV(libevdev_has_event_code);
    HG_LOAD_EVDEV(libevdev_next_event);

#undef HG_LOAD_EVDEV

    return true;
}

bool loadNative()
{
    if (!loadXlib())
        return false;
    if (!loadXrandr())
        return false;
    if (!loadPipeWire())
        return false;
    if (!loadXkb())
        return false;
    if (!loadEvdev())
        return false;

    return true;
}

} // namespace hg::linux_backend
