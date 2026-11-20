#include "sdl_audio.hpp"

#include "sdl_window.hpp"

#include "internal.hpp"

#include "hg/utility.hpp"
#include "hg/memory.hpp"
#include "hg/hash.hpp"
#include "hg/utility.hpp"

#include <span>
#include <string_view>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vulkan/vulkan.h>

namespace hg {

static Button sdlKeycodeToHgButton(u32 key)
{
    switch (key)
    {
        case SDLK_0: return Button::b0;
        case SDLK_1: return Button::b1;
        case SDLK_2: return Button::b2;
        case SDLK_3: return Button::b3;
        case SDLK_4: return Button::b4;
        case SDLK_5: return Button::b5;
        case SDLK_6: return Button::b6;
        case SDLK_7: return Button::b7;
        case SDLK_8: return Button::b8;
        case SDLK_9: return Button::b9;
        case SDLK_Q: return Button::q;
        case SDLK_W: return Button::w;
        case SDLK_E: return Button::e;
        case SDLK_R: return Button::r;
        case SDLK_T: return Button::t;
        case SDLK_Y: return Button::y;
        case SDLK_U: return Button::u;
        case SDLK_I: return Button::i;
        case SDLK_O: return Button::o;
        case SDLK_P: return Button::p;
        case SDLK_A: return Button::a;
        case SDLK_S: return Button::s;
        case SDLK_D: return Button::d;
        case SDLK_F: return Button::f;
        case SDLK_G: return Button::g;
        case SDLK_H: return Button::h;
        case SDLK_J: return Button::j;
        case SDLK_K: return Button::k;
        case SDLK_L: return Button::l;
        case SDLK_Z: return Button::z;
        case SDLK_X: return Button::x;
        case SDLK_C: return Button::c;
        case SDLK_V: return Button::v;
        case SDLK_B: return Button::b;
        case SDLK_N: return Button::n;
        case SDLK_M: return Button::m;
        case SDLK_SEMICOLON: return Button::semicolon;
        case SDLK_COLON: return Button::semicolon;
        case SDLK_APOSTROPHE: return Button::apostrophe;
        case SDLK_DBLAPOSTROPHE: return Button::apostrophe;
        case SDLK_COMMA: return Button::comma;
        case SDLK_PERIOD: return Button::period;
        case SDLK_QUESTION: return Button::slash;
        case SDLK_GRAVE: return Button::grave;
        case SDLK_TILDE: return Button::grave;
        case SDLK_EXCLAIM: return Button::b1;
        case SDLK_AT: return Button::b2;
        case SDLK_HASH: return Button::b3;
        case SDLK_DOLLAR: return Button::b4;
        case SDLK_PERCENT: return Button::b5;
        case SDLK_CARET: return Button::b6;
        case SDLK_AMPERSAND: return Button::b7;
        case SDLK_ASTERISK: return Button::b8;
        case SDLK_LEFTPAREN: return Button::b9;
        case SDLK_RIGHTPAREN: return Button::b0;
        case SDLK_LEFTBRACKET: return Button::lbracket;
        case SDLK_RIGHTBRACKET: return Button::rbracket;
        case SDLK_LEFTBRACE: return Button::lbracket;
        case SDLK_RIGHTBRACE: return Button::rbracket;
        case SDLK_EQUALS: return Button::equal;
        case SDLK_LESS: return Button::comma;
        case SDLK_GREATER: return Button::period;
        case SDLK_PLUS: return Button::equal;
        case SDLK_MINUS: return Button::minus;
        case SDLK_SLASH: return Button::slash;
        case SDLK_BACKSLASH: return Button::backslash;
        case SDLK_UNDERSCORE: return Button::minus;
        case SDLK_PIPE: return Button::backslash;
        case SDLK_UP: return Button::up;
        case SDLK_DOWN: return Button::down;
        case SDLK_LEFT: return Button::left;
        case SDLK_RIGHT: return Button::right;
        case SDLK_ESCAPE: return Button::escape;
        case SDLK_SPACE: return Button::space;
        case SDLK_RETURN: return Button::enter;
        case SDLK_BACKSPACE: return Button::backspace;
        case SDLK_DELETE: return Button::kdelete;
        case SDLK_INSERT: return Button::insert;
        case SDLK_TAB: return Button::tab;
        case SDLK_HOME: return Button::home;
        case SDLK_END: return Button::end;
        case SDLK_PAGEUP: return Button::pageup;
        case SDLK_PAGEDOWN: return Button::pagedown;
        case SDLK_F1: return Button::f1;
        case SDLK_F2: return Button::f2;
        case SDLK_F3: return Button::f3;
        case SDLK_F4: return Button::f4;
        case SDLK_F5: return Button::f5;
        case SDLK_F6: return Button::f6;
        case SDLK_F7: return Button::f7;
        case SDLK_F8: return Button::f8;
        case SDLK_F9: return Button::f9;
        case SDLK_F10: return Button::f10;
        case SDLK_F11: return Button::f11;
        case SDLK_F12: return Button::f12;
        case SDLK_PRINTSCREEN: return Button::printscreen;
        case SDLK_APPLICATION: return Button::context;
        case SDLK_KP_0: return Button::numpad0;
        case SDLK_KP_1: return Button::numpad1;
        case SDLK_KP_2: return Button::numpad2;
        case SDLK_KP_3: return Button::numpad3;
        case SDLK_KP_4: return Button::numpad4;
        case SDLK_KP_5: return Button::numpad5;
        case SDLK_KP_6: return Button::numpad6;
        case SDLK_KP_7: return Button::numpad7;
        case SDLK_KP_8: return Button::numpad8;
        case SDLK_KP_9: return Button::numpad9;
        case SDLK_KP_PERIOD: return Button::numpaddecimal;
        case SDLK_KP_DIVIDE: return Button::numpaddiv;
        case SDLK_KP_MULTIPLY: return Button::numpadmul;
        case SDLK_KP_MINUS: return Button::numpadminus;
        case SDLK_KP_PLUS: return Button::numpadplus;
        case SDLK_KP_ENTER: return Button::numpadenter;
        case SDLK_LSHIFT: return Button::lshift;
        case SDLK_RSHIFT: return Button::rshift;
        case SDLK_LCTRL: return Button::lctrl;
        case SDLK_RCTRL: return Button::rctrl;
        case SDLK_LALT: return Button::lalt;
        case SDLK_RALT: return Button::ralt;
        case SDLK_LGUI: return Button::lsuper;
        case SDLK_RGUI: return Button::rsuper;
        case SDLK_CAPSLOCK: return Button::capslock;
        case SDLK_NUMLOCKCLEAR: return Button::numlock;
        case SDLK_SCROLLLOCK: return Button::scrolllock;
        case SDLK_PAUSE: return Button::pause;
    }
    return Button::none;
}

static Button sdlButtonToHgButton(u32 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT: return Button::mouse1;
        case SDL_BUTTON_RIGHT: return Button::mouse2;
        case SDL_BUTTON_MIDDLE: return Button::mouse3;
        case SDL_BUTTON_X1: return Button::mouse4;
        case SDL_BUTTON_X2: return Button::mouse5;
    }
    return Button::none;
}

