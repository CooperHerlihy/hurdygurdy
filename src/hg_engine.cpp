#include "hg_engine.h"

namespace hg {

Result<Engine> Engine::create() {
    auto engine = ok<Engine>();

    engine->image_loader = ImageLoader{{ .max_images = 32 }};

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
    image_loader.destroy();
}

Engine::Engine(Engine&& other) noexcept
    : m_moved_from{other.m_moved_from}
    , image_loader{std::move(other.image_loader)}
    , vk{std::move(other.vk)}
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
