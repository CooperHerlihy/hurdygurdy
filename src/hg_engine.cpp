#include "hg_engine.h"

namespace hg {

Result<Engine> Engine::create(const Config& config) {
    auto engine = ok<Engine>();

    engine->m_memory = Memory::create(config);

    auto vk = Vk::create();
    if (vk.has_err())
        return vk.err();
    engine->m_vk = new (engine->memory().heap.alloc<Vk>()) Vk{std::move(*vk)};

    return engine;
}

Engine::~Engine() noexcept {
    if (m_moved_from)
        return;

    m_vk->destroy();
    m_memory.destroy();
}

Engine::Engine(Engine&& other) noexcept
    : m_moved_from{other.m_moved_from}
    , m_memory{std::move(other.m_memory)}
    , m_vk{std::move(other.m_vk)}
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
