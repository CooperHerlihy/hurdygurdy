#include "hurdygurdy.hpp"

static constexpr usize hg_internal_arena_count = 2;
static thread_local HgArena hg_internal_arenas[hg_internal_arena_count]{};

void hg_init_scratch() {
    for (usize i = 0; i < hg_internal_arena_count; ++i) {
        if (hg_internal_arenas[i].memory == nullptr) {
            usize arena_size = (u32)-1;
            hg_internal_arenas[i] = {malloc(arena_size), arena_size};
        }
    }
}

void hg_deinit_scratch() {
    for (usize i = 0; i < hg_internal_arena_count; ++i) {
        if (hg_internal_arenas[i].memory != nullptr) {
            std::free(hg_internal_arenas[i].memory);
            hg_internal_arenas[i] = {};
        }
    }
}

void* HgArena::alloc(usize size, usize alignment) {
    head = hg_align((usize)head, alignment) + size;
    hg_assert(head <= capacity);
    return (void*)((uptr)memory + head - size);
}

void* HgArena::realloc(void* allocation, usize old_size, usize new_size, usize alignment) {
    if (allocation >= memory && (uptr)allocation + old_size <= (uptr)memory + capacity) {
        if ((uptr)allocation + old_size - (uptr)memory == (uptr)head) {
            head = (uptr)allocation + new_size - (uptr)memory;
            hg_assert(head <= capacity);
            return allocation;
        }

        if (new_size < old_size)
            return allocation;
    }

    void* new_allocation = alloc(new_size, alignment);
    if (allocation != nullptr)
        std::memcpy(new_allocation, allocation, std::min(old_size, new_size));
    return new_allocation;
}

HgArena& hg_get_scratch() {
    return hg_internal_arenas[0];
}

HgArena& hg_get_scratch(const HgArena& conflict) {
    for (HgArena& arena : hg_internal_arenas) {
        if (&arena != &conflict)
            return arena;
    }
    hg_error("No scratch arena available\n");
}

HgArena& hg_get_scratch(const HgArena** conflicts, usize count) {
    for (HgArena& arena : hg_internal_arenas) {
        for (usize i = 0; i < count; ++i) {
            if (&arena == conflicts[i])
                goto next;
        }
        return arena;
next:
        continue;
    }
    hg_error("No scratch arena available\n");
}

