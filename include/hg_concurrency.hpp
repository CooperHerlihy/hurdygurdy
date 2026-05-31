/*
 * =============================================================================
 *
 * Copyright (c) 2025 Cooper Herlihy
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

#ifndef HG_CONCURRENCY_HPP
#define HG_CONCURRENCY_HPP

#include "hg_core.hpp"
#include "hg_memory.hpp"
#include "hg_templates.hpp"

/**
 * Returns the number of concurrent threads available in hardware
 */
u32 hgHardwareThreadCount();

/**
 * Initialize synchronization primitives
 */
void hgConcurrencyInit(HgArena* arena, u32 maxMutices, u32 maxFences);

/**
 * Deinitialize synchronization primitives
 */
void hgConcurrencyDeinit();

/**
 * A spinlock mutex for basic thread synchronization
 */
struct HgMutex {
    HgHandle handle;
};

/**
 * Create a new mutex
 */
HgMutex hgMutexCreate();

/**
 * Destroy a mutex
 */
void hgMutexDestroy(HgMutex mtx);

/**
 * Wait until the mutex is acquired
 */
void hgMutexAcquire(HgMutex mtx);

/**
 * Try to acquire the mutex
 *
 * Returns
 * - true if acquisition succeeded
 * - false if the mutex was already in use
 */
bool hgMutexTryAcquire(HgMutex mtx);

/**
 * Release the mutex lock
 */
void hgMutexRelease(HgMutex mtx);

/**
 * A spinlock fence for basic thread synchronization
 */
struct HgFence {
    HgHandle handle;
};

/**
 * Create a new fence
 */
HgFence hgFenceCreate();

/**
 * Destroy a fence
 */
void hgFenceDestroy(HgFence fence);

/**
 * Add more events for the fence to wait on
 *
 * Parameters
 * - fence The fence to attach to
 * - count The number of added events
 */
void hgFenceAttach(HgFence fence, u32 count);

/**
 * Signal that events have completed
 *
 * Parameters
 * - fence The fence to signal
 * - count The number of signaled events
 */
void hgFenceSignal(HgFence fence, u32 count);

/**
 * Returns whether all work has been completed
 */
bool hgFenceIsComplete(HgFence fence);

/**
 * Spin waits for all work submissions to be completed
 *
 * Parameters
 * - fence The fence to wait on
 * - timeoutSeconds The time in seconds to wait before timing out
 *
 * Returns
 * - true if the fence was completed, false if the timeout was triggered
 */
bool hgFenceWait(HgFence fence, f64 timeoutSeconds);

/**
 * Spin waits for all work submissions to be completed
 */
void hgFenceWaitIndefinite(HgFence fence);

/**
 * Initialize the thread pool
 *
 * Note, the recommended thread is the hardware thread count minus dedicated
 * threads such as main thread, IO thread, etc.
 *
 * Parameters
 * - arena The arena to allocate from
 * - queueSize The max capacity of the thread work queue
 * - threadCount The number of threads to spawn in the pool
 */
void hgThreadsInit(HgArena* arena, u32 queueSize, u32 threadCount);

/**
 * Deinitialize the thread pool
 */
void hgThreadsDeinit();

/**
 * Wait on a fence, and help complete work in the meantime
 *
 * Parameters
 * - fence The fence to wait on
 * - timeout The max time in seconds to spend working
 *
 * Returns
 * - true if the fence was completed
 * - false if the timeout was reached
 */
bool hgThreadsHelp(HgFence fence, f64 timeout);

/**
 * Pushes work to the thread pool queue to be executed
 *
 * Parameters
 * - fence The fences to signal upon completion
 * - data The data passed to the function
 * - work The function to be executed
 */
void hgThreadsCall(HgFence fence, void* data, void (*fn)(void* data));

/**
 * Iterates in parallel over a function n times using the thread pool
 *
 * Note, uses a fence internally to wait for all work to complete
 *
 * Parameters
 * - begin The first index to iterate from
 * - end The end index to iterate to
 * - data The data pointer passed to fn
 * - fn The function to use to iterate, takes the index
 */
void hgThreadsFor(u64 begin, u64 end, void* data, void (*fn)(void* data, u64 idx));

#endif // HG_CONCURRENCY_HPP
