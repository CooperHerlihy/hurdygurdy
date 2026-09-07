#include "sdl_internal.hpp"

#include "internal.hpp"

#include "hg/window.hpp"
#include "hg/array.hpp"
#include "hg/map.hpp"

#include "vulkan/vulkan.h"

namespace hg {

struct WindowData {
    GpuSwapchain swap{};

    SDL_Window* sdlWindow = nullptr;
    f32 mouseX = 0;
    f32 mouseY = 0;
    bool isKeyDown[Button_count]{};
    bool wasClosed = false;
    bool wasResized = false;
    Array<WindowEvent> events{};

    WindowData() noexcept = default;
    ~WindowData() noexcept;

    WindowData(WindowData&& other) noexcept;
    WindowData& operator=(WindowData&& other) noexcept;

    WindowData(const WindowData&) = delete;
    WindowData& operator=(const WindowData&) = delete;
};

static constexpr u32 maxGamepads = 4;

struct WindowState {
    Array<DisplayInfo> displays{};

    Map<SDL_WindowID, WindowData*> windowIds{};

    f32 mouseDX = 0.0f;
    f32 mouseDY = 0.0f;
    f32 wheelDX = 0.0f;
    f32 wheelDY = 0.0f;
    bool wasQuit = false;

    SDL_Cursor* cursors[CursorType_count]{};
    SDL_Cursor* currentCursor = nullptr;

