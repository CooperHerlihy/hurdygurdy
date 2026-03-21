#include "hurdygurdy.hpp"

#include <GLFW/glfw3.h>

#include "hurdygurdy.hpp"

void hgInternalCreateWindowSwapchain(HgWindow* window, const HgWindowConfig* config);
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
        window->isKeyDown[HgKey_lshift] ||
        window->isKeyDown[HgKey_rshift];

    HgKey key = HgKey_none;
    switch (glfwKey)
    {
        case GLFW_KEY_0:
            key = isShiftDown ? HgKey_rparen : HgKey_k0;
            break;
        case GLFW_KEY_1:
            key = isShiftDown ? HgKey_exclamation : HgKey_k1;
            break;
        case GLFW_KEY_2:
            key = isShiftDown ? HgKey_at : HgKey_k2;
            break;
        case GLFW_KEY_3:
            key = isShiftDown ? HgKey_hash : HgKey_k3;
            break;
        case GLFW_KEY_4:
            key = isShiftDown ? HgKey_dollar : HgKey_k4;
            break;
        case GLFW_KEY_5:
            key = isShiftDown ? HgKey_percent : HgKey_k5;
            break;
        case GLFW_KEY_6:
            key = isShiftDown ? HgKey_carot : HgKey_k6;
            break;
        case GLFW_KEY_7:
            key = isShiftDown ? HgKey_ampersand : HgKey_k7;
            break;
        case GLFW_KEY_8:
            key = isShiftDown ? HgKey_asterisk : HgKey_k8;
            break;
        case GLFW_KEY_9:
            key = isShiftDown ? HgKey_lparen : HgKey_k9;
            break;

        case GLFW_KEY_Q:
            key = HgKey_q;
            break;
        case GLFW_KEY_W:
            key = HgKey_w;
            break;
        case GLFW_KEY_E:
            key = HgKey_e;
            break;
        case GLFW_KEY_R:
            key = HgKey_r;
            break;
        case GLFW_KEY_T:
            key = HgKey_t;
            break;
        case GLFW_KEY_Y:
            key = HgKey_y;
            break;
        case GLFW_KEY_U:
            key = HgKey_u;
            break;
        case GLFW_KEY_I:
            key = HgKey_i;
            break;
        case GLFW_KEY_O:
            key = HgKey_o;
            break;
        case GLFW_KEY_P:
            key = HgKey_p;
            break;
        case GLFW_KEY_A:
            key = HgKey_a;
            break;
        case GLFW_KEY_S:
            key = HgKey_s;
            break;
        case GLFW_KEY_D:
            key = HgKey_d;
            break;
        case GLFW_KEY_F:
            key = HgKey_f;
            break;
        case GLFW_KEY_G:
            key = HgKey_g;
            break;
        case GLFW_KEY_H:
            key = HgKey_h;
            break;
        case GLFW_KEY_J:
            key = HgKey_j;
            break;
        case GLFW_KEY_K:
            key = HgKey_k;
            break;
        case GLFW_KEY_L:
            key = HgKey_l;
            break;
        case GLFW_KEY_Z:
            key = HgKey_z;
            break;
        case GLFW_KEY_X:
            key = HgKey_x;
            break;
        case GLFW_KEY_C:
            key = HgKey_c;
            break;
        case GLFW_KEY_V:
            key = HgKey_v;
            break;
        case GLFW_KEY_B:
            key = HgKey_b;
            break;
        case GLFW_KEY_N:
            key = HgKey_n;
            break;
        case GLFW_KEY_M:
            key = HgKey_m;
            break;

        case GLFW_KEY_SEMICOLON:
            key = isShiftDown ? HgKey_colon : HgKey_semicolon;
            break;
        case GLFW_KEY_APOSTROPHE:
            key = isShiftDown ? HgKey_quotation : HgKey_apostrophe;
            break;
        case GLFW_KEY_COMMA:
            key = isShiftDown ? HgKey_less : HgKey_comma;
            break;
        case GLFW_KEY_PERIOD:
            key = isShiftDown ? HgKey_greater : HgKey_period;
            break;
        case GLFW_KEY_SLASH:
            key = isShiftDown ? HgKey_question : HgKey_slash;
            break;
        case GLFW_KEY_BACKSLASH:
            key = isShiftDown ? HgKey_bar : HgKey_backslash;
            break;
        case GLFW_KEY_LEFT_BRACKET:
            key = isShiftDown ? HgKey_lbrace : HgKey_lbracket;
            break;
        case GLFW_KEY_RIGHT_BRACKET:
            key = isShiftDown ? HgKey_rbrace : HgKey_rbracket;
            break;
        case GLFW_KEY_GRAVE_ACCENT:
            key = isShiftDown ? HgKey_tilde : HgKey_grave;
            break;
        case GLFW_KEY_MINUS:
            key = isShiftDown ? HgKey_underscore : HgKey_minus;
            break;
        case GLFW_KEY_EQUAL:
            key = isShiftDown ? HgKey_plus : HgKey_equal;
            break;

        case GLFW_KEY_UP:
            key = HgKey_up;
            break;
        case GLFW_KEY_DOWN:
            key = HgKey_down;
            break;
        case GLFW_KEY_LEFT:
            key = HgKey_left;
            break;
        case GLFW_KEY_RIGHT:
            key = HgKey_right;
            break;

        case GLFW_KEY_ESCAPE:
            key = HgKey_escape;
            break;
        case GLFW_KEY_SPACE:
            key = HgKey_space;
            break;
        case GLFW_KEY_ENTER:
            key = HgKey_enter;
            break;
        case GLFW_KEY_BACKSPACE:
            key = HgKey_backspace;
            break;
        case GLFW_KEY_DELETE:
            key = HgKey_kdelete;
            break;
        case GLFW_KEY_INSERT:
            key = HgKey_insert;
            break;
        case GLFW_KEY_TAB:
            key = HgKey_tab;
            break;
        case GLFW_KEY_HOME:
            key = HgKey_home;
            break;
        case GLFW_KEY_END:
            key = HgKey_end;
            break;

        case GLFW_KEY_F1:
            key = HgKey_f1;
            break;
        case GLFW_KEY_F2:
            key = HgKey_f2;
            break;
        case GLFW_KEY_F3:
            key = HgKey_f3;
            break;
        case GLFW_KEY_F4:
            key = HgKey_f4;
            break;
        case GLFW_KEY_F5:
            key = HgKey_f5;
            break;
        case GLFW_KEY_F6:
            key = HgKey_f6;
            break;
        case GLFW_KEY_F7:
            key = HgKey_f7;
            break;
        case GLFW_KEY_F8:
            key = HgKey_f8;
            break;
        case GLFW_KEY_F9:
            key = HgKey_f9;
            break;
        case GLFW_KEY_F10:
            key = HgKey_f10;
            break;
        case GLFW_KEY_F11:
            key = HgKey_f11;
            break;
        case GLFW_KEY_F12:
            key = HgKey_f12;
            break;

        case GLFW_KEY_LEFT_SHIFT:
            key = HgKey_lshift;
            break;
        case GLFW_KEY_RIGHT_SHIFT:
            key = HgKey_rshift;
            break;
        case GLFW_KEY_LEFT_CONTROL:
            key = HgKey_lctrl;
            break;
        case GLFW_KEY_RIGHT_CONTROL:
            key = HgKey_rctrl;
            break;
        case GLFW_KEY_LEFT_ALT:
            key = HgKey_lalt;
            break;
        case GLFW_KEY_RIGHT_ALT:
            key = HgKey_ralt;
            break;
        case GLFW_KEY_LEFT_SUPER:
            key = HgKey_lsuper;
            break;
        case GLFW_KEY_RIGHT_SUPER:
            key = HgKey_rsuper;
            break;
        case GLFW_KEY_CAPS_LOCK:
            key = HgKey_capslock;
            break;
    }

    if (action == GLFW_PRESS)
    {
        window->isKeyDown[key] = true;
        window->wasKeyPressed[key] = true;
    } else if (action == GLFW_RELEASE)
    {
        window->isKeyDown[key] = false;
        window->wasKeyReleased[key] = true;
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

    HgKey key = HgKey_none;
    switch (button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:
            key = HgKey_lmouse;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            key = HgKey_rmouse;
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            key = HgKey_mmouse;
            break;
        case GLFW_MOUSE_BUTTON_4:
            key = HgKey_mouse4;
            break;
        case GLFW_MOUSE_BUTTON_5:
            key = HgKey_mouse5;
            break;
    }
    if (action == GLFW_PRESS)
    {
        window->isKeyDown[key] = true;
        window->wasKeyPressed[key] = true;
    } else if (action == GLFW_RELEASE)
    {
        window->isKeyDown[key] = false;
        window->wasKeyReleased[key] = true;
    }
}

HgWindow* HgWindow::create(HgArena* arena, const HgWindowConfig* config)
{
    HgWindow* window = hgAlloc<HgWindow>(arena, 1);
    *window = {};
    window->internals = hgAlloc<Internals>(arena, 1);
    *window->internals = {};

    const char* title = config->title != nullptr ? config->title : "Hurdy Gurdy";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (config->windowed)
    {
        window->width = config->width;
        window->height = config->height;
        window->internals->glfwWindow = glfwCreateWindow(
            (int)config->width,
            (int)config->height,
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
    for (u32 i = 0; i < extCount; ++i)
    {
        (*extBuffer)[i] = exts[i];
    }

    return extCount;
}

void hgProcessWindowEvents(HgWindow** windows, u32 windowCount)
{
    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    u32* oldWidths = hgAlloc<u32>(scratch, windowCount);
    u32* oldHeights = hgAlloc<u32>(scratch, windowCount);
    f64* oldMouseXs = hgAlloc<f64>(scratch, windowCount);
    f64* oldMouseYs = hgAlloc<f64>(scratch, windowCount);

    for (u32 i = 0; i < windowCount; ++i)
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

    for (u32 i = 0; i < windowCount; ++i)
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

