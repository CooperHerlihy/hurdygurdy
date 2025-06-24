#include "hg_mesh.h"

namespace hg {

Mesh generate_square() {
    return {
        .indices    = {0, 1, 2, 2, 3, 0},
        .positions  = {{-1.0f, -1.0f,  0.0f}, {-1.0f,  1.0f,  0.0f}, { 1.0f,  1.0f,  0.0f}, { 1.0f, -1.0f,  0.0f}},
        .normals    = {{ 0.0f,  0.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}},
        .tex_coords = {{ 0.0f,  0.0f       }, { 0.0f,  1.0f       }, { 1.0f,  1.0f       }, { 1.0f,  0.0f       }},
    };
}

Mesh generate_cube() {
    return {
        .indices = {
             0,  1,  2,  2,  3,  0,
             4,  5,  6,  6,  7,  4,
             8,  9, 10, 10, 11,  8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20,
        },
        .positions = {
            {-1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f},
            {-1.0f, -1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f},
            {-1.0f, -1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
            { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f},
            {-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f, -1.0f},
        },
        .normals = {
            { 0.0f, -1.0f,  0.0f}, { 0.0f, -1.0f,  0.0f}, { 0.0f, -1.0f,  0.0f}, { 0.0f, -1.0f,  0.0f},
            {-1.0f,  0.0f,  0.0f}, {-1.0f,  0.0f,  0.0f}, {-1.0f,  0.0f,  0.0f}, {-1.0f,  0.0f,  0.0f},
            { 0.0f,  0.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, { 0.0f,  0.0f, -1.0f},
            { 1.0f,  0.0f,  0.0f}, { 1.0f,  0.0f,  0.0f}, { 1.0f,  0.0f,  0.0f}, { 1.0f,  0.0f,  0.0f},
            { 0.0f,  0.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, { 0.0f,  0.0f,  1.0f},
            { 0.0f,  1.0f,  0.0f}, { 0.0f,  1.0f,  0.0f}, { 0.0f,  1.0f,  0.0f}, { 0.0f,  1.0f,  0.0f},
        },
        .tex_coords = {
            {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},
            {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},
            {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},
            {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},
            {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},
            {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},
        },
    };
}

Mesh generate_sphere(i32 fidelity) {
    ASSERT(fidelity >= 3);

    Mesh sphere = {};
    sphere.positions.reserve(static_cast<usize>(2 + fidelity * fidelity));
    sphere.normals.reserve(static_cast<usize>(2 + fidelity * fidelity));
    sphere.tex_coords.resize(static_cast<usize>(2 + fidelity * fidelity));

    glm::vec3 point = {0.0f, -1.0f, 0.0f};
    sphere.positions.emplace_back(point);
    sphere.normals.emplace_back(point);
    for (i32 i = 0; i < fidelity; ++i) {
        f32 h = -std::cos(glm::pi<f32>() * i / fidelity);
        f32 r = std::sin(glm::pi<f32>() * i / fidelity);
        for (i32 j = 0; j < fidelity; ++j) {
            point = glm::vec3{r * std::cos(glm::tau<f32>() * j / fidelity), h, r * std::sin(glm::tau<f32>() * j / fidelity)};
            sphere.positions.emplace_back(point);
            sphere.normals.emplace_back(point);
        }
    }
    point = {0.0f, 1.0f, 0.0f};
    sphere.positions.emplace_back(point);
    sphere.normals.emplace_back(point);

    sphere.indices.reserve(static_cast<usize>(fidelity * (fidelity + 2) * 3));
    for (i32 i = 0; i < fidelity; ++i) {
        sphere.indices.push_back(0);
        sphere.indices.push_back((i + 1) % fidelity + 1);
        sphere.indices.push_back((i + 2) % fidelity + 1);
    }
    for (i32 i = 1; i <= fidelity * (fidelity - 1); ++i) {
        sphere.indices.push_back(i);
        sphere.indices.push_back(i + fidelity);
        sphere.indices.push_back(i + 1);

        sphere.indices.push_back(i + 1);
        sphere.indices.push_back(i + fidelity);
        sphere.indices.push_back(i + fidelity + 1);
    }
    i32 top_index = 1 + fidelity * fidelity;
    for (i32 i = 0; i < fidelity; ++i) {
        sphere.indices.push_back(top_index);
        sphere.indices.push_back(top_index - ((i + 1) % fidelity + 1));
        sphere.indices.push_back(top_index - ((i + 2) % fidelity + 1));
    }

    ASSERT(!sphere.indices.empty());
    ASSERT(!sphere.positions.empty());
    ASSERT(!sphere.normals.empty());
    ASSERT(!sphere.tex_coords.empty());
    return sphere;
}

} // namespace hg
