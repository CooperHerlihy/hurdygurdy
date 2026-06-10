/*
 * =============================================================================
 *
 * Copyright (c) 2025-2026 Cooper Herlihy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 */

#ifndef HG_PLATFORM_HPP
#define HG_PLATFORM_HPP

#include "hg_core.hpp"
#include "hg_memory.hpp"
#include "hg_strings.hpp"

/**
 * Initializes global platform resources
 *
 * Parameters
 * - arena The arena to allocate from
 * - maxWindows The maximum number of windows that can be created
 * - maxEvents The maximum number of events recorded per frame
 * - maxAudioPlayers The maximum number of audio players
 */
void hgPlatformInit(HgArena* arena, u32 maxWindows, u32 maxEvents, u32 maxAudioPlayers);

/**
 * Deinitializes global platform resources
 */
void hgPlatformDeinit();

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
u32 hgPlatformGetVulkanExtensions(HgArena* arena, HgStringView** extBuffer);

#endif // HG_PLATFORM_HPP
