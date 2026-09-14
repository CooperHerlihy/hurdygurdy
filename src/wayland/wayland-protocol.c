#include "wayland-protocol.h"

// Forward declarations for interfaces referenced in type arrays

extern struct wl_interface wl_callback_interface;
extern struct wl_interface wl_region_interface;
extern struct wl_interface wl_buffer_interface;
extern struct wl_interface wl_shm_pool_interface;

// Type arrays

static const struct wl_interface* wayland_types[] = {
    NULL,                                    // 0
    NULL,                                    // 1
    NULL,                                    // 2
    NULL,                                    // 3
    NULL,                                    // 4
    NULL,                                    // 5
    NULL,                                    // 6
    NULL,                                    // 7
    &wl_callback_interface,                  // 8
    &wl_registry_interface,                  // 9
    &wl_surface_interface,                   // 10
    &wl_region_interface,                    // 11
    &wl_buffer_interface,                    // 12
    NULL,                                    // 13
    NULL,                                    // 14
    NULL,                                    // 15
    NULL,                                    // 16
    NULL,                                    // 17
    &wl_shm_pool_interface,                  // 18
    NULL,                                    // 19
    NULL,                                    // 20
    &wl_data_source_interface,               // 21
    &wl_surface_interface,                   // 22
    &wl_surface_interface,                   // 23
    NULL,                                    // 24
    &wl_data_source_interface,               // 25
    NULL,                                    // 26
    &wl_data_offer_interface,                // 27
    NULL,                                    // 28
    &wl_surface_interface,                   // 29
    NULL,                                    // 30
    NULL,                                    // 31
    &wl_data_offer_interface,                // 32
    &wl_data_offer_interface,                // 33
    &wl_data_source_interface,               // 34
    &wl_data_device_interface,               // 35
    &wl_seat_interface,                      // 36
    NULL,                                    // 37
    &wl_surface_interface,                   // 38
    &wl_seat_interface,                      // 39
    NULL,                                    // 40
    &wl_seat_interface,                      // 41
    NULL,                                    // 42
    NULL,                                    // 43
    &wl_surface_interface,                   // 44
    NULL,                                    // 45
    NULL,                                    // 46
    NULL,                                    // 47
    NULL,                                    // 48
    NULL,                                    // 49
    &wl_output_interface,                    // 50
    &wl_seat_interface,                      // 51
    NULL,                                    // 52
    &wl_surface_interface,                   // 53
    NULL,                                    // 54
    NULL,                                    // 55
    NULL,                                    // 56
    &wl_output_interface,                    // 57
    &wl_buffer_interface,                    // 58
    NULL,                                    // 59
    NULL,                                    // 60
    &wl_callback_interface,                  // 61
    &wl_region_interface,                    // 62
    &wl_region_interface,                    // 63
    &wl_callback_interface,                  // 64
    &wl_output_interface,                    // 65
    &wl_output_interface,                    // 66
    &wl_pointer_interface,                   // 67
    &wl_keyboard_interface,                  // 68
    NULL,                                    // 69
    &wl_surface_interface,                   // 70
    NULL,                                    // 71
    NULL,                                    // 72
    NULL,                                    // 73
    &wl_surface_interface,                   // 74
    NULL,                                    // 75
    NULL,                                    // 76
    NULL,                                    // 77
    &wl_surface_interface,                   // 78
    NULL,                                    // 79
    &wl_surface_interface,                   // 80
    NULL,                                    // 81
    NULL,                                    // 82
    &wl_surface_interface,                   // 83
};

// Registry

static const struct wl_message wl_registry_requests[] = {
    { "bind", "usun", wayland_types + 0 },
};

static const struct wl_message wl_registry_events[] = {
    { "global", "usu", wayland_types + 0 },
    { "global_remove", "u", wayland_types + 0 },
};

struct wl_interface wl_registry_interface = {
    "wl_registry", 1,
    1, wl_registry_requests,
    2, wl_registry_events,
};

// wl_compositor

static const struct wl_message wl_compositor_requests[] = {
    { "create_surface", "n", wayland_types + 10 },
    { "create_region", "n", wayland_types + 11 },
    { "release", "7", wayland_types + 0 },
};

struct wl_interface wl_compositor_interface = {
    "wl_compositor", 7,
    3, wl_compositor_requests,
    0, NULL,
};

