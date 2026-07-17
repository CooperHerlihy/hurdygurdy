#pragma once

#include "hurdygurdy.hpp"

namespace hg::internal {

/**
 * Initialize the platform
 *
 * Returns
 * - Whether init succeeded
 */
bool platformInit();

/**
 * Deinitialize the platform
 */
void platformDeinit();

/**
 * Get the platform's required instance extensions for windowing
 *
 * Parameters
 * - arena The arena to allocate from
 * - extBuffer A pointer to store the extension names
 *
 * Returns
 * - The number of required extensions
 */
u32 platformGetVulkanExtensions(Arena* arena, StringView** extBuffer);

/**
 * Initializes scratch arenas on this thread
 *
 * Parameters
 * - count The number of arenas to allocate
 * - size The size of each arena in bytes
 */
void initScratch(u32 count, u64 size);

/**
 * Deinitializes scratch arenas
 */
void deinitScratch();

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
 * Initialize the windowing subsystem
 */
void initWindowing();

/**
 * Deinitialize the windowing subsystem
 */
void deinitWindowing();

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

