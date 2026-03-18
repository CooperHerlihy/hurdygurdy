#include "hurdygurdy.hpp"

#ifdef HG_PLATFORM_WINDOWS

void hgInternalCreateWindowSwapchain(HgWindow* window, const HgWindowConfig& config);
void hgInternalResizeWindowSwapchain(HgWindow* window);
void hgInternalDestroyWindowSwapchain(HgWindow* window);

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>

struct HgWindow::Internals {
    HWND hwnd;
};

HINSTANCE win32Instance = nullptr;

void hgInitPlatform()
{
    win32Instance = GetModuleHandle(nullptr);
}

void hgDeinitPlatform()
{
    win32Instance = nullptr;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool imguiInitialized = false;

static LRESULT CALLBACK windowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    HgWindow* window = (HgWindow*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg)
    {
        case WM_NCCREATE:
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)(((CREATESTRUCTA*)lparam)->lpCreateParams));
            break;
        case WM_CLOSE:
            window->wasClosed = true;
            break;
        case WM_SIZE:
            window->width = LOWORD(lparam);
            window->height = HIWORD(lparam);
            break;
        case WM_KILLFOCUS:
            memset(window->isKeyDown, 0, sizeof(window->isKeyDown));
                break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            HgKey key = HgKey_none;
            HgKey shiftKey = HgKey_none;

            switch (wparam)
            {
                case '0':
                    key = HgKey_k0;
                    shiftKey = HgKey_rparen;
                    break;
                case '1':
                    key = HgKey_k1;
                    shiftKey = HgKey_exclamation;
                    break;
                case '2':
                    key = HgKey_k2;
                    shiftKey = HgKey_at;
                    break;
                case '3':
                    key = HgKey_k3;
                    shiftKey = HgKey_hash;
                    break;
                case '4':
                    key = HgKey_k4;
                    shiftKey = HgKey_dollar;
                    break;
                case '5':
                    key = HgKey_k5;
                    shiftKey = HgKey_percent;
                    break;
                case '6':
                    key = HgKey_k6;
                    shiftKey = HgKey_carot;
                    break;
                case '7':
                    key = HgKey_k7;
                    shiftKey = HgKey_ampersand;
                    break;
                case '8':
                    key = HgKey_k8;
                    shiftKey = HgKey_asterisk;
                    break;
                case '9':
                    key = HgKey_k9;
                    shiftKey = HgKey_lparen;
                    break;

                case 'A':
                    key = HgKey_a;
                    break;
                case 'B':
                    key = HgKey_b;
                    break;
                case 'C':
                    key = HgKey_c;
                    break;
                case 'D':
                    key = HgKey_d;
                    break;
                case 'E':
                    key = HgKey_e;
                    break;
                case 'F':
                    key = HgKey_f;
                    break;
                case 'G':
                    key = HgKey_g;
                    break;
                case 'H':
                    key = HgKey_h;
                    break;
                case 'I':
                    key = HgKey_i;
                    break;
                case 'J':
                    key = HgKey_j;
                    break;
                case 'K':
                    key = HgKey_k;
                    break;
                case 'L':
                    key = HgKey_l;
                    break;
                case 'M':
                    key = HgKey_m;
                    break;
                case 'N':
                    key = HgKey_n;
                    break;
                case 'O':
                    key = HgKey_o;
                    break;
                case 'P':
                    key = HgKey_p;
                    break;
                case 'Q':
                    key = HgKey_q;
                    break;
                case 'R':
                    key = HgKey_r;
                    break;
                case 'S':
                    key = HgKey_s;
                    break;
                case 'T':
                    key = HgKey_t;
                    break;
                case 'U':
                    key = HgKey_u;
                    break;
                case 'V':
                    key = HgKey_v;
                    break;
                case 'W':
                    key = HgKey_w;
                    break;
                case 'X':
                    key = HgKey_x;
                    break;
                case 'Y':
                    key = HgKey_y;
                    break;
                case 'Z':
                    key = HgKey_z;
                    break;

                case VK_OEM_1:
                    key = HgKey_semicolon;
                    shiftKey = HgKey_colon;
                    break;
                case VK_OEM_7:
                    key = HgKey_apostrophe;
                    shiftKey = HgKey_quotation;
                    break;
                case VK_OEM_COMMA:
                    key = HgKey_comma;
                    shiftKey = HgKey_less;
                    break;
                case VK_OEM_PERIOD:
                    key = HgKey_period;
                    shiftKey = HgKey_greater;
                    break;
                case VK_OEM_2:
                    key = HgKey_slash;
                    shiftKey = HgKey_question;
                    break;
                case VK_OEM_3:
                    key = HgKey_grave;
                    shiftKey = HgKey_tilde;
                    break;
                case VK_OEM_4:
                    key = HgKey_lbracket;
                    shiftKey = HgKey_lbrace;
                    break;
                case VK_OEM_6:
                    key = HgKey_rbracket;
                    shiftKey = HgKey_rbrace;
                    break;
                case VK_OEM_5:
                    key = HgKey_backslash;
                    shiftKey = HgKey_bar;
                    break;
                case VK_OEM_PLUS:
                    key = HgKey_equal;
                    shiftKey = HgKey_plus;
                    break;
                case VK_OEM_MINUS:
                    key = HgKey_minus;
                    shiftKey = HgKey_underscore;
                    break;

                case VK_UP:
                    key = HgKey_up;
                    break;
                case VK_DOWN:
                    key = HgKey_down;
                    break;
                case VK_LEFT:
                    key = HgKey_left;
                    break;
                case VK_RIGHT:
                    key = HgKey_right;
                    break;
                case VK_ESCAPE:
                    key = HgKey_escape;
                    break;
                case VK_SPACE:
                    key = HgKey_space;
                    break;
                case VK_RETURN:
                    key = HgKey_enter;
                    break;
                case VK_BACK:
                    key = HgKey_backspace;
                    break;
                case VK_DELETE:
                    key = HgKey_kdelete;
                    break;
                case VK_INSERT:
                    key = HgKey_insert;
                    break;
                case VK_TAB:
                    key = HgKey_tab;
                    break;
                case VK_HOME:
                    key = HgKey_home;
                    break;
                case VK_END:
                    key = HgKey_end;
                    break;

                case VK_F1:
                    key = HgKey_f1;
                    break;
                case VK_F2:
                    key = HgKey_f2;
                    break;
                case VK_F3:
                    key = HgKey_f3;
                    break;
                case VK_F4:
                    key = HgKey_f4;
                    break;
                case VK_F5:
                    key = HgKey_f5;
                    break;
                case VK_F6:
                    key = HgKey_f6;
                    break;
                case VK_F7:
                    key = HgKey_f7;
                    break;
                case VK_F8:
                    key = HgKey_f8;
                    break;
                case VK_F9:
                    key = HgKey_f9;
                    break;
                case VK_F10:
                    key = HgKey_f10;
                    break;
                case VK_F11:
                    key = HgKey_f11;
                    break;
                case VK_F12:
                    key = HgKey_f12;
                    break;

                case VK_SHIFT: {
                    u32 scancode = (lparam >> 16) & 0xff;
                    if (scancode == 0x36)
                        key = HgKey_rshift;
                    else if (scancode == 0x2A)
                        key = HgKey_lshift;
                } break;
                case VK_MENU:
                    if (lparam & (1 << 24))
                        key = HgKey_ralt;
                    else
                        key = HgKey_lalt;
                    break;
                case VK_CONTROL:
                    if (lparam & (1 << 24))
                        key = HgKey_rctrl;
                    else
                        key = HgKey_lctrl;
                    break;
                case VK_LWIN:
                    key = HgKey_lsuper;
                    break;
                case VK_RWIN:
                    key = HgKey_rsuper;
                    break;
                case VK_CAPITAL:
                    key = HgKey_capslock;
                    break;
            }
            if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)
            {
                if (shiftKey != HgKey_none &&
                   (window->isKeyDown[HgKey_lshift] ||
                    window->isKeyDown[HgKey_rshift]))
                {
                    window->wasKeyPressed[shiftKey] = true;
                    window->isKeyDown[shiftKey] = true;
                } else {
                    window->wasKeyPressed[key] = true;
                    window->isKeyDown[key] = true;
                }
            } else {
                window->wasKeyReleased[shiftKey] = window->isKeyDown[shiftKey];
                window->isKeyDown[shiftKey] = false;
                window->wasKeyReleased[key] = window->isKeyDown[key];
                window->isKeyDown[key] = false;
            }
        } break;
        case WM_LBUTTONDOWN:
            window->wasKeyPressed[HgKey_lmouse] = true;
            window->isKeyDown[HgKey_lmouse] = true;
            break;
        case WM_RBUTTONDOWN:
            window->wasKeyPressed[HgKey_rmouse] = true;
            window->isKeyDown[HgKey_rmouse] = true;
            break;
        case WM_MBUTTONDOWN:
            window->wasKeyPressed[HgKey_mmouse] = true;
            window->isKeyDown[HgKey_mmouse] = true;
            break;
        case WM_LBUTTONUP:
            window->wasKeyReleased[HgKey_lmouse] = true;
            window->isKeyDown[HgKey_lmouse] = false;
            break;
        case WM_RBUTTONUP:
            window->wasKeyReleased[HgKey_rmouse] = true;
            window->isKeyDown[HgKey_rmouse] = false;
            break;
        case WM_MBUTTONUP:
            window->wasKeyReleased[HgKey_mmouse] = true;
            window->isKeyDown[HgKey_mmouse] = false;
            break;
        case WM_MOUSEMOVE:
            window->mousePosX = (f64)LOWORD(lparam) / (f64)window->height;
            window->mousePosY = (f64)HIWORD(lparam) / (f64)window->height;
            break;
    }

    if (imguiInitialized)
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

