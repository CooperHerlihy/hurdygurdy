#include "hurdygurdy.hpp"

#include "hurdygurdy_internal.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "imgui_impl_sdl3.h"

void hgInternalCreateWindowSwapchain(HgWindow* window, const HgCreateWindow* config);
void hgInternalDestroyWindowSwapchain(HgWindow* window);
void hgInternalResizeWindowSwapchain(HgWindow* window);

struct PlatformState {
    u32 windowCapacity = 0;
    u32 windowWidth = 0;
    HgWindow* windowPool = nullptr;
    u32* windowFreeList = nullptr;
    u32 windowNext = 0;

    HgHashMap<SDL_WindowID, HgWindow*> windows;

    bool wasQuit = false;
    bool isKeyDown[HgKey_count]{};
    f32 mouseDeltaX = 0.0f;
    f32 mouseDeltaY = 0.0f;

    bool imguiInitialized = false;
};

static PlatformState state;

void hgInitPlatform(HgArena* arena, u32 maxWindows, u32 maxEvents)
{
    SDL_Init(
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_GAMEPAD |
        SDL_INIT_EVENTS);

    state.windowCapacity = maxWindows;
    state.windowFreeList = hgAlloc<u32>(arena, maxWindows);
    state.windowWidth = (u32)hgAlign(sizeof(HgWindow) + sizeof(HgKeyEvent) * (maxEvents - 1), alignof(HgWindow));
    state.windowPool = (HgWindow*)hgAlloc(arena, state.windowWidth * maxWindows, alignof(HgWindow));

    for (u32 i = 0; i < maxWindows; ++i)
    {
        state.windowFreeList[i] = i + 1;
    }
    state.windowNext = 0;

    state.windows = state.windows.create(arena, maxWindows * 2);

    SDL_GetMouseState(&state.mouseDeltaX, &state.mouseDeltaY);
}

void hgDeinitPlatform()
{
    SDL_Quit();
}

u32 hgGetPlatformVulkanExtensions(HgArena* arena, HgStringView** extBuffer)
{
    u32 extCount;
    const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&extCount);

    *extBuffer = hgAlloc<HgStringView>(arena, extCount);
    for (u32 i = 0; i < extCount; ++i)
    {
        (*extBuffer)[i] = exts[i];
    }

    return extCount;
}


HgWindow* hgCreateWindow(const HgCreateWindow* config)
{
    u32 windowIdx = state.windowNext;
    hgAssert(windowIdx < state.windowCapacity);
    state.windowNext = state.windowFreeList[windowIdx];
    state.windowFreeList[windowIdx] = windowIdx;

    HgWindow* window = &state.windowPool[windowIdx];
    *window = {};

    const char* title = config->title != nullptr ? config->title : "Hurdy Gurdy";

    if (config->fullscreen)
    {
        int modeCount = 0;
        SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &modeCount);

        window->sdlWindow = SDL_CreateWindow(
            title,
            modes[0]->w,
            modes[0]->h,
            SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN);

        SDL_free(modes);
    } else {
        window->sdlWindow = SDL_CreateWindow(
            title,
            config->width,
            config->height,
            SDL_WINDOW_VULKAN | (config->fixedSize ? 0 : SDL_WINDOW_RESIZABLE));
    }

    bool success = SDL_Vulkan_CreateSurface(window->sdlWindow, hgVkInstance, nullptr, &window->surface);
    if (!success || window->surface == nullptr)
        hgError("Failed to create Vulkan surface: %s\n", SDL_GetError());

    SDL_WindowID windowID = SDL_GetWindowID(window->sdlWindow);
    state.windows.add(windowID, window);

    SDL_GetWindowSize(window->sdlWindow, (int*)&window->width, (int*)&window->height);
    hgInternalCreateWindowSwapchain(window, config);

    return window;
}

void hgDestroyWindow(HgWindow* window)
{
    u32 windowIdx = (u32)(window - state.windowPool);
    hgAssert(windowIdx < state.windowCapacity);
    hgAssert(state.windowFreeList[windowIdx] == windowIdx);

    hgInternalDestroyWindowSwapchain(window);

    vkDestroySurfaceKHR(hgVkInstance, window->surface, nullptr);
    SDL_DestroyWindow(window->sdlWindow);

    state.windowFreeList[windowIdx] = state.windowNext;
    state.windowNext = windowIdx;
}

