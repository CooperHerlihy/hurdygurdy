#pragma once

#include "hg_utils.h"
#include "hg_memory.h"
#include "hg_vulkan.h"

namespace hg {

class Engine {
public:
    struct Config {
        usize global_allocator_size = 16 * 1024 * 1024;
        usize frame_allocator_size = 1024 * 1024;
        usize stack_allocator_size = 1024 * 1024;

        usize max_texture_count = 256;
    };
    [[nodiscard]] static Result<Engine> create(const Config& config);

    Engine() = default;
    ~Engine() noexcept;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&& other) noexcept;
    Engine& operator=(Engine&& other) noexcept;

    DoubleStack stack() { return DoubleStack{m_stacks[0], m_stacks[1]}; }

    [[nodiscard]] Vk& vk() {
        ASSERT(m_vk != nullptr);
        m_vk->stack = stack();
        return *m_vk;
    }

private:
    bool m_moved_from = false;

    PackedLinearAllocator<> m_global_storage{};
    PackedLinearAllocator<> m_frame_storage{};
    std::array<StackAllocator<>, 2> m_stacks{};

    Vk* m_vk = nullptr;
};

} // namespace hg
