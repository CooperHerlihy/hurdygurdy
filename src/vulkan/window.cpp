#include "backend.hpp"

#include "hg_error.hpp"

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

namespace hg {
using namespace vulkan;

struct WindowData {
    VkSurfaceKHR surface = nullptr;
    VkSwapchainKHR swapchain = nullptr;
    Array<GpuImage> images{};
    Array<GpuView> views{};
    Array<VkSemaphore> imageAvailable{};
    Array<VkSemaphore> readyToPresent{};
    u32 imageIdx = 0;
    Format format = Format_undefined;
    GpuImageUsageFlags imageUsage = {};
    GpuPresentMode presentMode = {};

    SDL_Window* sdlWindow = nullptr;
    u32 width = 0;
    u32 height = 0;
    f32 mouseX = 0;
    f32 mouseY = 0;
    bool isKeyDown[Button_count]{};
    bool wasClosed = false;

    Array<WindowEvent> events;

    WindowData() noexcept = default;
    ~WindowData() noexcept;

    WindowData(WindowData&& other) noexcept
        : surface{std::exchange(other.surface, nullptr)}
        , swapchain{std::exchange(other.swapchain, nullptr)}
        , images{std::exchange(other.images, {})}
        , views{std::exchange(other.views, {})}
        , imageAvailable{std::exchange(other.imageAvailable, {})}
        , readyToPresent{std::exchange(other.readyToPresent, {})}
        , imageIdx{std::exchange(other.imageIdx, 0)}
        , format{std::exchange(other.format, Format_undefined)}
        , imageUsage{std::exchange(other.imageUsage, {})}
        , presentMode{std::exchange(other.presentMode, {})}
        , sdlWindow{std::exchange(other.sdlWindow, nullptr)}
        , width{std::exchange(other.width, 0)}
        , height{std::exchange(other.height, 0)}
        , mouseX{std::exchange(other.mouseX, 0.0f)}
        , mouseY{std::exchange(other.mouseY, 0.0f)}
        , wasClosed{std::exchange(other.wasClosed, false)}
    {
        memcpy(isKeyDown, other.isKeyDown, sizeof(isKeyDown));
        memset(other.isKeyDown, 0, sizeof(other.isKeyDown));
    }

    WindowData& operator=(WindowData&& other) noexcept
    {
        if (this != &other)
        {
            this->~WindowData();
            new (this) WindowData{std::move(other)};
        }
        return *this;
    }

    WindowData(const WindowData&) = delete;
    WindowData& operator=(const WindowData&) = delete;
};

struct WindowState {
    Map<SDL_WindowID, WindowData*> ids{};
    f32 mouseDX = 0.0f;
    f32 mouseDY = 0.0f;
    bool wasQuit = false;
    bool imguiInitialized = false;
};

static WindowState windowState{};

void initImGui(
    const Window& window,
    Format colorFormat,
    Format depthFormat,
    Format stencilFormat)
{
    HG_ASSERT(colorFormat != Format_undefined);

    ArenaScope scratch = getScratch();

    ImGui_ImplSDL3_InitForVulkan(window.data->sdlWindow);

    VkFormat colorVkFormat = formatToVk(colorFormat);
    VkFormat depthVkFormat = formatToVk(depthFormat);
    VkFormat stencilVkFormat = formatToVk(stencilFormat);

    ImGui_ImplVulkan_InitInfo imguiInfo{};
    imguiInfo.Instance = vk.instance;
    imguiInfo.PhysicalDevice = vk.physicalDevice;
    imguiInfo.Device = vk.device;
    imguiInfo.QueueFamily = vk.queueFamily;
    imguiInfo.Queue = vk.queue;
    imguiInfo.DescriptorPoolSize = 1000;
    imguiInfo.MinImageCount = static_cast<u32>(window.data->images.count);
    imguiInfo.ImageCount = static_cast<u32>(window.data->images.count);
    imguiInfo.MinAllocationSize = 1 << 20;
    imguiInfo.UseDynamicRendering = true;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorVkFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = depthVkFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = stencilVkFormat;

    ImGui_ImplVulkan_Init(&imguiInfo);

    windowState.imguiInitialized = true;
}

void deinitImGui()
{
    windowState.imguiInitialized = false;

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void* createImGuiTexture(const GpuView& view, GpuLayout layout)
{
    return ImGui_ImplVulkan_AddTexture(view.data->sampler, view.data->view, gpuLayoutToVk(layout));
}

void destroyImGuiTexture(void* texture)
{
    ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(texture));
}

void beginImGuiFrame()
{
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
}

void renderImGui(GpuCmd* cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), reinterpret_cast<VkCommandBuffer>(cmd));
}

