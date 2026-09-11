#include "hg/window.hpp"

#include "sdl_internal.hpp"
#include "internal.hpp"
#include "hg/error.hpp"
#include "hg/array.hpp"
#include "hg/map.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

namespace hg::sdl {

struct WindowData {
    GpuSwapchain swap{};

    SDL_Window* sdlWindow = nullptr;

    Array<Event> events{};
    Vec2 mouse{};
    i32 x = 0;
    i32 y = 0;
    u32 width = 0;
    u32 height = 0;
    bool wasClosed = false;
    bool isFocused = false;
    bool wasFocusGained = false;
    bool wasFocusLost = false;
    bool wasMoved = false;
    bool wasResized = false;
    bool wasMaximized = false;
    bool wasMinimized = false;
    bool wasRestored = false;
    bool wasFullscreened = false;

    WindowData() noexcept = default;
    ~WindowData() noexcept;

    WindowData(WindowData&& other) noexcept;
    WindowData& operator=(WindowData&& other) noexcept;

    WindowData(const WindowData&) = delete;
    WindowData& operator=(const WindowData&) = delete;
};

static constexpr u32 maxGamepads = 8;

struct WindowState {
    Array<DisplayInfo> displays{};

    SDL_Cursor* cursors[CursorType_count]{};
    SDL_Cursor* currentCursor = nullptr;

    Array<Event> events{};
    bool wasQuit = false;
    bool isKeyDown[Button_count]{};
    bool wasKeyDown[Button_count]{};
    Vec2 mouseDelta{};
    Vec2 wheelDelta{};

    Map<SDL_JoystickID, u32> gamepadIds{};
    SDL_Gamepad* gamepads[maxGamepads]{};
    bool isGamepadButtonDown[maxGamepads][GamepadButton_count]{};
    bool wasGamepadButtonDown[maxGamepads][GamepadButton_count]{};
    i16 gamepadAxes[maxGamepads][6]{};

    Map<SDL_WindowID, WindowData*> windows{};

