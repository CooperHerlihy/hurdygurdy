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
    if (pixels == nullptr)
        return Err::ImageFileNotFound;
    if (width <= 0 || height <= 0 || channels <= 0)
        return Err::ImageFileInvalid;

    return ok<ImageData>(std::unique_ptr<u8[], decltype(FreeDeleter)>{pixels}, width, height, channels);
}

Result<ModelData> ModelData::load_gltf(const std::filesystem::path path) {
    ASSERT(!path.empty());

    fastgltf::Parser parser;

    auto buffer = fastgltf::GltfDataBuffer::FromPath(path);
    if (buffer.error() == fastgltf::Error::InvalidPath)
        return Err::GltfFileNotFound;
    if (buffer.error() != fastgltf::Error::None)
        return Err::GltfFileInvalid;

    const auto options = fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::GenerateMeshIndices | fastgltf::Options::LoadExternalBuffers; // | fastgltf::Options::LoadExternalImages;
    auto asset = parser.loadGltf(buffer.get(), path.parent_path(), options);
    if (asset.error() != fastgltf::Error::None)
        return Err::GltfFileInvalid;

    auto model = ok<ModelData>();
    model->roughness = 0.5f;
    model->metalness = 0.0f;

    for (const auto& mesh : asset->meshes) {
        for (const auto& primitive : mesh.primitives) {
            if (primitive.materialIndex.has_value()) {
                model->roughness = asset->materials[*primitive.materialIndex].pbrData.roughnessFactor;
                model->metalness = asset->materials[*primitive.materialIndex].pbrData.metallicFactor;
            }

            if (!primitive.indicesAccessor.has_value())
                return Err::GltfFileInvalid;

            auto& index_accessor = asset->accessors[primitive.indicesAccessor.value()];
            auto& position_accessor = asset->accessors[primitive.findAttribute("POSITION")->accessorIndex];
            auto& normal_accessor = asset->accessors[primitive.findAttribute("NORMAL")->accessorIndex];
            auto& tex_coord_accessor = asset->accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];

            if (!position_accessor.bufferViewIndex.has_value())
                return Err::GltfFileInvalid;
            if (position_accessor.type != fastgltf::AccessorType::Vec3)
                return Err::GltfFileInvalid;
            if (!normal_accessor.bufferViewIndex.has_value())
                return Err::GltfFileInvalid;
            if (normal_accessor.type != fastgltf::AccessorType::Vec3)
                return Err::GltfFileInvalid;
            if (!tex_coord_accessor.bufferViewIndex.has_value())
                return Err::GltfFileInvalid;
            if (tex_coord_accessor.type != fastgltf::AccessorType::Vec2)
                return Err::GltfFileInvalid;

            fastgltf::iterateAccessor<u32>(asset.get(), index_accessor, [&](u32 index) { 
                model->mesh.indices.emplace_back(index + to_u32(model->mesh.positions.size()));
            });
            fastgltf::iterateAccessor<glm::vec3>(asset.get(), position_accessor, [&](glm::vec3 position) {
                model->mesh.positions.emplace_back(position * glm::vec3{1.0f, -1.0f, -1.0f});
            });
            fastgltf::iterateAccessor<glm::vec3>(asset.get(), normal_accessor, [&](glm::vec3 normal) {
                model->mesh.normals.emplace_back(normal * glm::vec3{1.0f, -1.0f, -1.0f});
            });
            fastgltf::iterateAccessor<glm::vec2>(asset.get(), tex_coord_accessor, [&](glm::vec2 tex_coord) {
                model->mesh.tex_coords.emplace_back(tex_coord);
            });
        }
    }

    model->mesh.generate_tangents();

    ASSERT(!model->mesh.indices.empty());
    ASSERT(!model->mesh.positions.empty());
    ASSERT(!model->mesh.normals.empty());
    // ASSERT(!model->mesh.tangents.empty());
    ASSERT(!model->mesh.tex_coords.empty());
    return model;
}

} // namespace hg
