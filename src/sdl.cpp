#include "hurdygurdy.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

struct PlatformState {
    u32 windowCapacity = 0;
    u32 windowWidth = 0;
    HgWindow* windowPool = nullptr;
    u32* windowFreeList = nullptr;
    u32 windowNext = 0;

    HgHashMap<SDL_WindowID, HgWindow*> windows;

    bool wasQuit = false;
    bool isKeyDown[HgKey_count]{};
    f32 mouseDeltaX = 0.0f;
    f32 mouseDeltaY = 0.0f;

    bool imguiInitialized = false;
};

static PlatformState state;

void hgInitPlatform(HgArena* arena, u32 maxWindows, u32 maxEvents)
{
    SDL_Init(
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_GAMEPAD |
        SDL_INIT_EVENTS);

    state.windowCapacity = maxWindows;
    state.windowFreeList = hgAlloc<u32>(arena, maxWindows);
    state.windowWidth = (u32)hgAlign(sizeof(HgWindow) + sizeof(HgKeyEvent) * (maxEvents - 1), alignof(HgWindow));
    state.windowPool = (HgWindow*)hgAlloc(arena, state.windowWidth * maxWindows, alignof(HgWindow));

    for (u32 i = 0; i < maxWindows; ++i)
    {
        state.windowFreeList[i] = i + 1;
    }
    state.windowNext = 0;

    state.windows = state.windows.create(arena, maxWindows * 2);

    SDL_GetMouseState(&state.mouseDeltaX, &state.mouseDeltaY);
}

void hgDeinitPlatform()
{
    SDL_Quit();
}

u32 hgGetPlatformVulkanExtensions(HgArena* arena, HgStringView** extBuffer)
{
    u32 extCount;
    const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&extCount);

    *extBuffer = hgAlloc<HgStringView>(arena, extCount);
    for (u32 i = 0; i < extCount; ++i)
    {
        (*extBuffer)[i] = exts[i];
    }

    return extCount;
}

static void createWindowImages(HgWindow* window)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    VkImage* swapImages = hgAlloc<VkImage>(scratch, window->imageCount);
    vkGetSwapchainImagesKHR(hgVkDevice, window->swapchain, &window->imageCount, swapImages);

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        window->images[i].image = swapImages[i];
        window->images[i].type = VK_IMAGE_TYPE_2D;
        window->images[i].format = window->format;
        window->images[i].width = window->width;
        window->images[i].height = window->height;
        window->images[i].depth = 1;
        window->images[i].mipLevels = 1;
        window->images[i].arrayLayers = 1;
        window->images[i].msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = window->format;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        VkResult viewResult = vkCreateImageView(hgVkDevice, &viewInfo, nullptr, &window->views[i].view);
        if (window->views[i].view == nullptr)
            hgError("Could not create VkImageView: %s\n", hgVkResultToStr(viewResult));

        window->views[i].image = &window->images[i];
        window->views[i].type = VK_IMAGE_VIEW_TYPE_2D;
        window->views[i].aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        window->views[i].baseMipLevel = 0;
        window->views[i].levelCount = 1;
        window->views[i].baseArrayLayer = 0;
        window->views[i].layerCount = 1;
    }
}

static void destroyWindowImages(HgWindow* window)
{
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroyImageView(hgVkDevice, window->views[i].view, nullptr);
    }
}

static void createWindowCmdchain(HgWindow* window)
{
    if (window->imageCount > 0) {
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.pNext = nullptr;
        cmdAllocInfo.commandPool = hgVkCmdPool;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount = window->imageCount;

        vkAllocateCommandBuffers(hgVkDevice, &cmdAllocInfo, window->cmds);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(hgVkDevice, &info, nullptr, &window->frameFinished[i]);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(hgVkDevice, &info, nullptr, &window->imageAvailable[i]);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(hgVkDevice, &info, nullptr, &window->readyToPresent[i]);
    }
}

