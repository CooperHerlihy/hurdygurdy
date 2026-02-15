#include "hurdygurdy.hpp"

#ifdef HG_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>

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
    HWND hwnd;
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

HINSTANCE hg_internal_win32_instance = nullptr;

void hg_platform_init() {
    hg_internal_win32_instance = GetModuleHandle(nullptr);
}

void hg_platform_deinit() {
    hg_internal_win32_instance = nullptr;
}

static LRESULT CALLBACK hg_internal_window_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    HgWindow::Internals* window = (HgWindow::Internals*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_NCCREATE:
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)(((CREATESTRUCTA*)lparam)->lpCreateParams));
            break;
        case WM_CLOSE:
            window->input.was_closed = true;
            break;
        case WM_SIZE:
            window->input.width = LOWORD(lparam);
            window->input.height = HIWORD(lparam);
            break;
        case WM_KILLFOCUS:
            std::memset(window->input.keys_down, 0, sizeof(window->input.keys_down));
                break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            HgKey key = HgKey::none;
            HgKey shift_key = HgKey::none;

            switch (wparam) {
                case '0':
                    key = HgKey::k0;
                    shift_key = HgKey::rparen;
                    break;
                case '1':
                    key = HgKey::k1;
                    shift_key = HgKey::exclamation;
                    break;
                case '2':
                    key = HgKey::k2;
                    shift_key = HgKey::at;
                    break;
                case '3':
                    key = HgKey::k3;
                    shift_key = HgKey::hash;
                    break;
                case '4':
                    key = HgKey::k4;
                    shift_key = HgKey::dollar;
                    break;
                case '5':
                    key = HgKey::k5;
                    shift_key = HgKey::percent;
                    break;
                case '6':
                    key = HgKey::k6;
                    shift_key = HgKey::carot;
                    break;
                case '7':
                    key = HgKey::k7;
                    shift_key = HgKey::ampersand;
                    break;
                case '8':
                    key = HgKey::k8;
                    shift_key = HgKey::asterisk;
                    break;
                case '9':
                    key = HgKey::k9;
                    shift_key = HgKey::lparen;
                    break;

                case 'A':
                    key = HgKey::a;
                    break;
                case 'B':
                    key = HgKey::b;
                    break;
                case 'C':
                    key = HgKey::c;
                    break;
                case 'D':
                    key = HgKey::d;
                    break;
                case 'E':
                    key = HgKey::e;
                    break;
                case 'F':
                    key = HgKey::f;
                    break;
                case 'G':
                    key = HgKey::g;
                    break;
                case 'H':
                    key = HgKey::h;
                    break;
                case 'I':
                    key = HgKey::i;
                    break;
                case 'J':
                    key = HgKey::j;
                    break;
                case 'K':
                    key = HgKey::k;
                    break;
                case 'L':
                    key = HgKey::l;
                    break;
                case 'M':
                    key = HgKey::m;
                    break;
                case 'N':
                    key = HgKey::n;
                    break;
                case 'O':
                    key = HgKey::o;
                    break;
                case 'P':
                    key = HgKey::p;
                    break;
                case 'Q':
                    key = HgKey::q;
                    break;
                case 'R':
                    key = HgKey::r;
                    break;
                case 'S':
                    key = HgKey::s;
                    break;
                case 'T':
                    key = HgKey::t;
                    break;
                case 'U':
                    key = HgKey::u;
                    break;
                case 'V':
                    key = HgKey::v;
                    break;
                case 'W':
                    key = HgKey::w;
                    break;
                case 'X':
                    key = HgKey::x;
                    break;
                case 'Y':
                    key = HgKey::y;
                    break;
                case 'Z':
                    key = HgKey::z;
                    break;

                case VK_OEM_1:
                    key = HgKey::semicolon;
                    shift_key = HgKey::colon;
                    break;
                case VK_OEM_7:
                    key = HgKey::apostrophe;
                    shift_key = HgKey::quotation;
                    break;
                case VK_OEM_COMMA:
                    key = HgKey::comma;
                    shift_key = HgKey::less;
                    break;
                case VK_OEM_PERIOD:
                    key = HgKey::period;
                    shift_key = HgKey::greater;
                    break;
                case VK_OEM_2:
                    key = HgKey::slash;
                    shift_key = HgKey::question;
                    break;
                case VK_OEM_3:
                    key = HgKey::grave;
                    shift_key = HgKey::tilde;
                    break;
                case VK_OEM_4:
                    key = HgKey::lbracket;
                    shift_key = HgKey::lbrace;
                    break;
                case VK_OEM_6:
                    key = HgKey::rbracket;
                    shift_key = HgKey::rbrace;
                    break;
                case VK_OEM_5:
                    key = HgKey::backslash;
                    shift_key = HgKey::bar;
                    break;
                case VK_OEM_PLUS:
                    key = HgKey::equal;
                    shift_key = HgKey::plus;
                    break;
                case VK_OEM_MINUS:
                    key = HgKey::minus;
                    shift_key = HgKey::underscore;
                    break;

                case VK_UP:
                    key = HgKey::up;
                    break;
                case VK_DOWN:
                    key = HgKey::down;
                    break;
                case VK_LEFT:
                    key = HgKey::left;
                    break;
                case VK_RIGHT:
                    key = HgKey::right;
                    break;
                case VK_ESCAPE:
                    key = HgKey::escape;
                    break;
                case VK_SPACE:
                    key = HgKey::space;
                    break;
                case VK_RETURN:
                    key = HgKey::enter;
                    break;
                case VK_BACK:
                    key = HgKey::backspace;
                    break;
                case VK_DELETE:
                    key = HgKey::kdelete;
                    break;
                case VK_INSERT:
                    key = HgKey::insert;
                    break;
                case VK_TAB:
                    key = HgKey::tab;
                    break;
                case VK_HOME:
                    key = HgKey::home;
                    break;
                case VK_END:
                    key = HgKey::end;
                    break;

                case VK_F1:
                    key = HgKey::f1;
                    break;
                case VK_F2:
                    key = HgKey::f2;
                    break;
                case VK_F3:
                    key = HgKey::f3;
                    break;
                case VK_F4:
                    key = HgKey::f4;
                    break;
                case VK_F5:
                    key = HgKey::f5;
                    break;
                case VK_F6:
                    key = HgKey::f6;
                    break;
                case VK_F7:
                    key = HgKey::f7;
                    break;
                case VK_F8:
                    key = HgKey::f8;
                    break;
                case VK_F9:
                    key = HgKey::f9;
                    break;
                case VK_F10:
                    key = HgKey::f10;
                    break;
                case VK_F11:
                    key = HgKey::f11;
                    break;
                case VK_F12:
                    key = HgKey::f12;
                    break;

                case VK_SHIFT: {
                    u32 scancode = (lparam >> 16) & 0xff;
                    if (scancode == 0x36)
                        key = HgKey::rshift;
                    else if (scancode == 0x2A)
                        key = HgKey::lshift;
                } break;
                case VK_MENU:
                    if (lparam & (1 << 24))
                        key = HgKey::ralt;
                    else
                        key = HgKey::lalt;
                    break;
                case VK_CONTROL:
                    if (lparam & (1 << 24))
                        key = HgKey::rctrl;
                    else
                        key = HgKey::lctrl;
                    break;
                case VK_LWIN:
                    key = HgKey::lsuper;
                    break;
                case VK_RWIN:
                    key = HgKey::rsuper;
                    break;
                case VK_CAPITAL:
                    key = HgKey::capslock;
                    break;
            }
            if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
                if (shift_key != HgKey::none &&
                   (window->input.keys_down[(u32)HgKey::lshift] ||
                    window->input.keys_down[(u32)HgKey::rshift])
                ) {
                    window->input.keys_pressed[(u32)shift_key] = true;
                    window->input.keys_down[(u32)shift_key] = true;
                } else {
                    window->input.keys_pressed[(u32)key] = true;
                    window->input.keys_down[(u32)key] = true;
                }
            } else {
                window->input.keys_released[(u32)shift_key] = window->input.keys_down[(u32)shift_key];
                window->input.keys_down[(u32)shift_key] = false;
                window->input.keys_released[(u32)key] = window->input.keys_down[(u32)key];
                window->input.keys_down[(u32)key] = false;
            }
        } break;
        case WM_LBUTTONDOWN:
            window->input.keys_pressed[(u32)HgKey::lmouse] = true;
            window->input.keys_down[(u32)HgKey::lmouse] = true;
            break;
        case WM_RBUTTONDOWN:
            window->input.keys_pressed[(u32)HgKey::rmouse] = true;
            window->input.keys_down[(u32)HgKey::rmouse] = true;
            break;
        case WM_MBUTTONDOWN:
            window->input.keys_pressed[(u32)HgKey::mmouse] = true;
            window->input.keys_down[(u32)HgKey::mmouse] = true;
            break;
        case WM_LBUTTONUP:
            window->input.keys_released[(u32)HgKey::lmouse] = true;
            window->input.keys_down[(u32)HgKey::lmouse] = false;
            break;
        case WM_RBUTTONUP:
            window->input.keys_released[(u32)HgKey::rmouse] = true;
            window->input.keys_down[(u32)HgKey::rmouse] = false;
            break;
        case WM_MBUTTONUP:
            window->input.keys_released[(u32)HgKey::mmouse] = true;
            window->input.keys_down[(u32)HgKey::mmouse] = false;
            break;
        case WM_MOUSEMOVE:
            window->input.mouse_pos_x = (f64)LOWORD(lparam) / (f64)window->input.height;
            window->input.mouse_pos_y = (f64)HIWORD(lparam) / (f64)window->input.height;
            break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

