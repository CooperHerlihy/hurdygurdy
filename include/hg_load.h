#pragma once

#include "hg_mesh.h"
#include "hg_utils.h"

#include <filesystem>
#include <optional>

namespace hg {

struct ImageData {
    u8* pixels = nullptr;
    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;

    [[nodiscard]] static std::optional<ImageData> load(std::filesystem::path path);
    void unload() const { std::free(pixels); }
};

struct ModelData {
    Mesh mesh = {};
    float roughness = 0.0f;
    float metalness = 0.0f;

    [[nodiscard]] static std::optional<ModelData> load_gltf(std::filesystem::path path);
};

} // namespace hg