    Array<char> clipboard{};
};

static WindowState windowState{};

bool windowInit()
{
    windowState = WindowState{};

    int count = 0;
    SDL_DisplayID* ids = SDL_GetDisplays(&count);
    if (ids == nullptr)
    {
        setError(SDL_GetError());
        return false;
    }
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

    return true;
}

void windowDeinit()
{
    windowState.windows.forEach([](SDL_WindowID, WindowData* window)
    {
        *window = {};
    });

    for (SDL_Gamepad* gamepad : windowState.gamepads)
    {
        if (gamepad != nullptr)
            SDL_CloseGamepad(gamepad);
    }

    for (u32 i = 0; i < CursorType_count; i++)
    {
        if (windowState.cursors[i] != nullptr)
            SDL_DestroyCursor(windowState.cursors[i]);
    }
    windowState.currentCursor = nullptr;
}

WindowData::~WindowData() noexcept
{
    if (sdlWindow != nullptr)
    {
        windowState.windows.remove(SDL_GetWindowID(sdlWindow));
        SDL_DestroyWindow(sdlWindow);
    }
}

WindowData::WindowData(WindowData&& other) noexcept
    : swap{std::move(other.swap)}
    , sdlWindow{std::exchange(other.sdlWindow, nullptr)}
    , events{std::move(other.events)}
    , mouse{std::exchange(other.mouse, {})}
    , x{std::exchange(other.x, 0)}
    , y{std::exchange(other.y, 0)}
    , width{std::exchange(other.width, 0)}
    , height{std::exchange(other.height, 0)}
    , wasClosed{std::exchange(other.wasClosed, false)}
    , isFocused{std::exchange(other.isFocused, false)}
    , wasFocusGained{std::exchange(other.wasFocusGained, false)}
    , wasFocusLost{std::exchange(other.wasFocusLost, false)}
    , wasMoved{std::exchange(other.wasMoved, false)}
    , wasResized{std::exchange(other.wasResized, false)}
    , wasMaximized{std::exchange(other.wasMaximized, false)}
    , wasMinimized{std::exchange(other.wasMinimized, false)}
    , wasFullscreened{std::exchange(other.wasFullscreened, false)}
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

static GamepadButton sdlGamepadButtonToHgButton(u8 button)
{
    switch (button)
    {
        case SDL_GAMEPAD_BUTTON_SOUTH: return GamepadButton_south;
        case SDL_GAMEPAD_BUTTON_EAST: return GamepadButton_east;
        case SDL_GAMEPAD_BUTTON_WEST: return GamepadButton_west;
        case SDL_GAMEPAD_BUTTON_NORTH: return GamepadButton_north;
        case SDL_GAMEPAD_BUTTON_BACK: return GamepadButton_back;
        case SDL_GAMEPAD_BUTTON_GUIDE: return GamepadButton_guide;
        case SDL_GAMEPAD_BUTTON_START: return GamepadButton_start;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: return GamepadButton_leftStick;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return GamepadButton_rightStick;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return GamepadButton_leftShoulder;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return GamepadButton_rightShoulder;
        case SDL_GAMEPAD_BUTTON_DPAD_UP: return GamepadButton_dpadUp;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return GamepadButton_dpadDown;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return GamepadButton_dpadLeft;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return GamepadButton_dpadRight;
    }
    return GamepadButton_none;
}

static u32 findGamepadIndex(SDL_JoystickID id)
{
    u32* idx = windowState.gamepadIds.get(id);
    if (idx != nullptr)
        return *idx;
    return maxGamepads;
}

static WindowData* getWindow(SDL_WindowID id)
{
    WindowData** data = windowState.windows.get(id);
    if (data == nullptr)
        return nullptr;
    return *data;
}

Span<DisplayInfo> displayInfo()
{
    return windowState.displays;
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

void processEvents()
{
    windowState.events.reset();

    memcpy(windowState.wasKeyDown, windowState.isKeyDown, sizeof(windowState.isKeyDown));
    windowState.mouseDelta = {};
    windowState.wheelDelta = {};

    memcpy(windowState.wasGamepadButtonDown, windowState.isGamepadButtonDown, sizeof(windowState.isGamepadButtonDown));

    windowState.windows.forEach([&](SDL_WindowID, WindowData* data)
    {
        data->events.reset();
        data->wasFocusGained = false;
        data->wasFocusLost = false;
        data->wasMoved = false;
        data->wasResized = false;
        data->wasMaximized = false;
        data->wasMinimized = false;
        data->wasRestored = false;
        data->wasFullscreened = false;
    });

    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent))
    {
        switch (sdlEvent.type)
        {
            case SDL_EVENT_QUIT:
            {
                windowState.wasQuit = true;

                Event event{};
                event.type = EventType_quit;
                event.timestamp = sdlEvent.common.timestamp;
                windowState.events.push(event);
            } break;
            case SDL_EVENT_TEXT_INPUT:
            {
                Event event{};
                event.type = EventType_text;
                event.timestamp = sdlEvent.common.timestamp;
                memset(event.text, 0, sizeof(event.text));
                strncpy(event.text, sdlEvent.text.text, sizeof(event.text) - 1);
                windowState.events.push(event);
            } break;
            case SDL_EVENT_KEY_DOWN:
            {
                Event event{};
                event.type = EventType_keyPress;
                event.timestamp = sdlEvent.common.timestamp;
                event.button = sdlKeycodeToHgButton(sdlEvent.key.key);
                windowState.events.push(event);

                windowState.isKeyDown[event.button] = true;
            } break;
            case SDL_EVENT_KEY_UP:
            {
                Event event{};
                event.type = EventType_keyRelease;
                event.timestamp = sdlEvent.common.timestamp;
                event.button = sdlKeycodeToHgButton(sdlEvent.key.key);
                windowState.events.push(event);

                windowState.isKeyDown[event.button] = false;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                Event event{};
                event.type = EventType_keyPress;
                event.timestamp = sdlEvent.common.timestamp;
                event.button = sdlButtonToHgButton(sdlEvent.button.button);
                windowState.events.push(event);

                windowState.isKeyDown[event.button] = true;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                Event event{};
                event.type = EventType_keyRelease;
                event.timestamp = sdlEvent.common.timestamp;
                event.button = sdlButtonToHgButton(sdlEvent.button.button);
                windowState.events.push(event);

                windowState.isKeyDown[event.button] = false;
            } break;
            case SDL_EVENT_MOUSE_MOTION:
            {
                f32 gmx, gmy;
                SDL_GetGlobalMouseState(&gmx, &gmy);

                Event event{};
                event.type = EventType_mouseMoved;
                event.timestamp = sdlEvent.common.timestamp;
                event.mouse.delta = {sdlEvent.motion.xrel, sdlEvent.motion.yrel};
                event.mouse.pos = {sdlEvent.motion.x, sdlEvent.motion.y};
                event.mouse.globalPos = {gmx, gmy};
                windowState.events.push(event);

                WindowData* w = getWindow(sdlEvent.motion.windowID);
                if (w != nullptr)
                    w->mouse = Vec2{event.mouse.pos.x, event.mouse.pos.y};

                windowState.mouseDelta += event.mouse.delta;
            } break;
            case SDL_EVENT_MOUSE_WHEEL:
            {
                Event event{};
                event.type = EventType_wheelMoved;
                event.timestamp = sdlEvent.common.timestamp;
                event.wheel.delta = {sdlEvent.wheel.x, sdlEvent.wheel.y};
                windowState.events.push(event);

                windowState.wheelDelta += event.wheel.delta;
            } break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                Event event{};
                event.type = EventType_windowClosed;
                event.timestamp = sdlEvent.common.timestamp;

                WindowData* w = getWindow(sdlEvent.window.windowID);
                event.window.window = w;
                windowState.events.push(event);

                if (w != nullptr)
                {
                    w->wasClosed = true;
                    w->events.push(event);
                }
            } break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            {
                Event event{};
                event.type = EventType_windowFocused;
                event.timestamp = sdlEvent.common.timestamp;

                WindowData* w = getWindow(sdlEvent.window.windowID);
                if (w != nullptr)
                {
                    w->isFocused = true;
                    w->wasFocusGained = true;
                    w->events.push(event);
                }
            } break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
            {
                Event event{};
                event.type = EventType_windowUnfocused;
                event.timestamp = sdlEvent.common.timestamp;

                WindowData* w = getWindow(sdlEvent.window.windowID);
                if (w != nullptr)
                {
                    w->isFocused = false;
                    w->wasFocusLost = true;
                    w->events.push(event);
                }
            } break;
            case SDL_EVENT_WINDOW_MOVED:
            {
                i32 x = sdlEvent.window.data1;
                i32 y = sdlEvent.window.data2;

                Event event{};
                event.type = EventType_windowMoved;
                event.timestamp = sdlEvent.common.timestamp;

                WindowData* w = getWindow(sdlEvent.window.windowID);
                event.window.window = w;
                event.window.x = x;
                event.window.y = y;
                windowState.events.push(event);

                if (w != nullptr)
                {
                    w->x = x;
                    w->y = y;
                    w->wasMoved = true;
                    w->events.push(event);
                }
            } break;
            case SDL_EVENT_WINDOW_RESIZED:
            {
                u32 width = static_cast<u32>(sdlEvent.window.data1);
                u32 height = static_cast<u32>(sdlEvent.window.data2);

                Event event{};
                event.type = EventType_windowResized;
                event.timestamp = sdlEvent.common.timestamp;

                WindowData* w = getWindow(sdlEvent.window.windowID);
                event.window.window = w;
                event.window.width = width;
                event.window.height = height;
                windowState.events.push(event);

                if (w != nullptr)
                {
                    w->swap.resize(width, height);
                    w->width = width;
                    w->height = height;
                    w->wasResized = true;
                    w->events.push(event);
                }
            } break;
            case SDL_EVENT_WINDOW_MAXIMIZED:
            {
                Event event{};
                event.type = EventType_windowMaximized;
                event.timestamp = sdlEvent.common.timestamp;

                WindowData* w = getWindow(sdlEvent.window.windowID);
                event.window.window = w;
                windowState.events.push(event);

                if (w != nullptr)
                {
                    w->wasMaximized = true;
                    w->events.push(event);
                }
            } break;
            case SDL_EVENT_WINDOW_MINIMIZED:
            {
                Event event{};
                event.type = EventType_windowMinimized;
                event.timestamp = sdlEvent.common.timestamp;

                WindowData* w = getWindow(sdlEvent.window.windowID);
                event.window.window = w;
                windowState.events.push(event);

                if (w != nullptr)
                {
                    w->wasMinimized = true;
                    w->events.push(event);
                }
            } break;
            case SDL_EVENT_WINDOW_RESTORED:
            {
                Event event{};
                event.type = EventType_windowRestored;
                event.timestamp = sdlEvent.common.timestamp;

                WindowData* w = getWindow(sdlEvent.window.windowID);
                event.window.window = w;
                windowState.events.push(event);

                if (w != nullptr)
                {
                    w->wasRestored = true;
                    w->events.push(event);
                }
            } break;
            case SDL_EVENT_GAMEPAD_ADDED:
            {
                Event event{};
                event.type = EventType_gamepadConnected;
                event.timestamp = sdlEvent.common.timestamp;

                u32 idx = findGamepadIndex(sdlEvent.gdevice.which);
                if (idx >= maxGamepads)
                    break;
                event.gamepad.idx = idx;
                windowState.events.push(event);

                for (u32 i = 0; i < maxGamepads; i++)
                {
                    if (windowState.gamepads[i] == nullptr)
                    {
                        SDL_Gamepad* gp = SDL_OpenGamepad(sdlEvent.gdevice.which);
                        if (gp != nullptr)
                        {
                            windowState.gamepads[i] = gp;
                            windowState.gamepadIds.add(sdlEvent.gdevice.which, i);
                        }
                        break;
                    }
                }
            } break;
            case SDL_EVENT_GAMEPAD_REMOVED:
            {
                Event event{};
                event.type = EventType_gamepadDisconnected;
                event.timestamp = sdlEvent.common.timestamp;

                u32 idx = findGamepadIndex(sdlEvent.gdevice.which);
                event.gamepad.idx = idx;
                windowState.events.push(event);

                SDL_CloseGamepad(windowState.gamepads[idx]);
                windowState.gamepads[idx] = nullptr;
                windowState.gamepadIds.remove(sdlEvent.gdevice.which);
            } break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            {
                u32 idx = findGamepadIndex(sdlEvent.gbutton.which);
                if (idx < maxGamepads)
                {
                    Event event{};
                    event.type = EventType_gamepadPress;
                    event.timestamp = sdlEvent.common.timestamp;
                    event.gamepad.idx = idx;
                    event.gamepad.button = sdlGamepadButtonToHgButton(sdlEvent.gbutton.button);
                    windowState.events.push(event);

                    windowState.isGamepadButtonDown[idx][event.gamepad.button] = true;
                }
            } break;
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
            {
                u32 idx = findGamepadIndex(sdlEvent.gbutton.which);
                if (idx < maxGamepads)
                {
                    Event event{};
                    event.type = EventType_gamepadRelease;
                    event.timestamp = sdlEvent.common.timestamp;
                    event.gamepad.idx = idx;
                    event.gamepad.button = sdlGamepadButtonToHgButton(sdlEvent.gbutton.button);
                    windowState.events.push(event);

                    windowState.isGamepadButtonDown[idx][event.gamepad.button] = false;
                }
            } break;
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            {
                constexpr EventType types[] = {
                    EventType_gamepadLeftStick,
                    EventType_gamepadLeftStick,
                    EventType_gamepadRightStick,
                    EventType_gamepadRightStick,
                    EventType_gamepadLeftTrigger,
                    EventType_gamepadRightTrigger,
                };

                u32 idx = findGamepadIndex(sdlEvent.gaxis.which);
                if (idx < maxGamepads)
                {
                    Event event{};
                    event.type = types[sdlEvent.gaxis.axis];
                    event.timestamp = sdlEvent.common.timestamp;
                    event.gamepad.idx = idx;

                    if (idx < maxGamepads && sdlEvent.gaxis.axis < 6)
                        windowState.gamepadAxes[idx][sdlEvent.gaxis.axis] = sdlEvent.gaxis.value;

                    switch (event.type)
                    {
                        case EventType_gamepadLeftStick:
                            event.gamepad.stick = gamepadLeftStick(idx);
                            break;
                        case EventType_gamepadRightStick:
                            event.gamepad.stick = gamepadRightStick(idx);
                            break;
                        case EventType_gamepadLeftTrigger:
                            event.gamepad.trigger = gamepadLeftTrigger(idx);
                            break;
                        case EventType_gamepadRightTrigger:
                            event.gamepad.trigger = gamepadRightTrigger(idx);
                            break;
                        default:
                            break;
                    }

                    windowState.events.push(event);
                }
            } break;
        }
    }
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
    if (windowState.windows.count == 1)
    {
        Vec2 ret;
        windowState.windows.forEach([&](SDL_WindowID, WindowData* wd)
        {
            ret = wd->mouse;
        });
        return ret;
    }
    Vec2 pos;
    SDL_GetGlobalMouseState(&pos.x, &pos.y);
    return pos;
}