HgWindow HgWindow::create(HgArena& arena, const HgWindow::Config& config) {
    const char* title = config.title != nullptr ? config.title : "Hurdy Gurdy";

    HgWindow window;
    window.internals = arena.alloc<Internals>(1);
    *window.internals = {};

    WNDCLASSA window_class{};
    window_class.hInstance = hg_internal_win32_instance;
    window_class.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.lpszClassName = title;
    window_class.lpfnWndProc = hg_internal_window_callback;
    if (!RegisterClassA(&window_class))
        hg_error("Win32 failed to register window class for window: %s\n", config.title);

    if (config.windowed) {
        window.internals->input.width = config.width;
        window.internals->input.height = config.height;
        window.internals->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            window.internals->input.width,
            window.internals->input.height,
            nullptr,
            nullptr,
            hg_internal_win32_instance,
            window.internals
        );
    } else {
        window.internals->input.width = GetSystemMetrics(SM_CXSCREEN);
        window.internals->input.height = GetSystemMetrics(SM_CYSCREEN);
        window.internals->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_POPUP,
            0,
            0,
            window.internals->input.width,
            window.internals->input.height,
            nullptr,
            nullptr,
            hg_internal_win32_instance,
            window.internals
        );
    }
    if (window.internals->hwnd == nullptr)
        hg_error("Win32 window creation failed\n");

    ShowWindow(window.internals->hwnd, SW_SHOW);
    return window;
}