WindowData::~WindowData() noexcept
{
    if (sdlWindow != nullptr)
    {
        for (GpuImage& image : images)
        {
            image.data->image = nullptr;
        }
        for (VkSemaphore semaphore : readyToPresent)
        {
            vkDestroySemaphore(vk.device, semaphore, nullptr);
        }
        for (VkSemaphore semaphore : imageAvailable)
        {
            vkDestroySemaphore(vk.device, semaphore, nullptr);
        }
        vkDestroySwapchainKHR(vk.device, swapchain, nullptr);
        vkDestroySurfaceKHR(vk.instance, surface, nullptr);

        windowState.ids.remove(SDL_GetWindowID(sdlWindow));
        SDL_DestroyWindow(sdlWindow);
    }
}

Window::Window() noexcept = default;
Window::~Window() noexcept = default;
Window::Window(Window&& other) noexcept = default;
Window& Window::operator=(Window&& other) noexcept = default;

GpuView* Window::imageView() const
{
    return data->imageIdx < data->images.count ? &data->views[data->imageIdx] : nullptr;
}

Format Window::imageFormat() const
{
    return data->format;
}

bool Window::wasClosed() const
{
    return data->wasClosed;
}

bool Window::isFocused() const
{
    return SDL_GetMouseFocus() == data->sdlWindow;
}

u32 Window::width() const
{
    return data->width;
}

u32 Window::height() const
{
    return data->height;
}

f32 Window::mouseX() const
{
    return data->mouseX;
}

f32 Window::mouseY() const
{
    return data->mouseY;
}

f32 Window::mouseDX() const
{
    return windowState.mouseDX / static_cast<f32>(data->height);
}

f32 Window::mouseDY() const
{
    return windowState.mouseDY / static_cast<f32>(data->height);
}

bool Window::isButtonDown(Button key) const
{
    return data->isKeyDown[key];
}

Span<WindowEvent> Window::events() const
{
    return data->events;
}

