#pragma once

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <xkbcommon/xkbcommon.h>

#include <linux/input.h>
#include <libevdev/libevdev.h>

#define HG_X11_FUNC(name) decltype(&::name) name = nullptr

namespace hg::x11 {

struct XlibFuncs {
    HG_X11_FUNC(XOpenDisplay);
    HG_X11_FUNC(XCloseDisplay);
    HG_X11_FUNC(XCreateWindow);
    HG_X11_FUNC(XDestroyWindow);
    HG_X11_FUNC(XMapWindow);
    HG_X11_FUNC(XUnmapWindow);
    HG_X11_FUNC(XMapRaised);
    HG_X11_FUNC(XWithdrawWindow);
    HG_X11_FUNC(XResizeWindow);
    HG_X11_FUNC(XMoveResizeWindow);
    HG_X11_FUNC(XMoveWindow);
    HG_X11_FUNC(XGetWindowAttributes);
    HG_X11_FUNC(XSelectInput);
    HG_X11_FUNC(XInternAtom);
    HG_X11_FUNC(XSetWMProtocols);
    HG_X11_FUNC(XStoreName);
    HG_X11_FUNC(XFree);
    HG_X11_FUNC(XPending);
    HG_X11_FUNC(XNextEvent);
    HG_X11_FUNC(XSendEvent);
    HG_X11_FUNC(XDefaultScreen);
    HG_X11_FUNC(XRootWindow);
    HG_X11_FUNC(XCreatePixmapCursor);
    HG_X11_FUNC(XCreatePixmap);
    HG_X11_FUNC(XFreePixmap);
    HG_X11_FUNC(XCreateFontCursor);
    HG_X11_FUNC(XFreeCursor);
    HG_X11_FUNC(XDefineCursor);
    HG_X11_FUNC(XUndefineCursor);
    HG_X11_FUNC(XChangeProperty);
    HG_X11_FUNC(XGetWindowProperty);
    HG_X11_FUNC(XSetSelectionOwner);
    using XKeycodeToKeysymFn = KeySym(*)(Display*, KeyCode, int, int);
    XKeycodeToKeysymFn XKeycodeToKeysym = nullptr;
    HG_X11_FUNC(XKeysymToKeycode);
    HG_X11_FUNC(XQueryPointer);
};

struct XrandrFuncs {
    HG_X11_FUNC(XRRGetScreenResourcesCurrent);
    HG_X11_FUNC(XRRFreeScreenResources);
    HG_X11_FUNC(XRRGetCrtcInfo);
    HG_X11_FUNC(XRRFreeCrtcInfo);
};

struct XkbFuncs {
    HG_X11_FUNC(xkb_context_new);
    HG_X11_FUNC(xkb_context_unref);
    HG_X11_FUNC(xkb_keymap_new_from_names);
    HG_X11_FUNC(xkb_keymap_unref);
    HG_X11_FUNC(xkb_state_new);
    HG_X11_FUNC(xkb_state_unref);
    HG_X11_FUNC(xkb_state_key_get_utf8);
    HG_X11_FUNC(xkb_state_update_mask);
};

struct EvdevFuncs {
    HG_X11_FUNC(libevdev_new_from_fd);
    HG_X11_FUNC(libevdev_free);
    HG_X11_FUNC(libevdev_get_name);
    HG_X11_FUNC(libevdev_get_id_vendor);
    HG_X11_FUNC(libevdev_get_id_product);
    HG_X11_FUNC(libevdev_has_event_type);
    HG_X11_FUNC(libevdev_has_event_code);
    HG_X11_FUNC(libevdev_next_event);
};

#undef HG_X11_FUNC

extern XlibFuncs xlibFuncs;
extern XrandrFuncs xrandrFuncs;
extern XkbFuncs xkbFuncs;
extern EvdevFuncs evdevFuncs;

bool loadX11();

bool initX11();
void deinitX11();

} // namespace hg::x11
