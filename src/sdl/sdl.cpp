#include "sdl_internal.hpp"
#include "sdl_platform.hpp"
#include "hg/error.hpp"

namespace hg::sdl {

bool initPlatform()
{
    if (!loadSDL())
        return false;

    if (!SDL_Init(
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_GAMEPAD |
        SDL_INIT_EVENTS))
    {
        setError(static_cast<StringView>(SDL_GetError()));
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
    SDL_Quit();
sdlFailed:
    return false;
}

void deinitPlatform()
{
    deinitAudio();
    windowDeinit();
    SDL_Quit();
}

Span<StringView> getPlatformVulkanExtensions(Arena* arena)
{
    u32 extCount = 0;
    const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&extCount);
    if (exts == nullptr)
        HG_PANIC("SDL could not get Vulkan instance extensions: %s\n", SDL_GetError());

    Span<StringView> extBuffer{arena->alloc<StringView>(extCount), extCount};
    for (u32 i = 0; i < extCount; ++i)
    {
        extBuffer[i] = exts[i];
    }

    return extBuffer;
}

} // namespace hg::sdl
