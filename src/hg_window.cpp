#include "hg_window.h"
#include "SDL3/SDL_video.h"

namespace hg {

Window Window::create(const bool fullscreen, const i32 width, const i32 height) {
    if (!fullscreen)
        ASSERT(width > 0 && height > 0);

    Window window{};
    window.m_window = SDL_CreateWindow(
        "Hurdy Gurdy", width, height, SDL_WINDOW_VULKAN
        | (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE)
    );
    if (window.m_window == nullptr)
        ERRORF("Could not create window: {}", SDL_GetError());
    return window;
}

} // namespace hg
