#include "hurdygurdy.hpp"

#include <GLFW/glfw3.h>

#include "hurdygurdy.hpp"

#ifdef HG_PLATFORM_LINUX

void hg_internal_create_window_swapchain(HgWindow* window, const HgWindowConfig& config);
void hg_internal_resize_window_swapchain(HgWindow* window);
void hg_internal_destroy_window_swapchain(HgWindow* window);

struct HgWindow::Internals {
    GLFWwindow* glfw_window;
};

void hg_platform_init() {
    glfwInit();
}

void hg_platform_deinit() {
    glfwTerminate();
}

void window_size_callback(GLFWwindow* glfw_window, int width, int height) {
    HgWindow* window = (HgWindow*)glfwGetWindowUserPointer(glfw_window);
    window->width = (u32)width;
    window->height = (u32)height;
}

static void key_callback(
    GLFWwindow* glfw_window,
    int glfw_key,
    [[maybe_unused]] int scancode,
    int action,
    [[maybe_unused]] int mods
) {
    HgWindow* window = (HgWindow*)glfwGetWindowUserPointer(glfw_window);

    bool is_shift_down =
        window->is_key_down[(u32)HgKey::lshift] ||
        window->is_key_down[(u32)HgKey::rshift];

    HgKey key = HgKey::none;
    switch (glfw_key) {
        case GLFW_KEY_0:
            key = is_shift_down ? HgKey::rparen : HgKey::k0;
            break;
        case GLFW_KEY_1:
            key = is_shift_down ? HgKey::exclamation : HgKey::k1;
            break;
        case GLFW_KEY_2:
            key = is_shift_down ? HgKey::at : HgKey::k2;
            break;
        case GLFW_KEY_3:
            key = is_shift_down ? HgKey::hash : HgKey::k3;
            break;
        case GLFW_KEY_4:
            key = is_shift_down ? HgKey::dollar : HgKey::k4;
            break;
        case GLFW_KEY_5:
            key = is_shift_down ? HgKey::percent : HgKey::k5;
            break;
        case GLFW_KEY_6:
            key = is_shift_down ? HgKey::carot : HgKey::k6;
            break;
        case GLFW_KEY_7:
            key = is_shift_down ? HgKey::ampersand : HgKey::k7;
            break;
        case GLFW_KEY_8:
            key = is_shift_down ? HgKey::asterisk : HgKey::k8;
            break;
        case GLFW_KEY_9:
            key = is_shift_down ? HgKey::lparen : HgKey::k9;
            break;

        case GLFW_KEY_Q:
            key = HgKey::q;
            break;
        case GLFW_KEY_W:
            key = HgKey::w;
            break;
        case GLFW_KEY_E:
            key = HgKey::e;
            break;
        case GLFW_KEY_R:
            key = HgKey::r;
            break;
        case GLFW_KEY_T:
            key = HgKey::t;
            break;
        case GLFW_KEY_Y:
            key = HgKey::y;
            break;
        case GLFW_KEY_U:
            key = HgKey::u;
            break;
        case GLFW_KEY_I:
            key = HgKey::i;
            break;
        case GLFW_KEY_O:
            key = HgKey::o;
            break;
        case GLFW_KEY_P:
            key = HgKey::p;
            break;
        case GLFW_KEY_A:
            key = HgKey::a;
            break;
        case GLFW_KEY_S:
            key = HgKey::s;
            break;
        case GLFW_KEY_D:
            key = HgKey::d;
            break;
        case GLFW_KEY_F:
            key = HgKey::f;
            break;
        case GLFW_KEY_G:
            key = HgKey::g;
            break;
        case GLFW_KEY_H:
            key = HgKey::h;
            break;
        case GLFW_KEY_J:
            key = HgKey::j;
            break;
        case GLFW_KEY_K:
            key = HgKey::k;
            break;
        case GLFW_KEY_L:
            key = HgKey::l;
            break;
        case GLFW_KEY_Z:
            key = HgKey::z;
            break;
        case GLFW_KEY_X:
            key = HgKey::x;
            break;
        case GLFW_KEY_C:
            key = HgKey::c;
            break;
        case GLFW_KEY_V:
            key = HgKey::v;
            break;
        case GLFW_KEY_B:
            key = HgKey::b;
            break;
        case GLFW_KEY_N:
            key = HgKey::n;
            break;
        case GLFW_KEY_M:
            key = HgKey::m;
            break;

        case GLFW_KEY_SEMICOLON:
            key = is_shift_down ? HgKey::colon : HgKey::semicolon;
            break;
        case GLFW_KEY_APOSTROPHE:
            key = is_shift_down ? HgKey::quotation : HgKey::apostrophe;
            break;
        case GLFW_KEY_COMMA:
            key = is_shift_down ? HgKey::less : HgKey::comma;
            break;
        case GLFW_KEY_PERIOD:
            key = is_shift_down ? HgKey::greater : HgKey::period;
            break;
        case GLFW_KEY_SLASH:
            key = is_shift_down ? HgKey::question : HgKey::slash;
            break;
        case GLFW_KEY_BACKSLASH:
            key = is_shift_down ? HgKey::bar : HgKey::backslash;
            break;
        case GLFW_KEY_LEFT_BRACKET:
            key = is_shift_down ? HgKey::lbrace : HgKey::lbracket;
            break;
        case GLFW_KEY_RIGHT_BRACKET:
            key = is_shift_down ? HgKey::rbrace : HgKey::rbracket;
            break;
        case GLFW_KEY_GRAVE_ACCENT:
            key = is_shift_down ? HgKey::tilde : HgKey::grave;
            break;
        case GLFW_KEY_MINUS:
            key = is_shift_down ? HgKey::underscore : HgKey::minus;
            break;
        case GLFW_KEY_EQUAL:
            key = is_shift_down ? HgKey::plus : HgKey::equal;
            break;

        case GLFW_KEY_UP:
            key = HgKey::up;
            break;
        case GLFW_KEY_DOWN:
            key = HgKey::down;
            break;
        case GLFW_KEY_LEFT:
            key = HgKey::left;
            break;
        case GLFW_KEY_RIGHT:
            key = HgKey::right;
            break;

        case GLFW_KEY_ESCAPE:
            key = HgKey::escape;
            break;
        case GLFW_KEY_SPACE:
            key = HgKey::space;
            break;
        case GLFW_KEY_ENTER:
            key = HgKey::enter;
            break;
        case GLFW_KEY_BACKSPACE:
            key = HgKey::backspace;
            break;
        case GLFW_KEY_DELETE:
            key = HgKey::kdelete;
            break;
        case GLFW_KEY_INSERT:
            key = HgKey::insert;
            break;
        case GLFW_KEY_TAB:
            key = HgKey::tab;
            break;
        case GLFW_KEY_HOME:
            key = HgKey::home;
            break;
        case GLFW_KEY_END:
            key = HgKey::end;
            break;

        case GLFW_KEY_F1:
            key = HgKey::f1;
            break;
        case GLFW_KEY_F2:
            key = HgKey::f2;
            break;
        case GLFW_KEY_F3:
            key = HgKey::f3;
            break;
        case GLFW_KEY_F4:
            key = HgKey::f4;
            break;
        case GLFW_KEY_F5:
            key = HgKey::f5;
            break;
        case GLFW_KEY_F6:
            key = HgKey::f6;
            break;
        case GLFW_KEY_F7:
            key = HgKey::f7;
            break;
        case GLFW_KEY_F8:
            key = HgKey::f8;
            break;
        case GLFW_KEY_F9:
            key = HgKey::f9;
            break;
        case GLFW_KEY_F10:
            key = HgKey::f10;
            break;
        case GLFW_KEY_F11:
            key = HgKey::f11;
            break;
        case GLFW_KEY_F12:
            key = HgKey::f12;
            break;

        case GLFW_KEY_LEFT_SHIFT:
            key = HgKey::lshift;
            break;
        case GLFW_KEY_RIGHT_SHIFT:
            key = HgKey::rshift;
            break;
        case GLFW_KEY_LEFT_CONTROL:
            key = HgKey::lctrl;
            break;
        case GLFW_KEY_RIGHT_CONTROL:
            key = HgKey::rctrl;
            break;
        case GLFW_KEY_LEFT_ALT:
            key = HgKey::lalt;
            break;
        case GLFW_KEY_RIGHT_ALT:
            key = HgKey::ralt;
            break;
        case GLFW_KEY_LEFT_SUPER:
            key = HgKey::lsuper;
            break;
        case GLFW_KEY_RIGHT_SUPER:
            key = HgKey::rsuper;
            break;
        case GLFW_KEY_CAPS_LOCK:
            key = HgKey::capslock;
            break;
    }

    if (action == GLFW_PRESS) {
        window->is_key_down[(u32)key] = true;
        window->was_key_pressed[(u32)key] = true;
    } else if (action == GLFW_RELEASE) {
        window->is_key_down[(u32)key] = false;
        window->was_key_released[(u32)key] = true;
    }
}

