#include "hg_imgui.hpp"

#include "internal.hpp"

namespace hg {

void initImGui(
    const Window& window,
    Format colorFormat,
    Format depthFormat,
    Format stencilFormat)
{
    internal::initImGuiWindow(window);
    internal::initImGuiGpu(
        *reinterpret_cast<const internal::Swapchain*>(window.data.ptr),
        colorFormat,
        depthFormat,
        stencilFormat);
}

void deinitImGui()
{
    internal::deinitImGuiWindow();
    internal::deinitImGuiGpu();
}

void beginImGuiFrame()
{
    internal::beginImGuiFrameWindow();
    internal::beginImGuiFrameGpu();
}

} // namespace hg
