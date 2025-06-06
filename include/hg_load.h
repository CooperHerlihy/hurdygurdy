#pragma once

#include "hg_utils.h"

namespace hg {

struct ImageData {
    u8* pixels = nullptr;
    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;

    [[nodiscard]] static std::optional<ImageData> load(std::filesystem::path path);
    void unload() const { std::free(pixels); }
};

struct ModelVertex {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 normal = {1.0f, 0.0f, 0.0f};
    glm::vec2 uv = {0.0f, 0.0f};
};

struct ModelPushConstant {
    glm::mat4 model = {1.0f};
};

struct ModelData {
    std::vector<u32> indices = {};
    std::vector<ModelVertex> vertices = {};

    [[nodiscard]] static std::optional<ModelData> load_gltf(std::filesystem::path path);
};

} // namespace hg
