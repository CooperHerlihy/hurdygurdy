#pragma once

#include "hg_types.hpp"
#include "hg_memory.hpp"

namespace hg {

namespace internal {

/**
 * Initialize for the platform
 */
bool initPlatform();

/**
 * Deinitialize for the platform
 */
void deinitPlatform();

/**
 * Get Vulkan extensions for the platform
 */
Span<StringView> platformGetVulkanExtensions(Arena* arena);

/**
 * Initialize the gpu
 */
bool initGpu();

/**
 * Deinitialize the gpu
 */
void deinitGpu();

/**
 * Initialize audio
 */
bool initAudio();

/**
 * Deinitialize audio
 */
void deinitAudio();

} // namespace internal

} // namespace hg