Vec2 mouseDelta()
{
    if (windowState.windows.count == 1)
    {
        Vec2 ret = windowState.mouseDelta;
        windowState.windows.forEach([&](SDL_WindowID, WindowData* wd)
        {
            ret = ret / static_cast<f32>(wd->height);
        });
        return ret;
    }
    return windowState.mouseDelta;
}

Vec2 windowMousePos(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    if (wd != nullptr)
        return wd->mouse;

    Vec2 pos;
    SDL_GetGlobalMouseState(&pos.x, &pos.y);
    return pos;
}

Vec2 windowMouseDelta(void* data)
{
    WindowData* wd = static_cast<WindowData*>(data);
    if (wd != nullptr)
        return windowState.mouseDelta / static_cast<f32>(wd->height);

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
        if (windowState.gamepads[i] != nullptr)
            count++;
    }
    return count;
}

bool isGamepadConnected(u32 gamepad)
{
    return gamepad < maxGamepads && windowState.gamepads[gamepad] != nullptr;
}

bool isGamepadButtonDown(u32 gamepad, Button key)
{
    if (gamepad < maxGamepads)
        return windowState.isGamepadButtonDown[gamepad][key];
    return false;
}

bool wasGamepadButtonPressed(u32 gamepad, GamepadButton key)
{
    if (gamepad < maxGamepads)
        return !windowState.wasGamepadButtonDown[gamepad][key] && windowState.isGamepadButtonDown[gamepad][key];
    return false;
}

