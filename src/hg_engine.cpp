#include "hg_engine.h"

namespace hg {

Result<Engine> Engine::create(const Config& config) {
    auto engine = ok<Engine>();

    engine->m_global_storage = PackedLinearAllocator<>::create(mallocator(), config.global_allocator_size);
    engine->m_frame_storage = PackedLinearAllocator<>::create(engine->m_global_storage, config.frame_allocator_size);
    for (auto& stack : engine->m_stacks) {
        stack = StackAllocator<>::create(engine->m_global_storage, config.stack_allocator_size);
    }

    const auto vk = Vk::create(engine->stack());
    if (vk.has_err())
        return vk.err();
    engine->m_vk = new (engine->m_global_storage.alloc<Vk>()) Vk{*vk};

    return engine;
}

Engine::~Engine() noexcept {
    if (m_moved_from)
        return;

    m_vk->destroy();

    m_global_storage.destroy(mallocator());
}

Engine::Engine(Engine&& other) noexcept
    : m_moved_from{other.m_moved_from}
    , m_global_storage{other.m_global_storage}
    , m_frame_storage{other.m_frame_storage}
    , m_stacks{other.m_stacks}
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
