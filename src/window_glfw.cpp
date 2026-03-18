#include "hurdygurdy.hpp"

#include <GLFW/glfw3.h>

#include "hurdygurdy.hpp"

void hgInternalCreateWindowSwapchain(HgWindow* window, const HgWindowConfig& config);
void hgInternalResizeWindowSwapchain(HgWindow* window);
void hgInternalDestroyWindowSwapchain(HgWindow* window);

struct HgWindow::Internals {
    GLFWwindow* glfwWindow;
};

void hgInitPlatform()
{
    glfwInit();
}

void hgDeinitPlatform()
{
    glfwTerminate();
}

void windowSizeCallback(GLFWwindow* glfwWindow, int width, int height)
{
    HgWindow* window = (HgWindow*)glfwGetWindowUserPointer(glfwWindow);
    window->width = (u32)width;
    window->height = (u32)height;
}

static void keyCallback(
    GLFWwindow* glfwWindow,
    int glfwKey,
    [[maybe_unused]] int scancode,
    int action,
    [[maybe_unused]] int mods)
{
    HgWindow* window = (HgWindow*)glfwGetWindowUserPointer(glfwWindow);

    bool isShiftDown =
        window->isKeyDown[(u32)HgKey::lshift] ||
        window->isKeyDown[(u32)HgKey::rshift];

    HgKey key = HgKey::none;
    switch (glfwKey)
    {
        case GLFW_KEY_0:
            key = isShiftDown ? HgKey::rparen : HgKey::k0;
            break;
        case GLFW_KEY_1:
            key = isShiftDown ? HgKey::exclamation : HgKey::k1;
            break;
        case GLFW_KEY_2:
            key = isShiftDown ? HgKey::at : HgKey::k2;
            break;
        case GLFW_KEY_3:
            key = isShiftDown ? HgKey::hash : HgKey::k3;
            break;
        case GLFW_KEY_4:
            key = isShiftDown ? HgKey::dollar : HgKey::k4;
            break;
        case GLFW_KEY_5:
            key = isShiftDown ? HgKey::percent : HgKey::k5;
            break;
        case GLFW_KEY_6:
            key = isShiftDown ? HgKey::carot : HgKey::k6;
            break;
        case GLFW_KEY_7:
            key = isShiftDown ? HgKey::ampersand : HgKey::k7;
            break;
        case GLFW_KEY_8:
            key = isShiftDown ? HgKey::asterisk : HgKey::k8;
            break;
        case GLFW_KEY_9:
            key = isShiftDown ? HgKey::lparen : HgKey::k9;
            break;

        case GLFW_KEY_Q:
            key = HgKey::q;
            break;
        case GLFW_KEY_W:
            key = HgKey::w;
            break;
        case GLFW_KEY_E:
            key = HgKey::e;
            break;
        case GLFW_KEY_R:
            key = HgKey::r;
            break;
        case GLFW_KEY_T:
            key = HgKey::t;
            break;
        case GLFW_KEY_Y:
            key = HgKey::y;
            break;
        case GLFW_KEY_U:
            key = HgKey::u;
            break;
        case GLFW_KEY_I:
            key = HgKey::i;
            break;
        case GLFW_KEY_O:
            key = HgKey::o;
            break;
        case GLFW_KEY_P:
            key = HgKey::p;
            break;
        case GLFW_KEY_A:
            key = HgKey::a;
            break;
        case GLFW_KEY_S:
            key = HgKey::s;
            break;
        case GLFW_KEY_D:
            key = HgKey::d;
            break;
        case GLFW_KEY_F:
            key = HgKey::f;
            break;
        case GLFW_KEY_G:
            key = HgKey::g;
            break;
        case GLFW_KEY_H:
            key = HgKey::h;
            break;
        case GLFW_KEY_J:
            key = HgKey::j;
            break;
        case GLFW_KEY_K:
            key = HgKey::k;
            break;
        case GLFW_KEY_L:
            key = HgKey::l;
            break;
        case GLFW_KEY_Z:
            key = HgKey::z;
            break;
        case GLFW_KEY_X:
            key = HgKey::x;
            break;
        case GLFW_KEY_C:
            key = HgKey::c;
            break;
        case GLFW_KEY_V:
            key = HgKey::v;
            break;
        case GLFW_KEY_B:
            key = HgKey::b;
            break;
        case GLFW_KEY_N:
            key = HgKey::n;
            break;
        case GLFW_KEY_M:
            key = HgKey::m;
            break;

        case GLFW_KEY_SEMICOLON:
            key = isShiftDown ? HgKey::colon : HgKey::semicolon;
            break;
        case GLFW_KEY_APOSTROPHE:
            key = isShiftDown ? HgKey::quotation : HgKey::apostrophe;
            break;
        case GLFW_KEY_COMMA:
            key = isShiftDown ? HgKey::less : HgKey::comma;
            break;
        case GLFW_KEY_PERIOD:
            key = isShiftDown ? HgKey::greater : HgKey::period;
            break;
        case GLFW_KEY_SLASH:
            key = isShiftDown ? HgKey::question : HgKey::slash;
            break;
        case GLFW_KEY_BACKSLASH:
            key = isShiftDown ? HgKey::bar : HgKey::backslash;
            break;
        case GLFW_KEY_LEFT_BRACKET:
            key = isShiftDown ? HgKey::lbrace : HgKey::lbracket;
            break;
        case GLFW_KEY_RIGHT_BRACKET:
            key = isShiftDown ? HgKey::rbrace : HgKey::rbracket;
            break;
        case GLFW_KEY_GRAVE_ACCENT:
            key = isShiftDown ? HgKey::tilde : HgKey::grave;
            break;
        case GLFW_KEY_MINUS:
            key = isShiftDown ? HgKey::underscore : HgKey::minus;
            break;
        case GLFW_KEY_EQUAL:
            key = isShiftDown ? HgKey::plus : HgKey::equal;
            break;

        case GLFW_KEY_UP:
            key = HgKey::up;
            break;
        case GLFW_KEY_DOWN:
            key = HgKey::down;
            break;
        case GLFW_KEY_LEFT:
            key = HgKey::left;
            break;
        case GLFW_KEY_RIGHT:
            key = HgKey::right;
            break;

        case GLFW_KEY_ESCAPE:
            key = HgKey::escape;
            break;
        case GLFW_KEY_SPACE:
            key = HgKey::space;
            break;
        case GLFW_KEY_ENTER:
            key = HgKey::enter;
            break;
        case GLFW_KEY_BACKSPACE:
            key = HgKey::backspace;
            break;
        case GLFW_KEY_DELETE:
            key = HgKey::kdelete;
            break;
        case GLFW_KEY_INSERT:
            key = HgKey::insert;
            break;
        case GLFW_KEY_TAB:
            key = HgKey::tab;
            break;
        case GLFW_KEY_HOME:
            key = HgKey::home;
            break;
        case GLFW_KEY_END:
            key = HgKey::end;
            break;

        case GLFW_KEY_F1:
            key = HgKey::f1;
            break;
        case GLFW_KEY_F2:
            key = HgKey::f2;
            break;
        case GLFW_KEY_F3:
            key = HgKey::f3;
            break;
        case GLFW_KEY_F4:
            key = HgKey::f4;
            break;
        case GLFW_KEY_F5:
            key = HgKey::f5;
            break;
        case GLFW_KEY_F6:
            key = HgKey::f6;
            break;
        case GLFW_KEY_F7:
            key = HgKey::f7;
            break;
        case GLFW_KEY_F8:
            key = HgKey::f8;
            break;
        case GLFW_KEY_F9:
            key = HgKey::f9;
            break;
        case GLFW_KEY_F10:
            key = HgKey::f10;
            break;
        case GLFW_KEY_F11:
            key = HgKey::f11;
            break;
        case GLFW_KEY_F12:
            key = HgKey::f12;
            break;

        case GLFW_KEY_LEFT_SHIFT:
            key = HgKey::lshift;
            break;
        case GLFW_KEY_RIGHT_SHIFT:
            key = HgKey::rshift;
            break;
        case GLFW_KEY_LEFT_CONTROL:
            key = HgKey::lctrl;
            break;
        case GLFW_KEY_RIGHT_CONTROL:
            key = HgKey::rctrl;
            break;
        case GLFW_KEY_LEFT_ALT:
            key = HgKey::lalt;
            break;
        case GLFW_KEY_RIGHT_ALT:
            key = HgKey::ralt;
            break;
        case GLFW_KEY_LEFT_SUPER:
            key = HgKey::lsuper;
            break;
        case GLFW_KEY_RIGHT_SUPER:
            key = HgKey::rsuper;
            break;
        case GLFW_KEY_CAPS_LOCK:
            key = HgKey::capslock;
            break;
    }

    if (action == GLFW_PRESS)
    {
        window->isKeyDown[(u32)key] = true;
        window->wasKeyPressed[(u32)key] = true;
    } else if (action == GLFW_RELEASE)
    {
        window->isKeyDown[(u32)key] = false;
        window->wasKeyReleased[(u32)key] = true;
    }
}

