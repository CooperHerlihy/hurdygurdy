#include "hurdygurdy.hpp"

#include <thread>

void hg_init(void) {
    hg_init_scratch();

    HgArena& arena = hg_get_scratch();

    u32 thread_count = std::thread::hardware_concurrency()
        - 2; // main thread, io thread
    hg_thread_pool_init(arena, 4096, thread_count);
    hg_io_thread_init(arena, 4096);
    hg_resources_init(arena, 4096);
    hg_gpu_resources_init(arena, 4096);

    if (hg_ecs == nullptr) {
        hg_ecs = arena.alloc<HgECS>(1);
        *hg_ecs = hg_ecs->create(arena, 4096);
    }

    hg_graphics_init();

    hg_platform_init();
}

void hg_exit(void) {
    hg_platform_deinit();

    hg_graphics_deinit();

    if (hg_ecs != nullptr) {
        hg_ecs = nullptr;
    }

    hg_gpu_resources_reset();
    hg_resources_reset();
    hg_io_thread_deinit();
    hg_thread_pool_deinit();
    hg_deinit_scratch();
}