HgWindow* HgWindow::create(HgArena* arena, const HgWindowConfig& config)
{
    const char* title = config.title != nullptr ? config.title : "Hurdy Gurdy";

    HgWindow* window = hgAlloc<HgWindow>(arena, 1);
    *window = {};
    window->internals = hgAlloc<Internals>(arena, 1);
    *window->internals = {};

    WNDCLASSA windowClass{};
    windowClass.hInstance = win32Instance;
    windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = title;
    windowClass.lpfnWndProc = windowCallback;
    if (!RegisterClassA(&windowClass))
        hgError("Win32 failed to register window class for window: %s\n", config.title);

    if (config.windowed)
    {
        window->width = config.width;
        window->height = config.height;
        window->internals->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            window->width,
            window->height,
            nullptr,
            nullptr,
            win32Instance,
            window
        );
    } else {
        window->width = GetSystemMetrics(SM_CXSCREEN);
        window->height = GetSystemMetrics(SM_CYSCREEN);
        window->internals->hwnd = CreateWindowExA(
            0,
            title,
            title,
            WS_POPUP,
            0,
            0,
            window->width,
            window->height,
            nullptr,
            nullptr,
            win32Instance,
            window
        );
    }
    if (window->internals->hwnd == nullptr)
        hgError("Win32 window creation failed\n");

    PFN_vkCreateWin32SurfaceKHR pfnVkCreateWin32SurfaceKHR
        = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(hgVkInstance, "vkCreateWin32SurfaceKHR");
    if (pfnVkCreateWin32SurfaceKHR == nullptr)
        hgError("Could not load vkCreateWin32SurfaceKHR\n");

    VkWin32SurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hinstance = win32Instance;
    info.hwnd = window->internals->hwnd;

    VkResult result = pfnVkCreateWin32SurfaceKHR(hgVkInstance, &info, nullptr, &window->surface);
    if (window->surface == nullptr)
        hgError("Failed to create Vulkan surface: %s\n", hgVkResultToStr(result));

    hgInternalCreateWindowSwapchain(window, config);

    ShowWindow(window->internals->hwnd, SW_SHOW);
    return window;
}

