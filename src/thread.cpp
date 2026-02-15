#include "hurdygurdy.hpp"

#include <condition_variable>
#include <mutex>

#include <emmintrin.h>

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

std::thread* pool_threads;
usize pool_thread_count;
std::atomic_bool pool_should_close;

std::mutex pool_mtx;
std::condition_variable pool_cv;

struct ThreadWork {
    HgFence* fences;
    usize fence_count;
    void* data;
    void (*fn)(void*);
};
ThreadWork* pool_work;
std::atomic_bool* pool_has_work;
usize pool_work_capacity;

std::atomic<usize> pool_work_count;
std::atomic<usize> pool_tail;
std::atomic<usize> pool_working_tail;
std::atomic<usize> pool_head;
std::atomic<usize> pool_working_head;

static bool thread_pool_pop() {
    return false;
}

void hg_thread_pool_init(HgArena& arena, usize thread_count, usize queue_size) {
    hg_assert(thread_count > 1 && (queue_size & (queue_size - 1)) == 0);

    pool_should_close.store(false);
    pool_thread_count = std::min((usize)1, thread_count - 1);
    pool_threads = arena.alloc<std::thread>(pool_thread_count);

    pool_work = arena.alloc<ThreadWork>(queue_size);
    pool_has_work = arena.alloc<std::atomic_bool>(queue_size);
    pool_work_capacity = queue_size;

    hg_thread_pool_reset();

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
                    if (!thread_pool_pop())
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

void hg_thread_pool_reset() {
    pool_work_count.store(0);
    pool_tail.store(0);
    pool_working_tail.store(0);
    pool_head.store(0);
    pool_working_head.store(0);
    for (usize i = 0; i < pool_work_capacity; ++i) {
        pool_has_work[i].store(false);
    }
}

bool hg_thread_pool_pop() {
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

bool hg_thread_pool_help(HgFence& fence, f64 timeout_seconds) {
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout_seconds);
    while (!fence.is_complete() && std::chrono::steady_clock::now() < end) {
        if (!hg_thread_pool_pop())
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

HgIOThread* HgIOThread::create(HgArena& arena, usize queue_size) {
    hg_assert(queue_size > 1 && (queue_size & (queue_size - 1)) == 0);

    auto thread_fn = [](HgIOThread* io) {
        hg_init_scratch();
        hg_defer(hg_deinit_scratch());

        for (;;) {
            if (io->should_close.load())
                return;

            Request request;
            if (!io->pop(request))
                hg_thread_pool_pop();
        }
    };

    HgIOThread* io = arena.alloc<HgIOThread>(1);

    io->requests = arena.alloc<Request>(queue_size);
    io->capacity = queue_size;

    io->has_item = arena.alloc<std::atomic_bool>(queue_size);

    io->reset();

    io->should_close.store(false);
    new (&io->thread) std::thread(thread_fn, io);

    return io;
}

void HgIOThread::destroy() {
    should_close = true;
    thread.join();
    thread.~thread();
}

void HgIOThread::reset() {
    tail.store(0);
    head.store(0);
    working_head.store(0);
    for (usize i = 0; i < capacity; ++i) {
        has_item[i].store(false);
    }
}

void HgIOThread::push(const Request& request) {
    hg_assert(request.fn != nullptr);
    for (usize i = 0; i < request.fence_count; ++i) {
        request.fences[i].add(1);
    }

    usize idx = working_head.fetch_add(1) & (capacity - 1);

    requests[idx] = request;
    has_item[idx].store(true);

    usize h = head.load();
    while (has_item[h].load()) {
        usize next = (h + 1) & (capacity - 1);
        head.compare_exchange_strong(h, next);
        h = next;
    }
}

bool HgIOThread::pop(Request& request) {
    usize idx = tail.load() & (capacity - 1);
    if (idx == head.load())
        return false;

    request = requests[idx];
    has_item[idx].store(false);

    tail.fetch_add(1);

    hg_assert(request.fn != nullptr);
    request.fn(request.data, request.resource, request.path);

    for (usize i = 0; i < request.fence_count; ++i) {
        request.fences[i].signal(1);
    }
    return true;
}