static void destroyWindowCmdchain(HgWindow* window)
{
    if (window->cmds != nullptr)
        vkFreeCommandBuffers(hgVkDevice, hgVkCmdPool, window->imageCount, window->cmds);

    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroyFence(hgVkDevice, window->frameFinished[i], nullptr);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->imageAvailable[i], nullptr);
    }
    for (u32 i = 0; i < window->imageCount; ++i)
    {
        vkDestroySemaphore(hgVkDevice, window->readyToPresent[i], nullptr);
    }
}

static void resizeWindowSwapchain(HgWindow* window)
{
    if (window->width == 0 || window->height == 0)
        return;

    vkQueueWaitIdle(hgVkQueue);

    destroyWindowCmdchain(window);
    destroyWindowImages(window);

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(hgVkPhysicalDevice, window->surface, &surfaceCapabilities);

    if (surfaceCapabilities.currentExtent.width != (u32)-1)
        window->width = surfaceCapabilities.currentExtent.width;
    if (surfaceCapabilities.currentExtent.height != (u32)-1)
        window->height = surfaceCapabilities.currentExtent.height;

    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = window->surface;
    swapchainInfo.minImageCount
        = std::min(surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount - 1) + 1;
    swapchainInfo.imageFormat = window->format;
    swapchainInfo.imageExtent = {window->width, window->height};
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = window->imageUsage;
    swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = window->presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = window->swapchain;

    VkResult result = vkCreateSwapchainKHR(hgVkDevice, &swapchainInfo, nullptr, &window->swapchain);
    if (window->swapchain == nullptr)
        hgError("Failed to create swapchain: %s\n", hgVkResultToStr(result));

    u32 swapImageCount;
    vkGetSwapchainImagesKHR(hgVkDevice, window->swapchain, &swapImageCount, nullptr);

    if (window->imageCount != swapImageCount)
    {
        window->images = (HgImage*)realloc(window->images, sizeof(HgImage) * swapImageCount);
        window->views = (HgImageView*)realloc(window->views, sizeof(HgImageView) * swapImageCount);

        window->cmds = (VkCommandBuffer*)realloc(window->cmds, sizeof(VkCommandBuffer) * swapImageCount);
        window->frameFinished = (VkFence*)realloc(window->frameFinished, sizeof(VkFence) * swapImageCount);
        window->imageAvailable = (VkSemaphore*)realloc(window->imageAvailable, sizeof(VkSemaphore) * swapImageCount);
        window->readyToPresent = (VkSemaphore*)realloc(window->readyToPresent, sizeof(VkSemaphore) * swapImageCount);

        window->imageCount = swapImageCount;
    }
    createWindowImages(window);
    createWindowCmdchain(window);

    vkDestroySwapchainKHR(hgVkDevice, swapchainInfo.oldSwapchain, nullptr);
}

