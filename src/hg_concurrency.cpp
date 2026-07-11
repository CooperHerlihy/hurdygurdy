#include "hg_concurrency.hpp"
#include "hg_containers.hpp"
#include "hg_core.hpp"
#include "hg_memory.hpp"
#include "hg_time.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <emmintrin.h>

namespace hg {

struct Mutex {
    std::atomic_bool acquired;
};

static Pool mutices{};

struct Fence {
    std::atomic<u32> counter;
};

static Pool fences{};

struct ThreadWork {
    Fence* fence;
    void* data;
    void (*fn)(void*);
};

struct ThreadPoolState {
    std::thread* threads = nullptr;
    u32 threadCount = 0;
    std::atomic_bool shouldClose = false;

    std::mutex mtx{};
    std::condition_variable cv{};

    ThreadWork* work = nullptr;
    std::atomic_bool* hasWork = nullptr;
    u32 workCapacity = 0;

    std::atomic<u32> workCount = 0;
    std::atomic<u32> tail = 0;
    std::atomic<u32> workingTail = 0;
    std::atomic<u32> head = 0;
    std::atomic<u32> workingHead = 0;
};

static ThreadPoolState threadPool{};

static bool poolExecute()
{
    u32 idx = threadPool.workingTail.load();
    do {
        if (idx == threadPool.head.load())
            return false;
    } while (!threadPool.workingTail.compare_exchange_weak(idx, (idx + 1) & (threadPool.workCapacity - 1)));

    ThreadWork work = threadPool.work[idx];
    threadPool.hasWork[idx].store(false);

    u32 t = threadPool.tail.load();
    while (t != threadPool.head.load() && !threadPool.hasWork[t].load())
    {
        u32 next = (t + 1) & (threadPool.workCapacity - 1);
        threadPool.tail.compare_exchange_strong(t, next);
        t = next;
    }

    --threadPool.workCount;

    HG_ASSERT(work.fn != nullptr);
    work.fn(work.data);

    if (work.fence != nullptr)
        fenceSignal(work.fence, 1);
    return true;
}

static void poolInit()
{
    threadPool.shouldClose.store(false);
    threadPool.threadCount = max((u32)1, std::thread::hardware_concurrency() - 1);
    threadPool.threads = gpaAlloc<std::thread>(threadPool.threadCount);

    threadPool.workCapacity = 4096;
    threadPool.work = gpaAlloc<ThreadWork>(threadPool.workCapacity);
    threadPool.hasWork = gpaAlloc<std::atomic_bool>(threadPool.workCapacity);

    threadPool.workCount.store(0);
    threadPool.tail.store(0);
    threadPool.workingTail.store(0);
    threadPool.head.store(0);
    threadPool.workingHead.store(0);

    for (u32 i = 0; i < threadPool.workCapacity; ++i)
    {
        threadPool.hasWork[i].store(false);
    }

    for (u32 i = 0; i < threadPool.threadCount; ++i)
    {
        new (threadPool.threads + i) std::thread([] {
            scratchInit(2, 1 << 16);
            HG_DEFER(scratchDeinit());

            for (;;)
            {
                std::unique_lock<std::mutex> lock{threadPool.mtx};
                while (threadPool.workCount.load() == 0 && !threadPool.shouldClose.load())
                {
                    threadPool.cv.wait(lock);
                }
                lock.unlock();
                if (threadPool.shouldClose.load())
                    return;

                static constexpr u32 spinCount = 128;
                for (u32 j = 0; j < spinCount; ++j)
                {
                    if (!poolExecute())
                        _mm_pause();
                }
            }
        });
    }
}

void concurrencyInit()
{
    mutices = poolCreate<Mutex>();
    fences = poolCreate<Fence>();

    poolInit();
}

static void poolDeinit()
{
    threadPool.mtx.lock();
    threadPool.shouldClose = true;
    threadPool.mtx.unlock();
    threadPool.cv.notify_all();

    for (u32 i = 0; i < threadPool.threadCount; ++i)
    {
        threadPool.threads[i].join();
    }
    for (u32 i = 0; i < threadPool.threadCount; ++i)
    {
        threadPool.threads[i].~thread();
    }
    threadPool.cv.~condition_variable();
    threadPool.mtx.~mutex();

    gpaFree(threadPool.hasWork, threadPool.workCapacity);
    gpaFree(threadPool.work, threadPool.workCapacity);
    gpaFree(threadPool.threads, threadPool.threadCount);
}

void concurrencyDeinit()
{
    poolDeinit();

    poolDestroy(&fences);
    poolDestroy(&mutices);
}

Mutex* mutexCreate()
{
    return new (poolAlloc(&mutices)) Mutex{false};
}

void mutexDestroy(Mutex* mtx)
{
    poolFree(&mutices, mtx);
}

void mutexAcquire(Mutex* mtx)
{
    HG_ASSERT(mtx != nullptr);

    bool acquired = false;
    while (!mtx->acquired.compare_exchange_weak(acquired, true))
    {
        _mm_pause();
        acquired = false;
    }
}

bool mutexTryAcquire(Mutex* mtx)
{
    HG_ASSERT(mtx != nullptr);

    bool acquired = false;
    return mtx->acquired.compare_exchange_weak(acquired, true);
}

void mutexRelease(Mutex* mtx)
{
    HG_ASSERT(mtx != nullptr);
    HG_ASSERT(mtx->acquired);

    mtx->acquired.store(false);
}

Fence* fenceCreate()
{
    return new (poolAlloc(&fences)) Fence{0};
}

void fenceDestroy(Fence* fence)
{
    HG_ASSERT(fence != nullptr);
    if (fence != nullptr)
        poolFree(&fences, fence);
}

void fenceAttach(Fence* fence, u32 count)
{
    HG_ASSERT(fence != nullptr);
    fence->counter.fetch_add(count);
}

void fenceSignal(Fence* fence, u32 count)
{
    HG_ASSERT(fence != nullptr);
    fence->counter.fetch_sub(count);
}

bool fenceIsComplete(Fence* fence)
{
    HG_ASSERT(fence != nullptr);
    return fence->counter.load() == 0;
}

bool fenceWait(Fence* fence, f64 timeout)
{
    HG_ASSERT(fence != nullptr);

    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout);
    while (!fenceIsComplete(fence) && std::chrono::steady_clock::now() < end)
    {
        _mm_pause();
    }

