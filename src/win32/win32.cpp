#include "win32_audio.hpp"

#include "win32_window.hpp"

#include "internal.hpp"

#include "hg/error.hpp"
#include "hg/hash.hpp"
#include "hg/utility.hpp"

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
        case '0': return Button::b0;
        case '1': return Button::b1;
        case '2': return Button::b2;
        case '3': return Button::b3;
        case '4': return Button::b4;
        case '5': return Button::b5;
        case '6': return Button::b6;
        case '7': return Button::b7;
        case '8': return Button::b8;
        case '9': return Button::b9;
        case 'Q': return Button::q;
        case 'W': return Button::w;
        case 'E': return Button::e;
        case 'R': return Button::r;
        case 'T': return Button::t;
        case 'Y': return Button::y;
        case 'U': return Button::u;
        case 'I': return Button::i;
        case 'O': return Button::o;
        case 'P': return Button::p;
        case 'A': return Button::a;
        case 'S': return Button::s;
        case 'D': return Button::d;
        case 'F': return Button::f;
        case 'G': return Button::g;
        case 'H': return Button::h;
        case 'J': return Button::j;
        case 'K': return Button::k;
        case 'L': return Button::l;
        case 'Z': return Button::z;
        case 'X': return Button::x;
        case 'C': return Button::c;
        case 'V': return Button::v;
        case 'B': return Button::b;
        case 'N': return Button::n;
        case 'M': return Button::m;
        case VK_OEM_1: return Button::semicolon;
        case VK_OEM_7: return Button::apostrophe;
        case VK_OEM_COMMA: return Button::comma;
        case VK_OEM_PERIOD: return Button::period;
        case VK_OEM_3: return Button::grave;
        case VK_OEM_4: return Button::lbracket;
        case VK_OEM_6: return Button::rbracket;
        case VK_OEM_PLUS: return Button::equal;
        case VK_OEM_MINUS: return Button::minus;
        case VK_OEM_2: return Button::slash;
        case VK_OEM_5: return Button::backslash;
        case VK_UP: return Button::up;
        case VK_DOWN: return Button::down;
        case VK_LEFT: return Button::left;
        case VK_RIGHT: return Button::right;
        case VK_ESCAPE: return Button::escape;
        case VK_SPACE: return Button::space;
        case VK_RETURN: return Button::enter;
        case VK_BACK: return Button::backspace;
        case VK_DELETE: return Button::kdelete;
        case VK_INSERT: return Button::insert;
        case VK_TAB: return Button::tab;
        case VK_HOME: return Button::home;
        case VK_END: return Button::end;
        case VK_PRIOR: return Button::pageup;
        case VK_NEXT: return Button::pagedown;
        case VK_F1: return Button::f1;
        case VK_F2: return Button::f2;
        case VK_F3: return Button::f3;
        case VK_F4: return Button::f4;
        case VK_F5: return Button::f5;
        case VK_F6: return Button::f6;
        case VK_F7: return Button::f7;
        case VK_F8: return Button::f8;
        case VK_F9: return Button::f9;
        case VK_F10: return Button::f10;
        case VK_F11: return Button::f11;
        case VK_F12: return Button::f12;
        case VK_SNAPSHOT: return Button::printscreen;
        case VK_APPS: return Button::context;
        case VK_NUMPAD0: return Button::numpad0;
        case VK_NUMPAD1: return Button::numpad1;
        case VK_NUMPAD2: return Button::numpad2;
        case VK_NUMPAD3: return Button::numpad3;
        case VK_NUMPAD4: return Button::numpad4;
        case VK_NUMPAD5: return Button::numpad5;
        case VK_NUMPAD6: return Button::numpad6;
        case VK_NUMPAD7: return Button::numpad7;
        case VK_NUMPAD8: return Button::numpad8;
        case VK_NUMPAD9: return Button::numpad9;
        case VK_DECIMAL: return Button::numpaddecimal;
        case VK_DIVIDE: return Button::numpaddiv;
        case VK_MULTIPLY: return Button::numpadmul;
        case VK_SUBTRACT: return Button::numpadminus;
        case VK_ADD: return Button::numpadplus;
        case VK_SEPARATOR: return Button::numpadenter;
        case VK_LSHIFT: return Button::lshift;
        case VK_RSHIFT: return Button::rshift;
        case VK_LCONTROL: return Button::lctrl;
        case VK_RCONTROL: return Button::rctrl;
        case VK_LMENU: return Button::lalt;
        case VK_RMENU: return Button::ralt;
        case VK_LWIN: return Button::lsuper;
        case VK_RWIN: return Button::rsuper;
        case VK_CAPITAL: return Button::capslock;
        case VK_NUMLOCK: return Button::numlock;
        case VK_SCROLL: return Button::scrolllock;
        case VK_PAUSE: return Button::pause;
    }
    return Button::none;
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

    device->cursors[static_cast<u32>(CursorType::arrow)] = LoadCursorW(nullptr, IDC_ARROW);
    device->cursors[static_cast<u32>(CursorType::textInput)] = LoadCursorW(nullptr, IDC_IBEAM);
    device->cursors[static_cast<u32>(CursorType::resizeAll)] = LoadCursorW(nullptr, IDC_SIZEALL);
    device->cursors[static_cast<u32>(CursorType::resizeNS)] = LoadCursorW(nullptr, IDC_SIZENS);
    device->cursors[static_cast<u32>(CursorType::resizeEW)] = LoadCursorW(nullptr, IDC_SIZEWE);
    device->cursors[static_cast<u32>(CursorType::resizeNESW)] = LoadCursorW(nullptr, IDC_SIZENESW);
    device->cursors[static_cast<u32>(CursorType::resizeNWSE)] = LoadCursorW(nullptr, IDC_SIZENWSE);
    device->cursors[static_cast<u32>(CursorType::hand)] = LoadCursorW(nullptr, IDC_HAND);
    device->cursors[static_cast<u32>(CursorType::wait)] = LoadCursorW(nullptr, IDC_WAIT);
    device->cursors[static_cast<u32>(CursorType::progress)] = LoadCursorW(nullptr, IDC_APPSTARTING);
    device->cursors[static_cast<u32>(CursorType::notAllowed)] = LoadCursorW(nullptr, IDC_NO);

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
        info.sizeW = assertCast<u32>(mi.rcMonitor.right - mi.rcMonitor.left);
        info.sizeH = assertCast<u32>(mi.rcMonitor.bottom - mi.rcMonitor.top);
        info.workPosX = mi.rcWork.left;
        info.workPosY = mi.rcWork.top;
        info.workSizeW = assertCast<u32>(mi.rcWork.right - mi.rcWork.left);
        info.workSizeH = assertCast<u32>(mi.rcWork.bottom - mi.rcWork.top);
        info.dpiScale = 1.0f;

        if (data->getDpiForMonitor != nullptr)
        {
            u32 dpiX = 96;
            u32 dpiY = 96;
            if (SUCCEEDED(data->getDpiForMonitor(hmon, 0, &dpiX, &dpiY)))
                info.dpiScale = assertCast<f32>(dpiX) / 96.0f;
        }

        data->displays->push_back(info);
        return TRUE;
    };

    EnumDisplayMonitors(nullptr, nullptr, monitorEnumProc, reinterpret_cast<LPARAM>(&enumData));

    if (device->displays.empty())
    {
        DisplayInfo info{};
        info.sizeW = assertCast<u32>(GetSystemMetrics(SM_CXSCREEN));
        info.sizeH = assertCast<u32>(GetSystemMetrics(SM_CYSCREEN));
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

std::span<std::string_view> InputDeviceWin32::getPlatformGraphicsExtensions()
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
                event.type = EventType::windowClosed;
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

                if (dev->windows.size() == 0)
                {
                    dev->quit = true;

                    Event event{};
                    event.type = EventType::quit;
                    dev->events.push_back(event);
                }
            }
            return 0;
        }
        case WM_SIZE:
        {
            u32 width = assertCast<u32>(LOWORD(lParam));
            u32 height = assertCast<u32>(HIWORD(lParam));
            if (width == 0 || height == 0 || dev == nullptr)
                return 0;

            w->swap->resize(width, height);
            w->winWidth = width;
            w->winHeight = height;
            w->resized = true;

            Event event{};
            event.type = EventType::windowResized;
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
                event.type = EventType::windowFocused;
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
                event.type = EventType::windowUnfocused;
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

            Vec2 newPos{assertCast<f32>(GET_X_LPARAM(lParam)), assertCast<f32>(GET_Y_LPARAM(lParam))};
            Vec2 delta = newPos - w->mouse;
            w->mouse = newPos;
            dev->mouseDeltaAccum += delta;

            POINT globalPt{};
            GetCursorPos(&globalPt);

            Event event{};
            event.type = EventType::mouseMoved;
            event.mouse.delta = delta;
            event.mouse.pos = newPos;
            event.mouse.globalPos = Vec2{assertCast<f32>(globalPt.x), assertCast<f32>(globalPt.y)};
            dev->events.push_back(event);
            w->eventQueue.push_back(event);
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            if (dev == nullptr)
                return 0;

            f32 delta = assertCast<f32>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            dev->wheelDeltaAccum.y() += delta;

            Event event{};
            event.type = EventType::wheelMoved;
            event.wheel.delta = Vec2{0.0f, delta};
            dev->events.push_back(event);
            w->eventQueue.push_back(event);
            return 0;
        }
        case WM_MOUSEHWHEEL:
        {
            if (dev == nullptr)
                return 0;

            f32 delta = assertCast<f32>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            dev->wheelDeltaAccum.x() += delta;

            Event event{};
            event.type = EventType::wheelMoved;
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

            Button button = Button::none;
            switch (msg)
            {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP: button = Button::mouse1; break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP: button = Button::mouse2; break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP: button = Button::mouse3; break;
                case WM_XBUTTONDOWN:
                case WM_XBUTTONUP:
                {
                    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                        button = Button::mouse4;
                    else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
                        button = Button::mouse5;
                    break;
                }
            }

            if (button != Button::none)
            {
                dev->isKeyDown[static_cast<u32>(button)] = pressed;

                Event event{};
                event.type = pressed ? EventType::keyPress : EventType::keyRelease;
                event.button = button;
                dev->events.push_back(event);
                w->eventQueue.push_back(event);
            }
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (dev != nullptr)
            {
                Button button = win32VirtKeyToButton(assertCast<u32>(wParam));
                if (button != Button::none)
                {
                    dev->isKeyDown[static_cast<u32>(button)] = true;

                    Event event{};
                    event.type = EventType::keyPress;
                    event.button = button;
                    dev->events.push_back(event);
                    w->eventQueue.push_back(event);
                }
            }

            // DefWindowProc turns system keys like Alt+F4 into WM_SYSCOMMAND
            // (SC_CLOSE), so they must be passed through
            if (msg == WM_SYSKEYDOWN)
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            return 0;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            if (dev != nullptr)
            {
                Button button = win32VirtKeyToButton(assertCast<u32>(wParam));
                if (button != Button::none)
                {
                    dev->isKeyDown[static_cast<u32>(button)] = false;

                    Event event{};
                    event.type = EventType::keyRelease;
                    event.button = button;
                    dev->events.push_back(event);
                    w->eventQueue.push_back(event);
                }
            }

            if (msg == WM_SYSKEYUP)
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            return 0;
        }
        case WM_CHAR:
        {
            if (dev == nullptr)
                return 0;

            wchar_t ch = assertCast<wchar_t>(wParam);
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
                        event.type = EventType::text;
                        std::memset(event.text, 0, sizeof(event.text));
                        std::memcpy(event.text, utf8, assertCast<u64>(utf8Len) < sizeof(event.text) ? assertCast<u64>(utf8Len) : sizeof(event.text));
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
            event.type = EventType::quit;
            events.push_back(event);
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

std::span<const Event> InputDeviceWin32::getEvents()
{
    return events;
}

bool InputDeviceWin32::wasQuit()
{
    return quit;
}

std::unique_ptr<Window> InputDeviceWin32::createWindow(Gpu* gpu, const WindowConfig& config)
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
        assertCast<int>(sizeof(wtitle) / sizeof(wchar_t)) - 1);

    win->hwnd = CreateWindowExW(
        0,
        L"HurdyGurdyWindow",
        wtitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        assertCast<int>(width),
        assertCast<int>(height),
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
    win->winWidth = assertCast<u32>(rect.right - rect.left);
    win->winHeight = assertCast<u32>(rect.bottom - rect.top);
    win->focused = (GetFocus() == win->hwnd);

    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = hInstance;
    createInfo.hwnd = win->hwnd;

    auto vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
        internal::getVkInstanceProcAddr(gpu, "vkCreateWin32SurfaceKHR"));

    VkSurfaceKHR surface;
    VkResult result = vkCreateWin32SurfaceKHR(
        static_cast<VkInstance>(internal::getVkInstance(gpu)),
        &createInfo,
        nullptr,
        &surface);

    if (result != VK_SUCCESS)
    {
        setError("Could not create Vulkan surface");
        return nullptr;
    }

    win->swap = gpu->createSwapchain(surface, win->winWidth, win->winHeight, config.preferredPresentMode, config.imageUsage);

    ShowWindow(win->hwnd, SW_SHOW);
    UpdateWindow(win->hwnd);

    return win;
}

