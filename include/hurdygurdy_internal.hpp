#ifndef HURDYGURDY_INTERNAL_HPP
#define HURDYGURDY_INTERNAL_HPP

#include "hurdygurdy.hpp"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

struct HgWindow {
    /**
     * Platform specific resources for a window
     */
    SDL_Window* sdlWindow;
    /**
     * The window's Vulkan surface
     */
    VkSurfaceKHR surface;
    /**
     * The swapchain
     */
    VkSwapchainKHR swapchain;
    /**
     * The swapchain image format
     */
    HgFormat format;
    /**
     * The number of swapchain images
     */
    u32 imageCount;
    /**
     * The width of the images 
     */
    u32 width;
    /**
     * The height of the images
     */
    u32 height;
    /**
     * The swapchain images
     */
    HgImage** images;
    /**
     * The swapchain image views
     */
    HgImageView** views;
    /**
     * How the swapchain images are used
     */
    HgImageUsageFlags imageUsage;
    /**
     * The swapchain image format
     */
    HgPresentMode presentMode;

    /**
     * The command buffers per image
     */
    HgCommandBuffer** cmds;
    /**
     * The fences per image
     */
    VkFence* frameFinished;
    /**
     * The semaphores per image to signal availability
     */
    VkSemaphore* imageAvailable;
    /**
     * The semaphores per image to signal presentability
     */
    VkSemaphore* readyToPresent;
    /**
     * The current frame
     */
    u32 currentFrame;
    /**
     * The current image
     */
    u32 currentImage;

    /**
     * The maximum number of events
     */
    u32 maxEvents;
    /**
     * The number of events since last update
     */
    u32 eventCount;
    /**
     * The key events on this window since last update
     */
    HgKeyEvent events[1];
};

#endif // HURDYGURDY_INTERNAL_HPP