static void mousePosCallback(GLFWwindow* glfwWindow, double x, double y)
{
    HgWindow* window = (HgWindow*)glfwGetWindowUserPointer(glfwWindow);
    window->mousePosX = x / window->height;
    window->mousePosY = y / window->height;
}

void mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, [[maybe_unused]] int mods)
{
    HgWindow* window = (HgWindow*)glfwGetWindowUserPointer(glfwWindow);

    HgKey key = HgKey::none;
    switch (button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:
            key = HgKey::lmouse;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            key = HgKey::rmouse;
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            key = HgKey::mmouse;
            break;
        case GLFW_MOUSE_BUTTON_4:
            key = HgKey::mouse4;
            break;
        case GLFW_MOUSE_BUTTON_5:
            key = HgKey::mouse5;
            break;
    }
    if (action == GLFW_PRESS)
    {
        window->isKeyDown[(u32)key] = true;
        window->wasKeyPressed[(u32)key] = true;
    } else if (action == GLFW_RELEASE)
    {
        window->isKeyDown[(u32)key] = false;
        window->wasKeyReleased[(u32)key] = true;
    }
}

HgWindow* HgWindow::create(HgArena* arena, const HgWindowConfig& config)
{
    HgWindow* window = hgAlloc<HgWindow>(arena, 1);
    *window = {};
    window->internals = hgAlloc<Internals>(arena, 1);
    *window->internals = {};

    const char* title = config.title != nullptr ? config.title : "Hurdy Gurdy";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (config.windowed)
    {
        window->width = config.width;
        window->height = config.height;
        window->internals->glfwWindow = glfwCreateWindow(
            (int)config.width,
            (int)config.height,
            title,
            nullptr,
            nullptr);
    } else {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        window->width = (u32)mode->width;
        window->height = (u32)mode->height;
        window->internals->glfwWindow = glfwCreateWindow(
            mode->width,
            mode->height,
            title,
            monitor,
            nullptr);
    }

    glfwSetWindowUserPointer(window->internals->glfwWindow, window);
    glfwSetWindowSizeCallback(window->internals->glfwWindow, windowSizeCallback);
    glfwSetKeyCallback(window->internals->glfwWindow, keyCallback);
    glfwSetCursorPosCallback(window->internals->glfwWindow, mousePosCallback);
    glfwSetMouseButtonCallback(window->internals->glfwWindow, mouseButtonCallback);

    VkResult result = glfwCreateWindowSurface(
        hgVkInstance,
        window->internals->glfwWindow,
        nullptr,
        &window->surface);
    if (window->surface == nullptr)
        hgError("Failed to create Vulkan surface: %s\n", hgVkResultToStr(result));

    hgInternalCreateWindowSwapchain(window, config);

    return window;
}

