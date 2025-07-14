#pragma once

#include "hg_utils.h"
#include "hg_vulkan.h"

namespace hg {

class Engine {
public:
    [[nodiscard]] static Result<Engine> create();

    Engine() = default;
    ~Engine() noexcept;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&& other) noexcept;
    Engine& operator=(Engine&& other) noexcept;

    [[nodiscard]] Vk& vk() { return m_vk; }

private:
    bool m_moved_from = false;
    Vk m_vk{};
};

} // namespace hg