void HgWindow::destroy() {
    DestroyWindow(internals->hwnd);
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

    PFN_vkCreateWin32SurfaceKHR pfn_vkCreateWin32SurfaceKHR
        = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    if (pfn_vkCreateWin32SurfaceKHR == nullptr)
        hg_error("Could not load vkCreateWin32SurfaceKHR\n");

    VkWin32SurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hinstance = hg_internal_win32_instance;
    info.hwnd = window.internals->hwnd;

    VkSurfaceKHR surface = nullptr;
    VkResult result = pfn_vkCreateWin32SurfaceKHR(instance, &info, nullptr, &surface);
    if (surface == nullptr)
        hg_error("Failed to create Vulkan surface: %s\n", hg_vk_result_string(result));

    hg_assert(surface != nullptr);
    return surface;
}

void hg_process_window_events(const HgWindow* windows, usize window_count) {
    hg_assert(windows != nullptr);

    for (usize i = 0; i < window_count; ++i) {
        HgWindow::Internals* window = windows[i].internals;

        std::memset(window->input.keys_pressed, 0, sizeof(window->input.keys_pressed));
        std::memset(window->input.keys_released, 0, sizeof(window->input.keys_released));
        window->input.was_resized = false;

        u32 old_window_width = window->input.width;
        u32 old_window_height = window->input.height;
        f64 old_mouse_pos_x = window->input.mouse_pos_x;
        f64 old_mouse_pos_y = window->input.mouse_pos_y;

        MSG msg;
        while (PeekMessageA(&msg, window->hwnd, 0, 0, PM_REMOVE) != 0) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (window->input.width != old_window_width || window->input.height != old_window_height) {
            window->input.was_resized = true;
        }

        window->input.mouse_delta_x = window->input.mouse_pos_x - old_mouse_pos_x;
        window->input.mouse_delta_y = window->input.mouse_pos_y - old_mouse_pos_y;

        if (window->input.keys_down[(u32)HgKey::lshift] && window->input.keys_down[(u32)HgKey::rshift]) {
            bool lshift = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
            bool rshift = (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;
            if (!lshift) {
                window->input.keys_released[(u32)HgKey::lshift] = true;
                window->input.keys_down[(u32)HgKey::lshift] = false;
            }
            if (!rshift) {
                window->input.keys_released[(u32)HgKey::rshift] = true;
                window->input.keys_down[(u32)HgKey::rshift] = false;
            }
        }
    }
}

#endif
