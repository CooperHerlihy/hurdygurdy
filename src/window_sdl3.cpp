#include "hurdygurdy.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

static bool imguiInitialized = false;

void hgInternalCreateWindowSwapchain(HgWindow* window, const HgWindowConfig* config);
void hgInternalResizeWindowSwapchain(HgWindow* window);
void hgInternalDestroyWindowSwapchain(HgWindow* window);

struct HgWindow::Internals {
    SDL_Window* sdlWindow;
};

void hgInitPlatform()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
}

void hgDeinitPlatform()
{
    SDL_Quit();
}

HgWindow* HgWindow::create(HgArena* arena, const HgWindowConfig* config)
{
    HgWindow* window = hgAlloc<HgWindow>(arena, 1);
    *window = {};
    window->internals = hgAlloc<Internals>(arena, 1);
    *window->internals = {};

    const char* title = config->title != nullptr ? config->title : "Hurdy Gurdy";

    if (config->windowed)
    {
        window->width = config->width;
        window->height = config->height;
        window->internals->sdlWindow = SDL_CreateWindow(
            title,
            config->width,
            config->height,
            SDL_WINDOW_VULKAN | (config->resizable ? SDL_WINDOW_RESIZABLE : 0));
    } else {
        int modeCount = 0;
        SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &modeCount);
        hgDefer(SDL_free(modes));

        window->width = modes[0]->w;
        window->height = modes[0]->h;
        window->internals->sdlWindow = SDL_CreateWindow(
            title,
            window->width,
            window->height,
            SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN);
    }

    bool success = SDL_Vulkan_CreateSurface(window->internals->sdlWindow, hgVkInstance, nullptr, &window->surface);
    if (!success || window->surface == nullptr)
        hgError("Failed to create Vulkan surface: %s\n", SDL_GetError());

    hgInternalCreateWindowSwapchain(window, config);

    return window;
}

void HgWindow::destroy()
{
    hgInternalDestroyWindowSwapchain(this);
    vkDestroySurfaceKHR(hgVkInstance, surface, nullptr);
    SDL_DestroyWindow(internals->sdlWindow);
}

void HgWindow::setIcon(u32* iconData, u32 iconWidth, u32 iconHeight)
{
    hgError("window setIcon : TODO\n");
    (void)iconData;
    (void)iconWidth;
    (void)iconHeight;
}

bool HgWindow::isFullscreen()
{
    hgError("window isFullscreen : TODO\n");
}

void HgWindow::setFullscreen(bool fullscreen)
{
    hgError("window setFullscreen : TODO\n");
    (void)fullscreen;
}

void HgWindow::setCursor(HgWindow::Cursor cursor)
{
    hgError("window setCursor : TODO\n");
    (void)cursor;
}

void HgWindow::setCursorImage(u32* data, u32 imageWidth, u32 imageHeight)
{
    hgError("window setCursorImage : TODO\n");
    (void)data;
    (void)imageWidth;
    (void)imageHeight;
}

u32 hgVkGetPlatformExtensions(HgArena* arena, HgStringView** extBuffer)
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

