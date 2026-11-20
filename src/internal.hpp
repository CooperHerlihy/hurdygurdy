#pragma once

#include "hg/gpu.hpp"

namespace hg {

namespace internal {

void* getVkInstance(Gpu* gpu);
void* getVkInstanceProcAddr(Gpu* gpu, const char* name);

void initImGuiGpu(
    GpuSwapchain* swap,
    Format colorFormat,
    Format depthFormat = Format::undefined,
    Format stencilFormat = Format::undefined);

void deinitImGuiGpu();

void beginImGuiFrameGpu();

void convertPixelFont();

} // namespace internal

} // namespace hg
