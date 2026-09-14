#ifndef WAYLAND_PROTOCOL_H
#define WAYLAND_PROTOCOL_H

#include <wayland-client-core.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for opaque types

struct wl_compositor;
struct wl_shm;
struct wl_shm_pool;
struct wl_buffer;
struct wl_registry;
struct wl_callback;
struct wl_seat;
struct wl_keyboard;
struct wl_pointer;
struct wl_touch;
struct wl_output;
struct wl_region;
struct wl_data_device_manager;
struct wl_data_device;
struct wl_data_source;
struct wl_data_offer;
struct wl_surface;
struct xdg_wm_base;
struct xdg_positioner;
struct xdg_surface;
struct xdg_toplevel;
struct xdg_popup;

// Interface data

extern struct wl_interface wl_registry_interface;
extern struct wl_interface wl_compositor_interface;
extern struct wl_interface wl_shm_interface;
extern struct wl_interface wl_surface_interface;
extern struct wl_interface wl_seat_interface;
extern struct wl_interface wl_keyboard_interface;
extern struct wl_interface wl_pointer_interface;
extern struct wl_interface wl_output_interface;
extern struct wl_interface wl_data_device_manager_interface;
extern struct wl_interface wl_data_device_interface;
extern struct wl_interface wl_data_source_interface;
extern struct wl_interface wl_data_offer_interface;
extern struct wl_interface xdg_wm_base_interface;
extern struct wl_interface xdg_surface_interface;
extern struct wl_interface xdg_toplevel_interface;

// Registry opcodes

enum { WL_REGISTRY_BIND = 0 };

// Display opcodes

enum { WL_DISPLAY_GET_REGISTRY = 1 };

// Seat opcodes

enum {
    WL_SEAT_GET_POINTER = 0,
    WL_SEAT_GET_KEYBOARD = 1,
};

// Compositor opcodes

enum { WL_COMPOSITOR_CREATE_SURFACE = 0 };

// Surface opcodes

enum { WL_SURFACE_COMMIT = 6 };

// Data device manager opcodes

enum {
    WL_DATA_DEVICE_MANAGER_CREATE_DATA_SOURCE = 0,
    WL_DATA_DEVICE_MANAGER_GET_DATA_DEVICE = 1,
};

// Data device opcodes

enum { WL_DATA_DEVICE_SET_SELECTION = 1 };

// Data offer opcodes

enum {
    WL_DATA_OFFER_ACCEPT = 0,
    WL_DATA_OFFER_RECEIVE = 1,
    WL_DATA_OFFER_DESTROY = 2,
};

// Data source opcodes

enum {
    WL_DATA_SOURCE_OFFER = 0,
    WL_DATA_SOURCE_DESTROY = 1,
};

// xdg_wm_base opcodes

enum {
    XDG_WM_BASE_DESTROY = 0,
    XDG_WM_BASE_GET_XDG_SURFACE = 2,
    XDG_WM_BASE_PONG = 3,
};

// xdg_surface opcodes

enum {
    XDG_SURFACE_DESTROY = 0,
    XDG_SURFACE_GET_TOPLEVEL = 1,
    XDG_SURFACE_ACK_CONFIGURE = 4,
};

// xdg_toplevel opcodes

enum {
    XDG_TOPLEVEL_DESTROY = 0,
    XDG_TOPLEVEL_SET_TITLE = 2,
    XDG_TOPLEVEL_SET_MAXIMIZED = 9,
    XDG_TOPLEVEL_UNSET_MAXIMIZED = 10,
    XDG_TOPLEVEL_SET_FULLSCREEN = 11,
    XDG_TOPLEVEL_UNSET_FULLSCREEN = 12,
    XDG_TOPLEVEL_SET_MINIMIZED = 13,
};

// Keyboard events

enum { WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 = 1 };
enum {
    WL_KEYBOARD_KEY_STATE_RELEASED = 0,
    WL_KEYBOARD_KEY_STATE_PRESSED = 1,
};

// Pointer events

enum {
    WL_POINTER_BUTTON_STATE_RELEASED = 0,
    WL_POINTER_BUTTON_STATE_PRESSED = 1,
};

enum {
    WL_POINTER_AXIS_VERTICAL_SCROLL = 0,
    WL_POINTER_AXIS_HORIZONTAL_SCROLL = 1,
};

// Seat events

enum {
    WL_SEAT_CAPABILITY_POINTER = 1,
    WL_SEAT_CAPABILITY_KEYBOARD = 2,
};

// Output events

enum {
    WL_OUTPUT_MODE_CURRENT = 1,
};

// Listener struct definitions

