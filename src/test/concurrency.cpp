#include "tests.hpp"
#include "hg/concurrency.hpp"

void testConcurrency()
{
    // ============================================================================
    // Concurrency
    // ============================================================================
    //
    // SpinLock is a basic spinlock mutex.  Fence is a completion counter.
    // callPar pushes work to a thread pool.  forPar iterates in parallel over
    // a range.  helpThreads processes work items while waiting on a fence.

    // ------------------------------------------------------------------
    // SpinLock — single-threaded basics
    // ------------------------------------------------------------------

    // Acquire/release, tryAcquire success/failure
    {
        SpinLock lock{};
        lock.acquire();
        lock.release();

        bool ok = lock.tryAcquire();
        TEST(ok);
        lock.release();

        lock.acquire();
        ok = lock.tryAcquire();
        TEST(!ok);
        lock.release();
    }

    // SpinLock: repeated acquire/release
    {
        SpinLock lock{};
        for (u32 i = 0; i < 100; ++i)
        {
            lock.acquire();
            lock.release();
        }
    }

    // SpinLock: tryAcquire/release cycle
    {
        SpinLock lock{};
        for (u32 i = 0; i < 100; ++i)
        {
            bool ok = lock.tryAcquire();
            TEST(ok);
            lock.release();
        }
    }

    // ------------------------------------------------------------------
    // Fence — basics
    // ------------------------------------------------------------------

    // Add/signal/isComplete/wait
    {
        Fence fence{};
        TEST(fence.isComplete());

        fence.add();
        TEST(!fence.isComplete());
        fence.signal();
        TEST(fence.isComplete());

        fence.add(5);
        TEST(!fence.isComplete());
        fence.signal(3);
        TEST(!fence.isComplete());
        fence.signal(2);
        TEST(fence.isComplete());

        bool ok = fence.wait(1.0);
        TEST(ok);

        fence.add();
        ok = fence.wait(0.0001);
        TEST(!ok);
        fence.signal();
    }

    // waitIndefinite
    {
        Fence fence{};
        fence.add();
        std::thread t{[&fence] { fence.signal(); }};
        fence.waitIndefinite();
        TEST(fence.isComplete());
        t.join();
    }

    // Fence: add(0) and signal(0) are no-ops
    {
        Fence fence{};
        fence.add(0);
        TEST(fence.isComplete());
        fence.add();
        fence.signal(0);
        TEST(!fence.isComplete());
        fence.signal();
        TEST(fence.isComplete());
    }

    // ------------------------------------------------------------------
    // SpinLockScope
    // ------------------------------------------------------------------

    // Basic RAII
    {
        SpinLock lock{};
        {
            SpinLockScope scope{&lock};
        }
        lock.acquire();
        lock.release();
    }

    // Null lock is safe
    {
        SpinLockScope scope{};
        TEST(scope.lock == nullptr);
    }

    // Move construct transfers ownership
    {
        SpinLock lock{};
        SpinLockScope inner{&lock};
        SpinLockScope moved{std::move(inner)};
        TEST(inner.lock == nullptr);
        TEST(moved.lock == &lock);
        // moved releases on destruction
    }

    // Move assign transfers ownership
    {
        SpinLock lock{};
        SpinLockScope a{&lock};
        SpinLockScope b{};
        b = std::move(a);
        TEST(a.lock == nullptr);
        TEST(b.lock == &lock);
        // b releases on destruction
    }

    // ------------------------------------------------------------------
    // callPar
    // ------------------------------------------------------------------

    // Single, multiple, null fence, many items
    {
        Fence fence{};
        bool executed = false;
        callPar(&fence, &executed, [](void* data)
        {
            *static_cast<bool*>(data) = true;
        });
        bool ok = fence.wait(2.0);
        TEST(ok);
        TEST(executed);

        bool a = false, b = false, c = false;
        Fence fence2{};
        callPar(&fence2, &a, [](void* p) { *static_cast<bool*>(p) = true; });
        callPar(&fence2, &b, [](void* p) { *static_cast<bool*>(p) = true; });
        callPar(&fence2, &c, [](void* p) { *static_cast<bool*>(p) = true; });
        ok = fence2.wait(2.0);
        TEST(ok);
        TEST(a && b && c);

        // null fence (fire-and-forget)
        callPar(nullptr, nullptr, [](void*) {});
        Fence fence3{};
        executed = false;
        callPar(&fence3, &executed, [](void* data)
        {
            *static_cast<bool*>(data) = true;
        });
        fence3.wait(2.0);
        TEST(executed);

        // Many sequential work items
        Fence fence4{};
        static constexpr u32 count = 100;
        bool vals[count]{};
        for (u32 i = 0; i < count; ++i)
        {
            callPar(&fence4, &vals[i], [](void* p)
            {
                *static_cast<bool*>(p) = true;
            });
        }
        ok = fence4.wait(2.0);
        TEST(ok);
        for (u32 i = 0; i < count; ++i)
            TEST(vals[i]);
    }

    // callPar: modify through pointer
    {
        Fence fence{};
        u32 result = 0;
        callPar(&fence, &result, [](void* p)
        {
            *static_cast<u32*>(p) = 42;
        });
        fence.wait(2.0);
        TEST(result == 42);
    }

    // ------------------------------------------------------------------
    // helpThreads
    // ------------------------------------------------------------------

    // Single and many items
    {
        Fence fence{};
        bool executed = false;
        callPar(&fence, &executed, [](void* data)
        {
            *static_cast<bool*>(data) = true;
        });
        bool ok = helpThreads(&fence, 2.0);
        TEST(ok);
        TEST(executed);
    }

    {
        Fence fence{};
        static constexpr u32 count = 100;
        bool vals[count]{};
        for (u32 i = 0; i < count; ++i)
        {
            callPar(&fence, &vals[i], [](void* p)
            {
                *static_cast<bool*>(p) = true;
            });
        }
        bool ok = helpThreads(&fence, 2.0);
        TEST(ok);
        for (u32 i = 0; i < count; ++i)
            TEST(vals[i]);
    }

    // ------------------------------------------------------------------
    // forPar — C callback and template lambda
    // ------------------------------------------------------------------

    {
        static constexpr u64 count = 100;
        bool vals[count]{};
        forPar(u64{0}, u64{count}, vals, [](void* data, u64 idx)
        {
            static_cast<bool*>(data)[idx] = true;
        });
        for (u64 i = 0; i < count; ++i)
            TEST(vals[i]);

        // contiguous array modification
        u32 ints[count]{};
        forPar(u64{0}, u64{count}, ints, [](void* data, u64 idx)
        {
            static_cast<u32*>(data)[idx] = static_cast<u32>(idx + 1);
        });
        for (u64 i = 0; i < count; ++i)
            TEST(ints[i] == i + 1);
    }

    // Large range
    {
        static constexpr u64 count = 1000;
        std::atomic<u32> sum{0};
        forPar(u64{0}, u64{count}, [&](u64)
        {
            sum.fetch_add(1);
        });
        TEST(sum.load() == count);
    }

    // forPar lambda with reference capture
    {
        static constexpr u64 count = 500;
        u32 vals[count]{};
        forPar(u64{0}, u64{count}, [&](u64 idx)
        {
            vals[idx] = static_cast<u32>(idx * 2);
        });
        for (u64 i = 0; i < count; ++i)
            TEST(vals[i] == i * 2);
    }

    // ------------------------------------------------------------------
    // Multi-threaded stress tests  (run 3× to flush out races)
    // ------------------------------------------------------------------

    for (u32 concurrencyIter = 0; concurrencyIter < 3; ++concurrencyIter)
    {

    // Concurrent producers: 2 threads
    {
        Fence fence{};
        static constexpr u32 count = 50;
        bool vals[count]{};

        std::thread t1{[&]
        {
            for (u32 i = 0; i < count / 2; ++i)
                callPar(&fence, &vals[i], [](void* p)
                {
                    *static_cast<bool*>(p) = true;
                });
        }};
        std::thread t2{[&]
        {
            for (u32 i = count / 2; i < count; ++i)
                callPar(&fence, &vals[i], [](void* p)
                {
                    *static_cast<bool*>(p) = true;
                });
        }};
        t1.join();
        t2.join();
        bool ok = helpThreads(&fence, 2.0);
        TEST(ok);
        for (u32 i = 0; i < count; ++i)
            TEST(vals[i]);
    }

    // Four threads producing work
    {
        Fence fence{};
        static constexpr u32 count = 200;
        bool vals[count]{};
        static constexpr u32 threadCount = 4;

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
        {
            threads[t] = std::thread{[&, t]
            {
                u32 begin = (count / threadCount) * t;
                u32 end = (t == threadCount - 1) ? count : begin + count / threadCount;
                for (u32 i = begin; i < end; ++i)
                    callPar(&fence, &vals[i], [](void* p)
                    {
                        *static_cast<bool*>(p) = true;
                    });
            }};
        }
        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();
        bool ok = helpThreads(&fence, 2.0);
        TEST(ok);
        for (u32 i = 0; i < count; ++i)
            TEST(vals[i]);
    }

    // Concurrent fence add/signal from 8 threads
    {
        Fence fence{};
        static constexpr u32 threadCount = 8;
        static constexpr u32 signalsPerThread = 100;
        fence.add(threadCount * signalsPerThread);

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
        {
            threads[t] = std::thread{[&fence]
            {
                for (u32 i = 0; i < signalsPerThread; ++i)
                    fence.signal();
            }};
        }
        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();
        TEST(fence.isComplete());
    }

    // SpinLock multi-threaded mutual exclusion
    {
        SpinLock lock{};
        u32 shared = 0;
        static constexpr u32 threadCount = 4;
        static constexpr u32 incrementsPerThread = 5000;

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
            threads[t] = std::thread{[&]
            {
                for (u32 i = 0; i < incrementsPerThread; ++i)
                {
                    lock.acquire();
                    ++shared;
                    lock.release();
                }
            }};
        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();
        TEST(shared == threadCount * incrementsPerThread);
    }

    // SpinLockScope multi-threaded with RAII guard
    {
        SpinLock lock{};
        u32 shared = 0;
        static constexpr u32 threadCount = 4;
        static constexpr u32 incrementsPerThread = 5000;

        std::thread threads[threadCount];
        for (u32 t = 0; t < threadCount; ++t)
            threads[t] = std::thread{[&]
            {
                for (u32 i = 0; i < incrementsPerThread; ++i)
                {
                    SpinLockScope scope{&lock};
                    ++shared;
                }
            }};
        for (u32 t = 0; t < threadCount; ++t)
            threads[t].join();
        TEST(shared == threadCount * incrementsPerThread);
    }

    } // concurrencyIter
}