WindowSdl3* InputDeviceSdl3::getWindow(SDL_WindowID id)
{
    WindowSdl3** data = windows.get(id);
    if (data == nullptr)
        return nullptr;
    return *data;
}

std::unique_ptr<InputDeviceSdl3> InputDeviceSdl3::create()
{
    if (!SDL_InitSubSystem(
        SDL_INIT_VIDEO |
        SDL_INIT_EVENTS))
    {
        setError(SDL_GetError());
        return nullptr;
    }

    std::unique_ptr<InputDeviceSdl3> device = std::make_unique<InputDeviceSdl3>();

    int count = 0;
    SDL_DisplayID* ids = SDL_GetDisplays(&count);
    if (ids == nullptr)
    {
        setError(SDL_GetError());
        SDL_QuitSubSystem(
            SDL_INIT_VIDEO |
            SDL_INIT_EVENTS);
        return nullptr;
    }
    HG_DEFER(SDL_free(ids));

    device->displays.resize(assertCast<u64>(count));
    for (u32 i = 0; i < device->displays.size(); i++)
    {
        DisplayInfo& info = device->displays[i];

        SDL_DisplayID displayId = ids[i];
        SDL_Rect r;
        SDL_GetDisplayBounds(displayId, &r);
        info.posX = assertCast<i32>(r.x);
        info.posY = assertCast<i32>(r.y);
        info.sizeW = assertCast<u32>(r.w);
        info.sizeH = assertCast<u32>(r.h);

        if (SDL_GetDisplayUsableBounds(displayId, &r) && r.w > 0 && r.h > 0)
        {
            info.workPosX = assertCast<i32>(r.x);
            info.workPosY = assertCast<i32>(r.y);
            info.workSizeW = assertCast<u32>(r.w);
            info.workSizeH = assertCast<u32>(r.h);
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

    return device;
}

InputDeviceSdl3::~InputDeviceSdl3()
{
    for (u32 i = 0; i < static_cast<u32>(CursorType::count); i++)
    {
        if (cursors[i] != nullptr)
            SDL_DestroyCursor(cursors[i]);
    }
    currentCursor = nullptr;

    SDL_QuitSubSystem(
        SDL_INIT_VIDEO |
        SDL_INIT_EVENTS);
}

std::span<std::string_view> InputDeviceSdl3::getVulkanExtensions()
{
    u32 extCount = 0;
    const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&extCount);
    if (exts == nullptr)
        panic("SDL could not get Vulkan instance extensions: {}\n", SDL_GetError());

    static std::vector<std::string_view> extBuffer{};
    extBuffer.resize(extCount);
    for (u32 i = 0; i < extCount; ++i)
    {
        extBuffer[i] = exts[i];
    }

    return extBuffer;
}

std::span<DisplayInfo> InputDeviceSdl3::displayInfo()
{
    return displays;
}

void InputDeviceSdl3::processEvents()
{
    events.clear();

    std::memcpy(wasKeyDown, isKeyDown, sizeof(isKeyDown));
    mouseDeltaAccum = {};
    wheelDeltaAccum = {};

    windows.forEach([&](SDL_WindowID, WindowSdl3* data)
    {
        data->eventQueue.clear();
        data->focusGained = false;
        data->focusLost = false;
        data->resized = false;
    });

    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent))
    {
        switch (sdlEvent.type)
        {
            case SDL_EVENT_QUIT:
            {
                quit = true;

                Event event{};
                event.type = EventType::quit;
                event.timestamp = sdlEvent.common.timestamp;
                events.push_back(event);
            } break;
            case SDL_EVENT_TEXT_INPUT:
            {
                Event event{};
                event.type = EventType::text;
                event.timestamp = sdlEvent.common.timestamp;
                std::memset(event.text, 0, sizeof(event.text));
                strncpy(event.text, sdlEvent.text.text, sizeof(event.text) - 1);
                events.push_back(event);
            } break;
            case SDL_EVENT_KEY_DOWN:
            {
                Event event{};
                event.type = EventType::keyPress;
                event.timestamp = sdlEvent.common.timestamp;
                event.button = sdlKeycodeToHgButton(sdlEvent.key.key);
                events.push_back(event);

                isKeyDown[static_cast<u32>(event.button)] = true;
            } break;
            case SDL_EVENT_KEY_UP:
            {
                Event event{};
                event.type = EventType::keyRelease;
                event.timestamp = sdlEvent.common.timestamp;
                event.button = sdlKeycodeToHgButton(sdlEvent.key.key);
                events.push_back(event);

                isKeyDown[static_cast<u32>(event.button)] = false;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                Event event{};
                event.type = EventType::keyPress;
                event.timestamp = sdlEvent.common.timestamp;
                event.button = sdlButtonToHgButton(sdlEvent.button.button);
                events.push_back(event);

                isKeyDown[static_cast<u32>(event.button)] = true;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                Event event{};
                event.type = EventType::keyRelease;
                event.timestamp = sdlEvent.common.timestamp;
                event.button = sdlButtonToHgButton(sdlEvent.button.button);
                events.push_back(event);

                isKeyDown[static_cast<u32>(event.button)] = false;
            } break;
            case SDL_EVENT_MOUSE_MOTION:
            {
                f32 gmx, gmy;
                SDL_GetGlobalMouseState(&gmx, &gmy);

                Event event{};
                event.type = EventType::mouseMoved;
                event.timestamp = sdlEvent.common.timestamp;
                event.mouse.delta = {sdlEvent.motion.xrel, sdlEvent.motion.yrel};
                event.mouse.pos = {sdlEvent.motion.x, sdlEvent.motion.y};
                event.mouse.globalPos = {gmx, gmy};
                events.push_back(event);

                WindowSdl3* w = getWindow(sdlEvent.motion.windowID);
                if (w != nullptr)
                    w->mouse = event.mouse.pos;

                mouseDeltaAccum += event.mouse.delta;
            } break;
            case SDL_EVENT_MOUSE_WHEEL:
            {
                Event event{};
                event.type = EventType::wheelMoved;
                event.timestamp = sdlEvent.common.timestamp;
                event.wheel.delta = {sdlEvent.wheel.x, sdlEvent.wheel.y};
                events.push_back(event);

                wheelDeltaAccum += event.wheel.delta;
            } break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                Event event{};
                event.type = EventType::windowClosed;
                event.timestamp = sdlEvent.common.timestamp;

                WindowSdl3* w = getWindow(sdlEvent.window.windowID);
                event.window.window = w;
                events.push_back(event);

                if (w != nullptr)
                {
                    w->closed = true;
                    w->eventQueue.push_back(event);
                }
            } break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            {
                Event event{};
                event.type = EventType::windowFocused;
                event.timestamp = sdlEvent.common.timestamp;

                WindowSdl3* w = getWindow(sdlEvent.window.windowID);
                if (w != nullptr)
                {
                    w->focused = true;
                    w->focusGained = true;
                    activeWindow = w;
                    w->eventQueue.push_back(event);
                }
            } break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
            {
                Event event{};
                event.type = EventType::windowUnfocused;
                event.timestamp = sdlEvent.common.timestamp;

                WindowSdl3* w = getWindow(sdlEvent.window.windowID);
                if (w != nullptr)
                {
                    w->focused = false;
                    w->focusLost = true;
                    if (activeWindow == w)
                        activeWindow = nullptr;
                    w->eventQueue.push_back(event);
                }
            } break;
            case SDL_EVENT_WINDOW_RESIZED:
            {
                u32 width = assertCast<u32>(sdlEvent.window.data1);
                u32 height = assertCast<u32>(sdlEvent.window.data2);

                Event event{};
                event.type = EventType::windowResized;
                event.timestamp = sdlEvent.common.timestamp;

                WindowSdl3* w = getWindow(sdlEvent.window.windowID);
                event.window.window = w;
                event.window.width = width;
                event.window.height = height;
                events.push_back(event);

                if (w != nullptr)
                {
                    w->swap->resize(width, height);
                    w->winWidth = width;
                    w->winHeight = height;
                    w->resized = true;
                    w->eventQueue.push_back(event);
                }
            } break;
        }
    }
}