    SDL_Gamepad* gamepads[maxGamepads]{};
    Map<SDL_JoystickID, u32> gamepadIds{};
    bool isGamepadButtonDown[maxGamepads][Button_count]{};
    i16 gamepadAxis[maxGamepads][6]{};
};

static WindowState windowState{};

void windowInit()
{
    windowState = WindowState{};

    int count = 0;
    SDL_DisplayID* ids = SDL_GetDisplays(&count);
    HG_DEFER(SDL_free(ids));

    windowState.displays.resize(static_cast<u64>(count));
    for (u32 i = 0; i < windowState.displays.count; i++)
    {
        DisplayInfo& info = windowState.displays[i];

        SDL_DisplayID displayId = ids[i];
        SDL_Rect r;
        SDL_GetDisplayBounds(displayId, &r);
        info.posX = static_cast<i32>(r.x);
        info.posY = static_cast<i32>(r.y);
        info.sizeW = static_cast<u32>(r.w);
        info.sizeH = static_cast<u32>(r.h);

        if (SDL_GetDisplayUsableBounds(displayId, &r) && r.w > 0 && r.h > 0)
        {
            info.workPosX = static_cast<i32>(r.x);
            info.workPosY = static_cast<i32>(r.y);
            info.workSizeW = static_cast<u32>(r.w);
            info.workSizeH = static_cast<u32>(r.h);
        }
        else
        {
            info.workPosX = info.posX;
            info.workPosY = info.posY;
            info.workSizeW = info.sizeW;
            info.workSizeH = info.sizeH;
        }

        info.dpiScale = SDL_GetDisplayContentScale(displayId);
    }
}

void windowDeinit()
{
    for (u32 i = 0; i < maxGamepads; i++)
    {
        if (windowState.gamepads[i] != nullptr)
            SDL_CloseGamepad(windowState.gamepads[i]);
    }

    for (u32 i = 0; i < CursorType_count; i++)
    {
        if (windowState.cursors[i] != nullptr)
            SDL_DestroyCursor(windowState.cursors[i]);
    }
    windowState.currentCursor = nullptr;
}

Span<DisplayInfo> displayInfo()
{
    return windowState.displays;
}

static SDL_SystemCursor cursorToSdl(CursorType type)
{
    switch (type)
    {
        case CursorType_arrow:
            return SDL_SYSTEM_CURSOR_DEFAULT;
        case CursorType_textInput:
            return SDL_SYSTEM_CURSOR_TEXT;
        case CursorType_resizeAll:
            return SDL_SYSTEM_CURSOR_MOVE;
        case CursorType_resizeNS:
            return SDL_SYSTEM_CURSOR_NS_RESIZE;
        case CursorType_resizeEW:
            return SDL_SYSTEM_CURSOR_EW_RESIZE;
        case CursorType_resizeNESW:
            return SDL_SYSTEM_CURSOR_NESW_RESIZE;
        case CursorType_resizeNWSE:
            return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
        case CursorType_hand:
            return SDL_SYSTEM_CURSOR_POINTER;
        case CursorType_wait:
            return SDL_SYSTEM_CURSOR_WAIT;
        case CursorType_progress:
            return SDL_SYSTEM_CURSOR_PROGRESS;
        case CursorType_notAllowed:
            return SDL_SYSTEM_CURSOR_NOT_ALLOWED;
        default:
            return SDL_SYSTEM_CURSOR_DEFAULT;
    }
}

void setCursor(CursorType type)
{
    SDL_Cursor*& cursor = windowState.cursors[static_cast<u32>(type)];
    if (cursor == nullptr)
        cursor = SDL_CreateSystemCursor(cursorToSdl(type));

    if (windowState.currentCursor != cursor)
    {
        SDL_SetCursor(cursor);
        windowState.currentCursor = cursor;
    }
}

void showCursor(bool show)
{
    if (show)
        SDL_ShowCursor();
    else
        SDL_HideCursor();
}

String getClipboardText()
{
    char* sdlText = SDL_GetClipboardText();
    HG_DEFER(SDL_free(sdlText));
    return String::create(sdlText);
}

void setClipboardText(StringView text)
{
    SDL_SetClipboardText(cString(getScratch(), text));
}

void openURL(StringView url)
{
    SDL_OpenURL(cString(getScratch(), url));
}

static Button sdlKeycodeToHgButton(u32 key)
{
    switch (key)
    {
        case SDLK_0: return Button_0;
        case SDLK_1: return Button_1;
        case SDLK_2: return Button_2;
        case SDLK_3: return Button_3;
        case SDLK_4: return Button_4;
        case SDLK_5: return Button_5;
        case SDLK_6: return Button_6;
        case SDLK_7: return Button_7;
        case SDLK_8: return Button_8;
        case SDLK_9: return Button_9;
        case SDLK_Q: return Button_q;
        case SDLK_W: return Button_w;
        case SDLK_E: return Button_e;
        case SDLK_R: return Button_r;
        case SDLK_T: return Button_t;
        case SDLK_Y: return Button_y;
        case SDLK_U: return Button_u;
        case SDLK_I: return Button_i;
        case SDLK_O: return Button_o;
        case SDLK_P: return Button_p;
        case SDLK_A: return Button_a;
        case SDLK_S: return Button_s;
        case SDLK_D: return Button_d;
        case SDLK_F: return Button_f;
        case SDLK_G: return Button_g;
        case SDLK_H: return Button_h;
        case SDLK_J: return Button_j;
        case SDLK_K: return Button_k;
        case SDLK_L: return Button_l;
        case SDLK_Z: return Button_z;
        case SDLK_X: return Button_x;
        case SDLK_C: return Button_c;
        case SDLK_V: return Button_v;
        case SDLK_B: return Button_b;
        case SDLK_N: return Button_n;
        case SDLK_M: return Button_m;
        case SDLK_SEMICOLON: return Button_semicolon;
        case SDLK_COLON: return Button_semicolon;
        case SDLK_APOSTROPHE: return Button_apostrophe;
        case SDLK_DBLAPOSTROPHE: return Button_apostrophe;
        case SDLK_COMMA: return Button_comma;
        case SDLK_PERIOD: return Button_period;
        case SDLK_QUESTION: return Button_slash;
        case SDLK_GRAVE: return Button_grave;
        case SDLK_TILDE: return Button_grave;
        case SDLK_EXCLAIM: return Button_1;
        case SDLK_AT: return Button_2;
        case SDLK_HASH: return Button_3;
        case SDLK_DOLLAR: return Button_4;
        case SDLK_PERCENT: return Button_5;
        case SDLK_CARET: return Button_6;
        case SDLK_AMPERSAND: return Button_7;
        case SDLK_ASTERISK: return Button_8;
        case SDLK_LEFTPAREN: return Button_9;
        case SDLK_RIGHTPAREN: return Button_0;
        case SDLK_LEFTBRACKET: return Button_lbracket;
        case SDLK_RIGHTBRACKET: return Button_rbracket;
        case SDLK_LEFTBRACE: return Button_lbracket;
        case SDLK_RIGHTBRACE: return Button_rbracket;
        case SDLK_EQUALS: return Button_equal;
        case SDLK_LESS: return Button_comma;
        case SDLK_GREATER: return Button_period;
        case SDLK_PLUS: return Button_equal;
        case SDLK_MINUS: return Button_minus;
        case SDLK_SLASH: return Button_slash;
        case SDLK_BACKSLASH: return Button_backslash;
        case SDLK_UNDERSCORE: return Button_minus;
        case SDLK_PIPE: return Button_backslash;
        case SDLK_UP: return Button_up;
        case SDLK_DOWN: return Button_down;
        case SDLK_LEFT: return Button_left;
        case SDLK_RIGHT: return Button_right;
        case SDLK_ESCAPE: return Button_escape;
        case SDLK_SPACE: return Button_space;
        case SDLK_RETURN: return Button_enter;
        case SDLK_BACKSPACE: return Button_backspace;
        case SDLK_DELETE: return Button_kdelete;
        case SDLK_INSERT: return Button_insert;
        case SDLK_TAB: return Button_tab;
        case SDLK_HOME: return Button_home;
        case SDLK_END: return Button_end;
        case SDLK_PAGEUP: return Button_pageup;
        case SDLK_PAGEDOWN: return Button_pagedown;
        case SDLK_F1: return Button_f1;
        case SDLK_F2: return Button_f2;
        case SDLK_F3: return Button_f3;
        case SDLK_F4: return Button_f4;
        case SDLK_F5: return Button_f5;
        case SDLK_F6: return Button_f6;
        case SDLK_F7: return Button_f7;
        case SDLK_F8: return Button_f8;
        case SDLK_F9: return Button_f9;
        case SDLK_F10: return Button_f10;
        case SDLK_F11: return Button_f11;
        case SDLK_F12: return Button_f12;
        case SDLK_PRINTSCREEN: return Button_printscreen;
        case SDLK_APPLICATION: return Button_context;
        case SDLK_KP_0: return Button_numpad0;
        case SDLK_KP_1: return Button_numpad1;
        case SDLK_KP_2: return Button_numpad2;
        case SDLK_KP_3: return Button_numpad3;
        case SDLK_KP_4: return Button_numpad4;
        case SDLK_KP_5: return Button_numpad5;
        case SDLK_KP_6: return Button_numpad6;
        case SDLK_KP_7: return Button_numpad7;
        case SDLK_KP_8: return Button_numpad8;
        case SDLK_KP_9: return Button_numpad9;
        case SDLK_KP_PERIOD: return Button_numpaddecimal;
        case SDLK_KP_DIVIDE: return Button_numpaddiv;
        case SDLK_KP_MULTIPLY: return Button_numpadmul;
        case SDLK_KP_MINUS: return Button_numpadminus;
        case SDLK_KP_PLUS: return Button_numpadplus;
        case SDLK_KP_ENTER: return Button_numpadenter;
        case SDLK_LSHIFT: return Button_lshift;
        case SDLK_RSHIFT: return Button_rshift;
        case SDLK_LCTRL: return Button_lctrl;
        case SDLK_RCTRL: return Button_rctrl;
        case SDLK_LALT: return Button_lalt;
        case SDLK_RALT: return Button_ralt;
        case SDLK_LGUI: return Button_lsuper;
        case SDLK_RGUI: return Button_rsuper;
        case SDLK_CAPSLOCK: return Button_capslock;
        case SDLK_NUMLOCKCLEAR: return Button_numlock;
        case SDLK_SCROLLLOCK: return Button_scrolllock;
        case SDLK_PAUSE: return Button_pause;
    }
    return Button_none;
}

static Button sdlButtonToHgButton(u32 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT: return Button_mouse1;
        case SDL_BUTTON_RIGHT: return Button_mouse2;
        case SDL_BUTTON_MIDDLE: return Button_mouse3;
        case SDL_BUTTON_X1: return Button_mouse4;
        case SDL_BUTTON_X2: return Button_mouse5;
    }
    return Button_none;
}

