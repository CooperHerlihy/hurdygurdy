#include "hg_platform.h"
#include "hg_input.h"

#include <dlfcn.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <vulkan/vulkan_xlib.h>

static void hg_load_x11(void);

static Display* s_display = NULL;
static Window s_window;
static Atom s_delete_window_atom;

static bool s_window_closed = false;
static bool s_window_resized = false;

static f32 s_mouse_pos_x;
static f32 s_mouse_pos_y;
static f32 s_mouse_delta_x;
static f32 s_mouse_delta_y;

static bool s_keys_down[HG_KEY_LAST];
static bool s_keys_pressed[HG_KEY_LAST];
static bool s_keys_released[HG_KEY_LAST];

void hg_platform_init(void) {
    hg_load_x11();

    s_display = XOpenDisplay(NULL);
    if (!s_display)
        HG_ERROR("Could not open X display");
}

void hg_platform_shutdown(void) {
    HG_ASSERT(s_display != NULL);

    XCloseDisplay(s_display);
    s_display = NULL;
}

void hg_platform_open_window(const char* title, u32 width, u32 height, bool windowed) {
    HG_ASSERT(s_display != NULL);

    XSetWindowAttributes window_attributes = {
        .event_mask
            = KeyPressMask
            | KeyReleaseMask
            | ButtonPressMask
            | ButtonReleaseMask
            | PointerMotionMask
            | StructureNotifyMask
    };
    s_window = XCreateWindow(
        s_display, RootWindow(s_display, DefaultScreen(s_display)),
        0, 0,
        windowed ? width : (u32)DisplayWidth(s_display, DefaultScreen(s_display)),
        windowed ? height : (u32)DisplayHeight(s_display, DefaultScreen(s_display)),
        1,
        CopyFromParent,
        InputOutput,
        CopyFromParent,
        CWEventMask,
        &window_attributes
    );
    if (s_window == ~0U)
        HG_ERROR("Could not create window");

    int name_result = XStoreName(s_display, s_window, title);
    if (name_result == 0)
        HG_ERROR("Could not set window title");

    s_delete_window_atom = XInternAtom(s_display, "WM_DELETE_WINDOW", False);
    if (s_delete_window_atom == None)
        HG_ERROR("Could not get WM_DELETE_WINDOW atom");

    int set_protocols_result = XSetWMProtocols(s_display, s_window, &s_delete_window_atom, 1);
    if (set_protocols_result == 0)
        HG_ERROR("Could not set WM_DELETE_WINDOW protocol");

    int map_result = XMapWindow(s_display, s_window);
    if (map_result == 0)
        HG_ERROR("Could not map window");

    if (!windowed) {
        Atom state_atom = XInternAtom(s_display, "_NET_WM_STATE", False);
        if (state_atom == None)
            HG_ERROR("Failed to get state atom");

        Atom fullscreen_atom = XInternAtom(s_display, "_NET_WM_STATE_FULLSCREEN", False);
        if (fullscreen_atom == None)
            HG_ERROR("Failed to get fullscreen atom");

        int fullscreen_result = XSendEvent(
            s_display,
            RootWindow(s_display, DefaultScreen(s_display)),
            False,
            SubstructureRedirectMask | SubstructureNotifyMask,
            &(XEvent){.xclient = {
                .type = ClientMessage,
                .window = s_window,
                .message_type = state_atom,
                .format = 32,
                .data.l = {
                    [0] = 1,
                    [1] = (long)fullscreen_atom,
                    [2] = 0,
                    [3] = 0,
                    [4] = 0,
                },
            }}
        );
        if (fullscreen_result == 0)
            HG_ERROR("Could not send fullscreen message");
    }

    int flush_result = XFlush(s_display);
    if (flush_result == 0)
        HG_ERROR("Could not flush window");
}

void hg_platform_close_window(void) {
    HG_ASSERT(s_display != NULL);
    HG_ASSERT(s_window != None);

    XDestroyWindow(s_display, s_window);
    XFlush(s_display);
}

