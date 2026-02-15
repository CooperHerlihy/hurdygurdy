#include "hurdygurdy.hpp"

#ifdef HG_PLATFORM_LINUX

#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <vulkan/vulkan_xlib.h>

struct HgWindowInput {
    u32 width;
    u32 height;
    f64 mouse_pos_x;
    f64 mouse_pos_y;
    f64 mouse_delta_x;
    f64 mouse_delta_y;
    bool was_resized;
    bool was_closed;
    bool keys_down[(u32)HgKey::count];
    bool keys_pressed[(u32)HgKey::count];
    bool keys_released[(u32)HgKey::count];
};

struct HgWindow::Internals {
    HgWindowInput input;
    Window x11_window;
    Atom delete_atom;
};

bool HgWindow::was_closed() {
    return internals->input.was_closed;
}

bool HgWindow::was_resized() {
    return internals->input.was_resized;
}

void HgWindow::get_size(u32* width, u32* height) {
    *width = internals->input.width;
    *height = internals->input.height;
}

void HgWindow::get_mouse_pos(f64& x, f64& y) {
    x = internals->input.mouse_pos_x;
    y = internals->input.mouse_pos_y;
}

void HgWindow::get_mouse_delta(f64& x, f64& y) {
    x = internals->input.mouse_delta_x;
    y = internals->input.mouse_delta_y;
}

bool HgWindow::is_key_down(HgKey key) {
    hg_assert((u32)key > (u32)HgKey::none && (u32)key < (u32)HgKey::count);
    return internals->input.keys_down[(u32)key];
}

bool HgWindow::was_key_pressed(HgKey key) {
    hg_assert((u32)key > (u32)HgKey::none && (u32)key < (u32)HgKey::count);
    return internals->input.keys_pressed[(u32)key];
}

bool HgWindow::was_key_released(HgKey key) {
    hg_assert((u32)key > (u32)HgKey::none && (u32)key < (u32)HgKey::count);
    return internals->input.keys_released[(u32)key];
}

struct HgX11Funcs {
    Display* (*XOpenDisplay)(_Xconst char*);
    int (*XCloseDisplay)(Display*);
    Window (*XCreateWindow)(Display*, Window, int, int, unsigned int, unsigned int,
        unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*);
    int (*XDestroyWindow)(Display*, Window);
    int (*XStoreName)(Display*, Window, _Xconst char*);
    Atom (*XInternAtom)(Display*, _Xconst char*, Bool);
    Status (*XSetWMProtocols)(Display*, Window, Atom*, int);
    int (*XMapWindow)(Display*, Window);
    Status (*XSendEvent)(Display*, Window, Bool, long, XEvent*);
    int (*XFlush)(Display*);
    int (*XNextEvent)(Display*, XEvent*);
    int (*XPending)(Display*);
    KeySym (*XLookupKeysym)(XKeyEvent*, int);
};

static void* hg_internal_libx11 = nullptr;
static HgX11Funcs hg_internal_x11_funcs{};

Display* XOpenDisplay(_Xconst char* name) {
    return hg_internal_x11_funcs.XOpenDisplay(name);
}

int XCloseDisplay(Display* dpy) {
    return hg_internal_x11_funcs.XCloseDisplay(dpy);
}

Window XCreateWindow(
    Display* dpy,
    Window parent,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int border_width,
    int depth,
    unsigned int xclass,
    Visual* visual,
    unsigned long valuemask,
    XSetWindowAttributes* attributes
) {
    return hg_internal_x11_funcs.XCreateWindow(
        dpy, parent, x, y, width, height, border_width,
        depth, xclass, visual, valuemask, attributes
    );
}

int XDestroyWindow(Display* dpy, Window w) {
    return hg_internal_x11_funcs.XDestroyWindow(dpy, w);
}

int XStoreName(Display* dpy, Window w, _Xconst char* name) {
    return hg_internal_x11_funcs.XStoreName(dpy, w, name);
}

