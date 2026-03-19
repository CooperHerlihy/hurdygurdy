#include "hurdygurdy.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>

#include <emmintrin.h>

f64 HgClock::tick()
{
    auto prev = time;
    time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<f64>{time - prev}.count();
}

u32 hgHardwareThreadCount()
{
    return (u32)std::thread::hardware_concurrency();
}

void HgFence::add(u32 count)
{
    counter.fetch_add(count);
}

void HgFence::signal(u32 count)
{
    counter.fetch_sub(count);
}

bool HgFence::isComplete()
{
    return counter.load() == 0;
}

void HgFence::waitIndefinite()
{
    while (!isComplete())
    {
        _mm_pause();
    }
}

bool HgFence::wait(f64 timeout)
{
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout);
    while (!isComplete() && std::chrono::steady_clock::now() < end)
    {
        _mm_pause();
    }
    return isComplete();
}

std::thread* poolThreads = nullptr;
u32 poolThreadCount = 0;
std::atomic_bool poolShouldClose = false;

std::mutex poolMtx{};
std::condition_variable poolCv{};

struct ThreadWork {
    HgFence* fences;
    u32 fenceCount;
    void* data;
    void (*fn)(void*);
};
ThreadWork* poolWork = nullptr;
std::atomic_bool* poolHasWork = nullptr;
u32 poolWorkCapacity = 0;

std::atomic<u32> poolWorkCount = 0;
std::atomic<u32> poolTail = 0;
std::atomic<u32> poolWorkingTail = 0;
std::atomic<u32> poolHead = 0;
std::atomic<u32> poolWorkingHead = 0;

static bool poolExecute()
{
    u32 idx = poolWorkingTail.load();
    do {
        if (idx == poolHead.load())
            return false;
    } while (!poolWorkingTail.compare_exchange_weak(idx, (idx + 1) & (poolWorkCapacity - 1)));

    ThreadWork work = poolWork[idx];
    poolHasWork[idx].store(false);

    u32 t = poolTail.load();
    while (t != poolHead.load() && !poolHasWork[t].load())
    {
        u32 next = (t + 1) & (poolWorkCapacity - 1);
        poolTail.compare_exchange_strong(t, next);
        t = next;
    }

    --poolWorkCount;

    hgAssert(work.fn != nullptr);
    work.fn(work.data);

    for (u32 i = 0; i < work.fenceCount; ++i)
    {
        work.fences[i].signal(1);
    }
    return true;
}