struct wl_keyboard_listener {
    void (*keymap)(void* data, struct wl_keyboard* keyboard,
        uint32_t format, int fd, uint32_t size);
    void (*enter)(void* data, struct wl_keyboard* keyboard,
        uint32_t serial, struct wl_surface* surface, struct wl_array* keys);
    void (*leave)(void* data, struct wl_keyboard* keyboard,
        uint32_t serial, struct wl_surface* surface);
    void (*key)(void* data, struct wl_keyboard* keyboard,
        uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    void (*modifiers)(void* data, struct wl_keyboard* keyboard,
        uint32_t serial, uint32_t modsDepressed, uint32_t modsLatched,
        uint32_t modsLocked, uint32_t group);
    void (*repeat_info)(void* data, struct wl_keyboard* keyboard,
        int32_t rate, int32_t delay);
};

struct wl_pointer_listener {
    void (*enter)(void* data, struct wl_pointer* pointer,
        uint32_t serial, struct wl_surface* surface,
        wl_fixed_t sx, wl_fixed_t sy);
    void (*leave)(void* data, struct wl_pointer* pointer,
        uint32_t serial, struct wl_surface* surface);
    void (*motion)(void* data, struct wl_pointer* pointer,
        uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
    void (*button)(void* data, struct wl_pointer* pointer,
        uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    void (*axis)(void* data, struct wl_pointer* pointer,
        uint32_t time, uint32_t axis, wl_fixed_t value);
    void (*frame)(void* data, struct wl_pointer* pointer);
    void (*axis_source)(void* data, struct wl_pointer* pointer,
        uint32_t axisSource);
    void (*axis_stop)(void* data, struct wl_pointer* pointer,
        uint32_t time, uint32_t axis);
    void (*axis_discrete)(void* data, struct wl_pointer* pointer,
        uint32_t axis, int32_t discrete);
    void (*axis_value120)(void* data, struct wl_pointer* pointer,
        uint32_t axis, int32_t value120);
    void (*axis_relative_direction)(void* data, struct wl_pointer* pointer,
        uint32_t axis, uint32_t direction);
    void (*warp)(void* data, struct wl_pointer* pointer,
        wl_fixed_t sx, wl_fixed_t sy);
};

struct wl_seat_listener {
    void (*capabilities)(void* data, struct wl_seat* seat, uint32_t capabilities);
    void (*name)(void* data, struct wl_seat* seat, const char* name);
};

struct wl_output_listener {
    void (*geometry)(void* data, struct wl_output* output,
        int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight,
        int32_t subpixel, const char* make, const char* model, int32_t transform);
    void (*mode)(void* data, struct wl_output* output,
        uint32_t flags, int32_t width, int32_t height, int32_t refresh);
    void (*done)(void* data, struct wl_output* output);
    void (*scale)(void* data, struct wl_output* output, int32_t factor);
    void (*name)(void* data, struct wl_output* output, const char* name);
    void (*description)(void* data, struct wl_output* output, const char* description);
};

struct wl_registry_listener {
    void (*global)(void* data, struct wl_registry* registry,
        uint32_t name, const char* interface, uint32_t version);
    void (*global_remove)(void* data, struct wl_registry* registry, uint32_t name);
};

struct wl_data_offer_listener {
    void (*offer)(void* data, struct wl_data_offer* offer, const char* mimeType);
    void (*source_actions)(void* data, struct wl_data_offer* offer, uint32_t sourceActions);
    void (*action)(void* data, struct wl_data_offer* offer, uint32_t action);
};

struct wl_data_device_listener {
    void (*data_offer)(void* data, struct wl_data_device* device, struct wl_data_offer* offer);
    void (*enter)(void* data, struct wl_data_device* device, uint32_t serial,
        struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y,
        struct wl_data_offer* offer);
    void (*leave)(void* data, struct wl_data_device* device);
    void (*motion)(void* data, struct wl_data_device* device,
        uint32_t time, wl_fixed_t x, wl_fixed_t y);
    void (*drop)(void* data, struct wl_data_device* device);
    void (*selection)(void* data, struct wl_data_device* device, struct wl_data_offer* offer);
};

struct wl_data_source_listener {
    void (*target)(void* data, struct wl_data_source* source, const char* mimeType);
    void (*send)(void* data, struct wl_data_source* source, const char* mimeType, int32_t fd);
    void (*cancelled)(void* data, struct wl_data_source* source);
    void (*dnd_drop_performed)(void* data, struct wl_data_source* source);
    void (*dnd_finished)(void* data, struct wl_data_source* source);
    void (*action)(void* data, struct wl_data_source* source, uint32_t action);
};

struct xdg_wm_base_listener {
    void (*ping)(void* data, struct xdg_wm_base* xdgWmBase, uint32_t serial);
};

struct xdg_surface_listener {
    void (*configure)(void* data, struct xdg_surface* xdgSurface, uint32_t serial);
};

struct xdg_toplevel_listener {
    void (*configure)(void* data, struct xdg_toplevel* toplevel,
        int32_t width, int32_t height, struct wl_array* states);
    void (*close)(void* data, struct xdg_toplevel* toplevel);
    void (*configure_bounds)(void* data, struct xdg_toplevel* toplevel,
        int32_t width, int32_t height);
    void (*wm_capabilities)(void* data, struct xdg_toplevel* toplevel,
        struct wl_array* capabilities);
};

#ifdef __cplusplus
}
#endif

#endif