void hg_platform_get_vulkan_instance_extensions(u32 buffer_count, const char** extensions, u32* extension_count) {
    HG_ASSERT(buffer_count > 0);
    HG_ASSERT(extensions != NULL);
    HG_ASSERT(extension_count != NULL);
    *extension_count = 0;

    char* required_extensions[] = {
        "VK_KHR_surface",
        "VK_KHR_xlib_surface"
    };
    if (HG_ARRAY_SIZE(required_extensions) > buffer_count)
        HG_ERROR("Vulkan extension buffer too small");

    memcpy(extensions, required_extensions, HG_ARRAY_SIZE(required_extensions) * sizeof(char*));
    *extension_count += HG_ARRAY_SIZE(required_extensions);
}

VkSurfaceKHR hg_platform_create_vulkan_surface(VkInstance instance) {
    HG_ASSERT(s_display != NULL);
    HG_ASSERT(s_window != None);

    PFN_vkCreateXlibSurfaceKHR pfn_vkCreateXlibSurfaceKHR
        = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");
    if (pfn_vkCreateXlibSurfaceKHR == NULL)
        HG_ERROR("Could not load vkCreateXlibSurfaceKHR");

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkResult surface_result = pfn_vkCreateXlibSurfaceKHR(instance, &(VkXlibSurfaceCreateInfoKHR){
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = s_display,
        .window = s_window,
    }, NULL, &surface);
    switch (surface_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_HOST_MEMORY: HG_ERROR("Vulkan ran out of host memory");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: HG_ERROR("Vulkan ran out of device memory");
        case VK_ERROR_VALIDATION_FAILED: HG_ERROR("Vulkan validation failed");
        case VK_ERROR_UNKNOWN: HG_ERROR("Vulkan unknown error");
        default: HG_ERROR("Unexpected Vulkan error");
    }

    return surface;
}

bool hg_was_window_closed(void) {
    return s_window_closed;
}

bool hg_was_window_resized(void) {
    return s_window_resized;
}

bool hg_is_key_down(HgKey key) {
    return s_keys_down[key];
}

bool hg_was_key_pressed(HgKey key) {
    return s_keys_pressed[key];
}

bool hg_was_key_released(HgKey key) {
    return s_keys_released[key];
}

void hg_get_mouse_pos(f32* x, f32* y) {
    *x = s_mouse_pos_x;
    *y = s_mouse_pos_y;
}

void hg_get_mouse_delta(f32* x, f32* y) {
    *x = s_mouse_delta_x;
    *y = s_mouse_delta_y;
}