    return fenceIsComplete(fence);
}

void fenceWaitIndefinite(Fence* fence)
{
    HG_ASSERT(fence != nullptr);

    while (!fenceIsComplete(fence))
    {
        _mm_pause();
    }
}

void threadsCall(Fence* fence, void* data, void (*fn)(void* data))
{
    HG_ASSERT(fn != nullptr);
    if (fence != nullptr)
        fenceAttach(fence, 1);

    u32 idx = threadPool.workingHead.fetch_add(1) & (threadPool.workCapacity - 1);

    threadPool.work[idx].fence = fence;
    threadPool.work[idx].data = data;
    threadPool.work[idx].fn = fn;
    threadPool.hasWork[idx].store(true);

    u32 h = threadPool.head.load();
    while (threadPool.hasWork[h].load())
    {
        u32 next = (h + 1) & (threadPool.workCapacity - 1);
        threadPool.head.compare_exchange_strong(h, next);
        h = next;
    }

    ++threadPool.workCount;
    threadPool.mtx.lock();
    threadPool.mtx.unlock();
    threadPool.cv.notify_one();
}

bool threadsHelp(Fence* fence, f64 timeout)
{
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout);
    while (!fenceIsComplete(fence) && std::chrono::steady_clock::now() < end)
    {
        if (!poolExecute())
            _mm_pause();
    }
    return fenceIsComplete(fence);
}

void threadsFor(u64 begin, u64 end, void* data, void (*fn)(void* data, u64 idx))
{
    HG_ASSERT(begin <= end);
    HG_ASSERT(fn != nullptr);

    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    u64 chunkSize = (u64)std::ceil((f32)(end - begin) / (8.0f * (f32)threadPool.threadCount));

    Fence* fence = fenceCreate();
    HG_DEFER(fenceDestroy(fence));

    for (u64 i = begin; i < end; i += chunkSize)
    {
        struct Capture
        {
            void* data;
            void (*fn)(void* data, u64 idx);
            u64 begin;
            u64 end;
        };

        Capture* capture = arenaAlloc<Capture>(sc, 1);
        capture->data = data;
        capture->fn = fn;
        capture->begin = i;
        capture->end = min(i + chunkSize, end);

        threadsCall(fence, capture, [](void* pcapture)
        {
            Capture* capture = (Capture*)pcapture;
            for (u64 i = capture->begin; i < capture->end; ++i)
            {
                (capture->fn)(capture->data, i);
            }
        });
    }
    threadsHelp(fence, INFINITY);
}

} // namespace hg