void hgProcessWindowEvents(HgWindow** windows, u32 windowCount)
{
    hgAssert(windowCount == 1);
    HgWindow* window = windows[0];

    u32 oldWidth = window->width;
    u32 oldHeight = window->height;
    f64 oldMouseX = window->mousePosX;
    f64 oldMouseY = window->mousePosY;

    memset(window->wasKeyPressed, 0, sizeof(window->wasKeyPressed));
    memset(window->wasKeyReleased, 0, sizeof(window->wasKeyReleased));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (imguiInitialized)
            ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                window->wasClosed = true;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                SDL_GetWindowSize(window->internals->sdlWindow, (int*)&window->width, (int*)&window->height);
                break;
            case SDL_EVENT_KEY_DOWN:
            {
                HgKey key = HgKey_none;
                switch (event.key.key)
                {
                    case SDLK_0:
                        key = HgKey_k0;
                        break;
                    case SDLK_1:
                        key = HgKey_k1;
                        break;
                    case SDLK_2:
                        key = HgKey_k2;
                        break;
                    case SDLK_3:
                        key = HgKey_k3;
                        break;
                    case SDLK_4:
                        key = HgKey_k4;
                        break;
                    case SDLK_5:
                        key = HgKey_k5;
                        break;
                    case SDLK_6:
                        key = HgKey_k6;
                        break;
                    case SDLK_7:
                        key = HgKey_k7;
                        break;
                    case SDLK_8:
                        key = HgKey_k8;
                        break;
                    case SDLK_9:
                        key = HgKey_k9;
                        break;
                    case SDLK_Q:
                        key = HgKey_q;
                        break;
                    case SDLK_W:
                        key = HgKey_w;
                        break;
                    case SDLK_E:
                        key = HgKey_e;
                        break;
                    case SDLK_R:
                        key = HgKey_r;
                        break;
                    case SDLK_T:
                        key = HgKey_t;
                        break;
                    case SDLK_Y:
                        key = HgKey_y;
                        break;
                    case SDLK_U:
                        key = HgKey_u;
                        break;
                    case SDLK_I:
                        key = HgKey_i;
                        break;
                    case SDLK_O:
                        key = HgKey_o;
                        break;
                    case SDLK_P:
                        key = HgKey_p;
                        break;
                    case SDLK_A:
                        key = HgKey_a;
                        break;
                    case SDLK_S:
                        key = HgKey_s;
                        break;
                    case SDLK_D:
                        key = HgKey_d;
                        break;
                    case SDLK_F:
                        key = HgKey_f;
                        break;
                    case SDLK_G:
                        key = HgKey_g;
                        break;
                    case SDLK_H:
                        key = HgKey_h;
                        break;
                    case SDLK_J:
                        key = HgKey_j;
                        break;
                    case SDLK_K:
                        key = HgKey_k;
                        break;
                    case SDLK_L:
                        key = HgKey_l;
                        break;
                    case SDLK_Z:
                        key = HgKey_z;
                        break;
                    case SDLK_X:
                        key = HgKey_x;
                        break;
                    case SDLK_C:
                        key = HgKey_c;
                        break;
                    case SDLK_V:
                        key = HgKey_v;
                        break;
                    case SDLK_B:
                        key = HgKey_b;
                        break;
                    case SDLK_N:
                        key = HgKey_n;
                        break;
                    case SDLK_M:
                        key = HgKey_m;
                        break;

                    case SDLK_SEMICOLON:
                        key = HgKey_semicolon;
                        break;
                    case SDLK_COLON:
                        key = HgKey_colon;
                        break;
                    case SDLK_APOSTROPHE:
                        key = HgKey_apostrophe;
                        break;
                    case SDLK_DBLAPOSTROPHE:
                        key = HgKey_quotation;
                        break;
                    case SDLK_COMMA:
                        key = HgKey_comma;
                        break;
                    case SDLK_PERIOD:
                        key = HgKey_period;
                        break;
                    case SDLK_QUESTION:
                        key = HgKey_question;
                        break;

                    case SDLK_GRAVE:
                        key = HgKey_grave;
                        break;
                    case SDLK_TILDE:
                        key = HgKey_tilde;
                        break;

                    case SDLK_EXCLAIM:
                        key = HgKey_exclamation;
                        break;
                    case SDLK_AT:
                        key = HgKey_at;
                        break;
                    case SDLK_HASH:
                        key = HgKey_hash;
                        break;
                    case SDLK_DOLLAR:
                        key = HgKey_dollar;
                        break;
                    case SDLK_PERCENT:
                        key = HgKey_percent;
                        break;
                    case SDLK_CARET:
                        key = HgKey_carot;
                        break;
                    case SDLK_AMPERSAND:
                        key = HgKey_ampersand;
                        break;
                    case SDLK_ASTERISK:
                        key = HgKey_asterisk;
                        break;

                    case SDLK_LEFTPAREN:
                        key = HgKey_lparen;
                        break;
                    case SDLK_RIGHTPAREN:
                        key = HgKey_rparen;
                        break;

                    case SDLK_LEFTBRACKET:
                        key = HgKey_lbracket;
                        break;
                    case SDLK_RIGHTBRACKET:
                        key = HgKey_rbracket;
                        break;

                    case SDLK_LEFTBRACE:
                        key = HgKey_lbrace;
                        break;
                    case SDLK_RIGHTBRACE:
                        key = HgKey_rbrace;
                        break;

                    case SDLK_EQUALS:
                        key = HgKey_equal;
                        break;
                    case SDLK_LESS:
                        key = HgKey_less;
                        break;
                    case SDLK_GREATER:
                        key = HgKey_greater;
                        break;
                    case SDLK_PLUS:
                        key = HgKey_plus;
                        break;
                    case SDLK_MINUS:
                        key = HgKey_minus;
                        break;

                    case SDLK_SLASH:
                        key = HgKey_slash;
                        break;
                    case SDLK_BACKSLASH:
                        key = HgKey_backslash;
                        break;
                    case SDLK_UNDERSCORE:
                        key = HgKey_underscore;
                        break;
                    case SDLK_PIPE:
                        key = HgKey_bar;
                        break;

                    case SDLK_UP:
                        key = HgKey_up;
                        break;
                    case SDLK_DOWN:
                        key = HgKey_down;
                        break;
                    case SDLK_LEFT:
                        key = HgKey_left;
                        break;
                    case SDLK_RIGHT:
                        key = HgKey_right;
                        break;

                    case SDLK_ESCAPE:
                        key = HgKey_escape;
                        break;
                    case SDLK_SPACE:
                        key = HgKey_space;
                        break;
                    case SDLK_RETURN:
                        key = HgKey_enter;
                        break;
                    case SDLK_BACKSPACE:
                        key = HgKey_backspace;
                        break;
                    case SDLK_DELETE:
                        key = HgKey_kdelete;
                        break;
                    case SDLK_INSERT:
                        key = HgKey_insert;
                        break;
                    case SDLK_TAB:
                        key = HgKey_tab;
                        break;
                    case SDLK_HOME:
                        key = HgKey_home;
                        break;
                    case SDLK_END:
                        key = HgKey_end;
                        break;

                    case SDLK_F1:
                        key = HgKey_f1;
                        break;
                    case SDLK_F2:
                        key = HgKey_f2;
                        break;
                    case SDLK_F3:
                        key = HgKey_f3;
                        break;
                    case SDLK_F4:
                        key = HgKey_f4;
                        break;
                    case SDLK_F5:
                        key = HgKey_f5;
                        break;
                    case SDLK_F6:
                        key = HgKey_f6;
                        break;
                    case SDLK_F7:
                        key = HgKey_f7;
                        break;
                    case SDLK_F8:
                        key = HgKey_f8;
                        break;
                    case SDLK_F9:
                        key = HgKey_f9;
                        break;
                    case SDLK_F10:
                        key = HgKey_f10;
                        break;
                    case SDLK_F11:
                        key = HgKey_f11;
                        break;
                    case SDLK_F12:
                        key = HgKey_f12;
                        break;

                    case SDLK_LSHIFT:
                        key = HgKey_lshift;
                        break;
                    case SDLK_RSHIFT:
                        key = HgKey_rshift;
                        break;
                    case SDLK_LCTRL:
                        key = HgKey_lctrl;
                        break;
                    case SDLK_RCTRL:
                        key = HgKey_rctrl;
                        break;
                    case SDLK_LALT:
                        key = HgKey_lalt;
                        break;
                    case SDLK_RALT:
                        key = HgKey_ralt;
                        break;

                    case SDLK_LGUI:
                        key = HgKey_lsuper;
                        break;
                    case SDLK_RGUI:
                        key = HgKey_rsuper;
                        break;

                    case SDLK_CAPSLOCK:
                        key = HgKey_capslock;
                        break;
                }
                window->isKeyDown[key] = true;
                window->wasKeyPressed[key] = true;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                HgKey key = HgKey_none;

                switch (event.button.button)
                {
                    case SDL_BUTTON_LEFT:
                        key = HgKey_mouse1;
                        break;

                    case SDL_BUTTON_RIGHT:
                        key = HgKey_mouse2;
                        break;

                    case SDL_BUTTON_MIDDLE:
                        key = HgKey_mouse3;
                        break;

                    case SDL_BUTTON_X1:
                        key = HgKey_mouse4;
                        break;

                    case SDL_BUTTON_X2:
                        key = HgKey_mouse5;
                        break;
                }
                window->isKeyDown[key] = true;
                window->wasKeyPressed[key] = true;
            } break;
            case SDL_EVENT_KEY_UP:
            {
                HgKey key = HgKey_none;
                switch (event.key.key)
                {
                    case SDLK_0:
                        key = HgKey_k0;
                        break;
                    case SDLK_1:
                        key = HgKey_k1;
                        break;
                    case SDLK_2:
                        key = HgKey_k2;
                        break;
                    case SDLK_3:
                        key = HgKey_k3;
                        break;
                    case SDLK_4:
                        key = HgKey_k4;
                        break;
                    case SDLK_5:
                        key = HgKey_k5;
                        break;
                    case SDLK_6:
                        key = HgKey_k6;
                        break;
                    case SDLK_7:
                        key = HgKey_k7;
                        break;
                    case SDLK_8:
                        key = HgKey_k8;
                        break;
                    case SDLK_9:
                        key = HgKey_k9;
                        break;
                    case SDLK_Q:
                        key = HgKey_q;
                        break;
                    case SDLK_W:
                        key = HgKey_w;
                        break;
                    case SDLK_E:
                        key = HgKey_e;
                        break;
                    case SDLK_R:
                        key = HgKey_r;
                        break;
                    case SDLK_T:
                        key = HgKey_t;
                        break;
                    case SDLK_Y:
                        key = HgKey_y;
                        break;
                    case SDLK_U:
                        key = HgKey_u;
                        break;
                    case SDLK_I:
                        key = HgKey_i;
                        break;
                    case SDLK_O:
                        key = HgKey_o;
                        break;
                    case SDLK_P:
                        key = HgKey_p;
                        break;
                    case SDLK_A:
                        key = HgKey_a;
                        break;
                    case SDLK_S:
                        key = HgKey_s;
                        break;
                    case SDLK_D:
                        key = HgKey_d;
                        break;
                    case SDLK_F:
                        key = HgKey_f;
                        break;
                    case SDLK_G:
                        key = HgKey_g;
                        break;
                    case SDLK_H:
                        key = HgKey_h;
                        break;
                    case SDLK_J:
                        key = HgKey_j;
                        break;
                    case SDLK_K:
                        key = HgKey_k;
                        break;
                    case SDLK_L:
                        key = HgKey_l;
                        break;
                    case SDLK_Z:
                        key = HgKey_z;
                        break;
                    case SDLK_X:
                        key = HgKey_x;
                        break;
                    case SDLK_C:
                        key = HgKey_c;
                        break;
                    case SDLK_V:
                        key = HgKey_v;
                        break;
                    case SDLK_B:
                        key = HgKey_b;
                        break;
                    case SDLK_N:
                        key = HgKey_n;
                        break;
                    case SDLK_M:
                        key = HgKey_m;
                        break;

                    case SDLK_SEMICOLON:
                        key = HgKey_semicolon;
                        break;
                    case SDLK_COLON:
                        key = HgKey_colon;
                        break;
                    case SDLK_APOSTROPHE:
                        key = HgKey_apostrophe;
                        break;
                    case SDLK_DBLAPOSTROPHE:
                        key = HgKey_quotation;
                        break;
                    case SDLK_COMMA:
                        key = HgKey_comma;
                        break;
                    case SDLK_PERIOD:
                        key = HgKey_period;
                        break;
                    case SDLK_QUESTION:
                        key = HgKey_question;
                        break;

                    case SDLK_GRAVE:
                        key = HgKey_grave;
                        break;
                    case SDLK_TILDE:
                        key = HgKey_tilde;
                        break;

                    case SDLK_EXCLAIM:
                        key = HgKey_exclamation;
                        break;
                    case SDLK_AT:
                        key = HgKey_at;
                        break;
                    case SDLK_HASH:
                        key = HgKey_hash;
                        break;
                    case SDLK_DOLLAR:
                        key = HgKey_dollar;
                        break;
                    case SDLK_PERCENT:
                        key = HgKey_percent;
                        break;
                    case SDLK_CARET:
                        key = HgKey_carot;
                        break;
                    case SDLK_AMPERSAND:
                        key = HgKey_ampersand;
                        break;
                    case SDLK_ASTERISK:
                        key = HgKey_asterisk;
                        break;

                    case SDLK_LEFTPAREN:
                        key = HgKey_lparen;
                        break;
                    case SDLK_RIGHTPAREN:
                        key = HgKey_rparen;
                        break;

                    case SDLK_LEFTBRACKET:
                        key = HgKey_lbracket;
                        break;
                    case SDLK_RIGHTBRACKET:
                        key = HgKey_rbracket;
                        break;

                    case SDLK_LEFTBRACE:
                        key = HgKey_lbrace;
                        break;
                    case SDLK_RIGHTBRACE:
                        key = HgKey_rbrace;
                        break;

                    case SDLK_EQUALS:
                        key = HgKey_equal;
                        break;
                    case SDLK_LESS:
                        key = HgKey_less;
                        break;
                    case SDLK_GREATER:
                        key = HgKey_greater;
                        break;
                    case SDLK_PLUS:
                        key = HgKey_plus;
                        break;
                    case SDLK_MINUS:
                        key = HgKey_minus;
                        break;

                    case SDLK_SLASH:
                        key = HgKey_slash;
                        break;
                    case SDLK_BACKSLASH:
                        key = HgKey_backslash;
                        break;
                    case SDLK_UNDERSCORE:
                        key = HgKey_underscore;
                        break;
                    case SDLK_PIPE:
                        key = HgKey_bar;
                        break;

                    case SDLK_UP:
                        key = HgKey_up;
                        break;
                    case SDLK_DOWN:
                        key = HgKey_down;
                        break;
                    case SDLK_LEFT:
                        key = HgKey_left;
                        break;
                    case SDLK_RIGHT:
                        key = HgKey_right;
                        break;

                    case SDLK_ESCAPE:
                        key = HgKey_escape;
                        break;
                    case SDLK_SPACE:
                        key = HgKey_space;
                        break;
                    case SDLK_RETURN:
                        key = HgKey_enter;
                        break;
                    case SDLK_BACKSPACE:
                        key = HgKey_backspace;
                        break;
                    case SDLK_DELETE:
                        key = HgKey_kdelete;
                        break;
                    case SDLK_INSERT:
                        key = HgKey_insert;
                        break;
                    case SDLK_TAB:
                        key = HgKey_tab;
                        break;
                    case SDLK_HOME:
                        key = HgKey_home;
                        break;
                    case SDLK_END:
                        key = HgKey_end;
                        break;

                    case SDLK_F1:
                        key = HgKey_f1;
                        break;
                    case SDLK_F2:
                        key = HgKey_f2;
                        break;
                    case SDLK_F3:
                        key = HgKey_f3;
                        break;
                    case SDLK_F4:
                        key = HgKey_f4;
                        break;
                    case SDLK_F5:
                        key = HgKey_f5;
                        break;
                    case SDLK_F6:
                        key = HgKey_f6;
                        break;
                    case SDLK_F7:
                        key = HgKey_f7;
                        break;
                    case SDLK_F8:
                        key = HgKey_f8;
                        break;
                    case SDLK_F9:
                        key = HgKey_f9;
                        break;
                    case SDLK_F10:
                        key = HgKey_f10;
                        break;
                    case SDLK_F11:
                        key = HgKey_f11;
                        break;
                    case SDLK_F12:
                        key = HgKey_f12;
                        break;

                    case SDLK_LSHIFT:
                        key = HgKey_lshift;
                        break;
                    case SDLK_RSHIFT:
                        key = HgKey_rshift;
                        break;
                    case SDLK_LCTRL:
                        key = HgKey_lctrl;
                        break;
                    case SDLK_RCTRL:
                        key = HgKey_rctrl;
                        break;
                    case SDLK_LALT:
                        key = HgKey_lalt;
                        break;
                    case SDLK_RALT:
                        key = HgKey_ralt;
                        break;

                    case SDLK_LGUI:
                        key = HgKey_lsuper;
                        break;
                    case SDLK_RGUI:
                        key = HgKey_rsuper;
                        break;

                    case SDLK_CAPSLOCK:
                        key = HgKey_capslock;
                        break;
                }
                window->isKeyDown[key] = false;
                window->wasKeyReleased[key] = true;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                HgKey key = HgKey_none;

                switch (event.button.button)
                {
                    case SDL_BUTTON_LEFT:
                        key = HgKey_mouse1;
                        break;

                    case SDL_BUTTON_RIGHT:
                        key = HgKey_mouse2;
                        break;

                    case SDL_BUTTON_MIDDLE:
                        key = HgKey_mouse3;
                        break;

                    case SDL_BUTTON_X1:
                        key = HgKey_mouse4;
                        break;

                    case SDL_BUTTON_X2:
                        key = HgKey_mouse5;
                        break;
                }
                window->isKeyDown[key] = false;
                window->wasKeyReleased[key] = true;
            }
            break;
            case SDL_EVENT_MOUSE_MOTION:
                window->mousePosX = event.motion.x / window->height;
                window->mousePosY = event.motion.y / window->height;
        }
    }

    if (window->width != oldWidth || window->height != oldHeight)
        hgInternalResizeWindowSwapchain(window);

    window->mouseDeltaX = window->mousePosX - oldMouseX;
    window->mouseDeltaY = window->mousePosY - oldMouseY;
}

