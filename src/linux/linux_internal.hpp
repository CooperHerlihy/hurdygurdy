#pragma once

#include "hg/inttypes.hpp"

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <pipewire/stream.h>
#include <pipewire/thread-loop.h>
#include <pipewire/context.h>
#include <pipewire/core.h>

extern "C" {
void pw_init(int* argc, char** argv[]);
void pw_deinit(void);
}
#include <xkbcommon/xkbcommon.h>

#include <linux/input.h>
#include <libevdev/libevdev.h>

#define HG_LINUX_FUNC(name) decltype(&::name) name = nullptr

namespace hg::linux_backend {

struct XlibFuncs {
    HG_LINUX_FUNC(XOpenDisplay);
    HG_LINUX_FUNC(XCloseDisplay);
    HG_LINUX_FUNC(XCreateWindow);
    HG_LINUX_FUNC(XDestroyWindow);
    HG_LINUX_FUNC(XMapWindow);
    HG_LINUX_FUNC(XUnmapWindow);
    HG_LINUX_FUNC(XMapRaised);
    HG_LINUX_FUNC(XWithdrawWindow);
    HG_LINUX_FUNC(XResizeWindow);
    HG_LINUX_FUNC(XMoveResizeWindow);
    HG_LINUX_FUNC(XMoveWindow);
    HG_LINUX_FUNC(XGetWindowAttributes);
    HG_LINUX_FUNC(XSelectInput);
    HG_LINUX_FUNC(XInternAtom);
    HG_LINUX_FUNC(XSetWMProtocols);
    HG_LINUX_FUNC(XStoreName);
    HG_LINUX_FUNC(XFree);
    HG_LINUX_FUNC(XPending);
    HG_LINUX_FUNC(XNextEvent);
    HG_LINUX_FUNC(XSendEvent);
    HG_LINUX_FUNC(XDefaultScreen);
    HG_LINUX_FUNC(XRootWindow);
    HG_LINUX_FUNC(XCreatePixmapCursor);
    HG_LINUX_FUNC(XCreatePixmap);
    HG_LINUX_FUNC(XFreePixmap);
    HG_LINUX_FUNC(XCreateFontCursor);
    HG_LINUX_FUNC(XFreeCursor);
    HG_LINUX_FUNC(XDefineCursor);
    HG_LINUX_FUNC(XUndefineCursor);
    HG_LINUX_FUNC(XChangeProperty);
    HG_LINUX_FUNC(XGetWindowProperty);
    HG_LINUX_FUNC(XSetSelectionOwner);
    using XKeycodeToKeysymFn = KeySym(*)(Display*, KeyCode, int, int);
    XKeycodeToKeysymFn XKeycodeToKeysym = nullptr;
    HG_LINUX_FUNC(XKeysymToKeycode);
    HG_LINUX_FUNC(XQueryPointer);
};

struct XrandrFuncs {
    HG_LINUX_FUNC(XRRGetScreenResourcesCurrent);
    HG_LINUX_FUNC(XRRFreeScreenResources);
    HG_LINUX_FUNC(XRRGetCrtcInfo);
    HG_LINUX_FUNC(XRRFreeCrtcInfo);
};

struct PipeWireFuncs {
    HG_LINUX_FUNC(pw_init);
    HG_LINUX_FUNC(pw_deinit);
    HG_LINUX_FUNC(pw_thread_loop_new);
    HG_LINUX_FUNC(pw_thread_loop_destroy);
    HG_LINUX_FUNC(pw_thread_loop_start);
    HG_LINUX_FUNC(pw_thread_loop_stop);
    HG_LINUX_FUNC(pw_thread_loop_lock);
    HG_LINUX_FUNC(pw_thread_loop_unlock);
    HG_LINUX_FUNC(pw_thread_loop_get_loop);
    HG_LINUX_FUNC(pw_context_new);
    HG_LINUX_FUNC(pw_context_connect);
    HG_LINUX_FUNC(pw_context_destroy);
    HG_LINUX_FUNC(pw_core_disconnect);
    HG_LINUX_FUNC(pw_stream_new);
    HG_LINUX_FUNC(pw_stream_destroy);
    HG_LINUX_FUNC(pw_stream_connect);
    HG_LINUX_FUNC(pw_stream_disconnect);
    HG_LINUX_FUNC(pw_stream_dequeue_buffer);
    HG_LINUX_FUNC(pw_stream_queue_buffer);
    HG_LINUX_FUNC(pw_stream_add_listener);
    HG_LINUX_FUNC(pw_stream_update_params);
};

struct XkbFuncs {
    HG_LINUX_FUNC(xkb_context_new);
    HG_LINUX_FUNC(xkb_context_unref);
    HG_LINUX_FUNC(xkb_keymap_new_from_names);
    HG_LINUX_FUNC(xkb_keymap_unref);
    HG_LINUX_FUNC(xkb_state_new);
    HG_LINUX_FUNC(xkb_state_unref);
    HG_LINUX_FUNC(xkb_state_key_get_utf8);
    HG_LINUX_FUNC(xkb_state_update_mask);
};

struct EvdevFuncs {
    HG_LINUX_FUNC(libevdev_new_from_fd);
    HG_LINUX_FUNC(libevdev_free);
    HG_LINUX_FUNC(libevdev_get_name);
    HG_LINUX_FUNC(libevdev_get_id_vendor);
    HG_LINUX_FUNC(libevdev_get_id_product);
    HG_LINUX_FUNC(libevdev_has_event_type);
    HG_LINUX_FUNC(libevdev_has_event_code);
    HG_LINUX_FUNC(libevdev_next_event);
};

#undef HG_LINUX_FUNC

extern XlibFuncs xlibFuncs;
extern XrandrFuncs xrandrFuncs;
extern PipeWireFuncs pwFuncs;
extern XkbFuncs xkbFuncs;
extern EvdevFuncs evdevFuncs;

bool loadNative();

bool windowInit();
void windowDeinit();

bool initAudio();
void deinitAudio();

} // namespace hg::linux_backend
