#include "win32_platform.hpp"

#include "hg/error.hpp"
#include "hg/hash.hpp"

#include <string>
#include <vector>
#include <string_view>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

namespace hg::win32 {

static HINSTANCE hInstance = nullptr;

static bool windowInit(HINSTANCE hInst);
static void windowDeinit();
static bool initAudio();
static void deinitAudio();

bool loadWin32()
{
    hInstance = GetModuleHandleA(nullptr);
    if (hInstance == nullptr)
    {
        setError("Could not get module handle");
        return false;
    }
    return true;
}

bool initWin32()
{
    if (!windowInit(hInstance))
        return false;

    if (!initAudio())
    {
        windowDeinit();
        return false;
    }

    return true;
}

void deinitWin32()
{
    deinitAudio();
    windowDeinit();
}

std::span<std::string_view> getPlatformVulkanExtensions()
{
    static std::string_view exts[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
    };
    return exts;
}

struct WindowData {
    GpuSwapchain swap{};

    HWND hwnd = nullptr;

    std::vector<Event> events{};
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

struct WindowState {
    HINSTANCE hInstance = nullptr;
    ATOM windowClass = 0;

    HCURSOR cursors[CursorType_count]{};
    HCURSOR currentCursor = nullptr;

    std::vector<DisplayInfo> displays{};

    std::vector<Event> events{};
    bool wasQuit = false;
    bool isKeyDown[Button_count]{};
    bool wasKeyDown[Button_count]{};
    Vec2 mouseDelta{};
    Vec2 wheelDelta{};

    HashMap<u64, WindowData*> windows{};
    WindowData* activeWindow = nullptr;

    std::vector<char> clipboard{};
};

static WindowState windowState{};

static u64 hwndToKey(HWND hwnd)
{
    return std::bit_cast<u64>(hwnd);
}

static LRESULT CALLBACK win32WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool windowInit(HINSTANCE hInst)
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
        std::vector<DisplayInfo>* displays;
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

    if (windowState.displays.size() == 0)
    {
        DisplayInfo info{};
        info.sizeW = static_cast<u32>(GetSystemMetrics(SM_CXSCREEN));
        info.sizeH = static_cast<u32>(GetSystemMetrics(SM_CYSCREEN));
        info.workSizeW = info.sizeW;
        info.workSizeH = info.sizeH;
        info.dpiScale = 1.0f;
        windowState.displays.push_back(info);
    }