// wl_shm_pool

static const struct wl_message wl_shm_pool_requests[] = {
    { "create_buffer", "niiiiu", wayland_types + 13 },
    { "destroy", "", wayland_types + 0 },
    { "resize", "u", wayland_types + 0 },
};

struct wl_interface wl_shm_pool_interface = {
    "wl_shm_pool", 3,
    3, wl_shm_pool_requests,
    0, NULL,
};

// wl_shm

static const struct wl_message wl_shm_requests[] = {
    { "create_pool", "nhi", wayland_types + 18 },
    { "release", "2", wayland_types + 0 },
};

static const struct wl_message wl_shm_events[] = {
    { "format", "u", wayland_types + 0 },
};

struct wl_interface wl_shm_interface = {
    "wl_shm", 3,
    2, wl_shm_requests,
    1, wl_shm_events,
};

// wl_buffer

static const struct wl_message wl_buffer_requests[] = {
    { "destroy", "", wayland_types + 0 },
};

static const struct wl_message wl_buffer_events[] = {
    { "release", "", wayland_types + 0 },
};

struct wl_interface wl_buffer_interface = {
    "wl_buffer", 1,
    1, wl_buffer_requests,
    1, wl_buffer_events,
};

// wl_data_offer

static const struct wl_message wl_data_offer_requests[] = {
    { "accept", "u?s", wayland_types + 0 },
    { "receive", "sh", wayland_types + 0 },
    { "destroy", "", wayland_types + 0 },
    { "finish", "3", wayland_types + 0 },
    { "set_actions", "3uu", wayland_types + 0 },
};

static const struct wl_message wl_data_offer_events[] = {
    { "offer", "s", wayland_types + 0 },
    { "source_actions", "3u", wayland_types + 0 },
    { "action", "3u", wayland_types + 0 },
};

struct wl_interface wl_data_offer_interface = {
    "wl_data_offer", 4,
    5, wl_data_offer_requests,
    3, wl_data_offer_events,
};

// wl_data_source

static const struct wl_message wl_data_source_requests[] = {
    { "offer", "s", wayland_types + 0 },
    { "destroy", "", wayland_types + 0 },
    { "set_actions", "3u", wayland_types + 0 },
};

static const struct wl_message wl_data_source_events[] = {
    { "target", "?s", wayland_types + 0 },
    { "send", "sh", wayland_types + 0 },
    { "cancelled", "", wayland_types + 0 },
    { "dnd_drop_performed", "3", wayland_types + 0 },
    { "dnd_finished", "3", wayland_types + 0 },
    { "action", "3u", wayland_types + 0 },
};

struct wl_interface wl_data_source_interface = {
    "wl_data_source", 4,
    3, wl_data_source_requests,
    6, wl_data_source_events,
};

// wl_data_device

static const struct wl_message wl_data_device_requests[] = {
    { "start_drag", "?oo?ou", wayland_types + 21 },
    { "set_selection", "?ou", wayland_types + 25 },
    { "release", "2", wayland_types + 0 },
};

static const struct wl_message wl_data_device_events[] = {
    { "data_offer", "n", wayland_types + 27 },
    { "enter", "uoff?o", wayland_types + 28 },
    { "leave", "", wayland_types + 0 },
    { "motion", "uff", wayland_types + 0 },
    { "drop", "", wayland_types + 0 },
    { "selection", "?o", wayland_types + 33 },
};

struct wl_interface wl_data_device_interface = {
    "wl_data_device", 4,
    3, wl_data_device_requests,
    6, wl_data_device_events,
};

// wl_data_device_manager

static const struct wl_message wl_data_device_manager_requests[] = {
    { "create_data_source", "n", wayland_types + 34 },
    { "get_data_device", "no", wayland_types + 35 },
    { "release", "4", wayland_types + 0 },
};

struct wl_interface wl_data_device_manager_interface = {
    "wl_data_device_manager", 4,
    3, wl_data_device_manager_requests,
    0, NULL,
};

// wl_surface

