#pragma once

#include "hg_utils.h"
#include "hg_load.h"

#include <vector>

namespace hg {

struct Mesh {
    std::vector<u32> indices = {};
    std::vector<ModelVertex> vertices = {};
};

[[nodiscard]] Mesh generate_square();
[[nodiscard]] Mesh generate_cube();
[[nodiscard]] Mesh generate_sphere(i32 fidelity);

}