    return true;
}

static void windowDeinit()
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
                windowState.events.push_back(event);
                w->events.push_back(event);
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
                windowState.events.push_back(event);
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
                windowState.events.push_back(event);
                w->events.push_back(event);
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
                windowState.events.push_back(event);
                w->events.push_back(event);
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
                windowState.events.push_back(event);
                w->events.push_back(event);
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
                windowState.events.push_back(event);
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
            windowState.events.push_back(event);
            return 0;
        }
        case WM_MOUSEHWHEEL:
        {
            f32 delta = static_cast<f32>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;

            windowState.wheelDelta.x += delta;

            Event event{};
            event.type = EventType_wheelMoved;
            event.wheel.delta = Vec2{delta, 0.0f};
            windowState.events.push_back(event);
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
                windowState.events.push_back(event);
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
                windowState.events.push_back(event);
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
                windowState.events.push_back(event);
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
                    std::memset(event.text, 0, sizeof(event.text));
                    std::memcpy(event.text, utf8Buf, static_cast<u64>(utf8Len) < sizeof(event.text) - 1 ? static_cast<u64>(utf8Len) : sizeof(event.text) - 1);
                    windowState.events.push_back(event);
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
                windowState.events.push_back(event);
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

std::span<DisplayInfo> displayInfo()
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
    windowState.events.clear();

    std::memcpy(windowState.wasKeyDown, windowState.isKeyDown, sizeof(windowState.isKeyDown));
    windowState.mouseDelta = {};
    windowState.wheelDelta = {};

    windowState.windows.forEach([](u64, WindowData* data)
    {
        data->events.clear();
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
            windowState.events.push_back(event);
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

std::span<Event> getEvents()
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

Window windowCreate(const WindowConfig& config)
{
    Window window{};

    std::unique_ptr<WindowData> data = std::make_unique<WindowData>();
    WindowData* wd = data.get();

    u32 width = config.width != 0 ? config.width : 0;
    u32 height = config.height != 0 ? config.height : 0;
    std::string titleStr = config.title.length() != 0 ? std::string{config.title} : "Hurdy Gurdy";
    const char* title = titleStr.c_str();

    wchar_t wtitle[256]{};
    MultiByteToWideChar(
        CP_UTF8,
        0,
        title,
        -1,
        wtitle,
        static_cast<int>(sizeof(wtitle) / sizeof(wchar_t)) - 1);

    HWND hwnd = CreateWindowExW(
        0,
        L"HurdyGurdyWindow",
        wtitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width != 0 ? static_cast<int>(width) : CW_USEDEFAULT,
        height != 0 ? static_cast<int>(height) : CW_USEDEFAULT,
        nullptr, nullptr,
        windowState.hInstance,
        nullptr);

    if (hwnd == nullptr)
    {
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
        setError("Could not create Vulkan surface");
        return Window{};
    }

    wd->swap = GpuSwapchain::create(surface, wd->width, wd->height, config.preferredPresentMode, config.imageUsage);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    window.data = data.release();
    return window;
}

void windowDestroy(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    delete wd;
}

GpuSwapchain& windowSwapchain(void* data)
{
    return static_cast<WindowData*>(data)->swap;
}

void windowSetTitle(void* data, std::string_view title)
{
    WindowData* wd = static_cast<WindowData*>(data);
    char buf[256];
    u64 len = title.length() < sizeof(buf) - 1 ? title.length() : sizeof(buf) - 1;
    std::memcpy(buf, title.data(), len);
    buf[len] = '\0';

    // Convert to wide string
    wchar_t wbuf[256]{};
    MultiByteToWideChar(CP_UTF8, 0, buf, static_cast<int>(len), wbuf, 255);
    SetWindowTextW(wd->hwnd, wbuf);
}

std::span<Event> windowEvents(void* data)
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

std::string_view getClipboardText()
{
    if (!OpenClipboard(nullptr))
        return std::string_view{};

    std::string_view result{};

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
                windowState.clipboard.resize(static_cast<u64>(utf8Len));
                WideCharToMultiByte(CP_UTF8, 0, wstr, -1, windowState.clipboard.data(), utf8Len, nullptr, nullptr);
                result = std::string_view{windowState.clipboard.data(), static_cast<u64>(utf8Len - 1)};
            }
            GlobalUnlock(hData);
        }
    }

    CloseClipboard();
    return result;
}

void setClipboardText(std::string_view text)
{
    if (!OpenClipboard(nullptr))
        return;

    EmptyClipboard();

    // Convert UTF-8 to wide string
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.length()), nullptr, 0);
    if (wlen > 0)
    {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>((wlen + 1) * sizeof(wchar_t)));
        if (hMem != nullptr)
        {
            wchar_t* wstr = static_cast<wchar_t*>(GlobalLock(hMem));
            MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.length()), wstr, wlen);
            wstr[wlen] = L'\0';
            GlobalUnlock(hMem);

            SetClipboardData(CF_UNICODETEXT, hMem);
        }
    }

    CloseClipboard();
}

void openURL(std::string_view url)
{
    std::string cstr{url};
    ShellExecuteA(nullptr, "open", cstr.c_str(), nullptr, nullptr, SW_SHOW);
}

struct AudioState {
    IMMDeviceEnumerator* deviceEnumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioRenderClient* renderClient = nullptr;
    HANDLE audioThread = nullptr;
    HANDLE audioEvent = nullptr;
    bool audioThreadRunning = false;
    u32 bufferFrameCount = 0;

