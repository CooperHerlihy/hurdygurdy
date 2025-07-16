#include "hg_generate.h"

#include "hg_math.h"

namespace hg {

Generator::MeshHandle Generator::generate_square(MeshHandle mesh) {
    Slice<Vertex> primitives = m_stack.alloc<Vertex>(6);
    defer(m_stack.dealloc(primitives));

    primitives[0] = { {-1.0f, -1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 0.0f,  0.0f}, };
    primitives[1] = { {-1.0f,  1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 0.0f,  1.0f}, };
    primitives[2] = { { 1.0f,  1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 1.0f,  1.0f}, };
    primitives[3] = { { 1.0f,  1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 1.0f,  1.0f}, };
    primitives[4] = { { 1.0f, -1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 1.0f,  0.0f}, };
    primitives[5] = { {-1.0f, -1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 0.0f,  0.0f}, };
    generate_tangents(primitives);

    auto& square = get(mesh);
    square.indices = malloc_slice<u32>(primitives.count);
    square.vertices = malloc_slice<Vertex>(primitives.count);
    int count = weld_mesh(square, primitives);
    square.vertices = realloc_slice(square.vertices, count);

    return mesh;
}

Generator::MeshHandle Generator::generate_cube(MeshHandle mesh) {
    Slice<Vertex> primitives = m_stack.alloc<Vertex>(36);
    defer(m_stack.dealloc(primitives));

    primitives[ 0] = { {-1.0f, -1.0f,  1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {0.0f, 0.0f}, };
    primitives[ 1] = { {-1.0f, -1.0f, -1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {0.0f, 1.0f}, };
    primitives[ 2] = { { 1.0f, -1.0f, -1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {1.0f, 1.0f}, };
    primitives[ 3] = { { 1.0f, -1.0f, -1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {1.0f, 1.0f}, };
    primitives[ 4] = { { 1.0f, -1.0f,  1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {1.0f, 0.0f}, };
    primitives[ 5] = { {-1.0f, -1.0f,  1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {0.0f, 0.0f}, };

    primitives[ 6] = { {-1.0f, -1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}, };
    primitives[ 7] = { {-1.0f,  1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {0.0f, 1.0f}, };
    primitives[ 8] = { {-1.0f,  1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}, };
    primitives[ 9] = { {-1.0f,  1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}, };
    primitives[10] = { {-1.0f, -1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {1.0f, 0.0f}, };
    primitives[11] = { {-1.0f, -1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}, };

    primitives[12] = { {-1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {0.0f, 0.0f}, };
    primitives[13] = { {-1.0f,  1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {0.0f, 1.0f}, };
    primitives[14] = { { 1.0f,  1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {1.0f, 1.0f}, };
    primitives[15] = { { 1.0f,  1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {1.0f, 1.0f}, };
    primitives[16] = { { 1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {1.0f, 0.0f}, };
    primitives[17] = { {-1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {0.0f, 0.0f}, };

    primitives[18] = { { 1.0f, -1.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}, };
    primitives[19] = { { 1.0f,  1.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {0.0f, 1.0f}, };
    primitives[20] = { { 1.0f,  1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}, };
    primitives[21] = { { 1.0f,  1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}, };
    primitives[22] = { { 1.0f, -1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {1.0f, 0.0f}, };
    primitives[23] = { { 1.0f, -1.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}, };

    primitives[24] = { { 1.0f, -1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {0.0f, 0.0f}, };
    primitives[25] = { { 1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {0.0f, 1.0f}, };
    primitives[26] = { {-1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {1.0f, 1.0f}, };
    primitives[27] = { {-1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {1.0f, 1.0f}, };
    primitives[28] = { {-1.0f, -1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {1.0f, 0.0f}, };
    primitives[29] = { { 1.0f, -1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {0.0f, 0.0f}, };

    primitives[30] = { {-1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {0.0f, 0.0f}, };
    primitives[31] = { {-1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {0.0f, 1.0f}, };
    primitives[32] = { { 1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {1.0f, 1.0f}, };
    primitives[33] = { { 1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {1.0f, 1.0f}, };
    primitives[34] = { { 1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {1.0f, 0.0f}, };
    primitives[35] = { {-1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {0.0f, 0.0f}, };
    generate_tangents(primitives);

    auto& cube = get(mesh);
    cube.indices = malloc_slice<u32>(primitives.count);
    cube.vertices = malloc_slice<Vertex>(primitives.count);
    int count = weld_mesh(cube, primitives);
    cube.vertices = realloc_slice(cube.vertices, count);

    return mesh;
}

Generator::MeshHandle Generator::generate_sphere(MeshHandle mesh, const glm::uvec2 fidelity) {
    ASSERT(fidelity.x >= 3);
    ASSERT(fidelity.y >= 2);

    Slice<Vertex> primitives = m_stack.alloc<Vertex>((fidelity.x + 1) * (fidelity.y + 1) * 6);
    defer(m_stack.dealloc(primitives));

    usize index = 0;
    f32 h_next = -1.0f;
    f32 r_next = 0.0f;
    f32 x_next = 0.0f;
    f32 y_next = 1.0f;
    for (u32 i = 0; i < fidelity.y; ++i) {
        const f32 h = h_next;
        const f32 r = r_next;
        h_next = -std::cos(glm::pi<f32>() * (i + 1) / fidelity.y);
        r_next = std::sin(glm::pi<f32>() * (i + 1) / fidelity.y);

        for (u32 j = 0; j < fidelity.x; ++j) {
            const f32 x = x_next;
            const f32 y = y_next;
            x_next = -std::sin(glm::tau<f32>() * (j + 1) / fidelity.x);
            y_next = std::cos(glm::tau<f32>() * (j + 1) / fidelity.x);

            const f32 u = static_cast<f32>(j) / fidelity.x;
            const f32 v = static_cast<f32>(i) / fidelity.y;
            const f32 u_next = static_cast<f32>(j + 1) / fidelity.x;
            const f32 v_next = static_cast<f32>(i + 1) / fidelity.y;

            primitives[index++] = {
                glm::vec3{x * r, h, y * r},
                glm::vec3{x * r, h, y * r},
                glm::vec4{},
                glm::vec2{u, v}
            };
            primitives[index++] = {
                glm::vec3{x * r_next, h_next, y * r_next},
                glm::vec3{x * r_next, h_next, y * r_next},
                glm::vec4{},
                glm::vec2{u, v_next}
            };
            primitives[index++] = {
                glm::vec3{x_next * r_next, h_next, y_next * r_next},
                glm::vec3{x_next * r_next, h_next, y_next * r_next},
                glm::vec4{},
                glm::vec2{u_next, v_next}
            };
            primitives[index++] = {
                glm::vec3{x_next * r_next, h_next, y_next * r_next},
                glm::vec3{x_next * r_next, h_next, y_next * r_next},
                glm::vec4{},
                glm::vec2{u_next, v_next}
            };
            primitives[index++] = {
                glm::vec3{x_next * r, h, y_next * r},
                glm::vec3{x_next * r, h, y_next * r},
                glm::vec4{},
                glm::vec2{u_next, v}
            };
            primitives[index++] = {
                glm::vec3{x * r, h, y * r},
                glm::vec3{x * r, h, y * r},
                glm::vec4{},
                glm::vec2{u, v}
            };
        }
    }
    generate_tangents(primitives);

    auto& sphere = get(mesh);
    sphere.indices = malloc_slice<u32>(primitives.count);
    sphere.vertices = malloc_slice<Vertex>(primitives.count);
    int count = weld_mesh(sphere, primitives);
    sphere.vertices = realloc_slice(sphere.vertices, count);

    return mesh;
}

f32 get_value_noise(const glm::vec<2, usize> pos, const f32 point_width) {
    const f32 x = pos.x / point_width;
    const f32 y = pos.y / point_width;
    const f32 x_t = x - std::floor(x);
    const f32 y_t = y - std::floor(y);
    const usize x_floor = static_cast<usize>(x);
    const usize y_floor = static_cast<usize>(y);
    const usize x_ceil = x_floor + 1;
    const usize y_ceil = y_floor + 1;
    return std::lerp(
        std::lerp(
            rng<f32>(x_floor, y_floor),
            rng<f32>(x_ceil , y_floor),
            smoothstep(x_t)
        ),
        std::lerp(
            rng<f32>(x_floor, y_ceil),
            rng<f32>(x_ceil , y_ceil),
            smoothstep(x_t)
        ),
        smoothstep(y_t)
    );
}

f32 get_perlin_noise(const glm::vec<2, usize> pos, const f32 gradient_width) {
    const f32 x = pos.x / gradient_width;
    const f32 y = pos.y / gradient_width;
    const f32 x_t = x - std::floor(x);
    const f32 y_t = y - std::floor(y);
    const usize x_floor = static_cast<usize>(x);
    const usize y_floor = static_cast<usize>(y);
    const usize x_ceil = x_floor + 1;
    const usize y_ceil = y_floor + 1;
    return std::lerp(
        std::lerp(
            glm::dot(rng<glm::vec2>(x_floor, y_floor), glm::vec2{x_t       , y_t}),
            glm::dot(rng<glm::vec2>(x_ceil , y_floor), glm::vec2{x_t - 1.0f, y_t}),
            smoothstep_quintic(x_t)
        ),
        std::lerp(
            glm::dot(rng<glm::vec2>(x_floor, y_ceil), glm::vec2{x_t       , y_t - 1.0f}),
            glm::dot(rng<glm::vec2>(x_ceil , y_ceil), glm::vec2{x_t - 1.0f, y_t - 1.0f}),
            smoothstep_quintic(x_t)
        ),
        smoothstep_quintic(y_t)
    );
}

glm::vec4 get_normal_from_heightmap(const glm::vec<2, usize> pos, const Image<f32>& heightmap) {
    const f32 height = heightmap[pos.y][pos.x];

    const usize v_up = pos.y == 0 ? heightmap.size.y - 1 : pos.y - 1;
    const usize u_left = pos.x == 0 ? heightmap.size.x - 1 : pos.x - 1;
    const glm::vec3 up  = {0.0f, -1.0f, heightmap[v_up][pos.x] - height};
    const glm::vec3 left  = {-1.0f, 0.0f, heightmap[pos.y][u_left] - height};

    const usize v_down = (pos.y + 1) % heightmap.size.y;
    const usize u_right = (pos.x + 1) % heightmap.size.x;
    const glm::vec3 down = {0.0f, 1.0f, heightmap[v_down][pos.x] - height};
    const glm::vec3 right  = {1.0f, 0.0f, heightmap[pos.y][u_right] - height};

    return glm::vec4{glm::normalize(glm::cross(up, left) + glm::cross(down, right)), 0.0f};
}

// Image<f32> generate_fractal_value_noise(
//     const glm::vec<2, usize> size, const glm::vec<2, usize> initial_size, const usize max_octaves
// ) {
//     ASSERT(size.x > initial_size.x && size.y > initial_size.y);
//     ASSERT(initial_size.x > 0 && initial_size.y > 0);
//     ASSERT(max_octaves > 0);
//
//     Image<f32> image{size};
//     auto octave_size = initial_size;
//     const usize octaves = std::min(
//         max_octaves, static_cast<usize>(
//             std::floor(std::log2(std::min(
//                 static_cast<f32>(size.x) / static_cast<f32>(initial_size.x),
//                 static_cast<f32>(size.y) / static_cast<f32>(initial_size.y)
//             )))
//         )
//     );
//     const f32 divisions = std::exp2f(static_cast<f32>(octaves)) - 1.0f;
//     f32 amplitude = std::floor(divisions / 2.0f) / divisions;
//     for (usize i = 0; i < octaves; ++i, octave_size *= 2, amplitude *= 0.5f) {
//         image += generate_value_noise(size,
//             generate_white_noise<f32>(
//                 octave_size,
//                 [amplitude]() -> f32 { return rng<f32>() * amplitude; }
//             )
//         );
//     }
//     return image;
// }

// Image<f32> generate_fractal_perlin_noise(
//     const glm::vec<2, usize> size, const glm::vec<2, usize> initial_size, const usize max_octaves
// ) {
//     ASSERT(size.x > initial_size.x && size.y > initial_size.y);
//     ASSERT(initial_size.x > 0 && initial_size.y > 0);
//     ASSERT(max_octaves > 0);
//
//     Image<f32> image{size};
//     auto octave_size = initial_size;
//     const usize octaves = std::min(
//         max_octaves, static_cast<usize>(
//             std::floor(std::log2(std::min(
//                 static_cast<f32>(size.x) / static_cast<f32>(initial_size.x),
//                 static_cast<f32>(size.y) / static_cast<f32>(initial_size.y)
//             )))
//         )
//     );
//     const f32 divisions = std::exp2f(static_cast<f32>(octaves)) - 1.0f;
//     f32 amplitude = std::floor(divisions / 2.0f) / divisions;
//     for (usize i = 0; i < octaves; ++i, octave_size *= 2, amplitude *= 0.5f) {
//         image += transform_image(
//             generate_perlin_noise(size, generate_white_noise<glm::vec2>(octave_size)),
//             [amplitude](const f32 val) -> f32 { return val * amplitude; }
//         );
//     }
//     return image;
// }

} // namespace hg
