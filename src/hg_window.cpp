#include "hg_window.h"

namespace hg {

Window Window::create(const bool fullscreen, const glm::ivec2 size) {
    if (!fullscreen)
        ASSERT(size.x > 0 && size.y > 0);

    Window window{};
    window.m_window = SDL_CreateWindow(
        "Hurdy Gurdy", size.x, size.y, SDL_WINDOW_VULKAN
        | (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE)
    );
    if (window.m_window == nullptr)
        ERRORF("Could not create window: {}", SDL_GetError());
    return window;
}

} // namespace hg
