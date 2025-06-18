#pragma once

#include "hg_utils.h"

namespace hg {

struct Mesh {
    Vec<u32> indices = {};
    Vec<glm::vec3> positions = {};
    Vec<glm::vec3> normals = {};
    Vec<glm::vec2> tex_coords = {};
};

[[nodiscard]] Mesh generate_square();
[[nodiscard]] Mesh generate_cube();
[[nodiscard]] Mesh generate_sphere(i32 fidelity);

} // namespace hg
