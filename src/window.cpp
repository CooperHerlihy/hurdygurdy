#include "hurdygurdy.hpp"

static VkFormat find_swapchain_format(VkSurfaceKHR surface) {
    hg_assert(surface != nullptr);

    HgArena& scratch = hg_get_scratch();
    HgArenaScope scratch_scope{scratch};

    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(hg_vk_physical_device, surface, &format_count, nullptr);
    VkSurfaceFormatKHR* formats = scratch.alloc<VkSurfaceFormatKHR>(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(hg_vk_physical_device, surface, &format_count, formats);

    for (usize i = 0; i < format_count; ++i) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return VK_FORMAT_R8G8B8A8_SRGB;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return VK_FORMAT_B8G8R8A8_SRGB;
    }
    hg_error("No supported swapchain formats\n");
}

void hg_internal_resize_window_swapchain(HgWindow* window) {
    if (window->width == 0 || window->height == 0)
        return;

    vkQueueWaitIdle(hg_vk_queue);

    if (window->cmds != nullptr)
        vkFreeCommandBuffers(hg_vk_device, hg_vk_cmd_pool, window->image_count, window->cmds);

    for (usize i = 0; i < window->image_count; ++i) {
        vkDestroyFence(hg_vk_device, window->frame_finished[i], nullptr);
    }
    for (usize i = 0; i < window->image_count; ++i) {
        vkDestroySemaphore(hg_vk_device, window->image_available[i], nullptr);
    }
    for (usize i = 0; i < window->image_count; ++i) {
        vkDestroySemaphore(hg_vk_device, window->ready_to_present[i], nullptr);
    }
    for (usize i = 0; i < window->image_count; ++i) {
        vkDestroyImageView(hg_vk_device, window->views[i], nullptr);
    }

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(hg_vk_physical_device, window->surface, &surface_capabilities);

    if (surface_capabilities.currentExtent.width != (u32)-1)
        window->width = surface_capabilities.currentExtent.width;
    if (surface_capabilities.currentExtent.height != (u32)-1)
        window->height = surface_capabilities.currentExtent.height;

    if ( window->width < surface_capabilities.minImageExtent.width || window->height < surface_capabilities.minImageExtent.height ||
        window->width > surface_capabilities.maxImageExtent.width || window->height > surface_capabilities.maxImageExtent.height
    ) {
        hg_warn("Could not create swapchain of the surface's size: %d, %d | min: %d, %d - max: %d, %d\n",
            window->width, window->height,
            surface_capabilities.minImageExtent.width, surface_capabilities.minImageExtent.height,
            surface_capabilities.maxImageExtent.width, surface_capabilities.maxImageExtent.height);
        return;
    }

    window->format = find_swapchain_format(window->surface);

    VkSwapchainCreateInfoKHR swapchain_info{};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = window->surface;
    swapchain_info.minImageCount = window->present_mode == VK_PRESENT_MODE_FIFO_KHR
        ? std::max(surface_capabilities.minImageCount, (u32)2)
        : std::min(surface_capabilities.minImageCount, surface_capabilities.maxImageCount - 1) + 1;
    swapchain_info.imageFormat = window->format;
    swapchain_info.imageExtent = {window->width, window->height};
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = window->image_usage;
    swapchain_info.preTransform = surface_capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = window->present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = window->swapchain;

    VkResult result = vkCreateSwapchainKHR(hg_vk_device, &swapchain_info, nullptr, &window->swapchain);
    if (window->swapchain == nullptr)
        hg_error("Failed to create swapchain: %s\n", hg_vk_result_string(result));

    vkGetSwapchainImagesKHR(hg_vk_device, window->swapchain, &window->image_count, nullptr);
    window->images = (VkImage*)realloc(window->images, sizeof(VkImage) * window->image_count);
    window->views = (VkImageView*)realloc(window->views, sizeof(VkImageView) * window->image_count);
    vkGetSwapchainImagesKHR(hg_vk_device, window->swapchain, &window->image_count, window->images);
    for (usize i = 0; i < window->image_count; ++i) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = window->images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = window->format;
        create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCreateImageView(hg_vk_device, &create_info, nullptr, &window->views[i]);
    }

    window->cmds = (VkCommandBuffer*)realloc(window->cmds, sizeof(VkCommandBuffer) * window->image_count);

    VkCommandBufferAllocateInfo cmd_alloc_info{};
    cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.pNext = nullptr;
    cmd_alloc_info.commandPool = hg_vk_cmd_pool;
    cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc_info.commandBufferCount = window->image_count;

    vkAllocateCommandBuffers(hg_vk_device, &cmd_alloc_info, window->cmds);

    window->frame_finished = (VkFence*)realloc(window->frame_finished, sizeof(VkFence) * window->image_count);
    for (usize i = 0; i < window->image_count; ++i) {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(hg_vk_device, &info, nullptr, &window->frame_finished[i]);
    }

    window->image_available = (VkSemaphore*)realloc(window->image_available, sizeof(VkSemaphore) * window->image_count);
    for (usize i = 0; i < window->image_count; ++i) {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(hg_vk_device, &info, nullptr, &window->image_available[i]);
    }

    window->ready_to_present = (VkSemaphore*)realloc(window->ready_to_present, sizeof(VkSemaphore) * window->image_count);
    for (usize i = 0; i < window->image_count; ++i) {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(hg_vk_device, &info, nullptr, &window->ready_to_present[i]);
    }

    vkDestroySwapchainKHR(hg_vk_device, swapchain_info.oldSwapchain, nullptr);
}