void HgWindow::destroy()
{
    hgInternalDestroyWindowSwapchain(this);
    vkDestroySurfaceKHR(hgVkInstance, surface, nullptr);
    glfwDestroyWindow(internals->glfwWindow);
}

void HgWindow::setIcon(u32* iconData, u32 iconWidth, u32 iconHeight)
{
    hgError("window setIcon : TODO\n");
    (void)iconData;
    (void)iconWidth;
    (void)iconHeight;
}

bool HgWindow::isFullscreen()
{
    hgError("window isFullscreen : TODO\n");
}

void HgWindow::setFullscreen(bool fullscreen)
{
    hgError("window setFullscreen : TODO\n");
    (void)fullscreen;
}

void HgWindow::setCursor(HgWindow::Cursor cursor)
{
    hgError("window setCursor : TODO\n");
    (void)cursor;
}

void HgWindow::setCursorImage(u32* data, u32 imageWidth, u32 imageHeight)
{
    hgError("window setCursorImage : TODO\n");
    (void)data;
    (void)imageWidth;
    (void)imageHeight;
}

u32 hgVkGetPlatformExtensions(HgArena* arena, HgStringView** extBuffer)
{
    u32 extCount;
    const char** exts = glfwGetRequiredInstanceExtensions(&extCount);
    if (exts == nullptr)
        hgError("Could not get required instance extensions from glfw\n");

    *extBuffer = hgAlloc<HgStringView>(arena, extCount);
    for (usize i = 0; i < extCount; ++i)
    {
        (*extBuffer)[i] = exts[i];
    }

    return extCount;
}

