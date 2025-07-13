#pragma once

#include "hg_utils.h"
#include "hg_memory.h"
#include "hg_vulkan.h"

namespace hg {

class Engine {
public:
    struct Config : public Memory::Config {};
    [[nodiscard]] static Result<Engine> create(const Config& config);

    [[nodiscard]] Memory& memory() { return m_memory; }

    Engine() = default;
    ~Engine() noexcept;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&& other) noexcept;
    Engine& operator=(Engine&& other) noexcept;

    [[nodiscard]] Vk& vk() {
        ASSERT(m_vk != nullptr);
        return *m_vk;
    }

private:
    bool m_moved_from = false;

    Memory m_memory{};

    Vk* m_vk = nullptr;
};

} // namespace hg
