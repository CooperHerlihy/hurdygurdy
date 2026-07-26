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

#pragma once

#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <atomic>
#include <bit>
#include <concepts>
#include <thread>
#include <type_traits>
#include <utility>

// ============================================================================
// Contents
// ============================================================================
// Include order respects inter-section dependencies. Config and macros must
// come before all others. Core types must precede error handling and memory.
// Template method bodies must be last.
// ============================================================================

namespace hg {

#include "hurdygurdy/config.hpp"
#include "hurdygurdy/macros.hpp"
#include "hurdygurdy/core.hpp"
#include "hurdygurdy/error.hpp"
#include "hurdygurdy/init.hpp"
#include "hurdygurdy/utility.hpp"
#include "hurdygurdy/memory.hpp"
#include "hurdygurdy/concurrency.hpp"
#include "hurdygurdy/math.hpp"
#include "hurdygurdy/geometry_2d.hpp"
#include "hurdygurdy/geometry_3d.hpp"
#include "hurdygurdy/noise.hpp"
#include "hurdygurdy/strings.hpp"
#include "hurdygurdy/containers.hpp"
#include "hurdygurdy/asset.hpp"
#include "hurdygurdy/serialization.hpp"
#include "hurdygurdy/timing.hpp"
#include "hurdygurdy/dynlib.hpp"
#include "hurdygurdy/gpu.hpp"
#include "hurdygurdy/window.hpp"
#include "hurdygurdy/audio.hpp"
#include "hurdygurdy/rendering.hpp"
#include "hurdygurdy/ecs.hpp"
#include "hurdygurdy/templates.hpp"

} // namespace hg