bool wasGamepadButtonReleased(u32 gamepad, GamepadButton key)
{
    if (gamepad < maxGamepads)
        return windowState.wasGamepadButtonDown[gamepad][key] && !windowState.isGamepadButtonDown[gamepad][key];
    return false;
}

Vec2 gamepadLeftStick(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad] != nullptr)
        return Vec2{
            static_cast<f32>(windowState.gamepadAxes[gamepad][0]) / 32767.0f,
            static_cast<f32>(windowState.gamepadAxes[gamepad][1]) / 32767.0f,
        };
    else
        return Vec2{0};
}

Vec2 gamepadRightStick(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad] != nullptr)
        return Vec2{
            static_cast<f32>(windowState.gamepadAxes[gamepad][2]) / 32767.0f,
            static_cast<f32>(windowState.gamepadAxes[gamepad][3]) / 32767.0f,
        };
    else
        return Vec2{0};
}

f32 gamepadLeftTrigger(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad] != nullptr)
        return static_cast<f32>(windowState.gamepadAxes[gamepad][4]) / 32767.0f;
    else
        return 0;
}

f32 gamepadRightTrigger(u32 gamepad)
{
    if (gamepad < maxGamepads && windowState.gamepads[gamepad] != nullptr)
        return static_cast<f32>(windowState.gamepadAxes[gamepad][5]) / 32767.0f;
    else
        return 0;
}

