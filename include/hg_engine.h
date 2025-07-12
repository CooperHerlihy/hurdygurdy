#pragma once

#include "hg_utils.h"
#include "hg_vulkan.h"

namespace hg {

class Engine {
public:
    struct Config {
        usize global_allocator_size = 1024 * 1024 * 1024;
        usize stack_allocator_size = 128 * 1024 * 1024;
        usize frame_allocator_size = 128 * 1024;

        usize max_texture_count = 256;
    };
    [[nodiscard]] static Result<Engine> create(const Config& config);

    Engine() = default;
    ~Engine() noexcept;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&& other) noexcept;
    Engine& operator=(Engine&& other) noexcept;

    void use_global_allocator() { m_use_frame_allocator = false; }
    void use_frame_allocator() { m_use_frame_allocator = true; }

    operator Memory() {
        if (m_use_frame_allocator)
            return Memory{m_frame_allocator, m_stack_allocator};
        else
            return Memory{m_global_allocator, m_stack_allocator};
    }

    Memory global_memory() { return Memory{m_global_allocator, m_stack_allocator}; }
    Memory frame_memory() { return Memory{m_frame_allocator, m_stack_allocator}; }

    template <typename T> [[nodiscard]] Result<T> alloc_global() {
        return static_cast<T*>(m_global_allocator.alloc_v(sizeof(T), alignof(T)));
    }
    template <typename T> [[nodiscard]] Slice<T> alloc_global(usize count) {
        ASSERT(count > 0);
        return static_cast<T*>(m_global_allocator.alloc_v(count * sizeof(T), alignof(T)));
    }

    template <typename T> [[nodiscard]] Result<T> alloc_frame() {
        return static_cast<T*>(m_frame_allocator.alloc_v(sizeof(T), alignof(T)));
    }
    template <typename T> [[nodiscard]] Slice<T> alloc_frame(usize count) {
        ASSERT(count > 0);
        return static_cast<T*>(m_frame_allocator.alloc_v(count * sizeof(T), alignof(T)));
    }

    template <typename T> [[nodiscard]] Result<T> alloc_stack() {
        return static_cast<T*>(m_stack_allocator.alloc_v(sizeof(T), alignof(T)));
    }
    template <typename T> [[nodiscard]] Slice<T> alloc_stack(usize count) {
        ASSERT(count > 0);
        return static_cast<T*>(m_stack_allocator.alloc_v(count * sizeof(T), alignof(T)));
    }

    [[nodiscard]] PoolAllocator<Texture>& texture_allocator() { return m_texture_allocator; }
    [[nodiscard]] Texture* alloc_texture() { return m_texture_allocator.alloc(); }
    void dealloc_texture(Texture* texture) { m_texture_allocator.dealloc(texture); }

    [[nodiscard]] Vk& vk() { return *m_vk; }
    [[nodiscard]] const Vk& vk() const { return *m_vk; }

    operator Vk&() { return *m_vk; }
    operator const Vk&() const { return *m_vk; }

private:
    bool m_moved_from = false;

    bool m_use_frame_allocator = false;
    LinearAllocator<> m_global_allocator{};
    LinearAllocator<> m_frame_allocator{};
    StackAllocator<> m_stack_allocator{};

    PoolAllocator<Texture> m_texture_allocator{};

    Vk* m_vk = nullptr;
};

} // namespace hg
