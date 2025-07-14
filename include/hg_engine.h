#pragma once

#include "hg_utils.h"
#include "hg_load.h"
#include "hg_window.h"
#include "hg_vulkan.h"

namespace hg {

class Engine {
private:
    bool m_moved_from = false;
public:
    AssetLoader loader;
    Vk vk;

    [[nodiscard]] static Result<Engine> create();

    Engine() = default;
    ~Engine() noexcept;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&& other) noexcept;
    Engine& operator=(Engine&& other) noexcept;
};

} // namespace hg