static Button sdlGamepadButtonToHgButton(u8 button)
{
    switch (button)
    {
        case SDL_GAMEPAD_BUTTON_SOUTH: return Button_gamepadSouth;
        case SDL_GAMEPAD_BUTTON_EAST: return Button_gamepadEast;
        case SDL_GAMEPAD_BUTTON_WEST: return Button_gamepadWest;
        case SDL_GAMEPAD_BUTTON_NORTH: return Button_gamepadNorth;
        case SDL_GAMEPAD_BUTTON_BACK: return Button_gamepadBack;
        case SDL_GAMEPAD_BUTTON_GUIDE: return Button_gamepadGuide;
        case SDL_GAMEPAD_BUTTON_START: return Button_gamepadStart;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: return Button_gamepadLeftStick;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return Button_gamepadRightStick;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return Button_gamepadLeftShoulder;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return Button_gamepadRightShoulder;
        case SDL_GAMEPAD_BUTTON_DPAD_UP: return Button_gamepadDpadUp;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return Button_gamepadDpadDown;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return Button_gamepadDpadLeft;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return Button_gamepadDpadRight;
    }
    return Button_none;
}

static u32 findGamepadIndex(SDL_JoystickID id)
{
    u32* idx = windowState.gamepadIds.get(id);
    if (idx != nullptr)
        return *idx;
    return maxGamepads;
}

