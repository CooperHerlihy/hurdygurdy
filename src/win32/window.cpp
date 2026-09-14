#include "win32_platform.hpp"

#include "internal.hpp"
#include "hg/error.hpp"
#include "hg/array.hpp"
#include "hg/map.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <xinput.h>
#include <mmdeviceapi.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#ifndef XINPUT_GAMEPAD_GUIDE
#define XINPUT_GAMEPAD_GUIDE 0x0400
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>

namespace hg::win32 {

static constexpr u32 maxGamepads = 4;

struct WindowData {
    GpuSwapchain swap{};

    HWND hwnd = nullptr;

    Array<Event> events{};
    Vec2 mouse{};
    u32 width = 0;
    u32 height = 0;
    bool wasClosed = false;
    bool isFocused = false;
    bool wasFocusGained = false;
    bool wasFocusLost = false;
    bool wasResized = false;

    WindowData() noexcept = default;
    ~WindowData() noexcept;

    WindowData(WindowData&& other) noexcept;
    WindowData& operator=(WindowData&& other) noexcept;

    WindowData(const WindowData&) = delete;
    WindowData& operator=(const WindowData&) = delete;
};

struct GamepadState {
    bool connected = false;
    bool isButtonDown[GamepadButton_count]{};
    bool wasButtonDown[GamepadButton_count]{};
    i32 axes[6]{}; // lx, ly, rx, ry, lt, rt
};

struct WindowState {
    HINSTANCE hInstance = nullptr;
    ATOM windowClass = 0;

    HCURSOR cursors[CursorType_count]{};
    HCURSOR currentCursor = nullptr;

    Array<DisplayInfo> displays{};

    Array<Event> events{};
    bool wasQuit = false;
    bool isKeyDown[Button_count]{};
    bool wasKeyDown[Button_count]{};
    Vec2 mouseDelta{};
    Vec2 wheelDelta{};

    Map<u64, WindowData*> windows{};
    WindowData* activeWindow = nullptr;

    Array<char> clipboard{};

    GamepadState gamepads[maxGamepads]{};
};

static WindowState windowState{};

static u64 hwndToKey(HWND hwnd)
{
    return std::bit_cast<u64>(hwnd);
}

static LRESULT CALLBACK win32WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool windowInit(HINSTANCE hInst)
{
    windowState = WindowState{};
    windowState.hInstance = hInst;

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = win32WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.lpszClassName = "HurdyGurdyWindow";

    windowState.windowClass = RegisterClassExA(&wc);
    if (windowState.windowClass == 0)
    {
        setError("Could not register window class");
        return false;
    }

    // Create cursors
    windowState.cursors[CursorType_arrow] = LoadCursorA(nullptr, IDC_ARROW);
    windowState.cursors[CursorType_textInput] = LoadCursorA(nullptr, IDC_IBEAM);
    windowState.cursors[CursorType_resizeAll] = LoadCursorA(nullptr, IDC_SIZEALL);
    windowState.cursors[CursorType_resizeNS] = LoadCursorA(nullptr, IDC_SIZENS);
    windowState.cursors[CursorType_resizeEW] = LoadCursorA(nullptr, IDC_SIZEWE);
    windowState.cursors[CursorType_resizeNESW] = LoadCursorA(nullptr, IDC_SIZENESW);
    windowState.cursors[CursorType_resizeNWSE] = LoadCursorA(nullptr, IDC_SIZENWSE);
    windowState.cursors[CursorType_hand] = LoadCursorA(nullptr, IDC_HAND);
    windowState.cursors[CursorType_wait] = LoadCursorA(nullptr, IDC_WAIT);
    windowState.cursors[CursorType_progress] = LoadCursorA(nullptr, IDC_APPSTARTING);
    windowState.cursors[CursorType_notAllowed] = LoadCursorA(nullptr, IDC_NO);

    // Enumerate displays
    struct EnumData {
        Array<DisplayInfo>* displays;
    } enumData{&windowState.displays};

    MONITORENUMPROC monitorEnumProc = [](HMONITOR hmon, HDC, LPRECT, LPARAM lp) -> BOOL
    {
        EnumData* data = reinterpret_cast<EnumData*>(lp);

        MONITORINFOEXA mi{};
        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfoA(hmon, &mi))
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

        // DPI scale
       HDC hdc = GetDC(nullptr);
        i32 dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        info.dpiScale = static_cast<f32>(dpiX) / 96.0f;

        data->displays->push(info);
        return TRUE;
    };