static void mouse_pos_callback(GLFWwindow* glfw_window, double x, double y) {
    HgWindow* window = (HgWindow*)glfwGetWindowUserPointer(glfw_window);
    window->mouse_pos_x = x / window->height;
    window->mouse_pos_y = y / window->height;
}

void mouse_button_callback(GLFWwindow* glfw_window, int button, int action, [[maybe_unused]] int mods) {
    HgWindow* window = (HgWindow*)glfwGetWindowUserPointer(glfw_window);

    HgKey key = HgKey::none;
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            key = HgKey::lmouse;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            key = HgKey::rmouse;
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            key = HgKey::mmouse;
            break;
        case GLFW_MOUSE_BUTTON_4:
            key = HgKey::mouse4;
            break;
        case GLFW_MOUSE_BUTTON_5:
            key = HgKey::mouse5;
            break;
    }
    if (action == GLFW_PRESS) {
        window->is_key_down[(u32)key] = true;
        window->was_key_pressed[(u32)key] = true;
    } else if (action == GLFW_RELEASE) {
        window->is_key_down[(u32)key] = false;
        window->was_key_released[(u32)key] = true;
    }
}

HgWindow* HgWindow::create(HgArena& arena, const HgWindowConfig& config) {
    HgWindow* window = arena.alloc<HgWindow>(1);
    *window = {};
    window->internals = arena.alloc<Internals>(1);
    *window->internals = {};

    const char* title = config.title != nullptr ? config.title : "Hurdy Gurdy";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (config.windowed) {
        window->width = config.width;
        window->height = config.height;
        window->internals->glfw_window = glfwCreateWindow(
            (int)config.width,
            (int)config.height,
            title,
            nullptr,
            nullptr);
    } else {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        window->width = (u32)mode->width;
        window->height = (u32)mode->height;
        window->internals->glfw_window = glfwCreateWindow(
            mode->width,
            mode->height,
            title,
            monitor,
            nullptr);
    }

    glfwSetWindowUserPointer(window->internals->glfw_window, window);
    glfwSetWindowSizeCallback(window->internals->glfw_window, window_size_callback);
    glfwSetKeyCallback(window->internals->glfw_window, key_callback);
    glfwSetCursorPosCallback(window->internals->glfw_window, mouse_pos_callback);
    glfwSetMouseButtonCallback(window->internals->glfw_window, mouse_button_callback);

    VkResult result = glfwCreateWindowSurface(
        hg_vk_instance,
        window->internals->glfw_window,
        nullptr,
        &window->surface);
    if (window->surface == nullptr)
        hg_error("Failed to create Vulkan surface: %s\n", hg_vk_result_to_string(result));

    hg_internal_create_window_swapchain(window, config);

    return window;
}

