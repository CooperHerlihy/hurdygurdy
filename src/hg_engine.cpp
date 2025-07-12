#include "hg_engine.h"

namespace hg {

Result<Engine> Engine::create(const Config& config) {
    auto engine = ok<Engine>();

    engine->m_global_allocator = LinearAllocator<>::create(mallocator(), config.global_allocator_size);
    engine->m_stack_allocator = StackAllocator<>::create(engine->m_global_allocator, config.stack_allocator_size);
    engine->m_frame_allocator = LinearAllocator<>::create(engine->m_global_allocator, config.frame_allocator_size);

    engine->m_texture_allocator = PoolAllocator<Texture>::create(engine->m_global_allocator, config.max_texture_count);

    const auto vk = Vk::create();
    if (vk.has_err())
        return vk.err();
    engine->m_vk = new (engine->m_global_allocator.template alloc<Vk>()) Vk{*vk};

    return engine;
}

Engine::~Engine() noexcept {
    if (m_moved_from)
        return;

    m_vk->destroy();

    m_texture_allocator.destroy(m_global_allocator);

    m_frame_allocator.destroy(m_global_allocator);
    m_stack_allocator.destroy(m_global_allocator);
    m_global_allocator.destroy(mallocator());
}

Engine::Engine(Engine&& other) noexcept
    : m_moved_from{other.m_moved_from}
    , m_global_allocator{other.m_global_allocator}
    , m_frame_allocator{other.m_frame_allocator}
    , m_stack_allocator{other.m_stack_allocator}

    , m_texture_allocator{other.m_texture_allocator}

    , m_vk{other.m_vk}
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