void hgProcessWindowEvents(HgWindow** windows, usize windowCount)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    u32* oldWidths = hgAlloc<u32>(scratch, windowCount);
    u32* oldHeights = hgAlloc<u32>(scratch, windowCount);
    f64* oldMouseXs = hgAlloc<f64>(scratch, windowCount);
    f64* oldMouseYs = hgAlloc<f64>(scratch, windowCount);

    for (usize i = 0; i < windowCount; ++i)
    {
        HgWindow* window = windows[i];

        oldWidths[i] = window->width;
        oldHeights[i] = window->height;
        oldMouseXs[i] = window->mousePosX;
        oldMouseYs[i] = window->mousePosY;

        memset(window->wasKeyPressed, 0, sizeof(window->wasKeyPressed));
        memset(window->wasKeyReleased, 0, sizeof(window->wasKeyReleased));
    }

    glfwPollEvents();

    for (usize i = 0; i < windowCount; ++i)
    {
        HgWindow* window = windows[i];

        if (window->width != oldWidths[i] || window->height != oldHeights[i])
            hgInternalResizeWindowSwapchain(window);

        window->wasClosed = glfwWindowShouldClose(window->internals->glfwWindow);

        window->mouseDeltaX = window->mousePosX - oldMouseXs[i];
        window->mouseDeltaY = window->mousePosY - oldMouseYs[i];
    }
}

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

void ImGui_ImplHurdyGurdy_Init(
    HgWindow* window,
    u32 colorAttachmentCount,
    const VkFormat* colorFormats,
    VkFormat depthFormat,
    VkFormat stencilFormat)
{
    ImGui_ImplGlfw_InitForVulkan(window->internals->glfwWindow, true);

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
}

void ImGui_ImplHurdyGurdy_Shutdown()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

void ImGui_ImplHurdyGurdy_NewFrame()
{
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
}

void ImGui_ImplHurdyGurdy_Draw(VkCommandBuffer cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

