#include "hg_generate.h"

#include <mikktspace/mikktspace.h>

namespace hg {

void Mesh::generate_tangents() {
    // TODO: use mikktspace to generator tangents
}

Mesh generate_square() {
    Mesh square = {
        .indices = {0, 1, 2, 2, 3, 0},
        .positions = {
            {-1.0f, -1.0f,  0.0f},
            {-1.0f,  1.0f,  0.0f},
            { 1.0f,  1.0f,  0.0f},
            { 1.0f, -1.0f,  0.0f}
        },
        .normals = {
            { 0.0f,  0.0f, -1.0f},
            { 0.0f,  0.0f, -1.0f},
            { 0.0f,  0.0f, -1.0f},
            { 0.0f,  0.0f, -1.0f}
        },
        .tex_coords = {
            { 0.0f,  0.0f},
            { 0.0f,  1.0f},
            { 1.0f,  1.0f},
            { 1.0f,  0.0f}
        },
    };
    square.generate_tangents();
    return square;
}

Mesh generate_cube() {
    Mesh cube = {
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
    cube.generate_tangents();
    return cube;
}

Mesh generate_sphere(const glm::uvec2 fidelity) {
    ASSERT(fidelity.x >= 3);
    ASSERT(fidelity.y >= 2);

    Mesh sphere = {};

    sphere.positions.reserve((fidelity.x + 1) * (fidelity.y + 1));
    sphere.normals.reserve((fidelity.x + 1) * (fidelity.y + 1));
    sphere.tex_coords.reserve((fidelity.x + 1) * (fidelity.y + 1));
    for (u32 i = 0; i <= fidelity.y; ++i) {
        f32 h = -std::cos(glm::pi<f32>() * i / fidelity.y);
        f32 r = std::sin(glm::pi<f32>() * i / fidelity.y);
        for (u32 j = 0; j <= fidelity.x; ++j) {
            f32 x = -std::sin(glm::tau<f32>() * j / fidelity.x) * r;
            f32 y = std::cos(glm::tau<f32>() * j / fidelity.x) * r;
            sphere.positions.emplace_back(x, h, y);
            sphere.normals.emplace_back(x, h, y);
            sphere.tex_coords.emplace_back(static_cast<f32>(j) / fidelity.x, static_cast<f32>(i) / fidelity.y);
        }
    }

    sphere.indices.reserve(fidelity.x * fidelity.y * 6);
    for (u32 i = 0; i < (fidelity.x + 1) * (fidelity.y); ++i) {
        if (i % (fidelity.x + 1) == (fidelity.x))
            continue;
        sphere.indices.emplace_back(i);
        sphere.indices.emplace_back(i + fidelity.x + 1);
        sphere.indices.emplace_back(i + fidelity.x + 2);
        sphere.indices.emplace_back(i + fidelity.x + 2);
        sphere.indices.emplace_back(i + 1);
        sphere.indices.emplace_back(i);
    }

    sphere.generate_tangents();

    ASSERT(!sphere.indices.empty());
    ASSERT(!sphere.positions.empty());
    ASSERT(!sphere.normals.empty());
    // ASSERT(!sphere.tangents.empty());
    ASSERT(!sphere.tex_coords.empty());
    return sphere;
}

} // namespace hg
