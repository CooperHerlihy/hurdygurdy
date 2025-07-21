#include "hg_window.h"

namespace hg {

Window Window::create(const bool fullscreen, const glm::ivec2 size) {
    if (!fullscreen)
        ASSERT(size.x > 0 && size.y > 0);

    Window window{};
    if (fullscreen) {
        SDL_DisplayID display = SDL_GetPrimaryDisplay();
        if (display == 0)
            ERRORF("Could not get primary display: {}", SDL_GetError());

        int count = 0;
        SDL_DisplayMode** mode = SDL_GetFullscreenDisplayModes(display, &count);
        if (mode == nullptr)
            ERRORF("Could not get display modes: {}", SDL_GetError());
        if (count == 0)
            ERROR("No fullscreen modes available");

        window.m_window = SDL_CreateWindow(
            "Hurdy Gurdy",
            mode[0]->w, mode[0]->h,
            SDL_WINDOW_FULLSCREEN | SDL_WINDOW_VULKAN
        );
    } else {
        window.m_window = SDL_CreateWindow(
            "Hurdy Gurdy",
            size.x, size.y,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
        );
    }

    if (window.m_window == nullptr)
        ERRORF("Could not create window: {}", SDL_GetError());
    return window;
}

} // namespace hg
