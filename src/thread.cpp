#include "hurdygurdy.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>

#include <emmintrin.h>

usize hg_hardware_concurrency() {
    return std::thread::hardware_concurrency();
}

void HgFence::add(usize count) {
    counter.fetch_add(count);
}

void HgFence::signal(usize count) {
    counter.fetch_sub(count);
}

bool HgFence::is_complete() {
    return counter.load() == 0;
}

bool HgFence::wait(f64 timeout_seconds) {
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout_seconds);
    while (!is_complete() && std::chrono::steady_clock::now() < end) {
        _mm_pause();
    }
    return is_complete();
}

std::thread* pool_threads = nullptr;
usize pool_thread_count = 0;
std::atomic_bool pool_should_close = false;

std::mutex pool_mtx{};
std::condition_variable pool_cv{};

struct ThreadWork {
    HgFence* fences;
    usize fence_count;
    void* data;
    void (*fn)(void*);
};
ThreadWork* pool_work = nullptr;
std::atomic_bool* pool_has_work = nullptr;
usize pool_work_capacity = 0;

std::atomic<usize> pool_work_count = 0;
std::atomic<usize> pool_tail = 0;
std::atomic<usize> pool_working_tail = 0;
std::atomic<usize> pool_head = 0;
std::atomic<usize> pool_working_head = 0;

static bool pool_execute() {
    usize idx = pool_working_tail.load();
    do {
        if (idx == pool_head.load())
            return false;
    } while (!pool_working_tail.compare_exchange_weak(idx, (idx + 1) & (pool_work_capacity - 1)));

    ThreadWork work = pool_work[idx];
    pool_has_work[idx].store(false);

    usize t = pool_tail.load();
    while (t != pool_head.load() && !pool_has_work[t].load()) {
        usize next = (t + 1) & (pool_work_capacity - 1);
        pool_tail.compare_exchange_strong(t, next);
        t = next;
    }

    --pool_work_count;

    hg_assert(work.fn != nullptr);
    work.fn(work.data);

    for (usize i = 0; i < work.fence_count; ++i) {
        work.fences[i].signal(1);
    }
    return true;
}

void hg_thread_pool_init(HgArena& arena, usize queue_size, usize thread_count) {
    hg_assert(thread_count > 1 && (queue_size & (queue_size - 1)) == 0);

    pool_should_close.store(false);
    pool_thread_count = std::min((usize)1, thread_count - 1);
    pool_threads = arena.alloc<std::thread>(pool_thread_count);

    pool_work = arena.alloc<ThreadWork>(queue_size);
    pool_has_work = arena.alloc<std::atomic_bool>(queue_size);
    pool_work_capacity = queue_size;

    pool_work_count.store(0);
    pool_tail.store(0);
    pool_working_tail.store(0);
    pool_head.store(0);
    pool_working_head.store(0);
    for (usize i = 0; i < pool_work_capacity; ++i) {
        pool_has_work[i].store(false);
    }

    for (usize i = 0; i < pool_thread_count; ++i) {
        new (pool_threads + i) std::thread([] {
            for (;;) {
                std::unique_lock<std::mutex> lock{pool_mtx};
                while (pool_work_count.load() == 0 && !pool_should_close.load()) {
                    pool_cv.wait(lock);
                }
                lock.unlock();
                if (pool_should_close.load())
                    return;

                static constexpr usize spin_count = 128;
                for (usize j = 0; j < spin_count; ++j) {
                    if (!pool_execute())
                        _mm_pause();
                }
            }
        });
    }
}

void hg_thread_pool_deinit() {
    pool_mtx.lock();
    pool_should_close = true;
    pool_mtx.unlock();
    pool_cv.notify_all();

    for (usize i = 0; i < pool_thread_count; ++i) {
        pool_threads[i].join();
    }
    for (usize i = 0; i < pool_thread_count; ++i) {
        pool_threads[i].~thread();
    }
    pool_cv.~condition_variable();
    pool_mtx.~mutex();
}