static Button sdlKeycodeToHgButton(u32 key)
{
    switch (key)
    {
        case SDLK_0: return Button_k0;
        case SDLK_1: return Button_k1;
        case SDLK_2: return Button_k2;
        case SDLK_3: return Button_k3;
        case SDLK_4: return Button_k4;
        case SDLK_5: return Button_k5;
        case SDLK_6: return Button_k6;
        case SDLK_7: return Button_k7;
        case SDLK_8: return Button_k8;
        case SDLK_9: return Button_k9;
        case SDLK_Q: return Button_q;
        case SDLK_W: return Button_w;
        case SDLK_E: return Button_e;
        case SDLK_R: return Button_r;
        case SDLK_T: return Button_t;
        case SDLK_Y: return Button_y;
        case SDLK_U: return Button_u;
        case SDLK_I: return Button_i;
        case SDLK_O: return Button_o;
        case SDLK_P: return Button_p;
        case SDLK_A: return Button_a;
        case SDLK_S: return Button_s;
        case SDLK_D: return Button_d;
        case SDLK_F: return Button_f;
        case SDLK_G: return Button_g;
        case SDLK_H: return Button_h;
        case SDLK_J: return Button_j;
        case SDLK_K: return Button_k;
        case SDLK_L: return Button_l;
        case SDLK_Z: return Button_z;
        case SDLK_X: return Button_x;
        case SDLK_C: return Button_c;
        case SDLK_V: return Button_v;
        case SDLK_B: return Button_b;
        case SDLK_N: return Button_n;
        case SDLK_M: return Button_m;
        case SDLK_SEMICOLON: return Button_semicolon;
        case SDLK_COLON: return Button_colon;
        case SDLK_APOSTROPHE: return Button_apostrophe;
        case SDLK_DBLAPOSTROPHE: return Button_quotation;
        case SDLK_COMMA: return Button_comma;
        case SDLK_PERIOD: return Button_period;
        case SDLK_QUESTION: return Button_question;
        case SDLK_GRAVE: return Button_grave;
        case SDLK_TILDE: return Button_tilde;
        case SDLK_EXCLAIM: return Button_exclamation;
        case SDLK_AT: return Button_at;
        case SDLK_HASH: return Button_hash;
        case SDLK_DOLLAR: return Button_dollar;
        case SDLK_PERCENT: return Button_percent;
        case SDLK_CARET: return Button_carot;
        case SDLK_AMPERSAND: return Button_ampersand;
        case SDLK_ASTERISK: return Button_asterisk;
        case SDLK_LEFTPAREN: return Button_lparen;
        case SDLK_RIGHTPAREN: return Button_rparen;
        case SDLK_LEFTBRACKET: return Button_lbracket;
        case SDLK_RIGHTBRACKET: return Button_rbracket;
        case SDLK_LEFTBRACE: return Button_lbrace;
        case SDLK_RIGHTBRACE: return Button_rbrace;
        case SDLK_EQUALS: return Button_equal;
        case SDLK_LESS: return Button_less;
        case SDLK_GREATER: return Button_greater;
        case SDLK_PLUS: return Button_plus;
        case SDLK_MINUS: return Button_minus;
        case SDLK_SLASH: return Button_slash;
        case SDLK_BACKSLASH: return Button_backslash;
        case SDLK_UNDERSCORE: return Button_underscore;
        case SDLK_PIPE: return Button_bar;
        case SDLK_UP: return Button_up;
        case SDLK_DOWN: return Button_down;
        case SDLK_LEFT: return Button_left;
        case SDLK_RIGHT: return Button_right;
        case SDLK_ESCAPE: return Button_escape;
        case SDLK_SPACE: return Button_space;
        case SDLK_RETURN: return Button_enter;
        case SDLK_BACKSPACE: return Button_backspace;
        case SDLK_DELETE: return Button_kdelete;
        case SDLK_INSERT: return Button_insert;
        case SDLK_TAB: return Button_tab;
        case SDLK_HOME: return Button_home;
        case SDLK_END: return Button_end;
        case SDLK_F1: return Button_f1;
        case SDLK_F2: return Button_f2;
        case SDLK_F3: return Button_f3;
        case SDLK_F4: return Button_f4;
        case SDLK_F5: return Button_f5;
        case SDLK_F6: return Button_f6;
        case SDLK_F7: return Button_f7;
        case SDLK_F8: return Button_f8;
        case SDLK_F9: return Button_f9;
        case SDLK_F10: return Button_f10;
        case SDLK_F11: return Button_f11;
        case SDLK_F12: return Button_f12;
        case SDLK_LSHIFT: return Button_lshift;
        case SDLK_RSHIFT: return Button_rshift;
        case SDLK_LCTRL: return Button_lctrl;
        case SDLK_RCTRL: return Button_rctrl;
        case SDLK_LALT: return Button_lalt;
        case SDLK_RALT: return Button_ralt;
        case SDLK_LGUI: return Button_lsuper;
        case SDLK_RGUI: return Button_rsuper;
        case SDLK_CAPSLOCK: return Button_capslock;
    }
    return Button_none;
}

static Button sdlButtonToHgButton(u32 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT: return Button_mouse1;
        case SDL_BUTTON_RIGHT: return Button_mouse2;
        case SDL_BUTTON_MIDDLE: return Button_mouse3;
        case SDL_BUTTON_X1: return Button_mouse4;
        case SDL_BUTTON_X2: return Button_mouse5;
    }
    return Button_none;
}

