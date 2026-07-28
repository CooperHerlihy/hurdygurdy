#include "sdl/sdl_backend.hpp"

#include "hg/error.hpp"
#include "hg/dynlib.hpp"

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

Library libsdl{};
SdlFuncs sdlFuncs{};

bool loadSDL()
{
    Maybe<Library> lib = Library::load(
#if defined(HG_PLATFORM_LINUX)
        "libSDL3.so.0"
#elif defined(HG_PLATFORM_WINDOWS)
        "SDL3.dll"
#elif defined(HG_PLATFORM_MACOS)
        "libSDL3.0.dylib"
#endif
    );
    if (!lib.has)
    {
        setError("Could not load SDL3");
        return false;
    }
    libsdl = std::move(*lib);

#define HG_LOAD_SDL_FUNC(name) \
    *(void**)&sdlFuncs.name = \
        libsdl.findFunction(#name).orElse(nullptr); \
    if (sdlFuncs.name == nullptr) { \
        setError("Could not load " #name); \
        return false; \
    }

    HG_LOAD_SDL_FUNC(SDL_Init);
    HG_LOAD_SDL_FUNC(SDL_Quit);
    HG_LOAD_SDL_FUNC(SDL_GetError);
    HG_LOAD_SDL_FUNC(SDL_CreateWindow);
    HG_LOAD_SDL_FUNC(SDL_DestroyWindow);
    HG_LOAD_SDL_FUNC(SDL_GetWindowID);
    HG_LOAD_SDL_FUNC(SDL_GetWindowSize);
    HG_LOAD_SDL_FUNC(SDL_GetWindowSizeInPixels);
    HG_LOAD_SDL_FUNC(SDL_GetWindowPosition);
    HG_LOAD_SDL_FUNC(SDL_SetWindowPosition);
    HG_LOAD_SDL_FUNC(SDL_SetWindowSize);
    HG_LOAD_SDL_FUNC(SDL_SetWindowTitle);
    HG_LOAD_SDL_FUNC(SDL_SetWindowOpacity);
    HG_LOAD_SDL_FUNC(SDL_ShowWindow);
    HG_LOAD_SDL_FUNC(SDL_RaiseWindow);
    HG_LOAD_SDL_FUNC(SDL_SetWindowParent);
    HG_LOAD_SDL_FUNC(SDL_GetWindowFlags);
    HG_LOAD_SDL_FUNC(SDL_GetWindowDisplayScale);
    HG_LOAD_SDL_FUNC(SDL_GetWindowRelativeMouseMode);
    HG_LOAD_SDL_FUNC(SDL_GetWindowFromID);
    HG_LOAD_SDL_FUNC(SDL_GetWindowProperties);
    HG_LOAD_SDL_FUNC(SDL_GetDisplays);
    HG_LOAD_SDL_FUNC(SDL_GetPrimaryDisplay);
    HG_LOAD_SDL_FUNC(SDL_GetFullscreenDisplayModes);
    HG_LOAD_SDL_FUNC(SDL_GetDisplayBounds);
    HG_LOAD_SDL_FUNC(SDL_GetDisplayUsableBounds);
    HG_LOAD_SDL_FUNC(SDL_GetDisplayContentScale);
    HG_LOAD_SDL_FUNC(SDL_WarpMouseInWindow);
    HG_LOAD_SDL_FUNC(SDL_WarpMouseGlobal);
    HG_LOAD_SDL_FUNC(SDL_GetGlobalMouseState);
    HG_LOAD_SDL_FUNC(SDL_GetMouseFocus);
    HG_LOAD_SDL_FUNC(SDL_GetKeyboardFocus);
    HG_LOAD_SDL_FUNC(SDL_CaptureMouse);
    HG_LOAD_SDL_FUNC(SDL_GetKeyName);
    HG_LOAD_SDL_FUNC(SDL_GetScancodeName);
    HG_LOAD_SDL_FUNC(SDL_HasClipboardText);
    HG_LOAD_SDL_FUNC(SDL_GetClipboardText);
    HG_LOAD_SDL_FUNC(SDL_SetClipboardText);
    HG_LOAD_SDL_FUNC(SDL_CreateSystemCursor);
    HG_LOAD_SDL_FUNC(SDL_DestroyCursor);
    HG_LOAD_SDL_FUNC(SDL_SetCursor);
    HG_LOAD_SDL_FUNC(SDL_ShowCursor);
    HG_LOAD_SDL_FUNC(SDL_HideCursor);
    HG_LOAD_SDL_FUNC(SDL_SetHint);
    HG_LOAD_SDL_FUNC(SDL_PollEvent);
    HG_LOAD_SDL_FUNC(SDL_GetTicksNS);
    HG_LOAD_SDL_FUNC(SDL_GetPerformanceCounter);
    HG_LOAD_SDL_FUNC(SDL_GetPerformanceFrequency);
    HG_LOAD_SDL_FUNC(SDL_GetVersion);
    HG_LOAD_SDL_FUNC(SDL_GL_CreateContext);
    HG_LOAD_SDL_FUNC(SDL_GL_DestroyContext);
    HG_LOAD_SDL_FUNC(SDL_GL_GetCurrentContext);
    HG_LOAD_SDL_FUNC(SDL_GL_MakeCurrent);
    HG_LOAD_SDL_FUNC(SDL_GL_SetAttribute);
    HG_LOAD_SDL_FUNC(SDL_GL_SetSwapInterval);
    HG_LOAD_SDL_FUNC(SDL_GL_SwapWindow);
    HG_LOAD_SDL_FUNC(SDL_Vulkan_CreateSurface);
    HG_LOAD_SDL_FUNC(SDL_Vulkan_GetInstanceExtensions);
    HG_LOAD_SDL_FUNC(SDL_OpenAudioDevice);
    HG_LOAD_SDL_FUNC(SDL_CloseAudioDevice);
    HG_LOAD_SDL_FUNC(SDL_CreateAudioStream);
    HG_LOAD_SDL_FUNC(SDL_DestroyAudioStream);
    HG_LOAD_SDL_FUNC(SDL_BindAudioStream);
    HG_LOAD_SDL_FUNC(SDL_SetAudioStreamFormat);
    HG_LOAD_SDL_FUNC(SDL_ClearAudioStream);
    HG_LOAD_SDL_FUNC(SDL_PutAudioStreamData);
    HG_LOAD_SDL_FUNC(SDL_GetAudioStreamQueued);
    HG_LOAD_SDL_FUNC(SDL_SetAudioStreamGain);
    HG_LOAD_SDL_FUNC(SDL_StartTextInput);
    HG_LOAD_SDL_FUNC(SDL_StopTextInput);
    HG_LOAD_SDL_FUNC(SDL_TextInputActive);
    HG_LOAD_SDL_FUNC(SDL_SetTextInputArea);
    HG_LOAD_SDL_FUNC(SDL_OpenGamepad);
    HG_LOAD_SDL_FUNC(SDL_CloseGamepad);
    HG_LOAD_SDL_FUNC(SDL_GetGamepads);
    HG_LOAD_SDL_FUNC(SDL_GetGamepadAxis);
    HG_LOAD_SDL_FUNC(SDL_GetGamepadButton);
    HG_LOAD_SDL_FUNC(SDL_free);
    HG_LOAD_SDL_FUNC(SDL_OpenURL);
    HG_LOAD_SDL_FUNC(SDL_GetCurrentVideoDriver);
    HG_LOAD_SDL_FUNC(SDL_GetPointerProperty);

#undef HG_LOAD_SDL_FUNC

    return true;
}

void unloadSDL()
{
    libsdl = {};
}

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
