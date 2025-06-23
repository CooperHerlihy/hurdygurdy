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
    Mesh cube = {};

    std::array<Mesh, 6> squares = {};
    for (usize i = 0; i < 6; ++i)
        squares[i] = generate_square();

    for (auto& normal : squares[0].normals) {
        normal = {0.0f, -1.0f, 0.0f};
    }
    for (auto& normal : squares[1].normals) {
        normal = {-1.0f, 0.0f, 0.0f};
    }
    for (auto& normal : squares[2].normals) {
        normal = {0.0f, 0.0f, -1.0f};
    }
    for (auto& normal : squares[3].normals) {
        normal = {1.0f, 0.0f, 0.0f};
    }
    for (auto& normal : squares[4].normals) {
        normal = {0.0f, 0.0f, 1.0f};
    }
    for (auto& normal : squares[5].normals) {
        normal = {0.0f, 1.0f, 0.0f};
    }

    for (auto& position : squares[0].positions) {
        position.z = -1.0f;
        position = glm::angleAxis(glm::pi<f32>() * 0.5f, glm::vec3{-1.0f, 0.0f, 0.0f}) * position;
    }
    for (auto& position : squares[1].positions) {
        position.z = -1.0f;
        position = glm::angleAxis(glm::pi<f32>() * 0.5f, glm::vec3{0.0f, 1.0f, 0.0f}) * position;
    }
    for (auto& position : squares[2].positions) {
        position.z = -1.0f;
    }
    for (auto& position : squares[3].positions) {
        position.z = -1.0f;
        position = glm::angleAxis(glm::pi<f32>() * 0.5f, glm::vec3{0.0f, -1.0f, 0.0f}) * position;
    }
    for (auto& position : squares[4].positions) {
        position.z = -1.0f;
        position = glm::angleAxis(glm::pi<f32>(), glm::vec3{0.0f, 1.0f, 0.0f}) * position;
    }
    for (auto& position : squares[5].positions) {
        position.z = -1.0f;
        position = glm::angleAxis(glm::pi<f32>() * 0.5f, glm::vec3{1.0f, 0.0f, 0.0f}) * position;
    }

    cube.indices.reserve(36);
    cube.positions.reserve(24);
    cube.normals.reserve(24);
    cube.tex_coords.reserve(24);
    for (const auto& square : squares) {
        u32 vertex_count = to_u32(cube.positions.size());
        for (const auto index : square.indices) {
            cube.indices.push_back(vertex_count + index);
        }
        for (const auto position : square.positions) {
            cube.positions.push_back(position);
        }
        for (const auto normal : square.normals) {
            cube.normals.push_back(normal);
        }
        for (const auto tex_coord : square.tex_coords) {
            cube.tex_coords.push_back(tex_coord);
        }
    }

    ASSERT(cube.indices.size() == 36);
    ASSERT(cube.positions.size() == 24);
    ASSERT(cube.normals.size() == 24);
    ASSERT(cube.tex_coords.size() == 24);
    return cube;
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
