#pragma once

#include "hg/macros.hpp"
#include "hg/span.hpp"
#include "hg/memory.hpp"
#include "hg/strings.hpp"
#include "hg/gpu.hpp"

namespace hg {

namespace internal {

bool initPlatform();
void deinitPlatform();
Span<StringView> getPlatformVulkanExtensions(Arena* arena);

bool initGpu();
void deinitGpu();
void* getVulkanInstance();
void* getVulkanInstanceProcAddr(const char* name);

void initRender2D();
void deinitRender2D();

void initImGuiGpu(
    const GpuSwapchain& swap,
    Format colorFormat,
    Format depthFormat = Format_undefined,
    Format stencilFormat = Format_undefined);

void deinitImGuiGpu();

void beginImGuiFrameGpu();

} // namespace internal

} // namespace hg