static const struct wl_message wl_surface_requests[] = {
    { "destroy", "", wayland_types + 0 },
    { "attach", "?oii", wayland_types + 58 },
    { "damage", "iiii", wayland_types + 0 },
    { "frame", "n", wayland_types + 61 },
    { "set_opaque_region", "?o", wayland_types + 62 },
    { "set_input_region", "?o", wayland_types + 63 },
    { "commit", "", wayland_types + 0 },
    { "set_buffer_transform", "2i", wayland_types + 0 },
    { "set_buffer_scale", "3i", wayland_types + 0 },
    { "damage_buffer", "4iiii", wayland_types + 0 },
    { "offset", "5ii", wayland_types + 0 },
    { "get_release", "7n", wayland_types + 64 },
};

static const struct wl_message wl_surface_events[] = {
    { "enter", "o", wayland_types + 65 },
    { "leave", "o", wayland_types + 66 },
    { "preferred_buffer_scale", "6i", wayland_types + 0 },
    { "preferred_buffer_transform", "6u", wayland_types + 0 },
};

struct wl_interface wl_surface_interface = {
    "wl_surface", 7,
    12, wl_surface_requests,
    4, wl_surface_events,
};

// wl_seat

static const struct wl_message wl_seat_requests[] = {
    { "get_pointer", "n", wayland_types + 67 },
    { "get_keyboard", "n", wayland_types + 68 },
    { "get_touch", "n", wayland_types + 69 },
    { "release", "5", wayland_types + 0 },
};

static const struct wl_message wl_seat_events[] = {
    { "capabilities", "u", wayland_types + 0 },
    { "name", "2s", wayland_types + 0 },
};

struct wl_interface wl_seat_interface = {
    "wl_seat", 11,
    4, wl_seat_requests,
    2, wl_seat_events,
};

// wl_keyboard

static const struct wl_message wl_keyboard_requests[] = {
    { "release", "3", wayland_types + 0 },
};

static const struct wl_message wl_keyboard_events[] = {
    { "keymap", "uhu", wayland_types + 0 },
    { "enter", "uoa", wayland_types + 80 },
    { "leave", "uo", wayland_types + 83 },
    { "key", "uuuu", wayland_types + 0 },
    { "modifiers", "uuuuu", wayland_types + 0 },
    { "repeat_info", "4ii", wayland_types + 0 },
};

struct wl_interface wl_keyboard_interface = {
    "wl_keyboard", 11,
    1, wl_keyboard_requests,
    6, wl_keyboard_events,
};

// wl_pointer

static const struct wl_message wl_pointer_requests[] = {
    { "set_cursor", "u?oii", wayland_types + 70 },
    { "release", "3", wayland_types + 0 },
};

static const struct wl_message wl_pointer_events[] = {
    { "enter", "uoff", wayland_types + 74 },
    { "leave", "uo", wayland_types + 78 },
    { "motion", "uff", wayland_types + 0 },
    { "button", "uuuu", wayland_types + 0 },
    { "axis", "uuf", wayland_types + 0 },
    { "frame", "5", wayland_types + 0 },
    { "axis_source", "5u", wayland_types + 0 },
    { "axis_stop", "5uu", wayland_types + 0 },
    { "axis_discrete", "5ui", wayland_types + 0 },
    { "axis_value120", "8ui", wayland_types + 0 },
    { "axis_relative_direction", "9uu", wayland_types + 0 },
    { "warp", "11ff", wayland_types + 0 },
};

struct wl_interface wl_pointer_interface = {
    "wl_pointer", 11,
    2, wl_pointer_requests,
    12, wl_pointer_events,
};

// wl_output

static const struct wl_message wl_output_requests[] = {
    { "release", "3", wayland_types + 0 },
};

static const struct wl_message wl_output_events[] = {
    { "geometry", "iiiiissi", wayland_types + 0 },
    { "mode", "uiii", wayland_types + 0 },
    { "done", "2", wayland_types + 0 },
    { "scale", "2i", wayland_types + 0 },
    { "name", "4s", wayland_types + 0 },
    { "description", "4s", wayland_types + 0 },
};

struct wl_interface wl_output_interface = {
    "wl_output", 4,
    1, wl_output_requests,
    6, wl_output_events,
};

// wl_region

static const struct wl_message wl_region_requests[] = {
    { "destroy", "", wayland_types + 0 },
    { "add", "iiii", wayland_types + 0 },
    { "subtract", "iiii", wayland_types + 0 },
};