void hg_process_events(void) {
    HG_ASSERT(s_display != NULL);

    memset(s_keys_pressed, 0, sizeof(s_keys_pressed));
    memset(s_keys_released, 0, sizeof(s_keys_released));
    s_window_resized = false;
    s_mouse_delta_x = 0.0f;
    s_mouse_delta_y = 0.0f;

    while (XPending(s_display)) {
        XEvent event;
        int event_result = XNextEvent(s_display, &event);
        if (event_result != 0)
            HG_ERROR("Could not get next event");

        switch (event.type) {
            case ConfigureNotify: {
                s_window_resized = true;
            } break;
            case ClientMessage: {
                if ((Atom)event.xclient.data.l[0] == s_delete_window_atom)
                    s_window_closed = true;
            } break;
            case KeyPress: {
                switch (XLookupKeysym(&event.xkey, 0)) {
                    case XK_Escape: {
                        s_keys_pressed[HG_KEY_ESCAPE] = true;
                        s_keys_down[HG_KEY_ESCAPE] = true;
                    } break;
                    case XK_space: {
                        s_keys_pressed[HG_KEY_SPACE] = true;
                        s_keys_down[HG_KEY_SPACE] = true;
                    } break;
                    case XK_Return: {
                        s_keys_pressed[HG_KEY_ENTER] = true;
                        s_keys_down[HG_KEY_ENTER] = true;
                    } break;
                    case XK_Tab: {
                        s_keys_pressed[HG_KEY_TAB] = true;
                        s_keys_down[HG_KEY_TAB] = true;
                    } break;
                    case XK_Delete: {
                        s_keys_pressed[HG_KEY_DELETE] = true;
                        s_keys_down[HG_KEY_DELETE] = true;
                    } break;
                    case XK_BackSpace: {
                        s_keys_pressed[HG_KEY_BACKSPACE] = true;
                        s_keys_down[HG_KEY_BACKSPACE] = true;
                    } break;
                    case XK_Up: {
                        s_keys_pressed[HG_KEY_UP] = true;
                        s_keys_down[HG_KEY_UP] = true;
                    } break;
                    case XK_Down: {
                        s_keys_pressed[HG_KEY_DOWN] = true;
                        s_keys_down[HG_KEY_DOWN] = true;
                    } break;
                    case XK_Left: {
                        s_keys_pressed[HG_KEY_LEFT] = true;
                        s_keys_down[HG_KEY_LEFT] = true;
                    } break;
                    case XK_Right: {
                        s_keys_pressed[HG_KEY_RIGHT] = true;
                        s_keys_down[HG_KEY_RIGHT] = true;
                    } break;
                    case XK_a: {
                        s_keys_pressed[HG_KEY_A] = true;
                        s_keys_down[HG_KEY_A] = true;
                    } break;
                    case XK_b: {
                        s_keys_pressed[HG_KEY_B] = true;
                        s_keys_down[HG_KEY_B] = true;
                    } break;
                    case XK_c: {
                        s_keys_pressed[HG_KEY_C] = true;
                        s_keys_down[HG_KEY_C] = true;
                    } break;
                    case XK_d: {
                        s_keys_pressed[HG_KEY_D] = true;
                        s_keys_down[HG_KEY_D] = true;
                    } break;
                    case XK_e: {
                        s_keys_pressed[HG_KEY_E] = true;
                        s_keys_down[HG_KEY_E] = true;
                    } break;
                    case XK_f: {
                        s_keys_pressed[HG_KEY_F] = true;
                        s_keys_down[HG_KEY_F] = true;
                    } break;
                    case XK_g: {
                        s_keys_pressed[HG_KEY_G] = true;
                        s_keys_down[HG_KEY_G] = true;
                    } break;
                    case XK_h: {
                        s_keys_pressed[HG_KEY_H] = true;
                        s_keys_down[HG_KEY_H] = true;
                    } break;
                    case XK_i: {
                        s_keys_pressed[HG_KEY_I] = true;
                        s_keys_down[HG_KEY_I] = true;
                    } break;
                    case XK_j: {
                        s_keys_pressed[HG_KEY_J] = true;
                        s_keys_down[HG_KEY_J] = true;
                    } break;
                    case XK_k: {
                        s_keys_pressed[HG_KEY_K] = true;
                        s_keys_down[HG_KEY_K] = true;
                    } break;
                    case XK_l: {
                        s_keys_pressed[HG_KEY_L] = true;
                        s_keys_down[HG_KEY_L] = true;
                    } break;
                    case XK_m: {
                        s_keys_pressed[HG_KEY_M] = true;
                        s_keys_down[HG_KEY_M] = true;
                    } break;
                    case XK_n: {
                        s_keys_pressed[HG_KEY_N] = true;
                        s_keys_down[HG_KEY_N] = true;
                    } break;
                    case XK_o: {
                        s_keys_pressed[HG_KEY_O] = true;
                        s_keys_down[HG_KEY_O] = true;
                    } break;
                    case XK_p: {
                        s_keys_pressed[HG_KEY_P] = true;
                        s_keys_down[HG_KEY_P] = true;
                    } break;
                    case XK_q: {
                        s_keys_pressed[HG_KEY_Q] = true;
                        s_keys_down[HG_KEY_Q] = true;
                    } break;
                    case XK_r: {
                        s_keys_pressed[HG_KEY_R] = true;
                        s_keys_down[HG_KEY_R] = true;
                    } break;
                    case XK_s: {
                        s_keys_pressed[HG_KEY_S] = true;
                        s_keys_down[HG_KEY_S] = true;
                    } break;
                    case XK_t: {
                        s_keys_pressed[HG_KEY_T] = true;
                        s_keys_down[HG_KEY_T] = true;
                    } break;
                    case XK_u: {
                        s_keys_pressed[HG_KEY_U] = true;
                        s_keys_down[HG_KEY_U] = true;
                    } break;
                    case XK_v: {
                        s_keys_pressed[HG_KEY_V] = true;
                        s_keys_down[HG_KEY_V] = true;
                    } break;
                    case XK_w: {
                        s_keys_pressed[HG_KEY_W] = true;
                        s_keys_down[HG_KEY_W] = true;
                    } break;
                    case XK_x: {
                        s_keys_pressed[HG_KEY_X] = true;
                        s_keys_down[HG_KEY_X] = true;
                    } break;
                    case XK_y: {
                        s_keys_pressed[HG_KEY_Y] = true;
                        s_keys_down[HG_KEY_Y] = true;
                    } break;
                    case XK_z: {
                        s_keys_pressed[HG_KEY_Z] = true;
                        s_keys_down[HG_KEY_Z] = true;
                    } break;
                    case XK_Shift_L: {
                        s_keys_pressed[HG_KEY_LSHIFT] = true;
                        s_keys_down[HG_KEY_LSHIFT] = true;
                    } break;
                    case XK_Shift_R: {
                        s_keys_pressed[HG_KEY_RSHIFT] = true;
                        s_keys_down[HG_KEY_RSHIFT] = true;
                    } break;
                    case XK_Control_L: {
                        s_keys_pressed[HG_KEY_LCTRL] = true;
                        s_keys_down[HG_KEY_LCTRL] = true;
                    } break;
                    case XK_Control_R: {
                        s_keys_pressed[HG_KEY_RCTRL] = true;
                        s_keys_down[HG_KEY_RCTRL] = true;
                    } break;
                    case XK_Alt_L: {
                        s_keys_pressed[HG_KEY_LALT] = true;
                        s_keys_down[HG_KEY_LALT] = true;
                    } break;
                    case XK_Alt_R: {
                        s_keys_pressed[HG_KEY_RALT] = true;
                        s_keys_down[HG_KEY_RALT] = true;
                    } break;
                }
            } break;
            case KeyRelease: {
                switch (XLookupKeysym(&event.xkey, 0)) {
                    case XK_Escape: {
                        s_keys_released[HG_KEY_ESCAPE] = true;
                        s_keys_down[HG_KEY_ESCAPE] = false;
                    } break;
                    case XK_space: {
                        s_keys_released[HG_KEY_SPACE] = true;
                        s_keys_down[HG_KEY_SPACE] = false;
                    } break;
                    case XK_Return: {
                        s_keys_released[HG_KEY_ENTER] = true;
                        s_keys_down[HG_KEY_ENTER] = false;
                    } break;
                    case XK_Tab: {
                        s_keys_released[HG_KEY_TAB] = true;
                        s_keys_down[HG_KEY_TAB] = false;
                    } break;
                    case XK_Delete: {
                        s_keys_released[HG_KEY_DELETE] = true;
                        s_keys_down[HG_KEY_DELETE] = false;
                    } break;
                    case XK_BackSpace: {
                        s_keys_released[HG_KEY_BACKSPACE] = true;
                        s_keys_down[HG_KEY_BACKSPACE] = false;
                    } break;
                    case XK_Up: {
                        s_keys_released[HG_KEY_UP] = true;
                        s_keys_down[HG_KEY_UP] = false;
                    } break;
                    case XK_Down: {
                        s_keys_released[HG_KEY_DOWN] = true;
                        s_keys_down[HG_KEY_DOWN] = false;
                    } break;
                    case XK_Left: {
                        s_keys_released[HG_KEY_LEFT] = true;
                        s_keys_down[HG_KEY_LEFT] = false;
                    } break;
                    case XK_Right: {
                        s_keys_released[HG_KEY_RIGHT] = true;
                        s_keys_down[HG_KEY_RIGHT] = false;
                    } break;
                    case XK_a: {
                        s_keys_released[HG_KEY_A] = true;
                        s_keys_down[HG_KEY_A] = false;
                    } break;
                    case XK_b: {
                        s_keys_released[HG_KEY_B] = true;
                        s_keys_down[HG_KEY_B] = false;
                    } break;
                    case XK_c: {
                        s_keys_released[HG_KEY_C] = true;
                        s_keys_down[HG_KEY_C] = false;
                    } break;
                    case XK_d: {
                        s_keys_released[HG_KEY_D] = true;
                        s_keys_down[HG_KEY_D] = false;
                    } break;
                    case XK_e: {
                        s_keys_released[HG_KEY_E] = true;
                        s_keys_down[HG_KEY_E] = false;
                    } break;
                    case XK_f: {
                        s_keys_released[HG_KEY_F] = true;
                        s_keys_down[HG_KEY_F] = false;
                    } break;
                    case XK_g: {
                        s_keys_released[HG_KEY_G] = true;
                        s_keys_down[HG_KEY_G] = false;
                    } break;
                    case XK_h: {
                        s_keys_released[HG_KEY_H] = true;
                        s_keys_down[HG_KEY_H] = false;
                    } break;
                    case XK_i: {
                        s_keys_released[HG_KEY_I] = true;
                        s_keys_down[HG_KEY_I] = false;
                    } break;
                    case XK_j: {
                        s_keys_released[HG_KEY_J] = true;
                        s_keys_down[HG_KEY_J] = false;
                    } break;
                    case XK_k: {
                        s_keys_released[HG_KEY_K] = true;
                        s_keys_down[HG_KEY_K] = false;
                    } break;
                    case XK_l: {
                        s_keys_released[HG_KEY_L] = true;
                        s_keys_down[HG_KEY_L] = false;
                    } break;
                    case XK_m: {
                        s_keys_released[HG_KEY_M] = true;
                        s_keys_down[HG_KEY_M] = false;
                    } break;
                    case XK_n: {
                        s_keys_released[HG_KEY_N] = true;
                        s_keys_down[HG_KEY_N] = false;
                    } break;
                    case XK_o: {
                        s_keys_released[HG_KEY_O] = true;
                        s_keys_down[HG_KEY_O] = false;
                    } break;
                    case XK_p: {
                        s_keys_released[HG_KEY_P] = true;
                        s_keys_down[HG_KEY_P] = false;
                    } break;
                    case XK_q: {
                        s_keys_released[HG_KEY_Q] = true;
                        s_keys_down[HG_KEY_Q] = false;
                    } break;
                    case XK_r: {
                        s_keys_released[HG_KEY_R] = true;
                        s_keys_down[HG_KEY_R] = false;
                    } break;
                    case XK_s: {
                        s_keys_released[HG_KEY_S] = true;
                        s_keys_down[HG_KEY_S] = false;
                    } break;
                    case XK_t: {
                        s_keys_released[HG_KEY_T] = true;
                        s_keys_down[HG_KEY_T] = false;
                    } break;
                    case XK_u: {
                        s_keys_released[HG_KEY_U] = true;
                        s_keys_down[HG_KEY_U] = false;
                    } break;
                    case XK_v: {
                        s_keys_released[HG_KEY_V] = true;
                        s_keys_down[HG_KEY_V] = false;
                    } break;
                    case XK_w: {
                        s_keys_released[HG_KEY_W] = true;
                        s_keys_down[HG_KEY_W] = false;
                    } break;
                    case XK_x: {
                        s_keys_released[HG_KEY_X] = true;
                        s_keys_down[HG_KEY_X] = false;
                    } break;
                    case XK_y: {
                        s_keys_released[HG_KEY_Y] = true;
                        s_keys_down[HG_KEY_Y] = false;
                    } break;
                    case XK_z: {
                        s_keys_released[HG_KEY_Z] = true;
                        s_keys_down[HG_KEY_Z] = false;
                    } break;
                    case XK_Shift_L: {
                        s_keys_released[HG_KEY_LSHIFT] = true;
                        s_keys_down[HG_KEY_LSHIFT] = false;
                    } break;
                    case XK_Shift_R: {
                        s_keys_released[HG_KEY_RSHIFT] = true;
                        s_keys_down[HG_KEY_RSHIFT] = false;
                    } break;
                    case XK_Control_L: {
                        s_keys_released[HG_KEY_LCTRL] = true;
                        s_keys_down[HG_KEY_LCTRL] = false;
                    } break;
                    case XK_Control_R: {
                        s_keys_released[HG_KEY_RCTRL] = true;
                        s_keys_down[HG_KEY_RCTRL] = false;
                    } break;
                    case XK_Alt_L: {
                        s_keys_released[HG_KEY_LALT] = true;
                        s_keys_down[HG_KEY_LALT] = false;
                    } break;
                    case XK_Alt_R: {
                        s_keys_released[HG_KEY_RALT] = true;
                        s_keys_down[HG_KEY_RALT] = false;
                    } break;
                }
            } break;
            case ButtonPress: {
                switch (event.xbutton.button) {
                    case Button1: {
                        s_keys_pressed[HG_KEY_LMOUSE] = true;
                        s_keys_down[HG_KEY_LMOUSE] = true;
                    } break;
                    case Button2: {
                        s_keys_pressed[HG_KEY_RMOUSE] = true;
                        s_keys_down[HG_KEY_RMOUSE] = true;
                    } break;
                    case Button3: {
                        s_keys_pressed[HG_KEY_MMOUSE] = true;
                        s_keys_down[HG_KEY_MMOUSE] = true;
                    } break;
                }
            } break;
            case ButtonRelease: {
                switch (event.xbutton.button) {
                    case Button1: {
                        s_keys_released[HG_KEY_LMOUSE] = true;
                        s_keys_down[HG_KEY_LMOUSE] = false;
                    } break;
                    case Button2: {
                        s_keys_released[HG_KEY_RMOUSE] = true;
                        s_keys_down[HG_KEY_RMOUSE] = false;
                    } break;
                    case Button3: {
                        s_keys_released[HG_KEY_MMOUSE] = true;
                        s_keys_down[HG_KEY_MMOUSE] = false;
                    } break;
                }
            } break;
            case MotionNotify: {
                s_mouse_delta_x += (f32)event.xmotion.x - s_mouse_pos_x;
                s_mouse_delta_y += (f32)event.xmotion.y - s_mouse_pos_y;
                s_mouse_pos_x = (f32)event.xmotion.x;
                s_mouse_pos_y = (f32)event.xmotion.y;
            } break;
            default: break;
        }
    }
}

