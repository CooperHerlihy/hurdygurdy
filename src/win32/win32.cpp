#include "win32_audio.hpp"

#include "win32_window.hpp"

#include "internal.hpp"

#include "hg/error.hpp"
#include "hg/hash.hpp"

#include <span>
#include <string_view>
#include <string>
#include <vector>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include <stdlib.h>
#include <string.h>

namespace hg {

static Button win32VirtKeyToButton(u32 vk)
{
    switch (vk)
    {
        case '0': return Button_0;
        case '1': return Button_1;
        case '2': return Button_2;
        case '3': return Button_3;
        case '4': return Button_4;
        case '5': return Button_5;
        case '6': return Button_6;
        case '7': return Button_7;
        case '8': return Button_8;
        case '9': return Button_9;
        case 'Q': return Button_q;
        case 'W': return Button_w;
        case 'E': return Button_e;
        case 'R': return Button_r;
        case 'T': return Button_t;
        case 'Y': return Button_y;
        case 'U': return Button_u;
        case 'I': return Button_i;
        case 'O': return Button_o;
        case 'P': return Button_p;
        case 'A': return Button_a;
        case 'S': return Button_s;
        case 'D': return Button_d;
        case 'F': return Button_f;
        case 'G': return Button_g;
        case 'H': return Button_h;
        case 'J': return Button_j;
        case 'K': return Button_k;
        case 'L': return Button_l;
        case 'Z': return Button_z;
        case 'X': return Button_x;
        case 'C': return Button_c;
        case 'V': return Button_v;
        case 'B': return Button_b;
        case 'N': return Button_n;
        case 'M': return Button_m;
        case VK_OEM_1: return Button_semicolon;
        case VK_OEM_7: return Button_apostrophe;
        case VK_OEM_COMMA: return Button_comma;
        case VK_OEM_PERIOD: return Button_period;
        case VK_OEM_3: return Button_grave;
        case VK_OEM_4: return Button_lbracket;
        case VK_OEM_6: return Button_rbracket;
        case VK_OEM_PLUS: return Button_equal;
        case VK_OEM_MINUS: return Button_minus;
        case VK_OEM_2: return Button_slash;
        case VK_OEM_5: return Button_backslash;
        case VK_UP: return Button_up;
        case VK_DOWN: return Button_down;
        case VK_LEFT: return Button_left;
        case VK_RIGHT: return Button_right;
        case VK_ESCAPE: return Button_escape;
        case VK_SPACE: return Button_space;
        case VK_RETURN: return Button_enter;
        case VK_BACK: return Button_backspace;
        case VK_DELETE: return Button_kdelete;
        case VK_INSERT: return Button_insert;
        case VK_TAB: return Button_tab;
        case VK_HOME: return Button_home;
        case VK_END: return Button_end;
        case VK_PRIOR: return Button_pageup;
        case VK_NEXT: return Button_pagedown;
        case VK_F1: return Button_f1;
        case VK_F2: return Button_f2;
        case VK_F3: return Button_f3;
        case VK_F4: return Button_f4;
        case VK_F5: return Button_f5;
        case VK_F6: return Button_f6;
        case VK_F7: return Button_f7;
        case VK_F8: return Button_f8;
        case VK_F9: return Button_f9;
        case VK_F10: return Button_f10;
        case VK_F11: return Button_f11;
        case VK_F12: return Button_f12;
        case VK_SNAPSHOT: return Button_printscreen;
        case VK_APPS: return Button_context;
        case VK_NUMPAD0: return Button_numpad0;
        case VK_NUMPAD1: return Button_numpad1;
        case VK_NUMPAD2: return Button_numpad2;
        case VK_NUMPAD3: return Button_numpad3;
        case VK_NUMPAD4: return Button_numpad4;
        case VK_NUMPAD5: return Button_numpad5;
        case VK_NUMPAD6: return Button_numpad6;
        case VK_NUMPAD7: return Button_numpad7;
        case VK_NUMPAD8: return Button_numpad8;
        case VK_NUMPAD9: return Button_numpad9;
        case VK_DECIMAL: return Button_numpaddecimal;
        case VK_DIVIDE: return Button_numpaddiv;
        case VK_MULTIPLY: return Button_numpadmul;
        case VK_SUBTRACT: return Button_numpadminus;
        case VK_ADD: return Button_numpadplus;
        case VK_SEPARATOR: return Button_numpadenter;
        case VK_LSHIFT: return Button_lshift;
        case VK_RSHIFT: return Button_rshift;
        case VK_LCONTROL: return Button_lctrl;
        case VK_RCONTROL: return Button_rctrl;
        case VK_LMENU: return Button_lalt;
        case VK_RMENU: return Button_ralt;
        case VK_LWIN: return Button_lsuper;
        case VK_RWIN: return Button_rsuper;
        case VK_CAPITAL: return Button_capslock;
        case VK_NUMLOCK: return Button_numlock;
        case VK_SCROLL: return Button_scrolllock;
        case VK_PAUSE: return Button_pause;
    }
    return Button_none;
}

std::unique_ptr<InputDeviceWin32> InputDeviceWin32::create()
{
    std::unique_ptr<InputDeviceWin32> device = std::make_unique<InputDeviceWin32>();

    device->hInstance = GetModuleHandleW(nullptr);
    if (device->hInstance == nullptr)
    {
        setError("Could not get module handle");
        return nullptr;
    }

    // Prefer per-monitor DPI awareness when available
    using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    auto setProcessDpiAwarenessContext = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
        GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (setProcessDpiAwarenessContext != nullptr)
    {
        if (!setProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
            SetProcessDPIAware();
    }
    else
    {
        SetProcessDPIAware();
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &InputDeviceWin32::wndProc;
    wc.hInstance = device->hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"HurdyGurdyWindow";

    device->windowClass = RegisterClassExW(&wc);
    if (device->windowClass == 0)
    {
        setError("Could not register window class");
        return nullptr;
    }

    device->cursors[CursorType_arrow] = LoadCursorW(nullptr, IDC_ARROW);
    device->cursors[CursorType_textInput] = LoadCursorW(nullptr, IDC_IBEAM);
    device->cursors[CursorType_resizeAll] = LoadCursorW(nullptr, IDC_SIZEALL);
    device->cursors[CursorType_resizeNS] = LoadCursorW(nullptr, IDC_SIZENS);
    device->cursors[CursorType_resizeEW] = LoadCursorW(nullptr, IDC_SIZEWE);
    device->cursors[CursorType_resizeNESW] = LoadCursorW(nullptr, IDC_SIZENESW);
    device->cursors[CursorType_resizeNWSE] = LoadCursorW(nullptr, IDC_SIZENWSE);
    device->cursors[CursorType_hand] = LoadCursorW(nullptr, IDC_HAND);
    device->cursors[CursorType_wait] = LoadCursorW(nullptr, IDC_WAIT);
    device->cursors[CursorType_progress] = LoadCursorW(nullptr, IDC_APPSTARTING);
    device->cursors[CursorType_notAllowed] = LoadCursorW(nullptr, IDC_NO);

    // Enumerate displays
    using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, i32, u32*, u32*);
    HMODULE shcore = GetModuleHandleW(L"Shcore.dll");
    GetDpiForMonitorFn getDpiForMonitor = nullptr;
    if (shcore != nullptr)
        getDpiForMonitor = reinterpret_cast<GetDpiForMonitorFn>(GetProcAddress(shcore, "GetDpiForMonitor"));

    struct EnumData {
        std::vector<DisplayInfo>* displays;
        GetDpiForMonitorFn getDpiForMonitor;
    } enumData{&device->displays, getDpiForMonitor};

    MONITORENUMPROC monitorEnumProc = [](HMONITOR hmon, HDC, LPRECT, LPARAM lp) -> BOOL
    {
        EnumData* data = reinterpret_cast<EnumData*>(lp);

        MONITORINFOEXW mi{};
        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfoW(hmon, &mi))
            return TRUE;

        DisplayInfo info{};
        info.posX = mi.rcMonitor.left;
        info.posY = mi.rcMonitor.top;
        info.sizeW = static_cast<u32>(mi.rcMonitor.right - mi.rcMonitor.left);
        info.sizeH = static_cast<u32>(mi.rcMonitor.bottom - mi.rcMonitor.top);
        info.workPosX = mi.rcWork.left;
        info.workPosY = mi.rcWork.top;
        info.workSizeW = static_cast<u32>(mi.rcWork.right - mi.rcWork.left);
        info.workSizeH = static_cast<u32>(mi.rcWork.bottom - mi.rcWork.top);
        info.dpiScale = 1.0f;

        if (data->getDpiForMonitor != nullptr)
        {
            u32 dpiX = 96;
            u32 dpiY = 96;
            if (SUCCEEDED(data->getDpiForMonitor(hmon, 0, &dpiX, &dpiY)))
                info.dpiScale = static_cast<f32>(dpiX) / 96.0f;
        }

        data->displays->push_back(info);
        return TRUE;
    };

    EnumDisplayMonitors(nullptr, nullptr, monitorEnumProc, reinterpret_cast<LPARAM>(&enumData));

    if (device->displays.empty())
    {
        DisplayInfo info{};
        info.sizeW = static_cast<u32>(GetSystemMetrics(SM_CXSCREEN));
        info.sizeH = static_cast<u32>(GetSystemMetrics(SM_CYSCREEN));
        info.workSizeW = info.sizeW;
        info.workSizeH = info.sizeH;
        info.dpiScale = 1.0f;
        device->displays.push_back(info);
    }

    return device;
}

InputDeviceWin32::~InputDeviceWin32()
{
    windows.clear();

    if (windowClass != 0 && hInstance != nullptr)
        UnregisterClassW(L"HurdyGurdyWindow", hInstance);
}

std::span<std::string_view> InputDeviceWin32::getVulkanExtensions()
{
    static std::string_view exts[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
    };
    return exts;
}

std::span<DisplayInfo> InputDeviceWin32::displayInfo()
{
    return displays;
}

LRESULT CALLBACK InputDeviceWin32::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    WindowWin32* w = reinterpret_cast<WindowWin32*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    InputDeviceWin32* dev = w != nullptr ? static_cast<InputDeviceWin32*>(w->device) : nullptr;