void HgWindow::destroy()
{
    hgInternalDestroyWindowSwapchain(this);
    vkDestroySurfaceKHR(hgVkInstance, surface, nullptr);
    DestroyWindow(internals->hwnd);
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

void HgWindow::setCursorImage(u32* data, u32 cursorWidth, u32 cursorHeight)
{
    hgError("window setCursorImage : TODO\n");
    (void)data;
    (void)cursorWidth;
    (void)cursorHeight;
}

u32 hgVkGetPlatformExtensions(HgArena* arena, HgStringView** extBuffer)
{
    u32 count = 2;
    *extBuffer = hgAlloc<HgStringView>(arena, count);
    (*extBuffer)[0] = "VK_KHR_surface";
    (*extBuffer)[1] = "VK_KHR_win32_surface";
    return count;
}

void hgProcessWindowEvents(HgWindow** windows, u32 windowCount)
{
    hgAssert(windows != nullptr);

    for (u32 i = 0; i < windowCount; ++i)
    {
        HgWindow* window = windows[i];

        memset(window->wasKeyPressed, 0, sizeof(window->wasKeyPressed));
        memset(window->wasKeyReleased, 0, sizeof(window->wasKeyReleased));

        u32 oldWidth = window->width;
        u32 oldHeight = window->height;
        f64 oldMousePosX = window->mousePosX;
        f64 oldMousePosY = window->mousePosY;

        MSG msg;
        while (PeekMessageA(&msg, window->internals->hwnd, 0, 0, PM_REMOVE) != 0)
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (window->width != oldWidth || window->height != oldHeight)
            hgInternalResizeWindowSwapchain(window);

        window->mouseDeltaX = window->mousePosX - oldMousePosX;
        window->mouseDeltaY = window->mousePosY - oldMousePosY;

        if (window->isKeyDown[HgKey_lshift] && window->isKeyDown[HgKey_rshift])
        {
            bool lshift = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
            bool rshift = (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;
            if (!lshift)
            {
                window->wasKeyReleased[HgKey_lshift] = true;
                window->isKeyDown[HgKey_lshift] = false;
            }
            if (!rshift)
            {
                window->wasKeyReleased[HgKey_rshift] = true;
                window->isKeyDown[HgKey_rshift] = false;
            }
        }
    }
}

#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"

static int ImGui_ImplWin32_CreateVkSurface(
    ImGuiViewport* viewport,
    ImU64 vkInstance,
    const void* vkAllocator,
    ImU64* outVkSurface)
{
    PFN_vkCreateWin32SurfaceKHR pfnVkCreateWin32SurfaceKHR
        = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr((VkInstance)vkInstance, "vkCreateWin32SurfaceKHR");
    if (pfnVkCreateWin32SurfaceKHR == nullptr)
        hgError("Could not load vkCreateWin32SurfaceKHR\n");

    VkWin32SurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hwnd = (HWND)viewport->PlatformHandleRaw;
    info.hinstance = ::GetModuleHandle(nullptr);

    return (int)pfnVkCreateWin32SurfaceKHR(
        (VkInstance)vkInstance,
        &info,
        (VkAllocationCallbacks*)vkAllocator,
        (VkSurfaceKHR*)outVkSurface);
}

void ImGui_ImplHurdyGurdy_Init(
    HgWindow* window,
    u32 colorAttachmentCount,
    const VkFormat* colorFormats,
    VkFormat depthFormat,
    VkFormat stencilFormat)
{
    ImGui_ImplWin32_Init(window->internals->hwnd);
    ImGui::GetPlatformIO().Platform_CreateVkSurface = ImGui_ImplWin32_CreateVkSurface;
    imguiInitialized = true;

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
    ImGui_ImplWin32_Shutdown();
}

void ImGui_ImplHurdyGurdy_NewFrame()
{
    ImGui_ImplWin32_NewFrame();
    ImGui_ImplVulkan_NewFrame();
}

void ImGui_ImplHurdyGurdy_Draw(VkCommandBuffer cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

#endif