    EnumDisplayMonitors(nullptr, nullptr, monitorEnumProc, reinterpret_cast<LPARAM>(&enumData));

    if (windowState.displays.count == 0)
    {
        DisplayInfo info{};
        info.sizeW = static_cast<u32>(GetSystemMetrics(SM_CXSCREEN));
        info.sizeH = static_cast<u32>(GetSystemMetrics(SM_CYSCREEN));
        info.workSizeW = info.sizeW;
        info.workSizeH = info.sizeH;
        info.dpiScale = 1.0f;
        windowState.displays.push(info);
    }

    return true;
}

void windowDeinit()
{
    windowState.windows.forEach([](u64, WindowData* window)
    {
        *window = {};
    });

    windowState.currentCursor = nullptr;

    UnregisterClassA("HurdyGurdyWindow", windowState.hInstance);
}

WindowData::~WindowData() noexcept
{
    if (hwnd != nullptr)
    {
        windowState.windows.remove(hwndToKey(hwnd));
        DestroyWindow(hwnd);
    }
}

WindowData::WindowData(WindowData&& other) noexcept
    : swap{std::move(other.swap)}
    , hwnd{std::exchange(other.hwnd, nullptr)}
    , events{std::move(other.events)}
    , mouse{std::exchange(other.mouse, {})}
    , width{std::exchange(other.width, 0)}
    , height{std::exchange(other.height, 0)}
    , wasClosed{std::exchange(other.wasClosed, false)}
    , isFocused{std::exchange(other.isFocused, false)}
    , wasFocusGained{std::exchange(other.wasFocusGained, false)}
    , wasFocusLost{std::exchange(other.wasFocusLost, false)}
    , wasResized{std::exchange(other.wasResized, false)}
{}

WindowData& WindowData::operator=(WindowData&& other) noexcept
{
    if (this != &other)
    {
        this->~WindowData();
        new (this) WindowData{std::move(other)};
    }
    return *this;
}

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

static Button win32MouseButtonToButton(u32 btn)
{
    switch (btn)
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP: return Button_mouse1;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP: return Button_mouse2;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP: return Button_mouse3;
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        {
            u32 xbtn = GET_XBUTTON_WPARAM(btn);
            if (xbtn == XBUTTON1) return Button_mouse4;
            if (xbtn == XBUTTON2) return Button_mouse5;
            return Button_none;
        }
    }
    return Button_none;
}

