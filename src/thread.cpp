#include "hurdygurdy.hpp"

#include <emmintrin.h>

void HgFence::add() {
    ++counter;
}

void HgFence::add(usize count) {
    counter.fetch_add(count);
}

void HgFence::signal() {
    --counter;
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

HgThreadPool* HgThreadPool::create(HgArena& arena, usize thread_count, usize queue_size) {
    hg_assert(queue_size > 1 && (queue_size & (queue_size - 1)) == 0);

    static constexpr usize spin_count = 128;

    auto thread_fn = [](HgThreadPool* pool) {
        for (;;) {
            std::unique_lock<std::mutex> lock{pool->mtx};
            while (pool->count.load() == 0 && !pool->should_close.load()) {
                pool->cv.wait(lock);
            }
            lock.unlock();
            if (pool->should_close.load())
                return;

            for (usize i = 0; i < spin_count; ++i) {
                if (!pool->pop())
                    _mm_pause();
            }
        }
    };

    HgThreadPool *pool = new (arena.alloc<HgThreadPool>(1)) HgThreadPool{};

    pool->should_close.store(false);
    pool->thread_count = std::min((usize)1, thread_count - 1);
    pool->threads = arena.alloc<std::thread>(pool->thread_count);

    pool->has_item = arena.alloc<std::atomic_bool>(queue_size);
    pool->items = arena.alloc<Work>(queue_size);
    pool->capacity = queue_size;

    pool->reset();

    for (usize i = 0; i < pool->thread_count; ++i) {
        new (pool->threads + i) std::thread(thread_fn, pool);
    }

    return pool;
}

void HgThreadPool::destroy() {
    mtx.lock();
    should_close = true;
    mtx.unlock();
    cv.notify_all();

    for (usize i = 0; i < thread_count; ++i) {
        threads[i].join();
    }
    for (usize i = 0; i < thread_count; ++i) {
        threads[i].~thread();
    }
    cv.~condition_variable();
    mtx.~mutex();
}

void HgThreadPool::reset() {
    count.store(0);
    tail.store(0);
    working_tail.store(0);
    head.store(0);
    working_head.store(0);
    for (usize i = 0; i < capacity; ++i) {
        has_item[i].store(false);
    }
}

void HgThreadPool::push(HgFence* fences, usize fence_count, void* data, void (*fn)(void*)) {
    hg_assert(fn != nullptr);
    for (usize i = 0; i < fence_count; ++i) {
        fences[i].add();
    }

    usize idx = working_head.fetch_add(1) & (capacity - 1);

    items[idx].fences = fences;
    items[idx].fence_count = fence_count;
    items[idx].data = data;
    items[idx].fn = fn;
    has_item[idx].store(true);

    usize h = head.load();
    while (has_item[h].load()) {
        usize next = (h + 1) & (capacity - 1);
        head.compare_exchange_strong(h, next);
        h = next;
    }

    ++count;
    mtx.lock();
    mtx.unlock();
    cv.notify_one();
}

bool HgThreadPool::pop() {
    usize idx = working_tail.load();
    do {
        if (idx == head.load())
            return false;
    } while (!working_tail.compare_exchange_weak(idx, (idx + 1) & (capacity - 1)));

    Work work = items[idx];
    has_item[idx].store(false);

    usize t = tail.load();
    while (t != head.load() && !has_item[t].load()) {
        usize next = (t + 1) & (capacity - 1);
        tail.compare_exchange_strong(t, next);
        t = next;
    }

    --count;

    hg_assert(work.fn != nullptr);
    work.fn(work.data);

    for (usize i = 0; i < work.fence_count; ++i) {
        work.fences[i].signal();
    }
    return true;
}

bool HgThreadPool::help(HgFence& fence, f64 timeout_seconds) {
    auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64>(timeout_seconds);
    while (!fence.is_complete() && std::chrono::steady_clock::now() < end) {
        if (!pop())
            _mm_pause();
    }
    return fence.is_complete();
}

HgIOThread* HgIOThread::create(HgArena& arena, usize queue_size) {
    hg_assert(queue_size > 1 && (queue_size & (queue_size - 1)) == 0);

    auto thread_fn = [](HgIOThread* io) {
        hg_init_scratch();

        for (;;) {
            if (io->should_close.load())
                return;

            Request request;
            if (!io->pop(request))
                hg_threads->pop();
        }

        hg_deinit_scratch();
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
        request.fences[i].add();
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
        request.fences[i].signal();
    }
    return true;
}

