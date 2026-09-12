#include "wayland_platform.hpp"

#include "internal.hpp"
#include "hg/error.hpp"
#include "hg/dynlib.hpp"
#include "hg/array.hpp"
#include "hg/map.hpp"

#include <math.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <poll.h>

#include <wayland-client-core.h>
#include <wayland-egl-core.h>

#include <xkbcommon/xkbcommon.h>

#include <linux/input.h>
#include <libevdev/libevdev.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

#define HG_WL_FUNC(name) decltype(&::name) name = nullptr

struct WaylandFuncs {
    HG_WL_FUNC(wl_display_connect);
    HG_WL_FUNC(wl_display_disconnect);
    HG_WL_FUNC(wl_display_get_fd);
    HG_WL_FUNC(wl_display_dispatch);
    HG_WL_FUNC(wl_display_dispatch_pending);
    HG_WL_FUNC(wl_display_flush);
    HG_WL_FUNC(wl_display_roundtrip);
    HG_WL_FUNC(wl_display_get_error);
    HG_WL_FUNC(wl_display_prepare_read);
    HG_WL_FUNC(wl_display_read_events);
    HG_WL_FUNC(wl_display_cancel_read);
    HG_WL_FUNC(wl_proxy_marshal_flags);
    HG_WL_FUNC(wl_proxy_add_listener);
    HG_WL_FUNC(wl_proxy_destroy);
    HG_WL_FUNC(wl_proxy_get_user_data);
    HG_WL_FUNC(wl_proxy_set_user_data);
    HG_WL_FUNC(wl_proxy_get_version);
};

#undef HG_WL_FUNC

static WaylandFuncs wlFuncs{};

#include "wayland-client-protocol.h"
#include "xdg-shell-client-protocol.h"

