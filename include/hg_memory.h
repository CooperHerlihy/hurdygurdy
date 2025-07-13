#pragma once

#include "hg_utils.h"

namespace hg {

using byte = std::byte;

inline constexpr usize align_up(const usize size, const usize alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}
inline byte* align_ptr_up(void* ptr, const usize alignment) {
    return reinterpret_cast<byte*>(align_up(reinterpret_cast<usize>(ptr), alignment));
}
template <typename T> T* align_ptr_up(T* ptr) requires (!std::same_as<T, void>) {
    return reinterpret_cast<T*>(align_up(reinterpret_cast<usize>(ptr), alignof(T)));
}
template <typename T> T* align_ptr_up(void* ptr) requires (!std::same_as<T, void>) {
    return reinterpret_cast<T*>(align_up(reinterpret_cast<usize>(ptr), alignof(T)));
}

inline constexpr usize align_down(const usize size, const usize alignment) {
    return size & ~(alignment - 1);
}
inline byte* align_ptr_down(void* ptr, const usize alignment) {
    return reinterpret_cast<byte*>(align_down(reinterpret_cast<usize>(ptr), alignment));
}
template <typename T> T* align_ptr_down(T* ptr) requires (!std::same_as<T, void>) {
    return reinterpret_cast<T*>(align_down(reinterpret_cast<usize>(ptr), alignof(T)));
}
template <typename T> T* align_ptr_down(void* ptr) requires (!std::same_as<T, void>) {
    return reinterpret_cast<T*>(align_down(reinterpret_cast<usize>(ptr), alignof(T)));
}

template <typename T> struct Slice {
    T* data = nullptr;
    usize count = 0;

    T* begin() const { return data; }
    T* end() const { return data + count; }

    T& operator[](const usize index) const {
        ASSERT(index < count);
        return data[index];
    }
};

class Arena {
public:
    Arena() = default;
    Arena(Slice<byte> memory) : m_memory{memory}, m_head{0} {}
    Slice<byte> release() {
        Slice<byte> memory{m_memory};
        m_memory = Slice<byte>{};
        m_head = 0;
        return memory;
    }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&& other) noexcept
        : m_memory{other.m_memory}
        , m_head{other.m_head}
    {
        other.m_memory = Slice<byte>{};
        other.m_head = 0;
    }
    Arena& operator=(Arena&& other) noexcept {
        if (this == &other)
            return *this;
        if (m_memory.data != nullptr)
            ERROR("Occupied arena cannot be moved into");
        new (this) Arena(std::move(other));
        return *this;
    }

    Slice<byte> alloc(usize size) {
        ASSERT(size > 0);

        Slice<byte> allocation{m_memory.data + m_head, align_up(size, 16)};
        if (allocation.end() > m_memory.end())
            ERROR("Stack out of memory");
        m_head = allocation.end() - m_memory.data;

        return allocation;
    }

    template <typename T> [[nodiscard]] T* alloc() {
        return reinterpret_cast<T*>(alloc(sizeof(T)).data);
    }
    template <typename T> [[nodiscard]] Slice<T> alloc(usize count) {
        return {reinterpret_cast<T*>(alloc(count * sizeof(T)).data), count};
    }

    Slice<byte> realloc(Slice<byte> original, usize new_size) {
        ASSERT(original.data != nullptr);
        ASSERT(original.count > 0);
        ASSERT(new_size > 0);

        if (original.end() != m_memory.data + m_head)
            ERROR("Stack can only reallocate top allocation");

        Slice<byte> allocation{original.data, align_up(new_size, 16)};
        if (allocation.end() > m_memory.end())
            ERROR("Stack out of memory");
        m_head = allocation.end() - m_memory.data;

        return allocation;
    }

    template <typename T> [[nodiscard]] Slice<T> realloc(Slice<T> original, usize new_count) {
        return {reinterpret_cast<T*>(realloc(
            Slice<byte>{reinterpret_cast<byte*>(original.data), align_up(original.count * sizeof(T), 16)},
            new_count * sizeof(T)
        ).data), new_count};
    }

