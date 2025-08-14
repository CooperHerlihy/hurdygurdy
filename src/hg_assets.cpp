#include "hg_assets.h"

#include "hg_math.h"

#include <stb/stb_image.h>

#define FASTGLTF_COMPILE_AS_CPP20
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <mikktspace/mikktspace.h>
#include <welder/weldmesh.h>

namespace hg {

void generate_vertex_tangents(Slice<Vertex> primitives) {
    ASSERT(primitives.count % 3 == 0);

    SMikkTSpaceInterface mikk_functions{
        .m_getNumFaces = [](const SMikkTSpaceContext* pContext) {
            return static_cast<int>(static_cast<Slice<Vertex>*>(pContext->m_pUserData)->count / 3);
        },
        .m_getNumVerticesOfFace = [](const SMikkTSpaceContext*, const int) {
            return 3;
        },
        .m_getPosition = [](const SMikkTSpaceContext * pContext, float fvPosOut[], const int iFace, const int iVert) {
            glm::vec3 pos = (*static_cast<Slice<Vertex>*>(pContext->m_pUserData))[iFace * 3 + iVert].position;
            fvPosOut[0] = pos.x;
            fvPosOut[1] = pos.y;
            fvPosOut[2] = pos.z;
        },
        .m_getNormal = [](const SMikkTSpaceContext * pContext, float fvNormOut[], const int iFace, const int iVert) {
            glm::vec3 normal = (*static_cast<Slice<Vertex>*>(pContext->m_pUserData))[iFace * 3 + iVert].normal;
            fvNormOut[0] = normal.x;
            fvNormOut[1] = normal.y;
            fvNormOut[2] = normal.z;
        } ,
        .m_getTexCoord = [](const SMikkTSpaceContext * pContext, float fvTexcOut[], const int iFace, const int iVert) {
            glm::vec2 tex = (*static_cast<Slice<Vertex>*>(pContext->m_pUserData))[iFace * 3 + iVert].tex_coord;
            fvTexcOut[0] = tex.x;
            fvTexcOut[1] = tex.y;
        },
        .m_setTSpaceBasic = [](
            const SMikkTSpaceContext * pContext,
            const float fvTangent[],
            const float fSign,
            const int iFace,
            const int iVert
        ) {
            Slice<Vertex>& vertices = *static_cast<Slice<Vertex>*>(pContext->m_pUserData);
            vertices[iFace * 3 + iVert].tangent = {fvTangent[0], fvTangent[1], fvTangent[2], fSign};
        },
        .m_setTSpace = nullptr,
    };
    SMikkTSpaceContext mikk_context{
        .m_pInterface = &mikk_functions,
        .m_pUserData = &primitives,
    };
    genTangSpaceDefault(&mikk_context);
}

void weld_mesh(MeshData& out_mesh, int& out_index_count, Slice<const Vertex> primitives) {
    ASSERT(out_mesh.vertices.count >= primitives.count);
    ASSERT(out_mesh.indices.count >= primitives.count);
    ASSERT(primitives.count % 3 == 0);

    out_index_count = WeldMesh(
        reinterpret_cast<int*>(out_mesh.indices.data),
        reinterpret_cast<float*>(out_mesh.vertices.data),
        reinterpret_cast<const float*>(primitives.data),
        static_cast<i32>(primitives.count),
        sizeof(Vertex) / sizeof(float)
    );
}

AssetManager create_asset_manager(const AssetMangagerConfig& config) {
    return {
        .meshes = malloc_pool<MeshData>(config.max_meshes),
        .images = malloc_pool<ImageData>(config.max_images),
        .stack = malloc_slice<byte>(config.stack_size),
    };
}

void destroy_asset_manager(AssetManager& assets) {
    free_pool(assets.meshes);
    free_pool(assets.images);
    free_arena(assets.stack);
}

MeshHandle create_mesh(AssetManager& assets) {
    return assets.meshes.alloc();
}

void destroy_mesh(AssetManager& assets, const MeshHandle mesh) {
    MeshData& mesh_data = assets[mesh];
    free_slice(mesh_data.vertices);
    free_slice(mesh_data.indices);
    assets.meshes.dealloc(mesh);
}

MeshHandle generate_square(AssetManager& assets) {
    Slice<Vertex> primitives = assets.stack.alloc<Vertex>(6);
    DEFER(assets.stack.dealloc(primitives));

    primitives[0] = {{-1.0f, -1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 0.0f,  0.0f}};
    primitives[1] = {{-1.0f,  1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 0.0f,  1.0f}};
    primitives[2] = {{ 1.0f,  1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 1.0f,  1.0f}};
    primitives[3] = {{ 1.0f,  1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 1.0f,  1.0f}};
    primitives[4] = {{ 1.0f, -1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 1.0f,  0.0f}};
    primitives[5] = {{-1.0f, -1.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {}, { 0.0f,  0.0f}};
    generate_vertex_tangents(primitives);

    MeshHandle mesh = create_mesh(assets);
    auto& square = assets[mesh];
    square = {
        .indices = malloc_slice<u32>(primitives.count),
        .vertices = malloc_slice<Vertex>(primitives.count),
    };
    int count;
    weld_mesh(square, count, primitives);
    square.vertices = realloc_slice(square.vertices, count);

    return mesh;
}

MeshHandle generate_cube(AssetManager& assets) {
    Slice<Vertex> primitives = assets.stack.alloc<Vertex>(36);
    DEFER(assets.stack.dealloc(primitives));

    primitives[ 0] = {{-1.0f, -1.0f,  1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {0.0f, 0.0f}};
    primitives[ 1] = {{-1.0f, -1.0f, -1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {0.0f, 1.0f}};
    primitives[ 2] = {{ 1.0f, -1.0f, -1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {1.0f, 1.0f}};
    primitives[ 3] = {{ 1.0f, -1.0f, -1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {1.0f, 1.0f}};
    primitives[ 4] = {{ 1.0f, -1.0f,  1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {1.0f, 0.0f}};
    primitives[ 5] = {{-1.0f, -1.0f,  1.0f}, { 0.0f, -1.0f,  0.0f}, {}, {0.0f, 0.0f}};

    primitives[ 6] = {{-1.0f, -1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}};
    primitives[ 7] = {{-1.0f,  1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {0.0f, 1.0f}};
    primitives[ 8] = {{-1.0f,  1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}};
    primitives[ 9] = {{-1.0f,  1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}};
    primitives[10] = {{-1.0f, -1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {1.0f, 0.0f}};
    primitives[11] = {{-1.0f, -1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}};

    primitives[12] = {{-1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {0.0f, 0.0f}};
    primitives[13] = {{-1.0f,  1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {0.0f, 1.0f}};
    primitives[14] = {{ 1.0f,  1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {1.0f, 1.0f}};
    primitives[15] = {{ 1.0f,  1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {1.0f, 1.0f}};
    primitives[16] = {{ 1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {1.0f, 0.0f}};
    primitives[17] = {{-1.0f, -1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {}, {0.0f, 0.0f}};

    primitives[18] = {{ 1.0f, -1.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}};
    primitives[19] = {{ 1.0f,  1.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {0.0f, 1.0f}};
    primitives[20] = {{ 1.0f,  1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}};
    primitives[21] = {{ 1.0f,  1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {1.0f, 1.0f}};
    primitives[22] = {{ 1.0f, -1.0f,  1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {1.0f, 0.0f}};
    primitives[23] = {{ 1.0f, -1.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {}, {0.0f, 0.0f}};

    primitives[24] = {{ 1.0f, -1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {0.0f, 0.0f}};
    primitives[25] = {{ 1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {0.0f, 1.0f}};
    primitives[26] = {{-1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {1.0f, 1.0f}};
    primitives[27] = {{-1.0f,  1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {1.0f, 1.0f}};
    primitives[28] = {{-1.0f, -1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {1.0f, 0.0f}};
    primitives[29] = {{ 1.0f, -1.0f,  1.0f}, { 0.0f,  0.0f,  1.0f}, {}, {0.0f, 0.0f}};

    primitives[30] = {{-1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {0.0f, 0.0f}};
    primitives[31] = {{-1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {0.0f, 1.0f}};
    primitives[32] = {{ 1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {1.0f, 1.0f}};
    primitives[33] = {{ 1.0f,  1.0f,  1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {1.0f, 1.0f}};
    primitives[34] = {{ 1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {1.0f, 0.0f}};
    primitives[35] = {{-1.0f,  1.0f, -1.0f}, { 0.0f,  1.0f,  0.0f}, {}, {0.0f, 0.0f}};
    generate_vertex_tangents(primitives);

    MeshHandle mesh = create_mesh(assets);
    auto& cube = assets[mesh];
    cube = {
        .indices = malloc_slice<u32>(primitives.count),
        .vertices = malloc_slice<Vertex>(primitives.count),
    };
    int count;
    weld_mesh(cube, count, primitives);
    cube.vertices = realloc_slice(cube.vertices, count);

    return mesh;
}

MeshHandle generate_sphere(AssetManager& assets, const glm::uvec2 fidelity) {
    ASSERT(fidelity.x >= 3);
    ASSERT(fidelity.y >= 2);

    Slice<Vertex> primitives = assets.stack.alloc<Vertex>((fidelity.x + 1) * (fidelity.y + 1) * 6);
    DEFER(assets.stack.dealloc(primitives));

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
    generate_vertex_tangents(primitives);

    MeshHandle mesh = create_mesh(assets);
    auto& sphere = assets[mesh];
    sphere = {
        .indices = malloc_slice<u32>(primitives.count),
        .vertices = malloc_slice<Vertex>(primitives.count),
    };
    int count;
    weld_mesh(sphere, count, primitives);
    sphere.vertices = realloc_slice(sphere.vertices, count);

    return mesh;
}

Result<GltfData> load_gltf(AssetManager& assets, std::filesystem::path path) {
    ASSERT(!path.empty());

    fastgltf::Parser parser;

    auto buffer = fastgltf::GltfDataBuffer::FromPath(path);
    switch (buffer.error()) {
        case fastgltf::Error::None: break;
        case fastgltf::Error::InvalidPath: {
            LOGF_ERROR("Gltf file not found; invalid path: {}", path.string());
            return Err::GltfFileNotFound;
        }
        default: {
            LOGF_ERROR("Gltf file invalid; could not load: {}", path.string());
            return Err::GltfFileInvalid;
        }
    }

    const auto options = fastgltf::Options::DecomposeNodeMatrices
                       | fastgltf::Options::GenerateMeshIndices
                       // | fastgltf::Options::LoadExternalImages
                       | fastgltf::Options::LoadExternalBuffers;
    auto asset = parser.loadGltf(buffer.get(), path.parent_path(), options);
    switch (asset.error()) {
        case fastgltf::Error::None: break;
        default: {
            LOGF_ERROR("Gltf file invalid; could not parse: {}", path.string());
            return Err::GltfFileInvalid;
        }
    }

    f32 roughness = 0.5f;
    f32 metalness = 0.0f;
    Slice<Vertex> primitives{};
    DEFER(free_slice(primitives));
    for (const auto& mesh : asset->meshes) {
        for (const auto& primitive : mesh.primitives) {
            if (primitive.materialIndex.has_value()) {
                roughness = asset->materials[*primitive.materialIndex].pbrData.roughnessFactor;
                metalness = asset->materials[*primitive.materialIndex].pbrData.metallicFactor;
            }

            if (!primitive.indicesAccessor.has_value()) {
                LOGF_ERROR("Gltf file invalid; primitive has no indices accessor: {}", path.string());
                return Err::GltfFileInvalid;
            }

            auto& index_accessor = asset->accessors[primitive.indicesAccessor.value()];
            auto& position_accessor = asset->accessors[primitive.findAttribute("POSITION")->accessorIndex];
            auto& normal_accessor = asset->accessors[primitive.findAttribute("NORMAL")->accessorIndex];
            auto& tex_coord_accessor = asset->accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];

            if (index_accessor.count % 3 != 0) {
                LOGF_ERROR("Gltf file invalid; primitives are not a multiple of 3: {}", path.string());
                return Err::GltfFileInvalid;
            }
            if (!position_accessor.bufferViewIndex.has_value()) {
                LOGF_ERROR("Gltf file invalid; primitive has no position accessor: {}", path.string());
                return Err::GltfFileInvalid;
            }
            if (position_accessor.type != fastgltf::AccessorType::Vec3) {
                LOGF_ERROR("Gltf file invalid; primitive has invalid position accessor: {}", path.string());
                return Err::GltfFileInvalid;
            }
            if (!normal_accessor.bufferViewIndex.has_value()) {
                LOGF_ERROR("Gltf file invalid; primitive has no normal accessor: {}", path.string());
                return Err::GltfFileInvalid;
            }
            if (normal_accessor.type != fastgltf::AccessorType::Vec3) {
                LOGF_ERROR("Gltf file invalid; primitive has invalid normal accessor: {}", path.string());
                return Err::GltfFileInvalid;
            }
            if (!tex_coord_accessor.bufferViewIndex.has_value()) {
                LOGF_ERROR("Gltf file invalid; primitive has no tex coord accessor: {}", path.string());
                return Err::GltfFileInvalid;
            }
            if (tex_coord_accessor.type != fastgltf::AccessorType::Vec2) {
                LOGF_ERROR("Gltf file invalid; primitive has invalid tex coord accessor: {}", path.string());
                return Err::GltfFileInvalid;
            }

            usize begin = primitives.count;
            primitives = realloc_slice(primitives, primitives.count + index_accessor.count);

            fastgltf::iterateAccessorWithIndex<u32>(asset.get(), index_accessor, [&](u32 index, usize i) { 
                primitives[begin + i] = {
                    fastgltf::getAccessorElement<glm::vec3>(asset.get(), position_accessor, index)
                        * glm::vec3{1.0f, -1.0f, -1.0f},
                    fastgltf::getAccessorElement<glm::vec3>(asset.get(), normal_accessor, index)
                        * glm::vec3{1.0f, -1.0f, -1.0f},
                    glm::vec4{},
                    fastgltf::getAccessorElement<glm::vec2>(asset.get(), tex_coord_accessor, index)
                };
            });
        }
    }
    generate_vertex_tangents(primitives);

    auto gltf = ok<GltfData>(create_mesh(assets), roughness, metalness);

    MeshData& mesh = assets[gltf->mesh];
    mesh = {
        .indices = malloc_slice<u32>(primitives.count),
        .vertices = malloc_slice<Vertex>(primitives.count),
    };
    int count;
    weld_mesh(mesh, count, primitives);
    mesh.vertices = realloc_slice(mesh.vertices, count);

    return gltf;
}

void unload_gltf(AssetManager& assets, const GltfData gltf) {
    destroy_mesh(assets, gltf.mesh);
}

ImageHandle<void> create_image(AssetManager& assets) {
    return {assets.images.alloc()};
}

ImageHandle<void> create_image(AssetManager& assets, glm::vec<2, usize> size, u32 alignment) {
    ImageHandle<void> image = create_image(assets);
    assets[image] = {
        .pixels = malloc_slice<byte>(size.x * size.y * alignment).data,
        .alignment = alignment,
        .size = size,
    };
    return image;
}

void destroy_image(AssetManager& assets, const ImageHandle<void> image) {
    std::free(assets[image].pixels);
    assets.images.dealloc(image.handle);
}

Result<ImageHandle<u32>> load_image(AssetManager& assets, std::filesystem::path path) {
    ASSERT(!path.empty());

    int width = 0, height = 0, channels = 0;
    const auto pixels = stbi_load(path.string().data(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        LOGF_WARN("Image file not found: {}", path.string());
        return Err::ImageFileNotFound;
    }
    if (width <= 0 || height <= 0 || channels <= 0) {
        LOGF_WARN("Image file invalid; width, height, and/or channels are zero: {}", path.string());
        std::free(pixels);
        return Err::ImageFileInvalid;
    }

    auto image = ok(create_image<u32>(assets));
    assets[*image] = {
        .pixels = pixels,
        .alignment = 4,
        .size = {to_u32(width), to_u32(height)},
    };
    return image;
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

} // namespace hg