    AudioCallback callback = nullptr;
    void* callbackData = nullptr;
    AudioConfig callbackConfig{};
};

static AudioState audio{};

static DWORD WINAPI wasapiAudioThread(LPVOID lpParam)
{
    (void)lpParam;

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    while (audio.audioThreadRunning)
    {
        DWORD waitResult = WaitForSingleObject(audio.audioEvent, 1000);
        if (waitResult == WAIT_TIMEOUT)
            continue;
        if (!audio.audioThreadRunning)
            break;

        u32 padding = 0;
        HRESULT hr = audio.audioClient->GetCurrentPadding(&padding);
        if (FAILED(hr))
            continue;

        u32 availableFrames = audio.bufferFrameCount - padding;
        if (availableFrames == 0)
            continue;

        BYTE* data = nullptr;
        hr = audio.renderClient->GetBuffer(availableFrames, &data);
        if (FAILED(hr))
            continue;

        std::memset(data, 0, static_cast<u64>(availableFrames) * sizeof(f32) * audio.callbackConfig.channels);

        if (audio.callback != nullptr)
        {
            u64 totalSamples = static_cast<u64>(availableFrames) * audio.callbackConfig.channels;
            std::span<f32> buffer{reinterpret_cast<f32*>(data), totalSamples};
            audio.callback(audio.callbackData, buffer, audio.callbackConfig);
        }

        audio.renderClient->ReleaseBuffer(availableFrames, 0);
    }

    CoUninitialize();
    return 0;
}

static bool initAudio()
{
    // Initialize COM on the main thread (per-thread requirement)
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(&audio.deviceEnumerator));

    if (FAILED(hr) || audio.deviceEnumerator == nullptr)
    {
        setError("Could not create WASAPI device enumerator");
        return false;
    }

    hr = audio.deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audio.device);
    if (FAILED(hr) || audio.device == nullptr)
    {
        setError("Could not get default audio endpoint");
        audio.deviceEnumerator->Release();
        audio.deviceEnumerator = nullptr;
        return false;
    }

    hr = audio.device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audio.audioClient));
    if (FAILED(hr) || audio.audioClient == nullptr)
    {
        setError("Could not activate audio client");
        audio.device->Release();
        audio.device = nullptr;
        audio.deviceEnumerator->Release();
        audio.deviceEnumerator = nullptr;
        return false;
    }

    return true;
}

static void deinitAudio()
{
    if (audio.audioThreadRunning)
    {
        audio.audioThreadRunning = false;
        if (audio.audioEvent != nullptr)
            SetEvent(audio.audioEvent);
        if (audio.audioThread != nullptr)
        {
            WaitForSingleObject(audio.audioThread, 5000);
            CloseHandle(audio.audioThread);
            audio.audioThread = nullptr;
        }
    }

    if (audio.audioEvent != nullptr)
    {
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
    }

    if (audio.renderClient != nullptr)
    {
        audio.renderClient->Release();
        audio.renderClient = nullptr;
    }

    if (audio.audioClient != nullptr)
    {
        audio.audioClient->Release();
        audio.audioClient = nullptr;
    }

    if (audio.device != nullptr)
    {
        audio.device->Release();
        audio.device = nullptr;
    }

    if (audio.deviceEnumerator != nullptr)
    {
        audio.deviceEnumerator->Release();
        audio.deviceEnumerator = nullptr;
    }

    // Uninitialize COM on the main thread
    CoUninitialize();
}

