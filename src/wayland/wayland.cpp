#include "wayland_window.hpp"

#include "internal.hpp"
#include "hg/utility.hpp"
#include "hg/dynlib.hpp"

#include <string_view>
#include <memory>

#include <math.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <poll.h>

#include <wayland-client-core.h>

#include <xkbcommon/xkbcommon.h>

#include <linux/input.h>

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

#include "wayland-protocol.h"

extern "C" {

static struct wl_registry*
wl_display_get_registry(struct wl_display* display)
{
    return (struct wl_registry*)wlFuncs.wl_proxy_marshal_flags(
        (struct wl_proxy*)display, WL_DISPLAY_GET_REGISTRY,
        &wl_registry_interface, 1, 0);
}

static int
wl_keyboard_add_listener(struct wl_keyboard* keyboard,
    const struct wl_keyboard_listener* listener, void* data)
{
    return wlFuncs.wl_proxy_add_listener((struct wl_proxy*)keyboard,
        (void(**)(void))listener, data);
}

static void
wl_keyboard_destroy(struct wl_keyboard* keyboard)
{
    wlFuncs.wl_proxy_destroy((struct wl_proxy*)keyboard);
}

static int
wl_pointer_add_listener(struct wl_pointer* pointer,
    const struct wl_pointer_listener* listener, void* data)
{
    return wlFuncs.wl_proxy_add_listener((struct wl_proxy*)pointer,
        (void(**)(void))listener, data);
}

static void
wl_pointer_destroy(struct wl_pointer* pointer)
{
    wlFuncs.wl_proxy_destroy((struct wl_proxy*)pointer);
}

static int
wl_seat_add_listener(struct wl_seat* seat,
    const struct wl_seat_listener* listener, void* data)
{
    return wlFuncs.wl_proxy_add_listener((struct wl_proxy*)seat,
        (void(**)(void))listener, data);
}

static struct wl_keyboard*
wl_seat_get_keyboard(struct wl_seat* seat)
{
    return (struct wl_keyboard*)wlFuncs.wl_proxy_marshal_flags(
        (struct wl_proxy*)seat, WL_SEAT_GET_KEYBOARD,
        &wl_keyboard_interface,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)seat), 0, NULL);
}

static struct wl_pointer*
wl_seat_get_pointer(struct wl_seat* seat)
{
    return (struct wl_pointer*)wlFuncs.wl_proxy_marshal_flags(
        (struct wl_proxy*)seat, WL_SEAT_GET_POINTER,
        &wl_pointer_interface,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)seat), 0, NULL);
}

static void
wl_seat_destroy(struct wl_seat* seat)
{
    wlFuncs.wl_proxy_destroy((struct wl_proxy*)seat);
}

static int
wl_output_add_listener(struct wl_output* output,
    const struct wl_output_listener* listener, void* data)
{
    return wlFuncs.wl_proxy_add_listener((struct wl_proxy*)output,
        (void(**)(void))listener, data);
}

static void
wl_output_destroy(struct wl_output* output)
{
    wlFuncs.wl_proxy_destroy((struct wl_proxy*)output);
}

static struct wl_surface*
wl_compositor_create_surface(struct wl_compositor* compositor)
{
    return (struct wl_surface*)wlFuncs.wl_proxy_marshal_flags(
        (struct wl_proxy*)compositor, WL_COMPOSITOR_CREATE_SURFACE,
        &wl_surface_interface,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)compositor), 0, NULL);
}

static void
wl_compositor_destroy(struct wl_compositor* compositor)
{
    wlFuncs.wl_proxy_destroy((struct wl_proxy*)compositor);
}

static void
wl_shm_destroy(struct wl_shm* shm)
{
    wlFuncs.wl_proxy_destroy((struct wl_proxy*)shm);
}

static void
wl_surface_commit(struct wl_surface* surface)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)surface, WL_SURFACE_COMMIT, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)surface), 0);
}

static void
wl_surface_destroy(struct wl_surface* surface)
{
    wlFuncs.wl_proxy_destroy((struct wl_proxy*)surface);
}

static struct wl_data_device*
wl_data_device_manager_get_data_device(struct wl_data_device_manager* manager,
    struct wl_seat* seat)
{
    return (struct wl_data_device*)wlFuncs.wl_proxy_marshal_flags(
        (struct wl_proxy*)manager, WL_DATA_DEVICE_MANAGER_GET_DATA_DEVICE,
        &wl_data_device_interface,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)manager), 0,
        NULL, (struct wl_proxy*)seat);
}

static void
wl_data_device_manager_destroy(struct wl_data_device_manager* manager)
{
    wlFuncs.wl_proxy_destroy((struct wl_proxy*)manager);
}

static int
wl_data_device_add_listener(struct wl_data_device* device,
    const struct wl_data_device_listener* listener, void* data)
{
    return wlFuncs.wl_proxy_add_listener((struct wl_proxy*)device,
        (void(**)(void))listener, data);
}

static void
wl_data_device_destroy(struct wl_data_device* device)
{
    wlFuncs.wl_proxy_destroy((struct wl_proxy*)device);
}

static int
wl_data_offer_add_listener(struct wl_data_offer* offer,
    const struct wl_data_offer_listener* listener, void* data)
{
    return wlFuncs.wl_proxy_add_listener((struct wl_proxy*)offer,
        (void(**)(void))listener, data);
}

static void
wl_data_offer_accept(struct wl_data_offer* offer, uint32_t serial,
    const char* mimeType)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)offer, WL_DATA_OFFER_ACCEPT, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)offer), 0, serial, mimeType);
}

static void
wl_data_offer_receive(struct wl_data_offer* offer, const char* mimeType, int32_t fd)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)offer, WL_DATA_OFFER_RECEIVE, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)offer), 0, mimeType, fd);
}

static void
wl_data_offer_destroy(struct wl_data_offer* offer)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)offer, WL_DATA_OFFER_DESTROY, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)offer), WL_MARSHAL_FLAG_DESTROY);
}

static int
xdg_wm_base_add_listener(struct xdg_wm_base* xdgWmBase,
    const struct xdg_wm_base_listener* listener, void* data)
{
    return wlFuncs.wl_proxy_add_listener((struct wl_proxy*)xdgWmBase,
        (void(**)(void))listener, data);
}

static struct xdg_surface*
xdg_wm_base_get_xdg_surface(struct xdg_wm_base* xdgWmBase,
    struct wl_surface* surface)
{
    return (struct xdg_surface*)wlFuncs.wl_proxy_marshal_flags(
        (struct wl_proxy*)xdgWmBase, XDG_WM_BASE_GET_XDG_SURFACE,
        &xdg_surface_interface,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)xdgWmBase), 0,
        NULL, (struct wl_proxy*)surface);
}

