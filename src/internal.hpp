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

} // namespace hg