struct wl_interface wl_region_interface = {
    "wl_region", 7,
    3, wl_region_requests,
    0, NULL,
};

// wl_callback

static const struct wl_message wl_callback_events[] = {
    { "done", "u", wayland_types + 0 },
};

struct wl_interface wl_callback_interface = {
    "wl_callback", 1,
    0, NULL,
    1, wl_callback_events,
};

// xdg-shell types

static const struct wl_interface* xdg_shell_types[] = {
    NULL,                                    // 0
    NULL,                                    // 1
    NULL,                                    // 2
    NULL,                                    // 3
    NULL,                                    // 4 - xdg_positioner
    &xdg_surface_interface,                  // 5
    &wl_surface_interface,                   // 6
    &xdg_toplevel_interface,                 // 7
    NULL,                                    // 8 - xdg_popup
    &xdg_surface_interface,                  // 9
    NULL,                                    // 10 - xdg_positioner
    &xdg_toplevel_interface,                 // 11
    &wl_seat_interface,                      // 12
    NULL,                                    // 13
    NULL,                                    // 14
    NULL,                                    // 15
    &wl_seat_interface,                      // 16
    NULL,                                    // 17
    &wl_seat_interface,                      // 18
    NULL,                                    // 19
    NULL,                                    // 20
    &wl_output_interface,                    // 21
    &wl_seat_interface,                      // 22
    NULL,                                    // 23
    NULL,                                    // 24 - xdg_positioner
    NULL,                                    // 25
};

// xdg_wm_base

static const struct wl_message xdg_wm_base_requests[] = {
    { "destroy", "", xdg_shell_types + 0 },
    { "create_positioner", "n", xdg_shell_types + 4 },
    { "get_xdg_surface", "no", xdg_shell_types + 5 },
    { "pong", "u", xdg_shell_types + 0 },
};

static const struct wl_message xdg_wm_base_events[] = {
    { "ping", "u", xdg_shell_types + 0 },
};

struct wl_interface xdg_wm_base_interface = {
    "xdg_wm_base", 7,
    4, xdg_wm_base_requests,
    1, xdg_wm_base_events,
};

// xdg_surface

static const struct wl_message xdg_surface_requests[] = {
    { "destroy", "", xdg_shell_types + 0 },
    { "get_toplevel", "n", xdg_shell_types + 7 },
    { "get_popup", "n?oo", xdg_shell_types + 8 },
    { "set_window_geometry", "iiii", xdg_shell_types + 0 },
    { "ack_configure", "u", xdg_shell_types + 0 },
};

static const struct wl_message xdg_surface_events[] = {
    { "configure", "u", xdg_shell_types + 0 },
};

struct wl_interface xdg_surface_interface = {
    "xdg_surface", 7,
    5, xdg_surface_requests,
    1, xdg_surface_events,
};

// xdg_toplevel

static const struct wl_message xdg_toplevel_requests[] = {
    { "destroy", "", xdg_shell_types + 0 },
    { "set_parent", "?o", xdg_shell_types + 11 },
    { "set_title", "s", xdg_shell_types + 0 },
    { "set_app_id", "s", xdg_shell_types + 0 },
    { "show_window_menu", "ouii", xdg_shell_types + 12 },
    { "move", "ou", xdg_shell_types + 16 },
    { "resize", "ouu", xdg_shell_types + 18 },
    { "set_max_size", "ii", xdg_shell_types + 0 },
    { "set_min_size", "ii", xdg_shell_types + 0 },
    { "set_maximized", "", xdg_shell_types + 0 },
    { "unset_maximized", "", xdg_shell_types + 0 },
    { "set_fullscreen", "?o", xdg_shell_types + 21 },
    { "unset_fullscreen", "", xdg_shell_types + 0 },
    { "set_minimized", "", xdg_shell_types + 0 },
};

static const struct wl_message xdg_toplevel_events[] = {
    { "configure", "iia", xdg_shell_types + 0 },
    { "close", "", xdg_shell_types + 0 },
    { "configure_bounds", "4ii", xdg_shell_types + 0 },
    { "wm_capabilities", "5a", xdg_shell_types + 0 },
};

struct wl_interface xdg_toplevel_interface = {
    "xdg_toplevel", 7,
    14, xdg_toplevel_requests,
    4, xdg_toplevel_events,
};