namespace hg::wayland {

static u64 surfaceKey(struct wl_surface* surface)
{
    return reinterpret_cast<u64>(surface);
}

#define HG_WL_FUNC(name) decltype(&::name) name = nullptr

struct WaylandEglFuncs {
    HG_WL_FUNC(wl_egl_window_create);
    HG_WL_FUNC(wl_egl_window_destroy);
    HG_WL_FUNC(wl_egl_window_resize);
};

struct XkbFuncs {
    HG_WL_FUNC(xkb_context_new);
    HG_WL_FUNC(xkb_context_unref);
    HG_WL_FUNC(xkb_keymap_new_from_names);
    HG_WL_FUNC(xkb_keymap_new_from_string);
    HG_WL_FUNC(xkb_keymap_unref);
    HG_WL_FUNC(xkb_state_new);
    HG_WL_FUNC(xkb_state_unref);
    HG_WL_FUNC(xkb_state_key_get_one_sym);
    HG_WL_FUNC(xkb_state_key_get_utf8);
    HG_WL_FUNC(xkb_state_update_mask);
};

struct EvdevFuncs {
    HG_WL_FUNC(libevdev_new_from_fd);
    HG_WL_FUNC(libevdev_free);
    HG_WL_FUNC(libevdev_get_name);
    HG_WL_FUNC(libevdev_get_id_vendor);
    HG_WL_FUNC(libevdev_get_id_product);
    HG_WL_FUNC(libevdev_has_event_type);
    HG_WL_FUNC(libevdev_has_event_code);
    HG_WL_FUNC(libevdev_next_event);
};

#undef HG_WL_FUNC

static Library libWayland{};

static Library libWaylandEgl{};
static WaylandEglFuncs wlEglFuncs{};

static Library libxkb{};
static XkbFuncs xkbFuncs{};

static Library libevdevLib{};
static EvdevFuncs evdevFuncs{};

static bool loadLibwayland()
{
    Maybe<Library> lib = Library::load("libwayland-client.so.0");
    if (!lib.has)
    {
        setError("Could not load libwayland-client");
        return false;
    }
    libWayland = std::move(*lib);

#define HG_LOAD_WL(name) \
    *(void**)&wlFuncs.name = libWayland.loadSymbol(#name).orElse(nullptr); \
    if (wlFuncs.name == nullptr) { setError("Could not load " #name); return false; }

    HG_LOAD_WL(wl_display_connect);
    HG_LOAD_WL(wl_display_disconnect);
    HG_LOAD_WL(wl_display_get_fd);
    HG_LOAD_WL(wl_display_dispatch);
    HG_LOAD_WL(wl_display_dispatch_pending);
    HG_LOAD_WL(wl_display_flush);
    HG_LOAD_WL(wl_display_roundtrip);
    HG_LOAD_WL(wl_display_get_error);
    HG_LOAD_WL(wl_display_prepare_read);
    HG_LOAD_WL(wl_display_read_events);
    HG_LOAD_WL(wl_display_cancel_read);
    HG_LOAD_WL(wl_proxy_marshal_flags);
    HG_LOAD_WL(wl_proxy_add_listener);
    HG_LOAD_WL(wl_proxy_destroy);
    HG_LOAD_WL(wl_proxy_get_user_data);
    HG_LOAD_WL(wl_proxy_set_user_data);
    HG_LOAD_WL(wl_proxy_get_version);

#undef HG_LOAD_WL

    return true;
}

static bool loadLibwaylandEgl()
{
    Maybe<Library> lib = Library::load("libwayland-egl.so.1");
    if (!lib.has)
    {
        setError("Could not load libwayland-egl");
        return false;
    }
    libWaylandEgl = std::move(*lib);

#define HG_LOAD_WL_EGL(name) \
    *(void**)&wlEglFuncs.name = libWaylandEgl.loadSymbol(#name).orElse(nullptr); \
    if (wlEglFuncs.name == nullptr) { setError("Could not load " #name); return false; }

    HG_LOAD_WL_EGL(wl_egl_window_create);
    HG_LOAD_WL_EGL(wl_egl_window_destroy);
    HG_LOAD_WL_EGL(wl_egl_window_resize);

#undef HG_LOAD_WL_EGL

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
    HG_LOAD_XKB(xkb_keymap_new_from_string);
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

bool loadWayland()
{
    if (!loadLibwayland())
        return false;
    if (!loadLibwaylandEgl())
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
    extBuffer[1] = "VK_KHR_wayland_surface";
    return extBuffer;
}

struct WindowData {
    GpuSwapchain swap{};

    struct wl_surface* wlSurface = nullptr;
    struct xdg_surface* xdgSurface = nullptr;
    struct xdg_toplevel* xdgToplevel = nullptr;
    struct wl_egl_window* eglWindow = nullptr;

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
    i32 axes[6]{};
};

struct WindowState {
    struct wl_display* display = nullptr;
    struct wl_compositor* compositor = nullptr;
    struct wl_shm* shm = nullptr;
    struct xdg_wm_base* xdgWmBase = nullptr;
    struct wl_seat* seat = nullptr;
    struct wl_keyboard* keyboard = nullptr;
    struct wl_pointer* pointer = nullptr;
    struct wl_data_device_manager* dataDeviceManager = nullptr;
    struct wl_data_device* dataDevice = nullptr;
    struct wl_data_source* dataSource = nullptr;

    Array<DisplayInfo> displays{};

    Array<Event> events{};
    bool wasQuit = false;
    bool isKeyDown[Button_count]{};
    bool wasKeyDown[Button_count]{};
    Vec2 mouseDelta{};
    Vec2 wheelDelta{};

    Map<u64, WindowData*> windows{};
    WindowData* activeWindow = nullptr;

    Array<char> clipboard{};
    bool clipboardOwned = false;

    // Incoming clipboard transfer
    struct wl_data_offer* pendingOffer = nullptr;
    int clipboardFd = -1;
    bool clipboardHasText = false;
    Array<char> clipboardIncoming{};

    struct xkb_context* xkbContext = nullptr;
    struct xkb_keymap* xkbKeymap = nullptr;
    struct xkb_state* xkbState = nullptr;
    u32 xkbModifiers = 0;

    u32 pointerEnterSerial = 0;
    u32 lastSerial = 0;
    struct wl_surface* pointerFocus = nullptr;

    // Output tracking
    struct wl_output* outputs[16]{};
    u32 outputCount = 0;

    GamepadState gamepads[maxGamepads]{};
    u32 gamepadCount = 0;

    bool globalsReady = false;
};

static WindowState windowState{};

// Button mapping
static Button xkbKeysymToButton(uint32_t keysym)
{
    switch (keysym)
    {
        case XKB_KEY_Escape: return Button_escape;
        case XKB_KEY_Return: return Button_enter;
        case XKB_KEY_Tab: return Button_tab;
        case XKB_KEY_BackSpace: return Button_backspace;
        case XKB_KEY_space: return Button_space;

        case XKB_KEY_Caps_Lock: return Button_capslock;
        case XKB_KEY_Num_Lock: return Button_numlock;
        case XKB_KEY_Scroll_Lock: return Button_scrolllock;

        case XKB_KEY_Control_L: return Button_lctrl;
        case XKB_KEY_Control_R: return Button_rctrl;
        case XKB_KEY_Shift_L: return Button_lshift;
        case XKB_KEY_Shift_R: return Button_rshift;
        case XKB_KEY_Alt_L: return Button_lalt;
        case XKB_KEY_Alt_R: return Button_ralt;
        case XKB_KEY_Super_L: return Button_lsuper;
        case XKB_KEY_Super_R: return Button_rsuper;

        case XKB_KEY_Up: return Button_up;
        case XKB_KEY_Down: return Button_down;
        case XKB_KEY_Left: return Button_left;
        case XKB_KEY_Right: return Button_right;

        case XKB_KEY_F1: return Button_f1;
        case XKB_KEY_F2: return Button_f2;
        case XKB_KEY_F3: return Button_f3;
        case XKB_KEY_F4: return Button_f4;
        case XKB_KEY_F5: return Button_f5;
        case XKB_KEY_F6: return Button_f6;
        case XKB_KEY_F7: return Button_f7;
        case XKB_KEY_F8: return Button_f8;
        case XKB_KEY_F9: return Button_f9;
        case XKB_KEY_F10: return Button_f10;
        case XKB_KEY_F11: return Button_f11;
        case XKB_KEY_F12: return Button_f12;

        case XKB_KEY_a: return Button_a;
        case XKB_KEY_b: return Button_b;
        case XKB_KEY_c: return Button_c;
        case XKB_KEY_d: return Button_d;
        case XKB_KEY_e: return Button_e;
        case XKB_KEY_f: return Button_f;
        case XKB_KEY_g: return Button_g;
        case XKB_KEY_h: return Button_h;
        case XKB_KEY_i: return Button_i;
        case XKB_KEY_j: return Button_j;
        case XKB_KEY_k: return Button_k;
        case XKB_KEY_l: return Button_l;
        case XKB_KEY_m: return Button_m;
        case XKB_KEY_n: return Button_n;
        case XKB_KEY_o: return Button_o;
        case XKB_KEY_p: return Button_p;
        case XKB_KEY_q: return Button_q;
        case XKB_KEY_r: return Button_r;
        case XKB_KEY_s: return Button_s;
        case XKB_KEY_t: return Button_t;
        case XKB_KEY_u: return Button_u;
        case XKB_KEY_v: return Button_v;
        case XKB_KEY_w: return Button_w;
        case XKB_KEY_x: return Button_x;
        case XKB_KEY_y: return Button_y;
        case XKB_KEY_z: return Button_z;

        case XKB_KEY_0: return Button_0;
        case XKB_KEY_1: return Button_1;
        case XKB_KEY_2: return Button_2;
        case XKB_KEY_3: return Button_3;
        case XKB_KEY_4: return Button_4;
        case XKB_KEY_5: return Button_5;
        case XKB_KEY_6: return Button_6;
        case XKB_KEY_7: return Button_7;
        case XKB_KEY_8: return Button_8;
        case XKB_KEY_9: return Button_9;

        case XKB_KEY_minus: return Button_minus;
        case XKB_KEY_equal: return Button_equal;
        case XKB_KEY_bracketleft: return Button_lbracket;
        case XKB_KEY_bracketright: return Button_rbracket;
        case XKB_KEY_backslash: return Button_backslash;
        case XKB_KEY_semicolon: return Button_semicolon;
        case XKB_KEY_apostrophe: return Button_apostrophe;
        case XKB_KEY_grave: return Button_grave;
        case XKB_KEY_comma: return Button_comma;
        case XKB_KEY_period: return Button_period;
        case XKB_KEY_slash: return Button_slash;

        case XKB_KEY_Home: return Button_home;
        case XKB_KEY_End: return Button_end;
        case XKB_KEY_Page_Up: return Button_pageup;
        case XKB_KEY_Page_Down: return Button_pagedown;
        case XKB_KEY_Delete: return Button_kdelete;
        case XKB_KEY_Insert: return Button_insert;
        case XKB_KEY_Print: return Button_printscreen;
        case XKB_KEY_Pause: return Button_pause;
        case XKB_KEY_Menu: return Button_context;

        case XKB_KEY_KP_0: return Button_numpad0;
        case XKB_KEY_KP_1: return Button_numpad1;
        case XKB_KEY_KP_2: return Button_numpad2;
        case XKB_KEY_KP_3: return Button_numpad3;
        case XKB_KEY_KP_4: return Button_numpad4;
        case XKB_KEY_KP_5: return Button_numpad5;
        case XKB_KEY_KP_6: return Button_numpad6;
        case XKB_KEY_KP_7: return Button_numpad7;
        case XKB_KEY_KP_8: return Button_numpad8;
        case XKB_KEY_KP_9: return Button_numpad9;
        case XKB_KEY_KP_Decimal: return Button_numpaddecimal;
        case XKB_KEY_KP_Divide: return Button_numpaddiv;
        case XKB_KEY_KP_Multiply: return Button_numpadmul;
        case XKB_KEY_KP_Subtract: return Button_numpadminus;
        case XKB_KEY_KP_Add: return Button_numpadplus;
        case XKB_KEY_KP_Enter: return Button_numpadenter;

        default: return Button_count;
    }
}

static Button pointerButtonToHgButton(uint32_t button)
{
    switch (button)
    {
        case BTN_LEFT: return Button_mouse1;
        case BTN_RIGHT: return Button_mouse2;
        case BTN_MIDDLE: return Button_mouse3;
        case BTN_SIDE: return Button_mouse4;
        case BTN_EXTRA: return Button_mouse5;
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

// ---- Wayland protocol callbacks ----

static void keyboardKeymap(void* data, struct wl_keyboard* keyboard,
    uint32_t format, int fd, uint32_t size)
{
    (void)data;
    (void)keyboard;
    (void)format;


    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
        close(fd);
        return;
    }

    char* mapStr = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
    if (mapStr == MAP_FAILED)
    {
        close(fd);
        return;
    }

    if (windowState.xkbKeymap != nullptr)
        xkbFuncs.xkb_keymap_unref(windowState.xkbKeymap);
    if (windowState.xkbState != nullptr)
        xkbFuncs.xkb_state_unref(windowState.xkbState);

    windowState.xkbKeymap = xkbFuncs.xkb_keymap_new_from_string(
        windowState.xkbContext, mapStr, XKB_KEYMAP_FORMAT_TEXT_V1,
        static_cast<xkb_keymap_compile_flags>(0));

    munmap(mapStr, size);
    close(fd);

    if (windowState.xkbKeymap == nullptr)
    {
        return;
    }

    windowState.xkbState = xkbFuncs.xkb_state_new(windowState.xkbKeymap);
}

static void keyboardEnter(void* data, struct wl_keyboard* keyboard,
    uint32_t serial, struct wl_surface* surface, struct wl_array* keys)
{
    (void)data;
    (void)keyboard;
    (void)keys;


    windowState.lastSerial = serial;

    if (surface == nullptr)
        return;

    WindowData** found = windowState.windows.get(surfaceKey(surface));
    if (found != nullptr)
    {
        (*found)->isFocused = true;
        (*found)->wasFocusGained = true;
        windowState.activeWindow = *found;

        Event event{};
        event.type = EventType_windowFocused;
        windowState.events.push(event);
        (*found)->events.push(event);
    }
}

static void keyboardLeave(void* data, struct wl_keyboard* keyboard,
    uint32_t serial, struct wl_surface* surface)
{
    (void)data;
    (void)keyboard;


    windowState.lastSerial = serial;

    if (surface == nullptr)
        return;

    WindowData** found = windowState.windows.get(surfaceKey(surface));
    if (found != nullptr)
    {
        (*found)->isFocused = false;
        (*found)->wasFocusLost = true;
        if (windowState.activeWindow == *found)
            windowState.activeWindow = nullptr;

        Event event{};
        event.type = EventType_windowUnfocused;
        windowState.events.push(event);
        (*found)->events.push(event);
    }
}

static void keyboardKey(void* data, struct wl_keyboard* keyboard,
    uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    (void)data;
    (void)keyboard;
    (void)time;


    windowState.lastSerial = serial;

    bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

    xkb_keysym_t keysym = xkbFuncs.xkb_state_key_get_one_sym(windowState.xkbState, key + 8);
    Button button = xkbKeysymToButton(keysym);

    if (button < Button_count)
    {
        windowState.isKeyDown[button] = pressed;

        Event event{};
        event.type = pressed ? EventType_keyPress : EventType_keyRelease;
        event.button = button;
        windowState.events.push(event);
    }

    if (pressed && windowState.xkbState != nullptr)
    {
        char buf[32];
        int len = xkbFuncs.xkb_state_key_get_utf8(windowState.xkbState, key + 8, buf, sizeof(buf) - 1);
        if (len > 0)
        {
            buf[len] = '\0';
            Event event{};
            event.type = EventType_text;
            memset(event.text, 0, sizeof(event.text));
            memcpy(event.text, buf, static_cast<u64>(len));
            windowState.events.push(event);
        }
    }
}

static void keyboardModifiers(void* data, struct wl_keyboard* keyboard,
    uint32_t serial, uint32_t modsDepressed, uint32_t modsLatched,
    uint32_t modsLocked, uint32_t group)
{
    (void)data;
    (void)keyboard;
    (void)serial;

    xkbFuncs.xkb_state_update_mask(windowState.xkbState,
        modsDepressed, modsLatched, modsLocked, 0, 0, group);
    windowState.xkbModifiers = modsDepressed;
}

static void keyboardRepeatInfo(void* data, struct wl_keyboard* keyboard,
    int32_t rate, int32_t delay)
{
    (void)data;
    (void)keyboard;
    (void)rate;
    (void)delay;
}

static const struct wl_keyboard_listener keyboardListener = {
    .keymap = keyboardKeymap,
    .enter = keyboardEnter,
    .leave = keyboardLeave,
    .key = keyboardKey,
    .modifiers = keyboardModifiers,
    .repeat_info = keyboardRepeatInfo,
};
static void pointerEnter(void* data, struct wl_pointer* pointer,
    uint32_t serial, struct wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy)
{
    (void)data;
    (void)pointer;


    windowState.lastSerial = serial;
    windowState.pointerEnterSerial = serial;
    windowState.pointerFocus = surface;

    if (surface == nullptr)
        return;

    WindowData** found = windowState.windows.get(surfaceKey(surface));
    if (found != nullptr)
    {
        (*found)->mouse.x = static_cast<f32>(wl_fixed_to_double(sx));
        (*found)->mouse.y = static_cast<f32>(wl_fixed_to_double(sy));
    }
}

static void pointerLeave(void* data, struct wl_pointer* pointer,
    uint32_t serial, struct wl_surface* surface)
{
    (void)data;
    (void)pointer;

    windowState.lastSerial = serial;

    if (windowState.pointerFocus == surface)
        windowState.pointerFocus = nullptr;
}

static void pointerMotion(void* data, struct wl_pointer* pointer,
    uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    (void)data;
    (void)pointer;
    (void)time;

    if (windowState.pointerFocus == nullptr)
        return;

    WindowData** found = windowState.windows.get(surfaceKey(windowState.pointerFocus));
    if (found != nullptr)
    {
        Vec2 newPos{static_cast<f32>(wl_fixed_to_double(sx)), static_cast<f32>(wl_fixed_to_double(sy))};
        Vec2 delta = newPos - (*found)->mouse;
        (*found)->mouse = newPos;
        windowState.mouseDelta += delta;

        Event event{};
        event.type = EventType_mouseMoved;
        event.mouse.delta = delta;
        event.mouse.pos = newPos;
        event.mouse.globalPos = newPos;
        windowState.events.push(event);
    }
}

static void pointerButton(void* data, struct wl_pointer* pointer,
    uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    (void)data;
    (void)pointer;
    (void)time;

    windowState.lastSerial = serial;

    bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
    Button hgButton = pointerButtonToHgButton(button);

    if (hgButton < Button_count)
    {
        windowState.isKeyDown[hgButton] = pressed;

        Event event{};
        event.type = pressed ? EventType_keyPress : EventType_keyRelease;
        event.button = hgButton;
        windowState.events.push(event);
    }
}

static void pointerAxis(void* data, struct wl_pointer* pointer,
    uint32_t time, uint32_t axis, wl_fixed_t value)
{
    (void)data;
    (void)pointer;
    (void)time;

    Vec2 delta{};
    f32 amount = static_cast<f32>(wl_fixed_to_double(value));

    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        delta.y = amount;
    else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        delta.x = amount;

    if (delta.x != 0.0f || delta.y != 0.0f)
    {
        windowState.wheelDelta += delta;

        Event event{};
        event.type = EventType_wheelMoved;
        event.wheel.delta = delta;
        windowState.events.push(event);
    }
}

static const struct wl_pointer_listener pointerListener = {
    .enter = pointerEnter,
    .leave = pointerLeave,
    .motion = pointerMotion,
    .button = pointerButton,
    .axis = pointerAxis,
    .frame = [](void*, struct wl_pointer*) {},
    .axis_source = [](void*, struct wl_pointer*, uint32_t) {},
    .axis_stop = [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
    .axis_discrete = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
    .axis_value120 = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
    .axis_relative_direction = [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
    .warp = [](void*, struct wl_pointer*, wl_fixed_t, wl_fixed_t) {},
};
static void seatCapabilities(void* data, struct wl_seat* seat, uint32_t capabilities)
{
    (void)data;

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && windowState.keyboard == nullptr)
    {
        windowState.keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(windowState.keyboard, &keyboardListener, nullptr);
    }
    else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && windowState.keyboard != nullptr)
    {
        wl_keyboard_destroy(windowState.keyboard);
        windowState.keyboard = nullptr;
    }

    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && windowState.pointer == nullptr)
    {
        windowState.pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(windowState.pointer, &pointerListener, nullptr);
    }
    else if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && windowState.pointer != nullptr)
    {
        wl_pointer_destroy(windowState.pointer);
        windowState.pointer = nullptr;
    }
}

static void seatName(void* data, struct wl_seat* seat, const char* name)
{
    (void)data;
    (void)seat;
    (void)name;
}

static const struct wl_seat_listener seatListener = {
    .capabilities = seatCapabilities,
    .name = seatName,
};

// wl_keyboard


// wl_pointer

static void outputGeometry(void* data, struct wl_output* output,
    int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight,
    int32_t subpixel, const char* make, const char* model, int32_t transform)
{
    (void)data;
    (void)output;
    (void)physicalWidth;
    (void)physicalHeight;
    (void)subpixel;
    (void)make;
    (void)model;
    (void)transform;

    for (u32 i = 0; i < windowState.outputCount; i++)
    {
        if (windowState.outputs[i] == output)
        {
            if (windowState.displays.count <= i)
                windowState.displays.resize(static_cast<u64>(i + 1));
            windowState.displays[i].posX = x;
            windowState.displays[i].posY = y;
            break;
        }
    }
}

static void outputMode(void* data, struct wl_output* output,
    uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
    (void)data;
    (void)refresh;

    if (!(flags & WL_OUTPUT_MODE_CURRENT))
        return;

    for (u32 i = 0; i < windowState.outputCount; i++)
    {
        if (windowState.outputs[i] == output)
        {
            if (windowState.displays.count <= i)
                windowState.displays.resize(static_cast<u64>(i + 1));
            windowState.displays[i].sizeW = static_cast<u32>(width);
            windowState.displays[i].sizeH = static_cast<u32>(height);
            windowState.displays[i].workPosX = windowState.displays[i].posX;
            windowState.displays[i].workPosY = windowState.displays[i].posY;
            windowState.displays[i].workSizeW = windowState.displays[i].sizeW;
            windowState.displays[i].workSizeH = windowState.displays[i].sizeH;
            windowState.displays[i].dpiScale = 1.0f;
            break;
        }
    }
}

static void outputDone(void* data, struct wl_output* output)
{
    (void)data;
    (void)output;
}

static void outputScale(void* data, struct wl_output* output, int32_t factor)
{
    (void)data;

    for (u32 i = 0; i < windowState.outputCount; i++)
    {
        if (windowState.outputs[i] == output)
        {
            if (windowState.displays.count <= i)
                windowState.displays.resize(static_cast<u64>(i + 1));
            windowState.displays[i].dpiScale = static_cast<f32>(factor);
            break;
        }
    }
}

static void outputName(void* data, struct wl_output* output, const char* name)
{
    (void)data;
    (void)output;
    (void)name;
}

static void outputDescription(void* data, struct wl_output* output, const char* description)
{
    (void)data;
    (void)output;
    (void)description;
}

static const struct wl_output_listener outputListener = {
    .geometry = outputGeometry,
    .mode = outputMode,
    .done = outputDone,
    .scale = outputScale,
    .name = outputName,
    .description = outputDescription,
};
static void registryGlobal(void* data, struct wl_registry* registry,
    uint32_t name, const char* interface, uint32_t version)
{
    (void)data;

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        windowState.compositor = reinterpret_cast<struct wl_compositor*>(
            wlFuncs.wl_proxy_marshal_flags(
                reinterpret_cast<struct wl_proxy*>(registry),
                WL_REGISTRY_BIND, &wl_compositor_interface, version, 0,
                name, wl_compositor_interface.name, version, 0));
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        windowState.shm = reinterpret_cast<struct wl_shm*>(
            wlFuncs.wl_proxy_marshal_flags(
                reinterpret_cast<struct wl_proxy*>(registry),
                WL_REGISTRY_BIND, &wl_shm_interface, version, 0,
                name, wl_shm_interface.name, version, 0));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        windowState.xdgWmBase = reinterpret_cast<struct xdg_wm_base*>(
            wlFuncs.wl_proxy_marshal_flags(
                reinterpret_cast<struct wl_proxy*>(registry),
                WL_REGISTRY_BIND, &xdg_wm_base_interface, version, 0,
                name, xdg_wm_base_interface.name, version, 0));
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        windowState.seat = reinterpret_cast<struct wl_seat*>(
            wlFuncs.wl_proxy_marshal_flags(
                reinterpret_cast<struct wl_proxy*>(registry),
                WL_REGISTRY_BIND, &wl_seat_interface, version, 0,
                name, wl_seat_interface.name, version, 0));
        wl_seat_add_listener(windowState.seat, &seatListener, nullptr);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
        if (windowState.outputCount < 16)
        {
            struct wl_output* output = reinterpret_cast<struct wl_output*>(
                wlFuncs.wl_proxy_marshal_flags(
                    reinterpret_cast<struct wl_proxy*>(registry),
                    WL_REGISTRY_BIND, &wl_output_interface, version, 0,
                    name, wl_output_interface.name, version, 0));
            windowState.outputs[windowState.outputCount++] = output;
            wl_output_add_listener(output, &outputListener, nullptr);
        }
    }
    else if (strcmp(interface, wl_data_device_manager_interface.name) == 0)
    {
        windowState.dataDeviceManager = reinterpret_cast<struct wl_data_device_manager*>(
            wlFuncs.wl_proxy_marshal_flags(
                reinterpret_cast<struct wl_proxy*>(registry),
                WL_REGISTRY_BIND, &wl_data_device_manager_interface, version, 0,
                name, wl_data_device_manager_interface.name, version, 0));
    }
}

static void registryGlobalRemove(void* data, struct wl_registry* registry, uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

static struct wl_registry_listener registryListener = {
    .global = registryGlobal,
    .global_remove = registryGlobalRemove,
};

// xdg_wm_base

static void xdgWmBasePing(void* data, struct xdg_wm_base* xdgWmBase, uint32_t serial)
{
    (void)data;
    xdg_wm_base_pong(xdgWmBase, serial);
}

static const struct xdg_wm_base_listener xdgWmBaseListener = {
    .ping = xdgWmBasePing,
};

// wl_seat


// wl_output


// xdg_toplevel

static void xdgToplevelConfigure(void* data, struct xdg_toplevel* toplevel,
    int32_t width, int32_t height, struct wl_array* states)
{
    (void)toplevel;
    (void)states;

    WindowData* window = static_cast<WindowData*>(data);
    if (window == nullptr)
        return;

    if (width > 0 && height > 0)
    {
        u32 newW = static_cast<u32>(width);
        u32 newH = static_cast<u32>(height);

        if (newW != window->width || newH != window->height)
        {
            window->wasResized = true;
            window->width = newW;
            window->height = newH;

            if (window->eglWindow != nullptr)
                wlEglFuncs.wl_egl_window_resize(window->eglWindow, width, height, 0, 0);

            if (window->swap.data != nullptr)
                window->swap.resize(newW, newH);

            Event event{};
            event.type = EventType_windowResized;
            event.window.window = window;
            event.window.width = newW;
            event.window.height = newH;
            windowState.events.push(event);
            window->events.push(event);
        }
    }
}

static void xdgToplevelClose(void* data, struct xdg_toplevel* toplevel)
{
    (void)toplevel;

    WindowData* window = static_cast<WindowData*>(data);
    if (window == nullptr)
        return;

    window->wasClosed = true;

    Event event{};
    event.type = EventType_windowClosed;
    windowState.events.push(event);
    window->events.push(event);
}

static void xdgToplevelConfigureBounds(void* data, struct xdg_toplevel* toplevel,
    int32_t width, int32_t height)
{
    (void)data;
    (void)toplevel;
    (void)width;
    (void)height;
}

static void xdgToplevelWmCapabilities(void* data, struct xdg_toplevel* toplevel,
    struct wl_array* capabilities)
{
    (void)data;
    (void)toplevel;
    (void)capabilities;
}

static const struct xdg_toplevel_listener xdgToplevelListener = {
    .configure = xdgToplevelConfigure,
    .close = xdgToplevelClose,
    .configure_bounds = xdgToplevelConfigureBounds,
    .wm_capabilities = xdgToplevelWmCapabilities,
};

// xdg_surface

static void xdgSurfaceConfigure(void* data, struct xdg_surface* xdgSurface, uint32_t serial)
{
    (void)data;
    xdg_surface_ack_configure(xdgSurface, serial);
}

static const struct xdg_surface_listener xdgSurfaceListener = {
    .configure = xdgSurfaceConfigure,
};

// wl_data_device

static void dataOfferOffer(void* data, struct wl_data_offer* offer, const char* mimeType)
{
    (void)data;
    (void)offer;

    if (mimeType != nullptr &&
        (strcmp(mimeType, "text/plain") == 0 || strcmp(mimeType, "text/plain;charset=utf-8") == 0))
        windowState.clipboardHasText = true;
}

static const struct wl_data_offer_listener dataOfferListener = {
    .offer = dataOfferOffer,
    .source_actions = [](void*, struct wl_data_offer*, uint32_t) {},
    .action = [](void*, struct wl_data_offer*, uint32_t) {},
};

static void dataDeviceDataOffer(void* data, struct wl_data_device* device,
    struct wl_data_offer* offer)
{
    (void)data;
    (void)device;

    if (offer != nullptr)
    {
        windowState.clipboardHasText = false;
        wl_data_offer_add_listener(offer, &dataOfferListener, nullptr);
    }
}

static void dataDeviceEnter(void* data, struct wl_data_device* device,
    uint32_t serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y,
    struct wl_data_offer* offer)
{
    (void)data;
    (void)device;
    (void)serial;
    (void)surface;
    (void)x;
    (void)y;
    (void)offer;
}

static void dataDeviceMotion(void* data, struct wl_data_device* device,
    uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    (void)data;
    (void)device;
    (void)time;
    (void)x;
    (void)y;
}

static void dataDeviceLeave(void* data, struct wl_data_device* device)
{
    (void)data;
    (void)device;
}

static void dataDeviceDrop(void* data, struct wl_data_device* device)
{
    (void)data;
    (void)device;
}

static void dataDeviceSelection(void* data, struct wl_data_device* device,
    struct wl_data_offer* offer)
{
    (void)data;
    (void)device;

    if (windowState.pendingOffer != nullptr && windowState.pendingOffer != offer)
    {
        wl_data_offer_destroy(windowState.pendingOffer);
        windowState.pendingOffer = nullptr;
    }

    if (offer == nullptr)
        return;

    windowState.pendingOffer = offer;
    if (!windowState.clipboardHasText)
        return;

    wl_data_offer_accept(offer, windowState.lastSerial, "text/plain;charset=utf-8");

    if (windowState.clipboardFd >= 0)
    {
        close(windowState.clipboardFd);
        windowState.clipboardFd = -1;
    }
    windowState.clipboardIncoming.resize(0);

    windowState.clipboardFd = memfd_create("hurdygurdy-clipboard", 0);
    if (windowState.clipboardFd < 0)
        return;

    wl_data_offer_receive(offer, "text/plain;charset=utf-8", windowState.clipboardFd);
}

static const struct wl_data_device_listener dataDeviceListener = {
    .data_offer = dataDeviceDataOffer,
    .enter = dataDeviceEnter,
    .leave = dataDeviceLeave,
    .motion = dataDeviceMotion,
    .drop = dataDeviceDrop,
    .selection = dataDeviceSelection,
};

// wl_data_source (our clipboard, served to other apps)

static void dataSourceSend(void* data, struct wl_data_source* source,
    const char* mimeType, int32_t fd)
{
    (void)data;
    (void)source;
    (void)mimeType;

    if (windowState.clipboard.count > 0)
    {
        ssize_t written = write(fd, windowState.clipboard.vals, windowState.clipboard.count);
        (void)written;
    }
    close(fd);
}

static void dataSourceCancelled(void* data, struct wl_data_source* source)
{
    (void)data;
    (void)source;
    windowState.clipboardOwned = false;
}

static const struct wl_data_source_listener dataSourceListener = {
    .target = [](void*, struct wl_data_source*, const char*) {},
    .send = dataSourceSend,
    .cancelled = dataSourceCancelled,
    .dnd_drop_performed = [](void*, struct wl_data_source*) {},
    .dnd_finished = [](void*, struct wl_data_source*) {},
    .action = [](void*, struct wl_data_source*, uint32_t) {},
};

// ---- Gamepad handling (same as X11) ----

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
    static u32 lastPollFrame = 0;
    static u32 pollCounter = 0;
    ++pollCounter;
    if (pollCounter - lastPollFrame < 60)
        return;
    lastPollFrame = pollCounter;

    for (u32 i = 0; i < windowState.gamepadCount;)
    {
        GamepadState& gamepad = windowState.gamepads[i];
        struct stat st;
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);

        if (stat(path, &st) != 0)
        {
            if (gamepad.dev != nullptr)
                evdevFuncs.libevdev_free(gamepad.dev);
            if (gamepad.fd >= 0)
                close(gamepad.fd);

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

// ---- Lifecycle ----

bool initWayland()
{
    windowState = WindowState{};

    windowState.display = wlFuncs.wl_display_connect(nullptr);
    if (windowState.display == nullptr)
    {
        setError("Could not connect to Wayland display");
        return false;
    }

    windowState.xkbContext = xkbFuncs.xkb_context_new(static_cast<xkb_context_flags>(0));
    if (windowState.xkbContext == nullptr)
    {
        setError("Could not create XKB context");
        wlFuncs.wl_display_disconnect(windowState.display);
        return false;
    }

    struct wl_registry* registry = wl_display_get_registry(windowState.display);
    struct wl_registry_listener* listenerPtr = &registryListener;
    wlFuncs.wl_proxy_add_listener(
        reinterpret_cast<struct wl_proxy*>(registry),
        reinterpret_cast<void(**)(void)>(listenerPtr), nullptr);

    wlFuncs.wl_display_roundtrip(windowState.display);
    wlFuncs.wl_display_roundtrip(windowState.display);

    if (windowState.compositor == nullptr)
    {
        setError("Wayland compositor not available");
        wlFuncs.wl_display_disconnect(windowState.display);
        return false;
    }

    if (windowState.xdgWmBase != nullptr)
        xdg_wm_base_add_listener(windowState.xdgWmBase, &xdgWmBaseListener, nullptr);

    wlFuncs.wl_display_roundtrip(windowState.display);

    if (windowState.dataDeviceManager != nullptr)
    {
        windowState.dataDevice = wl_data_device_manager_get_data_device(
            windowState.dataDeviceManager, windowState.seat);
        if (windowState.dataDevice != nullptr)
            wl_data_device_add_listener(windowState.dataDevice, &dataDeviceListener, nullptr);
    }

    scanForGamepads();
    windowState.globalsReady = true;

    return true;
}

void deinitWayland()
{
    closeGamepads();

    if (windowState.clipboardFd >= 0)
    {
        close(windowState.clipboardFd);
        windowState.clipboardFd = -1;
    }
    if (windowState.pendingOffer != nullptr)
    {
        wl_data_offer_destroy(windowState.pendingOffer);
        windowState.pendingOffer = nullptr;
    }
    if (windowState.dataSource != nullptr)
    {
        wl_data_source_destroy(windowState.dataSource);
        windowState.dataSource = nullptr;
    }

    if (windowState.xkbState != nullptr)
        xkbFuncs.xkb_state_unref(windowState.xkbState);
    if (windowState.xkbKeymap != nullptr)
        xkbFuncs.xkb_keymap_unref(windowState.xkbKeymap);
    if (windowState.xkbContext != nullptr)
        xkbFuncs.xkb_context_unref(windowState.xkbContext);

    windowState.windows.forEach([](u64, WindowData* window)
    {
        window->~WindowData();
    });

    if (windowState.dataDevice != nullptr)
        wl_data_device_destroy(windowState.dataDevice);
    if (windowState.dataDeviceManager != nullptr)
        wl_data_device_manager_destroy(windowState.dataDeviceManager);

    if (windowState.keyboard != nullptr)
        wl_keyboard_destroy(windowState.keyboard);
    if (windowState.pointer != nullptr)
        wl_pointer_destroy(windowState.pointer);
    if (windowState.seat != nullptr)
        wl_seat_destroy(windowState.seat);

    for (u32 i = 0; i < windowState.outputCount; i++)
    {
        if (windowState.outputs[i] != nullptr)
            wl_output_destroy(windowState.outputs[i]);
    }

    if (windowState.xdgWmBase != nullptr)
        xdg_wm_base_destroy(windowState.xdgWmBase);
    if (windowState.shm != nullptr)
        wl_shm_destroy(windowState.shm);
    if (windowState.compositor != nullptr)
        wl_compositor_destroy(windowState.compositor);

    if (windowState.display != nullptr)
        wlFuncs.wl_display_disconnect(windowState.display);
}

WindowData::~WindowData() noexcept
{
    if (wlSurface != nullptr)
    {
        windowState.windows.remove(surfaceKey(wlSurface));

        if (xdgToplevel != nullptr)
            xdg_toplevel_destroy(xdgToplevel);
        if (xdgSurface != nullptr)
            xdg_surface_destroy(xdgSurface);
        if (eglWindow != nullptr)
            wlEglFuncs.wl_egl_window_destroy(eglWindow);
        if (wlSurface != nullptr)
            wl_surface_destroy(wlSurface);
    }
}

WindowData::WindowData(WindowData&& other) noexcept
    : swap{std::exchange(other.swap, GpuSwapchain{})}
    , wlSurface{std::exchange(other.wlSurface, nullptr)}
    , xdgSurface{std::exchange(other.xdgSurface, nullptr)}
    , xdgToplevel{std::exchange(other.xdgToplevel, nullptr)}
    , eglWindow{std::exchange(other.eglWindow, nullptr)}
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
        this->~WindowData();
        new (this) WindowData{std::move(other)};
    }
    return *this;
}

// ---- Public API ----

Span<DisplayInfo> displayInfo()
{
    return windowState.displays;
}

void setCursor(CursorType type)
{
    (void)type;
    // Wayland cursor handling would require wl_cursor_theme.
    // For now, let the compositor handle the default cursor.
}

void showCursor(bool show)
{
    (void)show;
    // Wayland cursor visibility is handled by the compositor.
}

static void readClipboard()
{
    if (windowState.clipboardFd < 0)
        return;

    char buf[4096];
    ssize_t n;
    bool eof = false;
    while ((n = read(windowState.clipboardFd, buf, sizeof(buf))) > 0)
    {
        u64 old = windowState.clipboardIncoming.count;
        windowState.clipboardIncoming.resize(old + static_cast<u64>(n));
        memcpy(windowState.clipboardIncoming.vals + old, buf, static_cast<u64>(n));
    }
    if (n == 0)
        eof = true;
    else if (errno != EAGAIN && errno != EWOULDBLOCK)
        eof = true;

    if (eof)
    {
        close(windowState.clipboardFd);
        windowState.clipboardFd = -1;

        windowState.clipboard.resize(windowState.clipboardIncoming.count);
        memcpy(windowState.clipboard.vals, windowState.clipboardIncoming.vals, windowState.clipboardIncoming.count);
        windowState.clipboardIncoming.resize(0);

        if (windowState.pendingOffer != nullptr)
        {
            wl_data_offer_finish(windowState.pendingOffer);
            wl_data_offer_destroy(windowState.pendingOffer);
            windowState.pendingOffer = nullptr;
        }
    }
}

void processEvents()
{
    windowState.events.reset();

    memcpy(windowState.wasKeyDown, windowState.isKeyDown, sizeof(windowState.isKeyDown));
    windowState.mouseDelta = {};
    windowState.wheelDelta = {};

    windowState.windows.forEach([](u64, WindowData* data)
    {
        data->events.reset();
        data->wasFocusGained = false;
        data->wasFocusLost = false;
        data->wasResized = false;
    });

    readGamepadEvents();
    pollGamepadDevices();

    if (wlFuncs.wl_display_get_error(windowState.display) != 0)
        return;

    while (wlFuncs.wl_display_prepare_read(windowState.display) != 0)
    {
        wlFuncs.wl_display_dispatch_pending(windowState.display);
        if (wlFuncs.wl_display_get_error(windowState.display) != 0)
            return;
    }

    wlFuncs.wl_display_flush(windowState.display);

    struct pollfd pfd{};
    pfd.fd = wlFuncs.wl_display_get_fd(windowState.display);
    pfd.events = POLLIN;

    if (poll(&pfd, 1, 0) > 0)
    {
        wlFuncs.wl_display_read_events(windowState.display);
        while (wlFuncs.wl_display_dispatch_pending(windowState.display) != 0)
        {
            if (wlFuncs.wl_display_get_error(windowState.display) != 0)
                return;
        }
    }
    else
    {
        wlFuncs.wl_display_cancel_read(windowState.display);
    }

    wlFuncs.wl_display_flush(windowState.display);

    readClipboard();
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
    return !windowState.wasKeyDown[key] && windowState.isKeyDown[key];
}

bool wasButtonReleased(Button key)
{
    return windowState.wasKeyDown[key] && !windowState.isKeyDown[key];
}

Vec2 mousePos()
{
    if (windowState.activeWindow != nullptr)
        return windowState.activeWindow->mouse;

    if (windowState.windows.count == 1)
    {
        Vec2 ret;
        windowState.windows.forEach([&](u64, WindowData* wd)
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
        windowState.windows.forEach([&](u64, WindowData* wd)
        {
            ret = ret / static_cast<f32>(wd->height);
        });
        return ret;
    }
    return windowState.mouseDelta;
}

Vec2 windowMousePos(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    return wd->mouse;
}

Vec2 windowMouseDelta(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    return windowState.mouseDelta / static_cast<f32>(wd->height);
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

Window windowCreate(const WindowConfig& config)
{
    Window window{};
    window.data = new (heapAlloc(sizeof(WindowData), alignof(WindowData))) WindowData{};

    WindowData* wd = static_cast<WindowData*>(window.data);

    wd->wlSurface = wl_compositor_create_surface(windowState.compositor);
    if (wd->wlSurface == nullptr)
    {
        wd->~WindowData();
        heapFree(wd, 1);
        setError("Could not create Wayland surface");
        return Window{};
    }

    wd->xdgSurface = xdg_wm_base_get_xdg_surface(windowState.xdgWmBase, wd->wlSurface);
    if (wd->xdgSurface == nullptr)
    {
        wl_surface_destroy(wd->wlSurface);
        wd->wlSurface = nullptr;
        wd->~WindowData();
        heapFree(wd, 1);
        setError("Could not create XDG surface");
        return Window{};
    }

    xdg_surface_add_listener(wd->xdgSurface, &xdgSurfaceListener, nullptr);

    wd->xdgToplevel = xdg_surface_get_toplevel(wd->xdgSurface);
    if (wd->xdgToplevel == nullptr)
    {
        xdg_surface_destroy(wd->xdgSurface);
        wl_surface_destroy(wd->wlSurface);
        wd->wlSurface = nullptr;
        wd->~WindowData();
        heapFree(wd, 1);
        setError("Could not create XDG toplevel");
        return Window{};
    }

    xdg_toplevel_add_listener(wd->xdgToplevel, &xdgToplevelListener, wd);
    xdg_toplevel_set_title(wd->xdgToplevel, "Hurdy Gurdy");

    wd->width = 800;
    wd->height = 600;

    wd->eglWindow = wlEglFuncs.wl_egl_window_create(wd->wlSurface, 800, 600);

    windowState.windows.add(surfaceKey(wd->wlSurface), wd);

    wl_surface_commit(wd->wlSurface);

    wlFuncs.wl_display_roundtrip(windowState.display);

    VkWaylandSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = windowState.display;
    createInfo.surface = wd->wlSurface;

    using PFN_vkCreateWaylandSurfaceKHR = VkResult(*)(VkInstance, const VkWaylandSurfaceCreateInfoKHR*, const VkAllocationCallbacks*, VkSurfaceKHR*);
    auto vkCreateWaylandSurfaceKHR = reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(
        internal::getVulkanInstanceProcAddr("vkCreateWaylandSurfaceKHR"));

    VkSurfaceKHR surface;
    VkResult result = vkCreateWaylandSurfaceKHR(
        static_cast<VkInstance>(internal::getVulkanInstance()),
        &createInfo,
        nullptr,
        &surface
    );

    if (result != VK_SUCCESS)
    {
        windowState.windows.remove(surfaceKey(wd->wlSurface));
        xdg_toplevel_destroy(wd->xdgToplevel);
        xdg_surface_destroy(wd->xdgSurface);
        if (wd->eglWindow != nullptr)
            wlEglFuncs.wl_egl_window_destroy(wd->eglWindow);
        wl_surface_destroy(wd->wlSurface);
        wd->wlSurface = nullptr;
        wd->~WindowData();
        heapFree(wd, 1);
        setError("Could not create Vulkan surface");
        return Window{};
    }

    wd->swap = GpuSwapchain::create(surface, wd->width, wd->height, config.preferredPresentMode, config.imageUsage);

    return window;
}

void windowDestroy(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    wd->~WindowData();
    heapFree(wd, 1);
}

GpuSwapchain& windowSwapchain(void* data)
{
    return static_cast<WindowData*>(data)->swap;
}

void windowSetTitle(void* data, StringView title)
{
    WindowData* wd = static_cast<WindowData*>(data);
    char buf[256];
    u64 len = title.length < sizeof(buf) - 1 ? title.length : sizeof(buf) - 1;
    memcpy(buf, title.chars, len);
    buf[len] = '\0';
    xdg_toplevel_set_title(wd->xdgToplevel, buf);
}

Span<Event> windowEvents(void* data)
{
    return static_cast<WindowData*>(data)->events;
}

bool windowWasClosed(void* data)
{
    return static_cast<WindowData*>(data)->wasClosed;
}

bool windowIsFocused(void* data)
{
    return static_cast<WindowData*>(data)->isFocused;
}

bool windowWasFocusGained(void* data)
{
    return static_cast<WindowData*>(data)->wasFocusGained;
}

bool windowWasFocusLost(void* data)
{
    return static_cast<WindowData*>(data)->wasFocusLost;
}





bool windowWasResized(void* data)
{
    return static_cast<WindowData*>(data)->wasResized;
}

void windowGetSize(void* data, u32* w, u32* h)
{
    WindowData* wd = static_cast<WindowData*>(data);
    if (w != nullptr)
        *w = wd->width;
    if (h != nullptr)
        *h = wd->height;
}




void windowMaximize(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    xdg_toplevel_set_maximized(wd->xdgToplevel);
}



void windowMinimize(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    xdg_toplevel_set_minimized(wd->xdgToplevel);
}


void windowRestore(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    xdg_toplevel_unset_maximized(wd->xdgToplevel);
    xdg_toplevel_unset_fullscreen(wd->xdgToplevel);
}



void windowSetFullscreen(void* data, bool set)
{
    WindowData* wd = static_cast<WindowData*>(data);
    if (set)
        xdg_toplevel_set_fullscreen(wd->xdgToplevel, nullptr);
    else
        xdg_toplevel_unset_fullscreen(wd->xdgToplevel);
}

StringView getClipboardText()
{
    if (windowState.clipboard.count == 0)
        return StringView{};
    return StringView{windowState.clipboard.vals, windowState.clipboard.count};
}

void setClipboardText(StringView text)
{
    if (windowState.dataDevice == nullptr)
        return;

    windowState.clipboard.resize(text.length);
    memcpy(windowState.clipboard.vals, text.chars, text.length);

    struct wl_data_source* source = wl_data_device_manager_create_data_source(windowState.dataDeviceManager);
    wl_data_source_offer(source, "text/plain;charset=utf-8");
    wl_data_source_add_listener(source, &dataSourceListener, nullptr);

    if (windowState.dataSource != nullptr)
        wl_data_source_destroy(windowState.dataSource);
    windowState.dataSource = source;
    wl_data_device_set_selection(windowState.dataDevice, source, windowState.lastSerial);
    windowState.clipboardOwned = true;
}

void openURL(StringView url)
{
    char cmd[1024];
    u64 len = url.length < sizeof(cmd) - 16 ? url.length : sizeof(cmd) - 16;
    memcpy(cmd, url.chars, len);
    cmd[len] = '\0';

    char fullCmd[1024];
    snprintf(fullCmd, sizeof(fullCmd), "xdg-open '%s' &", cmd);
    (void)system(fullCmd);
}

} // namespace hg::wayland
