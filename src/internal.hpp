#pragma once

// #include "hg/memory.hpp"
#include "hg/gpu.hpp"

#include <string_view>

namespace hg {

namespace internal {

std::span<std::string_view> getPlatformVulkanExtensions();

bool initGpu();
void deinitGpu();
void* getVkInstance();
void* getVkInstanceProcAddr(const char* name);

void initPixelFont();
void deinitPixelFont();

void initImGuiGpu(
    const GpuSwapchain& swap,
    Format colorFormat,
    Format depthFormat = Format_undefined,
    Format stencilFormat = Format_undefined);

void deinitImGuiGpu();

void beginImGuiFrameGpu();

} // namespace internal

} // namespace hg