std::span<const Event> InputDeviceSdl3::getEvents()
{
    return events;
}

bool InputDeviceSdl3::wasQuit()
{
    return quit;
}

std::unique_ptr<Window> InputDeviceSdl3::createWindow(Gpu* gpu, const WindowConfig& config)
{
    std::unique_ptr<WindowSdl3> win = std::make_unique<WindowSdl3>(this);

    u32 width = config.width != 0 ? config.width : 800;
    u32 height = config.height != 0 ? config.height : 600;
    std::string title = config.title.length() != 0 ? std::string{config.title} : "Hurdy Gurdy";

    win->sdlWindow = SDL_CreateWindow(
        title.c_str(),
        assertCast<int>(width),
        assertCast<int>(height),
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (win->sdlWindow == nullptr)
        panic("SDL could not create window: {}\n", SDL_GetError());

    windows.add(SDL_GetWindowID(win->sdlWindow), win.get());

    u32 w, h;
    SDL_GetWindowSize(win->sdlWindow, reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h));
    win->winWidth = w;
    win->winHeight = h;

    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(
        win->sdlWindow,
        static_cast<VkInstance>(internal::getVkInstance(gpu)),
        nullptr,
        &surface))
        panic("SDL could not create Vulkan surface: {}\n", SDL_GetError());

    win->swap = gpu->createSwapchain(surface, w, h, config.preferredPresentMode, config.imageUsage);

    return win;
}

