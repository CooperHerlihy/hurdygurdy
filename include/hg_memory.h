#pragma once

#include "hg_utils.h"

namespace hg {

template <typename T> struct Slice {
    T* data;
    usize count;

    T& operator[](const usize index) const { return data[index]; }

    T* begin() const { return data; }
    T* end() const { return data + count; }
};

class Allocator {
public:
    virtual ~Allocator() = default;

    [[nodiscard]] virtual void* alloc_v(usize size, usize alignment) = 0;
    [[nodiscard]] virtual void* realloc_v(void* original, usize original_size, usize new_size, usize alignment) = 0;
    virtual void dealloc_v(void* ptr, usize size, usize alignment) = 0;

    template <typename T> [[nodiscard]] T* alloc() {
        return static_cast<T*>(alloc_v(sizeof(T), alignof(T)));
    }
    template <typename T> [[nodiscard]] Slice<T> alloc(usize count) {
        ASSERT(count > 0);
        return {static_cast<T*>(alloc_v(count * sizeof(T), alignof(T))), count};
    }
    template <typename T> [[nodiscard]] Slice<T> realloc(Slice<T> original, usize new_count) {
        return {static_cast<T*>(realloc_v(original.data, original.count * sizeof(T), new_count * sizeof(T), alignof(T))), new_count};
    }
    template <typename T> void dealloc(T* ptr) {
        dealloc_v(ptr, sizeof(T), alignof(T));
    }
    template <typename T> void dealloc(Slice<T> slice) {
        dealloc_v(slice.data, slice.count * sizeof(T), alignof(T));
    }
};

struct Terminate { explicit constexpr Terminate() = default; };
struct ReturnNull { explicit constexpr ReturnNull() = default; };

template <typename T> concept FailurePolicyTag = (
    std::same_as<T, Terminate> ||
    std::same_as<T, ReturnNull>
);

inline constexpr usize align_size(const usize size, const usize alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}
inline std::byte* align_ptr(void* ptr, const usize alignment) {
    return reinterpret_cast<std::byte*>(align_size(reinterpret_cast<usize>(ptr), alignment));
}
template <typename T> T* align_ptr(T* ptr) requires (!std::same_as<T, void>) {
    return reinterpret_cast<T*>(align_size(reinterpret_cast<usize>(ptr), alignof(T)));
}

template <FailurePolicyTag FailurePolicy = Terminate>
class CAllocator : public Allocator {
public:
    void* alloc_v(usize size, usize alignment) override {
        ASSERT(size > 0);
        ASSERT(alignment > 0);

        auto ptr = std::malloc(align_size(size, alignment));
        if constexpr (std::same_as<FailurePolicy, Terminate>)
            if (ptr == nullptr)
                ERROR("Malloc returned null");
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
                ERROR("Realloc returned null");
        return ptr;
    }
    void dealloc_v(void* ptr, usize size, usize alignment) override {
        ASSERT(ptr != nullptr);
        ASSERT(size > 0);
        ASSERT(alignment > 0);

        std::free(ptr);
    }

    static CAllocator& instance() {
        static CAllocator instance{};
        return instance;
    }

    template <typename T> [[nodiscard]] static T* alloc() {
        return static_cast<T*>(instance().alloc_v(sizeof(T), alignof(T)));
    }
    template <typename T> [[nodiscard]] static Slice<T> alloc(usize count) {
        ASSERT(count > 0);
        return {static_cast<T*>(instance().alloc_v(count * sizeof(T), alignof(T))), count};
    }
    template <typename T> [[nodiscard]] static Slice<T> realloc(Slice<T> original, usize new_count) {
        return {static_cast<T*>(instance().realloc_v(original.data, original.count * sizeof(T), new_count * sizeof(T), alignof(T))), new_count};
    }
    template <typename T> static void dealloc(T* ptr) {
        instance().dealloc_v(ptr, sizeof(T), alignof(T));
    }
    template <typename T> static void dealloc(Slice<T> slice) {
        instance().dealloc_v(slice.data, slice.count * sizeof(T), alignof(T));
    }
};

inline CAllocator<>& mallocator() { return CAllocator<>::instance(); }

template <FailurePolicyTag FailurePolicy = Terminate>
class StackAllocator : public Allocator {
public:
    StackAllocator() = default;
    StackAllocator(Slice<std::byte> memory) : m_memory{memory}, m_head{align_ptr(memory.data, 16)} {}

    static StackAllocator create(Allocator& parent, const usize size) { return parent.alloc<std::byte>(size); }
    void destroy(Allocator& parent) const { parent.dealloc(m_memory); }

