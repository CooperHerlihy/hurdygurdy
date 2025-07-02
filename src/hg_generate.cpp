#include "hg_generate.h"

#include "hg_math.h"

#include <mikktspace/mikktspace.h>
#include <welder/weldmesh.h>

namespace hg {

void create_tangents(std::span<Vertex> primitives) {
    ASSERT(primitives.size() % 3 == 0);

    SMikkTSpaceInterface mikk_functions = {
        .m_getNumFaces = [](const SMikkTSpaceContext* pContext) {
            return static_cast<int>(static_cast<std::span<Vertex>*>(pContext->m_pUserData)->size() / 3);
        },
        .m_getNumVerticesOfFace = [](const SMikkTSpaceContext*, const int) {
            return 3;
        },
        .m_getPosition = [](const SMikkTSpaceContext * pContext, float fvPosOut[], const int iFace, const int iVert) {
            glm::vec3 pos = (*static_cast<std::span<Vertex>*>(pContext->m_pUserData))[iFace * 3 + iVert].position;
            fvPosOut[0] = pos.x;
            fvPosOut[1] = pos.y;
            fvPosOut[2] = pos.z;
        },
        .m_getNormal = [](const SMikkTSpaceContext * pContext, float fvNormOut[], const int iFace, const int iVert) {
            glm::vec3 normal = (*static_cast<std::span<Vertex>*>(pContext->m_pUserData))[iFace * 3 + iVert].normal;
            fvNormOut[0] = normal.x;
            fvNormOut[1] = normal.y;
            fvNormOut[2] = normal.z;
        } ,
        .m_getTexCoord = [](const SMikkTSpaceContext * pContext, float fvTexcOut[], const int iFace, const int iVert) {
            glm::vec2 tex = (*static_cast<std::span<Vertex>*>(pContext->m_pUserData))[iFace * 3 + iVert].tex_coord;
            fvTexcOut[0] = tex.x;
            fvTexcOut[1] = tex.y;
        },
        .m_setTSpaceBasic = [](const SMikkTSpaceContext * pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert) {
            (*static_cast<std::span<Vertex>*>(pContext->m_pUserData))[iFace * 3 + iVert].tangent = {fvTangent[0], fvTangent[1], fvTangent[2], fSign};
        },
        .m_setTSpace = nullptr,
    };
    SMikkTSpaceContext mikk_context = {
        .m_pInterface = &mikk_functions,
        .m_pUserData = reinterpret_cast<void*>(&primitives),
    };
    genTangSpaceDefault(&mikk_context);
}

Mesh Mesh::from_primitives(const std::span<const Vertex> primitives) {
    ASSERT(primitives.size() % 3 == 0);

    Mesh mesh = {};
    mesh.indices.resize(primitives.size());
    mesh.vertices.resize(primitives.size());
    WeldMesh(
        reinterpret_cast<int*>(mesh.indices.data()),
        reinterpret_cast<float*>(mesh.vertices.data()),
        reinterpret_cast<const float*>(primitives.data()),
        static_cast<i32>(primitives.size()),
        sizeof(Vertex) / sizeof(float)
    );

    return mesh;
}

Mesh generate_square() {
    std::vector<Vertex> square = {
        { {-1.0f, -1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 0.0f,  0.0f}, },
        { {-1.0f,  1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 0.0f,  1.0f}, },
        { { 1.0f,  1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 1.0f,  1.0f}, },
        { { 1.0f,  1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 1.0f,  1.0f}, },
        { { 1.0f, -1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 1.0f,  0.0f}, },
        { {-1.0f, -1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 0.0f,  0.0f}, },
    };
    create_tangents(square);
    return Mesh::from_primitives(square);
}

Mesh generate_cube() {
    std::vector<Vertex> cube = {
        { {-1.0f, -1.0f,  1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {0.0f, 0.0f}, },
        { {-1.0f, -1.0f, -1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {0.0f, 1.0f}, },
        { { 1.0f, -1.0f, -1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {1.0f, 1.0f}, },
        { { 1.0f, -1.0f, -1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {1.0f, 1.0f}, },
        { { 1.0f, -1.0f,  1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {1.0f, 0.0f}, },
        { {-1.0f, -1.0f,  1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {0.0f, 0.0f}, },

        { {-1.0f, -1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}, },
        { {-1.0f,  1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {0.0f, 1.0f}, },
        { {-1.0f,  1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}, },
        { {-1.0f,  1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}, },
        { {-1.0f, -1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {1.0f, 0.0f}, },
        { {-1.0f, -1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}, },

        { {-1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {0.0f, 0.0f}, },
        { {-1.0f,  1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {0.0f, 1.0f}, },
        { { 1.0f,  1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {1.0f, 1.0f}, },
        { { 1.0f,  1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {1.0f, 1.0f}, },
        { { 1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {1.0f, 0.0f}, },
        { {-1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {0.0f, 0.0f}, },

        { { 1.0f, -1.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}, },
        { { 1.0f,  1.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {0.0f, 1.0f}, },
        { { 1.0f,  1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}, },
        { { 1.0f,  1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}, },
        { { 1.0f, -1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {1.0f, 0.0f}, },
        { { 1.0f, -1.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}, },

        { { 1.0f, -1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {0.0f, 0.0f}, },
        { { 1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {0.0f, 1.0f}, },
        { {-1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {1.0f, 1.0f}, },
        { {-1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {1.0f, 1.0f}, },
        { {-1.0f, -1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {1.0f, 0.0f}, },
        { { 1.0f, -1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {0.0f, 0.0f}, },

        { {-1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {0.0f, 0.0f}, },
        { {-1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {0.0f, 1.0f}, },
        { { 1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {1.0f, 1.0f}, },
        { { 1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {1.0f, 1.0f}, },
        { { 1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {1.0f, 0.0f}, },
        { {-1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {0.0f, 0.0f}, },
    };
    create_tangents(cube);
    return Mesh::from_primitives(cube);
}

Mesh generate_sphere(const glm::uvec2 fidelity) {
    ASSERT(fidelity.x >= 3);
    ASSERT(fidelity.y >= 2);

    std::vector<Vertex> primitives = {};
    primitives.reserve((fidelity.x + 1) * (fidelity.y + 1) * 6);

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

            primitives.emplace_back(
                glm::vec3{x * r, h, y * r},
                glm::vec3{x * r, h, y * r},
                glm::vec4{},
                glm::vec2{u, v}
            );
            primitives.emplace_back(
                glm::vec3{x * r_next, h_next, y * r_next},
                glm::vec3{x * r_next, h_next, y * r_next},
                glm::vec4{},
                glm::vec2{u, v_next}
            );
            primitives.emplace_back(
                glm::vec3{x_next * r_next, h_next, y_next * r_next},
                glm::vec3{x_next * r_next, h_next, y_next * r_next},
                glm::vec4{},
                glm::vec2{u_next, v_next}
            );
            primitives.emplace_back(
                glm::vec3{x_next * r_next, h_next, y_next * r_next},
                glm::vec3{x_next * r_next, h_next, y_next * r_next},
                glm::vec4{},
                glm::vec2{u_next, v_next}
            );
            primitives.emplace_back(
                glm::vec3{x_next * r, h, y_next * r},
                glm::vec3{x_next * r, h, y_next * r},
                glm::vec4{},
                glm::vec2{u_next, v}
            );
            primitives.emplace_back(
                glm::vec3{x * r, h, y * r},
                glm::vec3{x * r, h, y * r},
                glm::vec4{},
                glm::vec2{u, v}
            );
        }
    }

    create_tangents(primitives);
    Mesh sphere = Mesh::from_primitives(primitives);

    ASSERT(!sphere.indices.empty());
    ASSERT(!sphere.vertices.empty());
    return sphere;
}

Image<glm::vec4> create_normals_from_heightmap(const Image<f32>& heightmap) {
    Image<glm::vec4> normals = {{heightmap.width(), heightmap.height()}};

    for (usize v = 0; v < heightmap.height(); ++v) {
        for (usize u = 0; u < heightmap.width(); ++u) {
            const f32 height = heightmap[v][u];

            const usize v_up = v == 0 ? heightmap.height() - 1 : v - 1;
            const usize u_left = u == 0 ? heightmap.width() - 1 : u - 1;
            const glm::vec3 up  = {0.0f, -1.0f, heightmap[v_up][u] - height};
            const glm::vec3 left  = {-1.0f, 0.0f, heightmap[v][u_left] - height};

            const usize v_down = (v + 1) % heightmap.height();
            const usize u_right = (u + 1) % heightmap.width();
            const glm::vec3 down = {0.0f, 1.0f, heightmap[v_down][u] - height};
            const glm::vec3 right  = {1.0f, 0.0f, heightmap[v][u_right] - height};

            normals[v][u] = glm::vec4{glm::normalize(glm::cross(up, left) + glm::cross(down, right)), 0.0f};
        }
    }

    return normals;
}

Image<f32> generate_value_noise(const glm::vec<2, usize> size, const Image<f32>& fixed_points) {
    ASSERT(fixed_points.width() < size.x);
    ASSERT(fixed_points.height() < size.y);
    Image<f32> interpolated = size;
    for (usize v = 0; v < size.y; ++v) {
        for (usize u = 0; u < size.x; ++u) {
            const f32 x = static_cast<f32>(u) * static_cast<f32>(fixed_points.width()) / static_cast<f32>(size.x);
            const f32 y = static_cast<f32>(v) * static_cast<f32>(fixed_points.height()) / static_cast<f32>(size.y);
            const usize x_floor = static_cast<usize>(std::floor(x));
            const usize y_floor = static_cast<usize>(std::floor(y));
            const usize x_ceil = (x_floor + 1) % fixed_points.width();
            const usize y_ceil = (y_floor + 1) % fixed_points.height();
            interpolated[v][u] = lerp(
                lerp(
                    fixed_points[y_floor][x_floor],
                    fixed_points[y_floor][x_ceil],
                    smoothstep(static_cast<f32>(x - std::floor(x)))
                ),
                lerp(
                    fixed_points[y_ceil][x_floor],
                    fixed_points[y_ceil][x_ceil],
                    smoothstep(static_cast<f32>(x - std::floor(x)))
                ),
                smoothstep(static_cast<f32>(y - std::floor(y)))
            );
        }
    }
    return interpolated;
}

Image<f32> generate_perlin_noise(const glm::vec<2, usize> size, const Image<glm::vec2>& gradients) {
    ASSERT(gradients.width() < size.x);
    ASSERT(gradients.height() < size.y);
    Image<f32> interpolated = size;
    for (usize v = 0; v < size.y; ++v) {
        for (usize u = 0; u < size.x; ++u) {
            const f32 x = static_cast<f32>(u) * static_cast<f32>(gradients.width()) / static_cast<f32>(size.x);
            const f32 y = static_cast<f32>(v) * static_cast<f32>(gradients.height()) / static_cast<f32>(size.y);
            const f32 x_t = x - std::floor(x);
            const f32 y_t = y - std::floor(y);
            const usize x_floor = static_cast<usize>(std::floor(x));
            const usize y_floor = static_cast<usize>(std::floor(y));
            const usize x_ceil = (x_floor + 1) % gradients.width();
            const usize y_ceil = (y_floor + 1) % gradients.height();

            interpolated[v][u] = lerp(
                lerp(
                    glm::dot(gradients[y_floor][x_floor], glm::vec2{x_t       , y_t}),
                    glm::dot(gradients[y_floor][x_ceil] , glm::vec2{x_t - 1.0f, y_t}),
                    smoothstep_quintic(x_t)
                ),
                lerp(
                    glm::dot(gradients[y_ceil][x_floor], glm::vec2{x_t       , y_t - 1.0f}),
                    glm::dot(gradients[y_ceil][x_ceil] , glm::vec2{x_t - 1.0f, y_t - 1.0f}),
                    smoothstep_quintic(x_t)
                ),
                smoothstep_quintic(y_t)
            ) * 0.5f + 0.5f;
        }
    }
    return interpolated;
}

Image<f32> generate_fractal_value_noise(
    const glm::vec<2, usize> size, const glm::vec<2, usize> initial_size, const usize max_octaves
) {
    ASSERT(size.x > initial_size.x && size.y > initial_size.y);
    ASSERT(initial_size.x > 0 && initial_size.y > 0);
    ASSERT(max_octaves > 0);

    Image<f32> image = size;
    auto octave_size = initial_size;
    const usize octaves = std::min(
        max_octaves, static_cast<usize>(
            std::floor(std::log2(std::min(
                static_cast<f32>(size.x) / static_cast<f32>(initial_size.x),
                static_cast<f32>(size.y) / static_cast<f32>(initial_size.y)
            )))
        )
    );
    const f32 divisions = std::exp2f(static_cast<f32>(octaves)) - 1.0f;
    f32 amplitude = std::floor(divisions / 2.0f) / divisions;
    for (usize i = 0; i < octaves; ++i, octave_size *= 2, amplitude *= 0.5f) {
        image += generate_value_noise(size,
            generate_white_noise<f32>(
                octave_size,
                [amplitude]() -> f32 { return rng<f32>() * amplitude; }
            )
        );
    }
    return image;
}

Image<f32> generate_fractal_perlin_noise(
    const glm::vec<2, usize> size, const glm::vec<2, usize> initial_size, const usize max_octaves
) {
    ASSERT(size.x > initial_size.x && size.y > initial_size.y);
    ASSERT(initial_size.x > 0 && initial_size.y > 0);
    ASSERT(max_octaves > 0);

    Image<f32> image = size;
    auto octave_size = initial_size;
    const usize octaves = std::min(
        max_octaves, static_cast<usize>(
            std::floor(std::log2(std::min(
                static_cast<f32>(size.x) / static_cast<f32>(initial_size.x),
                static_cast<f32>(size.y) / static_cast<f32>(initial_size.y)
            )))
        )
    );
    const f32 divisions = std::exp2f(static_cast<f32>(octaves)) - 1.0f;
    f32 amplitude = std::floor(divisions / 2.0f) / divisions;
    for (usize i = 0; i < octaves; ++i, octave_size *= 2, amplitude *= 0.5f) {
        image += transform_image(
            generate_perlin_noise(size, generate_white_noise<glm::vec2>(octave_size)),
            [amplitude](const f32 val) -> f32 { return val * amplitude; }
        );
    }
    return image;
}

} // namespace hg