void setAudioCallback(AudioCallback callback, void* userData, const AudioConfig& preferredConfig)
{
    if (audio.audioThreadRunning)
    {
        audio.audioThreadRunning = false;
        if (audio.audioEvent != nullptr)
            SetEvent(audio.audioEvent);
        if (audio.audioThread != nullptr)
        {
            WaitForSingleObject(audio.audioThread, 5000);
            CloseHandle(audio.audioThread);
            audio.audioThread = nullptr;
        }
    }

    if (audio.renderClient != nullptr)
    {
        audio.renderClient->Release();
        audio.renderClient = nullptr;
    }

    audio.callback = callback;
    audio.callbackData = userData;
    audio.callbackConfig = preferredConfig;

    if (audio.callbackConfig.sampleRate == 0)
        audio.callbackConfig.sampleRate = 48000;
    if (audio.callbackConfig.channels == 0)
        audio.callbackConfig.channels = 2;

    // Get default format from the audio client
    WAVEFORMATEX* mixFormat = nullptr;
    HRESULT hr = audio.audioClient->GetMixFormat(&mixFormat);
    if (FAILED(hr) || mixFormat == nullptr)
    {
        setError("Could not get audio mix format");
        return;
    }

    // In shared mode, use the mix format directly
    // Update callback config to match what the system provides
    audio.callbackConfig.sampleRate = static_cast<u32>(mixFormat->nSamplesPerSec);
    audio.callbackConfig.channels = static_cast<u32>(mixFormat->nChannels);

    // Set event handle for event-driven mode
    audio.audioEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    if (audio.audioEvent == nullptr)
    {
        setError("Could not create audio event");
        CoTaskMemFree(mixFormat);
        return;
    }

    // Initialize in shared mode with event callback, using mix format
    hr = audio.audioClient->Initialize(
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
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
        return;
    }

    audio.audioClient->SetEventHandle(audio.audioEvent);

    // Get buffer size
    hr = audio.audioClient->GetBufferSize(&audio.bufferFrameCount);
    if (FAILED(hr))
    {
        setError("Could not get audio buffer size");
        audio.audioClient->Release();
        audio.audioClient = nullptr;
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
        return;
    }

    // Get render client
    hr = audio.audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&audio.renderClient));
    if (FAILED(hr) || audio.renderClient == nullptr)
    {
        setError("Could not get audio render client");
        audio.audioClient->Release();
        audio.audioClient = nullptr;
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
        return;
    }

    // Start the audio client
    hr = audio.audioClient->Start();
    if (FAILED(hr))
    {
        setError("Could not start audio client");
        audio.renderClient->Release();
        audio.renderClient = nullptr;
        audio.audioClient->Release();
        audio.audioClient = nullptr;
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
        return;
    }

    // Start audio thread
    audio.audioThreadRunning = true;
    audio.audioThread = CreateThread(nullptr, 0, wasapiAudioThread, nullptr, 0, nullptr);
    if (audio.audioThread == nullptr)
    {
        setError("Could not create audio thread");
        audio.audioClient->Stop();
        audio.renderClient->Release();
        audio.renderClient = nullptr;
        audio.audioClient->Release();
        audio.audioClient = nullptr;
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
        audio.audioThreadRunning = false;
        return;
    }
}

void unsetAudioCallback()
{
    audio.callback = nullptr;
    audio.callbackData = nullptr;
    audio.callbackConfig = {};

    if (audio.audioThreadRunning)
    {
        audio.audioThreadRunning = false;
        if (audio.audioEvent != nullptr)
            SetEvent(audio.audioEvent);
        if (audio.audioThread != nullptr)
        {
            WaitForSingleObject(audio.audioThread, 5000);
            CloseHandle(audio.audioThread);
            audio.audioThread = nullptr;
        }
    }

    if (audio.audioClient != nullptr)
        audio.audioClient->Stop();

    if (audio.renderClient != nullptr)
    {
        audio.renderClient->Release();
        audio.renderClient = nullptr;
    }

    if (audio.audioClient != nullptr)
    {
        audio.audioClient->Release();
        audio.audioClient = nullptr;
    }

    if (audio.audioEvent != nullptr)
    {
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
    }
}

} // namespace hg::win32
