#pragma once

#include "hg_pch.h"
#include "hg_utils.h"

namespace hg {

SDL_Window* create_window(const glm::ivec2 size);
SDL_Window* create_fullscreen_window();

inline void destroy_window(SDL_Window* window) {
    ASSERT(window != nullptr);
    SDL_DestroyWindow(window);
}

[[nodiscard]] inline glm::ivec2 get_window_extent(SDL_Window* window) {
    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    return {width, height};
}

} // namespace hg
