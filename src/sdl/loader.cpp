#include "sdl/sdl_internal.hpp"

#include "hg/error.hpp"
#include "hg/dynlib.hpp"

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

Library libsdl{};
SdlFuncs sdlFuncs{};

bool loadSdl()
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
    HG_LOAD_SDL_FUNC(SDL_SetWindowResizable);
    HG_LOAD_SDL_FUNC(SDL_SetWindowFullscreen);
    HG_LOAD_SDL_FUNC(SDL_GetWindowFlags);
    HG_LOAD_SDL_FUNC(SDL_GetDisplays);
    HG_LOAD_SDL_FUNC(SDL_GetDisplayBounds);
    HG_LOAD_SDL_FUNC(SDL_GetDisplayUsableBounds);
    HG_LOAD_SDL_FUNC(SDL_GetDisplayContentScale);
    HG_LOAD_SDL_FUNC(SDL_GetGlobalMouseState);
    HG_LOAD_SDL_FUNC(SDL_GetMouseState);
    HG_LOAD_SDL_FUNC(SDL_GetMouseFocus);
    HG_LOAD_SDL_FUNC(SDL_HasClipboardText);
    HG_LOAD_SDL_FUNC(SDL_GetClipboardText);
    HG_LOAD_SDL_FUNC(SDL_SetClipboardText);
    HG_LOAD_SDL_FUNC(SDL_CreateSystemCursor);
    HG_LOAD_SDL_FUNC(SDL_DestroyCursor);
    HG_LOAD_SDL_FUNC(SDL_SetCursor);
    HG_LOAD_SDL_FUNC(SDL_ShowCursor);
    HG_LOAD_SDL_FUNC(SDL_HideCursor);
    HG_LOAD_SDL_FUNC(SDL_PollEvent);
    HG_LOAD_SDL_FUNC(SDL_Vulkan_CreateSurface);
    HG_LOAD_SDL_FUNC(SDL_Vulkan_GetInstanceExtensions);
    HG_LOAD_SDL_FUNC(SDL_OpenAudioDevice);
    HG_LOAD_SDL_FUNC(SDL_CloseAudioDevice);
    HG_LOAD_SDL_FUNC(SDL_CreateAudioStream);
    HG_LOAD_SDL_FUNC(SDL_DestroyAudioStream);
    HG_LOAD_SDL_FUNC(SDL_BindAudioStream);
    HG_LOAD_SDL_FUNC(SDL_SetAudioStreamGetCallback);
    HG_LOAD_SDL_FUNC(SDL_SetAudioStreamFormat);
    HG_LOAD_SDL_FUNC(SDL_PutAudioStreamData);
    HG_LOAD_SDL_FUNC(SDL_free);
    HG_LOAD_SDL_FUNC(SDL_OpenURL);
    HG_LOAD_SDL_FUNC(SDL_GetGamepads);
    HG_LOAD_SDL_FUNC(SDL_IsGamepad);
    HG_LOAD_SDL_FUNC(SDL_OpenGamepad);
    HG_LOAD_SDL_FUNC(SDL_CloseGamepad);
    HG_LOAD_SDL_FUNC(SDL_GetGamepadButton);
    HG_LOAD_SDL_FUNC(SDL_GetGamepadAxis);
    HG_LOAD_SDL_FUNC(SDL_GamepadConnected);
    HG_LOAD_SDL_FUNC(SDL_MaximizeWindow);
    HG_LOAD_SDL_FUNC(SDL_MinimizeWindow);
    HG_LOAD_SDL_FUNC(SDL_RestoreWindow);
    HG_LOAD_SDL_FUNC(SDL_GetTicksNS);

#undef HG_LOAD_SDL_FUNC

    return true;
}

} // namespace hg::sdl

extern "C" bool SDLCALL SDL_Init(SDL_InitFlags flags)
{
    return ::hg::sdl::sdlFuncs.SDL_Init(flags);
}

extern "C" void SDLCALL SDL_Quit()
{
    ::hg::sdl::sdlFuncs.SDL_Quit();
}

extern "C" const char* SDLCALL SDL_GetError()
{
    return ::hg::sdl::sdlFuncs.SDL_GetError();
}

extern "C" SDL_Window* SDLCALL SDL_CreateWindow(const char* title, int w, int h, SDL_WindowFlags flags)
{
    return ::hg::sdl::sdlFuncs.SDL_CreateWindow(title, w, h, flags);
}

extern "C" void SDLCALL SDL_DestroyWindow(SDL_Window* window)
{
    ::hg::sdl::sdlFuncs.SDL_DestroyWindow(window);
}

extern "C" SDL_WindowID SDLCALL SDL_GetWindowID(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowID(window);
}