static void
xdg_wm_base_pong(struct xdg_wm_base* xdgWmBase, uint32_t serial)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)xdgWmBase, XDG_WM_BASE_PONG, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)xdgWmBase), 0, serial);
}

static void
xdg_wm_base_destroy(struct xdg_wm_base* xdgWmBase)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)xdgWmBase, XDG_WM_BASE_DESTROY, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)xdgWmBase), WL_MARSHAL_FLAG_DESTROY);
}

static int
xdg_surface_add_listener(struct xdg_surface* xdgSurface,
    const struct xdg_surface_listener* listener, void* data)
{
    return wlFuncs.wl_proxy_add_listener((struct wl_proxy*)xdgSurface,
        (void(**)(void))listener, data);
}

static struct xdg_toplevel*
xdg_surface_get_toplevel(struct xdg_surface* xdgSurface)
{
    return (struct xdg_toplevel*)wlFuncs.wl_proxy_marshal_flags(
        (struct wl_proxy*)xdgSurface, XDG_SURFACE_GET_TOPLEVEL,
        &xdg_toplevel_interface,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)xdgSurface), 0, NULL);
}

static void
xdg_surface_ack_configure(struct xdg_surface* xdgSurface, uint32_t serial)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)xdgSurface, XDG_SURFACE_ACK_CONFIGURE, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)xdgSurface), 0, serial);
}

static void
xdg_surface_destroy(struct xdg_surface* xdgSurface)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)xdgSurface, XDG_SURFACE_DESTROY, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)xdgSurface), WL_MARSHAL_FLAG_DESTROY);
}

static int
xdg_toplevel_add_listener(struct xdg_toplevel* toplevel,
    const struct xdg_toplevel_listener* listener, void* data)
{
    return wlFuncs.wl_proxy_add_listener((struct wl_proxy*)toplevel,
        (void(**)(void))listener, data);
}

static void
xdg_toplevel_set_title(struct xdg_toplevel* toplevel, const char* title)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)toplevel, XDG_TOPLEVEL_SET_TITLE, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)toplevel), 0, title);
}

static void
xdg_toplevel_set_fullscreen(struct xdg_toplevel* toplevel,
    struct wl_output* output)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)toplevel, XDG_TOPLEVEL_SET_FULLSCREEN, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)toplevel), 0,
        (struct wl_proxy*)output);
}

static void
xdg_toplevel_unset_fullscreen(struct xdg_toplevel* toplevel)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)toplevel, XDG_TOPLEVEL_UNSET_FULLSCREEN, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)toplevel), 0);
}

static void
xdg_toplevel_destroy(struct xdg_toplevel* toplevel)
{
    wlFuncs.wl_proxy_marshal_flags((struct wl_proxy*)toplevel, XDG_TOPLEVEL_DESTROY, NULL,
        wlFuncs.wl_proxy_get_version((struct wl_proxy*)toplevel), WL_MARSHAL_FLAG_DESTROY);
}

} // extern "C"

