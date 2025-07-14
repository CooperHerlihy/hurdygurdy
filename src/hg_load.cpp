#include "hg_load.h"

#include <stb/stb_image.h>

#define FASTGLTF_COMPILE_AS_CPP20
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <mikktspace/mikktspace.h>
#include <welder/weldmesh.h>

#include <filesystem>

namespace hg {

Result<ImageLoader::Handle> ImageLoader::load(const std::filesystem::path path) {
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

    const auto image = m_pool.alloc();
    m_pool[image] = {
        .pixels = pixels,
        .alignment = 4,
        .size = {to_u32(width), to_u32(height), 1},
    };
    return ok<Handle>(image);
}

void ImageLoader::unload(const Handle image) {
    std::free(get(image).pixels);
    m_pool.dealloc(image.handle);
}

void create_tangents(std::span<Vertex> primitives) {
    ASSERT(primitives.size() % 3 == 0);

    SMikkTSpaceInterface mikk_functions{
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
    SMikkTSpaceContext mikk_context{
        .m_pInterface = &mikk_functions,
        .m_pUserData = reinterpret_cast<void*>(&primitives),
    };
    genTangSpaceDefault(&mikk_context);
}

Mesh create_mesh(const std::span<const Vertex> primitives) {
    ASSERT(primitives.size() % 3 == 0);

    Mesh mesh{};
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

Result<GltfData> load_gltf(const std::filesystem::path path) {
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

    auto model = ok<GltfData>();
    model->roughness = 0.5f;
    model->metalness = 0.0f;

    std::vector<Vertex> primitives{};
    for (const auto& mesh : asset->meshes) {
        for (const auto& primitive : mesh.primitives) {
            if (primitive.materialIndex.has_value()) {
                model->roughness = asset->materials[*primitive.materialIndex].pbrData.roughnessFactor;
                model->metalness = asset->materials[*primitive.materialIndex].pbrData.metallicFactor;
            }

            if (!primitive.indicesAccessor.has_value()) {
                LOGF_ERROR("Gltf file invalid; primitive has no indices accessor");
                return Err::GltfFileInvalid;
            }

            auto& index_accessor = asset->accessors[primitive.indicesAccessor.value()];
            auto& position_accessor = asset->accessors[primitive.findAttribute("POSITION")->accessorIndex];
            auto& normal_accessor = asset->accessors[primitive.findAttribute("NORMAL")->accessorIndex];
            auto& tex_coord_accessor = asset->accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];

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

            fastgltf::iterateAccessor<u32>(asset.get(), index_accessor, [&](u32 index) { 
                primitives.emplace_back(
                    fastgltf::getAccessorElement<glm::vec3>(asset.get(), position_accessor, index) * glm::vec3{1.0f, -1.0f, -1.0f},
                    fastgltf::getAccessorElement<glm::vec3>(asset.get(), normal_accessor, index) * glm::vec3{1.0f, -1.0f, -1.0f},
                    glm::vec4{},
                    fastgltf::getAccessorElement<glm::vec2>(asset.get(), tex_coord_accessor, index)
                );
            });

            if (primitives.size() % 3 != 0) {
                LOGF_ERROR("Gltf file invalid; primitives are not a multiple of 3: {}", path.string());
                return Err::GltfFileInvalid;
            }
        }
    }

    create_tangents(primitives);
    model->mesh = create_mesh(primitives);

    ASSERT(!model->mesh.indices.empty());
    ASSERT(!model->mesh.vertices.empty());
    return model;
}

} // namespace hg
