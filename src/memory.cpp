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

HgDynamicArray HgDynamicArray::create(
    HgArena& arena,
    u32 width,
    u32 alignment,
    usize count,
    usize capacity
) {
    hg_assert(count <= capacity);

    HgDynamicArray arr;
    arr.items = arena.alloc(capacity * width, alignment);
    arr.width = width;
    arr.alignment = alignment;
    arr.capacity = capacity;
    arr.count = count;
    return arr;
}

void HgDynamicArray::reserve(HgArena& arena, usize new_capacity) {
    items = arena.realloc(items, capacity * width, new_capacity * width, alignment);
    capacity = new_capacity;
}

void HgDynamicArray::grow(HgArena& arena, f32 factor) {
    hg_assert(factor > 1.0f);
    hg_assert(capacity <= (usize)((f32)SIZE_MAX / factor));
    reserve(arena, capacity == 0 ? 1 : (usize)((f32)capacity * factor));
}

void* HgDynamicArray::insert(usize index) {
    hg_assert(index <= count);
    hg_assert(count < capacity);

    std::memmove(get(index + 1), get(index), (count++ - index) * width);
    return get(index);
}

void HgDynamicArray::remove(usize index) {
    hg_assert(index < count);

    std::memmove(get(index), get(index + 1), (count - index - 1) * width);
    --count;
}

void* HgDynamicArray::swap_insert(usize index) {
    hg_assert(index <= count);
    hg_assert(count < capacity);
    if (index == count)
        return push();

    std::memcpy(get(count++), get(index), width);
    return get(index);
}

void HgDynamicArray::swap_remove(usize index) {
    hg_assert(index < count);
    if (index == count - 1) {
        pop();
        return;
    }

    std::memcpy(get(index), get(count - 1), width);
    --count;
}

