#include "hg_load.h"

#include <stb/stb_image.h>

#define FASTGLTF_COMPILE_AS_CPP20
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <filesystem>
#include <memory>

namespace hg {

Result<ImageData> ImageData::load(const std::filesystem::path path) {
    ASSERT(!path.empty());

    int width = 0, height = 0, channels = 0;
    const auto pixels = stbi_load(path.string().data(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        LOGF_WARN("Image file not found: {}", path.string());
        return Err::ImageFileNotFound;
    }
    if (width <= 0 || height <= 0 || channels <= 0) {
        LOGF_WARN("Image file invalid; width, height, and/or channels are zero: {}", path.string());
        return Err::ImageFileInvalid;
    }

    return ok<ImageData>(std::unique_ptr<u8[], decltype(FreeDeleter)>{pixels}, width, height, channels);
}

Result<GltfModelData> GltfModelData::load_gltf(const std::filesystem::path path) {
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

    auto model = ok<GltfModelData>();
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
    model->mesh = Mesh::from_primitives(primitives);

    ASSERT(!model->mesh.indices.empty());
    ASSERT(!model->mesh.vertices.empty());
    return model;
}

} // namespace hg