static void resizeWindowSwapchain(WindowData* window)
{
    ArenaScope scratch = getScratch();

    vkQueueWaitIdle(vk.queue);

    for (u32 i = 0; i < window->images.count; ++i)
    {
        vkDestroyImageView(vk.device, window->views[i].data->view, nullptr);

        vkDestroySemaphore(vk.device, window->readyToPresent[i], nullptr);
        window->readyToPresent[i] = nullptr;
    }

    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        vkDestroySemaphore(vk.device, window->imageAvailable[i], nullptr);
        window->imageAvailable[i] = nullptr;
    }

    VkSwapchainKHR oldSwapchain = window->swapchain;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physicalDevice, window->surface, &capabilities);

    if (capabilities.currentExtent.width != (u32)-1)
        window->width = capabilities.currentExtent.width;
    if (capabilities.currentExtent.height != (u32)-1)
        window->height = capabilities.currentExtent.height;

    if (window->width != 0 && window->height != 0)
    {
        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = window->surface;
        swapchainInfo.minImageCount = std::min(capabilities.minImageCount, capabilities.maxImageCount - 1) + 1;
        swapchainInfo.imageFormat = formatToVk(window->format);
        swapchainInfo.imageExtent = {window->width, window->height};
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = window->imageUsage;
        swapchainInfo.preTransform = capabilities.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = presentModeToVk(window->presentMode);
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = window->swapchain;

        [[maybe_unused]] VkResult result = vkCreateSwapchainKHR(
            vk.device, &swapchainInfo, nullptr, &window->swapchain);
        if (window->swapchain == nullptr)
            HG_PANIC("Failed to create swapchain: %s\n", vkResultToStr(result));

        u32 swapImageCount;
        vkGetSwapchainImagesKHR(vk.device, window->swapchain, &swapImageCount, nullptr);

        if (window->images.count != swapImageCount)
        {
            window->images.resize(swapImageCount);
            window->views.resize(swapImageCount);
            window->readyToPresent.resize(swapImageCount);
        }

        VkImage* swapImages = scratch.alloc<VkImage>(swapImageCount);
        vkGetSwapchainImagesKHR(vk.device, window->swapchain, &swapImageCount, swapImages);

        for (u32 i = 0; i < window->images.count; ++i)
        {
            if (window->images[i].data == nullptr)
                window->images[i].data = makeUnique<GpuImageData>();
            window->images[i].data->image = swapImages[i];
            window->images[i].data->dimensions = 2;
            window->images[i].data->format = window->format;
            window->images[i].data->width = window->width;
            window->images[i].data->height = window->height;
            window->images[i].data->depth = 1;
            window->images[i].data->mipLevels = 1;
            window->images[i].data->arrayLayers = 1;
            window->images[i].data->msaaSamples = 1;

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = formatToVk(window->format);
            viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            if (window->views[i].data == nullptr)
                window->views[i].data = makeUnique<GpuViewData>();

            [[maybe_unused]]
            VkResult viewResult = vkCreateImageView(
                vk.device, &viewInfo, nullptr, &window->views[i].data->view);
            if (window->views[i].data->view == nullptr)
                HG_PANIC("Could not create VkImageView: %s\n", vkResultToStr(viewResult));

            window->views[i].data->image = window->images[i].data;
            window->views[i].data->type = GpuViewType_2D;
            window->views[i].data->aspectFlags = GpuAspect_color;
            window->views[i].data->baseMipLevel = 0;
            window->views[i].data->levelCount = 1;
            window->views[i].data->baseArrayLayer = 0;
            window->views[i].data->layerCount = 1;

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            [[maybe_unused]] VkResult readyResult = vkCreateSemaphore(
                vk.device, &semaphoreInfo, nullptr, &window->readyToPresent[i]);
            if (window->readyToPresent[i] == nullptr)
                HG_PANIC("Could not create VkSemaphore: %s\n", vkResultToStr(readyResult));
        }

        for (u32 i = 0; i < vk.frameCount; ++i)
        {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            [[maybe_unused]] VkResult availableResult = vkCreateSemaphore(
                vk.device, &semaphoreInfo, nullptr, &window->imageAvailable[i]);
            if (window->imageAvailable[i] == nullptr)
                HG_PANIC("Could not create VkSemaphore: %s\n", vkResultToStr(availableResult));
        }
    }
    else
    {
        window->swapchain = nullptr;
    }

    window->imageIdx = (u32)-1;

    vkDestroySwapchainKHR(vk.device, oldSwapchain, nullptr);
}