namespace hg {

static InputDeviceWayland* wlDevice = nullptr;

static u64 surfaceKey(struct wl_surface* surface)
{
    return reinterpret_cast<u64>(surface);
}

#define HG_WL_FUNC(name) decltype(&::name) name = nullptr

struct XkbFuncs {
    HG_WL_FUNC(xkb_context_new);
    HG_WL_FUNC(xkb_context_unref);
    HG_WL_FUNC(xkb_keymap_new_from_names);
    HG_WL_FUNC(xkb_keymap_new_from_string);
    HG_WL_FUNC(xkb_keymap_unref);
    HG_WL_FUNC(xkb_state_new);
    HG_WL_FUNC(xkb_state_unref);
    HG_WL_FUNC(xkb_state_key_get_layout);
    HG_WL_FUNC(xkb_keymap_key_get_syms_by_level);
    HG_WL_FUNC(xkb_state_key_get_utf8);
    HG_WL_FUNC(xkb_state_update_mask);
};

#undef HG_WL_FUNC

static Library libWayland{};

static Library libxkb{};
static XkbFuncs xkbFuncs{};

static bool loadLibwayland()
{
    std::optional<Library> lib = Library::load("libwayland-client.so.0");
    if (!lib.has_value())
    {
        setError("Could not load libwayland-client");
        return false;
    }
    libWayland = std::move(*lib);

#define HG_LOAD_WL(name) \
    *(void**)&wlFuncs.name = libWayland.loadSymbol(#name).value_or(nullptr); \
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
    HG_LOAD_XKB(xkb_keymap_new_from_string);
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

static bool loadWayland()
{
    if (!loadLibwayland())
        return false;
    if (!loadXkb())
        return false;

    return true;
}

// Button mapping
static Button xkbKeysymToButton(uint32_t keysym)
{
    switch (keysym)
    {
        case XKB_KEY_Escape: return Button::escape;
        case XKB_KEY_Return: return Button::enter;
        case XKB_KEY_Tab: return Button::tab;
        case XKB_KEY_BackSpace: return Button::backspace;
        case XKB_KEY_space: return Button::space;

        case XKB_KEY_Caps_Lock: return Button::capslock;
        case XKB_KEY_Num_Lock: return Button::numlock;
        case XKB_KEY_Scroll_Lock: return Button::scrolllock;

        case XKB_KEY_Control_L: return Button::lctrl;
        case XKB_KEY_Control_R: return Button::rctrl;
        case XKB_KEY_Shift_L: return Button::lshift;
        case XKB_KEY_Shift_R: return Button::rshift;
        case XKB_KEY_Alt_L: return Button::lalt;
        case XKB_KEY_Alt_R: return Button::ralt;
        case XKB_KEY_Super_L: return Button::lsuper;
        case XKB_KEY_Super_R: return Button::rsuper;

        case XKB_KEY_Up: return Button::up;
        case XKB_KEY_Down: return Button::down;
        case XKB_KEY_Left: return Button::left;
        case XKB_KEY_Right: return Button::right;

        case XKB_KEY_F1: return Button::f1;
        case XKB_KEY_F2: return Button::f2;
        case XKB_KEY_F3: return Button::f3;
        case XKB_KEY_F4: return Button::f4;
        case XKB_KEY_F5: return Button::f5;
        case XKB_KEY_F6: return Button::f6;
        case XKB_KEY_F7: return Button::f7;
        case XKB_KEY_F8: return Button::f8;
        case XKB_KEY_F9: return Button::f9;
        case XKB_KEY_F10: return Button::f10;
        case XKB_KEY_F11: return Button::f11;
        case XKB_KEY_F12: return Button::f12;

        case XKB_KEY_a: return Button::a;
        case XKB_KEY_b: return Button::b;
        case XKB_KEY_c: return Button::c;
        case XKB_KEY_d: return Button::d;
        case XKB_KEY_e: return Button::e;
        case XKB_KEY_f: return Button::f;
        case XKB_KEY_g: return Button::g;
        case XKB_KEY_h: return Button::h;
        case XKB_KEY_i: return Button::i;
        case XKB_KEY_j: return Button::j;
        case XKB_KEY_k: return Button::k;
        case XKB_KEY_l: return Button::l;
        case XKB_KEY_m: return Button::m;
        case XKB_KEY_n: return Button::n;
        case XKB_KEY_o: return Button::o;
        case XKB_KEY_p: return Button::p;
        case XKB_KEY_q: return Button::q;
        case XKB_KEY_r: return Button::r;
        case XKB_KEY_s: return Button::s;
        case XKB_KEY_t: return Button::t;
        case XKB_KEY_u: return Button::u;
        case XKB_KEY_v: return Button::v;
        case XKB_KEY_w: return Button::w;
        case XKB_KEY_x: return Button::x;
        case XKB_KEY_y: return Button::y;
        case XKB_KEY_z: return Button::z;

        case XKB_KEY_0: return Button::b0;
        case XKB_KEY_1: return Button::b1;
        case XKB_KEY_2: return Button::b2;
        case XKB_KEY_3: return Button::b3;
        case XKB_KEY_4: return Button::b4;
        case XKB_KEY_5: return Button::b5;
        case XKB_KEY_6: return Button::b6;
        case XKB_KEY_7: return Button::b7;
        case XKB_KEY_8: return Button::b8;
        case XKB_KEY_9: return Button::b9;

        case XKB_KEY_minus: return Button::minus;
        case XKB_KEY_equal: return Button::equal;
        case XKB_KEY_bracketleft: return Button::lbracket;
        case XKB_KEY_bracketright: return Button::rbracket;
        case XKB_KEY_backslash: return Button::backslash;
        case XKB_KEY_semicolon: return Button::semicolon;
        case XKB_KEY_apostrophe: return Button::apostrophe;
        case XKB_KEY_grave: return Button::grave;
        case XKB_KEY_comma: return Button::comma;
        case XKB_KEY_period: return Button::period;
        case XKB_KEY_slash: return Button::slash;

        case XKB_KEY_Home: return Button::home;
        case XKB_KEY_End: return Button::end;
        case XKB_KEY_Page_Up: return Button::pageup;
        case XKB_KEY_Page_Down: return Button::pagedown;
        case XKB_KEY_Delete: return Button::kdelete;
        case XKB_KEY_Insert: return Button::insert;
        case XKB_KEY_Print: return Button::printscreen;
        case XKB_KEY_Pause: return Button::pause;
        case XKB_KEY_Menu: return Button::context;

        case XKB_KEY_KP_0: return Button::numpad0;
        case XKB_KEY_KP_1: return Button::numpad1;
        case XKB_KEY_KP_2: return Button::numpad2;
        case XKB_KEY_KP_3: return Button::numpad3;
        case XKB_KEY_KP_4: return Button::numpad4;
        case XKB_KEY_KP_5: return Button::numpad5;
        case XKB_KEY_KP_6: return Button::numpad6;
        case XKB_KEY_KP_7: return Button::numpad7;
        case XKB_KEY_KP_8: return Button::numpad8;
        case XKB_KEY_KP_9: return Button::numpad9;
        case XKB_KEY_KP_Decimal: return Button::numpaddecimal;
        case XKB_KEY_KP_Divide: return Button::numpaddiv;
        case XKB_KEY_KP_Multiply: return Button::numpadmul;
        case XKB_KEY_KP_Subtract: return Button::numpadminus;
        case XKB_KEY_KP_Add: return Button::numpadplus;
        case XKB_KEY_KP_Enter: return Button::numpadenter;

        default: return Button::count;
    }
}

static Button pointerButtonToHgButton(uint32_t button)
{
    switch (button)
    {
        case BTN_LEFT: return Button::mouse1;
        case BTN_RIGHT: return Button::mouse2;
        case BTN_MIDDLE: return Button::mouse3;
        case BTN_SIDE: return Button::mouse4;
        case BTN_EXTRA: return Button::mouse5;
        default: return Button::count;
    }
}

// ---- Wayland protocol callbacks ----

void InputDeviceWayland::keyboardKeymap(void* data, struct wl_keyboard* keyboard,
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

    if (wlDevice->xkbKeymap != nullptr)
        xkbFuncs.xkb_keymap_unref(wlDevice->xkbKeymap);
    if (wlDevice->xkbState != nullptr)
        xkbFuncs.xkb_state_unref(wlDevice->xkbState);

    wlDevice->xkbKeymap = xkbFuncs.xkb_keymap_new_from_string(
        wlDevice->xkbContext, mapStr, XKB_KEYMAP_FORMAT_TEXT_V1,
        static_cast<xkb_keymap_compile_flags>(0));

    munmap(mapStr, size);
    close(fd);

    if (wlDevice->xkbKeymap == nullptr)
    {
        return;
    }

    wlDevice->xkbState = xkbFuncs.xkb_state_new(wlDevice->xkbKeymap);
}

void InputDeviceWayland::keyboardEnter(void* data, struct wl_keyboard* keyboard,
    uint32_t serial, struct wl_surface* surface, struct wl_array* keys)
{
    (void)data;
    (void)keyboard;
    (void)keys;

    wlDevice->lastSerial = serial;

    if (surface == nullptr)
        return;

    WindowWayland** found = wlDevice->windows.get(surfaceKey(surface));
    if (found != nullptr)
    {
        (*found)->focused = true;
        (*found)->focusGained = true;
        wlDevice->activeWindow = *found;

        Event event{};
        event.type = EventType::windowFocused;
        wlDevice->events.push_back(event);
        (*found)->eventQueue.push_back(event);
    }
}

void InputDeviceWayland::keyboardLeave(void* data, struct wl_keyboard* keyboard,
    uint32_t serial, struct wl_surface* surface)
{
    (void)data;
    (void)keyboard;

    wlDevice->lastSerial = serial;

    if (surface == nullptr)
        return;

    WindowWayland** found = wlDevice->windows.get(surfaceKey(surface));
    if (found != nullptr)
    {
        (*found)->focused = false;
        (*found)->focusLost = true;
        if (wlDevice->activeWindow == *found)
            wlDevice->activeWindow = nullptr;

        Event event{};
        event.type = EventType::windowUnfocused;
        wlDevice->events.push_back(event);
        (*found)->eventQueue.push_back(event);
    }
}

void InputDeviceWayland::keyboardKey(void* data, struct wl_keyboard* keyboard,
    uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    (void)data;
    (void)keyboard;
    (void)time;

    wlDevice->lastSerial = serial;

    bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

    // Buttons represent physical keys, so map the level 0 (unmodified)
    // keysym of the active layout instead of the modifier-dependent keysym:
    // press/release must resolve to the same button with or without shift.
    Button button = Button::count;
    if (wlDevice->xkbState != nullptr)
    {
        xkb_layout_index_t layout =
            xkbFuncs.xkb_state_key_get_layout(wlDevice->xkbState, key + 8);
        const xkb_keysym_t* syms = nullptr;
        int numSyms = xkbFuncs.xkb_keymap_key_get_syms_by_level(
            wlDevice->xkbKeymap, key + 8, layout, 0, &syms);
        if (numSyms > 0 && syms != nullptr)
            button = xkbKeysymToButton(syms[0]);
    }

    if (button < Button::count)
    {
        wlDevice->isKeyDown[static_cast<u32>(button)] = pressed;

        Event event{};
        event.type = pressed ? EventType::keyPress : EventType::keyRelease;
        event.button = button;
        wlDevice->events.push_back(event);
    }

    if (pressed && wlDevice->xkbState != nullptr)
    {
        char buf[32];
        int len = xkbFuncs.xkb_state_key_get_utf8(wlDevice->xkbState, key + 8, buf, sizeof(buf) - 1);
        if (len > 0)
        {
            buf[len] = '\0';
            Event event{};
            event.type = EventType::text;
            std::memset(event.text, 0, sizeof(event.text));
            std::memcpy(event.text, buf, assertCast<u64>(len));
            wlDevice->events.push_back(event);
        }
    }
}

void InputDeviceWayland::keyboardModifiers(void* data, struct wl_keyboard* keyboard,
    uint32_t serial, uint32_t modsDepressed, uint32_t modsLatched,
    uint32_t modsLocked, uint32_t group)
{
    (void)data;
    (void)keyboard;
    (void)serial;

    xkbFuncs.xkb_state_update_mask(wlDevice->xkbState,
        modsDepressed, modsLatched, modsLocked, 0, 0, group);
    wlDevice->xkbModifiers = modsDepressed;
}

void InputDeviceWayland::keyboardRepeatInfo(void* data, struct wl_keyboard* keyboard,
    int32_t rate, int32_t delay)
{
    (void)data;
    (void)keyboard;
    (void)rate;
    (void)delay;
}

static const struct wl_keyboard_listener keyboardListener = {
    .keymap = InputDeviceWayland::keyboardKeymap,
    .enter = InputDeviceWayland::keyboardEnter,
    .leave = InputDeviceWayland::keyboardLeave,
    .key = InputDeviceWayland::keyboardKey,
    .modifiers = InputDeviceWayland::keyboardModifiers,
    .repeat_info = InputDeviceWayland::keyboardRepeatInfo,
};
void InputDeviceWayland::pointerEnter(void* data, struct wl_pointer* pointer,
    uint32_t serial, struct wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy)
{
    (void)data;
    (void)pointer;


    wlDevice->lastSerial = serial;
    wlDevice->pointerEnterSerial = serial;
    wlDevice->pointerFocus = surface;

    if (surface == nullptr)
        return;

    WindowWayland** found = wlDevice->windows.get(surfaceKey(surface));
    if (found != nullptr)
    {
        (*found)->mouse.x() = assertCast<f32>(wl_fixed_to_double(sx));
        (*found)->mouse.y() = assertCast<f32>(wl_fixed_to_double(sy));
    }
}

void InputDeviceWayland::pointerLeave(void* data, struct wl_pointer* pointer,
    uint32_t serial, struct wl_surface* surface)
{
    (void)data;
    (void)pointer;

    wlDevice->lastSerial = serial;

    if (wlDevice->pointerFocus == surface)
        wlDevice->pointerFocus = nullptr;
}

void InputDeviceWayland::pointerMotion(void* data, struct wl_pointer* pointer,
    uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    (void)data;
    (void)pointer;
    (void)time;

    if (wlDevice->pointerFocus == nullptr)
        return;

    WindowWayland** found = wlDevice->windows.get(surfaceKey(wlDevice->pointerFocus));
    if (found != nullptr)
    {
        Vec2 newPos{assertCast<f32>(wl_fixed_to_double(sx)), assertCast<f32>(wl_fixed_to_double(sy))};
        Vec2 delta = newPos - (*found)->mouse;
        (*found)->mouse = newPos;
        wlDevice->mouseDeltaAccum += delta;

        Event event{};
        event.type = EventType::mouseMoved;
        event.mouse.delta = delta;
        event.mouse.pos = newPos;
        event.mouse.globalPos = newPos;
        wlDevice->events.push_back(event);
    }
}

void InputDeviceWayland::pointerButton(void* data, struct wl_pointer* pointer,
    uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    (void)data;
    (void)pointer;
    (void)time;

    wlDevice->lastSerial = serial;

    bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
    Button hgButton = pointerButtonToHgButton(button);

    if (hgButton < Button::count)
    {
        wlDevice->isKeyDown[static_cast<u32>(hgButton)] = pressed;

        Event event{};
        event.type = pressed ? EventType::keyPress : EventType::keyRelease;
        event.button = hgButton;
        wlDevice->events.push_back(event);
    }
}

void InputDeviceWayland::pointerAxis(void* data, struct wl_pointer* pointer,
    uint32_t time, uint32_t axis, wl_fixed_t value)
{
    (void)data;
    (void)pointer;
    (void)time;

    Vec2 delta{};
    f32 amount = assertCast<f32>(wl_fixed_to_double(value));

    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        delta.y() = amount;
    else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        delta.x() = amount;

    if (delta.x() != 0.0f || delta.y() != 0.0f)
    {
        wlDevice->wheelDeltaAccum += delta;

        Event event{};
        event.type = EventType::wheelMoved;
        event.wheel.delta = delta;
        wlDevice->events.push_back(event);
    }
}

static const struct wl_pointer_listener pointerListener = {
    .enter = InputDeviceWayland::pointerEnter,
    .leave = InputDeviceWayland::pointerLeave,
    .motion = InputDeviceWayland::pointerMotion,
    .button = InputDeviceWayland::pointerButton,
    .axis = InputDeviceWayland::pointerAxis,
    .frame = [](void*, struct wl_pointer*) {},
    .axis_source = [](void*, struct wl_pointer*, uint32_t) {},
    .axis_stop = [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
    .axis_discrete = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
    .axis_value120 = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
    .axis_relative_direction = [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
    .warp = [](void*, struct wl_pointer*, wl_fixed_t, wl_fixed_t) {},
};

void InputDeviceWayland::seatCapabilities(void* data, struct wl_seat* seat, uint32_t capabilities)
{
    (void)data;

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && wlDevice->keyboard == nullptr)
    {
        wlDevice->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(wlDevice->keyboard, &keyboardListener, nullptr);
    }
    else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && wlDevice->keyboard != nullptr)
    {
        wl_keyboard_destroy(wlDevice->keyboard);
        wlDevice->keyboard = nullptr;
    }

    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && wlDevice->pointer == nullptr)
    {
        wlDevice->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(wlDevice->pointer, &pointerListener, nullptr);
    }
    else if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && wlDevice->pointer != nullptr)
    {
        wl_pointer_destroy(wlDevice->pointer);
        wlDevice->pointer = nullptr;
    }
}

void InputDeviceWayland::seatName(void* data, struct wl_seat* seat, const char* name)
{
    (void)data;
    (void)seat;
    (void)name;
}

static const struct wl_seat_listener seatListener = {
    .capabilities = InputDeviceWayland::seatCapabilities,
    .name = InputDeviceWayland::seatName,
};

void InputDeviceWayland::outputGeometry(void* data, struct wl_output* output,
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

    for (u32 i = 0; i < wlDevice->outputCount; i++)
    {
        if (wlDevice->outputs[i] == output)
        {
            if (wlDevice->displays.size() <= i)
                wlDevice->displays.resize(assertCast<u64>(i + 1));
            wlDevice->displays[i].posX = x;
            wlDevice->displays[i].posY = y;
            break;
        }
    }
}

void InputDeviceWayland::outputMode(void* data, struct wl_output* output,
    uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
    (void)data;
    (void)refresh;

    if (!(flags & WL_OUTPUT_MODE_CURRENT))
        return;

    for (u32 i = 0; i < wlDevice->outputCount; i++)
    {
        if (wlDevice->outputs[i] == output)
        {
            if (wlDevice->displays.size() <= i)
                wlDevice->displays.resize(assertCast<u64>(i + 1));
            wlDevice->displays[i].sizeW = assertCast<u32>(width);
            wlDevice->displays[i].sizeH = assertCast<u32>(height);
            wlDevice->displays[i].workPosX = wlDevice->displays[i].posX;
            wlDevice->displays[i].workPosY = wlDevice->displays[i].posY;
            wlDevice->displays[i].workSizeW = wlDevice->displays[i].sizeW;
            wlDevice->displays[i].workSizeH = wlDevice->displays[i].sizeH;
            wlDevice->displays[i].dpiScale = 1.0f;
            break;
        }
    }
}

void InputDeviceWayland::outputDone(void* data, struct wl_output* output)
{
    (void)data;
    (void)output;
}

void InputDeviceWayland::outputScale(void* data, struct wl_output* output, int32_t factor)
{
    (void)data;

    for (u32 i = 0; i < wlDevice->outputCount; i++)
    {
        if (wlDevice->outputs[i] == output)
        {
            if (wlDevice->displays.size() <= i)
                wlDevice->displays.resize(assertCast<u64>(i + 1));
            wlDevice->displays[i].dpiScale = assertCast<f32>(factor);
            break;
        }
    }
}

void InputDeviceWayland::outputName(void* data, struct wl_output* output, const char* name)
{
    (void)data;
    (void)output;
    (void)name;
}

void InputDeviceWayland::outputDescription(void* data, struct wl_output* output, const char* description)
{
    (void)data;
    (void)output;
    (void)description;
}

static const struct wl_output_listener outputListener = {
    .geometry = InputDeviceWayland::outputGeometry,
    .mode = InputDeviceWayland::outputMode,
    .done = InputDeviceWayland::outputDone,
    .scale = InputDeviceWayland::outputScale,
    .name = InputDeviceWayland::outputName,
    .description = InputDeviceWayland::outputDescription,
};

void InputDeviceWayland::registryGlobal(void* data, struct wl_registry* registry,
    uint32_t name, const char* interface, uint32_t version)
{
    (void)data;

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        wlDevice->compositor = reinterpret_cast<struct wl_compositor*>(
            wlFuncs.wl_proxy_marshal_flags(
                reinterpret_cast<struct wl_proxy*>(registry),
                WL_REGISTRY_BIND, &wl_compositor_interface, version, 0,
                name, wl_compositor_interface.name, version, NULL));
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        wlDevice->shm = reinterpret_cast<struct wl_shm*>(
            wlFuncs.wl_proxy_marshal_flags(
                reinterpret_cast<struct wl_proxy*>(registry),
                WL_REGISTRY_BIND, &wl_shm_interface, version, 0,
                name, wl_shm_interface.name, version, NULL));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        wlDevice->xdgWmBase = reinterpret_cast<struct xdg_wm_base*>(
            wlFuncs.wl_proxy_marshal_flags(
                reinterpret_cast<struct wl_proxy*>(registry),
                WL_REGISTRY_BIND, &xdg_wm_base_interface, version, 0,
                name, xdg_wm_base_interface.name, version, NULL));
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        wlDevice->seat = reinterpret_cast<struct wl_seat*>(
            wlFuncs.wl_proxy_marshal_flags(
                reinterpret_cast<struct wl_proxy*>(registry),
                WL_REGISTRY_BIND, &wl_seat_interface, version, 0,
                name, wl_seat_interface.name, version, NULL));
        wl_seat_add_listener(wlDevice->seat, &seatListener, nullptr);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
        if (wlDevice->outputCount < 16)
        {
            struct wl_output* output = reinterpret_cast<struct wl_output*>(
                wlFuncs.wl_proxy_marshal_flags(
                    reinterpret_cast<struct wl_proxy*>(registry),
                    WL_REGISTRY_BIND, &wl_output_interface, version, 0,
                    name, wl_output_interface.name, version, NULL));
            wlDevice->outputs[wlDevice->outputCount++] = output;
            wl_output_add_listener(output, &outputListener, nullptr);
        }
    }
    else if (strcmp(interface, wl_data_device_manager_interface.name) == 0)
    {
        wlDevice->dataDeviceManager = reinterpret_cast<struct wl_data_device_manager*>(
            wlFuncs.wl_proxy_marshal_flags(
                reinterpret_cast<struct wl_proxy*>(registry),
                WL_REGISTRY_BIND, &wl_data_device_manager_interface, version, 0,
                name, wl_data_device_manager_interface.name, version, NULL));
    }
}