void HgWindow::destroy() {
    hg_internal_destroy_window_swapchain(this);
    vkDestroySurfaceKHR(hg_vk_instance, surface, nullptr);
    glfwDestroyWindow(internals->glfw_window);
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

u32 hg_vk_get_platform_extensions(HgArena& arena, HgStringView** ext_buffer) {
    u32 ext_count;
    const char** exts = glfwGetRequiredInstanceExtensions(&ext_count);
    if (exts == nullptr)
        hg_error("Could not get required instance extensions from glfw\n");

    *ext_buffer = arena.alloc<HgStringView>(ext_count);
    for (usize i = 0; i < ext_count; ++i) {
        (*ext_buffer)[i] = exts[i];
    }

    return ext_count;
}

void hg_process_window_events(HgWindow** windows, usize window_count) {
    HgArena& scratch = hg_get_scratch();
    HgArenaScope scratch_scope{scratch};

    u32* old_widths = scratch.alloc<u32>(window_count);
    u32* old_heights = scratch.alloc<u32>(window_count);
    f64* old_mouse_xs = scratch.alloc<f64>(window_count);
    f64* old_mouse_ys = scratch.alloc<f64>(window_count);

    for (usize i = 0; i < window_count; ++i) {
        HgWindow* window = windows[i];

        old_widths[i] = window->width;
        old_heights[i] = window->height;
        old_mouse_xs[i] = window->mouse_pos_x;
        old_mouse_ys[i] = window->mouse_pos_y;

        memset(window->was_key_pressed, 0, sizeof(window->was_key_pressed));
        memset(window->was_key_released, 0, sizeof(window->was_key_released));
    }

    glfwPollEvents();

    for (usize i = 0; i < window_count; ++i) {
        HgWindow* window = windows[i];

        if (window->width != old_widths[i] || window->height != old_heights[i])
            hg_internal_resize_window_swapchain(window);

        window->was_closed = glfwWindowShouldClose(window->internals->glfw_window);

        window->mouse_delta_x = window->mouse_pos_x - old_mouse_xs[i];
        window->mouse_delta_y = window->mouse_pos_y - old_mouse_ys[i];
    }
}

#include "imgui_impl_glfw.h"

void ImGui_ImplHurdyGurdy_Init(HgWindow* window) {
    ImGui_ImplGlfw_InitForVulkan(window->internals->glfw_window, true);
}

void ImGui_ImplHurdyGurdy_Shutdown() {
    ImGui_ImplGlfw_Shutdown();
}

void ImGui_ImplHurdyGurdy_NewFrame() {
    ImGui_ImplGlfw_NewFrame();
}

#endif