    [[nodiscard]] void* alloc_v(const usize size, const usize) override {
        ASSERT(size > 0);

        std::byte* alloc_ptr = m_head;
        std::byte* alloc_end = alloc_ptr + align_size(size, 16);
        if (alloc_end > m_memory.end()) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Stack allocator out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_ERROR("Stack allocator out of memory");
                return nullptr;
            }
        }
        m_head = align_ptr(alloc_end, 16);

        return alloc_ptr;
    }

    [[nodiscard]] void* realloc_v(void* original, const usize original_size, const usize new_size, const usize) override {
        ASSERT(original != nullptr);
        ASSERT(original_size > 0);
        ASSERT(new_size > 0);

        std::byte* original_ptr = static_cast<std::byte*>(original);

        if (align_ptr(original_ptr + original_size, 16) != m_head) {
            void* alloc_ptr = alloc_v(new_size, 16);
            std::memmove(alloc_ptr, original, align_size(original_size, 16));
            return alloc_ptr;
        }

        std::byte* new_end = align_ptr(original_ptr + new_size, 16);
        if (new_end > m_memory.end())
            ERROR("Stack allocator out of memory");

        m_head = new_end;
        return original;
    }

    void dealloc_v(void* ptr, const usize size, const usize) override {
        ASSERT(ptr != nullptr);
        ASSERT(size > 0);

        std::byte* ptr_byte = static_cast<std::byte*>(ptr);

        if (align_ptr(ptr_byte + size, 16) != m_head) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Deallocation of invalid pointer from stack allocator");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_ERROR("Deallocation of invalid pointer from stack allocator");
                return;
            }
        }

        m_head = ptr_byte;
    }

    void reset() { m_head = m_memory.data; }

private:
    Slice<std::byte> m_memory{};
    std::byte* m_head = nullptr;
};

struct DoubleStack {
    StackAllocator<>* temporary_space{};
    StackAllocator<>* return_space{};

    DoubleStack swap() const {
        return DoubleStack{
            .temporary_space = return_space,
            .return_space = temporary_space,
        };
    }

    template <typename T> [[nodiscard]] T* alloc() const {
        return static_cast<T*>(temporary_space->alloc_v(sizeof(T), alignof(T)));
    }
    template <typename T> [[nodiscard]] Slice<T> alloc(usize count) const {
        return {static_cast<T*>(temporary_space->alloc_v(count * sizeof(T), alignof(T))), count};
    }
    template <typename T> [[nodiscard]] Slice<T> realloc(Slice<T> original, usize new_count) const {
        return {static_cast<T*>(temporary_space->realloc_v(original.data, original.count * sizeof(T), new_count * sizeof(T), alignof(T))), new_count};
    }

    template <typename T> void dealloc(T* ptr) const {
        temporary_space->dealloc_v(ptr, sizeof(T), alignof(T));
    }
    template <typename T> void dealloc(Slice<T> slice) const {
        temporary_space->dealloc_v(slice.data, slice.count * sizeof(T), alignof(T));
    }

    template <typename T> [[nodiscard]] T* alloc_return() const {
        return static_cast<T*>(return_space->alloc_v(sizeof(T), alignof(T)));
    }
    template <typename T> [[nodiscard]] Slice<T> alloc_return(usize count) const {
        return {static_cast<T*>(return_space->alloc_v(count * sizeof(T), alignof(T))), count};
    }
    template <typename T> [[nodiscard]] Slice<T> realloc_return(Slice<T> original, usize new_count) const {
        return {static_cast<T*>(return_space->realloc_v(
            original.data, original.count * sizeof(T), new_count * sizeof(T), alignof(T)
        )), new_count};
    }
};

template <FailurePolicyTag FailurePolicy = Terminate>
class PackedLinearAllocator : public Allocator {
public:
    PackedLinearAllocator() = default;
    PackedLinearAllocator(Slice<std::byte> memory) : m_memory{memory}, m_head{memory.data} {}

    static PackedLinearAllocator create(Allocator& parent, const usize size) { return parent.alloc<std::byte>(size); }
    void destroy(Allocator& parent) const { parent.dealloc(m_memory); }