Window windowCreate(const WindowConfig& config)
{
    Window window{};
    window.data = new (heapAlloc(sizeof(WindowData), alignof(WindowData))) WindowData{};

    WindowData* wd = static_cast<WindowData*>(window.data);

    ArenaScope scratch = getScratch();

    wd->sdlWindow = SDL_CreateWindow(
        "Hurdy Gurdy",
        800, 600,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (wd->sdlWindow == nullptr)
        HG_PANIC("SDL could not create window: %s\n", SDL_GetError());

    windowState.windows.add(SDL_GetWindowID(wd->sdlWindow), wd);

    i32 px, py;
    SDL_GetWindowPosition(wd->sdlWindow, &px, &py);
    wd->x = px;
    wd->y = py;

    u32 w, h;
    SDL_GetWindowSize(wd->sdlWindow, reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h));
    wd->width = w;
    wd->height = h;

    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(
        wd->sdlWindow,
        static_cast<VkInstance>(internal::getVulkanInstance()),
        nullptr,
        &surface))
        HG_PANIC("SDL could not create Vulkan surface: %s\n", SDL_GetError());

    wd->swap = GpuSwapchain::create(surface, w, h, config.preferredPresentMode, config.imageUsage);

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
    SDL_SetWindowTitle(static_cast<WindowData*>(data)->sdlWindow, cString(getScratch(), title));
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
    return SDL_GetMouseFocus() == static_cast<WindowData*>(data)->sdlWindow;
}

