#include "hg/window.hpp"

#include "x11_internal.hpp"
#include "x11_platform.hpp"
#include "internal.hpp"
#include "hg/error.hpp"
#include "hg/array.hpp"
#include "hg/map.hpp"

#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>

namespace hg::x11 {

struct WindowData {
    GpuSwapchain swap{};

    ::Window x11Window = 0;

    Array<Event> events{};
    Vec2 mouse{};
    i32 x = 0;
    i32 y = 0;
    u32 width = 0;
    u32 height = 0;
    bool wasClosed = false;
    bool isFocused = false;
    bool wasFocusGained = false;
    bool wasFocusLost = false;
    bool wasMoved = false;
    bool wasResized = false;
    bool wasMaximized = false;
    bool wasMinimized = false;
    bool wasRestored = false;
    bool wasFullscreened = false;

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

// Xlib function accessors
static inline Display* xDisplay() { return windowState.display; }
static inline int xDefaultScreen() { return windowState.screen; }
static inline ::Window xRoot() { return windowState.root; }

// xkb function accessors
static inline struct xkb_context* xkbContextNew(xkb_context_flags flags)
{
    return xkbFuncs.xkb_context_new(flags);
}
static inline struct xkb_keymap* xkbKeymapNewFromNames(struct xkb_context* context, const struct xkb_rule_names* names, xkb_keymap_compile_flags flags)
{
    return xkbFuncs.xkb_keymap_new_from_names(context, names, flags);
}
static inline struct xkb_state* xkbStateNew(struct xkb_keymap* keymap)
{
    return xkbFuncs.xkb_state_new(keymap);
}
static inline void xkbContextUnref(struct xkb_context* context)
{
    xkbFuncs.xkb_context_unref(context);
}
static inline void xkbKeymapUnref(struct xkb_keymap* keymap)
{
    xkbFuncs.xkb_keymap_unref(keymap);
}
static inline void xkbStateUnref(struct xkb_state* state)
{
    xkbFuncs.xkb_state_unref(state);
}

// evdev function accessors
static inline int evdevNewFromFd(int fd, struct libevdev** dev)
{
    return evdevFuncs.libevdev_new_from_fd(fd, dev);
}

static inline void evdevFree(struct libevdev* dev)
{
    evdevFuncs.libevdev_free(dev);
}

static inline const char* evdevGetName(struct libevdev* dev)
{
    return evdevFuncs.libevdev_get_name(dev);
}

static inline unsigned int evdevGetIdVendor(struct libevdev* dev)
{
    return static_cast<unsigned int>(evdevFuncs.libevdev_get_id_vendor(dev));
}

static inline unsigned int evdevGetIdProduct(struct libevdev* dev)
{
    return static_cast<unsigned int>(evdevFuncs.libevdev_get_id_product(dev));
}

static inline int evdevHasEventType(struct libevdev* dev, unsigned int type)
{
    return evdevFuncs.libevdev_has_event_type(dev, type);
}

static inline int evdevHasEventCode(struct libevdev* dev, unsigned int type, unsigned int code)
{
    return evdevFuncs.libevdev_has_event_code(dev, type, code);
}

static inline int evdevNextEvent(struct libevdev* dev, unsigned int flags, struct input_event* ev)
{
    return evdevFuncs.libevdev_next_event(dev, flags, ev);
}

// Xlib function accessors
static inline int xPending() { return xlibFuncs.XPending(xDisplay()); }
static inline int xNextEvent(XEvent* e) { return xlibFuncs.XNextEvent(xDisplay(), e); }
static inline Atom xInternAtom(const char* n, Bool e) { return xlibFuncs.XInternAtom(xDisplay(), n, e); }
static inline int xSelectInput(::Window w, long m) { return xlibFuncs.XSelectInput(xDisplay(), w, m); }
static inline int xMapWindow(::Window w) { return xlibFuncs.XMapWindow(xDisplay(), w); }
static inline int xWithdrawWindow(::Window w) { return xlibFuncs.XWithdrawWindow(xDisplay(), w, xDefaultScreen()); }
static inline int xDestroyWindow(::Window w) { return xlibFuncs.XDestroyWindow(xDisplay(), w); }
static inline int xResizeWindow(::Window w, unsigned int w2, unsigned int h) { return xlibFuncs.XResizeWindow(xDisplay(), w, w2, h); }
static inline int xMoveWindow(::Window w, int x, int y) { return xlibFuncs.XMoveWindow(xDisplay(), w, x, y); }
static inline int xGetWindowAttributes(::Window w, XWindowAttributes* a) { return xlibFuncs.XGetWindowAttributes(xDisplay(), w, a); }
static inline Status xSetWMProtocols(::Window w, Atom* p, int c) { return xlibFuncs.XSetWMProtocols(xDisplay(), w, p, c); }
static inline int xStoreName(::Window w, const char* n) { return xlibFuncs.XStoreName(xDisplay(), w, n); }
static inline int xFree(void* d) { return xlibFuncs.XFree(d); }
static inline int xChangeProperty(::Window w, Atom p, Atom t, int f, int m, const unsigned char* d, int n) { return xlibFuncs.XChangeProperty(xDisplay(), w, p, t, f, m, d, n); }
static inline int xGetWindowProperty(::Window w, Atom p, long o, long l, Bool d, Atom rt, Atom* at, int* af, unsigned long* ni, unsigned long* ba, unsigned char** pr) { return xlibFuncs.XGetWindowProperty(xDisplay(), w, p, o, l, d, rt, at, af, ni, ba, pr); }
static inline Status xSetSelectionOwner(Atom s, ::Window o, Time t) { return xlibFuncs.XSetSelectionOwner(xDisplay(), s, o, t); }
static inline int xSendEvent(::Window w, Bool p, long m, XEvent* e) { return xlibFuncs.XSendEvent(xDisplay(), w, p, m, e); }
static inline int xDefineCursor(::Window w, Cursor c) { return xlibFuncs.XDefineCursor(xDisplay(), w, c); }
static inline int xUndefineCursor(::Window w) { return xlibFuncs.XUndefineCursor(xDisplay(), w); }
static inline Cursor xCreateFontCursor(unsigned int s) { return xlibFuncs.XCreateFontCursor(xDisplay(), s); }
static inline int xFreeCursor(Cursor c) { return xlibFuncs.XFreeCursor(xDisplay(), c); }
static inline KeySym xkbKeycodeToKeysym(KeyCode kc, int g, int l) { return xlibFuncs.XKeycodeToKeysym(xDisplay(), kc, g, l); }

static KeySym xkbKeycodeToKeysym(KeyCode keycode)
{
    return xkbKeycodeToKeysym(keycode, 0, 0);
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
    XRRScreenResources* res = xrandrFuncs.XRRGetScreenResourcesCurrent(xDisplay(), xRoot());
    if (res == nullptr)
        return;

    windowState.displays.resize(static_cast<u64>(res->ncrtc));

    for (u32 i = 0; i < static_cast<u32>(res->ncrtc); i++)
    {
        DisplayInfo& info = windowState.displays[i];

        XRRCrtcInfo* crtc = xrandrFuncs.XRRGetCrtcInfo(xDisplay(), res, res->crtcs[i]);
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
        int rc = evdevNewFromFd(fd, &dev);
        if (rc < 0 || dev == nullptr)
        {
            close(fd);
            continue;
        }

        // Check if this is a gamepad (has BTN_SOUTH and ABS_X)
        if (!evdevHasEventType(dev, EV_KEY) || !evdevHasEventCode(dev, EV_KEY, BTN_SOUTH) ||
            !evdevHasEventType(dev, EV_ABS) || !evdevHasEventCode(dev, EV_ABS, ABS_X))
        {
            evdevFree(dev);
            close(fd);
            continue;
        }

        GamepadState& gamepad = windowState.gamepads[windowState.gamepadCount];
        gamepad.fd = fd;
        gamepad.dev = dev;
        strncpy(gamepad.name, evdevGetName(dev), sizeof(gamepad.name) - 1);
    gamepad.vendorId = static_cast<u16>(evdevGetIdVendor(dev));
    gamepad.productId = static_cast<u16>(evdevGetIdProduct(dev));

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
            evdevFree(gamepad.dev);
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
        while ((rc = evdevNextEvent(gamepad.dev, LIBEVDEV_READ_FLAG_NORMAL, &ev)) == LIBEVDEV_READ_STATUS_SUCCESS)
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
                evdevFree(gamepad.dev);
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

    windowState.screen = xDefaultScreen();
    windowState.root = xlibFuncs.XRootWindow(windowState.display, windowState.screen);

    // Create atoms
    windowState.wmProtocols = xInternAtom("WM_PROTOCOLS", False);
    windowState.wmDeleteMessage = xInternAtom("WM_DELETE_WINDOW", False);
    windowState.wmStateAtom = xInternAtom("_NET_WM_STATE", False);
    windowState.wmStateFullscreen = xInternAtom("_NET_WM_STATE_FULLSCREEN", False);
    windowState.clipboardAtom = xInternAtom("CLIPBOARD", False);
    windowState.targetsAtom = xInternAtom("TARGETS", False);
    windowState.utf8StringAtom = xInternAtom("UTF8_STRING", False);

    // Initialize XKB
    windowState.xkbContext = xkbContextNew(static_cast<xkb_context_flags>(0));
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

    windowState.xkbKeymap = xkbKeymapNewFromNames(windowState.xkbContext, &names, static_cast<xkb_keymap_compile_flags>(0));
    if (windowState.xkbKeymap == nullptr)
    {
        setError("Could not create XKB keymap");
        xkbContextUnref(windowState.xkbContext);
        xlibFuncs.XCloseDisplay(windowState.display);
        return false;
    }

    windowState.xkbState = xkbStateNew(windowState.xkbKeymap);
    if (windowState.xkbState == nullptr)
    {
        setError("Could not create XKB state");
        xkbKeymapUnref(windowState.xkbKeymap);
        xkbContextUnref(windowState.xkbContext);
        xlibFuncs.XCloseDisplay(windowState.display);
        return false;
    }

    // Create default cursors
    windowState.cursors[CursorType_arrow] = xCreateFontCursor(static_cast<unsigned int>(XC_left_ptr));
    windowState.cursors[CursorType_textInput] = xCreateFontCursor(static_cast<unsigned int>(XC_xterm));
    windowState.cursors[CursorType_resizeAll] = xCreateFontCursor(static_cast<unsigned int>(XC_fleur));
    windowState.cursors[CursorType_resizeNESW] = xCreateFontCursor(static_cast<unsigned int>(XC_plus));
    windowState.cursors[CursorType_resizeNS] = xCreateFontCursor(static_cast<unsigned int>(XC_sb_v_double_arrow));
    windowState.cursors[CursorType_resizeNWSE] = xCreateFontCursor(static_cast<unsigned int>(XC_plus));
    windowState.cursors[CursorType_resizeEW] = xCreateFontCursor(static_cast<unsigned int>(XC_sb_h_double_arrow));
    windowState.cursors[CursorType_hand] = xCreateFontCursor(static_cast<unsigned int>(XC_hand2));
    windowState.cursors[CursorType_wait] = xCreateFontCursor(static_cast<unsigned int>(XC_watch));
    windowState.cursors[CursorType_progress] = xCreateFontCursor(static_cast<unsigned int>(XC_watch));
    windowState.cursors[CursorType_notAllowed] = xCreateFontCursor(static_cast<unsigned int>(XC_X_cursor));

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
            xFreeCursor(windowState.cursors[i]);
    }
    windowState.currentCursor = 0;

    // Cleanup XKB
    if (windowState.xkbState != nullptr)
        xkbStateUnref(windowState.xkbState);
    if (windowState.xkbKeymap != nullptr)
        xkbKeymapUnref(windowState.xkbKeymap);
    if (windowState.xkbContext != nullptr)
        xkbContextUnref(windowState.xkbContext);

    if (windowState.display != nullptr)
        xlibFuncs.XCloseDisplay(windowState.display);
}

WindowData::~WindowData() noexcept
{
    if (x11Window != 0)
    {
        windowState.windows.remove(x11Window);
        xDestroyWindow(x11Window);
    }
}

WindowData::WindowData(WindowData&& other) noexcept
    : swap{std::exchange(other.swap, GpuSwapchain{})}
    , x11Window{std::exchange(other.x11Window, 0)}
    , events{std::exchange(other.events, Array<Event>{})}
    , mouse{other.mouse}
    , x{other.x}
    , y{other.y}
    , width{other.width}
    , height{other.height}
    , wasClosed{other.wasClosed}
    , isFocused{other.isFocused}
    , wasFocusGained{other.wasFocusGained}
    , wasFocusLost{other.wasFocusLost}
    , wasMoved{other.wasMoved}
    , wasResized{other.wasResized}
    , wasMaximized{other.wasMaximized}
    , wasMinimized{other.wasMinimized}
    , wasRestored{other.wasRestored}
    , wasFullscreened{other.wasFullscreened}
{}

WindowData& WindowData::operator=(WindowData&& other) noexcept
{
    if (this != &other)
    {
        if (x11Window != 0)
        {
            windowState.windows.remove(x11Window);
            xDestroyWindow(x11Window);
        }

        swap = std::exchange(other.swap, GpuSwapchain{});
        x11Window = std::exchange(other.x11Window, 0);
        events = std::exchange(other.events, Array<Event>{});
        mouse = other.mouse;
        x = other.x;
        y = other.y;
        width = other.width;
        height = other.height;
        wasClosed = other.wasClosed;
        isFocused = other.isFocused;
        wasFocusGained = other.wasFocusGained;
        wasFocusLost = other.wasFocusLost;
        wasMoved = other.wasMoved;
        wasResized = other.wasResized;
        wasMaximized = other.wasMaximized;
        wasMinimized = other.wasMinimized;
        wasRestored = other.wasRestored;
        wasFullscreened = other.wasFullscreened;
    }
    return *this;
}

Window windowCreate(const WindowConfig& config)
{
    (void)config;

    WindowData* window = new (heapAlloc(sizeof(WindowData), alignof(WindowData))) WindowData{};

    XWindowAttributes attrs;
    xGetWindowAttributes(xRoot(), &attrs);

    window->x11Window = xlibFuncs.XCreateWindow(
        xDisplay(),
        xRoot(),
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
    xStoreName(window->x11Window, "Hurdy Gurdy");

    // Set WM_DELETE_WINDOW protocol
    xSetWMProtocols(window->x11Window, &windowState.wmDeleteMessage, 1);

    // Select input events
    xSelectInput(window->x11Window,
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
    createInfo.dpy = xDisplay();
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
        xDestroyWindow(window->x11Window);
        window->~WindowData();
        heapFree(window, 1);
        setError("Could not create Vulkan surface");
        return Window{};
    }

    u32 winW = 800;
    u32 winH = 600;
    window->swap = GpuSwapchain::create(surface, winW, winH, config.preferredPresentMode, config.imageUsage);

    // Get initial window attributes
    xGetWindowAttributes(window->x11Window, &attrs);
    window->x = attrs.x;
    window->y = attrs.y;
    window->width = static_cast<u32>(attrs.width);
    window->height = static_cast<u32>(attrs.height);
    window->isFocused = (attrs.map_state == IsViewable);

    windowState.windows.add(window->x11Window, window);

    // Map the window
    xMapWindow(window->x11Window);

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
    xStoreName(window->x11Window, buf);
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

bool windowWasMoved(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->wasMoved;
}

void windowGetPos(void* data, i32* x, i32* y)
{
    WindowData* window = static_cast<WindowData*>(data);
    *x = window->x;
    *y = window->y;
}

void windowSetPos(void* data, i32 x, i32 y)
{
    WindowData* window = static_cast<WindowData*>(data);
    xMoveWindow(window->x11Window, x, y);
}

void windowSetResizable(void* data, bool set)
{
    (void)data;
    (void)set;
    // X11 doesn't have a direct way to toggle resizability
    // We would need to modify the WM hints
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

void windowSetSize(void* data, u32 w, u32 h)
{
    WindowData* window = static_cast<WindowData*>(data);
    xResizeWindow(window->x11Window, w, h);
}

bool windowIsMaximized(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->wasMaximized;
}

bool windowWasMaximized(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->wasMaximized;
}

void windowMaximize(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window->x11Window;
    event.xclient.message_type = xInternAtom("_NET_WM_STATE", False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
    event.xclient.data.l[1] = static_cast<long>(windowState.wmStateFullscreen);
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 1;

    xSendEvent(windowState.root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

bool windowIsMinimized(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->wasMinimized;
}

bool windowWasMinimized(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->wasMinimized;
}

void windowMinimize(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    xWithdrawWindow(window->x11Window);
}

bool windowWasRestored(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->wasRestored;
}

void windowRestore(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    xMapWindow(window->x11Window);
}

bool windowIsFullscreen(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    Atom actualType;
    int actualFormat;
    u64 itemCount;
    u64 bytesAfter;
    u8* prop = nullptr;
    xGetWindowProperty(
        window->x11Window, windowState.wmStateAtom, 0, 1024, False,
        XA_ATOM, &actualType, &actualFormat, &itemCount, &bytesAfter, &prop
    );
    if (prop == nullptr)
        return false;
    bool fullscreen = false;
    for (u64 i = 0; i < itemCount; ++i)
    {
        if (reinterpret_cast<Atom*>(prop)[i] == windowState.wmStateFullscreen)
        {
            fullscreen = true;
            break;
        }
    }
    xFree(prop);
    return fullscreen;
}

bool windowWasMadeFullscreen(void* data)
{
    WindowData* window = static_cast<WindowData*>(data);
    return window->wasFullscreened;
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

    xSendEvent(windowState.root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
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
                xDefineCursor(window->x11Window, windowState.currentCursor);
            else
                xUndefineCursor(window->x11Window);
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
                xDefineCursor(window->x11Window, windowState.currentCursor);
        });
    }
    else
    {
        windowState.windows.forEach([](::Window, WindowData* window)
        {
            // Create a 1x1 transparent cursor
            Pixmap pixmap = xlibFuncs.XCreatePixmap(xDisplay(), xRoot(), 1, 1, 1);
            XColor color{};
            Cursor cursor = xlibFuncs.XCreatePixmapCursor(xDisplay(), pixmap, pixmap, &color, &color, 0, 0);
            xDefineCursor(window->x11Window, cursor);
            xFreeCursor(cursor);
                xlibFuncs.XFreePixmap(xDisplay(), pixmap);
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
        window->wasMoved = false;
        window->wasResized = false;
        window->wasMaximized = false;
        window->wasMinimized = false;
        window->wasRestored = false;
        window->wasFullscreened = false;
        window->events.resize(0);
    });

    // Clear global events
    windowState.events.resize(0);

    // Read gamepad events
    readGamepadEvents();
    pollGamepadDevices();

    // Process X11 events
    while (xPending())
    {
        XEvent event;
        xNextEvent(&event);

        switch (event.type)
        {
            case KeyPress:
            case KeyRelease:
            {
                bool pressed = (event.type == KeyPress);
                KeySym keysym = xkbKeycodeToKeysym(static_cast<KeyCode>(event.xkey.keycode));

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
                            window->wasFocusGained = true;
                        else
                            window->wasFocusLost = true;

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
                    i32 newX = event.xconfigure.x;
                    i32 newY = event.xconfigure.y;
                    u32 newW = static_cast<u32>(event.xconfigure.width);
                    u32 newH = static_cast<u32>(event.xconfigure.height);

                    if (newX != window->x || newY != window->y)
                    {
                        window->wasMoved = true;
                        window->x = newX;
                        window->y = newY;
                    }

                    if (newW != window->width || newH != window->height)
                    {
                        window->wasResized = true;
                        window->width = newW;
                        window->height = newH;
                    }

                    // Update Vulkan swapchain
                    if (window->wasResized)
                    {
                        // TODO: Recreate swapchain
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

                    xGetWindowProperty(
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
                        xFree(data);
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
                        xChangeProperty(
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
                            xChangeProperty(
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

            case MapNotify:
            {
                WindowData** found = windowState.windows.get(event.xmap.window);
                if (found != nullptr)
                {
                    (*found)->wasRestored = true;
                }
                break;
            }

            case PropertyNotify:
            {
                if (event.xproperty.atom == windowState.wmStateAtom)
                {
                    WindowData** found = windowState.windows.get(event.xproperty.window);
                    if (found != nullptr)
                    {
                        // Query current fullscreen state
                        Atom actualType;
                        int actualFormat;
                        u64 itemCount;
                        u64 bytesAfter;
                        u8* prop = nullptr;
                        xGetWindowProperty(
                            event.xproperty.window, windowState.wmStateAtom, 0, 1024, False,
                            XA_ATOM, &actualType, &actualFormat, &itemCount, &bytesAfter, &prop
                        );
                        if (prop != nullptr)
                        {
                            bool fullscreen = false;
                            for (u64 i = 0; i < itemCount; ++i)
                            {
                                if (reinterpret_cast<Atom*>(prop)[i] == windowState.wmStateFullscreen)
                                {
                                    fullscreen = true;
                                    break;
                                }
                            }
                            if (fullscreen)
                                (*found)->wasFullscreened = true;
                            xFree(prop);
                        }
                    }
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
    if (windowState.windows.count == 1)
    {
        Vec2 ret{};
        windowState.windows.forEach([&](::Window, WindowData* wd)
        {
            ret = wd->mouse;
        });
        return ret;
    }
    int rootX, rootY, winX, winY;
    ::Window root_ret, child_ret;
    unsigned int mask;
    xlibFuncs.XQueryPointer(xDisplay(), xRoot(), &root_ret, &child_ret, &rootX, &rootY, &winX, &winY, &mask);
    return Vec2{static_cast<f32>(rootX), static_cast<f32>(rootY)};
}

Vec2 mouseDelta()
{
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
    xSetSelectionOwner(windowState.clipboardAtom, xRoot(), CurrentTime);
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

} // namespace hg::linux_backend
