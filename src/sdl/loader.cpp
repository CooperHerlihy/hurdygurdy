#include "sdl/sdl_backend.hpp"

namespace hg {
namespace sdl {

Library libsdl{};
SdlFuncs sdlFuncs{};

} // namespace sdl
} // namespace hg

#define SDL_WRAPPER(name, ret, params, args) \
    extern "C" ret SDLCALL name params \
    { \
        return ::hg::sdl::sdlFuncs.name args; \
    }

#define SDL_WRAPPER_VOID(name, params, args) \
    extern "C" void SDLCALL name params \
    { \
        ::hg::sdl::sdlFuncs.name args; \
    }

SDL_WRAPPER(SDL_Init, bool,
    (SDL_InitFlags flags), (flags))
SDL_WRAPPER_VOID(SDL_Quit, (), ())
SDL_WRAPPER(SDL_GetError, const char*, (), ())

// video/window
SDL_WRAPPER(SDL_CreateWindow, SDL_Window*,
    (const char* title, int w, int h, SDL_WindowFlags flags),
    (title, w, h, flags))
SDL_WRAPPER_VOID(SDL_DestroyWindow, (SDL_Window* window), (window))
SDL_WRAPPER(SDL_GetWindowID, SDL_WindowID, (SDL_Window* window), (window))
SDL_WRAPPER(SDL_GetWindowSize, bool,
    (SDL_Window* window, int* w, int* h), (window, w, h))
SDL_WRAPPER(SDL_GetWindowSizeInPixels, bool,
    (SDL_Window* window, int* w, int* hp), (window, w, hp))
SDL_WRAPPER(SDL_GetWindowPosition, bool,
    (SDL_Window* window, int* x, int* y), (window, x, y))
SDL_WRAPPER(SDL_SetWindowPosition, bool,
    (SDL_Window* window, int x, int y), (window, x, y))
SDL_WRAPPER(SDL_SetWindowSize, bool,
    (SDL_Window* window, int w, int h), (window, w, h))
SDL_WRAPPER(SDL_SetWindowTitle, bool,
    (SDL_Window* window, const char* title), (window, title))
SDL_WRAPPER(SDL_SetWindowOpacity, bool,
    (SDL_Window* window, float opacity), (window, opacity))
SDL_WRAPPER(SDL_ShowWindow, bool, (SDL_Window* window), (window))
SDL_WRAPPER(SDL_RaiseWindow, bool, (SDL_Window* window), (window))
SDL_WRAPPER(SDL_SetWindowParent, bool,
    (SDL_Window* window, SDL_Window* parent), (window, parent))
SDL_WRAPPER(SDL_GetWindowFlags, SDL_WindowFlags,
    (SDL_Window* window), (window))
SDL_WRAPPER(SDL_GetWindowDisplayScale, float,
    (SDL_Window* window), (window))
SDL_WRAPPER(SDL_GetWindowRelativeMouseMode, bool,
    (SDL_Window* window), (window))
SDL_WRAPPER(SDL_GetWindowFromID, SDL_Window*,
    (SDL_WindowID id), (id))
SDL_WRAPPER(SDL_GetWindowProperties, SDL_PropertiesID,
    (SDL_Window* window), (window))

// display
SDL_WRAPPER(SDL_GetDisplays, SDL_DisplayID*, (int* count), (count))
SDL_WRAPPER(SDL_GetPrimaryDisplay, SDL_DisplayID, (), ())
SDL_WRAPPER(SDL_GetFullscreenDisplayModes, SDL_DisplayMode**,
    (SDL_DisplayID displayID, int* count), (displayID, count))
SDL_WRAPPER(SDL_GetDisplayBounds, bool,
    (SDL_DisplayID displayID, SDL_Rect* rect), (displayID, rect))
SDL_WRAPPER(SDL_GetDisplayUsableBounds, bool,
    (SDL_DisplayID displayID, SDL_Rect* rect), (displayID, rect))
SDL_WRAPPER(SDL_GetDisplayContentScale, float,
    (SDL_DisplayID displayID), (displayID))

// input/mouse
SDL_WRAPPER_VOID(SDL_WarpMouseInWindow,
    (SDL_Window* window, float x, float y), (window, x, y))
SDL_WRAPPER(SDL_WarpMouseGlobal, bool, (float x, float y), (x, y))
SDL_WRAPPER(SDL_GetGlobalMouseState, SDL_MouseButtonFlags,
    (float* x, float* y), (x, y))
SDL_WRAPPER(SDL_GetMouseFocus, SDL_Window*, (), ())
SDL_WRAPPER(SDL_GetKeyboardFocus, SDL_Window*, (), ())
SDL_WRAPPER(SDL_CaptureMouse, bool, (bool capture), (capture))
SDL_WRAPPER(SDL_GetKeyName, const char*, (SDL_Keycode key), (key))
SDL_WRAPPER(SDL_GetScancodeName, const char*,
    (SDL_Scancode scancode), (scancode))

// clipboard
SDL_WRAPPER(SDL_HasClipboardText, bool, (), ())
SDL_WRAPPER(SDL_GetClipboardText, char*, (), ())
SDL_WRAPPER(SDL_SetClipboardText, bool, (const char* text), (text))

// cursor
SDL_WRAPPER(SDL_CreateSystemCursor, SDL_Cursor*,
    (SDL_SystemCursor id), (id))
