#pragma once

#include "hg/inttypes.hpp"
#include "hg/span.hpp"
#include "hg/memory.hpp"
#include "hg/strings.hpp"
#include "hg/gpu.hpp"
#include "hg/window.hpp"
#include "hg/smart_ptr.hpp"

namespace hg {

namespace internal {

bool initPlatform();
void deinitPlatform();

Span<StringView> platformGetVulkanExtensions(Arena* arena);
bool initGpu();
void deinitGpu();

bool initAudio();
void deinitAudio();

struct SwapchainData;

struct Swapchain {
    UniquePtr<SwapchainData> data = nullptr;

    Swapchain() noexcept;
    ~Swapchain() noexcept;

    u32 width() const;
    u32 height() const;
    Format format() const;
    GpuView* currentView() const;
    u32 imageCount() const;

    static Swapchain create(
        void* platformWindow,
        u32 width,
        u32 height,
        GpuPresentMode presentMode,
        GpuImageUsageFlags imageUsage);

    void resize(u32 width, u32 height);

    Swapchain(Swapchain&& other) noexcept;
    Swapchain& operator=(Swapchain&& other) noexcept;

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
};

void initImGuiWindow(const Window& window);
void initImGuiGpu(
    const Swapchain& swap,
    Format colorFormat,
    Format depthFormat = Format_undefined,
    Format stencilFormat = Format_undefined);

void deinitImGuiWindow();
void deinitImGuiGpu();

void beginImGuiFrameWindow();
void beginImGuiFrameGpu();

} // namespace internal

} // namespace hg
