#pragma once

#include "hurdygurdy.hpp"

namespace hg::internal {

/**
 * Initialize the platform
 *
 * Returns
 * - Whether init succeeded
 */
bool initPlatform();

/**
 * Deinitialize the platform
 */
void deinitPlatform();

/**
 * Get the platform's required instance extensions for windowing
 */
Span<StringView> platformGetVulkanExtensions(Arena* arena);

/**
 * Initialize synchronization and threads
 */
void initConcurrency();

/**
 * Deinitialize synchronization and threads
 */
void deinitConcurrency();

/**
 * Initializes the graphics subsystem, loading all global Vulkan resources
 *
 * Returns
 * - Whether init succeeded
 */
bool initGpu();

/**
 * Deinitializes the graphics subsystem, unloading all global Vulkan resources
 */
void deinitGpu();

/**
 * Initialize the audio subsystem
 *
 * Returns
 * - Whether init succeeded
 */
bool initAudio();

/**
 * Deinitialize the audio subsystem
 */
void deinitAudio();

} // namespace hg

