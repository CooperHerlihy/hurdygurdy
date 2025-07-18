#pragma once

#include "hg_pch.h"
#include "hg_utils.h"

namespace hg {

class Window {
public:
    [[nodiscard]] SDL_Window* get() const {
        ASSERT(m_window != nullptr);
        return m_window;
    }

    [[nodiscard]] glm::ivec2 get_extent() const {
        int width = 0, height = 0;
        SDL_GetWindowSize(m_window, &width, &height);
        return {width, height};
    }

    [[nodiscard]] static Window create(bool fullscreen, glm::ivec2 size);
    void destroy() const {
        ASSERT(m_window != nullptr);
        SDL_DestroyWindow(m_window);
    }

private:
    SDL_Window* m_window = nullptr;
};

} // namespace hg