static void openGamepad(SDL_JoystickID id)
{
    u32 idx = findGamepadIndex(id);
    if (idx < maxGamepads)
        return;

    for (u32 i = 0; i < maxGamepads; i++)
    {
        if (windowState.gamepads[i] == nullptr)
        {
            SDL_Gamepad* gp = SDL_OpenGamepad(id);
            if (gp != nullptr)
            {
                windowState.gamepads[i] = gp;
                windowState.gamepadIds.add(id, i);
            }
            return;
        }
    }
}

static void closeGamepad(SDL_JoystickID id)
{
    u32* idx = windowState.gamepadIds.get(id);
    if (idx == nullptr)
        return;

    SDL_CloseGamepad(windowState.gamepads[*idx]);
    windowState.gamepads[*idx] = nullptr;
    windowState.gamepadIds.remove(id);
}

void processEvents()
{
    windowState.mouseDX = 0;
    windowState.mouseDY = 0;
    windowState.wheelDX = 0;
    windowState.wheelDY = 0;

    windowState.windowIds.forEach([&](const SDL_WindowID&, WindowData*& window)
    {
        window->events.count = 0;
        window->wasResized = false;
    });

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
            {
                windowState.wasQuit = true;

                WindowEvent windowEvent{};
                windowEvent.type = WindowEventType_quit;
                windowEvent.timestamp = event.common.timestamp;

                windowState.windowIds.forEach([&](const SDL_WindowID&, WindowData*& w)
                {
                    w->events.push(windowEvent);
                });
            } break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                WindowData** w = windowState.windowIds.get(event.window.windowID);
                if (w != nullptr)
                {
                    (*w)->wasClosed = true;

                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_close;
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                }
            } break;
            case SDL_EVENT_WINDOW_RESIZED:
            {
                WindowData** w = windowState.windowIds.get(event.window.windowID);
                if (w != nullptr)
                {
                    u32 w2, h2;
                    SDL_GetWindowSize((*w)->sdlWindow,
                        reinterpret_cast<int*>(&w2),
                        reinterpret_cast<int*>(&h2));
                    (*w)->swap.resize(w2, h2);
                    (*w)->wasResized = true;

                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_resize;
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                }
            } break;
            case SDL_EVENT_WINDOW_MAXIMIZED:
            {
                WindowData** w = windowState.windowIds.get(event.window.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_maximize;
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                }
            } break;
            case SDL_EVENT_WINDOW_MINIMIZED:
            {
                WindowData** w = windowState.windowIds.get(event.window.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_minimize;
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                }
            } break;
            case SDL_EVENT_WINDOW_RESTORED:
            {
                WindowData** w = windowState.windowIds.get(event.window.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_restore;
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                }
            } break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            {
                WindowData** w = windowState.windowIds.get(event.window.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_focusGained;
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                }
            } break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
            {
                WindowData** w = windowState.windowIds.get(event.window.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_focusLost;
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                }
            } break;
            case SDL_EVENT_MOUSE_MOTION:
            {
                WindowData** w = windowState.windowIds.get(event.button.windowID);
                if (w != nullptr)
                {
                    (*w)->mouseX = event.motion.x;
                    (*w)->mouseY = event.motion.y;
                }
                windowState.mouseDX += event.motion.xrel;
                windowState.mouseDY += event.motion.yrel;
            } break;
            case SDL_EVENT_MOUSE_WHEEL:
            {
                windowState.wheelDX += event.wheel.x;
                windowState.wheelDY += event.wheel.y;
            } break;
            case SDL_EVENT_KEY_DOWN:
            {
                Button key = sdlKeycodeToHgButton(event.key.key);
                WindowData** w = windowState.windowIds.get(event.key.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_buttonPress;
                    windowEvent.button = key;
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = true;
                }
            } break;
            case SDL_EVENT_KEY_UP:
            {
                Button key = sdlKeycodeToHgButton(event.key.key);
                WindowData** w = windowState.windowIds.get(event.key.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_buttonRelease;
                    windowEvent.button = key;
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = false;
                }
            } break;
            case SDL_EVENT_TEXT_INPUT:
            {
                WindowData** w = windowState.windowIds.get(event.text.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_textInput;
                    memset(windowEvent.text, 0, sizeof(windowEvent.text));
                    strncpy(windowEvent.text, event.text.text, sizeof(windowEvent.text) - 1);
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                Button key = sdlButtonToHgButton(event.button.button);
                WindowData** w = windowState.windowIds.get(event.button.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_buttonPress;
                    windowEvent.button = key;
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = true;
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                Button key = sdlButtonToHgButton(event.button.button);
                WindowData** w = windowState.windowIds.get(event.button.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_buttonRelease;
                    windowEvent.button = key;
                    windowEvent.timestamp = event.common.timestamp;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = false;
                }
            } break;
            case SDL_EVENT_GAMEPAD_ADDED:
            {
                openGamepad(event.gdevice.which);

                WindowEvent windowEvent{};
                windowEvent.type = WindowEventType_gamepadConnected;
                windowEvent.button = Button_none;
                windowEvent.gamepad = findGamepadIndex(event.gdevice.which);
                windowEvent.timestamp = event.common.timestamp;

                windowState.windowIds.forEach([&](const SDL_WindowID&, WindowData*& w)
                {
                    w->events.push(windowEvent);
                });
            } break;
            case SDL_EVENT_GAMEPAD_REMOVED:
            {
                u32 idx = findGamepadIndex(event.gdevice.which);

                WindowEvent windowEvent{};
                windowEvent.type = WindowEventType_gamepadDisconnected;
                windowEvent.button = Button_none;
                windowEvent.gamepad = idx;
                windowEvent.timestamp = event.common.timestamp;

                windowState.windowIds.forEach([&](const SDL_WindowID&, WindowData*& w)
                {
                    w->events.push(windowEvent);
                });

                closeGamepad(event.gdevice.which);
            } break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            {
                Button key = sdlGamepadButtonToHgButton(event.gbutton.button);
                u32 idx = findGamepadIndex(event.gbutton.which);
                if (idx < maxGamepads)
                {
                    windowState.isGamepadButtonDown[idx][key] = true;

                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_gamepadButtonPress;
                    windowEvent.button = key;
                    windowEvent.gamepad = idx;
                    windowEvent.timestamp = event.common.timestamp;

                    windowState.windowIds.forEach([&](const SDL_WindowID&, WindowData*& w)
                    {
                        w->events.push(windowEvent);
                    });
                }
            } break;
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
            {
                Button key = sdlGamepadButtonToHgButton(event.gbutton.button);
                u32 idx = findGamepadIndex(event.gbutton.which);
                if (idx < maxGamepads)
                {
                    windowState.isGamepadButtonDown[idx][key] = false;

                    WindowEvent windowEvent{};
                    windowEvent.type = WindowEventType_gamepadButtonRelease;
                    windowEvent.button = key;
                    windowEvent.gamepad = idx;
                    windowEvent.timestamp = event.common.timestamp;

                    windowState.windowIds.forEach([&](const SDL_WindowID&, WindowData*& w)
                    {
                        w->events.push(windowEvent);
                    });
                }
            } break;
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            {
                u32 idx = findGamepadIndex(event.gaxis.which);
                if (idx < maxGamepads && event.gaxis.axis < 6)
                    windowState.gamepadAxis[idx][event.gaxis.axis] = event.gaxis.value;
            } break;
        }
    }
}

