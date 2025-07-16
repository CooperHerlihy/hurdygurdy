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

    [[nodiscard]] vk::Extent2D get_extent() const {
        int width = 0, height = 0;
        SDL_GetWindowSize(m_window, &width, &height);
        return {to_u32(width), to_u32(height)};
    }

    [[nodiscard]] static Window create(bool fullscreen, i32 width, i32 height);
    void destroy() const {
        ASSERT(m_window != nullptr);
        SDL_DestroyWindow(m_window);
    }

private:
    SDL_Window* m_window = nullptr;
};

} // namespace hg