SDL_WRAPPER_VOID(SDL_DestroyCursor, (SDL_Cursor* cursor), (cursor))
SDL_WRAPPER(SDL_SetCursor, bool, (SDL_Cursor* cursor), (cursor))
SDL_WRAPPER(SDL_ShowCursor, bool, (), ())
SDL_WRAPPER(SDL_HideCursor, bool, (), ())

// hints
SDL_WRAPPER(SDL_SetHint, bool, (const char* name, const char* value),
    (name, value))

// events
SDL_WRAPPER(SDL_PollEvent, bool, (SDL_Event* event), (event))

// timer/performance
SDL_WRAPPER(SDL_GetTicksNS, Uint64, (), ())
SDL_WRAPPER(SDL_GetPerformanceCounter, Uint64, (), ())
SDL_WRAPPER(SDL_GetPerformanceFrequency, Uint64, (), ())

// version
SDL_WRAPPER(SDL_GetVersion, int, (), ())

// OpenGL
SDL_WRAPPER(SDL_GL_CreateContext, SDL_GLContext,
    (SDL_Window* window), (window))
SDL_WRAPPER(SDL_GL_DestroyContext, bool,
    (SDL_GLContext context), (context))
SDL_WRAPPER(SDL_GL_GetCurrentContext, SDL_GLContext, (), ())
SDL_WRAPPER(SDL_GL_MakeCurrent, bool,
    (SDL_Window* window, SDL_GLContext context), (window, context))
SDL_WRAPPER(SDL_GL_SetAttribute, bool,
    (SDL_GLAttr attr, int value), (attr, value))
SDL_WRAPPER(SDL_GL_SetSwapInterval, bool, (int interval), (interval))
SDL_WRAPPER(SDL_GL_SwapWindow, bool, (SDL_Window* window), (window))

// Vulkan surface
SDL_WRAPPER(SDL_Vulkan_CreateSurface, bool,
    (SDL_Window* window, VkInstance instance,
     const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface),
    (window, instance, allocator, surface))
SDL_WRAPPER(SDL_Vulkan_GetInstanceExtensions, const char* const*,
    (Uint32* count), (count))

// audio
SDL_WRAPPER(SDL_OpenAudioDevice, SDL_AudioDeviceID,
    (SDL_AudioDeviceID devid, const SDL_AudioSpec* spec), (devid, spec))
SDL_WRAPPER_VOID(SDL_CloseAudioDevice,
    (SDL_AudioDeviceID devid), (devid))
SDL_WRAPPER(SDL_CreateAudioStream, SDL_AudioStream*,
    (const SDL_AudioSpec* src_spec,
     const SDL_AudioSpec* dst_spec), (src_spec, dst_spec))
SDL_WRAPPER_VOID(SDL_DestroyAudioStream,
    (SDL_AudioStream* stream), (stream))
SDL_WRAPPER(SDL_BindAudioStream, bool,
    (SDL_AudioDeviceID devid, SDL_AudioStream* stream),
    (devid, stream))
SDL_WRAPPER(SDL_SetAudioStreamFormat, bool,
    (SDL_AudioStream* stream, const SDL_AudioSpec* src_spec,
     const SDL_AudioSpec* dst_spec), (stream, src_spec, dst_spec))
SDL_WRAPPER(SDL_ClearAudioStream, bool,
    (SDL_AudioStream* stream), (stream))
SDL_WRAPPER(SDL_PutAudioStreamData, bool,
    (SDL_AudioStream* stream, const void* data, int len),
    (stream, data, len))
SDL_WRAPPER(SDL_GetAudioStreamQueued, int,
    (SDL_AudioStream* stream), (stream))
SDL_WRAPPER(SDL_SetAudioStreamGain, bool,
    (SDL_AudioStream* stream, float gain), (stream, gain))

// text input
SDL_WRAPPER(SDL_StartTextInput, bool, (SDL_Window* window), (window))
SDL_WRAPPER(SDL_StopTextInput, bool, (SDL_Window* window), (window))
SDL_WRAPPER(SDL_TextInputActive, bool, (SDL_Window* window), (window))
SDL_WRAPPER(SDL_SetTextInputArea, bool,
    (SDL_Window* window, const SDL_Rect* rect, int cursor),
    (window, rect, cursor))

// gamepad
SDL_WRAPPER(SDL_OpenGamepad, SDL_Gamepad*,
    (SDL_JoystickID instance_id), (instance_id))
SDL_WRAPPER_VOID(SDL_CloseGamepad, (SDL_Gamepad* gamepad), (gamepad))
SDL_WRAPPER(SDL_GetGamepads, SDL_JoystickID*, (int* count), (count));
SDL_WRAPPER(SDL_GetGamepadAxis, Sint16,
    (SDL_Gamepad* gamepad, SDL_GamepadAxis axis), (gamepad, axis))
SDL_WRAPPER(SDL_GetGamepadButton, bool,
    (SDL_Gamepad* gamepad, SDL_GamepadButton button), (gamepad, button))

// utility
SDL_WRAPPER_VOID(SDL_free, (void* mem), (mem))
SDL_WRAPPER(SDL_OpenURL, bool, (const char* url), (url))
SDL_WRAPPER(SDL_GetCurrentVideoDriver, const char*, (), ())
SDL_WRAPPER(SDL_GetPointerProperty, void*,
    (SDL_PropertiesID props, const char* name, void* defaultVal),
    (props, name, defaultVal))

#undef SDL_WRAPPER
#undef SDL_WRAPPER_VOID
