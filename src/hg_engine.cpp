#include "hg_engine.h"

namespace hg {

Result<Engine> Engine::create() {
    auto engine = ok<Engine>();

    auto sdl_success = SDL_Init(SDL_INIT_VIDEO);
    if (!sdl_success)
        ERRORF("Could not initialize SDL: {}", SDL_GetError());

    engine->loader = AssetLoader{{
        .max_images = 16,
        .max_gltfs = 16,
    }};

    auto vk = create_vk();
    if (vk.has_err())
        return vk.err();
    engine->vk = std::move(*vk);

    return engine;
}

} // namespace hg