bool wasQuit()
{
    return windowState.wasQuit;
}

WindowData::~WindowData() noexcept
{
    if (sdlWindow != nullptr)
    {
        windowState.windowIds.remove(SDL_GetWindowID(sdlWindow));
        SDL_DestroyWindow(sdlWindow);
    }
}

WindowData::WindowData(WindowData&& other) noexcept
    : swap{std::move(other.swap)}
    , sdlWindow{std::exchange(other.sdlWindow, nullptr)}
    , mouseX{std::exchange(other.mouseX, 0.0f)}
    , mouseY{std::exchange(other.mouseY, 0.0f)}
    , wasClosed{std::exchange(other.wasClosed, false)}
    , wasResized{std::exchange(other.wasResized, false)}
    , events{std::move(other.events)}
{
    memcpy(isKeyDown, other.isKeyDown, sizeof(isKeyDown));
    memset(other.isKeyDown, 0, sizeof(other.isKeyDown));
}

WindowData& WindowData::operator=(WindowData&& other) noexcept
{
    if (this != &other)
    {
        this->~WindowData();
        new (this) WindowData{std::move(other)};
    }
    return *this;
}

Window::Window() noexcept
    : data{nullptr}
{}

Window::~Window() noexcept = default;
Window::Window(Window&& other) noexcept = default;
Window& Window::operator=(Window&& other) noexcept = default;

