#pragma once

#include "hg_utils.h"

namespace hg {

inline constexpr usize align_size(const usize size, const usize alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}
inline std::byte* align_ptr(void* ptr, const usize alignment) {
    return reinterpret_cast<std::byte*>(align_size(reinterpret_cast<usize>(ptr), alignment));
}
template <typename T> T* align_ptr(T* ptr) requires (!std::same_as<T, void>) {
    return reinterpret_cast<T*>(align_size(reinterpret_cast<usize>(ptr), alignof(T)));
}

struct Terminate { explicit constexpr Terminate() = default; };
struct ReturnNull { explicit constexpr ReturnNull() = default; };

template <typename T> concept FailurePolicyTag = (
    std::same_as<T, Terminate> ||
    std::same_as<T, ReturnNull>
);

template <FailurePolicyTag FailurePolicy = Terminate>
class CAllocator : public Allocator {
public:
    static CAllocator& instance() {
        static CAllocator instance{};
        return instance;
    }

    void* alloc_v(usize size, usize alignment) override {
        ASSERT(size > 0);
        ASSERT(alignment > 0);

        auto ptr = std::malloc(align_size(size, alignment));
        if constexpr (std::same_as<FailurePolicy, Terminate>)
            if (ptr == nullptr)
                LOG_ERROR("Malloc returned null");
        return ptr;
    }
    void* realloc_v(void* original, usize original_size, usize new_size, usize alignment) override {
        ASSERT(original != nullptr);
        ASSERT(original_size > 0);
        ASSERT(new_size > 0);
        ASSERT(alignment > 0);

        auto ptr = std::realloc(original, align_size(new_size, alignment));
        if constexpr (std::same_as<FailurePolicy, Terminate>)
            if (ptr == nullptr)
                LOG_ERROR("Realloc returned null");
        return ptr;
    }
    void dealloc_v(void* ptr, usize size, usize alignment) override {
        ASSERT(ptr != nullptr);
        ASSERT(size > 0);
        ASSERT(alignment > 0);

        std::free(ptr);
    }

    template <typename T> [[nodiscard]] static T* alloc(const usize count) {
        return static_cast<T*>(CAllocator{}.alloc_v(count * sizeof(T), alignof(T)));
    }
    template <typename T> static T* realloc(T* original_ptr, const usize original_count, const usize new_count) {
        return static_cast<T*>(CAllocator{}.realloc_v(
            original_ptr, original_count * sizeof(T), new_count * sizeof(T), alignof(T)
        ));
    }
    template <typename T> static void dealloc(T* ptr, const usize) {
        CAllocator{}.dealloc_v(ptr, sizeof(T) * ptr, alignof(T));
    }
};

inline CAllocator<>& mallocator() { return CAllocator<>::instance(); }

template <FailurePolicyTag FailurePolicy = Terminate>
class LinearAllocator : public Allocator {
public:
    LinearAllocator() = default;
    LinearAllocator(std::byte* memory, usize size) : m_memory{memory, size}, m_head{memory} {}

    static LinearAllocator create(Allocator& parent, const usize size) {
        auto memory = parent.alloc<std::byte>(size);
        return {memory, size};
    }
    void destroy(Allocator& parent) const {
        parent.dealloc(m_memory.data(), m_memory.size());
    }

    [[nodiscard]] void* alloc_v(const usize size, const usize alignment) override {
        ASSERT(size > 0);
        ASSERT(alignment > 0);

        std::byte* alloc_ptr = align_ptr(m_head, alignment);
        std::byte* alloc_end = alloc_ptr + align_size(size, alignment);
        if (alloc_end > m_memory.data() + m_memory.size()) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                LOG_ERROR("Linear allocator out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_WARN("Linear allocator out of memory");
                return nullptr;
            }
        }
        m_head = alloc_end;

        return alloc_ptr;
    }

    void* realloc_v(
        void* original_ptr, const usize original_size, const usize new_size, const usize alignment
    ) override {
        ASSERT(original_ptr != nullptr);
        ASSERT(original_size > 0);
        ASSERT(new_size > 0);
        ASSERT(alignment > 0);

        std::byte* original = static_cast<std::byte*>(original_ptr);

        if (align_ptr(original + original_size, alignment) != m_head) {
            void* alloc_ptr = alloc_v(new_size, alignment);
            std::memmove(alloc_ptr, original, original_size);
            return alloc_ptr;
        }

        std::byte* new_end = align_ptr(original + new_size, alignment);
        if (new_end > m_memory.data() + m_memory.size())
            LOG_ERROR("Linear allocator out of memory");

        m_head = new_end;
        return original_ptr;
    }

    void dealloc_v(void*, usize, usize) override {}

    void reset() { m_head = m_memory.data(); }

private:
    std::span<std::byte> m_memory = {};
    std::byte* m_head = nullptr;
};

template <FailurePolicyTag FailurePolicy = Terminate>
class StackAllocator : public Allocator {
public:
    StackAllocator() = default;
    StackAllocator(std::byte* memory, usize size)
        : m_memory{memory, size}
        , m_head{align_ptr(memory, 16)}
    {}

    static StackAllocator create(Allocator& parent, const usize size) {
        auto memory = parent.alloc<std::byte>(size);
        return {memory, size};
    }
    void destroy(Allocator& parent) const {
        parent.dealloc(m_memory.data(), m_memory.size());
    }

    [[nodiscard]] void* alloc_v(const usize size, const usize) override {
        ASSERT(size > 0);

        std::byte* alloc_ptr = m_head;
        std::byte* alloc_end = alloc_ptr + align_size(size, 16);
        if (alloc_end > m_memory.data() + m_memory.size()) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                LOG_ERROR("Stack allocator out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_WARN("Stack allocator out of memory");
                return nullptr;
            }
        }
        m_head = align_ptr(alloc_end, 16);