    switch (msg)
    {
        case WM_CLOSE:
        {
            if (dev != nullptr)
            {
                w->closed = true;

                Event event{};
                event.type = EventType_windowClosed;
                event.window.window = w;
                dev->events.push_back(event);
                w->eventQueue.push_back(event);
            }
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_DESTROY:
        {
            if (dev != nullptr)
            {
                dev->windows.remove(std::bit_cast<u64>(hwnd));

                if (dev->windows.count == 0)
                {
                    dev->quit = true;

                    Event event{};
                    event.type = EventType_quit;
                    dev->events.push_back(event);
                }
            }
            return 0;
        }
        case WM_SIZE:
        {
            u32 width = static_cast<u32>(LOWORD(lParam));
            u32 height = static_cast<u32>(HIWORD(lParam));
            if (width == 0 || height == 0 || dev == nullptr)
                return 0;

            w->swap.resize(width, height);
            w->width = width;
            w->height = height;
            w->resized = true;

            Event event{};
            event.type = EventType_windowResized;
            event.window.window = w;
            event.window.width = width;
            event.window.height = height;
            dev->events.push_back(event);
            w->eventQueue.push_back(event);
            return 0;
        }
        case WM_SETFOCUS:
        {
            if (dev != nullptr && !w->focused)
            {
                w->focused = true;
                w->focusGained = true;
                dev->activeWindow = w;

                Event event{};
                event.type = EventType_windowFocused;
                event.window.window = w;
                dev->events.push_back(event);
                w->eventQueue.push_back(event);
            }
            return 0;
        }
        case WM_KILLFOCUS:
        {
            if (dev != nullptr && w->focused)
            {
                w->focused = false;
                w->focusLost = true;
                if (dev->activeWindow == w)
                    dev->activeWindow = nullptr;

                Event event{};
                event.type = EventType_windowUnfocused;
                event.window.window = w;
                dev->events.push_back(event);
                w->eventQueue.push_back(event);
            }
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            if (dev == nullptr)
                return 0;

            Vec2 newPos{static_cast<f32>(GET_X_LPARAM(lParam)), static_cast<f32>(GET_Y_LPARAM(lParam))};
            Vec2 delta = newPos - w->mouse;
            w->mouse = newPos;
            dev->mouseDeltaAccum += delta;

            POINT globalPt{};
            GetCursorPos(&globalPt);

            Event event{};
            event.type = EventType_mouseMoved;
            event.mouse.delta = delta;
            event.mouse.pos = newPos;
            event.mouse.globalPos = Vec2{static_cast<f32>(globalPt.x), static_cast<f32>(globalPt.y)};
            dev->events.push_back(event);
            w->eventQueue.push_back(event);
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            if (dev == nullptr)
                return 0;

            f32 delta = static_cast<f32>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            dev->wheelDeltaAccum.y += delta;

            Event event{};
            event.type = EventType_wheelMoved;
            event.wheel.delta = Vec2{0.0f, delta};
            dev->events.push_back(event);
            w->eventQueue.push_back(event);
            return 0;
        }
        case WM_MOUSEHWHEEL:
        {
            if (dev == nullptr)
                return 0;

            f32 delta = static_cast<f32>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            dev->wheelDeltaAccum.x += delta;

            Event event{};
            event.type = EventType_wheelMoved;
            event.wheel.delta = Vec2{delta, 0.0f};
            dev->events.push_back(event);
            w->eventQueue.push_back(event);
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            if (dev == nullptr)
                return 0;

            bool pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN ||
                msg == WM_MBUTTONDOWN || msg == WM_XBUTTONDOWN;

            Button button = Button_none;
            switch (msg)
            {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP: button = Button_mouse1; break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP: button = Button_mouse2; break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP: button = Button_mouse3; break;
                case WM_XBUTTONDOWN:
                case WM_XBUTTONUP:
                {
                    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                        button = Button_mouse4;
                    else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
                        button = Button_mouse5;
                    break;
                }
            }

            if (button != Button_none)
            {
                dev->isKeyDown[button] = pressed;

                Event event{};
                event.type = pressed ? EventType_keyPress : EventType_keyRelease;
                event.button = button;
                dev->events.push_back(event);
                w->eventQueue.push_back(event);
            }
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (dev == nullptr)
                return 0;

            Button button = win32VirtKeyToButton(static_cast<u32>(wParam));
            if (button != Button_none)
            {
                dev->isKeyDown[button] = true;

                Event event{};
                event.type = EventType_keyPress;
                event.button = button;
                dev->events.push_back(event);
                w->eventQueue.push_back(event);
            }
            return 0;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            if (dev == nullptr)
                return 0;

            Button button = win32VirtKeyToButton(static_cast<u32>(wParam));
            if (button != Button_none)
            {
                dev->isKeyDown[button] = false;

                Event event{};
                event.type = EventType_keyRelease;
                event.button = button;
                dev->events.push_back(event);
                w->eventQueue.push_back(event);
            }
            return 0;
        }
        case WM_CHAR:
        {
            if (dev == nullptr)
                return 0;

            wchar_t ch = static_cast<wchar_t>(wParam);
            if (ch >= 0x20 && ch != 0x7F)
            {
                static wchar_t pendingHigh = 0;
                if (ch >= 0xD800 && ch < 0xDC00)
                {
                    pendingHigh = ch;
                }
                else
                {
                    wchar_t units[2] = {ch, 0};
                    int unitCount = 1;
                    if (ch >= 0xDC00 && ch < 0xE000 && pendingHigh != 0)
                    {
                        units[0] = pendingHigh;
                        units[1] = ch;
                        unitCount = 2;
                    }
                    pendingHigh = 0;

                    char utf8[8]{};
                    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, units, unitCount, utf8, sizeof(utf8) - 1, nullptr, nullptr);
                    if (utf8Len > 0)
                    {
                        Event event{};
                        event.type = EventType_text;
                        std::memset(event.text, 0, sizeof(event.text));
                        std::memcpy(event.text, utf8, static_cast<u64>(utf8Len) < sizeof(event.text) ? static_cast<u64>(utf8Len) : sizeof(event.text));
                        dev->events.push_back(event);
                        w->eventQueue.push_back(event);
                    }
                }
            }
            return 0;
        }
        case WM_SETCURSOR:
        {
            if (dev != nullptr && LOWORD(lParam) == HTCLIENT)
            {
                if (dev->currentCursor != nullptr)
                    SetCursor(dev->currentCursor);
                return TRUE;
            }
            break;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void InputDeviceWin32::processEvents()
{
    std::memcpy(wasKeyDown, isKeyDown, sizeof(isKeyDown));
    mouseDeltaAccum = Vec2{};
    wheelDeltaAccum = Vec2{};

    windows.forEach([](u64, WindowWin32* data)
    {
        data->eventQueue.clear();
        data->focusGained = false;
        data->focusLost = false;
        data->resized = false;
    });

    events.clear();

    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            quit = true;

            Event event{};
            event.type = EventType_quit;
            events.push_back(event);
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

std::span<Event> InputDeviceWin32::getEvents()
{
    return events;
}

bool InputDeviceWin32::wasQuit()
{
    return quit;
}

std::unique_ptr<Window> InputDeviceWin32::createWindow(const WindowConfig& config)
{
    u32 width = config.width != 0 ? config.width : 800;
    u32 height = config.height != 0 ? config.height : 600;
    std::string titleStr = config.title.length() != 0 ? std::string{config.title} : "Hurdy Gurdy";

    std::unique_ptr<WindowWin32> win = std::make_unique<WindowWin32>(this);

    wchar_t wtitle[256]{};
    MultiByteToWideChar(
        CP_UTF8,
        0,
        titleStr.c_str(),
        -1,
        wtitle,
        static_cast<int>(sizeof(wtitle) / sizeof(wchar_t)) - 1);

    win->hwnd = CreateWindowExW(
        0,
        L"HurdyGurdyWindow",
        wtitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        static_cast<int>(width),
        static_cast<int>(height),
        nullptr, nullptr,
        hInstance,
        win.get());

    if (win->hwnd == nullptr)
    {
        setError("Could not create Win32 window");
        return nullptr;
    }

    windows.add(std::bit_cast<u64>(win->hwnd), win.get());

    RECT rect{};
    GetClientRect(win->hwnd, &rect);
    win->width = static_cast<u32>(rect.right - rect.left);
    win->height = static_cast<u32>(rect.bottom - rect.top);
    win->focused = (GetFocus() == win->hwnd);

    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = hInstance;
    createInfo.hwnd = win->hwnd;

    auto vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
        internal::getVkInstanceProcAddr("vkCreateWin32SurfaceKHR"));

    VkSurfaceKHR surface;
    VkResult result = vkCreateWin32SurfaceKHR(
        static_cast<VkInstance>(internal::getVkInstance()),
        &createInfo,
        nullptr,
        &surface);

    if (result != VK_SUCCESS)
    {
        setError("Could not create Vulkan surface");
        return nullptr;
    }

    win->swap = GpuSwapchain::create(surface, win->width, win->height, config.preferredPresentMode, config.imageUsage);

    ShowWindow(win->hwnd, SW_SHOW);
    UpdateWindow(win->hwnd);

    return win;
}

bool InputDeviceWin32::isButtonDown(Button button)
{
    return isKeyDown[button];
}

bool InputDeviceWin32::wasButtonPressed(Button button)
{
    return !wasKeyDown[button] && isKeyDown[button];
}

bool InputDeviceWin32::wasButtonReleased(Button button)
{
    return wasKeyDown[button] && !isKeyDown[button];
}

Vec2 InputDeviceWin32::mousePos()
{
    if (activeWindow != nullptr)
        return activeWindow->mouse;

    if (windows.count == 1)
    {
        Vec2 ret{};
        windows.forEach([&](u64, WindowWin32* wd)
        {
            ret = wd->mouse;
        });
        return ret;
    }
    return {};
}

Vec2 InputDeviceWin32::mouseDelta()
{
    if (activeWindow != nullptr)
        return mouseDeltaAccum / static_cast<f32>(activeWindow->height);

    if (windows.count == 1)
    {
        Vec2 ret = mouseDeltaAccum;
        windows.forEach([&](u64, WindowWin32* wd)
        {
            ret = ret / static_cast<f32>(wd->height);
        });
        return ret;
    }
    return mouseDeltaAccum;
}

Vec2 InputDeviceWin32::wheelDelta()
{
    return wheelDeltaAccum;
}

void InputDeviceWin32::setCursor(CursorType type)
{
    if (type < CursorType_count && cursors[type] != nullptr)
    {
        currentCursor = cursors[type];
        SetCursor(currentCursor);
    }
}

void InputDeviceWin32::showCursor(bool show)
{
    static bool visible = true;
    if (show == visible)
        return;
    ::ShowCursor(show ? TRUE : FALSE);
    visible = show;
}

WindowWin32::~WindowWin32()
{
    if (hwnd != nullptr)
    {
        static_cast<InputDeviceWin32*>(device)->windows.remove(std::bit_cast<u64>(hwnd));
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }
}

GpuSwapchain& WindowWin32::swapchain() const
{
    return swap;
}

void WindowWin32::setTitle(std::string_view title)
{
    char buf[256];
    u64 len = title.length() < sizeof(buf) - 1 ? title.length() : sizeof(buf) - 1;
    std::memcpy(buf, title.data(), len);
    buf[len] = '\0';

    wchar_t wbuf[256]{};
    MultiByteToWideChar(CP_UTF8, 0, buf, static_cast<int>(len), wbuf, 255);
    SetWindowTextW(hwnd, wbuf);
}

std::span<Event> WindowWin32::events() const
{
    return eventQueue;
}

bool WindowWin32::wasClosed() const
{
    return closed;
}

bool WindowWin32::isFocused() const
{
    return focused;
}

bool WindowWin32::wasFocusGained() const
{
    return focusGained;
}

bool WindowWin32::wasFocusLost() const
{
    return focusLost;
}

bool WindowWin32::wasResized() const
{
    return resized;
}

void WindowWin32::size(u32* outWidth, u32* outHeight) const
{
    if (outWidth != nullptr)
        *outWidth = width;
    if (outHeight != nullptr)
        *outHeight = height;
}

void WindowWin32::setFullscreen(bool set)
{
    if (fullscreen == set)
        return;
    fullscreen = set;

    if (set)
    {
        savedPlacement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd, &savedPlacement);

        DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
        style &= ~WS_OVERLAPPEDWINDOW;
        style |= WS_POPUP;
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);

        HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEXW mi{};
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(hmon, &mi);

        SetWindowPos(hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
    }
    else
    {
        DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
        style &= ~WS_POPUP;
        style |= WS_OVERLAPPEDWINDOW;
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);

        if (savedPlacement.length != 0)
            SetWindowPlacement(hwnd, &savedPlacement);

        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
    }

    RECT rect{};
    GetClientRect(hwnd, &rect);
    u32 w = static_cast<u32>(rect.right - rect.left);
    u32 h = static_cast<u32>(rect.bottom - rect.top);
    if (w > 0 && h > 0)
    {
        width = w;
        height = h;
        swap.resize(w, h);
    }
}

Vec2 WindowWin32::mousePos() const
{
    return mouse;
}

Vec2 WindowWin32::mouseDelta() const
{
    return static_cast<InputDeviceWin32*>(device)->mouseDeltaAccum / static_cast<f32>(height);
}

std::unique_ptr<AudioDeviceWasapi> AudioDeviceWasapi::create()
{
    std::unique_ptr<AudioDeviceWasapi> audio = std::make_unique<AudioDeviceWasapi>();

    // Initialize COM on the main thread (per-thread requirement)
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(&audio->deviceEnumerator));