static VkFormat findSwapchainFormat(VkSurfaceKHR surface)
{
    hgAssert(surface != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(hgVkPhysicalDevice, surface, &formatCount, nullptr);
    VkSurfaceFormatKHR* formats = hgAlloc<VkSurfaceFormatKHR>(scratch, formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(hgVkPhysicalDevice, surface, &formatCount, formats);

    for (u32 i = 0; i < formatCount; ++i)
    {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return VK_FORMAT_R8G8B8A8_SRGB;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return VK_FORMAT_B8G8R8A8_SRGB;
    }
    hgError("No supported swapchain formats\n");
}

static VkPresentModeKHR findSwapchainPresentMode(
    VkSurfaceKHR surface,
    VkPresentModeKHR desiredMode)
{
    hgAssert(surface != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    if (desiredMode == VK_PRESENT_MODE_FIFO_KHR)
        return desiredMode;

    u32 modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(hgVkPhysicalDevice, surface, &modeCount, nullptr);
    VkPresentModeKHR* presentModes = hgAlloc<VkPresentModeKHR>(scratch, modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(hgVkPhysicalDevice, surface, &modeCount, presentModes);

    for (u32 i = 0; i < modeCount; ++i)
    {
        if (presentModes[i] == desiredMode)
            return desiredMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static void createWindowSwapchain(HgWindow* window, const HgCreateWindow* config)
{
    window->format = findSwapchainFormat(window->surface);
    window->presentMode = findSwapchainPresentMode(window->surface, config->preferredPresentMode);
    window->imageUsage = config->imageUsage;
    resizeWindowSwapchain(window);
}

static void destroyWindowSwapchain(HgWindow* window)
{
    destroyWindowCmdchain(window);
    destroyWindowImages(window);

    vkDestroySwapchainKHR(hgVkDevice, window->swapchain, nullptr);

    free(window->cmds);
    free(window->frameFinished);
    free(window->imageAvailable);
    free(window->readyToPresent);
    free(window->views);
    free(window->images);
}

VkCommandBuffer HgWindow::beginRecording()
{
    hgAssert(hgVkDevice != nullptr);
    if (width == 0 || height == 0)
        return nullptr;

retry:
    currentFrame = (currentFrame + 1) % imageCount;
    vkWaitForFences(hgVkDevice, 1, &frameFinished[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(hgVkDevice, 1, &frameFinished[currentFrame]);

    VkResult result = vkAcquireNextImageKHR(
        hgVkDevice, swapchain, UINT64_MAX, imageAvailable[currentFrame], nullptr, &currentImage);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        resizeWindowSwapchain(this);
        goto retry;
    }
    if (result != VK_SUCCESS)
        hgError("Could not acquire next image: %s", hgVkResultToStr(result));

    VkCommandBuffer cmd = cmds[currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    return cmd;
}

void HgWindow::endAndPresent(VkCommandBuffer cmd)
{
    hgAssert(cmd == cmds[currentFrame]);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &imageAvailable[currentFrame];
    VkPipelineStageFlags stageFlags{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit.pWaitDstStageMask = &stageFlags;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &readyToPresent[currentFrame];

    vkQueueSubmit(hgVkQueue, 1, &submit, frameFinished[currentFrame]);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &readyToPresent[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImage;

    vkQueuePresentKHR(hgVkQueue, &presentInfo);
}

HgWindow* hgCreateWindow(const HgCreateWindow* config)
{
    u32 windowIdx = state.windowNext;
    hgAssert(windowIdx < state.windowCapacity);
    state.windowNext = state.windowFreeList[windowIdx];
    state.windowFreeList[windowIdx] = windowIdx;

    HgWindow* window = &state.windowPool[windowIdx];
    *window = {};

    const char* title = config->title != nullptr ? config->title : "Hurdy Gurdy";

    if (config->fullscreen)
    {
        int modeCount = 0;
        SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &modeCount);

        window->internals = SDL_CreateWindow(
            title,
            modes[0]->w,
            modes[0]->h,
            SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN);

        SDL_free(modes);
    } else {
        window->internals = SDL_CreateWindow(
            title,
            config->width,
            config->height,
            SDL_WINDOW_VULKAN | (config->fixedSize ? 0 : SDL_WINDOW_RESIZABLE));
    }

    bool success = SDL_Vulkan_CreateSurface((SDL_Window*)window->internals, hgVkInstance, nullptr, &window->surface);
    if (!success || window->surface == nullptr)
        hgError("Failed to create Vulkan surface: %s\n", SDL_GetError());

    SDL_WindowID windowID = SDL_GetWindowID((SDL_Window*)window->internals);
    state.windows.add(windowID, window);

    SDL_GetWindowSize((SDL_Window*)window->internals, (int*)&window->width, (int*)&window->height);
    createWindowSwapchain(window, config);

    return window;
}

void hgDestroyWindow(HgWindow* window)
{
    u32 windowIdx = (u32)(window - state.windowPool);
    hgAssert(windowIdx < state.windowCapacity);
    hgAssert(state.windowFreeList[windowIdx] == windowIdx);

    destroyWindowSwapchain(window);

    vkDestroySurfaceKHR(hgVkInstance, window->surface, nullptr);
    SDL_DestroyWindow((SDL_Window*)window->internals);

    state.windowFreeList[windowIdx] = state.windowNext;
    state.windowNext = windowIdx;
}

static HgKey sdlKeycodeToHgKey(u32 key)
{
    switch (key)
    {
        case SDLK_0:
            return HgKey_k0;
        case SDLK_1:
            return HgKey_k1;
        case SDLK_2:
            return HgKey_k2;
        case SDLK_3:
            return HgKey_k3;
        case SDLK_4:
            return HgKey_k4;
        case SDLK_5:
            return HgKey_k5;
        case SDLK_6:
            return HgKey_k6;
        case SDLK_7:
            return HgKey_k7;
        case SDLK_8:
            return HgKey_k8;
        case SDLK_9:
            return HgKey_k9;

        case SDLK_Q:
            return HgKey_q;
        case SDLK_W:
            return HgKey_w;
        case SDLK_E:
            return HgKey_e;
        case SDLK_R:
            return HgKey_r;
        case SDLK_T:
            return HgKey_t;
        case SDLK_Y:
            return HgKey_y;
        case SDLK_U:
            return HgKey_u;
        case SDLK_I:
            return HgKey_i;
        case SDLK_O:
            return HgKey_o;
        case SDLK_P:
            return HgKey_p;
        case SDLK_A:
            return HgKey_a;
        case SDLK_S:
            return HgKey_s;
        case SDLK_D:
            return HgKey_d;
        case SDLK_F:
            return HgKey_f;
        case SDLK_G:
            return HgKey_g;
        case SDLK_H:
            return HgKey_h;
        case SDLK_J:
            return HgKey_j;
        case SDLK_K:
            return HgKey_k;
        case SDLK_L:
            return HgKey_l;
        case SDLK_Z:
            return HgKey_z;
        case SDLK_X:
            return HgKey_x;
        case SDLK_C:
            return HgKey_c;
        case SDLK_V:
            return HgKey_v;
        case SDLK_B:
            return HgKey_b;
        case SDLK_N:
            return HgKey_n;
        case SDLK_M:
            return HgKey_m;

        case SDLK_SEMICOLON:
            return HgKey_semicolon;
        case SDLK_COLON:
            return HgKey_colon;
        case SDLK_APOSTROPHE:
            return HgKey_apostrophe;
        case SDLK_DBLAPOSTROPHE:
            return HgKey_quotation;
        case SDLK_COMMA:
            return HgKey_comma;
        case SDLK_PERIOD:
            return HgKey_period;
        case SDLK_QUESTION:
            return HgKey_question;
        case SDLK_GRAVE:
            return HgKey_grave;
        case SDLK_TILDE:
            return HgKey_tilde;
        case SDLK_EXCLAIM:
            return HgKey_exclamation;
        case SDLK_AT:
            return HgKey_at;
        case SDLK_HASH:
            return HgKey_hash;
        case SDLK_DOLLAR:
            return HgKey_dollar;
        case SDLK_PERCENT:
            return HgKey_percent;
        case SDLK_CARET:
            return HgKey_carot;
        case SDLK_AMPERSAND:
            return HgKey_ampersand;
        case SDLK_ASTERISK:
            return HgKey_asterisk;

        case SDLK_LEFTPAREN:
            return HgKey_lparen;
        case SDLK_RIGHTPAREN:
            return HgKey_rparen;
        case SDLK_LEFTBRACKET:
            return HgKey_lbracket;
        case SDLK_RIGHTBRACKET:
            return HgKey_rbracket;
        case SDLK_LEFTBRACE:
            return HgKey_lbrace;
        case SDLK_RIGHTBRACE:
            return HgKey_rbrace;

        case SDLK_EQUALS:
            return HgKey_equal;
        case SDLK_LESS:
            return HgKey_less;
        case SDLK_GREATER:
            return HgKey_greater;
        case SDLK_PLUS:
            return HgKey_plus;
        case SDLK_MINUS:
            return HgKey_minus;
        case SDLK_SLASH:
            return HgKey_slash;
        case SDLK_BACKSLASH:
            return HgKey_backslash;
        case SDLK_UNDERSCORE:
            return HgKey_underscore;
        case SDLK_PIPE:
            return HgKey_bar;

        case SDLK_UP:
            return HgKey_up;
        case SDLK_DOWN:
            return HgKey_down;
        case SDLK_LEFT:
            return HgKey_left;
        case SDLK_RIGHT:
            return HgKey_right;

        case SDLK_ESCAPE:
            return HgKey_escape;
        case SDLK_SPACE:
            return HgKey_space;
        case SDLK_RETURN:
            return HgKey_enter;
        case SDLK_BACKSPACE:
            return HgKey_backspace;
        case SDLK_DELETE:
            return HgKey_kdelete;
        case SDLK_INSERT:
            return HgKey_insert;
        case SDLK_TAB:
            return HgKey_tab;
        case SDLK_HOME:
            return HgKey_home;
        case SDLK_END:
            return HgKey_end;

        case SDLK_F1:
            return HgKey_f1;
        case SDLK_F2:
            return HgKey_f2;
        case SDLK_F3:
            return HgKey_f3;
        case SDLK_F4:
            return HgKey_f4;
        case SDLK_F5:
            return HgKey_f5;
        case SDLK_F6:
            return HgKey_f6;
        case SDLK_F7:
            return HgKey_f7;
        case SDLK_F8:
            return HgKey_f8;
        case SDLK_F9:
            return HgKey_f9;
        case SDLK_F10:
            return HgKey_f10;
        case SDLK_F11:
            return HgKey_f11;
        case SDLK_F12:
            return HgKey_f12;

        case SDLK_LSHIFT:
            return HgKey_lshift;
        case SDLK_RSHIFT:
            return HgKey_rshift;
        case SDLK_LCTRL:
            return HgKey_lctrl;
        case SDLK_RCTRL:
            return HgKey_rctrl;
        case SDLK_LALT:
            return HgKey_lalt;
        case SDLK_RALT:
            return HgKey_ralt;
        case SDLK_LGUI:
            return HgKey_lsuper;
        case SDLK_RGUI:
            return HgKey_rsuper;
        case SDLK_CAPSLOCK:
            return HgKey_capslock;
    }
    return HgKey_none;
}

static HgKey sdlButtonToHgKey(u32 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT:
            return HgKey_mouse1;
        case SDL_BUTTON_RIGHT:
            return HgKey_mouse2;
        case SDL_BUTTON_MIDDLE:
            return HgKey_mouse3;
        case SDL_BUTTON_X1:
            return HgKey_mouse4;
        case SDL_BUTTON_X2:
            return HgKey_mouse5;
    }
    return HgKey_none;
}

void hgProcessEvents()
{
    f32 oldMouseX, oldMouseY;
    SDL_GetMouseState(&oldMouseX, &oldMouseY);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (state.imguiInitialized)
            ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                state.wasQuit = true;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
            {
                HgWindow* window = *state.windows.get(event.window.windowID);
                SDL_GetWindowSize((SDL_Window*)window->internals, (int*)&window->width, (int*)&window->height);
                resizeWindowSwapchain(window);
            } break;
            case SDL_EVENT_KEY_DOWN:
            {
                HgKeyEvent windowEvent{};
                windowEvent.type = HgKeyEventType_keyPress;
                windowEvent.key = sdlKeycodeToHgKey(event.key.key);
                state.isKeyDown[windowEvent.key] = true;

                HgWindow* window = *state.windows.get(event.key.windowID);
                window->events[window->eventCount++] = windowEvent;
            } break;
            case SDL_EVENT_KEY_UP:
            {
                HgKeyEvent windowEvent{};
                windowEvent.type = HgKeyEventType_keyRelease;
                windowEvent.key = sdlKeycodeToHgKey(event.key.key);
                state.isKeyDown[windowEvent.key] = false;

                HgWindow* window = *state.windows.get(event.key.windowID);
                window->events[window->eventCount++] = windowEvent;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                HgKeyEvent windowEvent{};
                windowEvent.type = HgKeyEventType_keyPress;
                windowEvent.key = sdlButtonToHgKey(event.button.button);
                state.isKeyDown[windowEvent.key] = true;

                HgWindow* window = *state.windows.get(event.button.windowID);
                window->events[window->eventCount++] = windowEvent;
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                HgKeyEvent windowEvent{};
                windowEvent.type = HgKeyEventType_keyRelease;
                windowEvent.key = sdlButtonToHgKey(event.button.button);
                state.isKeyDown[windowEvent.key] = false;

                HgWindow* window = *state.windows.get(event.button.windowID);
                window->events[window->eventCount++] = windowEvent;
            } break;
        }
    }

    f32 newMouseX, newMouseY;
    SDL_GetMouseState(&newMouseX, &newMouseY);
    state.mouseDeltaX = newMouseX - oldMouseX;
    state.mouseDeltaY = newMouseY - oldMouseY;
}

bool hgWasQuit()
{
    return state.wasQuit;
}

HgKeyEvent* hgGetKeyEvents(HgWindow* window, u32* count)
{
    hgAssert(window != nullptr);
    hgAssert(count != nullptr);

    *count = window->eventCount;
    return window->events;
}

bool hgIsKeyDown(HgKey key)
{
    return state.isKeyDown[key];
}

void hgGetMousePos(f32* x, f32* y)
{
    SDL_GetMouseState(x, y);
}

void hgGetMouseDelta(f32* x, f32* y)
{
    *x = state.mouseDeltaX;
    *y = state.mouseDeltaY;
}

bool hgIsMouseFocused(HgWindow* window)
{
    return SDL_GetMouseFocus() == (SDL_Window*)window->internals;
}

void hgGetWindowSize(HgWindow* window, u32* x, u32* y)
{
    *x = window->width;
    *y = window->height;
}

void ImGui_ImplHurdyGurdy_Init(
    HgWindow* window,
    u32 colorAttachmentCount,
    const VkFormat* colorFormats,
    VkFormat depthFormat,
    VkFormat stencilFormat)
{
    ImGui_ImplSDL3_InitForVulkan((SDL_Window*)window->internals);

    ImGui_ImplVulkan_InitInfo imguiInfo{};
    imguiInfo.Instance = hgVkInstance;
    imguiInfo.PhysicalDevice = hgVkPhysicalDevice;
    imguiInfo.Device = hgVkDevice;
    imguiInfo.QueueFamily = hgVkQueueFamily;
    imguiInfo.Queue = hgVkQueue;
    imguiInfo.DescriptorPoolSize = 1000;
    imguiInfo.MinImageCount = window->imageCount;
    imguiInfo.ImageCount = window->imageCount;
    imguiInfo.MinAllocationSize = 1 << 20;
    imguiInfo.UseDynamicRendering = true;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = colorAttachmentCount;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = stencilFormat;
#ifdef HG_VK_DEBUG_MESSENGER
    imguiInfo.CheckVkResultFn = [](VkResult err)
    {
        if (err != VK_SUCCESS)
            hgWarn("Vulkan error from ImGui: %s\n", hgVkResultToStr(err));
    };
#endif

    ImGui_ImplVulkan_Init(&imguiInfo);

    state.imguiInitialized = true;
}

void ImGui_ImplHurdyGurdy_Shutdown()
{
    state.imguiInitialized = false;

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void ImGui_ImplHurdyGurdy_NewFrame()
{
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
}

void ImGui_ImplHurdyGurdy_Draw(VkCommandBuffer cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