    void dealloc(Slice<byte> allocation) {
        ASSERT(allocation.data != nullptr);
        ASSERT(allocation.count > 0);

        if (allocation.end() != m_memory.data + m_head)
            ERROR("Stack can only deallocate top allocation");

        m_head = allocation.data - m_memory.data;
    }

    template <typename T> void dealloc(T* ptr) {
        dealloc({reinterpret_cast<byte*>(ptr), align_up(sizeof(T), 16)});
    }
    template <typename T> void dealloc(Slice<T> slice) {
        dealloc({reinterpret_cast<byte*>(slice.data), align_up(slice.count * sizeof(T), 16)});
    }

private:
    Slice<byte> m_memory{};
    usize m_head = 0;
};

class Stack {
public:
    struct Config {
        usize size = 0;
    };
    [[nodiscard]] static Stack create(const Config& config) {
        byte* memory = static_cast<byte*>(malloc(config.size));
        if (memory == nullptr)
            ERROR("Malloc out of memory");

        Stack stack{};
        stack.m_arena = Slice<byte>{memory, config.size};
        return stack;
    }
    void destroy() {
        free(m_arena.release().data);
    }

    [[nodiscard]] Slice<byte> alloc(usize size) {
        return m_arena.alloc(size);
    }
    [[nodiscard]] Slice<byte> realloc(Slice<byte> original, usize new_size) {
        return m_arena.realloc(original, new_size);
    }
    void dealloc(Slice<byte> allocation) {
        m_arena.dealloc(allocation);
    }

    template <typename T> [[nodiscard]] T* alloc() {
        return m_arena.alloc<T>();
    }
    template <typename T> [[nodiscard]] Slice<T> alloc(usize count) {
        return m_arena.alloc<T>(count);
    }

    template <typename T> void dealloc(T* ptr) {
        m_arena.dealloc<T>(ptr);
    }
    template <typename T> void dealloc(Slice<T> slice) {
        m_arena.dealloc<T>(slice);
    }

private:
    Arena m_arena{};
};

template <typename T> class Pool {
public:
    union Block {
        T data;
        usize next;
    };

    Pool() = default;
    Pool(Slice<Block> memory) : m_blocks{memory} {
        for (usize i = 0; i < m_blocks.count; ++i) {
            m_blocks[i].next = i + 1;
        }
    }
    Slice<Block> release() {
        Slice<Block> memory{m_blocks};
        m_blocks = Slice<Block>{};
        m_next = 0;
        return memory;
    }

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;
    Pool(Pool&& other) noexcept
        : m_blocks{other.m_blocks}
        , m_next{other.m_next}
    {
        other.m_blocks = Slice<Block>{};
        other.m_next = 0;
    }
    Pool& operator=(Pool&& other) noexcept {
        if (this == &other)
            return *this;
        if (m_blocks.data != nullptr)
            ERROR("Occupied pool cannot be moved into");
        new (this) Pool(std::move(other));
        return *this;
    }

    T* alloc() {
        ASSERT(m_blocks.count > 0);

        usize index = m_next;
        if (index >= m_blocks.count)
            ERROR("Pool out of memory");

        m_next = m_blocks[index].next;
        return &m_blocks[index].data;
    }

    void dealloc(T* ptr) {
        ASSERT(ptr != nullptr);

        Block* block = reinterpret_cast<Block*>(ptr);

        if (block < m_blocks.begin() || block >= m_blocks.end())
            ERROR("Pool cannot deallocate block outside of pool");
        //if (((block - m_blocks.data) & (15)) != 0)
            //ERROR("Pool cannot deallocate misaligned block");

        block->next = m_next;
        m_next = block - m_blocks.data;
    }

    void check_leaks() const {
#ifndef NDEBUG
        usize count = 0;
        usize index = m_next;
        while (index != m_blocks.count && count <= m_blocks.count) {
            index = m_blocks[index].next;
            ++count;
        }
        if (count < m_blocks.count)
            ERROR("Pool leaked memory");
        if (count > m_blocks.count)
            ERROR("Pool had double frees");
#endif
    }

private:
    Slice<Block> m_blocks{};
    usize m_next = 0;
};

