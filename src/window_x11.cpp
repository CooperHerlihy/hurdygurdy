#include "hurdygurdy.hpp"

#ifdef HG_PLATFORM_LINUX

#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <vulkan/vulkan_xlib.h>

void hgInternalCreateWindowSwapchain(HgWindow* window, const HgWindowConfig& config);
void hgInternalResizeWindowSwapchain(HgWindow* window);
void hgInternalDestroyWindowSwapchain(HgWindow* window);

struct HgWindow::Internals {
    Window x11Window;
    Atom deleteAtom;
};

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

static void* libx11 = nullptr;
static HgX11Funcs x11Funcs{};

Display* XOpenDisplay(_Xconst char* name)
{
    return x11Funcs.XOpenDisplay(name);
}

int XCloseDisplay(Display* dpy)
{
    return x11Funcs.XCloseDisplay(dpy);
}

Window XCreateWindow(
    Display* dpy,
    Window parent,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int borderWidth,
    int depth,
    unsigned int xclass,
    Visual* visual,
    unsigned long valuemask,
    XSetWindowAttributes* attributes)
{
    return x11Funcs.XCreateWindow(
        dpy, parent, x, y, width, height, borderWidth,
        depth, xclass, visual, valuemask, attributes
    );
}

int XDestroyWindow(Display* dpy, Window w)
{
    return x11Funcs.XDestroyWindow(dpy, w);
}

int XStoreName(Display* dpy, Window w, _Xconst char* name)
{
    return x11Funcs.XStoreName(dpy, w, name);
}

Atom XInternAtom(Display* dpy, _Xconst char* name, Bool onlyIfExists)
{
    return x11Funcs.XInternAtom(dpy, name, onlyIfExists);
}

Status XSetWMProtocols(Display* dpy, Window w, Atom* protocols, int count)
{
    return x11Funcs.XSetWMProtocols(dpy, w, protocols, count);
}

int XMapWindow(Display* dpy, Window w)
{
    return x11Funcs.XMapWindow(dpy, w);
}

Status XSendEvent(Display* dpy, Window w, Bool propagate, long eventMask, XEvent* event)
{
    return x11Funcs.XSendEvent(dpy, w, propagate, eventMask, event);
}

int XFlush(Display* dpy)
{
    return x11Funcs.XFlush(dpy);
}

int XNextEvent(Display* dpy, XEvent* event)
{
    return x11Funcs.XNextEvent(dpy, event);
}

int XPending(Display* dpy)
{
    return x11Funcs.XPending(dpy);
}

KeySym XLookupKeysym(XKeyEvent* keyEvent, int index)
{
    return x11Funcs.XLookupKeysym(keyEvent, index);
}