bool InputDeviceSdl3::isButtonDown(Button button)
{
    return isKeyDown[static_cast<u32>(button)];
}

bool InputDeviceSdl3::wasButtonPressed(Button button)
{
    return !wasKeyDown[static_cast<u32>(button)] && isKeyDown[static_cast<u32>(button)];
}

bool InputDeviceSdl3::wasButtonReleased(Button button)
{
    return wasKeyDown[static_cast<u32>(button)] && !isKeyDown[static_cast<u32>(button)];
}

Vec2 InputDeviceSdl3::mousePos()
{
    if (activeWindow != nullptr)
        return activeWindow->mouse;

    if (windows.size() == 1)
    {
        Vec2 ret;
        windows.forEach([&](SDL_WindowID, WindowSdl3* wd)
        {
            ret = wd->mouse;
        });
        return ret;
    }
    return {};
}

Vec2 InputDeviceSdl3::mouseDelta()
{
    if (activeWindow != nullptr)
        return mouseDeltaAccum / assertCast<f32>(activeWindow->winHeight);

    if (windows.size() == 1)
    {
        Vec2 ret = mouseDeltaAccum;
        windows.forEach([&](SDL_WindowID, WindowSdl3* wd)
        {
            ret = ret / assertCast<f32>(wd->winHeight);
        });
        return ret;
    }
    return mouseDeltaAccum;
}