static void* s_libx11 = NULL;

static Display* (*s_pfn_XOpenDisplay)(
    _Xconst char*
);

Display *XOpenDisplay(
    _Xconst char* display_name
) {
    return s_pfn_XOpenDisplay(display_name);
}

static int (*s_pfn_XCloseDisplay)(
    Display*
);

int XCloseDisplay(
    Display* display
) {
    return s_pfn_XCloseDisplay(display);
}

static Window (*s_pfn_XCreateWindow)(
    Display*,
    Window,
    int,
    int,
    unsigned int,
    unsigned int,
    unsigned int,
    int,
    unsigned int,
    Visual*,
    unsigned long,
    XSetWindowAttributes*
);

Window XCreateWindow(
    Display* display,
    Window parent,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int border_width,
    int depth,
    unsigned int class,
    Visual* visual,
    unsigned long valuemask,
    XSetWindowAttributes* attributes
) {
    return s_pfn_XCreateWindow(
        display,
        parent,
        x,
        y,
        width,
        height,
        border_width,
        depth,
        class,
        visual,
        valuemask,
        attributes
    );
}

static int (*s_pfn_XDestroyWindow)(
    Display*,
    Window
);

int XDestroyWindow(
    Display* display,
    Window w
) {
    return s_pfn_XDestroyWindow(display, w);
}