extern "C" bool SDLCALL SDL_GetWindowSize(SDL_Window* window, int* w, int* h)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowSize(window, w, h);
}

extern "C" bool SDLCALL SDL_GetWindowSizeInPixels(SDL_Window* window, int* w, int* hp)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowSizeInPixels(window, w, hp);
}

extern "C" bool SDLCALL SDL_GetWindowPosition(SDL_Window* window, int* x, int* y)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowPosition(window, x, y);
}

extern "C" bool SDLCALL SDL_SetWindowPosition(SDL_Window* window, int x, int y)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowPosition(window, x, y);
}

extern "C" bool SDLCALL SDL_SetWindowSize(SDL_Window* window, int w, int h)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowSize(window, w, h);
}

extern "C" bool SDLCALL SDL_SetWindowTitle(SDL_Window* window, const char* title)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowTitle(window, title);
}

extern "C" bool SDLCALL SDL_SetWindowResizable(SDL_Window* window, bool resizable)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowResizable(window, resizable);
}

extern "C" bool SDLCALL SDL_SetWindowFullscreen(SDL_Window* window, bool fullscreen)
{
    return ::hg::sdl::sdlFuncs.SDL_SetWindowFullscreen(window, fullscreen);
}

extern "C" SDL_WindowFlags SDLCALL SDL_GetWindowFlags(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_GetWindowFlags(window);
}

extern "C" SDL_DisplayID* SDLCALL SDL_GetDisplays(int* count)
{
    return ::hg::sdl::sdlFuncs.SDL_GetDisplays(count);
}

extern "C" bool SDLCALL SDL_GetDisplayBounds(SDL_DisplayID displayID, SDL_Rect* rect)
{
    return ::hg::sdl::sdlFuncs.SDL_GetDisplayBounds(displayID, rect);
}

extern "C" bool SDLCALL SDL_GetDisplayUsableBounds(SDL_DisplayID displayID, SDL_Rect* rect)
{
    return ::hg::sdl::sdlFuncs.SDL_GetDisplayUsableBounds(displayID, rect);
}

extern "C" float SDLCALL SDL_GetDisplayContentScale(SDL_DisplayID displayID)
{
    return ::hg::sdl::sdlFuncs.SDL_GetDisplayContentScale(displayID);
}

extern "C" SDL_MouseButtonFlags SDLCALL SDL_GetGlobalMouseState(float* x, float* y)
{
    return ::hg::sdl::sdlFuncs.SDL_GetGlobalMouseState(x, y);
}

extern "C" SDL_MouseButtonFlags SDLCALL SDL_GetMouseState(float* x, float* y)
{
    return ::hg::sdl::sdlFuncs.SDL_GetMouseState(x, y);
}

extern "C" SDL_Window* SDLCALL SDL_GetMouseFocus()
{
    return ::hg::sdl::sdlFuncs.SDL_GetMouseFocus();
}

extern "C" bool SDLCALL SDL_HasClipboardText()
{
    return ::hg::sdl::sdlFuncs.SDL_HasClipboardText();
}

extern "C" char* SDLCALL SDL_GetClipboardText()
{
    return ::hg::sdl::sdlFuncs.SDL_GetClipboardText();
}

extern "C" bool SDLCALL SDL_SetClipboardText(const char* text)
{
    return ::hg::sdl::sdlFuncs.SDL_SetClipboardText(text);
}

extern "C" SDL_Cursor* SDLCALL SDL_CreateSystemCursor(SDL_SystemCursor id)
{
    return ::hg::sdl::sdlFuncs.SDL_CreateSystemCursor(id);
}

extern "C" void SDLCALL SDL_DestroyCursor(SDL_Cursor* cursor)
{
    ::hg::sdl::sdlFuncs.SDL_DestroyCursor(cursor);
}

extern "C" bool SDLCALL SDL_SetCursor(SDL_Cursor* cursor)
{
    return ::hg::sdl::sdlFuncs.SDL_SetCursor(cursor);
}

extern "C" bool SDLCALL SDL_ShowCursor()
{
    return ::hg::sdl::sdlFuncs.SDL_ShowCursor();
}

extern "C" bool SDLCALL SDL_HideCursor()
{
    return ::hg::sdl::sdlFuncs.SDL_HideCursor();
}

extern "C" bool SDLCALL SDL_PollEvent(SDL_Event* event)
{
    return ::hg::sdl::sdlFuncs.SDL_PollEvent(event);
}

extern "C" bool SDLCALL SDL_Vulkan_CreateSurface(SDL_Window* window, VkInstance instance,
    const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface)
{
    return ::hg::sdl::sdlFuncs.SDL_Vulkan_CreateSurface(window, instance, allocator, surface);
}