void InputDeviceWayland::registryGlobalRemove(void* data, struct wl_registry* registry, uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

static struct wl_registry_listener registryListener = {
    .global = InputDeviceWayland::registryGlobal,
    .global_remove = InputDeviceWayland::registryGlobalRemove,
};

void InputDeviceWayland::xdgWmBasePing(void* data, struct xdg_wm_base* xdgWmBase, uint32_t serial)
{
    (void)data;
    xdg_wm_base_pong(xdgWmBase, serial);
}

static const struct xdg_wm_base_listener xdgWmBaseListener = {
    .ping = InputDeviceWayland::xdgWmBasePing,
};

void InputDeviceWayland::xdgToplevelConfigure(void* data, struct xdg_toplevel* toplevel,
    int32_t width, int32_t height, struct wl_array* states)
{
    (void)toplevel;
    (void)states;

    WindowWayland* window = static_cast<WindowWayland*>(data);
    if (window == nullptr)
        return;

    if (width > 0 && height > 0)
    {
        u32 newW = assertCast<u32>(width);
        u32 newH = assertCast<u32>(height);

        if (newW != window->width || newH != window->height)
        {
            // Ignore suggests while creating, before the swapchain exists
            if (window->swap == nullptr)
            {
                window->width = newW;
                window->height = newH;
                return;
            }

            window->resized = true;
            window->width = newW;
            window->height = newH;

            window->swap->resize(newW, newH);

            Event event{};
            event.type = EventType::windowResized;
            event.window.window = window;
            event.window.width = newW;
            event.window.height = newH;
            wlDevice->events.push_back(event);
            window->eventQueue.push_back(event);
        }
    }
}

void InputDeviceWayland::xdgToplevelClose(void* data, struct xdg_toplevel* toplevel)
{
    (void)toplevel;

    WindowWayland* window = static_cast<WindowWayland*>(data);
    if (window == nullptr)
        return;

    window->closed = true;

    Event event{};
    event.type = EventType::windowClosed;
    wlDevice->events.push_back(event);
    window->eventQueue.push_back(event);
}

void InputDeviceWayland::xdgToplevelConfigureBounds(void* data, struct xdg_toplevel* toplevel,
    int32_t width, int32_t height)
{
    (void)data;
    (void)toplevel;
    (void)width;
    (void)height;
}

void InputDeviceWayland::xdgToplevelWmCapabilities(void* data, struct xdg_toplevel* toplevel,
    struct wl_array* capabilities)
{
    (void)data;
    (void)toplevel;
    (void)capabilities;
}

static const struct xdg_toplevel_listener xdgToplevelListener = {
    .configure = InputDeviceWayland::xdgToplevelConfigure,
    .close = InputDeviceWayland::xdgToplevelClose,
    .configure_bounds = InputDeviceWayland::xdgToplevelConfigureBounds,
    .wm_capabilities = InputDeviceWayland::xdgToplevelWmCapabilities,
};

void InputDeviceWayland::xdgSurfaceConfigure(void* data, struct xdg_surface* xdgSurface, uint32_t serial)
{
    (void)data;
    xdg_surface_ack_configure(xdgSurface, serial);
}

static const struct xdg_surface_listener xdgSurfaceListener = {
    .configure = InputDeviceWayland::xdgSurfaceConfigure,
};

void InputDeviceWayland::dataOfferOffer(void* data, struct wl_data_offer* offer, const char* mimeType)
{
    (void)data;
    (void)offer;

    if (mimeType != nullptr &&
        (strcmp(mimeType, "text/plain") == 0 || strcmp(mimeType, "text/plain;charset=utf-8") == 0))
        wlDevice->clipboardHasText = true;
}

static const struct wl_data_offer_listener dataOfferListener = {
    .offer = InputDeviceWayland::dataOfferOffer,
    .source_actions = [](void*, struct wl_data_offer*, uint32_t) {},
    .action = [](void*, struct wl_data_offer*, uint32_t) {},
};

void InputDeviceWayland::dataDeviceDataOffer(void* data, struct wl_data_device* device,
    struct wl_data_offer* offer)
{
    (void)data;
    (void)device;

    if (offer != nullptr)
    {
        wlDevice->clipboardHasText = false;
        wl_data_offer_add_listener(offer, &dataOfferListener, nullptr);
    }
}

void InputDeviceWayland::dataDeviceEnter(void* data, struct wl_data_device* device,
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

void InputDeviceWayland::dataDeviceMotion(void* data, struct wl_data_device* device,
    uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    (void)data;
    (void)device;
    (void)time;
    (void)x;
    (void)y;
}

void InputDeviceWayland::dataDeviceLeave(void* data, struct wl_data_device* device)
{
    (void)data;
    (void)device;
}

void InputDeviceWayland::dataDeviceDrop(void* data, struct wl_data_device* device)
{
    (void)data;
    (void)device;
}

void InputDeviceWayland::dataDeviceSelection(void* data, struct wl_data_device* device,
    struct wl_data_offer* offer)
{
    (void)data;
    (void)device;

    if (wlDevice->pendingOffer != nullptr && wlDevice->pendingOffer != offer)
    {
        wl_data_offer_destroy(wlDevice->pendingOffer);
        wlDevice->pendingOffer = nullptr;
    }

    if (offer == nullptr)
        return;

    wlDevice->pendingOffer = offer;
    if (!wlDevice->clipboardHasText)
        return;

    wl_data_offer_accept(offer, wlDevice->lastSerial, "text/plain;charset=utf-8");

    if (wlDevice->clipboardFd >= 0)
    {
        close(wlDevice->clipboardFd);
        wlDevice->clipboardFd = -1;
    }
    wlDevice->clipboardIncoming.resize(0);

    wlDevice->clipboardFd = memfd_create("hurdygurdy-clipboard", 0);
    if (wlDevice->clipboardFd < 0)
        return;

    wl_data_offer_receive(offer, "text/plain;charset=utf-8", wlDevice->clipboardFd);
}

static const struct wl_data_device_listener dataDeviceListener = {
    .data_offer = InputDeviceWayland::dataDeviceDataOffer,
    .enter = InputDeviceWayland::dataDeviceEnter,
    .leave = InputDeviceWayland::dataDeviceLeave,
    .motion = InputDeviceWayland::dataDeviceMotion,
    .drop = InputDeviceWayland::dataDeviceDrop,
    .selection = InputDeviceWayland::dataDeviceSelection,
};




void InputDeviceWayland::readClipboard()
{
    if (clipboardFd < 0)
        return;

    char buf[4096];
    ssize_t n;
    bool eof = false;
    while ((n = read(clipboardFd, buf, sizeof(buf))) > 0)
    {
        u64 old = clipboardIncoming.size();
        clipboardIncoming.resize(old + assertCast<u64>(n));
        std::memcpy(clipboardIncoming.data() + old, buf, assertCast<u64>(n));
    }
    if (n == 0)
        eof = true;
    else if (errno != EAGAIN && errno != EWOULDBLOCK)
        eof = true;

    if (eof)
    {
        close(clipboardFd);
        clipboardFd = -1;

        clipboard.resize(clipboardIncoming.size());
        std::memcpy(clipboard.data(), clipboardIncoming.data(), clipboardIncoming.size());
        clipboardIncoming.resize(0);

        if (pendingOffer != nullptr)
        {
            wl_data_offer_destroy(pendingOffer);
            pendingOffer = nullptr;
        }
    }
}

std::span<std::string_view> InputDeviceWayland::getVulkanExtensions()
{
    static std::string_view exts[] = {
        "VK_KHR_surface",
        "VK_KHR_wayland_surface",
    };
    return exts;
}

std::span<DisplayInfo> InputDeviceWayland::displayInfo()
{
    return displays;
}

void InputDeviceWayland::processEvents()
{
    events.clear();

    std::memcpy(wasKeyDown, isKeyDown, sizeof(isKeyDown));
    mouseDeltaAccum = {};
    wheelDeltaAccum = {};

    windows.forEach([](u64, WindowWayland* data)
    {
        data->eventQueue.clear();
        data->focusGained = false;
        data->focusLost = false;
        data->resized = false;
    });

    if (wlFuncs.wl_display_get_error(display) != 0)
        return;

    while (wlFuncs.wl_display_prepare_read(display) != 0)
    {
        wlFuncs.wl_display_dispatch_pending(display);
        if (wlFuncs.wl_display_get_error(display) != 0)
            return;
    }

    wlFuncs.wl_display_flush(display);

    struct pollfd pfd{};
    pfd.fd = wlFuncs.wl_display_get_fd(display);
    pfd.events = POLLIN;

    if (poll(&pfd, 1, 0) > 0)
    {
        wlFuncs.wl_display_read_events(display);
        while (wlFuncs.wl_display_dispatch_pending(display) != 0)
        {
            if (wlFuncs.wl_display_get_error(display) != 0)
                return;
        }
    }
    else
    {
        wlFuncs.wl_display_cancel_read(display);
    }

    wlFuncs.wl_display_flush(display);

    readClipboard();
}

std::span<const Event> InputDeviceWayland::getEvents()
{
    return events;
}

bool InputDeviceWayland::wasQuit()
{
    return quit;
}

std::unique_ptr<Window> InputDeviceWayland::createWindow(Gpu* gpu, const WindowConfig& config)
{
    std::unique_ptr<WindowWayland> window = std::make_unique<WindowWayland>(this);
    WindowWayland* wd = window.get();

    wd->wlSurface = wl_compositor_create_surface(compositor);
    if (wd->wlSurface == nullptr)
    {
        setError("Could not create Wayland surface");
        return nullptr;
    }

    wd->xdgSurface = xdg_wm_base_get_xdg_surface(xdgWmBase, wd->wlSurface);
    if (wd->xdgSurface == nullptr)
    {
        setError("Could not create XDG surface");
        return nullptr;
    }

    xdg_surface_add_listener(wd->xdgSurface, &xdgSurfaceListener, nullptr);

    wd->xdgToplevel = xdg_surface_get_toplevel(wd->xdgSurface);
    if (wd->xdgToplevel == nullptr)
    {
        setError("Could not create XDG toplevel");
        return nullptr;
    }

    xdg_toplevel_add_listener(wd->xdgToplevel, &xdgToplevelListener, wd);
    xdg_toplevel_set_title(wd->xdgToplevel, config.title.length() != 0
        ? std::string{config.title}.c_str()
        : "Hurdy Gurdy");

    // 0 lets the compositor suggest a size, which arrives in the roundtrip below
    wd->width = config.width != 0 ? config.width : 800;
    wd->height = config.height != 0 ? config.height : 600;

    windows.add(surfaceKey(wd->wlSurface), wd);

    wl_surface_commit(wd->wlSurface);

    wlFuncs.wl_display_roundtrip(display);

    VkWaylandSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = display;
    createInfo.surface = wd->wlSurface;

    using PFN_vkCreateWaylandSurfaceKHR = VkResult(*)(VkInstance, const VkWaylandSurfaceCreateInfoKHR*, const VkAllocationCallbacks*, VkSurfaceKHR*);
    auto vkCreateWaylandSurfaceKHR = reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(
        internal::getVkInstanceProcAddr(gpu, "vkCreateWaylandSurfaceKHR"));

    VkSurfaceKHR surface;
    VkResult result = vkCreateWaylandSurfaceKHR(
        static_cast<VkInstance>(internal::getVkInstance(gpu)),
        &createInfo,
        nullptr,
        &surface
    );

    if (result != VK_SUCCESS)
    {
        setError("Could not create Vulkan surface");
        return nullptr;
    }

    wd->swap = gpu->createSwapchain(surface, wd->width, wd->height, config.preferredPresentMode, config.imageUsage);

    return window;
}

bool InputDeviceWayland::isButtonDown(Button button)
{
    return isKeyDown[static_cast<u32>(button)];
}

bool InputDeviceWayland::wasButtonPressed(Button button)
{
    return !wasKeyDown[static_cast<u32>(button)] && isKeyDown[static_cast<u32>(button)];
}

bool InputDeviceWayland::wasButtonReleased(Button button)
{
    return wasKeyDown[static_cast<u32>(button)] && !isKeyDown[static_cast<u32>(button)];
}

Vec2 InputDeviceWayland::mousePos()
{
    if (activeWindow != nullptr)
        return activeWindow->mouse;

    if (windows.size() == 1)
    {
        Vec2 ret;
        windows.forEach([&](u64, WindowWayland* wd)
        {
            ret = wd->mouse;
        });
        return ret;
    }

    return {};
}

Vec2 InputDeviceWayland::mouseDelta()
{
    if (activeWindow != nullptr)
        return mouseDeltaAccum / assertCast<f32>(activeWindow->height);

    if (windows.size() == 1)
    {
        Vec2 ret = mouseDeltaAccum;
        windows.forEach([&](u64, WindowWayland* wd)
        {
            ret = ret / assertCast<f32>(wd->height);
        });
        return ret;
    }

    return mouseDeltaAccum;
}

Vec2 InputDeviceWayland::wheelDelta()
{
    return wheelDeltaAccum;
}

void InputDeviceWayland::setCursor(CursorType type)
{
    (void)type;
    // Wayland cursor handling would require wl_cursor_theme.
    // For now, let the compositor handle the default cursor.
}

void InputDeviceWayland::showCursor(bool show)
{
    (void)show;
    // Wayland cursor visibility is handled by the compositor.
}

std::unique_ptr<InputDeviceWayland> InputDeviceWayland::create()
{
    if (!loadWayland())
        return nullptr;

    std::unique_ptr<InputDeviceWayland> device = std::make_unique<InputDeviceWayland>();
    wlDevice = device.get();

    device->display = wlFuncs.wl_display_connect(nullptr);
    if (device->display == nullptr)
    {
        setError("Could not connect to Wayland display");
        wlDevice = nullptr;
        return nullptr;
    }

    device->xkbContext = xkbFuncs.xkb_context_new(static_cast<xkb_context_flags>(0));
    if (device->xkbContext == nullptr)
    {
        setError("Could not create XKB context");
        wlFuncs.wl_display_disconnect(device->display);
        wlDevice = nullptr;
        return nullptr;
    }

    struct wl_registry* registry = wl_display_get_registry(device->display);
    struct wl_registry_listener* listenerPtr = &registryListener;
    wlFuncs.wl_proxy_add_listener(
        reinterpret_cast<struct wl_proxy*>(registry),
        reinterpret_cast<void(**)(void)>(listenerPtr), nullptr);

    wlFuncs.wl_display_roundtrip(device->display);
    wlFuncs.wl_display_roundtrip(device->display);

    if (device->compositor == nullptr)
    {
        setError("Wayland compositor not available");
        wlFuncs.wl_display_disconnect(device->display);
        wlDevice = nullptr;
        return nullptr;
    }

    if (device->xdgWmBase != nullptr)
        xdg_wm_base_add_listener(device->xdgWmBase, &xdgWmBaseListener, nullptr);

    wlFuncs.wl_display_roundtrip(device->display);

    if (device->dataDeviceManager != nullptr)
    {
        device->dataDevice = wl_data_device_manager_get_data_device(
            device->dataDeviceManager, device->seat);
        if (device->dataDevice != nullptr)
            wl_data_device_add_listener(device->dataDevice, &dataDeviceListener, nullptr);
    }

    device->globalsReady = true;

    return device;
}

InputDeviceWayland::~InputDeviceWayland()
{
    wlDevice = nullptr;

    if (clipboardFd >= 0)
    {
        close(clipboardFd);
        clipboardFd = -1;
    }
    if (pendingOffer != nullptr)
    {
        wl_data_offer_destroy(pendingOffer);
        pendingOffer = nullptr;
    }

    if (xkbState != nullptr)
        xkbFuncs.xkb_state_unref(xkbState);
    if (xkbKeymap != nullptr)
        xkbFuncs.xkb_keymap_unref(xkbKeymap);
    if (xkbContext != nullptr)
        xkbFuncs.xkb_context_unref(xkbContext);

    if (dataDevice != nullptr)
        wl_data_device_destroy(dataDevice);
    if (dataDeviceManager != nullptr)
        wl_data_device_manager_destroy(dataDeviceManager);

    if (keyboard != nullptr)
        wl_keyboard_destroy(keyboard);
    if (pointer != nullptr)
        wl_pointer_destroy(pointer);
    if (seat != nullptr)
        wl_seat_destroy(seat);

    for (u32 i = 0; i < outputCount; i++)
    {
        if (outputs[i] != nullptr)
            wl_output_destroy(outputs[i]);
    }

    if (xdgWmBase != nullptr)
        xdg_wm_base_destroy(xdgWmBase);
    if (shm != nullptr)
        wl_shm_destroy(shm);
    if (compositor != nullptr)
        wl_compositor_destroy(compositor);

    if (display != nullptr)
        wlFuncs.wl_display_disconnect(display);
}

WindowWayland::~WindowWayland()
{
    if (wlSurface != nullptr)
    {
        static_cast<InputDeviceWayland*>(device)->windows.remove(surfaceKey(wlSurface));

        if (xdgToplevel != nullptr)
            xdg_toplevel_destroy(xdgToplevel);
        if (xdgSurface != nullptr)
            xdg_surface_destroy(xdgSurface);
        if (wlSurface != nullptr)
            wl_surface_destroy(wlSurface);
    }
}

GpuSwapchain* WindowWayland::swapchain() const
{
    return swap.get();
}

void WindowWayland::setTitle(std::string_view title)
{
    char buf[256];
    u64 len = title.length() < sizeof(buf) - 1 ? title.length() : sizeof(buf) - 1;
    std::memcpy(buf, title.data(), len);
    buf[len] = '\0';
    xdg_toplevel_set_title(xdgToplevel, buf);
}

std::span<const Event> WindowWayland::getEvents() const
{
    return eventQueue;
}

bool WindowWayland::wasClosed() const
{
    return closed;
}

bool WindowWayland::isFocused() const
{
    return focused;
}

bool WindowWayland::wasFocusGained() const
{
    return focusGained;
}

bool WindowWayland::wasFocusLost() const
{
    return focusLost;
}

bool WindowWayland::wasResized() const
{
    return resized;
}

void WindowWayland::size(u32* outWidth, u32* outHeight) const
{
    if (outWidth != nullptr)
        *outWidth = width;
    if (outHeight != nullptr)
        *outHeight = height;
}

void WindowWayland::setFullscreen(bool set)
{
    if (set)
        xdg_toplevel_set_fullscreen(xdgToplevel, nullptr);
    else
        xdg_toplevel_unset_fullscreen(xdgToplevel);
}

Vec2 WindowWayland::mousePos() const
{
    return mouse;
}

Vec2 WindowWayland::mouseDelta() const
{
    return static_cast<InputDeviceWayland*>(device)->mouseDeltaAccum / assertCast<f32>(height);
}

} // namespace hg