    if (FAILED(hr) || audio->deviceEnumerator == nullptr)
    {
        setError("Could not create WASAPI device enumerator");
        audio->deviceEnumerator = nullptr;
        CoUninitialize();
        return nullptr;
    }

    hr = audio->deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audio->device);
    if (FAILED(hr) || audio->device == nullptr)
    {
        setError("Could not get default audio endpoint");
        audio->device = nullptr;
        audio->deviceEnumerator->Release();
        audio->deviceEnumerator = nullptr;
        CoUninitialize();
        return nullptr;
    }

    return audio;
}

AudioDeviceWasapi::~AudioDeviceWasapi()
{
    stopThread();
    releaseStream();

    if (device != nullptr)
    {
        device->Release();
        device = nullptr;
    }

    if (deviceEnumerator != nullptr)
    {
        deviceEnumerator->Release();
        deviceEnumerator = nullptr;
    }

    CoUninitialize();
}

void AudioDeviceWasapi::stopThread()
{
    if (audioThreadRunning)
    {
        audioThreadRunning = false;
        if (audioEvent != nullptr)
            SetEvent(audioEvent);
        if (audioThread != nullptr)
        {
            WaitForSingleObject(audioThread, 5000);
            CloseHandle(audioThread);
            audioThread = nullptr;
        }
    }
}

