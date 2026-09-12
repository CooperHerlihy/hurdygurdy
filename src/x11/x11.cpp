#include "x11_platform.hpp"

namespace hg::x11 {

Span<StringView> getPlatformVulkanExtensions(Arena* arena)
{
    Span<StringView> extBuffer{arena->alloc<StringView>(2), 2};
    extBuffer[0] = "VK_KHR_surface";
    extBuffer[1] = "VK_KHR_xlib_surface";
    return extBuffer;
}

} // namespace hg::x11