        return alloc_ptr;
    }

    [[nodiscard]] void* realloc_v(
        void* original_ptr, const usize original_size, const usize new_size, const usize
    ) override {
        ASSERT(original_ptr != nullptr);
        ASSERT(original_size > 0);
        ASSERT(new_size > 0);

        std::byte* original = static_cast<std::byte*>(original_ptr);

        if (align_ptr(original + original_size, 16) != m_head) {
            void* alloc_ptr = alloc_v(new_size, 16);
            std::memmove(alloc_ptr, original_ptr, align_size(original_size, 16));
            return alloc_ptr;
        }

        std::byte* new_end = align_ptr(original + new_size, 16);
        if (new_end > m_memory.data() + m_memory.size())
            LOG_ERROR("Stack allocator out of memory");

        m_head = new_end;
        return original_ptr;
    }

    void dealloc_v(void* ptr, const usize size, const usize) override {
        ASSERT(ptr != nullptr);
        ASSERT(size > 0);

        std::byte* ptr_byte = static_cast<std::byte*>(ptr);

        if (align_ptr(ptr_byte + size, 16) != m_head) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                LOG_ERROR("Deallocation of invalid pointer from stack allocator");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_WARN("Deallocation of invalid pointer from stack allocator");
                return;
            }
        }

        m_head = ptr_byte;
    }

    void reset() { m_head = m_memory.data(); }

private:
    std::span<std::byte> m_memory = {};
    std::byte* m_head = nullptr;
};

template <usize Size, FailurePolicyTag FailurePolicy = Terminate>
class FixedSizeAllocator : public Allocator {
private:
    union Slot {
        std::byte data[align_size(Size, 16)];
        usize next;
    };

public:
    FixedSizeAllocator() = default;
    FixedSizeAllocator(Slot* memory, usize count) : m_slots{memory, count} {
        for (usize i = 0; i < count; ++i) {
            m_slots[i].next = i + 1;
        }
    }

    static FixedSizeAllocator create(Allocator& parent, const usize count) {
        auto memory = parent.alloc<Slot>(count);
        return {memory, count};
    }
    void destroy(Allocator& parent) const {
        check_leaks();
        parent.dealloc(m_slots.data(), m_slots.size());
    }

    [[nodiscard]] void* alloc_v(const usize size, const usize) override {
        ASSERT(align_size(size, 16) <= Size);

        usize index = m_next;
        if (index >= m_slots.size()) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                LOG_ERROR("Resource pool out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_WARN("Resource pool out of memory");
                return nullptr;
            }
        }
        m_next = m_slots[index].next;
        return &m_slots[index].data;
    }

    [[nodiscard]] void* realloc_v(void* original_ptr, const usize, const usize new_size, const usize) override {
        ASSERT(original_ptr != nullptr);
        ASSERT(align_size(new_size, 16) <= Size);
        return original_ptr;
    }

    void dealloc_v(void* ptr, const usize, const usize) override {
        ASSERT(ptr != nullptr);

        Slot* slot = reinterpret_cast<Slot*>(ptr);
        const usize index = static_cast<usize>(slot - m_slots.data());
        slot->next = m_next;
        m_next = index;
    }

private:
    void check_leaks() const {
#ifndef NDEBUG
        if (m_slots.data() == nullptr)
            return;

        usize count = 0;
        usize index = m_next;
        while (index != m_slots.size() && count <= m_slots.size()) {
            index = m_slots[index].next;
            ++count;
        }
        if (count < m_slots.size())
            LOG_ERROR("Fixed size allocator leaked memory");
        if (count > m_slots.size())
            LOG_ERROR("Fixed size allocator had double frees");
#endif
    }

    std::span<Slot> m_slots{};
    usize m_next = 0;
};

template <typename T, FailurePolicyTag FailurePolicy = Terminate>
class PoolAllocator {
private:
    union Slot {
        std::byte data[align_size(sizeof(T), alignof(T))];
        usize next;
    };

public:
    PoolAllocator() = default;
    PoolAllocator(Slot* memory, usize count) : m_slots{memory, count} {
        for (usize i = 0; i < count; ++i) {
            m_slots[i].next = i + 1;
        }
    }

    static PoolAllocator create(Allocator& parent, const usize count) {
        auto memory = parent.alloc<Slot>(count);
        return {memory, count};
    }
    void destroy(Allocator& parent) const {
        check_leaks();
        parent.dealloc(m_slots.data(), m_slots.size());
    }

    [[nodiscard]] T* alloc(const usize count = 1) {
        ASSERT(count > 0);

        usize index = m_next;
        if (index >= m_slots.size()) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                LOG_ERROR("Resource pool out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_WARN("Resource pool out of memory");
                return nullptr;
            }
        }
        m_next = m_slots[index].next;
        return &m_slots[index].data;
    }

    void dealloc(T* ptr) {
        ASSERT(ptr != nullptr);

        Slot* slot = reinterpret_cast<Slot*>(ptr);
        const usize index = static_cast<usize>(slot - m_slots.data());
        slot->next = m_next;
        m_next = index;
    }

private:
    void check_leaks() const {
#ifndef NDEBUG
        if (m_slots.data() == nullptr)
            return;

        usize count = 0;
        usize index = m_next;
        while (index != m_slots.size() && count <= m_slots.size()) {
            index = m_slots[index].next;
            ++count;
        }
        if (count < m_slots.size())
            LOG_ERROR("Pool allocator leaked memory");
        if (count > m_slots.size())
            LOG_ERROR("Pool allocator had double frees");
#endif
    }

    std::span<Slot> m_slots{};
    usize m_next = 0;
};

} // namespace hg