static HgKey sdlKeycodeToHgKey(u32 key)
{
    switch (key)
    {
        case SDLK_0:
            return HgKey_k0;
        case SDLK_1:
            return HgKey_k1;
        case SDLK_2:
            return HgKey_k2;
        case SDLK_3:
            return HgKey_k3;
        case SDLK_4:
            return HgKey_k4;
        case SDLK_5:
            return HgKey_k5;
        case SDLK_6:
            return HgKey_k6;
        case SDLK_7:
            return HgKey_k7;
        case SDLK_8:
            return HgKey_k8;
        case SDLK_9:
            return HgKey_k9;

        case SDLK_Q:
            return HgKey_q;
        case SDLK_W:
            return HgKey_w;
        case SDLK_E:
            return HgKey_e;
        case SDLK_R:
            return HgKey_r;
        case SDLK_T:
            return HgKey_t;
        case SDLK_Y:
            return HgKey_y;
        case SDLK_U:
            return HgKey_u;
        case SDLK_I:
            return HgKey_i;
        case SDLK_O:
            return HgKey_o;
        case SDLK_P:
            return HgKey_p;
        case SDLK_A:
            return HgKey_a;
        case SDLK_S:
            return HgKey_s;
        case SDLK_D:
            return HgKey_d;
        case SDLK_F:
            return HgKey_f;
        case SDLK_G:
            return HgKey_g;
        case SDLK_H:
            return HgKey_h;
        case SDLK_J:
            return HgKey_j;
        case SDLK_K:
            return HgKey_k;
        case SDLK_L:
            return HgKey_l;
        case SDLK_Z:
            return HgKey_z;
        case SDLK_X:
            return HgKey_x;
        case SDLK_C:
            return HgKey_c;
        case SDLK_V:
            return HgKey_v;
        case SDLK_B:
            return HgKey_b;
        case SDLK_N:
            return HgKey_n;
        case SDLK_M:
            return HgKey_m;

        case SDLK_SEMICOLON:
            return HgKey_semicolon;
        case SDLK_COLON:
            return HgKey_colon;
        case SDLK_APOSTROPHE:
            return HgKey_apostrophe;
        case SDLK_DBLAPOSTROPHE:
            return HgKey_quotation;
        case SDLK_COMMA:
            return HgKey_comma;
        case SDLK_PERIOD:
            return HgKey_period;
        case SDLK_QUESTION:
            return HgKey_question;
        case SDLK_GRAVE:
            return HgKey_grave;
        case SDLK_TILDE:
            return HgKey_tilde;
        case SDLK_EXCLAIM:
            return HgKey_exclamation;
        case SDLK_AT:
            return HgKey_at;
        case SDLK_HASH:
            return HgKey_hash;
        case SDLK_DOLLAR:
            return HgKey_dollar;
        case SDLK_PERCENT:
            return HgKey_percent;
        case SDLK_CARET:
            return HgKey_carot;
        case SDLK_AMPERSAND:
            return HgKey_ampersand;
        case SDLK_ASTERISK:
            return HgKey_asterisk;

        case SDLK_LEFTPAREN:
            return HgKey_lparen;
        case SDLK_RIGHTPAREN:
            return HgKey_rparen;
        case SDLK_LEFTBRACKET:
            return HgKey_lbracket;
        case SDLK_RIGHTBRACKET:
            return HgKey_rbracket;
        case SDLK_LEFTBRACE:
            return HgKey_lbrace;
        case SDLK_RIGHTBRACE:
            return HgKey_rbrace;

        case SDLK_EQUALS:
            return HgKey_equal;
        case SDLK_LESS:
            return HgKey_less;
        case SDLK_GREATER:
            return HgKey_greater;
        case SDLK_PLUS:
            return HgKey_plus;
        case SDLK_MINUS:
            return HgKey_minus;
        case SDLK_SLASH:
            return HgKey_slash;
        case SDLK_BACKSLASH:
            return HgKey_backslash;
        case SDLK_UNDERSCORE:
            return HgKey_underscore;
        case SDLK_PIPE:
            return HgKey_bar;

        case SDLK_UP:
            return HgKey_up;
        case SDLK_DOWN:
            return HgKey_down;
        case SDLK_LEFT:
            return HgKey_left;
        case SDLK_RIGHT:
            return HgKey_right;

        case SDLK_ESCAPE:
            return HgKey_escape;
        case SDLK_SPACE:
            return HgKey_space;
        case SDLK_RETURN:
            return HgKey_enter;
        case SDLK_BACKSPACE:
            return HgKey_backspace;
        case SDLK_DELETE:
            return HgKey_kdelete;
        case SDLK_INSERT:
            return HgKey_insert;
        case SDLK_TAB:
            return HgKey_tab;
        case SDLK_HOME:
            return HgKey_home;
        case SDLK_END:
            return HgKey_end;

        case SDLK_F1:
            return HgKey_f1;
        case SDLK_F2:
            return HgKey_f2;
        case SDLK_F3:
            return HgKey_f3;
        case SDLK_F4:
            return HgKey_f4;
        case SDLK_F5:
            return HgKey_f5;
        case SDLK_F6:
            return HgKey_f6;
        case SDLK_F7:
            return HgKey_f7;
        case SDLK_F8:
            return HgKey_f8;
        case SDLK_F9:
            return HgKey_f9;
        case SDLK_F10:
            return HgKey_f10;
        case SDLK_F11:
            return HgKey_f11;
        case SDLK_F12:
            return HgKey_f12;

        case SDLK_LSHIFT:
            return HgKey_lshift;
        case SDLK_RSHIFT:
            return HgKey_rshift;
        case SDLK_LCTRL:
            return HgKey_lctrl;
        case SDLK_RCTRL:
            return HgKey_rctrl;
        case SDLK_LALT:
            return HgKey_lalt;
        case SDLK_RALT:
            return HgKey_ralt;
        case SDLK_LGUI:
            return HgKey_lsuper;
        case SDLK_RGUI:
            return HgKey_rsuper;
        case SDLK_CAPSLOCK:
            return HgKey_capslock;
    }
    return HgKey_none;
}

