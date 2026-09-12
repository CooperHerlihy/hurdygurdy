#include "sdl_platform.hpp"

#include "sdl_internal.hpp"
#include "hg/error.hpp"
#include "hg/dynlib.hpp"

namespace hg::sdl {

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

bool initSdl()
{
    if (!sdlFuncs.SDL_Init(
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_GAMEPAD |
        SDL_INIT_EVENTS))
    {
        setError(sdlFuncs.SDL_GetError());
        goto sdlFailed;
    }

    if (!windowInit())
        goto windowFailed;

    if (!initAudio())
        goto audioFailed;

    return true;

audioFailed:
    windowDeinit();
windowFailed:
    sdlFuncs.SDL_Quit();
sdlFailed:
    return false;
}

void deinitSdl()
{
    deinitAudio();
    windowDeinit();
    sdlFuncs.SDL_Quit();
}

Span<StringView> getPlatformVulkanExtensions(Arena* arena)
{
    u32 extCount = 0;
    const char* const* exts = sdlFuncs.SDL_Vulkan_GetInstanceExtensions(&extCount);
    if (exts == nullptr)
        HG_PANIC("SDL could not get Vulkan instance extensions: %s\n", sdlFuncs.SDL_GetError());

    Span<StringView> extBuffer{arena->alloc<StringView>(extCount), extCount};
    for (u32 i = 0; i < extCount; ++i)
    {
        extBuffer[i] = exts[i];
    }

    return extBuffer;
}

} // namespace hg::sdl
