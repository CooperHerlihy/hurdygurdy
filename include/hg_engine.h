#pragma once

#include "hg_utils.h"
#include "hg_renderer.h"

namespace hg {

class Engine {
public:
    using GlobalAllocator = LinearAllocator<CAllocator<Terminate>>;
    using GlobalStackAllocator = StackAllocator<GlobalAllocator>;
    using FrameAllocator = LinearAllocator<GlobalAllocator>;
    using TextureAllocator = PoolAllocator<Texture, GlobalAllocator>;

    struct Config {
        bool fullscreen = false;
        glm::uvec2 window_size{1920, 1080};

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

    [[nodiscard]] Result<void> draw(Window::Renderer& renderer) {
        m_frame_index = (m_frame_index + 1) % 2;

        const auto frame_result = m_window->draw(*m_vk, renderer);
        if (frame_result.has_err())
            return frame_result.err();

        return ok();
    }

    [[nodiscard]] GlobalAllocator& long_term_allocator() { return m_global_allocator; }
    [[nodiscard]] FrameAllocator& short_term_allocator() { return m_frame_allocator; }
    [[nodiscard]] GlobalStackAllocator& stack_allocator() { return m_stack_allocator; }

    [[nodiscard]] TextureAllocator& texture_allocator() { return m_texture_allocator; }

    [[nodiscard]] Vk& vk() { return *m_vk; }
    [[nodiscard]] Window& window() { return *m_window; }
    [[nodiscard]] const Vk& vk() const { return *m_vk; }
    [[nodiscard]] const Window& window() const { return *m_window; }

private:
    bool m_moved_from = false;

    GlobalAllocator m_global_allocator{};
    FrameAllocator m_frame_allocator{};
    GlobalStackAllocator m_stack_allocator{};
    usize m_frame_index = 0;

    TextureAllocator m_texture_allocator{};

    Vk* m_vk = nullptr;
    Window* m_window = nullptr;
};

} // namespace hg