static HgKey sdlButtonToHgKey(u32 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT:
            return HgKey_mouse1;
        case SDL_BUTTON_RIGHT:
            return HgKey_mouse2;
        case SDL_BUTTON_MIDDLE:
            return HgKey_mouse3;
        case SDL_BUTTON_X1:
            return HgKey_mouse4;
        case SDL_BUTTON_X2:
            return HgKey_mouse5;
    }
    return HgKey_none;
}

void hgProcessEvents()
{
    f32 oldMouseX, oldMouseY;
    SDL_GetMouseState(&oldMouseX, &oldMouseY);

    HgKeyEvent windowEvent{};

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (state.imguiInitialized)
            ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                state.wasQuit = true;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                if (HgWindow** window = state.windows.get(event.window.windowID); window != nullptr)
                {
                    SDL_GetWindowSize((*window)->sdlWindow, (int*)&(*window)->width, (int*)&(*window)->height);
                    hgInternalResizeWindowSwapchain(*window);
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                windowEvent.type = HgKeyEventType_keyPress;
                windowEvent.key = sdlKeycodeToHgKey(event.key.key);
                state.isKeyDown[windowEvent.key] = true;
                break;
            case SDL_EVENT_KEY_UP:
                windowEvent.type = HgKeyEventType_keyRelease;
                windowEvent.key = sdlKeycodeToHgKey(event.key.key);
                state.isKeyDown[windowEvent.key] = false;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                windowEvent.type = HgKeyEventType_keyPress;
                windowEvent.key = sdlButtonToHgKey(event.button.button);
                state.isKeyDown[windowEvent.key] = true;
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                windowEvent.type = HgKeyEventType_keyRelease;
                windowEvent.key = sdlButtonToHgKey(event.button.button);
                state.isKeyDown[windowEvent.key] = false;
                break;
        }
    }

    if (windowEvent.type != HgKeyEventType_none)
    {
        HgWindow** window = state.windows.get(event.button.windowID);
        if (window != nullptr)
            (*window)->events[(*window)->eventCount++] = windowEvent;
    }

    f32 newMouseX, newMouseY;
    SDL_GetMouseState(&newMouseX, &newMouseY);
    state.mouseDeltaX = newMouseX - oldMouseX;
    state.mouseDeltaY = newMouseY - oldMouseY;
}

bool hgWasQuit()
{
    return state.wasQuit;
}

HgKeyEvent* hgGetKeyEvents(HgWindow* window, u32* count)
{
    hgAssert(window != nullptr);
    hgAssert(count != nullptr);

    *count = window->eventCount;
    return window->events;
}

bool hgIsKeyDown(HgKey key)
{
    return state.isKeyDown[key];
}

void hgGetMousePos(f32* x, f32* y)
{
    SDL_GetMouseState(x, y);
}

void hgGetMouseDelta(f32* x, f32* y)
{
    *x = state.mouseDeltaX;
    *y = state.mouseDeltaY;
}

bool hgIsMouseFocused(HgWindow* window)
{
    return SDL_GetMouseFocus() == window->sdlWindow;
}

void hgGetWindowSize(HgWindow* window, u32* width, u32* height, HgFormat* format)
{
    if (width != nullptr)
        *width = window->width;
    if (height != nullptr)
        *height = window->height;
    if (format != nullptr)
        *format = window->format;
}

HgGpuView* hgGetCurrentWindowImage(HgWindow* window)
{
    return window->views[window->currentImage];
}

void hgInternalInitImGuiPlatform(HgWindow* window)
{
    ImGui_ImplSDL3_InitForVulkan(window->sdlWindow);
    state.imguiInitialized = true;
}

void hgInternalDeinitImGuiPlatform()
{
    state.imguiInitialized = false;
    ImGui_ImplSDL3_Shutdown();
}

void ImGui_ImplHurdyGurdy_NewFrame()
{
    ImGui_ImplSDL3_NewFrame();
}

