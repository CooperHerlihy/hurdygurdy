#include "hg_engine.h"

namespace hg {

Result<Engine> Engine::create(const Config& config) {
    auto engine = ok<Engine>();

    engine->m_global_allocator = LinearAllocator<>::create<CAllocator<>>(config.global_allocator_size);
    engine->m_stack_allocator = StackAllocator<>::create(engine->m_global_allocator, config.stack_allocator_size);
    engine->m_frame_allocator = LinearAllocator<>::create(engine->m_global_allocator, config.frame_allocator_size);

    engine->m_texture_allocator = PoolAllocator<Texture>::create(engine->m_global_allocator, config.max_texture_count);

    const auto vk = Vk::create();
    if (vk.has_err())
        return vk.err();
    engine->m_vk = new (engine->m_global_allocator.template alloc<Vk>()) Vk{*vk};

    const auto window = Window::create(engine->vk(), config.fullscreen, config.window_size.x, config.window_size.y);
    if (window.has_err())
        return window.err();
    engine->m_window = new (engine->m_global_allocator.template alloc<Window>()) Window{*window};

    return engine;
}

Engine::~Engine() noexcept {
    if (m_moved_from)
        return;

    m_window->destroy(*m_vk);
    m_vk->destroy();

    m_texture_allocator.destroy(m_global_allocator);

    m_frame_allocator.destroy(m_global_allocator);
    m_stack_allocator.destroy(m_global_allocator);
    m_global_allocator.destroy<CAllocator<>>();
}

Engine::Engine(Engine&& other) noexcept
    : m_moved_from{other.m_moved_from}
    , m_global_allocator{other.m_global_allocator}
    , m_frame_allocator{other.m_frame_allocator}
    , m_stack_allocator{other.m_stack_allocator}
    , m_frame_index{other.m_frame_index}

    , m_texture_allocator{other.m_texture_allocator}

    , m_vk{other.m_vk}
    , m_window{other.m_window}
{
    other.m_moved_from = true;
}

Engine& Engine::operator=(Engine&& other) noexcept {
    if (this == &other)
        return *this;
    this->~Engine();
    new (this) Engine(std::move(other));
    return *this;
}

} // namespace hg
