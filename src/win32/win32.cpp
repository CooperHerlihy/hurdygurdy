#include "win32_platform.hpp"

#include "hg/error.hpp"
#include "hg/dynlib.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

namespace hg::win32 {

static HINSTANCE hInstance = nullptr;

bool windowInit(HINSTANCE hInst);
void windowDeinit();

bool initAudio();
void deinitAudio();

bool loadWin32()
{
    hInstance = GetModuleHandleA(nullptr);
    if (hInstance == nullptr)
    {
        setError("Could not get module handle");
        return false;
    }
    return true;
}

bool initWin32()
{
    if (!windowInit(hInstance))
        return false;

    if (!initAudio())
    {
        windowDeinit();
        return false;
    }

    return true;
}

void deinitWin32()
{
    deinitAudio();
    windowDeinit();
}

Span<StringView> getPlatformVulkanExtensions(Arena* arena)
{
    Span<StringView> extBuffer{arena->alloc<StringView>(2), 2};
    extBuffer[0] = "VK_KHR_surface";
    extBuffer[1] = "VK_KHR_win32_surface";
    return extBuffer;
}

} // namespace hg::win32
