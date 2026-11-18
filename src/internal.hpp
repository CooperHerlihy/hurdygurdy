#pragma once

#include "hg/gpu.hpp"

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