Vec2 InputDeviceSdl3::wheelDelta()
{
    return wheelDeltaAccum;
}

void InputDeviceSdl3::setCursor(CursorType type)
{
    SDL_Cursor*& cursor = cursors[static_cast<u32>(type)];
    if (cursor == nullptr)
    {
        SDL_SystemCursor systemCursor;
        switch (type)
        {
            case CursorType::arrow:
                systemCursor = SDL_SYSTEM_CURSOR_DEFAULT;
                break;
            case CursorType::textInput:
                systemCursor = SDL_SYSTEM_CURSOR_TEXT;
                break;
            case CursorType::resizeAll:
                systemCursor = SDL_SYSTEM_CURSOR_MOVE;
                break;
            case CursorType::resizeNS:
                systemCursor = SDL_SYSTEM_CURSOR_NS_RESIZE;
                break;
            case CursorType::resizeEW:
                systemCursor = SDL_SYSTEM_CURSOR_EW_RESIZE;
                break;
            case CursorType::resizeNESW:
                systemCursor = SDL_SYSTEM_CURSOR_NESW_RESIZE;
                break;
            case CursorType::resizeNWSE:
                systemCursor = SDL_SYSTEM_CURSOR_NWSE_RESIZE;
                break;
            case CursorType::hand:
                systemCursor = SDL_SYSTEM_CURSOR_POINTER;
                break;
            case CursorType::wait:
                systemCursor = SDL_SYSTEM_CURSOR_WAIT;
                break;
            case CursorType::progress:
                systemCursor = SDL_SYSTEM_CURSOR_PROGRESS;
                break;
            case CursorType::notAllowed:
                systemCursor = SDL_SYSTEM_CURSOR_NOT_ALLOWED;
                break;
            default:
                systemCursor = SDL_SYSTEM_CURSOR_DEFAULT;
                break;
        }
        cursor = SDL_CreateSystemCursor(systemCursor);
    }

    if (currentCursor != cursor)
    {
        SDL_SetCursor(cursor);
        currentCursor = cursor;
    }
}

void InputDeviceSdl3::showCursor(bool show)
{
    if (show)
        SDL_ShowCursor();
    else
        SDL_HideCursor();
}

WindowSdl3::~WindowSdl3() noexcept
{
    if (sdlWindow != nullptr)
    {
        static_cast<InputDeviceSdl3*>(device)->windows.remove(SDL_GetWindowID(sdlWindow));
        SDL_DestroyWindow(sdlWindow);
    }
}

GpuSwapchain* WindowSdl3::swapchain() const
{
    return swap.get();
}

void WindowSdl3::setTitle(std::string_view title)
{
    SDL_SetWindowTitle(sdlWindow, std::string{title}.c_str());
}

std::span<const Event> WindowSdl3::getEvents() const
{
    return eventQueue;
}

