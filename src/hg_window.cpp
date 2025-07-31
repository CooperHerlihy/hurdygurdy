#include "hg_window.h"

namespace hg {

void destroy_window(Vk& vk, Window& window) {
    ASSERT(window.window != nullptr);
    ASSERT(window.surface != nullptr);
    ASSERT(window.swapchain.swapchain != nullptr);

    destroy_swapchain(vk, window.swapchain);
    vkDestroySurfaceKHR(vk.instance, window.surface, nullptr);
    SDL_DestroyWindow(window.window);
}

Result<Window> create_window(Vk& vk, glm::ivec2 size) {
    ASSERT(size.x > 0 && size.y > 0);

    auto window = ok<Window>();

    window->window = SDL_CreateWindow(
        "Hurdy Gurdy",
        size.x, size.y,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
    );
    if (window->window == nullptr)
        ERRORF("Could not create window: {}", SDL_GetError());

    window->surface = create_surface(vk, window->window);

    auto swapchain = create_swapchain(vk, window->surface);
    if (swapchain.has_err())
        ERRORF("Could not create swapchain: {}", to_string(swapchain.err()));
    window->swapchain = *swapchain;

    return window;
}

Result<Window> create_fullscreen_window(Vk& vk) {
    auto window = ok<Window>();

    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    if (display == 0)
        ERRORF("Could not get primary display: {}", SDL_GetError());

    int count = 0;
    SDL_DisplayMode** mode = SDL_GetFullscreenDisplayModes(display, &count);
    if (mode == nullptr)
        ERRORF("Could not get display modes: {}", SDL_GetError());
    if (count == 0)
        ERROR("No fullscreen modes available");

    window->window = SDL_CreateWindow(
        "Hurdy Gurdy",
        mode[0]->w, mode[0]->h,
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_VULKAN
    );
    if (window->window == nullptr)
        ERRORF("Could not create window: {}", SDL_GetError());

    window->surface = create_surface(vk, window->window);

    auto swapchain = create_swapchain(vk, window->surface);
    if (swapchain.has_err())
        ERRORF("Could not create swapchain: {}", to_string(swapchain.err()));
    window->swapchain = *swapchain;

    return window;
}

Result<void> resize_window(Vk& vk, Window& window) {
    ASSERT(window.window != nullptr);
    ASSERT(window.surface != nullptr);
    ASSERT(window.swapchain.swapchain != nullptr);

    auto swapchain = resize_swapchain(vk, window.swapchain, window.surface);
    if (swapchain.has_err())
        return swapchain.err();
    return ok();
}

glm::ivec2 get_window_size(Window window) {
    ASSERT(window.window != nullptr);

    int width = 0, height = 0;
    SDL_GetWindowSize(window.window, &width, &height);
    return {width, height};
}

} // namespace hg
