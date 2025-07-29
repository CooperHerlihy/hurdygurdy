#include "hg_window.h"

namespace hg {

SDL_Window* create_window(const glm::ivec2 size) {
    ASSERT(size.x > 0 && size.y > 0);

    SDL_Window* window = SDL_CreateWindow(
        "Hurdy Gurdy",
        size.x, size.y,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
    );
    if (window == nullptr)
        ERRORF("Could not create window: {}", SDL_GetError());

    return window;
}

SDL_Window* create_fullscreen_window() {
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    if (display == 0)
        ERRORF("Could not get primary display: {}", SDL_GetError());

    int count = 0;
    SDL_DisplayMode** mode = SDL_GetFullscreenDisplayModes(display, &count);
    if (mode == nullptr)
        ERRORF("Could not get display modes: {}", SDL_GetError());
    if (count == 0)
        ERROR("No fullscreen modes available");

    SDL_Window* window = SDL_CreateWindow(
        "Hurdy Gurdy",
        mode[0]->w, mode[0]->h,
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_VULKAN
    );
    if (window == nullptr)
        ERRORF("Could not create window: {}", SDL_GetError());

    return window;
}

} // namespace hg