bool WindowSdl3::wasClosed() const
{
    return closed;
}

bool WindowSdl3::isFocused() const
{
    return SDL_GetMouseFocus() == sdlWindow;
}

bool WindowSdl3::wasFocusGained() const
{
    return focusGained;
}

bool WindowSdl3::wasFocusLost() const
{
    return focusLost;
}

bool WindowSdl3::wasResized() const
{
    return resized;
}

u32 WindowSdl3::width() const
{
    return winWidth;
}

u32 WindowSdl3::height() const
{
    return winHeight;
}

void WindowSdl3::setFullscreen(bool set)
{
    SDL_SetWindowFullscreen(sdlWindow, set ? SDL_WINDOW_FULLSCREEN : 0);

    swap->resize(swap->width(), swap->height());
}

Vec2 WindowSdl3::mousePos() const
{
    return mouse;
}

Vec2 WindowSdl3::mouseDelta() const
{
    return static_cast<InputDeviceSdl3*>(device)->mouseDeltaAccum / assertCast<f32>(winHeight);
}

std::unique_ptr<AudioDeviceSdl3> AudioDeviceSdl3::create()
{
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        setError("SDL could not initialize audio subsystem: {}", SDL_GetError());
        return nullptr;
    }

    std::unique_ptr<AudioDeviceSdl3> audio = std::make_unique<AudioDeviceSdl3>();

    audio->device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (audio->device == 0)
    {
        setError("SDL could not open audio device: {}", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return nullptr;
    }

    audio->stream = SDL_CreateAudioStream(nullptr, nullptr);
    if (audio->stream == nullptr)
    {
        setError("SDL could not open audio stream: {}", SDL_GetError());
        SDL_CloseAudioDevice(audio->device);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return nullptr;
    }

    if (!SDL_BindAudioStream(audio->device, audio->stream))
    {
        setError("SDL could not bind audio stream to device: {}", SDL_GetError());
        SDL_DestroyAudioStream(audio->stream);
        SDL_CloseAudioDevice(audio->device);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return nullptr;
    }

    return audio;
}

AudioDeviceSdl3::~AudioDeviceSdl3()
{
    if (stream != nullptr)
        SDL_DestroyAudioStream(stream);
    if (device != 0)
        SDL_CloseAudioDevice(device);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void AudioDeviceSdl3::sdlCallback(
    void* userData,
    SDL_AudioStream* stream,
    int additionalAmount,
    int totalAmount)
{
    (void)totalAmount;

    ArenaScope scratch = getScratch();

    AudioDeviceSdl3* audio = static_cast<AudioDeviceSdl3*>(userData);

    void* buf = scratch.alloc(assertCast<u64>(additionalAmount), alignof(f32));
    std::memset(buf, 0, assertCast<u64>(additionalAmount));

    if (audio->callback != nullptr)
    {
        audio->callback(
            {static_cast<f32*>(buf), assertCast<u64>(additionalAmount) / sizeof(f32)},
            audio->callbackConfig);
    }

    if (!SDL_PutAudioStreamData(stream, buf, additionalAmount))
        panic("SDL could not push audio stream data: {}\n", SDL_GetError());
}

void AudioDeviceSdl3::setCallback(AudioCallback callbackVal, const AudioConfig& preferredConfig)
{
    AudioConfig config = preferredConfig;
    if (config.sampleRate == 0)
        config.sampleRate = 48000;
    if (config.channels == 0)
        config.channels = 2;

    SDL_AudioSpec audioSpec{};
    audioSpec.format = SDL_AUDIO_F32;
    audioSpec.freq = assertCast<int>(config.sampleRate);
    audioSpec.channels = assertCast<int>(config.channels);

    if (!SDL_SetAudioStreamFormat(stream, &audioSpec, nullptr))
        panic("SDL could not set audio stream format: {}\n", SDL_GetError());

    this->callback = std::move(callbackVal);
    callbackConfig = config;

    if (!SDL_SetAudioStreamGetCallback(stream, sdlCallback, this))
        panic("SDL could not set audio stream callback: {}\n", SDL_GetError());
}

void AudioDeviceSdl3::unsetCallback()
{
    callback = {};
    callbackConfig = {};
}

} // namespace hg