Window Window::create(const WindowConfig& config)
{
    Window window{};
    window.data = makeUnique<WindowData>();

    ArenaScope scratch = getScratch();

    window.data->sdlWindow = SDL_CreateWindow(
        "Hurdy Gurdy",
        800, 600,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (window.data->sdlWindow == nullptr)
        HG_PANIC("SDL could not create window: %s\n", SDL_GetError());

    windowState.windowIds.add(SDL_GetWindowID(window.data->sdlWindow), window.data);

    u32 w, h;
    SDL_GetWindowSize(window.data->sdlWindow,
        reinterpret_cast<int*>(&w),
        reinterpret_cast<int*>(&h));

    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(
        window.data->sdlWindow,
        static_cast<VkInstance>(internal::getVulkanInstance()),
        nullptr,
        &surface))
        HG_PANIC("SDL could not create Vulkan surface: %s\n", SDL_GetError());

    window.data->swap = GpuSwapchain::create(surface, w, h, config.preferredPresentMode, config.imageUsage);

    return window;
}

GpuSwapchain& Window::swapchain()
{
    return data->swap;
}

GpuView* Window::imageView() const
{
    return data->swap.currentView();
}

Format Window::imageFormat() const
{
    return data->swap.format();
}

void Window::setTitle(StringView title)
{
    SDL_SetWindowTitle(data->sdlWindow, cString(getScratch(), title));
}

void Window::pos(i32* x, i32* y) const
{
    SDL_GetWindowPosition(data->sdlWindow, x, y);
}

void Window::setPos(i32 x, i32 y)
{
    SDL_SetWindowPosition(data->sdlWindow, x, y);
}

void Window::size(u32* w, u32* h) const
{
    if (data != nullptr)
    {
        u32 sw;
        u32 sh;
        data->swap.size(&sw, &sh);
        if (w != nullptr)
            *w = sw;
        if (h != nullptr)
            *h = sh;
    }
    else
    {
        if (w != nullptr)
            *w = 0;
        if (h != nullptr)
            *h = 0;
    }
}

