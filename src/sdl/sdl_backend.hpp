#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_audio.h>

#include "hg_dynlib.hpp"

namespace hg {
namespace sdl {

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
    HG_MAKE_SDL_FUNC(SDL_SetWindowOpacity);
    HG_MAKE_SDL_FUNC(SDL_ShowWindow);
    HG_MAKE_SDL_FUNC(SDL_RaiseWindow);
    HG_MAKE_SDL_FUNC(SDL_SetWindowParent);
    HG_MAKE_SDL_FUNC(SDL_GetWindowFlags);
    HG_MAKE_SDL_FUNC(SDL_GetWindowDisplayScale);
    HG_MAKE_SDL_FUNC(SDL_GetWindowRelativeMouseMode);
    HG_MAKE_SDL_FUNC(SDL_GetWindowFromID);
    HG_MAKE_SDL_FUNC(SDL_GetWindowProperties);
    HG_MAKE_SDL_FUNC(SDL_GetDisplays);
    HG_MAKE_SDL_FUNC(SDL_GetPrimaryDisplay);
    HG_MAKE_SDL_FUNC(SDL_GetFullscreenDisplayModes);
    HG_MAKE_SDL_FUNC(SDL_GetDisplayBounds);
    HG_MAKE_SDL_FUNC(SDL_GetDisplayUsableBounds);
    HG_MAKE_SDL_FUNC(SDL_GetDisplayContentScale);
    HG_MAKE_SDL_FUNC(SDL_WarpMouseInWindow);
    HG_MAKE_SDL_FUNC(SDL_WarpMouseGlobal);
    HG_MAKE_SDL_FUNC(SDL_GetGlobalMouseState);
    HG_MAKE_SDL_FUNC(SDL_GetMouseFocus);
    HG_MAKE_SDL_FUNC(SDL_GetKeyboardFocus);
    HG_MAKE_SDL_FUNC(SDL_CaptureMouse);
    HG_MAKE_SDL_FUNC(SDL_GetKeyName);
    HG_MAKE_SDL_FUNC(SDL_GetScancodeName);
    HG_MAKE_SDL_FUNC(SDL_HasClipboardText);
    HG_MAKE_SDL_FUNC(SDL_GetClipboardText);
    HG_MAKE_SDL_FUNC(SDL_SetClipboardText);
    HG_MAKE_SDL_FUNC(SDL_CreateSystemCursor);
    HG_MAKE_SDL_FUNC(SDL_DestroyCursor);
    HG_MAKE_SDL_FUNC(SDL_SetCursor);
    HG_MAKE_SDL_FUNC(SDL_ShowCursor);
    HG_MAKE_SDL_FUNC(SDL_HideCursor);
    HG_MAKE_SDL_FUNC(SDL_SetHint);
    HG_MAKE_SDL_FUNC(SDL_PollEvent);
    HG_MAKE_SDL_FUNC(SDL_GetTicksNS);
    HG_MAKE_SDL_FUNC(SDL_GetPerformanceCounter);
    HG_MAKE_SDL_FUNC(SDL_GetPerformanceFrequency);
    HG_MAKE_SDL_FUNC(SDL_GetVersion);
    HG_MAKE_SDL_FUNC(SDL_GL_CreateContext);
    HG_MAKE_SDL_FUNC(SDL_GL_DestroyContext);
    HG_MAKE_SDL_FUNC(SDL_GL_GetCurrentContext);
    HG_MAKE_SDL_FUNC(SDL_GL_MakeCurrent);
    HG_MAKE_SDL_FUNC(SDL_GL_SetAttribute);
    HG_MAKE_SDL_FUNC(SDL_GL_SetSwapInterval);
    HG_MAKE_SDL_FUNC(SDL_GL_SwapWindow);
    HG_MAKE_SDL_FUNC(SDL_Vulkan_CreateSurface);
    HG_MAKE_SDL_FUNC(SDL_Vulkan_GetInstanceExtensions);
    HG_MAKE_SDL_FUNC(SDL_OpenAudioDevice);
    HG_MAKE_SDL_FUNC(SDL_CloseAudioDevice);
    HG_MAKE_SDL_FUNC(SDL_CreateAudioStream);
    HG_MAKE_SDL_FUNC(SDL_DestroyAudioStream);
    HG_MAKE_SDL_FUNC(SDL_BindAudioStream);
    HG_MAKE_SDL_FUNC(SDL_SetAudioStreamFormat);
    HG_MAKE_SDL_FUNC(SDL_ClearAudioStream);
    HG_MAKE_SDL_FUNC(SDL_PutAudioStreamData);
    HG_MAKE_SDL_FUNC(SDL_GetAudioStreamQueued);
    HG_MAKE_SDL_FUNC(SDL_SetAudioStreamGain);
    HG_MAKE_SDL_FUNC(SDL_StartTextInput);
    HG_MAKE_SDL_FUNC(SDL_StopTextInput);
    HG_MAKE_SDL_FUNC(SDL_TextInputActive);
    HG_MAKE_SDL_FUNC(SDL_SetTextInputArea);
    HG_MAKE_SDL_FUNC(SDL_OpenGamepad);
    HG_MAKE_SDL_FUNC(SDL_CloseGamepad);
    HG_MAKE_SDL_FUNC(SDL_GetGamepads);
    HG_MAKE_SDL_FUNC(SDL_GetGamepadAxis);
    HG_MAKE_SDL_FUNC(SDL_GetGamepadButton);
    HG_MAKE_SDL_FUNC(SDL_free);
    HG_MAKE_SDL_FUNC(SDL_OpenURL);
    HG_MAKE_SDL_FUNC(SDL_GetCurrentVideoDriver);
    HG_MAKE_SDL_FUNC(SDL_GetPointerProperty);
};

#undef HG_MAKE_SDL_FUNC

extern Library libsdl;
extern SdlFuncs sdlFuncs;

bool loadSDL();
void unloadSDL();

} // namespace sdl
} // namespace hg