extern "C" const char* const* SDLCALL SDL_Vulkan_GetInstanceExtensions(Uint32* count)
{
    return ::hg::sdl::sdlFuncs.SDL_Vulkan_GetInstanceExtensions(count);
}

extern "C" SDL_AudioDeviceID SDLCALL SDL_OpenAudioDevice(SDL_AudioDeviceID devid, const SDL_AudioSpec* spec)
{
    return ::hg::sdl::sdlFuncs.SDL_OpenAudioDevice(devid, spec);
}

extern "C" void SDLCALL SDL_CloseAudioDevice(SDL_AudioDeviceID devid)
{
    ::hg::sdl::sdlFuncs.SDL_CloseAudioDevice(devid);
}

extern "C" SDL_AudioStream* SDLCALL SDL_CreateAudioStream(const SDL_AudioSpec* src_spec,
    const SDL_AudioSpec* dst_spec)
{
    return ::hg::sdl::sdlFuncs.SDL_CreateAudioStream(src_spec, dst_spec);
}

extern "C" void SDLCALL SDL_DestroyAudioStream(SDL_AudioStream* stream)
{
    ::hg::sdl::sdlFuncs.SDL_DestroyAudioStream(stream);
}

extern "C" bool SDLCALL SDL_BindAudioStream(SDL_AudioDeviceID devid, SDL_AudioStream* stream)
{
    return ::hg::sdl::sdlFuncs.SDL_BindAudioStream(devid, stream);
}

extern "C" bool SDLCALL SDL_SetAudioStreamGetCallback(SDL_AudioStream* stream, SDL_AudioStreamCallback callback, void *userdata)
{
    return ::hg::sdl::sdlFuncs.SDL_SetAudioStreamGetCallback(stream, callback, userdata);
}

extern "C" bool SDLCALL SDL_SetAudioStreamFormat(SDL_AudioStream* stream, const SDL_AudioSpec* src_spec,
    const SDL_AudioSpec* dst_spec)
{
    return ::hg::sdl::sdlFuncs.SDL_SetAudioStreamFormat(stream, src_spec, dst_spec);
}

extern "C" bool SDLCALL SDL_PutAudioStreamData(SDL_AudioStream* stream, const void* data, int len)
{
    return ::hg::sdl::sdlFuncs.SDL_PutAudioStreamData(stream, data, len);
}

extern "C" void SDLCALL SDL_free(void* mem)
{
    ::hg::sdl::sdlFuncs.SDL_free(mem);
}

extern "C" bool SDLCALL SDL_OpenURL(const char* url)
{
    return ::hg::sdl::sdlFuncs.SDL_OpenURL(url);
}

extern "C" SDL_JoystickID* SDLCALL SDL_GetGamepads(int* count)
{
    return ::hg::sdl::sdlFuncs.SDL_GetGamepads(count);
}

extern "C" bool SDLCALL SDL_IsGamepad(SDL_JoystickID instance_id)
{
    return ::hg::sdl::sdlFuncs.SDL_IsGamepad(instance_id);
}

extern "C" SDL_Gamepad* SDLCALL SDL_OpenGamepad(SDL_JoystickID instance_id)
{
    return ::hg::sdl::sdlFuncs.SDL_OpenGamepad(instance_id);
}

extern "C" void SDLCALL SDL_CloseGamepad(SDL_Gamepad* gamepad)
{
    ::hg::sdl::sdlFuncs.SDL_CloseGamepad(gamepad);
}

extern "C" bool SDLCALL SDL_GetGamepadButton(SDL_Gamepad* gamepad, SDL_GamepadButton button)
{
    return ::hg::sdl::sdlFuncs.SDL_GetGamepadButton(gamepad, button);
}

extern "C" Sint16 SDLCALL SDL_GetGamepadAxis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis)
{
    return ::hg::sdl::sdlFuncs.SDL_GetGamepadAxis(gamepad, axis);
}

extern "C" bool SDLCALL SDL_GamepadConnected(SDL_Gamepad* gamepad)
{
    return ::hg::sdl::sdlFuncs.SDL_GamepadConnected(gamepad);
}

extern "C" bool SDLCALL SDL_MaximizeWindow(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_MaximizeWindow(window);
}

extern "C" bool SDLCALL SDL_MinimizeWindow(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_MinimizeWindow(window);
}

extern "C" bool SDLCALL SDL_RestoreWindow(SDL_Window* window)
{
    return ::hg::sdl::sdlFuncs.SDL_RestoreWindow(window);
}

extern "C" Uint64 SDLCALL SDL_GetTicksNS()
{
    return ::hg::sdl::sdlFuncs.SDL_GetTicksNS();
}
