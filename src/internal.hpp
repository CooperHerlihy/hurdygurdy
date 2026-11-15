#pragma once

// #include "hg/memory.hpp"
#include "hg/gpu.hpp"

#include <string_view>

namespace hg {

namespace internal {

void* getVkInstance();
void* getVkInstanceProcAddr(const char* name);

void convertPixelFont();

void initImGuiGpu(
    const GpuSwapchain& swap,
    Format colorFormat,
    Format depthFormat = Format_undefined,
    Format stencilFormat = Format_undefined);

void deinitImGuiGpu();

void beginImGuiFrameGpu();

} // namespace internal

} // namespace hg
