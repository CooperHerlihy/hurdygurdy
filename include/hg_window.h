#pragma once

#include "hg_utils.h"
#include "hg_vulkan.h"

namespace hg {

struct Window {
    SDL_Window* window{};
    VkSurfaceKHR surface{};
    Swapchain swapchain{};
};
void destroy_window(Vk& vk, Window& window);

[[nodiscard]] Result<Window> create_window(Vk& vk, glm::ivec2 size);
[[nodiscard]] Result<Window> create_fullscreen_window(Vk& vk);

[[nodiscard]] Result<void> resize_window(Vk& vk, Window& window);

[[nodiscard]] glm::ivec2 get_window_size(Window window);

} // namespace hg
