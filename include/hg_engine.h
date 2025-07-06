#pragma once

#include "hg_vulkan.h"

namespace hg {

class Engine {
public:
    [[nodiscard]] static Result<Engine> create() {
        const auto init_result = init_vulkan();
        if (init_result.has_err())
            return init_result.err();
        return ok<Engine>();
    }

    void destroy() const {
        deinit_vulkan();
    }

private:
};

} // namespace hg