void AudioDeviceWasapi::releaseStream()
{
    if (renderClient != nullptr)
    {
        renderClient->Release();
        renderClient = nullptr;
    }

    if (audioClient != nullptr)
    {
        audioClient->Release();
        audioClient = nullptr;
    }

    if (audioEvent != nullptr)
    {
        CloseHandle(audioEvent);
        audioEvent = nullptr;
    }
}

DWORD WINAPI AudioDeviceWasapi::audioThreadProc(LPVOID param)
{
    AudioDeviceWasapi* audio = static_cast<AudioDeviceWasapi*>(param);

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    while (audio->audioThreadRunning)
    {
        DWORD waitResult = WaitForSingleObject(audio->audioEvent, 2000);
        if (waitResult != WAIT_OBJECT_0 || !audio->audioThreadRunning)
            continue;

        u32 padding = 0;
        if (FAILED(audio->audioClient->GetCurrentPadding(&padding)))
            continue;

        u32 availableFrames = audio->bufferFrameCount - padding;
        if (availableFrames == 0)
            continue;

        BYTE* data = nullptr;
        if (FAILED(audio->renderClient->GetBuffer(availableFrames, &data)))
            continue;

        std::memset(data, 0, static_cast<u64>(availableFrames) * sizeof(f32) * audio->callbackConfig.channels);

        if (audio->callback != nullptr)
        {
            u64 totalSamples = static_cast<u64>(availableFrames) * audio->callbackConfig.channels;
            audio->callback(std::span<f32>{reinterpret_cast<f32*>(data), totalSamples}, audio->callbackConfig);
        }

        audio->renderClient->ReleaseBuffer(availableFrames, 0);
    }

    CoUninitialize();
    return 0;
}

