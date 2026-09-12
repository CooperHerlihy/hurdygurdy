#include "x11_platform.hpp"

#include "internal.hpp"
#include "hg/error.hpp"
#include "hg/dynlib.hpp"
#include "hg/array.hpp"
#include "hg/map.hpp"

#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <xkbcommon/xkbcommon.h>

#include <linux/input.h>
#include <libevdev/libevdev.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>

namespace hg::x11 {

#define HG_X11_FUNC(name) decltype(&::name) name = nullptr

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
    HG_X11_FUNC(xkb_state_key_get_one_sym);
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

static Library libX11{};
XlibFuncs xlibFuncs{};

static Library libXrandr{};
XrandrFuncs xrandrFuncs{};

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
    *(void**)&xlibFuncs.name = libX11.loadSymbol(#name).orElse(nullptr); \
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
    *(void**)&xrandrFuncs.name = libXrandr.loadSymbol(#name).orElse(nullptr); \
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
    Maybe<Library> lib = Library::load("libxkbcommon.so.0");
    if (!lib.has)
    {
        setError("Could not load libxkbcommon");
        return false;
    }
    libxkb = std::move(*lib);

#define HG_LOAD_XKB(name) \
    *(void**)&xkbFuncs.name = libxkb.loadSymbol(#name).orElse(nullptr); \
    if (xkbFuncs.name == nullptr) { setError("Could not load " #name); return false; }

    HG_LOAD_XKB(xkb_context_new);
    HG_LOAD_XKB(xkb_context_unref);
    HG_LOAD_XKB(xkb_keymap_new_from_names);
    HG_LOAD_XKB(xkb_keymap_unref);
    HG_LOAD_XKB(xkb_state_new);
    HG_LOAD_XKB(xkb_state_unref);
    HG_LOAD_XKB(xkb_state_key_get_one_sym);
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
    *(void**)&evdevFuncs.name = libevdevLib.loadSymbol(#name).orElse(nullptr); \
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

bool loadX11()
{
    if (!loadXlib())
        return false;
    if (!loadXrandr())
        return false;
    if (!loadXkb())
        return false;
    if (!loadEvdev())
        return false;

    return true;
}

Span<StringView> getPlatformVulkanExtensions(Arena* arena)
{
    Span<StringView> extBuffer{arena->alloc<StringView>(2), 2};
    extBuffer[0] = "VK_KHR_surface";
    extBuffer[1] = "VK_KHR_xlib_surface";
    return extBuffer;
}

struct WindowData {
    GpuSwapchain swap{};

    ::Window x11Window = 0;

    Array<Event> events{};
    Vec2 mouse{};
    u32 width = 0;
    u32 height = 0;
    bool wasClosed = false;
    bool isFocused = false;
    bool wasFocusGained = false;
    bool wasFocusLost = false;
    bool wasResized = false;

    WindowData() noexcept = default;
    ~WindowData() noexcept;

    WindowData(WindowData&& other) noexcept;
    WindowData& operator=(WindowData&& other) noexcept;

    WindowData(const WindowData&) = delete;
    WindowData& operator=(const WindowData&) = delete;
};

static constexpr u32 maxGamepads = 8;

struct GamepadState {
    int fd = -1;
    struct libevdev* dev = nullptr;
    char name[256]{};
    u16 vendorId = 0;
    u16 productId = 0;

    bool isButtonDown[GamepadButton_count]{};
    bool wasButtonDown[GamepadButton_count]{};
    i32 axes[6]{}; // leftx, lefty, rightx, righty, leftz, rightz
};

struct WindowState {
    ::Display* display = nullptr;
    int screen = 0;
    ::Window root = 0;

    Atom wmProtocols = 0;
    Atom wmDeleteMessage = 0;
    Atom wmStateAtom = 0;
    Atom wmStateFullscreen = 0;
    Atom clipboardAtom = 0;
    Atom targetsAtom = 0;
    Atom utf8StringAtom = 0;

    Array<DisplayInfo> displays{};

    Cursor cursors[CursorType_count]{};
    Cursor currentCursor = 0;

    Array<Event> events{};
    bool wasQuit = false;
    bool isKeyDown[Button_count]{};
    bool wasKeyDown[Button_count]{};
    Vec2 mouseDelta{};
    Vec2 wheelDelta{};

    Map<::Window, WindowData*> windows{};
    WindowData* activeWindow = nullptr;

    Array<char> clipboard{};
    ::Window clipboardRequestor = 0;
    Atom clipboardProperty = 0;

    // XKB state
    struct xkb_context* xkbContext = nullptr;
    struct xkb_keymap* xkbKeymap = nullptr;
    struct xkb_state* xkbState = nullptr;

    // Gamepad state
    GamepadState gamepads[maxGamepads]{};
    u32 gamepadCount = 0;
};

static WindowState windowState{};

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

static GamepadButton evdevButtonToGamepadButton(unsigned int code)
{
    switch (code)
    {
        case BTN_SOUTH: return GamepadButton_south;
        case BTN_EAST: return GamepadButton_east;
        case BTN_NORTH: return GamepadButton_north;
        case BTN_WEST: return GamepadButton_west;
        case BTN_TL: return GamepadButton_leftShoulder;
        case BTN_TR: return GamepadButton_rightShoulder;
        case BTN_SELECT: return GamepadButton_back;
        case BTN_START: return GamepadButton_start;
        case BTN_MODE: return GamepadButton_guide;
        case BTN_THUMBL: return GamepadButton_leftStick;
        case BTN_THUMBR: return GamepadButton_rightStick;
        default: return GamepadButton_count;
    }
}

static void updateDisplayInfo()
{
    XRRScreenResources* res = xrandrFuncs.XRRGetScreenResourcesCurrent(windowState.display, windowState.root);
    if (res == nullptr)
        return;

    windowState.displays.resize(static_cast<u64>(res->ncrtc));

    for (u32 i = 0; i < static_cast<u32>(res->ncrtc); i++)
    {
        DisplayInfo& info = windowState.displays[i];

        XRRCrtcInfo* crtc = xrandrFuncs.XRRGetCrtcInfo(windowState.display, res, res->crtcs[i]);
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

static void scanForGamepads()
{
    windowState.gamepadCount = 0;

    DIR* dir = opendir("/dev/input");
    if (dir == nullptr)
        return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (windowState.gamepadCount >= maxGamepads)
            break;

        if (strncmp(entry->d_name, "event", 5) != 0)
            continue;

        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);

        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0)
            continue;

        struct libevdev* dev = nullptr;
        int rc = evdevFuncs.libevdev_new_from_fd(fd, &dev);
        if (rc < 0 || dev == nullptr)
        {
            close(fd);
            continue;
        }

        // Check if this is a gamepad (has BTN_SOUTH and ABS_X)
        if (!evdevFuncs.libevdev_has_event_type(dev, EV_KEY) || !evdevFuncs.libevdev_has_event_code(dev, EV_KEY, BTN_SOUTH) ||
            !evdevFuncs.libevdev_has_event_type(dev, EV_ABS) || !evdevFuncs.libevdev_has_event_code(dev, EV_ABS, ABS_X))
        {
            evdevFuncs.libevdev_free(dev);
            close(fd);
            continue;
        }

        GamepadState& gamepad = windowState.gamepads[windowState.gamepadCount];
        gamepad.fd = fd;
        gamepad.dev = dev;
        strncpy(gamepad.name, evdevFuncs.libevdev_get_name(dev), sizeof(gamepad.name) - 1);
    gamepad.vendorId = static_cast<u16>(evdevFuncs.libevdev_get_id_vendor(dev));
    gamepad.productId = static_cast<u16>(evdevFuncs.libevdev_get_id_product(dev));

        windowState.gamepadCount++;
    }

    closedir(dir);
}

static void closeGamepads()
{
    for (u32 i = 0; i < windowState.gamepadCount; i++)
    {
        GamepadState& gamepad = windowState.gamepads[i];
        if (gamepad.dev != nullptr)
        {
            evdevFuncs.libevdev_free(gamepad.dev);
            gamepad.dev = nullptr;
        }
        if (gamepad.fd >= 0)
        {
            close(gamepad.fd);
            gamepad.fd = -1;
        }
    }
    windowState.gamepadCount = 0;
}

static void readGamepadEvents()
{
    for (u32 i = 0; i < windowState.gamepadCount; i++)
    {
        GamepadState& gamepad = windowState.gamepads[i];
        if (gamepad.fd < 0)
            continue;

        // Copy previous state
        memcpy(gamepad.wasButtonDown, gamepad.isButtonDown, sizeof(gamepad.isButtonDown));

        struct input_event ev;
        int rc;
        while ((rc = evdevFuncs.libevdev_next_event(gamepad.dev, LIBEVDEV_READ_FLAG_NORMAL, &ev)) == LIBEVDEV_READ_STATUS_SUCCESS)
        {
            if (ev.type == EV_KEY)
            {
                GamepadButton button = evdevButtonToGamepadButton(ev.code);
                if (button < GamepadButton_count)
                    gamepad.isButtonDown[button] = (ev.value != 0);
            }
            else if (ev.type == EV_ABS)
            {
                switch (ev.code)
                {
                    case ABS_X: gamepad.axes[0] = ev.value; break;
                    case ABS_Y: gamepad.axes[1] = ev.value; break;
                    case ABS_RX: gamepad.axes[2] = ev.value; break;
                    case ABS_RY: gamepad.axes[3] = ev.value; break;
                    case ABS_Z: gamepad.axes[4] = ev.value; break;
                    case ABS_RZ: gamepad.axes[5] = ev.value; break;
                }
            }
        }
    }
}

static void pollGamepadDevices()
{
    // Simple polling: check if any new devices appeared
    // In a production system, we'd use inotify or uevent, but polling is simpler
    static u32 lastPollFrame = 0;
    static u32 pollCounter = 0;
    ++pollCounter;
    if (pollCounter - lastPollFrame < 60) // Poll every ~1 second at 60fps
        return;
    lastPollFrame = pollCounter;

    // Check if existing gamepads are still valid
    for (u32 i = 0; i < windowState.gamepadCount;)
    {
        GamepadState& gamepad = windowState.gamepads[i];
        struct stat st;
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);

        if (stat(path, &st) != 0)
        {
            // Device removed
            if (gamepad.dev != nullptr)
                evdevFuncs.libevdev_free(gamepad.dev);
            if (gamepad.fd >= 0)
                close(gamepad.fd);

            // Move remaining gamepads down
            for (u32 j = i; j < windowState.gamepadCount - 1; j++)
                windowState.gamepads[j] = windowState.gamepads[j + 1];
            windowState.gamepadCount--;
        }
        else
        {
            i++;
        }
    }
}

bool initX11()
{
    windowState = WindowState{};

    windowState.display = xlibFuncs.XOpenDisplay(nullptr);
    if (windowState.display == nullptr)
    {
        setError("Could not open X11 display");
        return false;
    }

    windowState.screen = windowState.screen;
    windowState.root = xlibFuncs.XRootWindow(windowState.display, windowState.screen);

    // Create atoms
    windowState.wmProtocols = xlibFuncs.XInternAtom(windowState.display,"WM_PROTOCOLS", False);
    windowState.wmDeleteMessage = xlibFuncs.XInternAtom(windowState.display,"WM_DELETE_WINDOW", False);
    windowState.wmStateAtom = xlibFuncs.XInternAtom(windowState.display,"_NET_WM_STATE", False);
    windowState.wmStateFullscreen = xlibFuncs.XInternAtom(windowState.display,"_NET_WM_STATE_FULLSCREEN", False);
    windowState.clipboardAtom = xlibFuncs.XInternAtom(windowState.display,"CLIPBOARD", False);
    windowState.targetsAtom = xlibFuncs.XInternAtom(windowState.display,"TARGETS", False);
    windowState.utf8StringAtom = xlibFuncs.XInternAtom(windowState.display,"UTF8_STRING", False);

    // Initialize XKB
    windowState.xkbContext = xkbFuncs.xkb_context_new(static_cast<xkb_context_flags>(0));
    if (windowState.xkbContext == nullptr)
    {
        setError("Could not create XKB context");
        xlibFuncs.XCloseDisplay(windowState.display);
        return false;
    }

    struct xkb_rule_names names = {
        .rules = "evdev",
        .model = "pc105",
        .layout = "us",
        .variant = "",
        .options = ""
    };

    windowState.xkbKeymap = xkbFuncs.xkb_keymap_new_from_names(windowState.xkbContext, &names, static_cast<xkb_keymap_compile_flags>(0));
    if (windowState.xkbKeymap == nullptr)
    {
        setError("Could not create XKB keymap");
        xkbFuncs.xkb_context_unref(windowState.xkbContext);
        xlibFuncs.XCloseDisplay(windowState.display);
        return false;
    }

    windowState.xkbState = xkbFuncs.xkb_state_new(windowState.xkbKeymap);
    if (windowState.xkbState == nullptr)
    {
        setError("Could not create XKB state");
        xkbFuncs.xkb_keymap_unref(windowState.xkbKeymap);
        xkbFuncs.xkb_context_unref(windowState.xkbContext);
        xlibFuncs.XCloseDisplay(windowState.display);
        return false;
    }

    // Create default cursors
    windowState.cursors[CursorType_arrow] = xlibFuncs.XCreateFontCursor(windowState.display,static_cast<unsigned int>(XC_left_ptr));
    windowState.cursors[CursorType_textInput] = xlibFuncs.XCreateFontCursor(windowState.display,static_cast<unsigned int>(XC_xterm));
    windowState.cursors[CursorType_resizeAll] = xlibFuncs.XCreateFontCursor(windowState.display,static_cast<unsigned int>(XC_fleur));
    windowState.cursors[CursorType_resizeNESW] = xlibFuncs.XCreateFontCursor(windowState.display,static_cast<unsigned int>(XC_plus));
    windowState.cursors[CursorType_resizeNS] = xlibFuncs.XCreateFontCursor(windowState.display,static_cast<unsigned int>(XC_sb_v_double_arrow));
    windowState.cursors[CursorType_resizeNWSE] = xlibFuncs.XCreateFontCursor(windowState.display,static_cast<unsigned int>(XC_plus));
    windowState.cursors[CursorType_resizeEW] = xlibFuncs.XCreateFontCursor(windowState.display,static_cast<unsigned int>(XC_sb_h_double_arrow));
    windowState.cursors[CursorType_hand] = xlibFuncs.XCreateFontCursor(windowState.display,static_cast<unsigned int>(XC_hand2));
    windowState.cursors[CursorType_wait] = xlibFuncs.XCreateFontCursor(windowState.display,static_cast<unsigned int>(XC_watch));
    windowState.cursors[CursorType_progress] = xlibFuncs.XCreateFontCursor(windowState.display,static_cast<unsigned int>(XC_watch));
    windowState.cursors[CursorType_notAllowed] = xlibFuncs.XCreateFontCursor(windowState.display,static_cast<unsigned int>(XC_X_cursor));

    // Enumerate displays
    updateDisplayInfo();

    // Scan for gamepads
    scanForGamepads();

    return true;
}

void deinitX11()
{
    closeGamepads();

    for (u32 i = 0; i < CursorType_count; i++)
    {
        if (windowState.cursors[i] != 0)
            xlibFuncs.XFreeCursor(windowState.display,windowState.cursors[i]);
    }
    windowState.currentCursor = 0;

    // Cleanup XKB
    if (windowState.xkbState != nullptr)
        xkbFuncs.xkb_state_unref(windowState.xkbState);
    if (windowState.xkbKeymap != nullptr)
        xkbFuncs.xkb_keymap_unref(windowState.xkbKeymap);
    if (windowState.xkbContext != nullptr)
        xkbFuncs.xkb_context_unref(windowState.xkbContext);

    if (windowState.display != nullptr)
        xlibFuncs.XCloseDisplay(windowState.display);
}

WindowData::~WindowData() noexcept
{
    if (x11Window != 0)
    {
        windowState.windows.remove(x11Window);
        xlibFuncs.XDestroyWindow(windowState.display,x11Window);
    }
}

WindowData::WindowData(WindowData&& other) noexcept
    : swap{std::exchange(other.swap, GpuSwapchain{})}
    , x11Window{std::exchange(other.x11Window, 0)}
    , events{std::exchange(other.events, Array<Event>{})}
    , mouse{other.mouse}
    , width{other.width}
    , height{other.height}
    , wasClosed{other.wasClosed}
    , isFocused{other.isFocused}
    , wasFocusGained{other.wasFocusGained}
    , wasFocusLost{other.wasFocusLost}
    , wasResized{other.wasResized}
{}

WindowData& WindowData::operator=(WindowData&& other) noexcept
{
    if (this != &other)
    {
        if (x11Window != 0)
        {
            windowState.windows.remove(x11Window);
            xlibFuncs.XDestroyWindow(windowState.display,x11Window);
        }

        swap = std::exchange(other.swap, GpuSwapchain{});
        x11Window = std::exchange(other.x11Window, 0);
        events = std::exchange(other.events, Array<Event>{});
        mouse = other.mouse;
        width = other.width;
        height = other.height;
        wasClosed = other.wasClosed;
        isFocused = other.isFocused;
        wasFocusGained = other.wasFocusGained;
        wasFocusLost = other.wasFocusLost;
        wasResized = other.wasResized;
    }
    return *this;
}

Window windowCreate(const WindowConfig& config)
{
    (void)config;

    WindowData* window = new (heapAlloc(sizeof(WindowData), alignof(WindowData))) WindowData{};

    XWindowAttributes attrs;
    xlibFuncs.XGetWindowAttributes(windowState.display,windowState.root, &attrs);

    window->x11Window = xlibFuncs.XCreateWindow(
        windowState.display,
        windowState.root,
        0, 0,
        800, 600, // Default size
        0,
        CopyFromParent,
        InputOutput,
        nullptr,
        0,
        nullptr
    );

    if (window->x11Window == 0)
    {
        window->~WindowData();
        heapFree(window, 1);
        setError("Could not create X11 window");
        return Window{};
    }

    // Set window title
    xlibFuncs.XStoreName(windowState.display,window->x11Window, "Hurdy Gurdy");

    // Set WM_DELETE_WINDOW protocol
    xlibFuncs.XSetWMProtocols(windowState.display,window->x11Window, &windowState.wmDeleteMessage, 1);

    // Select input events
    xlibFuncs.XSelectInput(windowState.display,window->x11Window,
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
    createInfo.dpy = windowState.display;
    createInfo.window = window->x11Window;

    using PFN_vkCreateXlibSurfaceKHR = VkResult(*)(VkInstance, const VkXlibSurfaceCreateInfoKHR*, const VkAllocationCallbacks*, VkSurfaceKHR*);
    auto vkCreateXlibSurfaceKHR = reinterpret_cast<PFN_vkCreateXlibSurfaceKHR>(
        internal::getVulkanInstanceProcAddr("vkCreateXlibSurfaceKHR"));

    VkSurfaceKHR surface;
    VkResult result = vkCreateXlibSurfaceKHR(
        static_cast<VkInstance>(internal::getVulkanInstance()),
        &createInfo,
        nullptr,
        &surface
    );

    if (result != VK_SUCCESS)
    {
        xlibFuncs.XDestroyWindow(windowState.display,window->x11Window);
        window->~WindowData();
        heapFree(window, 1);
        setError("Could not create Vulkan surface");
        return Window{};
    }

    u32 winW = 800;
    u32 winH = 600;
    window->swap = GpuSwapchain::create(surface, winW, winH, config.preferredPresentMode, config.imageUsage);

    // Get initial window attributes
    xlibFuncs.XGetWindowAttributes(windowState.display,window->x11Window, &attrs);
    window->width = static_cast<u32>(attrs.width);
    window->height = static_cast<u32>(attrs.height);
    window->isFocused = (attrs.map_state == IsViewable);

    windowState.windows.add(window->x11Window, window);

    // Map the window
    xlibFuncs.XMapWindow(windowState.display,window->x11Window);

    Window w{};
    w.data = window;
    return w;
}

void windowDestroy(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    window->~WindowData();
    heapFree(window, 1);
}

GpuSwapchain& windowSwapchain(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->swap;
}

void windowSetTitle(void* data, StringView title)
{
    WindowData* window = static_cast<WindowData*>(data);
    char buf[256];
    u64 len = title.length < sizeof(buf) - 1 ? title.length : sizeof(buf) - 1;
    memcpy(buf, title.chars, len);
    buf[len] = '\0';
    xlibFuncs.XStoreName(windowState.display,window->x11Window, buf);
}

Span<Event> windowEvents(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->events;
}

bool windowWasClosed(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->wasClosed;
}

bool windowIsFocused(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->isFocused;
}

bool windowWasFocusGained(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->wasFocusGained;
}

bool windowWasFocusLost(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->wasFocusLost;
}





bool windowWasResized(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->wasResized;
}

void windowGetSize(void* data, u32* w, u32* h)
{
    WindowData* window = static_cast<WindowData*>(data);
    *w = window->width;
    *h = window->height;
}




void windowMaximize(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window->x11Window;
    event.xclient.message_type = xlibFuncs.XInternAtom(windowState.display,"_NET_WM_STATE", False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
    event.xclient.data.l[1] = static_cast<long>(windowState.wmStateFullscreen);
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 1;

    xlibFuncs.XSendEvent(windowState.display,windowState.root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}



void windowMinimize(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    xlibFuncs.XWithdrawWindow(windowState.display, window->x11Window, windowState.screen);
}


void windowRestore(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    xlibFuncs.XMapWindow(windowState.display,window->x11Window);
}



void windowSetFullscreen(void* data, bool set)
{
    WindowData* window = static_cast<WindowData*>(data);

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window->x11Window;
    event.xclient.message_type = windowState.wmStateAtom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = set ? 1 : 0; // _NET_WM_STATE_ADD or _NET_WM_STATE_REMOVE
    event.xclient.data.l[1] = static_cast<long>(windowState.wmStateFullscreen);
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 1;

    xlibFuncs.XSendEvent(windowState.display,windowState.root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

Vec2 windowMousePos(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->mouse;
}

Vec2 windowMouseDelta(void* data)
{
    (void)data;
    return windowState.mouseDelta;
}

// Global functions
Span<DisplayInfo> displayInfo()
{
    return windowState.displays;
}

void setCursor(CursorType type)
{
    if (type < CursorType_count && windowState.cursors[type] != 0)
    {
        windowState.currentCursor = windowState.cursors[type];
        // Apply to all windows
        windowState.windows.forEach([](::Window, WindowData* window)
        {
            if (windowState.currentCursor != 0)
                xlibFuncs.XDefineCursor(windowState.display,window->x11Window, windowState.currentCursor);
            else
                xlibFuncs.XUndefineCursor(windowState.display,window->x11Window);
        });
    }
}

void showCursor(bool show)
{
    if (show)
    {
        windowState.windows.forEach([](::Window, WindowData* window)
        {
            if (windowState.currentCursor != 0)
                xlibFuncs.XDefineCursor(windowState.display,window->x11Window, windowState.currentCursor);
        });
    }
    else
    {
        windowState.windows.forEach([](::Window, WindowData* window)
        {
            // Create a 1x1 transparent cursor
            Pixmap pixmap = xlibFuncs.XCreatePixmap(windowState.display, windowState.root, 1, 1, 1);
            XColor color{};
            Cursor cursor = xlibFuncs.XCreatePixmapCursor(windowState.display, pixmap, pixmap, &color, &color, 0, 0);
            xlibFuncs.XDefineCursor(windowState.display,window->x11Window, cursor);
            xlibFuncs.XFreeCursor(windowState.display,cursor);
                xlibFuncs.XFreePixmap(windowState.display, pixmap);
        });
    }
}

void processEvents()
{
    // Copy previous key state
    memcpy(windowState.wasKeyDown, windowState.isKeyDown, sizeof(windowState.isKeyDown));

    // Reset deltas
    windowState.mouseDelta = Vec2{};
    windowState.wheelDelta = Vec2{};

    // Reset per-window flags
    windowState.windows.forEach([](::Window, WindowData* window)
    {
        window->wasFocusGained = false;
        window->wasFocusLost = false;
        window->wasResized = false;
        window->events.resize(0);
    });

    // Clear global events
    windowState.events.resize(0);

    // Read gamepad events
    readGamepadEvents();
    pollGamepadDevices();

    // Process X11 events
    while (xlibFuncs.XPending(windowState.display))
    {
        XEvent event;
        xlibFuncs.XNextEvent(windowState.display,&event);

        switch (event.type)
        {
            case KeyPress:
            case KeyRelease:
            {
                bool pressed = (event.type == KeyPress);
                KeySym keysym = xkbFuncs.xkb_state_key_get_one_sym(windowState.xkbState, event.xkey.keycode);

                // Update XKB state
                xkbFuncs.xkb_state_update_mask(windowState.xkbState,
                    event.xkey.state & ShiftMask ? 1 : 0,
                    event.xkey.state & LockMask ? 1 : 0,
                    0,
                    0, 0, 0);

                Button button = xkbKeysymToButton(keysym);
                if (button < Button_count)
                {
                    windowState.isKeyDown[button] = pressed;

                    Event e{};
                    e.type = pressed ? EventType_keyPress : EventType_keyRelease;
                    e.button = button;
                    windowState.events.push(e);
                }

                // Text input
                if (pressed)
                {
                    char buf[32];
                    int len = xkbFuncs.xkb_state_key_get_utf8(windowState.xkbState, event.xkey.keycode, buf, sizeof(buf) - 1);
                    if (len > 0)
                    {
                        buf[len] = '\0';
                        Event e{};
                        e.type = EventType_text;
                        memset(e.text, 0, sizeof(e.text));
                        memcpy(e.text, buf, static_cast<u64>(len));
                        windowState.events.push(e);
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
                        if (event.xbutton.button == 4) delta.y = 1.0f;
                        else if (event.xbutton.button == 5) delta.y = -1.0f;
                        else if (event.xbutton.button == 6) delta.x = 1.0f;
                        else if (event.xbutton.button == 7) delta.x = -1.0f;

                        windowState.wheelDelta += delta;

                        Event e{};
                        e.type = EventType_wheelMoved;
                        e.wheel.delta = delta;
                        windowState.events.push(e);
                    }
                    break;
                }

                Button button = xButtonToButton(event.xbutton.button);

                if (button < Button_count)
                {
                    windowState.isKeyDown[button] = pressed;

                    Event e{};
                    e.type = pressed ? EventType_keyPress : EventType_keyRelease;
                    e.button = button;
                    windowState.events.push(e);
                }

                break;
            }

            case MotionNotify:
            {
                WindowData** found = windowState.windows.get(event.xmotion.window);
                if (found != nullptr)
                {
                    WindowData* window = *found;
                    Vec2 newPos{static_cast<f32>(event.xmotion.x), static_cast<f32>(event.xmotion.y)};
                    Vec2 delta = newPos - window->mouse;
                    window->mouse = newPos;
                    windowState.mouseDelta += delta;

                    Event e{};
                    e.type = EventType_mouseMoved;
                    e.mouse.delta = delta;
                    e.mouse.pos = newPos;
                    e.mouse.globalPos = Vec2{static_cast<f32>(event.xmotion.x_root), static_cast<f32>(event.xmotion.y_root)};
                    windowState.events.push(e);
                }
                break;
            }

            case FocusIn:
            case FocusOut:
            {
                WindowData** found = windowState.windows.get(event.xfocus.window);
                if (found != nullptr)
                {
                    WindowData* window = *found;
                    bool focused = (event.type == FocusIn);
                    if (focused != window->isFocused)
                    {
                        window->isFocused = focused;
                        if (focused)
                        {
                            window->wasFocusGained = true;
                            windowState.activeWindow = window;
                        }
                        else
                        {
                            window->wasFocusLost = true;
                            if (windowState.activeWindow == window)
                                windowState.activeWindow = nullptr;
                        }

                        Event e{};
                        e.type = focused ? EventType_windowFocused : EventType_windowUnfocused;
                        windowState.events.push(e);
                    }
                }
                break;
            }

            case ConfigureNotify:
            {
                WindowData** found = windowState.windows.get(event.xconfigure.window);
                if (found != nullptr)
                {
                    WindowData* window = *found;
                    u32 newW = static_cast<u32>(event.xconfigure.width);
                    u32 newH = static_cast<u32>(event.xconfigure.height);

                    if (newW != window->width || newH != window->height)
                    {
                        window->wasResized = true;
                        window->width = newW;
                        window->height = newH;
                    }
                }
                break;
            }

            case ClientMessage:
            {
                if (event.xclient.message_type == windowState.wmProtocols &&
                    static_cast<Atom>(event.xclient.data.l[0]) == windowState.wmDeleteMessage)
                {
                    windowState.wasQuit = true;
                    Event e{};
                    e.type = EventType_quit;
                    windowState.events.push(e);

                    WindowData** found = windowState.windows.get(event.xclient.window);
                    if (found != nullptr)
                    {
                        (*found)->wasClosed = true;
                        Event e2{};
                        e2.type = EventType_windowClosed;
                        windowState.events.push(e2);
                    }
                }
                break;
            }

            case SelectionNotify:
            {
                if (event.xselection.selection == windowState.clipboardAtom &&
                    event.xselection.property != None)
                {
                    // Get clipboard data
                    Atom actualType;
                    int actualFormat;
                    unsigned long nitems, bytesAfter;
                    unsigned char* data = nullptr;

                    xlibFuncs.XGetWindowProperty(windowState.display,
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
                        windowState.clipboard.resize(nitems);
                        memcpy(windowState.clipboard.vals, data, nitems);
                        xlibFuncs.XFree(data);
                    }
                }
                break;
            }

            case SelectionRequest:
            {
                if (event.xselectionrequest.selection == windowState.clipboardAtom)
                {
                    XEvent reply{};
                    reply.xselection.type = SelectionNotify;
                    reply.xselection.display = event.xselectionrequest.display;
                    reply.xselection.requestor = event.xselectionrequest.requestor;
                    reply.xselection.selection = event.xselectionrequest.selection;
                    reply.xselection.target = event.xselectionrequest.target;
                    reply.xselection.time = event.xselectionrequest.time;

                    if (event.xselectionrequest.target == windowState.targetsAtom)
                    {
                        // Send supported targets
                        Atom targets[] = {windowState.targetsAtom, windowState.utf8StringAtom};
                        xlibFuncs.XChangeProperty(windowState.display,
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
                    else if (event.xselectionrequest.target == windowState.utf8StringAtom)
                    {
                        // Send clipboard content
                        if (windowState.clipboard.count > 0)
                        {
                            xlibFuncs.XChangeProperty(windowState.display,
                                event.xselectionrequest.requestor,
                                event.xselectionrequest.property,
                                windowState.utf8StringAtom,
                                8,
                                PropModeReplace,
                                reinterpret_cast<const unsigned char*>(windowState.clipboard.vals),
                                static_cast<int>(windowState.clipboard.count)
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
    windowState.windows.forEach([](::Window, WindowData* window)
    {
        window->events.count = 0;
        for (u64 i = 0; i < windowState.events.count; ++i)
            window->events.push(windowState.events[i]);
    });
}

Span<Event> getEvents()
{
    return windowState.events;
}

bool wasQuit()
{
    return windowState.wasQuit;
}

bool isButtonDown(Button key)
{
    return windowState.isKeyDown[key];
}

bool wasButtonPressed(Button key)
{
    return windowState.isKeyDown[key] && !windowState.wasKeyDown[key];
}

bool wasButtonReleased(Button key)
{
    return !windowState.isKeyDown[key] && windowState.wasKeyDown[key];
}

Vec2 mousePos()
{
    if (windowState.activeWindow != nullptr)
        return windowState.activeWindow->mouse;

    if (windowState.windows.count == 1)
    {
        Vec2 ret{};
        windowState.windows.forEach([&](::Window, WindowData* wd)
        {
            ret = wd->mouse;
        });
        return ret;
    }
    return {};
}

Vec2 mouseDelta()
{
    if (windowState.activeWindow != nullptr)
        return windowState.mouseDelta / static_cast<f32>(windowState.activeWindow->height);

    if (windowState.windows.count == 1)
    {
        Vec2 ret = windowState.mouseDelta;
        windowState.windows.forEach([&](::Window, WindowData* wd)
        {
            ret = ret / static_cast<f32>(wd->height);
        });
        return ret;
    }
    return windowState.mouseDelta;
}

Vec2 wheelDelta()
{
    return windowState.wheelDelta;
}

u32 connectedGamepadCount()
{
    return windowState.gamepadCount;
}

bool isGamepadConnected(u32 gamepad)
{
    return gamepad < windowState.gamepadCount && windowState.gamepads[gamepad].fd >= 0;
}

bool isGamepadButtonDown(u32 gamepad, Button key)
{
    if (gamepad >= windowState.gamepadCount)
        return false;

    if (static_cast<u32>(key) >= GamepadButton_count)
        return false;

    return windowState.gamepads[gamepad].isButtonDown[static_cast<GamepadButton>(key)];
}

bool wasGamepadButtonPressed(u32 gamepad, GamepadButton key)
{
    if (gamepad >= windowState.gamepadCount)
        return false;

    const GamepadState& state = windowState.gamepads[gamepad];
    return !state.wasButtonDown[key] && state.isButtonDown[key];
}

bool wasGamepadButtonReleased(u32 gamepad, GamepadButton key)
{
    if (gamepad >= windowState.gamepadCount)
        return false;

    const GamepadState& state = windowState.gamepads[gamepad];
    return state.wasButtonDown[key] && !state.isButtonDown[key];
}

Vec2 gamepadLeftStick(u32 gamepad)
{
    if (gamepad >= windowState.gamepadCount)
        return Vec2{};

    const GamepadState& state = windowState.gamepads[gamepad];
    constexpr f32 deadzone = 0.15f;
    f32 x = static_cast<f32>(state.axes[0]) / 32767.0f;
    f32 y = static_cast<f32>(state.axes[1]) / 32767.0f;

    if (fabsf(x) < deadzone) x = 0.0f;
    if (fabsf(y) < deadzone) y = 0.0f;

    return Vec2{x, y};
}

Vec2 gamepadRightStick(u32 gamepad)
{
    if (gamepad >= windowState.gamepadCount)
        return Vec2{};

    const GamepadState& state = windowState.gamepads[gamepad];
    constexpr f32 deadzone = 0.15f;
    f32 x = static_cast<f32>(state.axes[2]) / 32767.0f;
    f32 y = static_cast<f32>(state.axes[3]) / 32767.0f;

    if (fabsf(x) < deadzone) x = 0.0f;
    if (fabsf(y) < deadzone) y = 0.0f;

    return Vec2{x, y};
}

f32 gamepadLeftTrigger(u32 gamepad)
{
    if (gamepad >= windowState.gamepadCount)
        return 0.0f;

    const GamepadState& state = windowState.gamepads[gamepad];
    return static_cast<f32>(state.axes[4]) / 32767.0f;
}

f32 gamepadRightTrigger(u32 gamepad)
{
    if (gamepad >= windowState.gamepadCount)
        return 0.0f;

    const GamepadState& state = windowState.gamepads[gamepad];
    return static_cast<f32>(state.axes[5]) / 32767.0f;
}

StringView getClipboardText()
{
    if (windowState.clipboard.count == 0)
        return StringView{};

    return StringView{windowState.clipboard.vals, windowState.clipboard.count};
}

void setClipboardText(StringView text)
{
    windowState.clipboard.resize(text.length);
    memcpy(windowState.clipboard.vals, text.chars, text.length);

    // Claim clipboard ownership
    xlibFuncs.XSetSelectionOwner(windowState.display,windowState.clipboardAtom, windowState.root, CurrentTime);
}

void openURL(StringView url)
{
    // Use xdg-open for URL opening
    char cmd[1024];
    u64 len = url.length < sizeof(cmd) - 16 ? url.length : sizeof(cmd) - 16;
    memcpy(cmd, url.chars, len);
    cmd[len] = '\0';

    char fullCmd[1024];
    snprintf(fullCmd, sizeof(fullCmd), "xdg-open '%s' &", cmd);
    (void)system(fullCmd);
}

} // namespace hg::x11
