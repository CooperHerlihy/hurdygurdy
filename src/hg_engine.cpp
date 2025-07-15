#include "hg_engine.h"

namespace hg {

Result<Engine> Engine::create() {
    auto engine = ok<Engine>();

    engine->loader = AssetLoader{{
        .max_images = 16,
        .max_gltfs = 16,
    }};

    auto vk = Vk::create();
    if (vk.has_err())
        return vk.err();
    engine->vk = std::move(*vk);

    return engine;
}

Engine::~Engine() noexcept {
    if (m_moved_from)
        return;

    vk.destroy();
    loader.destroy();
}

Engine::Engine(Engine&& other) noexcept
    : loader{std::move(other.loader)}
    , vk{std::move(other.vk)}
{
    if (other.m_moved_from)
        ERROR("Engine already moved from");
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