void processEvents()
{
    windowState.mouseDX = 0;
    windowState.mouseDY = 0;

    windowState.ids.forEach([&](SDL_WindowID*, WindowData** window)
    {
        (*window)->events.count = 0;
    });

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (windowState.imguiInitialized)
            ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                windowState.wasQuit = true;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                WindowData** w = windowState.ids.get(event.window.windowID);
                if (w != nullptr)
                    (*w)->wasClosed = true;
            } break;
            case SDL_EVENT_WINDOW_MINIMIZED: [[fallthrough]];
            case SDL_EVENT_WINDOW_RESTORED: [[fallthrough]];
            case SDL_EVENT_WINDOW_RESIZED:
            {
                WindowData** w = windowState.ids.get(event.window.windowID);
                if (w != nullptr)
                {
                    SDL_GetWindowSize((*w)->sdlWindow,
                        reinterpret_cast<int*>(&(*w)->width),
                        reinterpret_cast<int*>(&(*w)->height));
                    resizeWindowSwapchain(*w);
                }
            } break;
            case SDL_EVENT_MOUSE_MOTION:
            {
                WindowData** w = windowState.ids.get(event.button.windowID);
                if (w != nullptr)
                {
                    (*w)->mouseX = event.motion.x;
                    (*w)->mouseY = event.motion.y;
                }
                windowState.mouseDX += event.motion.xrel;
                windowState.mouseDY += event.motion.yrel;
            } break;
            case SDL_EVENT_KEY_DOWN:
            {
                Button key = sdlKeycodeToHgButton(event.key.key);
                WindowData** w = windowState.ids.get(event.key.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.button.type = WindowEventType_buttonPress;
                    windowEvent.button.button = key;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = true;
                }
            } break;
            case SDL_EVENT_KEY_UP:
            {
                Button key = sdlKeycodeToHgButton(event.key.key);
                WindowData** w = windowState.ids.get(event.key.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.button.type = WindowEventType_buttonRelease;
                    windowEvent.button.button = key;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = false;
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                Button key = sdlButtonToHgButton(event.button.button);
                WindowData** w = windowState.ids.get(event.button.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.button.type = WindowEventType_buttonPress;
                    windowEvent.button.button = key;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = true;
                }
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                Button key = sdlButtonToHgButton(event.button.button);
                WindowData** w = windowState.ids.get(event.button.windowID);
                if (w != nullptr)
                {
                    WindowEvent windowEvent{};
                    windowEvent.button.type = WindowEventType_buttonRelease;
                    windowEvent.button.button = key;

                    (*w)->events.push(windowEvent);
                    (*w)->isKeyDown[key] = false;
                }
            } break;
        }
    }
}

bool wasQuit()
{
    return windowState.wasQuit;
}

static Format findSwapchainFormat(VkSurfaceKHR surface)
{
    HG_ASSERT(surface != nullptr);

    ArenaScope scratch = getScratch();

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, surface, &formatCount, nullptr);
    VkSurfaceFormatKHR* formats = scratch.alloc<VkSurfaceFormatKHR>(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, surface, &formatCount, formats);

    for (u32 i = 0; i < formatCount; ++i)
    {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            return Format_r8g8b8a8_srgb;
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            return Format_b8g8r8a8_srgb;
    }
    HG_PANIC("No supported swapchain formats\n");
}

static GpuPresentMode findSwapchainPresentMode(
    VkSurfaceKHR surface,
    GpuPresentMode desiredMode)
{
    HG_ASSERT(surface != nullptr);

    ArenaScope scratch = getScratch();

    if (desiredMode == GpuPresentMode_fifo)
        return desiredMode;

    u32 modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, surface, &modeCount, nullptr);
    VkPresentModeKHR* presentModes = scratch.alloc<VkPresentModeKHR>(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, surface, &modeCount, presentModes);

    for (u32 i = 0; i < modeCount; ++i)
    {
        if (presentModes[i] == presentModeToVk(desiredMode))
            return desiredMode;
    }
    return GpuPresentMode_fifo;
}

Maybe<Window> Window::create(StringView title, u32 width, u32 height, const WindowConfig& config)
{
    Maybe<Window> window = some<Window>();
    window->data = makeUnique<WindowData>();

    if (title == "")
        title = "Hurdy Gurdy";

    u64 flags = SDL_WINDOW_VULKAN;
    if (!config.fixedSize)
        flags |= SDL_WINDOW_RESIZABLE;

    if (config.fullscreen)
    {
        int modeCount = 0;
        SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &modeCount);
        HG_DEFER(SDL_free(modes));

        width = static_cast<u32>(modes[0]->w);
        height = static_cast<u32>(modes[0]->h);
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    ArenaScope scratch = getScratch();

    window->data->sdlWindow = SDL_CreateWindow(
        cString(scratch, title), static_cast<int>(width), static_cast<int>(height), flags);
    if (window->data->sdlWindow == nullptr)
    {
        setError(SDL_GetError());
        goto windowFailed;
    }

    if (!SDL_Vulkan_CreateSurface(window->data->sdlWindow, vk.instance, nullptr, &window->data->surface))
    {
        setError(SDL_GetError());
        goto surfaceFailed;
    }

    windowState.ids.add(SDL_GetWindowID(window->data->sdlWindow), window->data);

    SDL_GetWindowSize(window->data->sdlWindow,
        reinterpret_cast<int*>(&window->data->width),
        reinterpret_cast<int*>(&window->data->height));

    window->data->format = findSwapchainFormat(window->data->surface);
    window->data->presentMode = findSwapchainPresentMode(window->data->surface, config.preferredPresentMode);
    window->data->imageUsage = config.imageUsage;

    window->data->imageAvailable = Array<VkSemaphore>{vk.frameCount, vk.frameCount};
    for (u32 i = 0; i < vk.frameCount; ++i)
    {
        window->data->imageAvailable[i] = nullptr;
    }

    resizeWindowSwapchain(window->data);

    window->data->events = Array<WindowEvent>(0, 1024);

    return window;

surfaceFailed:
    SDL_DestroyWindow(window->data->sdlWindow);
windowFailed:
    window->data = {};
    return {};
}

