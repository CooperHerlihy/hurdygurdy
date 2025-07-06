#pragma once

#include "hg_utils.h"
#include "hg_generate.h"

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

struct ModelData {
    Mesh mesh{};
    float roughness = 0.0f;
    float metalness = 0.0f;

    [[nodiscard]] static Result<ModelData> load_gltf(std::filesystem::path path);
};

} // namespace hg
