#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace hg::sdl {

#define HG_MAKE_SDL_FUNC(name) decltype(&::name) name = nullptr

struct SdlFuncs {
    HG_MAKE_SDL_FUNC(SDL_Init);
    HG_MAKE_SDL_FUNC(SDL_Quit);
    HG_MAKE_SDL_FUNC(SDL_GetError);
    HG_MAKE_SDL_FUNC(SDL_CreateWindow);
    HG_MAKE_SDL_FUNC(SDL_DestroyWindow);
    HG_MAKE_SDL_FUNC(SDL_GetWindowID);
    HG_MAKE_SDL_FUNC(SDL_GetWindowSize);
    HG_MAKE_SDL_FUNC(SDL_GetWindowSizeInPixels);
    HG_MAKE_SDL_FUNC(SDL_GetWindowPosition);
    HG_MAKE_SDL_FUNC(SDL_SetWindowPosition);
    HG_MAKE_SDL_FUNC(SDL_SetWindowSize);
    HG_MAKE_SDL_FUNC(SDL_SetWindowTitle);
    HG_MAKE_SDL_FUNC(SDL_SetWindowResizable);
    HG_MAKE_SDL_FUNC(SDL_SetWindowFullscreen);
    HG_MAKE_SDL_FUNC(SDL_GetWindowFlags);
    HG_MAKE_SDL_FUNC(SDL_GetDisplays);
    HG_MAKE_SDL_FUNC(SDL_GetDisplayBounds);
    HG_MAKE_SDL_FUNC(SDL_GetDisplayUsableBounds);
    HG_MAKE_SDL_FUNC(SDL_GetDisplayContentScale);
    HG_MAKE_SDL_FUNC(SDL_GetGlobalMouseState);
    HG_MAKE_SDL_FUNC(SDL_GetMouseState);
    HG_MAKE_SDL_FUNC(SDL_GetMouseFocus);
    HG_MAKE_SDL_FUNC(SDL_HasClipboardText);
    HG_MAKE_SDL_FUNC(SDL_GetClipboardText);
    HG_MAKE_SDL_FUNC(SDL_SetClipboardText);
    HG_MAKE_SDL_FUNC(SDL_CreateSystemCursor);
    HG_MAKE_SDL_FUNC(SDL_DestroyCursor);
    HG_MAKE_SDL_FUNC(SDL_SetCursor);
    HG_MAKE_SDL_FUNC(SDL_ShowCursor);
    HG_MAKE_SDL_FUNC(SDL_HideCursor);
    HG_MAKE_SDL_FUNC(SDL_PollEvent);
    HG_MAKE_SDL_FUNC(SDL_Vulkan_CreateSurface);
    HG_MAKE_SDL_FUNC(SDL_Vulkan_GetInstanceExtensions);
    HG_MAKE_SDL_FUNC(SDL_OpenAudioDevice);
    HG_MAKE_SDL_FUNC(SDL_CloseAudioDevice);
    HG_MAKE_SDL_FUNC(SDL_CreateAudioStream);
    HG_MAKE_SDL_FUNC(SDL_DestroyAudioStream);
    HG_MAKE_SDL_FUNC(SDL_BindAudioStream);
    HG_MAKE_SDL_FUNC(SDL_SetAudioStreamGetCallback);
    HG_MAKE_SDL_FUNC(SDL_SetAudioStreamFormat);
    HG_MAKE_SDL_FUNC(SDL_PutAudioStreamData);
    HG_MAKE_SDL_FUNC(SDL_free);
    HG_MAKE_SDL_FUNC(SDL_OpenURL);
    HG_MAKE_SDL_FUNC(SDL_GetGamepads);
    HG_MAKE_SDL_FUNC(SDL_IsGamepad);
    HG_MAKE_SDL_FUNC(SDL_OpenGamepad);
    HG_MAKE_SDL_FUNC(SDL_CloseGamepad);
    HG_MAKE_SDL_FUNC(SDL_GetGamepadButton);
    HG_MAKE_SDL_FUNC(SDL_GetGamepadAxis);
    HG_MAKE_SDL_FUNC(SDL_GamepadConnected);
    HG_MAKE_SDL_FUNC(SDL_MaximizeWindow);
    HG_MAKE_SDL_FUNC(SDL_MinimizeWindow);
    HG_MAKE_SDL_FUNC(SDL_RestoreWindow);
    HG_MAKE_SDL_FUNC(SDL_GetTicksNS);
};

#undef HG_MAKE_SDL_FUNC

extern SdlFuncs sdlFuncs;

bool windowInit();
void windowDeinit();

bool initAudio();
void deinitAudio();

} // namespace hg::sdl
