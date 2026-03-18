#include "hurdygurdy.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>

#include <emmintrin.h>

usize hgHardwareThreadCount()
{
    return std::thread::hardware_concurrency();
}

void HgFence::add(usize count)
{
    counter.fetch_add(count);
}

void HgFence::signal(usize count)
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
usize poolThreadCount = 0;
std::atomic_bool poolShouldClose = false;

std::mutex poolMtx{};
std::condition_variable poolCv{};

struct ThreadWork {
    HgFence* fences;
    usize fenceCount;
    void* data;
    void (*fn)(void*);
};
ThreadWork* poolWork = nullptr;
std::atomic_bool* poolHasWork = nullptr;
usize poolWorkCapacity = 0;

std::atomic<usize> poolWorkCount = 0;
std::atomic<usize> poolTail = 0;
std::atomic<usize> poolWorkingTail = 0;
std::atomic<usize> poolHead = 0;
std::atomic<usize> poolWorkingHead = 0;

static bool poolExecute()
{
    usize idx = poolWorkingTail.load();
    do {
        if (idx == poolHead.load())
            return false;
    } while (!poolWorkingTail.compare_exchange_weak(idx, (idx + 1) & (poolWorkCapacity - 1)));

    ThreadWork work = poolWork[idx];
    poolHasWork[idx].store(false);

    usize t = poolTail.load();
    while (t != poolHead.load() && !poolHasWork[t].load())
    {
        usize next = (t + 1) & (poolWorkCapacity - 1);
        poolTail.compare_exchange_strong(t, next);
        t = next;
    }

    --poolWorkCount;

    hgAssert(work.fn != nullptr);
    work.fn(work.data);

    for (usize i = 0; i < work.fenceCount; ++i)
    {
        work.fences[i].signal(1);
    }
    return true;
}

void hgInitThreadPool(HgArena* arena, usize queueSize, usize threadCount)
{
    hgAssert(threadCount > 1 && (queueSize & (queueSize - 1)) == 0);

    poolShouldClose.store(false);
    poolThreadCount = std::min((usize)1, threadCount - 1);
    poolThreads = hgAlloc<std::thread>(arena, poolThreadCount);

    poolWork = hgAlloc<ThreadWork>(arena, queueSize);
    poolHasWork = hgAlloc<std::atomic_bool>(arena, queueSize);
    poolWorkCapacity = queueSize;

    poolWorkCount.store(0);
    poolTail.store(0);
    poolWorkingTail.store(0);
    poolHead.store(0);
    poolWorkingHead.store(0);
    for (usize i = 0; i < poolWorkCapacity; ++i)
    {
        poolHasWork[i].store(false);
    }

    for (usize i = 0; i < poolThreadCount; ++i)
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

                static constexpr usize spinCount = 128;
                for (usize j = 0; j < spinCount; ++j)
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

    for (usize i = 0; i < poolThreadCount; ++i)
    {
        poolThreads[i].join();
    }
    for (usize i = 0; i < poolThreadCount; ++i)
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

void hgCallPar(HgFence* fences, usize fenceCount, void* data, void (*fn)(void*))
{
    hgAssert(fn != nullptr);
    for (usize i = 0; i < fenceCount; ++i)
    {
        fences[i].add(1);
    }

    usize idx = poolWorkingHead.fetch_add(1) & (poolWorkCapacity - 1);

    poolWork[idx].fences = fences;
    poolWork[idx].fenceCount = fenceCount;
    poolWork[idx].data = data;
    poolWork[idx].fn = fn;
    poolHasWork[idx].store(true);

    usize h = poolHead.load();
    while (poolHasWork[h].load())
    {
        usize next = (h + 1) & (poolWorkCapacity - 1);
        poolHead.compare_exchange_strong(h, next);
        h = next;
    }

    ++poolWorkCount;
    poolMtx.lock();
    poolMtx.unlock();
    poolCv.notify_one();
}

std::thread ioThread{};
std::atomic_bool ioThreadShouldClose = false;

struct Request {
    HgFence* fences;
    usize fenceCount;
    void* resource;
    HgStringView path;
    void (*fn)(void* resource, HgStringView path);
};
Request* ioRequests = nullptr;
std::atomic_bool* ioHasRequest = nullptr;
usize ioRequestCapacity = 0;

std::atomic<usize> ioTail = 0;
std::atomic<usize> ioHead = 0;
std::atomic<usize> ioWorkingHead = 0;

static bool ioPop()
{
    usize idx = ioTail.load() & (ioRequestCapacity - 1);
    if (idx == ioHead.load())
        return false;

    Request request = ioRequests[idx];
    ioHasRequest[idx].store(false);

    ioTail.fetch_add(1);

    hgAssert(request.fn != nullptr);
    request.fn(request.resource, request.path);

    for (usize i = 0; i < request.fenceCount; ++i)
    {
        request.fences[i].signal(1);
    }
    return true;
}

void hgInitIOThread(HgArena* arena, usize queueSize)
{
    hgAssert(queueSize > 1 && (queueSize & (queueSize - 1)) == 0);

    ioRequests = hgAlloc<Request>(arena, queueSize);
    ioHasRequest = hgAlloc<std::atomic_bool>(arena, queueSize);
    ioRequestCapacity = queueSize;

    ioTail.store(0);
    ioHead.store(0);
    ioWorkingHead.store(0);
    for (usize i = 0; i < ioRequestCapacity; ++i)
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
    usize fenceCount,
    void* resource,
    HgStringView path,
    void (*fn)(void* resource, HgStringView path)
)
{
    hgAssert(fn != nullptr);
    for (usize i = 0; i < fenceCount; ++i)
    {
        fences[i].add(1);
    }

    usize idx = ioWorkingHead.fetch_add(1) & (ioRequestCapacity - 1);

    ioRequests[idx].fences = fences;
    ioRequests[idx].fenceCount = fenceCount;
    ioRequests[idx].resource = resource;
    ioRequests[idx].path = path;
    ioRequests[idx].fn = fn;
    ioHasRequest[idx].store(true);

    usize h = ioHead.load();
    while (ioHasRequest[h].load())
    {
        usize next = (h + 1) & (ioRequestCapacity - 1);
        ioHead.compare_exchange_strong(h, next);
        h = next;
    }
}

