#include "x11_window.hpp"

#include "internal.hpp"
#include "hg/error.hpp"
#include "hg/dynlib.hpp"
#include "hg/hash.hpp"
#include "hg/utility.hpp"

#include <vector>
#include <string_view>
#include <memory>

#include <math.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <xkbcommon/xkbcommon.h>

#include <linux/input.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>

namespace hg {

#define HG_X11_FUNC(name) decltype(&::name) name = nullptr

struct XlibFuncs {
    HG_X11_FUNC(XOpenDisplay);
    HG_X11_FUNC(XCloseDisplay);
    HG_X11_FUNC(XCreateWindow);
    HG_X11_FUNC(XDestroyWindow);
    HG_X11_FUNC(XMapWindow);
    HG_X11_FUNC(XUnmapWindow);
    HG_X11_FUNC(XMapRaised);
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
    HG_X11_FUNC(xkb_state_key_get_layout);
    HG_X11_FUNC(xkb_keymap_key_get_syms_by_level);
    HG_X11_FUNC(xkb_state_key_get_utf8);
    HG_X11_FUNC(xkb_state_update_mask);
};

#undef HG_X11_FUNC

static Library libX11{};
static XlibFuncs xlibFuncs{};

static Library libXrandr{};
static XrandrFuncs xrandrFuncs{};

static Library libxkb{};
static XkbFuncs xkbFuncs{};

static bool loadXlib()
{
    std::optional<Library> lib = Library::load("libX11.so.6");
    if (!lib.has_value())
    {
        setError("Could not load libX11");
        return false;
    }
    libX11 = std::move(*lib);

#define HG_LOAD_XLIB(name) \
    *(void**)&xlibFuncs.name = libX11.loadSymbol(#name).value_or(nullptr); \
    if (xlibFuncs.name == nullptr) { setError("Could not load " #name); return false; }

    HG_LOAD_XLIB(XOpenDisplay);
    HG_LOAD_XLIB(XCloseDisplay);
    HG_LOAD_XLIB(XCreateWindow);
    HG_LOAD_XLIB(XDestroyWindow);
    HG_LOAD_XLIB(XMapWindow);
    HG_LOAD_XLIB(XUnmapWindow);
    HG_LOAD_XLIB(XMapRaised);
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
    HG_LOAD_XLIB(XKeysymToKeycode);
    HG_LOAD_XLIB(XQueryPointer);

#undef HG_LOAD_XLIB

    return true;
}

static bool loadXrandr()
{
    std::optional<Library> lib = Library::load("libXrandr.so.2");
    if (!lib.has_value())
    {
        setError("Could not load libXrandr");
        return false;
    }
    libXrandr = std::move(*lib);

#define HG_LOAD_XRANDR(name) \
    *(void**)&xrandrFuncs.name = libXrandr.loadSymbol(#name).value_or(nullptr); \
    if (xrandrFuncs.name == nullptr) { setError("Could not load " #name); return false; }

    HG_LOAD_XRANDR(XRRGetScreenResourcesCurrent);
    HG_LOAD_XRANDR(XRRFreeScreenResources);
    HG_LOAD_XRANDR(XRRGetCrtcInfo);
    HG_LOAD_XRANDR(XRRFreeCrtcInfo);

#undef HG_LOAD_XRANDR

    return true;
}

static bool loadXkb()
{
    std::optional<Library> lib = Library::load("libxkbcommon.so.0");
    if (!lib.has_value())
    {
        setError("Could not load libxkbcommon");
        return false;
    }
    libxkb = std::move(*lib);

#define HG_LOAD_XKB(name) \
    *(void**)&xkbFuncs.name = libxkb.loadSymbol(#name).value_or(nullptr); \
    if (xkbFuncs.name == nullptr) { setError("Could not load " #name); return false; }

    HG_LOAD_XKB(xkb_context_new);
    HG_LOAD_XKB(xkb_context_unref);
    HG_LOAD_XKB(xkb_keymap_new_from_names);
    HG_LOAD_XKB(xkb_keymap_unref);
    HG_LOAD_XKB(xkb_state_new);
    HG_LOAD_XKB(xkb_state_unref);
    HG_LOAD_XKB(xkb_state_key_get_layout);
    HG_LOAD_XKB(xkb_keymap_key_get_syms_by_level);
    HG_LOAD_XKB(xkb_state_key_get_utf8);
    HG_LOAD_XKB(xkb_state_update_mask);

#undef HG_LOAD_XKB

    return true;
}

static bool loadX11()
{
    if (!loadXlib())
        return false;
    if (!loadXrandr())
        return false;
    if (!loadXkb())
        return false;

    return true;
}

// Button mapping
static Button xkbKeysymToButton(KeySym keysym)
{
    switch (keysym)
    {
        case XK_Escape: return Button_escape;
        case XK_Return: return Button_enter;
        case XK_Tab: return Button_tab;
        case XK_BackSpace: return Button_backspace;
        case XK_space: return Button_space;

        case XK_Caps_Lock: return Button_capslock;
        case XK_Num_Lock: return Button_numlock;
        case XK_Scroll_Lock: return Button_scrolllock;

        case XK_Control_L: return Button_lctrl;
        case XK_Control_R: return Button_rctrl;
        case XK_Shift_L: return Button_lshift;
        case XK_Shift_R: return Button_rshift;
        case XK_Alt_L: return Button_lalt;
        case XK_Alt_R: return Button_ralt;
        case XK_Super_L: return Button_lsuper;
        case XK_Super_R: return Button_rsuper;

        case XK_Up: return Button_up;
        case XK_Down: return Button_down;
        case XK_Left: return Button_left;
        case XK_Right: return Button_right;

        case XK_F1: return Button_f1;
        case XK_F2: return Button_f2;
        case XK_F3: return Button_f3;
        case XK_F4: return Button_f4;
        case XK_F5: return Button_f5;
        case XK_F6: return Button_f6;
        case XK_F7: return Button_f7;
        case XK_F8: return Button_f8;
        case XK_F9: return Button_f9;
        case XK_F10: return Button_f10;
        case XK_F11: return Button_f11;
        case XK_F12: return Button_f12;

        case XK_a: return Button_a;
        case XK_b: return Button_b;
        case XK_c: return Button_c;
        case XK_d: return Button_d;
        case XK_e: return Button_e;
        case XK_f: return Button_f;
        case XK_g: return Button_g;
        case XK_h: return Button_h;
        case XK_i: return Button_i;
        case XK_j: return Button_j;
        case XK_k: return Button_k;
        case XK_l: return Button_l;
        case XK_m: return Button_m;
        case XK_n: return Button_n;
        case XK_o: return Button_o;
        case XK_p: return Button_p;
        case XK_q: return Button_q;
        case XK_r: return Button_r;
        case XK_s: return Button_s;
        case XK_t: return Button_t;
        case XK_u: return Button_u;
        case XK_v: return Button_v;
        case XK_w: return Button_w;
        case XK_x: return Button_x;
        case XK_y: return Button_y;
        case XK_z: return Button_z;

        case XK_0: return Button_0;
        case XK_1: return Button_1;
        case XK_2: return Button_2;
        case XK_3: return Button_3;
        case XK_4: return Button_4;
        case XK_5: return Button_5;
        case XK_6: return Button_6;
        case XK_7: return Button_7;
        case XK_8: return Button_8;
        case XK_9: return Button_9;

        case XK_minus: return Button_minus;
        case XK_equal: return Button_equal;
        case XK_bracketleft: return Button_lbracket;
        case XK_bracketright: return Button_rbracket;
        case XK_backslash: return Button_backslash;
        case XK_semicolon: return Button_semicolon;
        case XK_apostrophe: return Button_apostrophe;
        case XK_grave: return Button_grave;
        case XK_comma: return Button_comma;
        case XK_period: return Button_period;
        case XK_slash: return Button_slash;

        case XK_Home: return Button_home;
        case XK_End: return Button_end;
        case XK_Page_Up: return Button_pageup;
        case XK_Page_Down: return Button_pagedown;
        case XK_Delete: return Button_kdelete;
        case XK_Insert: return Button_insert;
        case XK_Print: return Button_printscreen;
        case XK_Pause: return Button_pause;
        case XK_Menu: return Button_context;

        case XK_KP_0: return Button_numpad0;
        case XK_KP_1: return Button_numpad1;
        case XK_KP_2: return Button_numpad2;
        case XK_KP_3: return Button_numpad3;
        case XK_KP_4: return Button_numpad4;
        case XK_KP_5: return Button_numpad5;
        case XK_KP_6: return Button_numpad6;
        case XK_KP_7: return Button_numpad7;
        case XK_KP_8: return Button_numpad8;
        case XK_KP_9: return Button_numpad9;
        case XK_KP_Decimal: return Button_numpaddecimal;
        case XK_KP_Divide: return Button_numpaddiv;
        case XK_KP_Multiply: return Button_numpadmul;
        case XK_KP_Subtract: return Button_numpadminus;
        case XK_KP_Add: return Button_numpadplus;
        case XK_KP_Enter: return Button_numpadenter;

        default: return Button_count;
    }
}

static Button xButtonToButton(unsigned int button)
{
    switch (button)
    {
        case Button1: return Button_mouse1;
        case Button2: return Button_mouse2;
        case Button3: return Button_mouse3;
        case 8: return Button_mouse4;
        case 9: return Button_mouse5;
        default: return Button_count;
    }
}

void InputDeviceX11::updateDisplayInfo()
{
    XRRScreenResources* res = xrandrFuncs.XRRGetScreenResourcesCurrent(display, root);
    if (res == nullptr)
        return;

    displays.resize(assertCast<u64>(res->ncrtc));

    for (u32 i = 0; i < assertCast<u32>(res->ncrtc); i++)
    {
        DisplayInfo& info = displays[i];

        XRRCrtcInfo* crtc = xrandrFuncs.XRRGetCrtcInfo(display, res, res->crtcs[i]);
        if (crtc == nullptr)
            continue;

        info.posX = crtc->x;
        info.posY = crtc->y;
        info.sizeW = crtc->width;
        info.sizeH = crtc->height;

        // TODO: Get work area from _NET_WORKAREA
        info.workPosX = info.posX;
        info.workPosY = info.posY;
        info.workSizeW = info.sizeW;
        info.workSizeH = info.sizeH;

        // DPI scale: assume 1.0 for now
        info.dpiScale = 1.0f;

        xrandrFuncs.XRRFreeCrtcInfo(crtc);
    }

    xrandrFuncs.XRRFreeScreenResources(res);
}


} // namespace hg

namespace hg {

std::unique_ptr<InputDeviceX11> InputDeviceX11::create()
{
    if (!loadX11())
        return nullptr;

    std::unique_ptr<InputDeviceX11> device = std::make_unique<InputDeviceX11>();

    device->display = xlibFuncs.XOpenDisplay(nullptr);
    if (device->display == nullptr)
    {
        setError("Could not open X11 display");
        return nullptr;
    }

    device->screen = xlibFuncs.XDefaultScreen(device->display);
    device->root = xlibFuncs.XRootWindow(device->display, device->screen);

    // Create atoms
    device->wmProtocols = xlibFuncs.XInternAtom(device->display,"WM_PROTOCOLS", False);
    device->wmDeleteMessage = xlibFuncs.XInternAtom(device->display,"WM_DELETE_WINDOW", False);
    device->wmStateAtom = xlibFuncs.XInternAtom(device->display,"_NET_WM_STATE", False);
    device->wmStateFullscreen = xlibFuncs.XInternAtom(device->display,"_NET_WM_STATE_FULLSCREEN", False);
    device->clipboardAtom = xlibFuncs.XInternAtom(device->display,"CLIPBOARD", False);
    device->targetsAtom = xlibFuncs.XInternAtom(device->display,"TARGETS", False);
    device->utf8StringAtom = xlibFuncs.XInternAtom(device->display,"UTF8_STRING", False);

    // Initialize XKB
    device->xkbContext = xkbFuncs.xkb_context_new(static_cast<xkb_context_flags>(0));
    if (device->xkbContext == nullptr)
    {
        setError("Could not create XKB context");
        xlibFuncs.XCloseDisplay(device->display);
        return nullptr;
    }

    struct xkb_rule_names names = {
        .rules = "evdev",
        .model = "pc105",
        .layout = "us",
        .variant = "",
        .options = ""
    };

    device->xkbKeymap = xkbFuncs.xkb_keymap_new_from_names(device->xkbContext, &names, static_cast<xkb_keymap_compile_flags>(0));
    if (device->xkbKeymap == nullptr)
    {
        setError("Could not create XKB keymap");
        xkbFuncs.xkb_context_unref(device->xkbContext);
        xlibFuncs.XCloseDisplay(device->display);
        return nullptr;
    }

    device->xkbState = xkbFuncs.xkb_state_new(device->xkbKeymap);
    if (device->xkbState == nullptr)
    {
        setError("Could not create XKB state");
        xkbFuncs.xkb_keymap_unref(device->xkbKeymap);
        xkbFuncs.xkb_context_unref(device->xkbContext);
        xlibFuncs.XCloseDisplay(device->display);
        return nullptr;
    }

    // Create default cursors
    device->cursors[CursorType_arrow] = xlibFuncs.XCreateFontCursor(device->display,assertCast<unsigned int>(XC_left_ptr));
    device->cursors[CursorType_textInput] = xlibFuncs.XCreateFontCursor(device->display,assertCast<unsigned int>(XC_xterm));
    device->cursors[CursorType_resizeAll] = xlibFuncs.XCreateFontCursor(device->display,assertCast<unsigned int>(XC_fleur));
    device->cursors[CursorType_resizeNESW] = xlibFuncs.XCreateFontCursor(device->display,assertCast<unsigned int>(XC_plus));
    device->cursors[CursorType_resizeNS] = xlibFuncs.XCreateFontCursor(device->display,assertCast<unsigned int>(XC_sb_v_double_arrow));
    device->cursors[CursorType_resizeNWSE] = xlibFuncs.XCreateFontCursor(device->display,assertCast<unsigned int>(XC_plus));
    device->cursors[CursorType_resizeEW] = xlibFuncs.XCreateFontCursor(device->display,assertCast<unsigned int>(XC_sb_h_double_arrow));
    device->cursors[CursorType_hand] = xlibFuncs.XCreateFontCursor(device->display,assertCast<unsigned int>(XC_hand2));
    device->cursors[CursorType_wait] = xlibFuncs.XCreateFontCursor(device->display,assertCast<unsigned int>(XC_watch));
    device->cursors[CursorType_progress] = xlibFuncs.XCreateFontCursor(device->display,assertCast<unsigned int>(XC_watch));
    device->cursors[CursorType_notAllowed] = xlibFuncs.XCreateFontCursor(device->display,assertCast<unsigned int>(XC_X_cursor));

    // Enumerate displays
    device->updateDisplayInfo();

    return device;
}

InputDeviceX11::~InputDeviceX11()
{
    if (display == nullptr)
        return;

    for (u32 i = 0; i < CursorType_count; i++)
    {
        if (cursors[i] != 0)
            xlibFuncs.XFreeCursor(display,cursors[i]);
    }
    currentCursor = 0;

    // Cleanup XKB
    if (xkbState != nullptr)
        xkbFuncs.xkb_state_unref(xkbState);
    if (xkbKeymap != nullptr)
        xkbFuncs.xkb_keymap_unref(xkbKeymap);
    if (xkbContext != nullptr)
        xkbFuncs.xkb_context_unref(xkbContext);

    if (display != nullptr)
        xlibFuncs.XCloseDisplay(display);
}

std::span<std::string_view> InputDeviceX11::getVulkanExtensions()
{
    static std::string_view exts[] = {
        "VK_KHR_surface",
        "VK_KHR_xlib_surface",
    };
    return exts;
}

std::span<DisplayInfo> InputDeviceX11::displayInfo()
{
    return displays;
}

void InputDeviceX11::processEvents()
{
    // Copy previous key state
    std::memcpy(wasKeyDown, isKeyDown, sizeof(isKeyDown));

    // Reset deltas
    mouseDeltaAccum = Vec2{};
    wheelDeltaAccum = Vec2{};

    // Reset per-window flags
    windows.forEach([&](::Window, WindowX11* window)
    {
        window->focusGained = false;
        window->focusLost = false;
        window->resized = false;
        window->eventQueue.resize(0);
    });

    // Clear global events
    events.resize(0);

    // Process X11 events
    while (xlibFuncs.XPending(display))
    {
        XEvent event;
        xlibFuncs.XNextEvent(display,&event);

        switch (event.type)
        {
            case KeyPress:
            case KeyRelease:
            {
                bool pressed = (event.type == KeyPress);

                // Update XKB state
                xkbFuncs.xkb_state_update_mask(xkbState,
                    event.xkey.state & ShiftMask ? 1 : 0,
                    event.xkey.state & LockMask ? 1 : 0,
                    0,
                    0, 0, 0);

                // Buttons represent physical keys, so map the level 0
                // (unmodified) keysym of the active layout instead of the
                // modifier-dependent keysym: press/release must resolve to
                // the same button with or without shift.
                Button button = Button_count;
                if (xkbState != nullptr)
                {
                    xkb_layout_index_t layout =
                        xkbFuncs.xkb_state_key_get_layout(xkbState, event.xkey.keycode);
                    const xkb_keysym_t* syms = nullptr;
                    int numSyms = xkbFuncs.xkb_keymap_key_get_syms_by_level(
                        xkbKeymap, event.xkey.keycode, layout, 0, &syms);
                    if (numSyms > 0 && syms != nullptr)
                        button = xkbKeysymToButton(syms[0]);
                }

                if (button < Button_count)
                {
                    isKeyDown[button] = pressed;

                    Event e{};
                    e.type = pressed ? EventType_keyPress : EventType_keyRelease;
                    e.button = button;
                    events.push_back(e);
                }

                // Text input
                if (pressed)
                {
                    char buf[32];
                    int len = xkbFuncs.xkb_state_key_get_utf8(xkbState, event.xkey.keycode, buf, sizeof(buf) - 1);
                    if (len > 0)
                    {
                        buf[len] = '\0';
                        Event e{};
                        e.type = EventType_text;
                        std::memset(e.text, 0, sizeof(e.text));
                        std::memcpy(e.text, buf, assertCast<u64>(len));
                        events.push_back(e);
                    }
                }
                break;
            }

            case ButtonPress:
            case ButtonRelease:
            {
                bool pressed = (event.type == ButtonPress);

                // Mouse wheel (X11 buttons 4-7)
                if (event.xbutton.button >= 4 && event.xbutton.button <= 7)
                {
                    if (pressed)
                    {
                        Vec2 delta{};
                        if (event.xbutton.button == 4) delta.y() = 1.0f;
                        else if (event.xbutton.button == 5) delta.y() = -1.0f;
                        else if (event.xbutton.button == 6) delta.x() = 1.0f;
                        else if (event.xbutton.button == 7) delta.x() = -1.0f;

                        wheelDeltaAccum += delta;

                        Event e{};
                        e.type = EventType_wheelMoved;
                        e.wheel.delta = delta;
                        events.push_back(e);
                    }
                    break;
                }

                Button button = xButtonToButton(event.xbutton.button);

                if (button < Button_count)
                {
                    isKeyDown[button] = pressed;

                    Event e{};
                    e.type = pressed ? EventType_keyPress : EventType_keyRelease;
                    e.button = button;
                    events.push_back(e);
                }

                break;
            }

            case MotionNotify:
            {
                WindowX11** found = windows.get(event.xmotion.window);
                if (found != nullptr)
                {
                    WindowX11* window = *found;
                    Vec2 newPos{assertCast<f32>(event.xmotion.x), assertCast<f32>(event.xmotion.y)};
                    Vec2 delta = newPos - window->mouse;
                    window->mouse = newPos;
                    mouseDeltaAccum += delta;

                    Event e{};
                    e.type = EventType_mouseMoved;
                    e.mouse.delta = delta;
                    e.mouse.pos = newPos;
                    e.mouse.globalPos = Vec2{assertCast<f32>(event.xmotion.x_root), assertCast<f32>(event.xmotion.y_root)};
                    events.push_back(e);
                }
                break;
            }

            case FocusIn:
            case FocusOut:
            {
                WindowX11** found = windows.get(event.xfocus.window);
                if (found != nullptr)
                {
                    WindowX11* window = *found;
                    bool focused = (event.type == FocusIn);
                    if (focused != window->focused)
                    {
                        window->focused = focused;
                        if (focused)
                        {
                            window->focusGained = true;
                            activeWindow = window;
                        }
                        else
                        {
                            window->focusLost = true;
                            if (activeWindow == window)
                                activeWindow = nullptr;
                        }

                        Event e{};
                        e.type = focused ? EventType_windowFocused : EventType_windowUnfocused;
                        events.push_back(e);
                    }
                }
                break;
            }

            case ConfigureNotify:
            {
                WindowX11** found = windows.get(event.xconfigure.window);
                if (found != nullptr)
                {
                    WindowX11* window = *found;
                    u32 newW = assertCast<u32>(event.xconfigure.width);
                    u32 newH = assertCast<u32>(event.xconfigure.height);

                    if (newW != window->width || newH != window->height)
                    {
                        window->resized = true;
                        window->width = newW;
                        window->height = newH;
                    }
                }
                break;
            }

            case ClientMessage:
            {
                if (event.xclient.message_type == wmProtocols &&
                    static_cast<Atom>(event.xclient.data.l[0]) == wmDeleteMessage)
                {
                    quit = true;
                    Event e{};
                    e.type = EventType_quit;
                    events.push_back(e);

                    WindowX11** found = windows.get(event.xclient.window);
                    if (found != nullptr)
                    {
                        (*found)->closed = true;
                        Event e2{};
                        e2.type = EventType_windowClosed;
                        events.push_back(e2);
                    }
                }
                break;
            }

            case SelectionNotify:
            {
                if (event.xselection.selection == clipboardAtom &&
                    event.xselection.property != None)
                {
                    // Get clipboard data
                    Atom actualType;
                    int actualFormat;
                    unsigned long nitems, bytesAfter;
                    unsigned char* data = nullptr;

                    xlibFuncs.XGetWindowProperty(display,
                        event.xselection.requestor,
                        event.xselection.property,
                        0, 1024 * 1024, // 1MB max
                        False,
                        AnyPropertyType,
                        &actualType,
                        &actualFormat,
                        &nitems,
                        &bytesAfter,
                        &data
                    );

                    if (data != nullptr && nitems > 0)
                    {
                        clipboard.resize(nitems);
                        std::memcpy(clipboard.data(), data, nitems);
                        xlibFuncs.XFree(data);
                    }
                }
                break;
            }

            case SelectionRequest:
            {
                if (event.xselectionrequest.selection == clipboardAtom)
                {
                    XEvent reply{};
                    reply.xselection.type = SelectionNotify;
                    reply.xselection.display = event.xselectionrequest.display;
                    reply.xselection.requestor = event.xselectionrequest.requestor;
                    reply.xselection.selection = event.xselectionrequest.selection;
                    reply.xselection.target = event.xselectionrequest.target;
                    reply.xselection.time = event.xselectionrequest.time;

                    if (event.xselectionrequest.target == targetsAtom)
                    {
                        // Send supported targets
                        Atom targets[] = {targetsAtom, utf8StringAtom};
                        xlibFuncs.XChangeProperty(display,
                            event.xselectionrequest.requestor,
                            event.xselectionrequest.property,
                            XA_ATOM,
                            32,
                            PropModeReplace,
                            reinterpret_cast<const unsigned char*>(targets),
                            2
                        );
                        reply.xselection.property = event.xselectionrequest.property;
                    }
                    else if (event.xselectionrequest.target == utf8StringAtom)
                    {
                        // Send clipboard content
                        if (clipboard.size() > 0)
                        {
                            xlibFuncs.XChangeProperty(display,
                                event.xselectionrequest.requestor,
                                event.xselectionrequest.property,
                                utf8StringAtom,
                                8,
                                PropModeReplace,
                                reinterpret_cast<const unsigned char*>(clipboard.data()),
                                assertCast<int>(clipboard.size())
                            );
                            reply.xselection.property = event.xselectionrequest.property;
                        }
                    }

                    xlibFuncs.XSendEvent(
                        event.xselectionrequest.display,
                        event.xselectionrequest.requestor,
                        True,
                        0,
                        &reply
                    );
                }
                break;
            }
        }
    }

    // Forward events to windows
    windows.forEach([&](::Window, WindowX11* window)
    {
        window->eventQueue.clear();
        for (u64 i = 0; i < events.size(); ++i)
            window->eventQueue.push_back(events[i]);
    });
}

std::span<Event> InputDeviceX11::getEvents()
{
    return events;
}

bool InputDeviceX11::wasQuit()
{
    return quit;
}

std::unique_ptr<Window> InputDeviceX11::createWindow(const WindowConfig& config)
{
    u32 width = config.width != 0 ? config.width : 800;
    u32 height = config.height != 0 ? config.height : 600;
    std::string titleStr = config.title.length() != 0 ? std::string{config.title} : "Hurdy Gurdy";
    const char* title = titleStr.c_str();

    std::unique_ptr<WindowX11> window = std::make_unique<WindowX11>(this);

    XWindowAttributes attrs;
    xlibFuncs.XGetWindowAttributes(display,root, &attrs);

    window->x11Window = xlibFuncs.XCreateWindow(
        display,
        root,
        0, 0,
        assertCast<u16>(width), assertCast<u16>(height),
        0,
        CopyFromParent,
        InputOutput,
        nullptr,
        0,
        nullptr
    );

    if (window->x11Window == 0)
    {
        setError("Could not create X11 window");
        return nullptr;
    }

    // Set window title
    xlibFuncs.XStoreName(display,window->x11Window, title);

    // Set WM_DELETE_WINDOW protocol
    xlibFuncs.XSetWMProtocols(display,window->x11Window, &wmDeleteMessage, 1);

    // Select input events
    xlibFuncs.XSelectInput(display,window->x11Window,
        ExposureMask |
        KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask |
        PointerMotionMask |
        FocusChangeMask |
        StructureNotifyMask |
        PropertyChangeMask
    );

    // Create Vulkan surface
    VkXlibSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.dpy = display;
    createInfo.window = window->x11Window;

    using PFN_vkCreateXlibSurfaceKHR = VkResult(*)(VkInstance, const VkXlibSurfaceCreateInfoKHR*, const VkAllocationCallbacks*, VkSurfaceKHR*);
    auto vkCreateXlibSurfaceKHR = reinterpret_cast<PFN_vkCreateXlibSurfaceKHR>(
        internal::getVkInstanceProcAddr("vkCreateXlibSurfaceKHR"));

    VkSurfaceKHR surface;
    VkResult result = vkCreateXlibSurfaceKHR(
        static_cast<VkInstance>(internal::getVkInstance()),
        &createInfo,
        nullptr,
        &surface
    );

    if (result != VK_SUCCESS)
    {
        setError("Could not create Vulkan surface");
        return nullptr;
    }

    u32 winW = 800;
    u32 winH = 600;
    window->swap = GpuSwapchain::create(surface, winW, winH, config.preferredPresentMode, config.imageUsage);

    // Get initial window attributes
    xlibFuncs.XGetWindowAttributes(display,window->x11Window, &attrs);
    window->width = static_cast<u32>(attrs.width);
    window->height = static_cast<u32>(attrs.height);
    window->focused = (attrs.map_state == IsViewable);

    windows.add(window->x11Window, window.get());

    // Map the window
    xlibFuncs.XMapWindow(display,window->x11Window);

    return window;
}

bool InputDeviceX11::isButtonDown(Button button)
{
    return isKeyDown[button];
}

bool InputDeviceX11::wasButtonPressed(Button button)
{
    return isKeyDown[button] && !wasKeyDown[button];
}

bool InputDeviceX11::wasButtonReleased(Button button)
{
    return !isKeyDown[button] && wasKeyDown[button];
}

Vec2 InputDeviceX11::mousePos()
{
    if (activeWindow != nullptr)
        return activeWindow->mouse;

    if (windows.count == 1)
    {
        Vec2 ret{};
        windows.forEach([&](::Window, WindowX11* wd)
        {
            ret = wd->mouse;
        });
        return ret;
    }
    return {};
}

Vec2 InputDeviceX11::mouseDelta()
{
    if (activeWindow != nullptr)
        return mouseDeltaAccum / assertCast<f32>(activeWindow->height);

    if (windows.count == 1)
    {
        Vec2 ret = mouseDeltaAccum;
        windows.forEach([&](::Window, WindowX11* wd)
        {
            ret = ret / assertCast<f32>(wd->height);
        });
        return ret;
    }
    return mouseDeltaAccum;
}

Vec2 InputDeviceX11::wheelDelta()
{
    return wheelDeltaAccum;
}

void InputDeviceX11::setCursor(CursorType type)
{
    if (type < CursorType_count && cursors[type] != 0)
    {
        currentCursor = cursors[type];
        // Apply to all windows
        windows.forEach([&](::Window, WindowX11* window)
        {
            if (currentCursor != 0)
                xlibFuncs.XDefineCursor(display,window->x11Window, currentCursor);
            else
                xlibFuncs.XUndefineCursor(display,window->x11Window);
        });
    }
}

void InputDeviceX11::showCursor(bool show)
{
    if (show)
    {
        windows.forEach([&](::Window, WindowX11* window)
        {
            if (currentCursor != 0)
                xlibFuncs.XDefineCursor(display,window->x11Window, currentCursor);
        });
    }
    else
    {
        windows.forEach([&](::Window, WindowX11* window)
        {
            // Create a 1x1 transparent cursor
            Pixmap pixmap = xlibFuncs.XCreatePixmap(display, root, 1, 1, 1);
            XColor color{};
            Cursor cursor = xlibFuncs.XCreatePixmapCursor(display, pixmap, pixmap, &color, &color, 0, 0);
            xlibFuncs.XDefineCursor(display,window->x11Window, cursor);
            xlibFuncs.XFreeCursor(display,cursor);
            xlibFuncs.XFreePixmap(display, pixmap);
        });
    }
}

WindowX11::~WindowX11()
{
    if (x11Window != 0)
    {
        static_cast<InputDeviceX11*>(device)->windows.remove(x11Window);
        xlibFuncs.XDestroyWindow(static_cast<InputDeviceX11*>(device)->display, x11Window);
    }
}

GpuSwapchain& WindowX11::swapchain() const
{
    return swap;
}

void WindowX11::setTitle(std::string_view title)
{
    char buf[256];
    u64 len = title.length() < sizeof(buf) - 1 ? title.length() : sizeof(buf) - 1;
    std::memcpy(buf, title.data(), len);
    buf[len] = '\0';
    xlibFuncs.XStoreName(static_cast<InputDeviceX11*>(device)->display, x11Window, buf);
}

std::span<Event> WindowX11::events() const
{
    return eventQueue;
}

bool WindowX11::wasClosed() const
{
    return closed;
}

bool WindowX11::isFocused() const
{
    return focused;
}

bool WindowX11::wasFocusGained() const
{
    return focusGained;
}

bool WindowX11::wasFocusLost() const
{
    return focusLost;
}

bool WindowX11::wasResized() const
{
    return resized;
}

void WindowX11::size(u32* outWidth, u32* outHeight) const
{
    if (outWidth != nullptr)
        *outWidth = width;
    if (outHeight != nullptr)
        *outHeight = height;
}

void WindowX11::setFullscreen(bool set)
{
    InputDeviceX11* dev = static_cast<InputDeviceX11*>(device);

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = x11Window;
    event.xclient.message_type = dev->wmStateAtom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = set ? 1 : 0; // _NET_WM_STATE_ADD or _NET_WM_STATE_REMOVE
    event.xclient.data.l[1] = assertCast<long>(dev->wmStateFullscreen);
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 1;

    xlibFuncs.XSendEvent(dev->display, dev->root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

Vec2 WindowX11::mousePos() const
{
    return mouse;
}

Vec2 WindowX11::mouseDelta() const
{
    return static_cast<InputDeviceX11*>(device)->mouseDeltaAccum;
}

} // namespace hg