    [[nodiscard]] void* alloc_v(const usize size, const usize alignment) override {
        ASSERT(size > 0);
        ASSERT(alignment > 0);

        std::byte* alloc_ptr = align_ptr(m_head, alignment);
        std::byte* alloc_end = alloc_ptr + align_size(size, alignment);
        if (alloc_end > m_memory.end()) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Linear allocator out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_ERROR("Linear allocator out of memory");
                return nullptr;
            }
        }
        m_head = alloc_end;

        return alloc_ptr;
    }

    void* realloc_v(void* original, const usize original_size, const usize new_size, const usize alignment) override {
        ASSERT(original != nullptr);
        ASSERT(original_size > 0);
        ASSERT(new_size > 0);
        ASSERT(alignment > 0);

        std::byte* original_ptr = static_cast<std::byte*>(original);

        if (align_ptr(original_ptr + original_size, alignment) != m_head) {
            void* alloc_ptr = alloc_v(new_size, alignment);
            std::memmove(alloc_ptr, original_ptr, original_size);
            return alloc_ptr;
        }

        std::byte* new_end = align_ptr(original_ptr + new_size, alignment);
        if (new_end > m_memory.end())
            ERROR("Linear allocator out of memory");

        m_head = new_end;
        return original;
    }

    void dealloc_v(void*, usize, usize) override {}

    void reset() { m_head = m_memory.data; }

private:
    Slice<std::byte> m_memory{};
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
    FixedSizeAllocator(Slice<Slot> slots) : m_slots{slots} {
        for (usize i = 0; i < m_slots.count; ++i) {
            m_slots[i].next = i + 1;
        }
    }

    static FixedSizeAllocator create(Allocator& parent, const usize count) { return parent.alloc<Slot>(count); }
    void destroy(Allocator& parent) const {
        ASSERT(m_slots.data != nullptr);
        check_leaks();
        parent.dealloc(m_slots);
    }

    [[nodiscard]] void* alloc_v(const usize size, const usize) override {
        ASSERT(align_size(size, 16) <= Size);

        usize index = m_next;
        if (index >= m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Fixed size allocator out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_ERROR("Fixed size allocator out of memory");
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
        const usize index = static_cast<usize>(slot - m_slots.data);
        slot->next = m_next;
        m_next = index;
    }

private:
    void check_leaks() const {
#ifndef NDEBUG
        usize count = 0;
        usize index = m_next;
        while (index != m_slots.count && count <= m_slots.size()) {
            index = m_slots[index].next;
            ++count;
        }
        if (count < m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Fixed size allocator leaked memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>)
                LOG_ERROR("Fixed size allocator leaked memory");
        }
        if (count > m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Fixed size allocator had double frees");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>)
                LOG_ERROR("Fixed size allocator had double frees");
        }
#endif
    }

    Slice<Slot> m_slots{};
    usize m_next = 0;
};

template <typename T, FailurePolicyTag FailurePolicy = Terminate>
class PoolAllocator {
private:
    union Slot {
        T data;
        usize next;
    };

public:
    PoolAllocator() = default;
    PoolAllocator(Slice<Slot> slots) : m_slots{slots} {
        for (usize i = 0; i < m_slots.count; ++i) {
            m_slots[i].next = i + 1;
        }
    }

    static PoolAllocator create(Allocator& parent, const usize count) { return parent.alloc<Slot>(count); }
    void destroy(Allocator& parent) const {
        ASSERT(m_slots.data != nullptr);
        check_leaks();
        parent.dealloc(m_slots);
    }

    [[nodiscard]] T* alloc(const usize count = 1) {
        ASSERT(count > 0);

        usize index = m_next;
        if (index >= m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Resource pool out of memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>) {
                LOG_ERROR("Resource pool out of memory");
                return nullptr;
            }
        }
        m_next = m_slots[index].next;
        return &m_slots[index].data;
    }

    void dealloc(T* ptr) {
        ASSERT(ptr != nullptr);

        Slot* slot = reinterpret_cast<Slot*>(ptr);
        const usize index = static_cast<usize>(slot - m_slots.data);
        slot->next = m_next;
        m_next = index;
    }

private:
    void check_leaks() const {
#ifndef NDEBUG
        usize count = 0;
        usize index = m_next;
        while (index != m_slots.count && count <= m_slots.count) {
            index = m_slots[index].next;
            ++count;
        }
        if (count < m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Pool allocator leaked memory");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>)
                LOG_ERROR("Pool allocator leaked memory");
        }
        if (count > m_slots.count) {
            if constexpr (std::same_as<FailurePolicy, Terminate>)
                ERROR("Pool allocator had double frees");
            if constexpr (std::same_as<FailurePolicy, ReturnNull>)
                LOG_ERROR("Pool allocator had double frees");
        }
#endif
    }

    Slice<Slot> m_slots{};
    usize m_next = 0;
};

} // namespace hg
