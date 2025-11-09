#include "hg_internal.h"

#include <dlfcn.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <vulkan/vulkan_xlib.h>

typedef struct HgWindowPlatformInternals {
    Window window;
    Atom delete_atom;
} HgWindowPlatformInternals;

static void hg_load_x11(void);

void hg_init_platform_internals(
    HurdyGurdy* hg
) {
    HG_ASSERT(hg != NULL);

    hg_load_x11();

    hg->platform_internals = (void*)XOpenDisplay(NULL);
    if (hg->platform_internals == NULL)
        HG_ERROR("Could not open X display");
}

void hg_shutdown_platform_internals(
    HurdyGurdy* hg
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(hg->platform_internals != NULL);

    XCloseDisplay((Display*)hg->platform_internals);
}

u32 hg_platform_get_vulkan_instance_extensions(
    const char** extension_buffer,
    u32 extension_buffer_capacity
) {
    HG_ASSERT(extension_buffer != NULL);
    HG_ASSERT(extension_buffer_capacity > 0);

    const char* required_extensions[] = {
        "VK_KHR_surface",
        "VK_KHR_xlib_surface"
    };
    if (HG_ARRAY_SIZE(required_extensions) > extension_buffer_capacity)
        HG_ERROR("Vulkan extension buffer too small");

    memcpy(extension_buffer, required_extensions, HG_ARRAY_SIZE(required_extensions) * sizeof(char*));
    return HG_ARRAY_SIZE(required_extensions);
}

static Window hg_create_x11_window(
    Display* display,
    u32 width,
    u32 height,
    const char* title
) {
    XSetWindowAttributes window_attributes = {
        .event_mask
            = KeyPressMask
            | KeyReleaseMask
            | ButtonPressMask
            | ButtonReleaseMask
            | PointerMotionMask
            | StructureNotifyMask
    };
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
        HG_ERROR("X11 could not create window");

    int name_result = XStoreName(display, window, title);
    if (name_result == 0)
        HG_ERROR("X11 could not set window title");

    int map_result = XMapWindow(display, window);
    if (map_result == 0)
        HG_ERROR("X11 could not map window");

    return window;
}

static Atom hg_set_delete_behavior(
    Display* display,
    Window window
) {
    Atom delete_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
    if (delete_atom == None)
        HG_ERROR("X11 could not get WM_DELETE_WINDOW atom");

    int set_protocols_result = XSetWMProtocols(
        display,
        window,
        &delete_atom,
        1
    );
    if (set_protocols_result == 0)
        HG_ERROR("X11 could not set WM_DELETE_WINDOW protocol");

    return delete_atom;
}

