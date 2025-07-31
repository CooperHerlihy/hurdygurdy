#pragma once

#include "hg_utils.h"
#include "hg_engine.h"

namespace hg {

struct Window {
    SDL_Window* window{};
    VkSurfaceKHR surface{};
    Swapchain swapchain{};
};
void destroy_window(Engine& engine, Window& window);

[[nodiscard]] Result<Window> create_window(Engine& engine, glm::ivec2 size);
[[nodiscard]] Result<Window> create_fullscreen_window(Engine& engine);

[[nodiscard]] Result<void> resize_window(Engine& engine, Window& window);

[[nodiscard]] glm::ivec2 get_window_size(Window window);

} // namespace hg