void AudioDeviceWasapi::setCallback(AudioCallback newCallback, const AudioConfig& preferredConfig)
{
    stopThread();

    AudioConfig config = preferredConfig;
    if (config.sampleRate == 0)
        config.sampleRate = 48000;
    if (config.channels == 0)
        config.channels = 2;

    // A client cannot be re-initialized, so release the old stream
    releaseStream();

    // Get the engine mix format, in shared mode it must be used directly
    WAVEFORMATEX* mixFormat = nullptr;
    HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audioClient));
    if (FAILED(hr) || audioClient == nullptr)
    {
        setError("Could not activate audio client");
        return;
    }

    hr = audioClient->GetMixFormat(&mixFormat);
    if (FAILED(hr) || mixFormat == nullptr)
    {
        setError("Could not get audio mix format");
        releaseStream();
        return;
    }

    // Verify the mix format is 32-bit float, shared mode requires it
    bool isFloat = false;
    if (mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        isFloat = true;
    }
    else if (mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        static const GUID floatSubtype = {
            0x00000003,
            0x0000,
            0x0010,
            {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
        };
        WAVEFORMATEXTENSIBLE* ext = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(mixFormat);
        isFloat = ext->SubFormat == floatSubtype;
    }

    if (!isFloat)
    {
        setError("Audio mix format is not 32-bit float");
        CoTaskMemFree(mixFormat);
        releaseStream();
        return;
    }

    // Update callback config to match what the system provides
    config.sampleRate = static_cast<u32>(mixFormat->nSamplesPerSec);
    config.channels = static_cast<u32>(mixFormat->nChannels);

    audioEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (audioEvent == nullptr)
    {
        setError("Could not create audio event");
        CoTaskMemFree(mixFormat);
        releaseStream();
        return;
    }

    hr = audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        0,
        0,
        mixFormat,
        nullptr);

    CoTaskMemFree(mixFormat);

    if (FAILED(hr))
    {
        setError("Could not initialize audio client");
        releaseStream();
        return;
    }

    audioClient->SetEventHandle(audioEvent);

    hr = audioClient->GetBufferSize(&bufferFrameCount);
    if (FAILED(hr))
    {
        setError("Could not get audio buffer size");
        releaseStream();
        return;
    }

    hr = audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&renderClient));
    if (FAILED(hr) || renderClient == nullptr)
    {
        setError("Could not get audio render client");
        renderClient = nullptr;
        releaseStream();
        return;
    }

    hr = audioClient->Start();
    if (FAILED(hr))
    {
        setError("Could not start audio client");
        releaseStream();
        return;
    }

    callback = std::move(newCallback);
    callbackConfig = config;

    audioThreadRunning = true;
    audioThread = CreateThread(nullptr, 0, audioThreadProc, this, 0, nullptr);
    if (audioThread == nullptr)
    {
        setError("Could not create audio thread");
        audioThreadRunning = false;
        audioClient->Stop();
        releaseStream();
        return;
    }
}

void AudioDeviceWasapi::unsetCallback()
{
    stopThread();

    if (audioClient != nullptr)
        audioClient->Stop();

    releaseStream();

    callback = {};
    callbackConfig = {};
}

} // namespace hg