bool windowWasFocusGained(void* data)
{
    return static_cast<WindowData*>(data)->wasFocusGained;
}

bool windowWasFocusLost(void* data)
{
    return static_cast<WindowData*>(data)->wasFocusLost;
}

bool windowWasMoved(void* data)
{
    return static_cast<WindowData*>(data)->wasMoved;
}

void windowGetPos(void* data, i32* x, i32* y)
{
    WindowData* wd = static_cast<WindowData*>(data);
    if (x != nullptr)
        *x = wd != nullptr ? wd->x : 0;
    if (y != nullptr)
        *y = wd != nullptr ? wd->y : 0;
}

void windowSetPos(void* data, i32 x, i32 y)
{
    WindowData* wd = static_cast<WindowData*>(data);
    SDL_SetWindowPosition(wd->sdlWindow, x, y);
    wd->x = x;
    wd->y = y;
}

void windowSetResizable(void* data, bool set)
{
    SDL_SetWindowResizable(static_cast<WindowData*>(data)->sdlWindow, set);
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

void windowSetSize(void* data, u32 width, u32 height)
{
    WindowData* wd = static_cast<WindowData*>(data);
    SDL_SetWindowSize(wd->sdlWindow, static_cast<int>(width), static_cast<int>(height));
    wd->swap.resize(width, height);
    wd->width = width;
    wd->height = height;
}

bool windowIsMaximized(void* data)
{
    return (SDL_GetWindowFlags(static_cast<WindowData*>(data)->sdlWindow) & SDL_WINDOW_MAXIMIZED) != 0;
}

bool windowWasMaximized(void* data)
{
    return static_cast<WindowData*>(data)->wasMaximized;
}

void windowMaximize(void* data)
{
    SDL_MaximizeWindow(static_cast<WindowData*>(data)->sdlWindow);
}

bool windowIsMinimized(void* data)
{
    return (SDL_GetWindowFlags(static_cast<WindowData*>(data)->sdlWindow) & SDL_WINDOW_MINIMIZED) != 0;
}

bool windowWasMinimized(void* data)
{
    return static_cast<WindowData*>(data)->wasMinimized;
}

void windowMinimize(void* data)
{
    SDL_MinimizeWindow(static_cast<WindowData*>(data)->sdlWindow);
}

bool windowWasRestored(void* data)
{
    return static_cast<WindowData*>(data)->wasRestored;
}

bool windowwasMadeFullscreen(void* data)
{
    return static_cast<WindowData*>(data)->wasFullscreened;
}

void windowRestore(void* data)
{
    SDL_RestoreWindow(static_cast<WindowData*>(data)->sdlWindow);
}

bool windowIsFullscreen(void* data)
{
    return (SDL_GetWindowFlags(static_cast<WindowData*>(data)->sdlWindow) & SDL_WINDOW_FULLSCREEN) != 0;
}

void windowSetFullscreen(void* data, bool set)
{
    WindowData* wd = static_cast<WindowData*>(data);
    SDL_SetWindowFullscreen(wd->sdlWindow, set ? SDL_WINDOW_FULLSCREEN : 0);

    u32 w, h;
    wd->swap.size(&w, &h);
    wd->swap.resize(w, h);
}

StringView getClipboardText()
{
    return Span<const char>(windowState.clipboard);
}

void setClipboardText(StringView text)
{
    SDL_SetClipboardText(cString(getScratch(), text));
    windowState.clipboard.resize(text.length);
    memcpy(windowState.clipboard.vals, text.chars, text.length);
}

void openURL(StringView url)
{
    SDL_OpenURL(cString(getScratch(), url));
}

} // namespace hg::sdl
