#include "internal.hpp"
#include "sdl/sdl_backend.hpp"
#include "hg_error.hpp"

namespace hg {
namespace sdl {

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

namespace internal {

bool initPlatform()
{
    if (!sdl::loadSDL())
        return false;

    if (!SDL_Init(
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_GAMEPAD |
        SDL_INIT_EVENTS))
    {
        setError(static_cast<StringView>(SDL_GetError()));
        sdl::unloadSDL();
        return false;
    }
    return true;
}

void deinitPlatform()
{
    SDL_Quit();
    sdl::unloadSDL();
}

Span<StringView> platformGetVulkanExtensions(Arena* arena)
{
    u32 extCount;
    const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&extCount);

    Span<StringView> extBuffer{arena->alloc<StringView>(extCount), extCount};
    for (u32 i = 0; i < extCount; ++i)
    {
        extBuffer[i] = exts[i];
    }

    return extBuffer;
}

} // namespace internal
} // namespace hg