static void updateGamepads()
{
    for (u32 i = 0; i < maxGamepads; i++)
    {
        XINPUT_STATE state{};
        if (XInputGetState(i, &state) == ERROR_SUCCESS)
        {
            GamepadState& gp = windowState.gamepads[i];
            bool wasConnected = gp.connected;
            gp.connected = true;

            u16 buttons = state.Gamepad.wButtons;

            bool aDown = (buttons & XINPUT_GAMEPAD_A) != 0;
            bool bDown = (buttons & XINPUT_GAMEPAD_B) != 0;
            bool xDown = (buttons & XINPUT_GAMEPAD_X) != 0;
            bool yDown = (buttons & XINPUT_GAMEPAD_Y) != 0;
            bool guideDown = (buttons & XINPUT_GAMEPAD_GUIDE) != 0;

            gp.isButtonDown[GamepadButton_south] = aDown;
            gp.isButtonDown[GamepadButton_east] = bDown;
            gp.isButtonDown[GamepadButton_west] = xDown;
            gp.isButtonDown[GamepadButton_north] = yDown;
            gp.isButtonDown[GamepadButton_guide] = guideDown;

            gp.isButtonDown[GamepadButton_back] = (buttons & XINPUT_GAMEPAD_BACK) != 0;
            gp.isButtonDown[GamepadButton_start] = (buttons & XINPUT_GAMEPAD_START) != 0;
            gp.isButtonDown[GamepadButton_leftShoulder] = (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
            gp.isButtonDown[GamepadButton_rightShoulder] = (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
            gp.isButtonDown[GamepadButton_leftStick] = (buttons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
            gp.isButtonDown[GamepadButton_rightStick] = (buttons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
            gp.isButtonDown[GamepadButton_dpadUp] = (buttons & XINPUT_GAMEPAD_DPAD_UP) != 0;
            gp.isButtonDown[GamepadButton_dpadDown] = (buttons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
            gp.isButtonDown[GamepadButton_dpadLeft] = (buttons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
            gp.isButtonDown[GamepadButton_dpadRight] = (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;

            gp.axes[0] = state.Gamepad.sThumbLX;
            gp.axes[1] = state.Gamepad.sThumbLY;
            gp.axes[2] = state.Gamepad.sThumbRX;
            gp.axes[3] = state.Gamepad.sThumbRY;
            gp.axes[4] = state.Gamepad.bLeftTrigger;
            gp.axes[5] = state.Gamepad.bRightTrigger;

            if (!wasConnected)
            {
                Event event{};
                event.type = EventType_gamepadConnected;
                event.gamepad.idx = i;
                windowState.events.push(event);
            }
        }
        else
        {
            if (windowState.gamepads[i].connected)
            {
                Event event{};
                event.type = EventType_gamepadDisconnected;
                event.gamepad.idx = i;
                windowState.events.push(event);
            }
            windowState.gamepads[i] = {};
        }
    }
}

static void processGamepadEvents()
{
    for (u32 i = 0; i < maxGamepads; i++)
    {
        GamepadState& gp = windowState.gamepads[i];
        if (!gp.connected)
            continue;

        memcpy(gp.wasButtonDown, gp.isButtonDown, sizeof(gp.isButtonDown));
    }

    updateGamepads();

    for (u32 i = 0; i < maxGamepads; i++)
    {
        GamepadState& gp = windowState.gamepads[i];
        if (!gp.connected)
            continue;

        for (u32 b = 0; b < GamepadButton_count; b++)
        {
            if (gp.isButtonDown[b] && !gp.wasButtonDown[b])
            {
                Event event{};
                event.type = EventType_gamepadPress;
                event.gamepad.idx = i;
                event.gamepad.button = static_cast<GamepadButton>(b);
                windowState.events.push(event);
            }
            if (!gp.isButtonDown[b] && gp.wasButtonDown[b])
            {
                Event event{};
                event.type = EventType_gamepadRelease;
                event.gamepad.idx = i;
                event.gamepad.button = static_cast<GamepadButton>(b);
                windowState.events.push(event);
            }
        }

        // Axes
        constexpr f32 deadzone = 7849.0f / 32767.0f;
        f32 lx = static_cast<f32>(gp.axes[0]) / 32767.0f;
        f32 ly = static_cast<f32>(gp.axes[1]) / 32767.0f;
        f32 rx = static_cast<f32>(gp.axes[2]) / 32767.0f;
        f32 ry = static_cast<f32>(gp.axes[3]) / 32767.0f;

        if (fabsf(lx) < deadzone) lx = 0.0f;
        if (fabsf(ly) < deadzone) ly = 0.0f;
        if (fabsf(rx) < deadzone) rx = 0.0f;
        if (fabsf(ry) < deadzone) ry = 0.0f;

        {
            Event event{};
            event.type = EventType_gamepadLeftStick;
            event.gamepad.idx = i;
            event.gamepad.stick = Vec2{lx, ly};
            windowState.events.push(event);
        }
        {
            Event event{};
            event.type = EventType_gamepadRightStick;
            event.gamepad.idx = i;
            event.gamepad.stick = Vec2{rx, ry};
            windowState.events.push(event);
        }
        {
            Event event{};
            event.type = EventType_gamepadLeftTrigger;
            event.gamepad.idx = i;
            event.gamepad.trigger = static_cast<f32>(gp.axes[4]) / 255.0f;
            windowState.events.push(event);
        }
        {
            Event event{};
            event.type = EventType_gamepadRightTrigger;
            event.gamepad.idx = i;
            event.gamepad.trigger = static_cast<f32>(gp.axes[5]) / 255.0f;
            windowState.events.push(event);
        }
    }
}

static WindowData* getWindow(HWND hwnd)
{
    WindowData** data = windowState.windows.get(hwndToKey(hwnd));
    if (data == nullptr)
        return nullptr;
    return *data;
}

static LRESULT CALLBACK win32WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CLOSE:
        {
            WindowData* w = getWindow(hwnd);
            if (w != nullptr)
            {
                w->wasClosed = true;
                Event event{};
                event.type = EventType_windowClosed;
                event.window.window = w;
                windowState.events.push(event);
                w->events.push(event);
            }
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_DESTROY:
        {
            if (windowState.windows.count == 0)
            {
                windowState.wasQuit = true;
                Event event{};
                event.type = EventType_quit;
                windowState.events.push(event);
            }
            return 0;
        }
        case WM_SIZE:
        {
            u32 width = static_cast<u32>(LOWORD(lParam));
            u32 height = static_cast<u32>(HIWORD(lParam));
            if (width == 0 || height == 0)
                return 0;

            WindowData* w = getWindow(hwnd);
            if (w != nullptr)
            {
                w->swap.resize(width, height);
                w->width = width;
                w->height = height;
                w->wasResized = true;

                Event event{};
                event.type = EventType_windowResized;
                event.window.window = w;
                event.window.width = width;
                event.window.height = height;
                windowState.events.push(event);
                w->events.push(event);
            }
            return 0;
        }
        case WM_SETFOCUS:
        {
            WindowData* w = getWindow(hwnd);
            if (w != nullptr)
            {
                w->isFocused = true;
                w->wasFocusGained = true;
                windowState.activeWindow = w;

                Event event{};
                event.type = EventType_windowFocused;
                windowState.events.push(event);
                w->events.push(event);
            }
            return 0;
        }
        case WM_KILLFOCUS:
        {
            WindowData* w = getWindow(hwnd);
            if (w != nullptr)
            {
                w->isFocused = false;
                w->wasFocusLost = true;
                if (windowState.activeWindow == w)
                    windowState.activeWindow = nullptr;

                Event event{};
                event.type = EventType_windowUnfocused;
                windowState.events.push(event);
                w->events.push(event);
            }
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            f32 mx = static_cast<f32>(GET_X_LPARAM(lParam));
            f32 my = static_cast<f32>(GET_Y_LPARAM(lParam));

            WindowData* w = getWindow(hwnd);
            if (w != nullptr)
            {
                Vec2 newPos{mx, my};
                Vec2 delta = newPos - w->mouse;
                w->mouse = newPos;
                windowState.mouseDelta += delta;

                Event event{};
                event.type = EventType_mouseMoved;
                event.mouse.delta = delta;
                event.mouse.pos = newPos;
                event.mouse.globalPos = newPos;
                windowState.events.push(event);
            }
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            f32 delta = static_cast<f32>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;

            windowState.wheelDelta.y += delta;

            Event event{};
            event.type = EventType_wheelMoved;
            event.wheel.delta = Vec2{0.0f, delta};
            windowState.events.push(event);
            return 0;
        }
        case WM_MOUSEHWHEEL:
        {
            f32 delta = static_cast<f32>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;

            windowState.wheelDelta.x += delta;

            Event event{};
            event.type = EventType_wheelMoved;
            event.wheel.delta = Vec2{delta, 0.0f};
            windowState.events.push(event);
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        {
            Button button = win32MouseButtonToButton(msg);
            if (button != Button_none)
            {
                windowState.isKeyDown[button] = true;
                Event event{};
                event.type = EventType_keyPress;
                event.button = button;
                windowState.events.push(event);
            }
            return 0;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            Button button = win32MouseButtonToButton(msg);
            if (button != Button_none)
            {
                windowState.isKeyDown[button] = false;
                Event event{};
                event.type = EventType_keyRelease;
                event.button = button;
                windowState.events.push(event);
            }
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            u32 vk = static_cast<u32>(wParam);
            Button button = win32VirtKeyToButton(vk);
            if (button != Button_none)
            {
                windowState.isKeyDown[button] = true;
                Event event{};
                event.type = EventType_keyPress;
                event.button = button;
                windowState.events.push(event);
            }

            // Text input
            UINT scancode = (lParam >> 16) & 0xFF;
            bool extended = (lParam >> 24) & 1;
            if (extended)
                scancode |= 0xE000;

            BYTE keyboardState[256]{};
            GetKeyboardState(keyboardState);

            wchar_t unicodeBuf[8]{};
            int len = ToUnicode(vk, scancode, keyboardState, unicodeBuf, 8, 0);
            if (len > 0)
            {
                char utf8Buf[32]{};
                int utf8Len = WideCharToMultiByte(CP_UTF8, 0, unicodeBuf, len, utf8Buf, sizeof(utf8Buf) - 1, nullptr, nullptr);
                if (utf8Len > 0)
                {
                    Event event{};
                    event.type = EventType_text;
                    memset(event.text, 0, sizeof(event.text));
                    memcpy(event.text, utf8Buf, static_cast<u64>(utf8Len) < sizeof(event.text) - 1 ? static_cast<u64>(utf8Len) : sizeof(event.text) - 1);
                    windowState.events.push(event);
                }
            }
            return 0;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            u32 vk = static_cast<u32>(wParam);
            Button button = win32VirtKeyToButton(vk);
            if (button != Button_none)
            {
                windowState.isKeyDown[button] = false;
                Event event{};
                event.type = EventType_keyRelease;
                event.button = button;
                windowState.events.push(event);
            }
            return 0;
        }
        case WM_CHAR:
        {
            // WM_CHAR handles text input for non-dead keys
            return 0;
        }
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

Span<DisplayInfo> displayInfo()
{
    return windowState.displays;
}

void setCursor(CursorType type)
{
    if (type < CursorType_count && windowState.cursors[type] != nullptr)
    {
        windowState.currentCursor = windowState.cursors[type];
        SetCursor(windowState.currentCursor);
    }
}

void showCursor(bool show)
{
    ::ShowCursor(show ? TRUE : FALSE);
}

void processEvents()
{
    windowState.events.reset();

    memcpy(windowState.wasKeyDown, windowState.isKeyDown, sizeof(windowState.isKeyDown));
    windowState.mouseDelta = {};
    windowState.wheelDelta = {};

    windowState.windows.forEach([](u64, WindowData* data)
    {
        data->events.reset();
        data->wasFocusGained = false;
        data->wasFocusLost = false;
        data->wasResized = false;
    });

    MSG msg{};
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            windowState.wasQuit = true;
            Event event{};
            event.type = EventType_quit;
            windowState.events.push(event);
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    processGamepadEvents();
}

Span<Event> getEvents()
{
    return windowState.events;
}

bool wasQuit()
{
    return windowState.wasQuit;
}

bool isButtonDown(Button key)
{
    return windowState.isKeyDown[key];
}

bool wasButtonPressed(Button key)
{
    return !windowState.wasKeyDown[key] && windowState.isKeyDown[key];
}

bool wasButtonReleased(Button key)
{
    return windowState.wasKeyDown[key] && !windowState.isKeyDown[key];
}

Vec2 mousePos()
{
    if (windowState.activeWindow != nullptr)
        return windowState.activeWindow->mouse;

    if (windowState.windows.count == 1)
    {
        Vec2 ret{};
        windowState.windows.forEach([&](u64, WindowData* wd)
        {
            ret = wd->mouse;
        });
        return ret;
    }
    return {};
}

Vec2 mouseDelta()
{
    if (windowState.activeWindow != nullptr)
        return windowState.mouseDelta / static_cast<f32>(windowState.activeWindow->height);

    if (windowState.windows.count == 1)
    {
        Vec2 ret = windowState.mouseDelta;
        windowState.windows.forEach([&](u64, WindowData* wd)
        {
            ret = ret / static_cast<f32>(wd->height);
        });
        return ret;
    }
    return windowState.mouseDelta;
}

Vec2 wheelDelta()
{
    return windowState.wheelDelta;
}

u32 connectedGamepadCount()
{
    u32 count = 0;
    for (u32 i = 0; i < maxGamepads; i++)
    {
        if (windowState.gamepads[i].connected)
            count++;
    }
    return count;
}

bool isGamepadConnected(u32 gamepad)
{
    return gamepad < maxGamepads && windowState.gamepads[gamepad].connected;
}

bool isGamepadButtonDown(u32 gamepad, Button key)
{
    if (gamepad < maxGamepads)
    {
        GamepadButton gb = static_cast<GamepadButton>(static_cast<u32>(key));
        if (gb < GamepadButton_count)
            return windowState.gamepads[gamepad].isButtonDown[gb];
    }
    return false;
}

bool wasGamepadButtonPressed(u32 gamepad, GamepadButton key)
{
    if (gamepad < maxGamepads)
        return !windowState.gamepads[gamepad].wasButtonDown[key] && windowState.gamepads[gamepad].isButtonDown[key];
    return false;
}

bool wasGamepadButtonReleased(u32 gamepad, GamepadButton key)
{
    if (gamepad < maxGamepads)
        return windowState.gamepads[gamepad].wasButtonDown[key] && !windowState.gamepads[gamepad].isButtonDown[key];
    return false;
}

Vec2 gamepadLeftStick(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad].connected)
    {
        constexpr f32 deadzone = 0.15f;
        f32 x = static_cast<f32>(windowState.gamepads[gamepad].axes[0]) / 32767.0f;
        f32 y = static_cast<f32>(windowState.gamepads[gamepad].axes[1]) / 32767.0f;
        if (fabsf(x) < deadzone) x = 0.0f;
        if (fabsf(y) < deadzone) y = 0.0f;
        return Vec2{x, y};
    }
    return Vec2{0};
}

Vec2 gamepadRightStick(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad].connected)
    {
        constexpr f32 deadzone = 0.15f;
        f32 x = static_cast<f32>(windowState.gamepads[gamepad].axes[2]) / 32767.0f;
        f32 y = static_cast<f32>(windowState.gamepads[gamepad].axes[3]) / 32767.0f;
        if (fabsf(x) < deadzone) x = 0.0f;
        if (fabsf(y) < deadzone) y = 0.0f;
        return Vec2{x, y};
    }
    return Vec2{0};
}

f32 gamepadLeftTrigger(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad].connected)
        return static_cast<f32>(windowState.gamepads[gamepad].axes[4]) / 255.0f;
    return 0;
}

f32 gamepadRightTrigger(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad].connected)
        return static_cast<f32>(windowState.gamepads[gamepad].axes[5]) / 255.0f;
    return 0;
}

Window windowCreate(const WindowConfig& config)
{
    Window window{};
    window.data = new (heapAlloc(sizeof(WindowData), alignof(WindowData))) WindowData{};

    WindowData* wd = static_cast<WindowData*>(window.data);

    HWND hwnd = CreateWindowExA(
        0,
        "HurdyGurdyWindow",
        "Hurdy Gurdy",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr, nullptr,
        windowState.hInstance,
        nullptr);

    if (hwnd == nullptr)
    {
        wd->~WindowData();
        heapFree(wd, 1);
        setError("Could not create Win32 window");
        return Window{};
    }

    wd->hwnd = hwnd;
    windowState.windows.add(hwndToKey(hwnd), wd);

    RECT rect{};
    GetClientRect(hwnd, &rect);
    wd->width = static_cast<u32>(rect.right - rect.left);
    wd->height = static_cast<u32>(rect.bottom - rect.top);
    wd->isFocused = (GetFocus() == hwnd);

    // Create Vulkan surface
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = windowState.hInstance;
    createInfo.hwnd = hwnd;

    auto vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
        internal::getVulkanInstanceProcAddr("vkCreateWin32SurfaceKHR"));

    VkSurfaceKHR surface;
    VkResult result = vkCreateWin32SurfaceKHR(
        static_cast<VkInstance>(internal::getVulkanInstance()),
        &createInfo,
        nullptr,
        &surface);

    if (result != VK_SUCCESS)
    {
        DestroyWindow(hwnd);
        wd->~WindowData();
        heapFree(wd, 1);
        setError("Could not create Vulkan surface");
        return Window{};
    }

    wd->swap = GpuSwapchain::create(surface, wd->width, wd->height, config.preferredPresentMode, config.imageUsage);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return window;
}