static int (*s_pfn_XStoreName)(
    Display*,
    Window,
    _Xconst char*
);

int XStoreName(
    Display* display,
    Window w,
    _Xconst char* window_name
) {
    return s_pfn_XStoreName(display, w, window_name);
}

static Atom (*s_pfn_XInternAtom)(
    Display*,
    _Xconst char*,
    Bool
);

Atom XInternAtom(
    Display* display,
    _Xconst char* atom_name,
    Bool only_if_exists
) {
    return s_pfn_XInternAtom(display, atom_name, only_if_exists);
}

static Status (*s_pfn_XSetWMProtocols)(
    Display*,
    Window,
    Atom*,
    int
);

Status XSetWMProtocols(
    Display* display,
    Window w,
    Atom* protocols,
    int count
) {
    return s_pfn_XSetWMProtocols(display, w, protocols, count);
}

static int (*s_pfn_XMapWindow)(
    Display*,
    Window
);

int XMapWindow(
    Display* display,
    Window w
) {
    return s_pfn_XMapWindow(display, w);
}

static Status (*s_pfn_XSendEvent)(
    Display*,
    Window,
    Bool,
    long,
    XEvent*
);

Status XSendEvent(
    Display* display,
    Window w,
    Bool propagate,
    long event_mask,
    XEvent* event_send
) {
    return s_pfn_XSendEvent(display, w, propagate, event_mask, event_send);
}

