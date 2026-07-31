#include "hg/concurrency.hpp"
#include "hg/array.hpp"
#include "hg/time.hpp"

#include <cmath>
#include <condition_variable>
#include <mutex>
#include <emmintrin.h>

namespace hg {

void SpinLock::acquire()
{
    bool acquiredLocal = false;
    while (!acquired.compare_exchange_weak(acquiredLocal, true))
    {
        _mm_pause();
        acquiredLocal = false;
    }
}

bool SpinLock::tryAcquire()
{
    bool acquiredLocal = false;
    return acquired.compare_exchange_weak(acquiredLocal, true);
}

void SpinLock::release()
{
    acquired.store(false);
}

void Fence::add(u32 count)
{
    counter.fetch_add(count);
}

void Fence::signal(u32 count)
{
    [[maybe_unused]]
    u32 prev = counter.fetch_sub(count);
    HG_ASSERT(prev >= count);
}

bool Fence::isComplete()
{
    return counter.load() == 0;
}

bool Fence::wait(f64 timeout)
{
    Clock c{};
    while (!isComplete() && (timeout -= c.tick()) > 0)
    {
        _mm_pause();
    }
    return isComplete();
}

void Fence::waitIndefinite()
{
    while (!isComplete())
    {
        _mm_pause();
    }
}

struct ThreadWork {
    Fence* fence = nullptr;
    void* data = nullptr;
    void (*fn)(void*) = nullptr;
};

struct ThreadPoolState {
    Array<ThreadWork> work{};
    Array<std::atomic_bool> hasWork{};

    std::atomic<u32> workCount = 0;
    std::atomic<u32> tail = 0;
    std::atomic<u32> workingTail = 0;
    std::atomic<u32> head = 0;
    std::atomic<u32> workingHead = 0;

    std::mutex mtx{};
    std::condition_variable_any cv{};
    Array<std::jthread> threads{};

    ThreadPoolState() noexcept = default;

    ThreadPoolState(u64 workCapacity, u32 threadCount)
    {
        work = {workCapacity, workCapacity};
        hasWork = {workCapacity, workCapacity};
        for (std::atomic_bool& hw : hasWork)
            hw.store(false);

        workCount.store(0);
        tail.store(0);
        workingTail.store(0);
        head.store(0);
        workingHead.store(0);

        auto threadFn = [this](std::stop_token st) {
            while (!st.stop_requested())
            {
                static constexpr u32 spinCount = 128;
                for (u32 j = 0; j < spinCount; ++j)
                {
                    if (!execute())
                        _mm_pause();
                }

                std::unique_lock lock{mtx};
                cv.wait(lock, st, [&] {
                    return workCount.load() > 0 || st.stop_requested();
                });
            }
        };

        threads.reserve(threadCount);
        for (u32 i = 0; i < threadCount; ++i)
            threads.push(std::jthread{threadFn});
    }

    ThreadPoolState(ThreadPoolState&&) = delete;
    ThreadPoolState& operator=(ThreadPoolState&&) = delete;
    ThreadPoolState(const ThreadPoolState&) = delete;
    ThreadPoolState& operator=(const ThreadPoolState&) = delete;

    bool execute()
    {
        u32 idx = workingTail.load();
        do {
            if (idx == head.load())
                return false;
        } while (!workingTail.compare_exchange_weak(idx, (idx + 1) & (static_cast<u32>(work.count) - 1)));

        ThreadWork w = work[idx];
        hasWork[idx].store(false);

        u32 t = tail.load();
        while (t != head.load() && !hasWork[t].load())
        {
            u32 next = (t + 1) & (static_cast<u32>(work.count) - 1);
            tail.compare_exchange_strong(t, next);
            t = next;
        }

        --workCount;

        HG_ASSERT(w.fn != nullptr);
        w.fn(w.data);

        if (w.fence != nullptr)
            w.fence->signal();
        return true;
    }
};

static ThreadPoolState& threadPool()
{
    static ThreadPoolState pool{4096, std::max(
        (u32)1, std::thread::hardware_concurrency() - 1)};
    return pool;
}

bool helpThreads(Fence* fence, f64 timeout)
{
    Clock c{};
    while (!fence->isComplete() && (timeout -= c.tick()) > 0)
    {
        if (!threadPool().execute())
            _mm_pause();
    }
    return fence->isComplete();
}

void callPar(Fence* fence, void* data, void (*fn)(void* data))
{
    ThreadPoolState& pool = threadPool();

    HG_ASSERT(fn != nullptr);
    if (fence != nullptr)
        fence->add();

    u32 idx = pool.workingHead.fetch_add(1) & (static_cast<u32>(pool.work.count) - 1);

    pool.work[idx].fence = fence;
    pool.work[idx].data = data;
    pool.work[idx].fn = fn;
    pool.hasWork[idx].store(true);

    u32 h = pool.head.load();
    while (pool.hasWork[h].load())
    {
        u32 next = (h + 1) & (static_cast<u32>(pool.work.count) - 1);
        pool.head.compare_exchange_strong(h, next);
        h = next;
    }

    ++pool.workCount;
    pool.cv.notify_one();
}

void forPar(u64 begin, u64 end, void* data, void (*fn)(void* data, u64 idx))
{
    HG_ASSERT(begin <= end);
    HG_ASSERT(fn != nullptr);

    ArenaScope scratch = getScratch();

    u64 chunkSize = static_cast<u64>(std::ceil(static_cast<f64>(end - begin) / (8.0 * static_cast<f64>(threadPool().threads.count))));

    Fence fence{};
    for (u64 i = begin; i < end; i += chunkSize)
    {
        struct Capture
        {
            void* data = nullptr;
            void (*fn)(void* data, u64 idx) = nullptr;
            u64 begin = 0;
            u64 end = 0;
        };

        Capture* capture = scratch.alloc<Capture>(1);
        capture->data = data;
        capture->fn = fn;
        capture->begin = i;
        capture->end = std::min(i + chunkSize, end);

        callPar(&fence, capture, [](void* pcapture)
        {
            Capture* capture = static_cast<Capture*>(pcapture);
            for (u64 i = capture->begin; i < capture->end; ++i)
            {
                (capture->fn)(capture->data, i);
            }
        });
    }
    helpThreads(&fence, INFINITY);
}

} // namespace hg