void ImGui_ImplHurdyGurdy_Init(
    HgWindow* window,
    u32 colorAttachmentCount,
    const VkFormat* colorFormats,
    VkFormat depthFormat,
    VkFormat stencilFormat)
{
    ImGui_ImplSDL3_InitForVulkan(window->internals->sdlWindow);

    ImGui_ImplVulkan_InitInfo imguiInfo{};
    imguiInfo.Instance = hgVkInstance;
    imguiInfo.PhysicalDevice = hgVkPhysicalDevice;
    imguiInfo.Device = hgVkDevice;
    imguiInfo.QueueFamily = hgVkQueueFamily;
    imguiInfo.Queue = hgVkQueue;
    imguiInfo.DescriptorPoolSize = 1000;
    imguiInfo.MinImageCount = window->imageCount;
    imguiInfo.ImageCount = window->imageCount;
    imguiInfo.MinAllocationSize = 1 << 20;
    imguiInfo.UseDynamicRendering = true;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = colorAttachmentCount;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = stencilFormat;
#ifdef HG_VK_DEBUG_MESSENGER
    imguiInfo.CheckVkResultFn = [](VkResult err)
    {
        if (err != VK_SUCCESS)
            hgWarn("Vulkan error from ImGui: %s\n", hgVkResultToStr(err));
    };
#endif

    ImGui_ImplVulkan_Init(&imguiInfo);

    imguiInitialized = true;
}

void ImGui_ImplHurdyGurdy_Shutdown()
{
    imguiInitialized = false;

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void ImGui_ImplHurdyGurdy_NewFrame()
{
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
}

void ImGui_ImplHurdyGurdy_Draw(VkCommandBuffer cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

