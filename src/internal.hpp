#pragma once

#include "hg/span.hpp"
#include "hg/memory.hpp"
#include "hg/strings.hpp"
#include "hg/gpu.hpp"

namespace hg {

namespace internal {

Span<StringView> getPlatformVulkanExtensions(Arena* arena);

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