GpuCmd* gpuFrameBegin(Span<Window*> windows)
{
    Frame* frame = &vk.frames[vk.currentFrame];

    vkWaitForFences(vk.device, 1, &frame->fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &frame->fence);

    frame->windows.count = 0;
    for (u32 i = 0; i < windows.count; ++i)
    {
        if (windows[i]->data->swapchain == nullptr)
            continue;

        VkResult result = vkAcquireNextImageKHR(
            vk.device,
            windows[i]->data->swapchain,
            UINT64_MAX,
            windows[i]->data->imageAvailable[vk.currentFrame],
            nullptr,
            &windows[i]->data->imageIdx);

        if (result == VK_SUCCESS)
        {
            frame->windows.push(windows[i]->data);
        }
        else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            resizeWindowSwapchain(windows[i]->data);
            windows[i]->data->imageIdx = (u32)-1;
        }
        else
        {
            HG_PANIC("Could not acquire next image: %s\n", vkResultToStr(result));
        }
    }

    vkResetCommandPool(vk.device, frame->cmdPool, 0);

    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = frame->cmdPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(vk.device, &cmdInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    return reinterpret_cast<GpuCmd*>(cmd);
}

void gpuFrameEnd(GpuCmd* cmd)
{
    HG_ASSERT(cmd != nullptr);

    ArenaScope scratch = getScratch();

    Frame* frame = &vk.frames[vk.currentFrame];

    ArrayTemp<GpuImageBarrier> presentBarriers = ArrayTemp<GpuImageBarrier>{
        scratch, 0, frame->windows.count};
    for (u32 i = 0; i < frame->windows.count; ++i)
    {
        GpuImageBarrier* barrier = presentBarriers.push();
        barrier->image = &frame->windows[i]->views[frame->windows[i]->imageIdx];
        barrier->nextLayout = GpuLayout_presentSrc;
    }
    gpuMemoryBarrier(cmd, {}, presentBarriers);

    vkEndCommandBuffer(reinterpret_cast<VkCommandBuffer>(cmd));

    VkPipelineStageFlags* waitStages = scratch.alloc<VkPipelineStageFlags>(frame->windows.count);
    VkSemaphore* imageAvailableSemaphores = scratch.alloc<VkSemaphore>(frame->windows.count);
    VkSemaphore* readyToPresentSemaphores = scratch.alloc<VkSemaphore>(frame->windows.count);

    VkSwapchainKHR* swapchains = scratch.alloc<VkSwapchainKHR>(frame->windows.count);
    u32* imageIndices = scratch.alloc<u32>(frame->windows.count);

    for (u32 i = 0; i < frame->windows.count; ++i)
    {
        waitStages[i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        imageAvailableSemaphores[i] = frame->windows[i]->imageAvailable[vk.currentFrame];
        readyToPresentSemaphores[i] = frame->windows[i]->readyToPresent[frame->windows[i]->imageIdx];

        swapchains[i] = frame->windows[i]->swapchain;
        imageIndices[i] = frame->windows[i]->imageIdx;
        frame->windows[i]->imageIdx = (u32)-1;
    }

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = static_cast<u32>(frame->windows.count);
    submit.pWaitSemaphores = imageAvailableSemaphores;
    submit.pWaitDstStageMask = waitStages;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = reinterpret_cast<VkCommandBuffer*>(&cmd);
    submit.signalSemaphoreCount = static_cast<u32>(frame->windows.count);
    submit.pSignalSemaphores = readyToPresentSemaphores;

    vkQueueSubmit(vk.queue, 1, &submit, frame->fence);

    if (frame->windows.count > 0)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = static_cast<u32>(frame->windows.count);
        presentInfo.pWaitSemaphores = readyToPresentSemaphores;
        presentInfo.swapchainCount = static_cast<u32>(frame->windows.count);
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = imageIndices;

        vkQueuePresentKHR(vk.queue, &presentInfo);
    }

    vk.currentFrame = (vk.currentFrame + 1) % vk.frameCount;
}

} // namespace hg