Atom XInternAtom(Display* dpy, _Xconst char* name, Bool only_if_exists) {
    return hg_internal_x11_funcs.XInternAtom(dpy, name, only_if_exists);
}

Status XSetWMProtocols(Display* dpy, Window w, Atom* protocols, int count) {
    return hg_internal_x11_funcs.XSetWMProtocols(dpy, w, protocols, count);
}

int XMapWindow(Display* dpy, Window w) {
    return hg_internal_x11_funcs.XMapWindow(dpy, w);
}

Status XSendEvent(Display* dpy, Window w, Bool propagate, long event_mask, XEvent* event) {
    return hg_internal_x11_funcs.XSendEvent(dpy, w, propagate, event_mask, event);
}

int XFlush(Display* dpy) {
    return hg_internal_x11_funcs.XFlush(dpy);
}

int XNextEvent(Display* dpy, XEvent* event) {
    return hg_internal_x11_funcs.XNextEvent(dpy, event);
}

int XPending(Display* dpy) {
    return hg_internal_x11_funcs.XPending(dpy);
}

KeySym XLookupKeysym(XKeyEvent* key_event, int index) {
    return hg_internal_x11_funcs.XLookupKeysym(key_event, index);
}

#define HG_LOAD_X11_FUNC(name)* (void**)&hg_internal_x11_funcs. name \
    = dlsym(hg_internal_libx11, #name); \
    if (hg_internal_x11_funcs. name == nullptr) { hg_error("Could not load Xlib function: \n" #name); }

Display* hg_internal_x11_display = nullptr;

void hg_platform_init() {
    if (hg_internal_libx11 == nullptr)
        hg_internal_libx11 = dlopen("libX11.so.6", RTLD_LAZY);
    if (hg_internal_libx11 == nullptr)
        hg_error("Could not open Xlib\n");

    HG_LOAD_X11_FUNC(XOpenDisplay);
    HG_LOAD_X11_FUNC(XCloseDisplay);
    HG_LOAD_X11_FUNC(XCreateWindow);
    HG_LOAD_X11_FUNC(XDestroyWindow);
    HG_LOAD_X11_FUNC(XStoreName);
    HG_LOAD_X11_FUNC(XInternAtom);
    HG_LOAD_X11_FUNC(XSetWMProtocols);
    HG_LOAD_X11_FUNC(XMapWindow);
    HG_LOAD_X11_FUNC(XSendEvent);
    HG_LOAD_X11_FUNC(XFlush);
    HG_LOAD_X11_FUNC(XPending);
    HG_LOAD_X11_FUNC(XNextEvent);
    HG_LOAD_X11_FUNC(XLookupKeysym);

    if (hg_internal_x11_display == nullptr)
        hg_internal_x11_display = XOpenDisplay(nullptr);
    if (hg_internal_x11_display == nullptr)
        hg_error("Could not open X display\n");
}

void hg_platform_deinit() {
    if (hg_internal_x11_display != nullptr) {
        XCloseDisplay(hg_internal_x11_display);
        hg_internal_x11_display = nullptr;
    }
    if (hg_internal_libx11 != nullptr) {
        dlclose(hg_internal_libx11);
        hg_internal_libx11 = nullptr;
    }
}

static Window hg_internal_create_x11_window(
    Display* display,
    u32 width,
    u32 height,
    const char* title
) {
    XSetWindowAttributes window_attributes{};
    window_attributes.event_mask
        = KeyPressMask | KeyReleaseMask
        | ButtonPressMask
        | ButtonReleaseMask
        | PointerMotionMask
        | StructureNotifyMask;
    Window window = XCreateWindow(
        display, RootWindow(display, DefaultScreen(display)),
        0, 0,
        width,
        height,
        1,
        CopyFromParent,
        InputOutput,
        CopyFromParent,
        CWEventMask,
        &window_attributes
    );
    if (window == ~0U)
        hg_error("X11 could not create window\n");

    if (title != nullptr) {
        int name_result = XStoreName(display, window, title);
        if (name_result == 0)
            hg_error("X11 could not set window title\n");
    }

    int map_result = XMapWindow(display, window);
    if (map_result == 0)
        hg_error("X11 could not map window\n");

    return window;
}

static Atom hg_internal_set_delete_behavior(
    Display* display,
    Window window
) {
    Atom delete_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
    if (delete_atom == None)
        hg_error("X11 could not get WM_DELETE_WINDOW atom\n");

    int set_protocols_result = XSetWMProtocols(
        display,
        window,
        &delete_atom,
        1
    );
    if (set_protocols_result == 0)
        hg_error("X11 could not set WM_DELETE_WINDOW protocol\n");

    return delete_atom;
}

static void hg_internal_set_fullscreen(
    Display* display,
    Window window
) {
    Atom state_atom = XInternAtom(display, "_NET_WM_STATE", False);
    if (state_atom == None)
        hg_error("X11 failed to get state atom\n");

    Atom fullscreen_atom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    if (fullscreen_atom == None)
        hg_error("X11 failed to get fullscreen atom\n");

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = state_atom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1;
    event.xclient.data.l[1] = (long)fullscreen_atom;

    int fullscreen_result = XSendEvent(
        display,
        RootWindow(display, DefaultScreen(display)),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &event
    );
    if (fullscreen_result == 0)
        hg_error("X11 could not send fullscreen message\n");
}

HgWindow HgWindow::create(HgArena& arena, const HgWindow::Config& config) {
    u32 width = config.windowed ? config.width
        : (u32)DisplayWidth(hg_internal_x11_display, DefaultScreen(hg_internal_x11_display));
    u32 height = config.windowed ? config.height
        : (u32)DisplayHeight(hg_internal_x11_display, DefaultScreen(hg_internal_x11_display));

    HgWindow window;
    window.internals = arena.alloc<Internals>(1);
    *window.internals = {};

    window.internals->input.width = width;
    window.internals->input.height = height;

    window.internals->x11_window = hg_internal_create_x11_window(
        hg_internal_x11_display, width, height, config.title);
    window.internals->delete_atom = hg_internal_set_delete_behavior(
        hg_internal_x11_display, window.internals->x11_window);

    if (!config.windowed)
        hg_internal_set_fullscreen(hg_internal_x11_display, window.internals->x11_window);

    int flush_result = XFlush(hg_internal_x11_display);
    if (flush_result == 0)
        hg_error("X11 could not flush window\n");

    return window;
}

void HgWindow::destroy() {
    XDestroyWindow(hg_internal_x11_display, internals->x11_window);
    XFlush(hg_internal_x11_display);
}

void HgWindow::set_icon(u32* icon_data, u32 width, u32 height) {
    hg_error("window set_icon : TODO\n");
    (void)icon_data;
    (void)width;
    (void)height;
}

bool HgWindow::is_fullscreen() {
    hg_error("window is_fullscreen : TODO\n");
}

void HgWindow::set_fullscreen(bool fullscreen) {
    hg_error("window set_fullscreen : TODO\n");
    (void)fullscreen;
}

void HgWindow::set_cursor(HgWindow::Cursor cursor) {
    hg_error("window set_cursor : TODO\n");
    (void)cursor;
}

void HgWindow::set_cursor_image(u32* data, u32 width, u32 height) {
    hg_error("window set_cursor_image : TODO\n");
    (void)data;
    (void)width;
    (void)height;
}

VkSurfaceKHR hg_vk_create_surface(VkInstance instance, HgWindow window) {
    hg_assert(instance != nullptr);
    hg_assert(window.internals != nullptr);

    PFN_vkCreateXlibSurfaceKHR pfn_vkCreateXlibSurfaceKHR
        = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");
    if (pfn_vkCreateXlibSurfaceKHR == nullptr)
        hg_error("Could not load vkCreateXlibSurfaceKHR\n");

    VkXlibSurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    info.dpy = hg_internal_x11_display;
    info.window = window.internals->x11_window;

    VkSurfaceKHR surface = nullptr;
    VkResult result = pfn_vkCreateXlibSurfaceKHR(instance, &info, nullptr, &surface);
    if (surface == nullptr)
        hg_error("Failed to create Vulkan surface: %s\n", hg_vk_result_string(result));

    return surface;
}

void hg_process_window_events(const HgWindow* windows, usize window_count) {
    hg_assert(windows != nullptr);

    if (window_count != 1)
        hg_error("Multiple windows unsupported\n"); // : TODO
    HgWindow window = windows[0];

    std::memset(window.internals->input.keys_pressed, 0, sizeof(window.internals->input.keys_pressed));
    std::memset(window.internals->input.keys_released, 0, sizeof(window.internals->input.keys_released));
    window.internals->input.was_resized = false;

    u32 old_window_width = window.internals->input.width;
    u32 old_window_height = window.internals->input.height;
    f64 old_mouse_pos_x = window.internals->input.mouse_pos_x;
    f64 old_mouse_pos_y = window.internals->input.mouse_pos_y;

    while (XPending(hg_internal_x11_display)) {
        XEvent event;
        int event_result = XNextEvent(hg_internal_x11_display, &event);
        if (event_result != 0)
            hg_error("X11 could not get next event\n");

        switch (event.type) {
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == window.internals->delete_atom)
                    window.internals->input.was_closed = true;
                break;
            case ConfigureNotify:
                window.internals->input.width = (u32)event.xconfigure.width;
                window.internals->input.height = (u32)event.xconfigure.height;
                break;
            case KeyPress:
            case KeyRelease: {
                HgKey key = HgKey::none;
                switch (XLookupKeysym(&event.xkey, 0)) {
                    case XK_0:
                        key = HgKey::k0;
                        break;
                    case XK_1:
                        key = HgKey::k1;
                        break;
                    case XK_2:
                        key = HgKey::k2;
                        break;
                    case XK_3:
                        key = HgKey::k3;
                        break;
                    case XK_4:
                        key = HgKey::k4;
                        break;
                    case XK_5:
                        key = HgKey::k5;
                        break;
                    case XK_6:
                        key = HgKey::k6;
                        break;
                    case XK_7:
                        key = HgKey::k7;
                        break;
                    case XK_8:
                        key = HgKey::k8;
                        break;
                    case XK_9:
                        key = HgKey::k9;
                        break;

                    case XK_q:
                    case XK_Q:
                        key = HgKey::q;
                        break;
                    case XK_w:
                    case XK_W:
                        key = HgKey::w;
                        break;
                    case XK_e:
                    case XK_E:
                        key = HgKey::e;
                        break;
                    case XK_r:
                    case XK_R:
                        key = HgKey::r;
                        break;
                    case XK_t:
                    case XK_T:
                        key = HgKey::t;
                        break;
                    case XK_y:
                    case XK_Y:
                        key = HgKey::y;
                        break;
                    case XK_u:
                    case XK_U:
                        key = HgKey::u;
                        break;
                    case XK_i:
                    case XK_I:
                        key = HgKey::i;
                        break;
                    case XK_o:
                    case XK_O:
                        key = HgKey::o;
                        break;
                    case XK_p:
                    case XK_P:
                        key = HgKey::p;
                        break;
                    case XK_a:
                    case XK_A:
                        key = HgKey::a;
                        break;
                    case XK_s:
                    case XK_S:
                        key = HgKey::s;
                        break;
                    case XK_d:
                    case XK_D:
                        key = HgKey::d;
                        break;
                    case XK_f:
                    case XK_F:
                        key = HgKey::f;
                        break;
                    case XK_g:
                    case XK_G:
                        key = HgKey::g;
                        break;
                    case XK_h:
                    case XK_H:
                        key = HgKey::h;
                        break;
                    case XK_j:
                    case XK_J:
                        key = HgKey::j;
                        break;
                    case XK_k:
                    case XK_K:
                        key = HgKey::k;
                        break;
                    case XK_l:
                    case XK_L:
                        key = HgKey::l;
                        break;
                    case XK_z:
                    case XK_Z:
                        key = HgKey::z;
                        break;
                    case XK_x:
                    case XK_X:
                        key = HgKey::x;
                        break;
                    case XK_c:
                    case XK_C:
                        key = HgKey::c;
                        break;
                    case XK_v:
                    case XK_V:
                        key = HgKey::v;
                        break;
                    case XK_b:
                    case XK_B:
                        key = HgKey::b;
                        break;
                    case XK_n:
                    case XK_N:
                        key = HgKey::n;
                        break;
                    case XK_m:
                    case XK_M:
                        key = HgKey::m;
                        break;

                    case XK_semicolon:
                        key = HgKey::semicolon;
                        break;
                    case XK_colon:
                        key = HgKey::colon;
                        break;
                    case XK_apostrophe:
                        key = HgKey::apostrophe;
                        break;
                    case XK_quotedbl:
                        key = HgKey::quotation;
                        break;
                    case XK_comma:
                        key = HgKey::comma;
                        break;
                    case XK_period:
                        key = HgKey::period;
                        break;
                    case XK_question:
                        key = HgKey::question;
                        break;
                    case XK_grave:
                        key = HgKey::grave;
                        break;
                    case XK_asciitilde:
                        key = HgKey::tilde;
                        break;
                    case XK_exclam:
                        key = HgKey::exclamation;
                        break;
                    case XK_at:
                        key = HgKey::at;
                        break;
                    case XK_numbersign:
                        key = HgKey::hash;
                        break;
                    case XK_dollar:
                        key = HgKey::dollar;
                        break;
                    case XK_percent:
                        key = HgKey::percent;
                        break;
                    case XK_asciicircum:
                        key = HgKey::carot;
                        break;
                    case XK_ampersand:
                        key = HgKey::ampersand;
                        break;
                    case XK_asterisk:
                        key = HgKey::asterisk;
                        break;
                    case XK_parenleft:
                        key = HgKey::lparen;
                        break;
                    case XK_parenright:
                        key = HgKey::rparen;
                        break;
                    case XK_bracketleft:
                        key = HgKey::lbracket;
                        break;
                    case XK_bracketright:
                        key = HgKey::rbracket;
                        break;
                    case XK_braceleft:
                        key = HgKey::lbrace;
                        break;
                    case XK_braceright:
                        key = HgKey::rbrace;
                        break;
                    case XK_equal:
                        key = HgKey::equal;
                        break;
                    case XK_less:
                        key = HgKey::less;
                        break;
                    case XK_greater:
                        key = HgKey::greater;
                        break;
                    case XK_plus:
                        key = HgKey::plus;
                        break;
                    case XK_minus:
                        key = HgKey::minus;
                        break;
                    case XK_slash:
                        key = HgKey::slash;
                        break;
                    case XK_backslash:
                        key = HgKey::backslash;
                        break;
                    case XK_underscore:
                        key = HgKey::underscore;
                        break;
                    case XK_bar:
                        key = HgKey::bar;
                        break;

                    case XK_Up:
                        key = HgKey::up;
                        break;
                    case XK_Down:
                        key = HgKey::down;
                        break;
                    case XK_Left:
                        key = HgKey::left;
                        break;
                    case XK_Right:
                        key = HgKey::right;
                        break;
                    case XK_Escape:
                        key = HgKey::escape;
                        break;
                    case XK_space:
                        key = HgKey::space;
                        break;
                    case XK_Return:
                        key = HgKey::enter;
                        break;
                    case XK_BackSpace:
                        key = HgKey::backspace;
                        break;
                    case XK_Delete:
                        key = HgKey::kdelete;
                        break;
                    case XK_Insert:
                        key = HgKey::insert;
                        break;
                    case XK_Tab:
                        key = HgKey::tab;
                        break;
                    case XK_Home:
                        key = HgKey::home;
                        break;
                    case XK_End:
                        key = HgKey::end;
                        break;

                    case XK_F1:
                        key = HgKey::f1;
                        break;
                    case XK_F2:
                        key = HgKey::f2;
                        break;
                    case XK_F3:
                        key = HgKey::f3;
                        break;
                    case XK_F4:
                        key = HgKey::f4;
                        break;
                    case XK_F5:
                        key = HgKey::f5;
                        break;
                    case XK_F6:
                        key = HgKey::f6;
                        break;
                    case XK_F7:
                        key = HgKey::f7;
                        break;
                    case XK_F8:
                        key = HgKey::f8;
                        break;
                    case XK_F9:
                        key = HgKey::f9;
                        break;
                    case XK_F10:
                        key = HgKey::f10;
                        break;
                    case XK_F11:
                        key = HgKey::f11;
                        break;
                    case XK_F12:
                        key = HgKey::f12;
                        break;

                    case XK_Shift_L:
                        key = HgKey::lshift;
                        break;
                    case XK_Shift_R:
                        key = HgKey::rshift;
                        break;
                    case XK_Control_L:
                        key = HgKey::lctrl;
                        break;
                    case XK_Control_R:
                        key = HgKey::rctrl;
                        break;
                    case XK_Meta_L:
                        key = HgKey::lmeta;
                        break;
                    case XK_Meta_R:
                        key = HgKey::rmeta;
                        break;
                    case XK_Alt_L:
                        key = HgKey::lalt;
                        break;
                    case XK_Alt_R:
                        key = HgKey::ralt;
                        break;
                    case XK_Super_L:
                        key = HgKey::lsuper;
                        break;
                    case XK_Super_R:
                        key = HgKey::rsuper;
                        break;
                    case XK_Caps_Lock:
                        key = HgKey::capslock;
                        break;
                }
                if (event.type == KeyPress) {
                    window.internals->input.keys_pressed[(u32)key] = true;
                    window.internals->input.keys_down[(u32)key] = true;
                } else if (event.type == KeyRelease) {
                    window.internals->input.keys_released[(u32)key] = true;
                    window.internals->input.keys_down[(u32)key] = false;
                }
            } break;
            case ButtonPress:
            case ButtonRelease: {
                HgKey key = HgKey::none;
                switch (event.xbutton.button) {
                    case Button1:
                        key = HgKey::mouse1;
                        break;
                    case Button2:
                        key = HgKey::mouse2;
                        break;
                    case Button3:
                        key = HgKey::mouse3;
                        break;
                    case Button4:
                        key = HgKey::mouse4;
                        break;
                    case Button5:
                        key = HgKey::mouse5;
                        break;
                }
                if (event.type == ButtonPress) {
                    window.internals->input.keys_pressed[(u32)key] = true;
                    window.internals->input.keys_down[(u32)key] = true;
                } else if (event.type == ButtonRelease) {
                    window.internals->input.keys_released[(u32)key] = true;
                    window.internals->input.keys_down[(u32)key] = false;
                }
            } break;
            case MotionNotify:
                window.internals->input.mouse_pos_x = (f64)event.xmotion.x / (f64)window.internals->input.height;
                window.internals->input.mouse_pos_y = (f64)event.xmotion.y / (f64)window.internals->input.height;
                break;
            default:
                break;
        }
    }

    if (window.internals->input.width != old_window_width || window.internals->input.height != old_window_height) {
        window.internals->input.was_resized = true;
    }

    window.internals->input.mouse_delta_x = window.internals->input.mouse_pos_x - old_mouse_pos_x;
    window.internals->input.mouse_delta_y = window.internals->input.mouse_pos_y - old_mouse_pos_y;
}

#endif
