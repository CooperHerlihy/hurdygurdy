#include "hurdygurdy.hpp"

HgAnyArray HgAnyArray::create(
    HgArena& arena,
    u32 width,
    u32 alignment,
    usize count,
    usize capacity
) {
    hg_assert(count <= capacity);

    HgAnyArray arr;
    arr.items = arena.alloc(capacity * width, alignment);
    arr.width = width;
    arr.alignment = alignment;
    arr.capacity = capacity;
    arr.count = count;
    return arr;
}

void HgAnyArray::reserve(HgArena& arena, usize new_capacity) {
    items = arena.realloc(items, capacity * width, new_capacity * width, alignment);
    capacity = new_capacity;
}

void HgAnyArray::grow(HgArena& arena, f32 factor) {
    hg_assert(factor > 1.0f);
    hg_assert(capacity <= (usize)((f32)SIZE_MAX / factor));
    reserve(arena, capacity == 0 ? 1 : (usize)((f32)capacity * factor));
}

void* HgAnyArray::insert(usize index) {
    hg_assert(index <= count);
    hg_assert(count < capacity);

    std::memmove(get(index + 1), get(index), (count++ - index) * width);
    return get(index);
}

void HgAnyArray::remove(usize index) {
    hg_assert(index < count);

    std::memmove(get(index), get(index + 1), (count - index - 1) * width);
    --count;
}

void* HgAnyArray::swap_insert(usize index) {
    hg_assert(index <= count);
    hg_assert(count < capacity);
    if (index == count)
        return push();

    std::memcpy(get(count++), get(index), width);
    return get(index);
}

void HgAnyArray::swap_remove(usize index) {
    hg_assert(index < count);
    if (index == count - 1) {
        pop();
        return;
    }

    std::memcpy(get(index), get(count - 1), width);
    --count;
}