static int (*s_pfn_XFlush)(
    Display*
);

int XFlush(
    Display* display
) {
    return s_pfn_XFlush(display);
}

static int (*s_pfn_XNextEvent)(
    Display*,
    XEvent*
);

static int (*s_pfn_XPending)(
    Display*
);

int XPending(
    Display* display
) {
    return s_pfn_XPending(display);
}

int XNextEvent(
    Display* display,
    XEvent* event_return
) {
    return s_pfn_XNextEvent(display, event_return);
}

static KeySym (*s_pfn_XLookupKeysym)(
    XKeyEvent*,
    int
);

KeySym XLookupKeysym(
    XKeyEvent* key_event,
    int index
) {
    return s_pfn_XLookupKeysym(key_event, index);
}


#define HG_MAKE_X11_FUNC(name) *(void**)&s_pfn_##name = dlsym(s_libx11, #name); \
    if (s_pfn_##name == NULL) { HG_ERROR("Could not load " #name); }

static void hg_load_x11(void) {
    s_libx11 = dlopen("libX11.so.6", RTLD_LAZY);
    if (s_libx11 == NULL)
        HG_ERRORF("Could not load libX11.so.6: %s", dlerror());

    HG_MAKE_X11_FUNC(XOpenDisplay)
    HG_MAKE_X11_FUNC(XCloseDisplay)
    HG_MAKE_X11_FUNC(XCreateWindow)
    HG_MAKE_X11_FUNC(XDestroyWindow)
    HG_MAKE_X11_FUNC(XStoreName)
    HG_MAKE_X11_FUNC(XInternAtom)
    HG_MAKE_X11_FUNC(XSetWMProtocols)
    HG_MAKE_X11_FUNC(XMapWindow)
    HG_MAKE_X11_FUNC(XSendEvent)
    HG_MAKE_X11_FUNC(XFlush)
    HG_MAKE_X11_FUNC(XPending)
    HG_MAKE_X11_FUNC(XNextEvent)
    HG_MAKE_X11_FUNC(XLookupKeysym)

}

#undef HG_MAKE_X11_FUNC

