#include "internal.hpp"
#include "hg_error.hpp"

#include "SDL3/SDL.h"
#include <SDL3/SDL_vulkan.h>

namespace hg {

bool internal::initPlatform()
{
    if (!SDL_Init(
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_GAMEPAD |
        SDL_INIT_EVENTS))
    {
        setError(static_cast<StringView>(SDL_GetError()));
        return false;
    }
    return true;
}

void internal::deinitPlatform()
{
    SDL_Quit();
}

Span<StringView> internal::platformGetVulkanExtensions(Arena* arena)
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

} // namespace hg