template <usize SmallSize, usize LargeSize, usize HugeSize> class Heap {
public:
    using SmallType = std::array<byte, SmallSize>;
    using LargeType = std::array<byte, LargeSize>;
    using HugeType = std::array<byte, HugeSize>;

    struct Config {
        usize small_block_count = 0;
        usize large_block_count = 0;
        usize huge_block_count = 0;
    };
    [[nodiscard]] static Heap create(const Config& config) {
        using SmallBlock = Pool<SmallType>::Block;
        using LargeBlock = Pool<LargeType>::Block;
        using HugeBlock = Pool<HugeType>::Block;

        SmallBlock* small_blocks = static_cast<SmallBlock*>(malloc(sizeof(SmallBlock) * config.small_block_count));
        LargeBlock* large_blocks = static_cast<LargeBlock*>(malloc(sizeof(LargeBlock) * config.large_block_count));
        HugeBlock* huge_blocks = static_cast<HugeBlock*>(malloc(sizeof(HugeBlock) * config.huge_block_count));

        if (huge_blocks == nullptr || large_blocks == nullptr || small_blocks == nullptr) {
            free(huge_blocks);
            free(large_blocks);
            free(small_blocks);
            ERROR("Malloc out of memory");
        }

        Heap heap{};
        heap.m_huge_heap = Slice<HugeBlock>{huge_blocks, config.huge_block_count};
        heap.m_large_heap = Slice<LargeBlock>{large_blocks, config.large_block_count};
        heap.m_small_heap = Slice<SmallBlock>{small_blocks, config.small_block_count};
        return heap;
    }
    void destroy() {
        free(m_huge_heap.release().data);
        free(m_large_heap.release().data);
        free(m_small_heap.release().data);
    }

    [[nodiscard]] Slice<byte> alloc(usize size) {
        if (size <= SmallSize)
            return {m_small_heap.alloc()->data(), size};
        else if (size <= LargeSize)
            return {m_large_heap.alloc()->data(), size};
        else if (size <= HugeSize)
            return {m_huge_heap.alloc()->data(), size};
        else
            ERROR("Heap out of memory");
    }

    template <typename T> [[nodiscard]] T* alloc() {
        return reinterpret_cast<T*>(alloc(sizeof(T)).data);
    }
    template <typename T> [[nodiscard]] Slice<T> alloc(usize count) {
        return {reinterpret_cast<T*>(alloc(count * sizeof(T)).data), count};
    }

    void dealloc(Slice<byte> allocation) {
        if (allocation.count <= SmallSize)
            m_small_heap.dealloc(reinterpret_cast<SmallType*>(allocation.data));
        else if (allocation.count <= LargeSize)
            m_large_heap.dealloc(reinterpret_cast<LargeType*>(allocation.data));
        else if (allocation.count <= HugeSize)
            m_huge_heap.dealloc(reinterpret_cast<HugeType*>(allocation.data));
        else
            ERROR("Heap out of memory");
    }

    template <typename T> void dealloc(T* ptr) {
        dealloc({reinterpret_cast<byte*>(ptr), sizeof(T)});
    }
    template <typename T> void dealloc(Slice<T> slice) {
        dealloc({reinterpret_cast<byte*>(slice.data), slice.count * sizeof(T)});
    }

private:
    Pool<SmallType> m_small_heap{};
    Pool<LargeType> m_large_heap{};
    Pool<HugeType> m_huge_heap{};
};

struct Memory {
    using Heap = Heap<
        1024,
        1024 * 32,
        1024 * 1024
    >;

    Stack stack{};
    Heap heap{};

    struct Config {
        Stack::Config stack{
            .size = 1024 * 1024
        };
        Heap::Config heap{
            .small_block_count = 1024,
            .large_block_count = 256,
            .huge_block_count = 64,
        };
    };
    [[nodiscard]] static Memory create(const Config& config) {
        Memory memory{};
        memory.heap = Heap::create(config.heap);
        memory.stack = Stack::create(config.stack);
        return memory;
    }
    void destroy() {
        stack.destroy();
        heap.destroy();
    }
};

} // namespace hg
