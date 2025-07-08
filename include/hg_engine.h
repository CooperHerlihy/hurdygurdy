#pragma once

#include "hg_vulkan.h"

namespace hg {

template <typename T> concept EngineRenderer = requires(T t) {
    { T::create(std::declval<Vk>(), std::declval<vk::Extent2D>()) } -> std::same_as<T>;
    { t.resize(std::declval<Vk>(), std::declval<vk::Extent2D>()) } -> std::same_as<void>;
    { t.destroy(std::declval<Vk>()) } -> std::same_as<void>;

    { t.draw(std::declval<Window::DrawInfo>()) } -> std::same_as<void>;
};

template <typename Renderer>
class Engine {
    static_assert(EngineRenderer<Renderer>);
public:
    struct Config {
        bool fullscreen = false;
        glm::uvec2 window_size{1920, 1080};

        usize global_allocator_size = 1024 * 1024 * 1024;
        usize stack_allocator_size = 1024 * 1024 * 1024;

        usize frame_allocator_size = 1024 * 1024 * 1024;

        usize max_texture_count = 1024;
    };
    static Result<Engine> create(const Config& config) {
        auto engine = ok<Engine>();

        engine->m_global_memory = std::make_unique<std::byte[]>(config.global_allocator_size);
        engine->m_stack_memory = std::make_unique<std::byte[]>(config.stack_allocator_size);

        if (engine->m_global_memory == nullptr)
            return Err::OutOfMemory;
        if (engine->m_stack_memory == nullptr)
            return Err::OutOfMemory;

        engine->m_global_allocator = LinearAllocator<ReturnNull>{
            engine->m_global_memory.get(), config.global_allocator_size
        };
        engine->m_stack_allocator = StackAllocator<ReturnNull>{
            engine->m_stack_memory.get(), config.stack_allocator_size
        };

        for (usize i = 0; i < 2; ++i) {
            engine->m_frame_memories[i] = std::make_unique<std::byte[]>(config.frame_allocator_size);
            if (engine->m_frame_memories[i] == nullptr)
                return Err::OutOfMemory;
            engine->m_frame_allocators[i] = LinearAllocator<ReturnNull>{
                engine->m_frame_memories[i].get(), config.frame_allocator_size
            };
        }

        const auto vk = Vk::create();
        if (vk.has_err())
            return vk.err();
        engine->m_vk.reset(new (engine->m_global_allocator.template alloc<Vk>()) Vk{std::move(*vk)});

        const auto window = Window::create(engine->vk(), config.fullscreen, config.window_size.x, config.window_size.y);
        if (window.has_err())
            return window.err();
        engine->m_window.reset(new (engine->m_global_allocator.template alloc<Window>()) Window{std::move(*window)});

        engine->m_renderer.reset(new (engine->m_global_allocator.template alloc<Renderer>()) Renderer{
            Renderer::create(engine->vk(), engine->window().extent())
        });

        auto texture_memory = reinterpret_cast<Texture*>(malloc(config.max_texture_count * sizeof(Texture)));
        if (texture_memory == nullptr)
            return Err::OutOfMemory;
        engine->m_texture_allocator = PoolAllocator<Texture>{texture_memory, config.max_texture_count};

        return engine;
    }

    ~Engine() noexcept {
        if (m_vk != nullptr) {
            m_renderer->destroy(*m_vk);
            m_window->destroy(*m_vk);
            m_vk->destroy();
        }
    }

    Engine() = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&& other) noexcept = default;
    Engine& operator=(Engine&& other) noexcept = default;

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
    std::unique_ptr<std::byte[]> m_global_memory = nullptr;
    std::unique_ptr<std::byte[]> m_stack_memory = nullptr;
    LinearAllocator<ReturnNull> m_global_allocator{};
    StackAllocator<ReturnNull> m_stack_allocator{};

    std::array<std::unique_ptr<std::byte[]>, 2> m_frame_memories{};
    std::array<LinearAllocator<ReturnNull>, 2> m_frame_allocators{};
    usize m_frame_index = 0;

    std::unique_ptr<Vk, decltype([](void*) {})> m_vk = nullptr;
    std::unique_ptr<Window, decltype([](void*) {})> m_window = nullptr;
    std::unique_ptr<Renderer, decltype([](void*) {})> m_renderer = nullptr;

    std::unique_ptr<Texture[]> m_texture_memory = nullptr;
    PoolAllocator<Texture> m_texture_allocator{};
};

} // namespace hg
