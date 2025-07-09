#pragma once

#include "hg_utils.h"
#include "hg_vulkan.h"

namespace hg {

template <typename T> concept EngineRenderer = requires(T t) {
    { T::create(std::declval<Vk>(), std::declval<vk::Extent2D>()) } -> std::same_as<T>;
    { t.resize(std::declval<Vk>(), std::declval<vk::Extent2D>()) } -> std::same_as<void>;
    { t.destroy(std::declval<Vk>()) } -> std::same_as<void>;

    { t.draw(std::declval<Window::DrawInfo>()) } -> std::same_as<void>;
};

template <EngineRenderer Renderer>
class Engine {
public:
    struct Config {
        bool fullscreen = false;
        glm::uvec2 window_size{1920, 1080};

        usize global_allocator_size = 1024 * 1024 * 1024;
        usize stack_allocator_size = 128 * 1024 * 1024;
        usize frame_allocator_size = 128 * 1024;

        usize max_texture_count = 256;
    };
    static Result<Engine> create(const Config& config) {
        auto engine = ok<Engine>();

        engine->m_global_allocator = LinearAllocator<CAllocator>{config.global_allocator_size};
        engine->m_stack_allocator = StackAllocator<decltype(engine->m_global_allocator)>{engine->m_global_allocator, config.stack_allocator_size};
        for (auto& frame_allocator : engine->m_frame_allocators) {
            frame_allocator = LinearAllocator<decltype(engine->m_global_allocator)>{engine->m_global_allocator, config.frame_allocator_size};
        }

        engine->m_texture_allocator = PoolAllocator<Texture, decltype(engine->m_global_allocator)>{engine->m_global_allocator, config.max_texture_count};

        const auto vk = Vk::create();
        if (vk.has_err())
            return vk.err();
        engine->m_vk = new (engine->m_global_allocator.template alloc<Vk>()) Vk{*vk};

        const auto window = Window::create(engine->vk(), config.fullscreen, config.window_size.x, config.window_size.y);
        if (window.has_err())
            return window.err();
        engine->m_window = new (engine->m_global_allocator.template alloc<Window>()) Window{*window};

        engine->m_renderer = new (engine->m_global_allocator.template alloc<Renderer>()) Renderer{
            Renderer::create(engine->vk(), engine->window().extent())
        };

        return engine;
    }

    Engine() = default;
    ~Engine() noexcept {
        if (m_moved_from)
            return;

        m_renderer->destroy(*m_vk);
        m_window->destroy(*m_vk);
        m_vk->destroy();

        m_texture_allocator.destroy(m_global_allocator);

        for (auto& frame_allocator : m_frame_allocators) {
            frame_allocator.destroy(m_global_allocator);
        }
        m_stack_allocator.destroy(m_global_allocator);
        m_global_allocator.destroy();
    }
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&& other) noexcept
        : m_moved_from{other.m_moved_from}
        , m_global_allocator{other.m_global_allocator}
        , m_stack_allocator{other.m_stack_allocator}
        , m_frame_allocators{other.m_frame_allocators}
        , m_frame_index{other.m_frame_index}

        , m_texture_allocator{other.m_texture_allocator}

        , m_vk{other.m_vk}
        , m_window{other.m_window}
        , m_renderer{other.m_renderer}
    {
        other.m_moved_from = true;
    }
    Engine& operator=(Engine&& other) noexcept {
        if (this == &other)
            return *this;
        this->~Engine();
        new (this) Engine(std::move(other));
        return *this;
    }

    [[nodiscard]] const Vk& vk() const { return *m_vk; }
    [[nodiscard]] const Window& window() const { return *m_window; }
    [[nodiscard]] const Renderer& renderer() const { return m_renderer; }
    [[nodiscard]] Vk& vk() { return *m_vk; }
    [[nodiscard]] Window& window() { return *m_window; }
    [[nodiscard]] Renderer& renderer() { return *m_renderer; }

    [[nodiscard]] Result<void> draw() {
        CONTEXT("Drawing frame");
        m_frame_index = (m_frame_index + 1) % MaxFramesInFlight;

        const auto frame_result = [this]() -> Result<void> {
            const auto begin_result = m_window->begin_frame(*m_vk);
            if (begin_result.has_err())
                return begin_result.err();

            m_renderer->draw(m_window->draw_info());

            const auto end_result = m_window->end_frame(*m_vk);
            if (end_result.has_err())
                return end_result.err();

            return ok();
        }();
        if (frame_result.has_err()) {
            switch (frame_result.err()) {
            case Err::FrameTimeout: return frame_result.err();
            case Err::InvalidWindow: {
                for (int w = 0, h = 0; w <= 1 || h <= 1; glfwGetWindowSize(m_window->window(), &w, &h)) {
                    glfwPollEvents();
                }

                const auto resize_result = m_window->resize(*m_vk);
                if (resize_result.has_err())
                    ERROR("Could not resize window: {}", to_string(resize_result.err()));
                }

                m_renderer->resize(*m_vk, m_window->extent());
                break;
            default: ERROR("Unexpected error: {}", to_string(frame_result.err()));
            }
        }

        return ok();
    }

private:
    bool m_moved_from = false;

    LinearAllocator<CAllocator> m_global_allocator{};
    StackAllocator<decltype(m_global_allocator)> m_stack_allocator{};
    std::array<LinearAllocator<decltype(m_global_allocator)>, 2> m_frame_allocators{};
    usize m_frame_index = 0;

    PoolAllocator<Texture, decltype(m_global_allocator)> m_texture_allocator{};

    Vk* m_vk = nullptr;
    Window* m_window = nullptr;
    Renderer* m_renderer = nullptr;
};

} // namespace hg
