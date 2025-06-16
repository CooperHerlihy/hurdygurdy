#include "hg_mesh.h"

#include "hg_load.h"

namespace hg {

Mesh generate_square() {
    return {
        .indices = {0, 1, 2, 2, 3, 0},
        .vertices =
            {
                ModelVertex{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
                ModelVertex{{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
                ModelVertex{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
                ModelVertex{{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
            },
    };
}

Mesh generate_cube() {
    std::array<Mesh, 6> squares = {};
    for (usize i = 0; i < 6; ++i) {
        squares[i] = generate_square();
    }

    for (auto& vertex : squares[0].vertices) {
        vertex.position.z -= 1.0f;
        vertex.normal = glm::vec3{0.0f, -1.0f, 0.0f};
        vertex.position = glm::angleAxis(glm::pi<f32>() * 0.5f, glm::vec3{-1.0f, 0.0f, 0.0f}) * vertex.position;
    }
    for (auto& vertex : squares[1].vertices) {
        vertex.position.z -= 1.0f;
        vertex.normal = glm::vec3{-1.0f, 0.0f, 0.0f};
        vertex.position = glm::angleAxis(glm::pi<f32>() * 0.5f, glm::vec3{0.0f, 1.0f, 0.0f}) * vertex.position;
    }
    for (auto& vertex : squares[2].vertices) {
        vertex.position.z -= 1.0f;
    }
    for (auto& vertex : squares[3].vertices) {
        vertex.position.z -= 1.0f;
        vertex.normal = glm::vec3{1.0f, 0.0f, 0.0f};
        vertex.position = glm::angleAxis(glm::pi<f32>() * 0.5f, glm::vec3{0.0f, -1.0f, 0.0f}) * vertex.position;
    }
    for (auto& vertex : squares[4].vertices) {
        vertex.position.z -= 1.0f;
        vertex.normal = glm::vec3{0.0f, 0.0f, 1.0f};
        vertex.position = glm::angleAxis(glm::pi<f32>(), glm::vec3{0.0f, 1.0f, 0.0f}) * vertex.position;
    }
    for (auto& vertex : squares[5].vertices) {
        vertex.position.z -= 1.0f;
        vertex.normal = glm::vec3{0.0f, 1.0f, 0.0f};
        vertex.position = glm::angleAxis(glm::pi<f32>() * 0.5f, glm::vec3{1.0f, 0.0f, 0.0f}) * vertex.position;
    }

    Mesh cube = {};
    cube.indices.reserve(6 * 6);
    cube.vertices.reserve(4 * 6);
    for (const auto& square : squares) {
        u32 vertex_count = static_cast<u32>(cube.vertices.size());
        for (const auto index : square.indices) {
            cube.indices.push_back(vertex_count + index);
        }
        for (const auto vertex : square.vertices) {
            cube.vertices.push_back(vertex);
        }
    }

    return cube;
}

Mesh generate_sphere(const i32 fidelity) {
    debug_assert(fidelity >= 3);

    std::vector<hg::ModelVertex> vertices = {};
    vertices.reserve(static_cast<usize>(2 + fidelity * fidelity));

    glm::vec3 point = {0.0f, -1.0f, 0.0f};
    vertices.emplace_back(point, point, glm::vec2{});
    for (i32 i = 0; i < fidelity; ++i) {
        f32 h = -std::cos(glm::pi<f32>() * i / fidelity);
        f32 r = std::sin(glm::pi<f32>() * i / fidelity);
        for (i32 j = 0; j < fidelity; ++j) {
            point = glm::vec3{r * std::cos(glm::tau<f32>() * j / fidelity), h, r * std::sin(glm::tau<f32>() * j / fidelity)};
            vertices.emplace_back(point, point, glm::vec2{});
        }
    }
    point = {0.0f, 1.0f, 0.0f};
    vertices.emplace_back(point, point, glm::vec2{});

    std::vector<u32> indices = {};
    indices.reserve(static_cast<usize>(fidelity * (fidelity + 2) * 3));
    for (i32 i = 0; i < fidelity; ++i) {
        indices.push_back(0);
        indices.push_back((i + 1) % fidelity + 1);
        indices.push_back((i + 2) % fidelity + 1);
    }
    for (i32 i = 1; i <= fidelity * (fidelity - 1); ++i) {
        indices.push_back(i);
        indices.push_back(i + fidelity);
        indices.push_back(i + 1);

        indices.push_back(i + 1);
        indices.push_back(i + fidelity);
        indices.push_back(i + fidelity + 1);
    }
    i32 top_index = 1 + fidelity * fidelity;
    for (i32 i = 0; i < fidelity; ++i) {
        indices.push_back(top_index);
        indices.push_back(top_index - ((i + 1) % fidelity + 1));
        indices.push_back(top_index - ((i + 2) % fidelity + 1));
    }

    return {std::move(indices), std::move(vertices)};
}

} // namespace hg
