#pragma once

#include "hg_utils.h"

#include <filesystem>
#include <memory>

namespace hg {

struct ImageData {
private:
    static constexpr auto FreeDeleter = [](u8* ptr) { std::free(ptr); };

public:
    std::unique_ptr<u8[], decltype(FreeDeleter)> pixels;
    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;

    [[nodiscard]] static Result<ImageData> load(std::filesystem::path path);
};

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec4 tangent{};
    glm::vec2 tex_coord{};
};

struct Mesh {
    std::vector<u32> indices{};
    std::vector<Vertex> vertices{};

    [[nodiscard]] static Mesh from_primitives(std::span<const Vertex> primitives);
};

struct GltfModelData {
    Mesh mesh{};
    float roughness = 0.0f;
    float metalness = 0.0f;

    [[nodiscard]] static Result<GltfModelData> load_gltf(std::filesystem::path path);
};

} // namespace hg
