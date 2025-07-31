#pragma once

#include "hg_utils.h"
#include "hg_load.h"
#include "hg_vulkan.h"

namespace hg {

class Engine {
public:
    AssetLoader loader;
    Vk vk;

    Engine() = default;

    [[nodiscard]] static Result<Engine> create();
    void destroy() {
        destroy_vk(vk);
        loader.destroy();

        SDL_Quit();
    }
};

} // namespace hg
