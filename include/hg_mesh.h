#pragma once

#include "hg_utils.h"

namespace hg {

struct Mesh {
    std::vector<u32> indices = {};
    std::vector<glm::vec3> positions = {};
    std::vector<glm::vec3> normals = {};
    std::vector<glm::vec2> tex_coords = {};
};

[[nodiscard]] Mesh generate_square();
[[nodiscard]] Mesh generate_cube();
[[nodiscard]] Mesh generate_sphere(i32 fidelity);

} // namespace hg
