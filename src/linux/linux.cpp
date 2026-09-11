#include "linux_platform.hpp"
#include "linux_internal.hpp"
#include "hg/error.hpp"

namespace hg::linux_backend {

bool initPlatform()
{
    if (!loadNative())
        return false;

    if (!windowInit())
        return false;

    if (!initAudio())
        return false;

    return true;
}

void deinitPlatform()
{
    deinitAudio();
    windowDeinit();
}

Span<StringView> getPlatformVulkanExtensions(Arena* arena)
{
    Span<StringView> extBuffer{arena->alloc<StringView>(2), 2};
    extBuffer[0] = "VK_KHR_surface";
    extBuffer[1] = "VK_KHR_xlib_surface";
    return extBuffer;
}

} // namespace hg::linux_backend
