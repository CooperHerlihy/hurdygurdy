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

    [[nodiscard]] LinearAllocator<>& long_term_allocator() { return m_global_allocator; }
    [[nodiscard]] LinearAllocator<>& short_term_allocator() { return m_frame_allocator; }
    [[nodiscard]] StackAllocator<>& stack_allocator() { return m_stack_allocator; }

    [[nodiscard]] PoolAllocator<Texture>& texture_allocator() { return m_texture_allocator; }

    [[nodiscard]] Vk& vk() { return *m_vk; }
    [[nodiscard]] const Vk& vk() const { return *m_vk; }

private:
    bool m_moved_from = false;

    LinearAllocator<> m_global_allocator{};
    LinearAllocator<> m_frame_allocator{};
    StackAllocator<> m_stack_allocator{};

    PoolAllocator<Texture> m_texture_allocator{};

    Vk* m_vk = nullptr;
};

} // namespace hg