void Window::setSize(u32 width, u32 height)
{
    SDL_SetWindowSize(data->sdlWindow, static_cast<int>(width), static_cast<int>(height));
    data->swap.resize(width, height);
}

bool Window::isFullscreen() const
{
    return (SDL_GetWindowFlags(data->sdlWindow) & SDL_WINDOW_FULLSCREEN) != 0;
}

void Window::setFullscreen(bool set)
{
    SDL_SetWindowFullscreen(data->sdlWindow, set ? SDL_WINDOW_FULLSCREEN : 0);

    u32 w, h;
    size(&w, &h);
    data->swap.resize(w, h);
}

void Window::setResizeable(bool set)
{
    SDL_SetWindowResizable(data->sdlWindow, set);
}

bool Window::isFocused() const
{
    return SDL_GetMouseFocus() == data->sdlWindow;
}

bool Window::wasClosed() const
{
    return data->wasClosed;
}

bool Window::wasResized() const
{
    return data->wasResized;
}

bool Window::isMaximized() const
{
    return (SDL_GetWindowFlags(data->sdlWindow) & SDL_WINDOW_MAXIMIZED) != 0;
}

bool Window::isMinimized() const
{
    return (SDL_GetWindowFlags(data->sdlWindow) & SDL_WINDOW_MINIMIZED) != 0;
}

void Window::maximize()
{
    SDL_MaximizeWindow(data->sdlWindow);
}

void Window::minimize()
{
    SDL_MinimizeWindow(data->sdlWindow);
}

void Window::restore()
{
    SDL_RestoreWindow(data->sdlWindow);
}

Vec2 Window::globalMousePos() const
{
    f32 x, y;
    SDL_GetGlobalMouseState(&x, &y);
    return Vec2{x, y};
}

Vec2 Window::mousePos() const
{
    if (data != nullptr)
        return Vec2{data->mouseX, data->mouseY};
    return Vec2{0};
}

Vec2 Window::mouseDelta() const
{
    if (data != nullptr)
    {
        u32 h;
        data->swap.size(nullptr, &h);
        return Vec2{
            windowState.mouseDX / static_cast<f32>(h),
            windowState.mouseDY / static_cast<f32>(h),
        };
    }
    return Vec2{0};
}

Vec2 Window::wheelDelta() const
{
    return Vec2{windowState.wheelDX, windowState.wheelDY};
}

bool Window::isButtonDown(Button key) const
{
    return data->isKeyDown[key];
}

Span<WindowEvent> Window::events() const
{
    return data->events;
}

u32 gamepadCount()
{
    u32 count = 0;
    for (u32 i = 0; i < maxGamepads; i++)
    {
        if (windowState.gamepads[i] != nullptr)
            count++;
    }
    return count;
}

bool isGamepadActive(u32 gamepad)
{
    return gamepad < maxGamepads && windowState.gamepads[gamepad] != nullptr;
}

bool isGamepadButtonDown(u32 gamepad, Button key)
{
    if (gamepad < maxGamepads)
        return windowState.isGamepadButtonDown[gamepad][key];
    return false;
}

Vec2 gamepadLeftStick(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad] != nullptr)
        return Vec2{
            static_cast<f32>(windowState.gamepadAxis[gamepad][0]) / 32767.0f,
            static_cast<f32>(windowState.gamepadAxis[gamepad][1]) / 32767.0f,
        };
    else
        return Vec2{0};
}

Vec2 gamepadRightStick(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad] != nullptr)
        return Vec2{
            static_cast<f32>(windowState.gamepadAxis[gamepad][2]) / 32767.0f,
            static_cast<f32>(windowState.gamepadAxis[gamepad][3]) / 32767.0f,
        };
    else
        return Vec2{0};
}

f32 gamepadLeftTrigger(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad] != nullptr)
        return static_cast<f32>(windowState.gamepadAxis[gamepad][4]) / 32767.0f;
    else
        return 0;
}

f32 gamepadRightTrigger(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad] != nullptr)
        return static_cast<f32>(windowState.gamepadAxis[gamepad][5]) / 32767.0f;
    else
        return 0;
}

} // namespace hg