bool hg_thread_pool_help(HgFence& fence, f64 timeout_seconds) {
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout_seconds);
    while (!fence.is_complete() && std::chrono::steady_clock::now() < end) {
        if (!pool_execute())
            _mm_pause();
    }
    return fence.is_complete();
}

void hg_call_par(HgFence* fences, usize fence_count, void* data, void (*fn)(void*)) {
    hg_assert(fn != nullptr);
    for (usize i = 0; i < fence_count; ++i) {
        fences[i].add(1);
    }

    usize idx = pool_working_head.fetch_add(1) & (pool_work_capacity - 1);

    pool_work[idx].fences = fences;
    pool_work[idx].fence_count = fence_count;
    pool_work[idx].data = data;
    pool_work[idx].fn = fn;
    pool_has_work[idx].store(true);

    usize h = pool_head.load();
    while (pool_has_work[h].load()) {
        usize next = (h + 1) & (pool_work_capacity - 1);
        pool_head.compare_exchange_strong(h, next);
        h = next;
    }

    ++pool_work_count;
    pool_mtx.lock();
    pool_mtx.unlock();
    pool_cv.notify_one();
}

std::thread io_thread{};
std::atomic_bool io_thread_should_close = false;

struct Request {
    HgFence* fences;
    usize fence_count;
    void* resource;
    HgStringView path;
    void (*fn)(void* resource, HgStringView path);
};
Request* io_requests = nullptr;
std::atomic_bool* io_has_request = nullptr;
usize io_request_capacity = 0;

std::atomic<usize> io_tail = 0;
std::atomic<usize> io_head = 0;
std::atomic<usize> io_working_head = 0;

bool io_pop() {
    usize idx = io_tail.load() & (io_request_capacity - 1);
    if (idx == io_head.load())
        return false;

    Request request = io_requests[idx];
    io_has_request[idx].store(false);

    io_tail.fetch_add(1);

    hg_assert(request.fn != nullptr);
    request.fn(request.resource, request.path);

    for (usize i = 0; i < request.fence_count; ++i) {
        request.fences[i].signal(1);
    }
    return true;
}

void hg_io_thread_init(HgArena& arena, usize queue_size) {
    hg_assert(queue_size > 1 && (queue_size & (queue_size - 1)) == 0);

    io_requests = arena.alloc<Request>(queue_size);
    io_has_request = arena.alloc<std::atomic_bool>(queue_size);
    io_request_capacity = queue_size;

    io_tail.store(0);
    io_head.store(0);
    io_working_head.store(0);
    for (usize i = 0; i < io_request_capacity; ++i) {
        io_has_request[i].store(false);
    }

    io_thread_should_close.store(false);
    io_thread = std::thread([]() {
        hg_init_scratch();
        hg_defer(hg_deinit_scratch());

        for (;;) {
            if (io_thread_should_close.load())
                return;

            if (!io_pop())
                pool_execute();
        }
    });
}

void hg_io_thread_deinit() {
    io_thread_should_close = true;
    io_thread.join();
}

void hg_io_request(
    HgFence* fences,
    usize fence_count,
    void* resource,
    HgStringView path,
    void (*fn)(void* resource, HgStringView path)
) {
    hg_assert(fn != nullptr);
    for (usize i = 0; i < fence_count; ++i) {
        fences[i].add(1);
    }

    usize idx = io_working_head.fetch_add(1) & (io_request_capacity - 1);

    io_requests[idx].fences = fences;
    io_requests[idx].fence_count = fence_count;
    io_requests[idx].resource = resource;
    io_requests[idx].path = path;
    io_requests[idx].fn = fn;
    io_has_request[idx].store(true);

    usize h = io_head.load();
    while (io_has_request[h].load()) {
        usize next = (h + 1) & (io_request_capacity - 1);
        io_head.compare_exchange_strong(h, next);
        h = next;
    }
}