void windowDestroy(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    wd->~WindowData();
    heapFree(wd, 1);
}

GpuSwapchain& windowSwapchain(void* data)
{
    return static_cast<WindowData*>(data)->swap;
}

void windowSetTitle(void* data, StringView title)
{
    WindowData* wd = static_cast<WindowData*>(data);
    char buf[256];
    u64 len = title.length < sizeof(buf) - 1 ? title.length : sizeof(buf) - 1;
    memcpy(buf, title.chars, len);
    buf[len] = '\0';

    // Convert to wide string
    wchar_t wbuf[256]{};
    MultiByteToWideChar(CP_UTF8, 0, buf, static_cast<int>(len), wbuf, 255);
    SetWindowTextW(wd->hwnd, wbuf);
}

Span<Event> windowEvents(void* data)
{
    return static_cast<WindowData*>(data)->events;
}

bool windowWasClosed(void* data)
{
    return static_cast<WindowData*>(data)->wasClosed;
}

bool windowIsFocused(void* data)
{
    return static_cast<WindowData*>(data)->isFocused;
}

bool windowWasFocusGained(void* data)
{
    return static_cast<WindowData*>(data)->wasFocusGained;
}

bool windowWasFocusLost(void* data)
{
    return static_cast<WindowData*>(data)->wasFocusLost;
}

