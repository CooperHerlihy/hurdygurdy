#include "hg_load.h"

namespace hg {

std::optional<ImageData> ImageData::load(const std::filesystem::path path) {
    debug_assert(!path.empty());

    ImageData data;
    data.pixels = stbi_load(path.string().data(), &data.width, &data.height, &data.channels, STBI_rgb_alpha);
    if (data.pixels == nullptr) {
        return std::nullopt;
    }

    critical_assert(data.width > 0);
    critical_assert(data.height > 0);
    critical_assert(data.channels > 0);
    return std::optional{data};
}

std::optional<ModelData> ModelData::load_gltf(const std::filesystem::path path) {
    fastgltf::Parser parser;

    auto buffer = fastgltf::GltfDataBuffer::FromPath(path);
    if (buffer.error() == fastgltf::Error::InvalidPath) {
        return std::nullopt;
    }
    critical_assert(buffer.error() == fastgltf::Error::None);

    const auto options =
        fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::GenerateMeshIndices | fastgltf::Options::LoadExternalBuffers; // | fastgltf::Options::LoadExternalImages;
    auto asset = parser.loadGltf(buffer.get(), path.parent_path(), options);
    critical_assert(asset.error() == fastgltf::Error::None);

    std::vector<u32> indices;
    std::vector<ModelVertex> vertices;
    float roughness = 0.5f;
    float metalness = 0.3f;

    for (const auto& mesh : asset->meshes) {
        for (const auto& primitive : mesh.primitives) {
            const usize initial_index = indices.size();
            const usize initial_vertex = vertices.size();

            if (primitive.materialIndex.has_value()) {
                roughness = asset->materials[*primitive.materialIndex].pbrData.roughnessFactor;
                metalness = asset->materials[*primitive.materialIndex].pbrData.metallicFactor;
            }

            critical_assert(primitive.indicesAccessor.has_value());

            auto& index_accessor = asset->accessors[primitive.indicesAccessor.value()];
            auto& position_accessor = asset->accessors[primitive.findAttribute("POSITION")->accessorIndex];
            auto& normal_accessor = asset->accessors[primitive.findAttribute("NORMAL")->accessorIndex];
            auto& uv_accessor = asset->accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];

            critical_assert(position_accessor.bufferViewIndex.has_value());
            critical_assert(position_accessor.type == fastgltf::AccessorType::Vec3);
            critical_assert(normal_accessor.bufferViewIndex.has_value());
            critical_assert(normal_accessor.type == fastgltf::AccessorType::Vec3);
            critical_assert(uv_accessor.bufferViewIndex.has_value());
            critical_assert(uv_accessor.type == fastgltf::AccessorType::Vec2);

            indices.resize(initial_index + index_accessor.count);
            vertices.resize(initial_vertex + position_accessor.count);

            fastgltf::iterateAccessorWithIndex<u32>(asset.get(), index_accessor,
                                                    [&](u32 index, usize i) { indices[i + initial_index] = index + static_cast<u32>(initial_vertex); });
            fastgltf::iterateAccessorWithIndex<glm::vec3>(asset.get(), position_accessor,
                                                          [&](glm::vec3 position, usize i) { vertices[i + initial_vertex].position = position * glm::vec3{1.0f, -1.0f, -1.0f}; });
            fastgltf::iterateAccessorWithIndex<glm::vec3>(asset.get(), normal_accessor,
                                                          [&](glm::vec3 normal, usize i) { vertices[i + initial_vertex].normal = normal * glm::vec3{1.0f, -1.0f, -1.0f}; });
            fastgltf::iterateAccessorWithIndex<glm::vec2>(asset.get(), uv_accessor, [&](glm::vec2 uv, usize i) { vertices[i + initial_vertex].uv = uv; });
        }
    }

    return std::make_optional<ModelData>(std::move(indices), std::move(vertices), roughness, metalness);
}

} // namespace hg