#define HG_LOAD_X11_FUNC(name)* (void**)&x11Funcs. name \
    = dlsym(libx11, #name); \
    if (x11Funcs. name == nullptr) { hgError("Could not load Xlib function: \n" #name); }

Display* x11Display = nullptr;

void hgInitPlatform()
{
    if (libx11 == nullptr)
        libx11 = dlopen("libX11.so.6", RTLD_LAZY);
    if (libx11 == nullptr)
        hgError("Could not open Xlib\n");

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

    if (x11Display == nullptr)
        x11Display = XOpenDisplay(nullptr);
    if (x11Display == nullptr)
        hgError("Could not open X display\n");
}

void hgDeinitPlatform()
{
    if (x11Display != nullptr)
    {
        XCloseDisplay(x11Display);
        x11Display = nullptr;
    }
    if (libx11 != nullptr)
    {
        dlclose(libx11);
        libx11 = nullptr;
    }
}

static Window createX11Window(Display* display, u32 width, u32 height, const char* title) {
    XSetWindowAttributes windowAttributes{};
    windowAttributes.event_mask
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
        &windowAttributes
    );
    if (window == ~0U)
        hgError("X11 could not create window\n");

    if (title != nullptr)
    {
        int nameResult = XStoreName(display, window, title);
        if (nameResult == 0)
            hgError("X11 could not set window title\n");
    }

    int mapResult = XMapWindow(display, window);
    if (mapResult == 0)
        hgError("X11 could not map window\n");

    return window;
}

static Atom setX11DeleteBehavior(Display* display, Window window)
{
    Atom deleteAtom = XInternAtom(display, "WM_DELETE_WINDOW", False);
    if (deleteAtom == None)
        hgError("X11 could not get WM_DELETE_WINDOW atom\n");

    int setProtocolsResult = XSetWMProtocols(
        display,
        window,
        &deleteAtom,
        1
    );
    if (setProtocolsResult == 0)
        hgError("X11 could not set WM_DELETE_WINDOW protocol\n");

    return deleteAtom;
}

static void setX11Fullscreen(Display* display, Window window)
{
    Atom stateAtom = XInternAtom(display, "_NET_WM_STATE", False);
    if (stateAtom == None)
        hgError("X11 failed to get state atom\n");

    Atom fullscreenAtom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    if (fullscreenAtom == None)
        hgError("X11 failed to get fullscreen atom\n");

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = stateAtom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1;
    event.xclient.data.l[1] = (long)fullscreenAtom;

    int fullscreenResult = XSendEvent(
        display,
        RootWindow(display, DefaultScreen(display)),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &event
    );
    if (fullscreenResult == 0)
        hgError("X11 could not send fullscreen message\n");
}

HgWindow* HgWindow::create(HgArena* arena, const HgWindowConfig& config)
{
    HgWindow* window = hgAlloc<HgWindow>(arena, 1);
    *window = {};
    window->internals = hgAlloc<Internals>(arena, 1);
    *window->internals = {};

    window->width = config.windowed ? config.width : (u32)DisplayWidth(x11Display, DefaultScreen(x11Display));
    window->height = config.windowed ? config.height : (u32)DisplayHeight(x11Display, DefaultScreen(x11Display));

    window->internals->x11Window = createX11Window(
        x11Display, window->width, window->height, config.title);
    window->internals->deleteAtom = setX11DeleteBehavior(
        x11Display, window->internals->x11Window);

    if (!config.windowed)
        setX11Fullscreen(x11Display, window->internals->x11Window);

    int flushResult = XFlush(x11Display);
    if (flushResult == 0)
        hgError("X11 could not flush window\n");

    PFN_vkCreateXlibSurfaceKHR pfnVkCreateXlibSurfaceKHR
        = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(hgVkInstance, "vkCreateXlibSurfaceKHR");
    if (pfnVkCreateXlibSurfaceKHR == nullptr)
        hgError("Could not load vkCreateXlibSurfaceKHR\n");

    VkXlibSurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    info.dpy = x11Display;
    info.window = window->internals->x11Window;

    VkResult result = pfnVkCreateXlibSurfaceKHR(hgVkInstance, &info, nullptr, &window->surface);
    if (window->surface == nullptr)
        hgError("Failed to create Vulkan surface: %s\n", hgVkResultToStr(result));

    hgInternalCreateWindowSwapchain(window, config);

    return window;
}

void HgWindow::destroy()
{
    hgInternalDestroyWindowSwapchain(this);
    vkDestroySurfaceKHR(hgVkInstance, surface, nullptr);
    XDestroyWindow(x11Display, internals->x11Window);
    XFlush(x11Display);
}

void HgWindow::setIcon(u32* iconData, u32 width, u32 height)
{
    hgError("window setIcon : TODO\n");
    (void)iconData;
    (void)width;
    (void)height;
}

bool HgWindow::isFullscreen()
{
    hgError("window isFullscreen : TODO\n");
}

void HgWindow::setFullscreen(bool fullscreen)
{
    hgError("window setFullscreen : TODO\n");
    (void)fullscreen;
}

void HgWindow::setCursor(HgWindow::Cursor cursor)
{
    hgError("window setCursor : TODO\n");
    (void)cursor;
}

void HgWindow::setCursorImage(u32* data, u32 width, u32 height)
{
    hgError("window setCursorImage : TODO\n");
    (void)data;
    (void)width;
    (void)height;
}

u32 hgVkGetPlatformExtensions(HgArena* arena, HgStringView** extBuffer)
{
    u32 count = 2;
    *extBuffer = hgAlloc<HgStringView>(arena, count);
    (*extBuffer)[0] = "VK_KHR_surface";
    (*extBuffer)[1] = "VK_KHR_xlib_surface";
    return count;
}

void hgProcessWindowEvents(HgWindow** windows, u32 windowCount)
{
    hgAssert(windows != nullptr);

    if (windowCount != 1)
        hgError("Multiple windows unsupported\n"); // : TODO
    HgWindow* window = windows[0];

    memset(window->wasKeyPressed, 0, sizeof(window->wasKeyPressed));
    memset(window->wasKeyReleased, 0, sizeof(window->wasKeyReleased));

    u32 oldWidth = window->width;
    u32 oldHeight = window->height;
    f64 oldMousePosX = window->mousePosX;
    f64 oldMousePosY = window->mousePosY;

    while (XPending(x11Display))
    {
        XEvent event;
        int eventResult = XNextEvent(x11Display, &event);
        if (eventResult != 0)
            hgError("X11 could not get next event\n");

        switch (event.type)
        {
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == window->internals->deleteAtom)
                    window->wasClosed = true;
                break;
            case ConfigureNotify:
                window->width = (u32)event.xconfigure.width;
                window->height = (u32)event.xconfigure.height;
                break;
            case KeyPress:
            case KeyRelease: {
                HgKey key = HgKey_none;
                switch (XLookupKeysym(&event.xkey, 0))
                {
                    case XK_0:
                        key = HgKey_k0;
                        break;
                    case XK_1:
                        key = HgKey_k1;
                        break;
                    case XK_2:
                        key = HgKey_k2;
                        break;
                    case XK_3:
                        key = HgKey_k3;
                        break;
                    case XK_4:
                        key = HgKey_k4;
                        break;
                    case XK_5:
                        key = HgKey_k5;
                        break;
                    case XK_6:
                        key = HgKey_k6;
                        break;
                    case XK_7:
                        key = HgKey_k7;
                        break;
                    case XK_8:
                        key = HgKey_k8;
                        break;
                    case XK_9:
                        key = HgKey_k9;
                        break;

                    case XK_q:
                    case XK_Q:
                        key = HgKey_q;
                        break;
                    case XK_w:
                    case XK_W:
                        key = HgKey_w;
                        break;
                    case XK_e:
                    case XK_E:
                        key = HgKey_e;
                        break;
                    case XK_r:
                    case XK_R:
                        key = HgKey_r;
                        break;
                    case XK_t:
                    case XK_T:
                        key = HgKey_t;
                        break;
                    case XK_y:
                    case XK_Y:
                        key = HgKey_y;
                        break;
                    case XK_u:
                    case XK_U:
                        key = HgKey_u;
                        break;
                    case XK_i:
                    case XK_I:
                        key = HgKey_i;
                        break;
                    case XK_o:
                    case XK_O:
                        key = HgKey_o;
                        break;
                    case XK_p:
                    case XK_P:
                        key = HgKey_p;
                        break;
                    case XK_a:
                    case XK_A:
                        key = HgKey_a;
                        break;
                    case XK_s:
                    case XK_S:
                        key = HgKey_s;
                        break;
                    case XK_d:
                    case XK_D:
                        key = HgKey_d;
                        break;
                    case XK_f:
                    case XK_F:
                        key = HgKey_f;
                        break;
                    case XK_g:
                    case XK_G:
                        key = HgKey_g;
                        break;
                    case XK_h:
                    case XK_H:
                        key = HgKey_h;
                        break;
                    case XK_j:
                    case XK_J:
                        key = HgKey_j;
                        break;
                    case XK_k:
                    case XK_K:
                        key = HgKey_k;
                        break;
                    case XK_l:
                    case XK_L:
                        key = HgKey_l;
                        break;
                    case XK_z:
                    case XK_Z:
                        key = HgKey_z;
                        break;
                    case XK_x:
                    case XK_X:
                        key = HgKey_x;
                        break;
                    case XK_c:
                    case XK_C:
                        key = HgKey_c;
                        break;
                    case XK_v:
                    case XK_V:
                        key = HgKey_v;
                        break;
                    case XK_b:
                    case XK_B:
                        key = HgKey_b;
                        break;
                    case XK_n:
                    case XK_N:
                        key = HgKey_n;
                        break;
                    case XK_m:
                    case XK_M:
                        key = HgKey_m;
                        break;

                    case XK_semicolon:
                        key = HgKey_semicolon;
                        break;
                    case XK_colon:
                        key = HgKey_colon;
                        break;
                    case XK_apostrophe:
                        key = HgKey_apostrophe;
                        break;
                    case XK_quotedbl:
                        key = HgKey_quotation;
                        break;
                    case XK_comma:
                        key = HgKey_comma;
                        break;
                    case XK_period:
                        key = HgKey_period;
                        break;
                    case XK_question:
                        key = HgKey_question;
                        break;
                    case XK_grave:
                        key = HgKey_grave;
                        break;
                    case XK_asciitilde:
                        key = HgKey_tilde;
                        break;
                    case XK_exclam:
                        key = HgKey_exclamation;
                        break;
                    case XK_at:
                        key = HgKey_at;
                        break;
                    case XK_numbersign:
                        key = HgKey_hash;
                        break;
                    case XK_dollar:
                        key = HgKey_dollar;
                        break;
                    case XK_percent:
                        key = HgKey_percent;
                        break;
                    case XK_asciicircum:
                        key = HgKey_carot;
                        break;
                    case XK_ampersand:
                        key = HgKey_ampersand;
                        break;
                    case XK_asterisk:
                        key = HgKey_asterisk;
                        break;
                    case XK_parenleft:
                        key = HgKey_lparen;
                        break;
                    case XK_parenright:
                        key = HgKey_rparen;
                        break;
                    case XK_bracketleft:
                        key = HgKey_lbracket;
                        break;
                    case XK_bracketright:
                        key = HgKey_rbracket;
                        break;
                    case XK_braceleft:
                        key = HgKey_lbrace;
                        break;
                    case XK_braceright:
                        key = HgKey_rbrace;
                        break;
                    case XK_equal:
                        key = HgKey_equal;
                        break;
                    case XK_less:
                        key = HgKey_less;
                        break;
                    case XK_greater:
                        key = HgKey_greater;
                        break;
                    case XK_plus:
                        key = HgKey_plus;
                        break;
                    case XK_minus:
                        key = HgKey_minus;
                        break;
                    case XK_slash:
                        key = HgKey_slash;
                        break;
                    case XK_backslash:
                        key = HgKey_backslash;
                        break;
                    case XK_underscore:
                        key = HgKey_underscore;
                        break;
                    case XK_bar:
                        key = HgKey_bar;
                        break;

                    case XK_Up:
                        key = HgKey_up;
                        break;
                    case XK_Down:
                        key = HgKey_down;
                        break;
                    case XK_Left:
                        key = HgKey_left;
                        break;
                    case XK_Right:
                        key = HgKey_right;
                        break;
                    case XK_Escape:
                        key = HgKey_escape;
                        break;
                    case XK_space:
                        key = HgKey_space;
                        break;
                    case XK_Return:
                        key = HgKey_enter;
                        break;
                    case XK_BackSpace:
                        key = HgKey_backspace;
                        break;
                    case XK_Delete:
                        key = HgKey_kdelete;
                        break;
                    case XK_Insert:
                        key = HgKey_insert;
                        break;
                    case XK_Tab:
                        key = HgKey_tab;
                        break;
                    case XK_Home:
                        key = HgKey_home;
                        break;
                    case XK_End:
                        key = HgKey_end;
                        break;

                    case XK_F1:
                        key = HgKey_f1;
                        break;
                    case XK_F2:
                        key = HgKey_f2;
                        break;
                    case XK_F3:
                        key = HgKey_f3;
                        break;
                    case XK_F4:
                        key = HgKey_f4;
                        break;
                    case XK_F5:
                        key = HgKey_f5;
                        break;
                    case XK_F6:
                        key = HgKey_f6;
                        break;
                    case XK_F7:
                        key = HgKey_f7;
                        break;
                    case XK_F8:
                        key = HgKey_f8;
                        break;
                    case XK_F9:
                        key = HgKey_f9;
                        break;
                    case XK_F10:
                        key = HgKey_f10;
                        break;
                    case XK_F11:
                        key = HgKey_f11;
                        break;
                    case XK_F12:
                        key = HgKey_f12;
                        break;

                    case XK_Shift_L:
                        key = HgKey_lshift;
                        break;
                    case XK_Shift_R:
                        key = HgKey_rshift;
                        break;
                    case XK_Control_L:
                        key = HgKey_lctrl;
                        break;
                    case XK_Control_R:
                        key = HgKey_rctrl;
                        break;
                    case XK_Meta_L:
                        key = HgKey_lmeta;
                        break;
                    case XK_Meta_R:
                        key = HgKey_rmeta;
                        break;
                    case XK_Alt_L:
                        key = HgKey_lalt;
                        break;
                    case XK_Alt_R:
                        key = HgKey_ralt;
                        break;
                    case XK_Super_L:
                        key = HgKey_lsuper;
                        break;
                    case XK_Super_R:
                        key = HgKey_rsuper;
                        break;
                    case XK_Caps_Lock:
                        key = HgKey_capslock;
                        break;
                }
                if (event.type == KeyPress)
                {
                    window->wasKeyPressed[key] = true;
                    window->isKeyDown[key] = true;
                } else if (event.type == KeyRelease)
                {
                    window->wasKeyReleased[key] = true;
                    window->isKeyDown[key] = false;
                }
            } break;
            case ButtonPress:
            case ButtonRelease: {
                HgKey key = HgKey_none;
                switch (event.xbutton.button)
                {
                    case Button1:
                        key = HgKey_mouse1;
                        break;
                    case Button2:
                        key = HgKey_mouse2;
                        break;
                    case Button3:
                        key = HgKey_mouse3;
                        break;
                    case Button4:
                        key = HgKey_mouse4;
                        break;
                    case Button5:
                        key = HgKey_mouse5;
                        break;
                }
                if (event.type == ButtonPress)
                {
                    window->wasKeyPressed[key] = true;
                    window->isKeyDown[key] = true;
                } else if (event.type == ButtonRelease)
                {
                    window->wasKeyReleased[key] = true;
                    window->isKeyDown[key] = false;
                }
            } break;
            case MotionNotify:
                window->mousePosX = (f64)event.xmotion.x / (f64)window->height;
                window->mousePosY = (f64)event.xmotion.y / (f64)window->height;
                break;
            default:
                break;
        }
    }

    if (window->width != oldWidth || window->height != oldHeight)
        hgInternalResizeWindowSwapchain(window);

    window->mouseDeltaX = window->mousePosX - oldMousePosX;
    window->mouseDeltaY = window->mousePosY - oldMousePosY;
}

void ImGui_ImplHurdyGurdy_Init(HgWindow window)
{
    (void)window;
    hgError("x11 has no imgui impl yet\n");
}

void ImGui_ImplHurdyGurdy_Shutdown()
{
    hgError("x11 has no imgui impl yet\n");
}

void ImGui_ImplHurdyGurdy_NewFrame()
{
    hgError("x11 has no imgui impl yet\n");
}

#endif