bool windowWasResized(void* data)
{
    return static_cast<WindowData*>(data)->wasResized;
}

void windowGetSize(void* data, u32* w, u32* h)
{
    WindowData* wd = static_cast<WindowData*>(data);
    if (w != nullptr)
        *w = wd != nullptr ? wd->width : 0;
    if (h != nullptr)
        *h = wd != nullptr ? wd->height : 0;
}

void windowMaximize(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    ShowWindow(wd->hwnd, SW_MAXIMIZE);
}

void windowMinimize(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    ShowWindow(wd->hwnd, SW_MINIMIZE);
}

void windowRestore(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    ShowWindow(wd->hwnd, SW_RESTORE);
}

void windowSetFullscreen(void* data, bool set)
{
    WindowData* wd = static_cast<WindowData*>(data);

    if (set)
    {
        RECT rect{};
        GetWindowRect(wd->hwnd, &rect);

        DWORD style = static_cast<DWORD>(GetWindowLongPtrA(wd->hwnd, GWL_STYLE));
        style &= ~WS_OVERLAPPEDWINDOW;
        style |= WS_POPUP;
        SetWindowLongPtrA(wd->hwnd, GWL_STYLE, style);

        HMONITOR hmon = MonitorFromWindow(wd->hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEXA mi{};
        mi.cbSize = sizeof(mi);
        GetMonitorInfoA(hmon, &mi);

        SetWindowPos(wd->hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED);

        ShowWindow(wd->hwnd, SW_SHOW);
    }
    else
    {
        DWORD style = static_cast<DWORD>(GetWindowLongPtrA(wd->hwnd, GWL_STYLE));
        style &= ~WS_POPUP;
        style |= WS_OVERLAPPEDWINDOW;
        SetWindowLongPtrA(wd->hwnd, GWL_STYLE, style);

        SetWindowPos(wd->hwnd, HWND_TOP,
            100, 100, 800, 600,
            SWP_FRAMECHANGED);

        ShowWindow(wd->hwnd, SW_SHOW);
    }

    RECT rect{};
    GetClientRect(wd->hwnd, &rect);
    u32 w = static_cast<u32>(rect.right - rect.left);
    u32 h = static_cast<u32>(rect.bottom - rect.top);
    if (w > 0 && h > 0)
    {
        wd->width = w;
        wd->height = h;
        wd->swap.resize(w, h);
    }
}

Vec2 windowMousePos(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    if (wd != nullptr)
        return wd->mouse;

    POINT pt{};
    GetCursorPos(&pt);
    return Vec2{static_cast<f32>(pt.x), static_cast<f32>(pt.y)};
}

Vec2 windowMouseDelta(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    if (wd != nullptr)
        return windowState.mouseDelta / static_cast<f32>(wd->height);

    return windowState.mouseDelta;
}

StringView getClipboardText()
{
    if (!OpenClipboard(nullptr))
        return StringView{};

    StringView result{};

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData != nullptr)
    {
        wchar_t* wstr = static_cast<wchar_t*>(GlobalLock(hData));
        if (wstr != nullptr)
        {
            // Convert wide string to UTF-8
            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
            if (utf8Len > 0)
            {
                windowState.clipboard.resize(static_cast<u64>(utf8Len - 1));
                WideCharToMultiByte(CP_UTF8, 0, wstr, -1, windowState.clipboard.vals, utf8Len, nullptr, nullptr);
                result = StringView{windowState.clipboard.vals, static_cast<u64>(utf8Len - 1)};
            }
            GlobalUnlock(hData);
        }
    }

    CloseClipboard();
    return result;
}

void setClipboardText(StringView text)
{
    if (!OpenClipboard(nullptr))
        return;

    EmptyClipboard();

    // Convert UTF-8 to wide string
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.chars, static_cast<int>(text.length), nullptr, 0);
    if (wlen > 0)
    {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>((wlen + 1) * sizeof(wchar_t)));
        if (hMem != nullptr)
        {
            wchar_t* wstr = static_cast<wchar_t*>(GlobalLock(hMem));
            MultiByteToWideChar(CP_UTF8, 0, text.chars, static_cast<int>(text.length), wstr, wlen);
            wstr[wlen] = L'\0';
            GlobalUnlock(hMem);

            SetClipboardData(CF_UNICODETEXT, hMem);
        }
    }

    CloseClipboard();
}

void openURL(StringView url)
{
    ArenaScope scratch = getScratch();
    char* cstr = cString(scratch, url);
    ShellExecuteA(nullptr, "open", cstr, nullptr, nullptr, SW_SHOW);
}

} // namespace hg::win32
