#include "hg_generate.h"

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

[[nodiscard]] Image<f32> generate_value_noise(const glm::vec<2, usize>& size, const Image<f32>& fixed_points) {
    ASSERT(fixed_points.width() < size.x);
    ASSERT(fixed_points.height() < size.y);
    Image<f32> interpolated = size;
    for (usize v = 0; v < size.y; ++v) {
        for (usize u = 0; u < size.x; ++u) {
            const f64 x = static_cast<f64>(u) * static_cast<f64>(fixed_points.width()) / static_cast<f64>(size.x);
            const f64 y = static_cast<f64>(v) * static_cast<f64>(fixed_points.height()) / static_cast<f64>(size.y);
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
} // namespace hg