void hgInitThreadPool(HgArena* arena, u32 queueSize, u32 threadCount)
{
    hgAssert(threadCount > 1 && (queueSize & (queueSize - 1)) == 0);

    poolShouldClose.store(false);
    poolThreadCount = std::min((u32)1, threadCount - 1);
    poolThreads = hgAlloc<std::thread>(arena, poolThreadCount);

    poolWork = hgAlloc<ThreadWork>(arena, queueSize);
    poolHasWork = hgAlloc<std::atomic_bool>(arena, queueSize);
    poolWorkCapacity = queueSize;

    poolWorkCount.store(0);
    poolTail.store(0);
    poolWorkingTail.store(0);
    poolHead.store(0);
    poolWorkingHead.store(0);
    for (u32 i = 0; i < poolWorkCapacity; ++i)
    {
        poolHasWork[i].store(false);
    }

    for (u32 i = 0; i < poolThreadCount; ++i)
    {
        new (poolThreads + i) std::thread([] {
            for (;;)
            {
                std::unique_lock<std::mutex> lock{poolMtx};
                while (poolWorkCount.load() == 0 && !poolShouldClose.load())
                {
                    poolCv.wait(lock);
                }
                lock.unlock();
                if (poolShouldClose.load())
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

void hgDeinitThreadPool()
{
    poolMtx.lock();
    poolShouldClose = true;
    poolMtx.unlock();
    poolCv.notify_all();

    for (u32 i = 0; i < poolThreadCount; ++i)
    {
        poolThreads[i].join();
    }
    for (u32 i = 0; i < poolThreadCount; ++i)
    {
        poolThreads[i].~thread();
    }
    poolCv.~condition_variable();
    poolMtx.~mutex();
}

bool hgHelpThreadPool(HgFence& fence, f64 timeout)
{
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout);
    while (!fence.isComplete() && std::chrono::steady_clock::now() < end)
    {
        if (!poolExecute())
            _mm_pause();
    }
    return fence.isComplete();
}

void hgCallPar(HgFence* fences, u32 fenceCount, void* data, void (*fn)(void*))
{
    hgAssert(fn != nullptr);
    for (u32 i = 0; i < fenceCount; ++i)
    {
        fences[i].add(1);
    }

    u32 idx = poolWorkingHead.fetch_add(1) & (poolWorkCapacity - 1);

    poolWork[idx].fences = fences;
    poolWork[idx].fenceCount = fenceCount;
    poolWork[idx].data = data;
    poolWork[idx].fn = fn;
    poolHasWork[idx].store(true);

    u32 h = poolHead.load();
    while (poolHasWork[h].load())
    {
        u32 next = (h + 1) & (poolWorkCapacity - 1);
        poolHead.compare_exchange_strong(h, next);
        h = next;
    }

    ++poolWorkCount;
    poolMtx.lock();
    poolMtx.unlock();
    poolCv.notify_one();
}

void hgForPar(u64 begin, u64 end, void* data, void (*fn)(void* data, u64 idx))
{
    hgAssert(begin <= end);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    u64 chunkSize = (u64)std::ceil((f32)(end - begin) / (8.0f * (f32)hgHardwareThreadCount()));

    HgFence fence;
    for (u64 i = begin; i < end; i += chunkSize)
    {
        struct Capture
        {
            void* data;
            void (*fn)(void* data, u64 idx);
            u64 begin;
            u64 end;
        };

        Capture* capture = hgAlloc<Capture>(scratch, 1);
        capture->data = data;
        capture->fn = fn;
        capture->begin = i;
        capture->end = std::min(i + chunkSize, end);

        hgCallPar(&fence, 1, capture, [](void* pcapture)
        {
            Capture* capture = (Capture*)pcapture;
            for (u64 i = capture->begin; i < capture->end; ++i)
            {
                (capture->fn)(capture->data, i);
            }
        });
    }
    hgHelpThreadPool(fence, INFINITY);
}

std::thread ioThread{};
std::atomic_bool ioThreadShouldClose = false;

struct Request {
    HgFence* fences;
    u32 fenceCount;
    void* resource;
    HgStringView path;
    void (*fn)(void* resource, HgStringView path);
};
Request* ioRequests = nullptr;
std::atomic_bool* ioHasRequest = nullptr;
u32 ioRequestCapacity = 0;

std::atomic<u32> ioTail = 0;
std::atomic<u32> ioHead = 0;
std::atomic<u32> ioWorkingHead = 0;

static bool ioPop()
{
    u32 idx = ioTail.load() & (ioRequestCapacity - 1);
    if (idx == ioHead.load())
        return false;

    Request request = ioRequests[idx];
    ioHasRequest[idx].store(false);

    ioTail.fetch_add(1);

    hgAssert(request.fn != nullptr);
    request.fn(request.resource, request.path);

    for (u32 i = 0; i < request.fenceCount; ++i)
    {
        request.fences[i].signal(1);
    }
    return true;
}

void hgInitIOThread(HgArena* arena, u32 queueSize)
{
    hgAssert(queueSize > 1 && (queueSize & (queueSize - 1)) == 0);

    ioRequests = hgAlloc<Request>(arena, queueSize);
    ioHasRequest = hgAlloc<std::atomic_bool>(arena, queueSize);
    ioRequestCapacity = queueSize;

    ioTail.store(0);
    ioHead.store(0);
    ioWorkingHead.store(0);
    for (u32 i = 0; i < ioRequestCapacity; ++i)
    {
        ioHasRequest[i].store(false);
    }

    ioThreadShouldClose.store(false);
    ioThread = std::thread([]()
            {
        hgInitScratchMemory();
        hgDefer(hgDeinitScratchMemory());

        for (;;)
        {
            if (ioThreadShouldClose.load())
                return;

            if (!ioPop())
                poolExecute();
        }
    });
}

void hgDeinitIOThread()
{
    ioThreadShouldClose = true;
    ioThread.join();
}

void hgRequestIO(
    HgFence* fences,
    u32 fenceCount,
    void* resource,
    HgStringView path,
    void (*fn)(void* resource, HgStringView path)
)
{
    hgAssert(fn != nullptr);
    for (u32 i = 0; i < fenceCount; ++i)
    {
        fences[i].add(1);
    }

    u32 idx = ioWorkingHead.fetch_add(1) & (ioRequestCapacity - 1);

    ioRequests[idx].fences = fences;
    ioRequests[idx].fenceCount = fenceCount;
    ioRequests[idx].resource = resource;
    ioRequests[idx].path = path;
    ioRequests[idx].fn = fn;
    ioHasRequest[idx].store(true);

    u32 h = ioHead.load();
    while (ioHasRequest[h].load())
    {
        u32 next = (h + 1) & (ioRequestCapacity - 1);
        ioHead.compare_exchange_strong(h, next);
        h = next;
    }
}

