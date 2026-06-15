#include "hg_concurrency.hpp"
#include "hg_containers.hpp"
#include "hg_core.hpp"
#include "hg_memory.hpp"
#include "hg_time.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <emmintrin.h>

struct HgMutex {
    std::atomic_bool acquired;
};

static HgPool mutices{};

HgMutex* hgMutexCreate()
{
    return new (hgPoolAlloc(&mutices)) HgMutex{false};
}

void hgMutexDestroy(HgMutex* mtx)
{
    hgPoolFree(&mutices, mtx);
}

void hgMutexAcquire(HgMutex* mtx)
{
    hgAssert(mtx != nullptr);

    bool acquired = false;
    while (!mtx->acquired.compare_exchange_weak(acquired, true))
    {
        _mm_pause();
        acquired = false;
    }
}

bool hgMutexTryAcquire(HgMutex* mtx)
{
    hgAssert(mtx != nullptr);

    bool acquired = false;
    return mtx->acquired.compare_exchange_weak(acquired, true);
}

void hgMutexRelease(HgMutex* mtx)
{
    hgAssert(mtx != nullptr);
    hgAssert(mtx->acquired);

    mtx->acquired.store(false);
}

struct HgFence {
    std::atomic<u32> counter;
};

static HgPool fences{};

HgFence* hgFenceCreate()
{
    return new (hgPoolAlloc(&fences)) HgFence{0};
}

void hgFenceDestroy(HgFence* fence)
{
    hgAssert(fence != nullptr);
    if (fence != nullptr)
        hgPoolFree(&fences, fence);
}

void hgFenceAttach(HgFence* fence, u32 count)
{
    hgAssert(fence != nullptr);
    fence->counter.fetch_add(count);
}

void hgFenceSignal(HgFence* fence, u32 count)
{
    hgAssert(fence != nullptr);
    fence->counter.fetch_sub(count);
}

bool hgFenceIsComplete(HgFence* fence)
{
    hgAssert(fence != nullptr);
    return fence->counter.load() == 0;
}

bool hgFenceWait(HgFence* fence, f64 timeoutSeconds)
{
    hgAssert(fence != nullptr);

    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeoutSeconds);
    while (!hgFenceIsComplete(fence) && std::chrono::steady_clock::now() < end)
    {
        _mm_pause();
    }

    return hgFenceIsComplete(fence);
}

void hgFenceWaitIndefinite(HgFence* fence)
{
    hgAssert(fence != nullptr);

    while (!hgFenceIsComplete(fence))
    {
        _mm_pause();
    }
}

struct ThreadWork {
    HgFence* fence;
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

    hgAssert(work.fn != nullptr);
    work.fn(work.data);

    if (work.fence != nullptr)
        hgFenceSignal(work.fence, 1);
    return true;
}

void hgThreadsCall(HgFence* fence, void* data, void (*fn)(void* data))
{
    hgAssert(fn != nullptr);
    if (fence != nullptr)
        hgFenceAttach(fence, 1);

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

bool hgThreadsHelp(HgFence* fence, f64 timeout)
{
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout);
    while (!hgFenceIsComplete(fence) && std::chrono::steady_clock::now() < end)
    {
        if (!poolExecute())
            _mm_pause();
    }
    return hgFenceIsComplete(fence);
}

void hgThreadsFor(u64 begin, u64 end, void* data, void (*fn)(void* data, u64 idx))
{
    hgAssert(begin <= end);
    hgAssert(fn != nullptr);

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    u64 chunkSize = (u64)std::ceil((f32)(end - begin) / (8.0f * (f32)threadPool.threadCount));

    HgFence* fence = hgFenceCreate();
    hgDefer(hgFenceDestroy(fence));

    for (u64 i = begin; i < end; i += chunkSize)
    {
        struct Capture
        {
            void* data;
            void (*fn)(void* data, u64 idx);
            u64 begin;
            u64 end;
        };

        Capture* capture = hgArenaAlloc<Capture>(scratch, 1);
        capture->data = data;
        capture->fn = fn;
        capture->begin = i;
        capture->end = hgMin(i + chunkSize, end);

        hgThreadsCall(fence, capture, [](void* pcapture)
        {
            Capture* capture = (Capture*)pcapture;
            for (u64 i = capture->begin; i < capture->end; ++i)
            {
                (capture->fn)(capture->data, i);
            }
        });
    }
    hgThreadsHelp(fence, INFINITY);
}

void hgConcurrencyInit()
{
    mutices = hgPoolCreate<HgMutex>();
    fences = hgPoolCreate<HgFence>();

    threadPool.shouldClose.store(false);
    threadPool.threadCount = hgMax((u32)1, std::thread::hardware_concurrency() - 1);
    threadPool.threads = hgGpaAlloc<std::thread>(threadPool.threadCount);

    threadPool.workCapacity = 4096;
    threadPool.work = hgGpaAlloc<ThreadWork>(threadPool.workCapacity);
    threadPool.hasWork = hgGpaAlloc<std::atomic_bool>(threadPool.workCapacity);

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
            hgScratchInit(2, 1 << 16);
            hgDefer(hgScratchDeinit());

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

void hgConcurrencyDeinit()
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

    hgGpaFree(threadPool.hasWork, threadPool.workCapacity);
    hgGpaFree(threadPool.work, threadPool.workCapacity);
    hgGpaFree(threadPool.threads, threadPool.threadCount);

    hgPoolDestroy(&fences);
    hgPoolDestroy(&mutices);
}

f64 hgClockTick(HgClock* clock)
{
    hgAssert(clock != nullptr);

    f64 prev = clock->time;
    timespec ts;
    timespec_get(&ts, TIME_UTC);
    clock->time = (f64)ts.tv_sec + (f64)ts.tv_nsec * 1e-9;
    return clock->time - prev;
}

