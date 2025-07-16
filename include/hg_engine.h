#pragma once

#include "hg_utils.h"
#include "hg_load.h"
#include "hg_window.h"
#include "hg_vulkan.h"

namespace hg {

class Engine {
public:
    AssetLoader loader;
    Vk vk;

    Engine() = default;

    [[nodiscard]] static Result<Engine> create();
    void destroy() {
        vk.destroy();
        loader.destroy();

        SDL_Quit();
    }
};

} // namespace hg