bool InputDeviceWin32::isButtonDown(Button button)
{
    return isKeyDown[static_cast<u32>(button)];
}

bool InputDeviceWin32::wasButtonPressed(Button button)
{
    return !wasKeyDown[static_cast<u32>(button)] && isKeyDown[static_cast<u32>(button)];
}

bool InputDeviceWin32::wasButtonReleased(Button button)
{
    return wasKeyDown[static_cast<u32>(button)] && !isKeyDown[static_cast<u32>(button)];
}

Vec2 InputDeviceWin32::mousePos()
{
    if (activeWindow != nullptr)
        return activeWindow->mouse;

    if (windows.size() == 1)
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
        return mouseDeltaAccum / assertCast<f32>(activeWindow->winHeight);

    if (windows.size() == 1)
    {
        Vec2 ret = mouseDeltaAccum;
        windows.forEach([&](u64, WindowWin32* wd)
        {
            ret = ret / assertCast<f32>(wd->winHeight);
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
    if (type < CursorType::count && cursors[static_cast<u32>(type)] != nullptr)
    {
        currentCursor = cursors[static_cast<u32>(type)];
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

GpuSwapchain* WindowWin32::swapchain() const
{
    return swap.get();
}

void WindowWin32::setTitle(std::string_view title)
{
    char buf[256];
    u64 len = title.length() < sizeof(buf) - 1 ? title.length() : sizeof(buf) - 1;
    std::memcpy(buf, title.data(), len);
    buf[len] = '\0';

    wchar_t wbuf[256]{};
    MultiByteToWideChar(CP_UTF8, 0, buf, assertCast<int>(len), wbuf, 255);
    SetWindowTextW(hwnd, wbuf);
}

std::span<const Event> WindowWin32::getEvents() const
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

u32 WindowWin32::width() const
{
    return winWidth;
}

u32 WindowWin32::height() const
{
    return winHeight;
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

        DWORD style = assertCast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
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
        DWORD style = assertCast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
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
    u32 w = assertCast<u32>(rect.right - rect.left);
    u32 h = assertCast<u32>(rect.bottom - rect.top);
    if (w > 0 && h > 0)
    {
        winWidth = w;
        winHeight = h;
        swap->resize(w, h);
    }
}

Vec2 WindowWin32::mousePos() const
{
    return mouse;
}

Vec2 WindowWin32::mouseDelta() const
{
    return static_cast<InputDeviceWin32*>(device)->mouseDeltaAccum;
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

        std::memset(data, 0, assertCast<u64>(availableFrames) * sizeof(f32) * audio->callbackConfig.channels);

        if (audio->callback != nullptr)
        {
            u64 totalSamples = assertCast<u64>(availableFrames) * audio->callbackConfig.channels;
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
    config.sampleRate = assertCast<u32>(mixFormat->nSamplesPerSec);
    config.channels = assertCast<u32>(mixFormat->nChannels);

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