static VkPresentModeKHR find_swapchain_present_mode(
    VkSurfaceKHR surface,
    VkPresentModeKHR desired_mode
) {
    hg_assert(surface != nullptr);

    if (desired_mode == VK_PRESENT_MODE_FIFO_KHR)
        return desired_mode;

    u32 mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(hg_vk_physical_device, surface, &mode_count, nullptr);
    VkPresentModeKHR* present_modes = (VkPresentModeKHR*)alloca(mode_count * sizeof(*present_modes));
    vkGetPhysicalDeviceSurfacePresentModesKHR(hg_vk_physical_device, surface, &mode_count, present_modes);

    for (usize i = 0; i < mode_count; ++i) {
        if (present_modes[i] == desired_mode)
            return desired_mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

void hg_internal_create_window_swapchain(HgWindow* window, const HgWindowConfig& config) {
    window->present_mode = find_swapchain_present_mode(window->surface, config.desired_present_mode);
    window->image_usage = config.image_usage;
    hg_internal_resize_window_swapchain(window);
}

void hg_internal_destroy_window_swapchain(HgWindow* window) {
    vkFreeCommandBuffers(hg_vk_device, hg_vk_cmd_pool, window->image_count, window->cmds);
    free(window->cmds);

    for (usize i = 0; i < window->image_count; ++i) {
        vkDestroyFence(hg_vk_device, window->frame_finished[i], nullptr);
    }
    free(window->frame_finished);

    for (usize i = 0; i < window->image_count; ++i) {
        vkDestroySemaphore(hg_vk_device, window->image_available[i], nullptr);
    }
    free(window->image_available);

    for (usize i = 0; i < window->image_count; ++i) {
        vkDestroySemaphore(hg_vk_device, window->ready_to_present[i], nullptr);
    }
    free(window->ready_to_present);

    for (usize i = 0; i < window->image_count; ++i) {
        vkDestroyImageView(hg_vk_device, window->views[i], nullptr);
    }
    free(window->views);
    free(window->images);

    vkDestroySwapchainKHR(hg_vk_device, window->swapchain, nullptr);
}

VkCommandBuffer HgWindow::acquire_and_record() {
    hg_assert(hg_vk_device != nullptr);
    if (width == 0 || height == 0)
        return nullptr;

retry: // is this necessary? : TODO
    current_frame = (current_frame + 1) % image_count;

    vkWaitForFences(hg_vk_device, 1, &frame_finished[current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(hg_vk_device, 1, &frame_finished[current_frame]);

    VkResult result = vkAcquireNextImageKHR(
        hg_vk_device, swapchain, UINT64_MAX, image_available[current_frame], nullptr, &current_image);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        hg_internal_resize_window_swapchain(this);
        goto retry;
    }

    VkCommandBuffer cmd = cmds[current_frame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);
    return cmd;
}

void HgWindow::end_and_present(VkQueue queue) {
    hg_assert(queue != nullptr);

    VkCommandBuffer cmd = cmds[current_frame];
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &image_available[current_frame];
    VkPipelineStageFlags stage_flags{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit.pWaitDstStageMask = &stage_flags;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &ready_to_present[current_frame];

    vkQueueSubmit(queue, 1, &submit, frame_finished[current_frame]);

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &ready_to_present[current_frame];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image;

    vkQueuePresentKHR(queue, &present_info);
}