static void hg_set_fullscreen(
    Display* display,
    Window window
) {
    Atom state_atom = XInternAtom(display, "_NET_WM_STATE", False);
    if (state_atom == None)
        HG_ERROR("X11 failed to get state atom");

    Atom fullscreen_atom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    if (fullscreen_atom == None)
        HG_ERROR("X11 failed to get fullscreen atom");

    int fullscreen_result = XSendEvent(
        display,
        RootWindow(display, DefaultScreen(display)),
        False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &(XEvent){.xclient = {
            .type = ClientMessage,
            .window = window,
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
        HG_ERROR("X11 could not send fullscreen message");
}

static VkSurfaceKHR hg_create_surface(
    const HurdyGurdy* hg,
    Display* display,
    Window window
) {
    PFN_vkCreateXlibSurfaceKHR pfn_vkCreateXlibSurfaceKHR
        = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(
            hg->instance, "vkCreateXlibSurfaceKHR"
        );
    if (pfn_vkCreateXlibSurfaceKHR == NULL)
        HG_ERROR("Could not load vkCreateXlibSurfaceKHR");

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult surface_result = pfn_vkCreateXlibSurfaceKHR(
        hg->instance,
        &(VkXlibSurfaceCreateInfoKHR){
            .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            .dpy = display,
            .window = window,
        },
        NULL,
        &surface
    );
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

void hg_window_create_platform_internals(
    const HurdyGurdy* hg,
    const HgWindowConfig* config,
    HgWindow* window
) {
    HG_ASSERT(hg != NULL);
    Display* display = (Display*)hg->platform_internals;

    HgWindowPlatformInternals* x11_window = hg_heap_alloc(
        sizeof(HgWindowPlatformInternals)
    );

    x11_window->window = hg_create_x11_window(
        display,
        config->windowed
            ? config->width
            : (u32)DisplayWidth(display, DefaultScreen(display)),
        config->windowed
            ? config->height
            : (u32)DisplayHeight(display, DefaultScreen(display)),
        config->title
    );

    x11_window->delete_atom = hg_set_delete_behavior(
        display,
        x11_window->window
    );

    if (!config->windowed)
        hg_set_fullscreen(display, x11_window->window);

    int flush_result = XFlush(display);
    if (flush_result == 0)
        HG_ERROR("X11 could not flush window");

    window->platform_internals = (void*)x11_window;
    window->surface = hg_create_surface(hg, display, x11_window->window);
}

void hg_window_destroy_platform_internals(
    const HurdyGurdy* hg,
    HgWindow* window
) {
    HG_ASSERT(hg != NULL);
    Display* display = (Display*)hg->platform_internals;
    HgWindowPlatformInternals* x11_window
        = (HgWindowPlatformInternals*)window->platform_internals;

    XDestroyWindow(display, x11_window->window);
    XFlush(display);

    hg_heap_free(x11_window, sizeof(HgWindowPlatformInternals));
}

void hg_window_process_events(
    const HurdyGurdy* hg,
    HgWindow** windows,
    u32 window_count
) {
    HG_ASSERT(hg != NULL);
    HG_ASSERT(windows != NULL);
    HG_ASSERT(window_count > 0);
    Display* display = (Display*)hg->platform_internals;

    if (window_count > 1)
        HG_ERROR("Multiple windows unsupported"); // : TODO
    HgWindow* window = windows[0];
    HgWindowPlatformInternals* x11_window
        = (HgWindowPlatformInternals*)window->platform_internals;

    memset(window->keys_pressed, 0, sizeof(window->keys_pressed));
    memset(window->keys_released, 0, sizeof(window->keys_released));
    window->was_resized = false;

    u32 new_window_width = window->swapchain_width;
    u32 new_window_height = window->swapchain_height;
    f32 new_mouse_pos_x = window->mouse_pos_x;
    f32 new_mouse_pos_y = window->mouse_pos_y;

    while (XPending(display)) {
        XEvent event;
        int event_result = XNextEvent(display, &event);
        if (event_result != 0)
            HG_ERROR("Could not get next event");

        switch (event.type) {
            case ConfigureNotify: {
                new_window_width = (u32)event.xconfigure.width;
                new_window_height = (u32)event.xconfigure.height;
            } break;
            case ClientMessage: {
                if ((Atom)event.xclient.data.l[0] == x11_window->delete_atom)
                    window->was_closed = true;
            } break;
            case KeyPress: {
                switch (XLookupKeysym(&event.xkey, 0)) {
                    case XK_Escape: {
                        window->keys_pressed[HG_KEY_ESCAPE] = true;
                        window->keys_down[HG_KEY_ESCAPE] = true;
                    } break;
                    case XK_space: {
                        window->keys_pressed[HG_KEY_SPACE] = true;
                        window->keys_down[HG_KEY_SPACE] = true;
                    } break;
                    case XK_Return: {
                        window->keys_pressed[HG_KEY_ENTER] = true;
                        window->keys_down[HG_KEY_ENTER] = true;
                    } break;
                    case XK_Tab: {
                        window->keys_pressed[HG_KEY_TAB] = true;
                        window->keys_down[HG_KEY_TAB] = true;
                    } break;
                    case XK_Delete: {
                        window->keys_pressed[HG_KEY_DELETE] = true;
                        window->keys_down[HG_KEY_DELETE] = true;
                    } break;
                    case XK_BackSpace: {
                        window->keys_pressed[HG_KEY_BACKSPACE] = true;
                        window->keys_down[HG_KEY_BACKSPACE] = true;
                    } break;
                    case XK_Up: {
                        window->keys_pressed[HG_KEY_UP] = true;
                        window->keys_down[HG_KEY_UP] = true;
                    } break;
                    case XK_Down: {
                        window->keys_pressed[HG_KEY_DOWN] = true;
                        window->keys_down[HG_KEY_DOWN] = true;
                    } break;
                    case XK_Left: {
                        window->keys_pressed[HG_KEY_LEFT] = true;
                        window->keys_down[HG_KEY_LEFT] = true;
                    } break;
                    case XK_Right: {
                        window->keys_pressed[HG_KEY_RIGHT] = true;
                        window->keys_down[HG_KEY_RIGHT] = true;
                    } break;
                    case XK_a: {
                        window->keys_pressed[HG_KEY_A] = true;
                        window->keys_down[HG_KEY_A] = true;
                    } break;
                    case XK_b: {
                        window->keys_pressed[HG_KEY_B] = true;
                        window->keys_down[HG_KEY_B] = true;
                    } break;
                    case XK_c: {
                        window->keys_pressed[HG_KEY_C] = true;
                        window->keys_down[HG_KEY_C] = true;
                    } break;
                    case XK_d: {
                        window->keys_pressed[HG_KEY_D] = true;
                        window->keys_down[HG_KEY_D] = true;
                    } break;
                    case XK_e: {
                        window->keys_pressed[HG_KEY_E] = true;
                        window->keys_down[HG_KEY_E] = true;
                    } break;
                    case XK_f: {
                        window->keys_pressed[HG_KEY_F] = true;
                        window->keys_down[HG_KEY_F] = true;
                    } break;
                    case XK_g: {
                        window->keys_pressed[HG_KEY_G] = true;
                        window->keys_down[HG_KEY_G] = true;
                    } break;
                    case XK_h: {
                        window->keys_pressed[HG_KEY_H] = true;
                        window->keys_down[HG_KEY_H] = true;
                    } break;
                    case XK_i: {
                        window->keys_pressed[HG_KEY_I] = true;
                        window->keys_down[HG_KEY_I] = true;
                    } break;
                    case XK_j: {
                        window->keys_pressed[HG_KEY_J] = true;
                        window->keys_down[HG_KEY_J] = true;
                    } break;
                    case XK_k: {
                        window->keys_pressed[HG_KEY_K] = true;
                        window->keys_down[HG_KEY_K] = true;
                    } break;
                    case XK_l: {
                        window->keys_pressed[HG_KEY_L] = true;
                        window->keys_down[HG_KEY_L] = true;
                    } break;
                    case XK_m: {
                        window->keys_pressed[HG_KEY_M] = true;
                        window->keys_down[HG_KEY_M] = true;
                    } break;
                    case XK_n: {
                        window->keys_pressed[HG_KEY_N] = true;
                        window->keys_down[HG_KEY_N] = true;
                    } break;
                    case XK_o: {
                        window->keys_pressed[HG_KEY_O] = true;
                        window->keys_down[HG_KEY_O] = true;
                    } break;
                    case XK_p: {
                        window->keys_pressed[HG_KEY_P] = true;
                        window->keys_down[HG_KEY_P] = true;
                    } break;
                    case XK_q: {
                        window->keys_pressed[HG_KEY_Q] = true;
                        window->keys_down[HG_KEY_Q] = true;
                    } break;
                    case XK_r: {
                        window->keys_pressed[HG_KEY_R] = true;
                        window->keys_down[HG_KEY_R] = true;
                    } break;
                    case XK_s: {
                        window->keys_pressed[HG_KEY_S] = true;
                        window->keys_down[HG_KEY_S] = true;
                    } break;
                    case XK_t: {
                        window->keys_pressed[HG_KEY_T] = true;
                        window->keys_down[HG_KEY_T] = true;
                    } break;
                    case XK_u: {
                        window->keys_pressed[HG_KEY_U] = true;
                        window->keys_down[HG_KEY_U] = true;
                    } break;
                    case XK_v: {
                        window->keys_pressed[HG_KEY_V] = true;
                        window->keys_down[HG_KEY_V] = true;
                    } break;
                    case XK_w: {
                        window->keys_pressed[HG_KEY_W] = true;
                        window->keys_down[HG_KEY_W] = true;
                    } break;
                    case XK_x: {
                        window->keys_pressed[HG_KEY_X] = true;
                        window->keys_down[HG_KEY_X] = true;
                    } break;
                    case XK_y: {
                        window->keys_pressed[HG_KEY_Y] = true;
                        window->keys_down[HG_KEY_Y] = true;
                    } break;
                    case XK_z: {
                        window->keys_pressed[HG_KEY_Z] = true;
                        window->keys_down[HG_KEY_Z] = true;
                    } break;
                    case XK_Shift_L: {
                        window->keys_pressed[HG_KEY_LSHIFT] = true;
                        window->keys_down[HG_KEY_LSHIFT] = true;
                    } break;
                    case XK_Shift_R: {
                        window->keys_pressed[HG_KEY_RSHIFT] = true;
                        window->keys_down[HG_KEY_RSHIFT] = true;
                    } break;
                    case XK_Control_L: {
                        window->keys_pressed[HG_KEY_LCTRL] = true;
                        window->keys_down[HG_KEY_LCTRL] = true;
                    } break;
                    case XK_Control_R: {
                        window->keys_pressed[HG_KEY_RCTRL] = true;
                        window->keys_down[HG_KEY_RCTRL] = true;
                    } break;
                    case XK_Alt_L: {
                        window->keys_pressed[HG_KEY_LALT] = true;
                        window->keys_down[HG_KEY_LALT] = true;
                    } break;
                    case XK_Alt_R: {
                        window->keys_pressed[HG_KEY_RALT] = true;
                        window->keys_down[HG_KEY_RALT] = true;
                    } break;
                }
            } break;
            case KeyRelease: {
                switch (XLookupKeysym(&event.xkey, 0)) {
                    case XK_Escape: {
                        window->keys_released[HG_KEY_ESCAPE] = true;
                        window->keys_down[HG_KEY_ESCAPE] = false;
                    } break;
                    case XK_space: {
                        window->keys_released[HG_KEY_SPACE] = true;
                        window->keys_down[HG_KEY_SPACE] = false;
                    } break;
                    case XK_Return: {
                        window->keys_released[HG_KEY_ENTER] = true;
                        window->keys_down[HG_KEY_ENTER] = false;
                    } break;
                    case XK_Tab: {
                        window->keys_released[HG_KEY_TAB] = true;
                        window->keys_down[HG_KEY_TAB] = false;
                    } break;
                    case XK_Delete: {
                        window->keys_released[HG_KEY_DELETE] = true;
                        window->keys_down[HG_KEY_DELETE] = false;
                    } break;
                    case XK_BackSpace: {
                        window->keys_released[HG_KEY_BACKSPACE] = true;
                        window->keys_down[HG_KEY_BACKSPACE] = false;
                    } break;
                    case XK_Up: {
                        window->keys_released[HG_KEY_UP] = true;
                        window->keys_down[HG_KEY_UP] = false;
                    } break;
                    case XK_Down: {
                        window->keys_released[HG_KEY_DOWN] = true;
                        window->keys_down[HG_KEY_DOWN] = false;
                    } break;
                    case XK_Left: {
                        window->keys_released[HG_KEY_LEFT] = true;
                        window->keys_down[HG_KEY_LEFT] = false;
                    } break;
                    case XK_Right: {
                        window->keys_released[HG_KEY_RIGHT] = true;
                        window->keys_down[HG_KEY_RIGHT] = false;
                    } break;
                    case XK_a: {
                        window->keys_released[HG_KEY_A] = true;
                        window->keys_down[HG_KEY_A] = false;
                    } break;
                    case XK_b: {
                        window->keys_released[HG_KEY_B] = true;
                        window->keys_down[HG_KEY_B] = false;
                    } break;
                    case XK_c: {
                        window->keys_released[HG_KEY_C] = true;
                        window->keys_down[HG_KEY_C] = false;
                    } break;
                    case XK_d: {
                        window->keys_released[HG_KEY_D] = true;
                        window->keys_down[HG_KEY_D] = false;
                    } break;
                    case XK_e: {
                        window->keys_released[HG_KEY_E] = true;
                        window->keys_down[HG_KEY_E] = false;
                    } break;
                    case XK_f: {
                        window->keys_released[HG_KEY_F] = true;
                        window->keys_down[HG_KEY_F] = false;
                    } break;
                    case XK_g: {
                        window->keys_released[HG_KEY_G] = true;
                        window->keys_down[HG_KEY_G] = false;
                    } break;
                    case XK_h: {
                        window->keys_released[HG_KEY_H] = true;
                        window->keys_down[HG_KEY_H] = false;
                    } break;
                    case XK_i: {
                        window->keys_released[HG_KEY_I] = true;
                        window->keys_down[HG_KEY_I] = false;
                    } break;
                    case XK_j: {
                        window->keys_released[HG_KEY_J] = true;
                        window->keys_down[HG_KEY_J] = false;
                    } break;
                    case XK_k: {
                        window->keys_released[HG_KEY_K] = true;
                        window->keys_down[HG_KEY_K] = false;
                    } break;
                    case XK_l: {
                        window->keys_released[HG_KEY_L] = true;
                        window->keys_down[HG_KEY_L] = false;
                    } break;
                    case XK_m: {
                        window->keys_released[HG_KEY_M] = true;
                        window->keys_down[HG_KEY_M] = false;
                    } break;
                    case XK_n: {
                        window->keys_released[HG_KEY_N] = true;
                        window->keys_down[HG_KEY_N] = false;
                    } break;
                    case XK_o: {
                        window->keys_released[HG_KEY_O] = true;
                        window->keys_down[HG_KEY_O] = false;
                    } break;
                    case XK_p: {
                        window->keys_released[HG_KEY_P] = true;
                        window->keys_down[HG_KEY_P] = false;
                    } break;
                    case XK_q: {
                        window->keys_released[HG_KEY_Q] = true;
                        window->keys_down[HG_KEY_Q] = false;
                    } break;
                    case XK_r: {
                        window->keys_released[HG_KEY_R] = true;
                        window->keys_down[HG_KEY_R] = false;
                    } break;
                    case XK_s: {
                        window->keys_released[HG_KEY_S] = true;
                        window->keys_down[HG_KEY_S] = false;
                    } break;
                    case XK_t: {
                        window->keys_released[HG_KEY_T] = true;
                        window->keys_down[HG_KEY_T] = false;
                    } break;
                    case XK_u: {
                        window->keys_released[HG_KEY_U] = true;
                        window->keys_down[HG_KEY_U] = false;
                    } break;
                    case XK_v: {
                        window->keys_released[HG_KEY_V] = true;
                        window->keys_down[HG_KEY_V] = false;
                    } break;
                    case XK_w: {
                        window->keys_released[HG_KEY_W] = true;
                        window->keys_down[HG_KEY_W] = false;
                    } break;
                    case XK_x: {
                        window->keys_released[HG_KEY_X] = true;
                        window->keys_down[HG_KEY_X] = false;
                    } break;
                    case XK_y: {
                        window->keys_released[HG_KEY_Y] = true;
                        window->keys_down[HG_KEY_Y] = false;
                    } break;
                    case XK_z: {
                        window->keys_released[HG_KEY_Z] = true;
                        window->keys_down[HG_KEY_Z] = false;
                    } break;
                    case XK_Shift_L: {
                        window->keys_released[HG_KEY_LSHIFT] = true;
                        window->keys_down[HG_KEY_LSHIFT] = false;
                    } break;
                    case XK_Shift_R: {
                        window->keys_released[HG_KEY_RSHIFT] = true;
                        window->keys_down[HG_KEY_RSHIFT] = false;
                    } break;
                    case XK_Control_L: {
                        window->keys_released[HG_KEY_LCTRL] = true;
                        window->keys_down[HG_KEY_LCTRL] = false;
                    } break;
                    case XK_Control_R: {
                        window->keys_released[HG_KEY_RCTRL] = true;
                        window->keys_down[HG_KEY_RCTRL] = false;
                    } break;
                    case XK_Alt_L: {
                        window->keys_released[HG_KEY_LALT] = true;
                        window->keys_down[HG_KEY_LALT] = false;
                    } break;
                    case XK_Alt_R: {
                        window->keys_released[HG_KEY_RALT] = true;
                        window->keys_down[HG_KEY_RALT] = false;
                    } break;
                }
            } break;
            case ButtonPress: {
                switch (event.xbutton.button) {
                    case Button1: {
                        window->keys_pressed[HG_KEY_LMOUSE] = true;
                        window->keys_down[HG_KEY_LMOUSE] = true;
                    } break;
                    case Button2: {
                        window->keys_pressed[HG_KEY_RMOUSE] = true;
                        window->keys_down[HG_KEY_RMOUSE] = true;
                    } break;
                    case Button3: {
                        window->keys_pressed[HG_KEY_MMOUSE] = true;
                        window->keys_down[HG_KEY_MMOUSE] = true;
                    } break;
                }
            } break;
            case ButtonRelease: {
                switch (event.xbutton.button) {
                    case Button1: {
                        window->keys_released[HG_KEY_LMOUSE] = true;
                        window->keys_down[HG_KEY_LMOUSE] = false;
                    } break;
                    case Button2: {
                        window->keys_released[HG_KEY_RMOUSE] = true;
                        window->keys_down[HG_KEY_RMOUSE] = false;
                    } break;
                    case Button3: {
                        window->keys_released[HG_KEY_MMOUSE] = true;
                        window->keys_down[HG_KEY_MMOUSE] = false;
                    } break;
                }
            } break;
            case MotionNotify: {
                new_mouse_pos_x = (f32)event.xmotion.x;
                new_mouse_pos_y = (f32)event.xmotion.y;
            } break;
            default: break;
        }
    }

    if (new_window_width != window->swapchain_width ||
        new_window_height != window->swapchain_height) {
        hg_window_update_swapchain(hg, window);
        window->was_resized = true;
    }

    window->mouse_delta_x = new_mouse_pos_x - window->mouse_pos_x;
    window->mouse_delta_y = new_mouse_pos_y - window->mouse_pos_y;
    window->mouse_pos_x = new_mouse_pos_x;
    window->mouse_pos_y = new_mouse_pos_y;
}

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

static void* s_libx11 = NULL;

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

