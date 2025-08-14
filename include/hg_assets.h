#pragma once

#include "hg_utils.h"

namespace hg {

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec4 tangent{};
    glm::vec2 tex_coord{};
};

void generate_vertex_tangents(Slice<Vertex> primitives);

struct MeshData {
    Slice<u32> indices{};
    Slice<Vertex> vertices{};
};

void weld_mesh(MeshData& out_mesh, int& out_index_count, Slice<const Vertex> primitives);

struct ImageData {
    void* pixels = nullptr;
    usize alignment = 0;
    glm::vec<2, usize> size{};
};

template <typename T> struct Image {
    T* pixels = nullptr;
    glm::vec<2, usize> size{};

    [[nodiscard]] constexpr usize count() const { return size.x * size.y; }

    [[nodiscard]] constexpr Slice<T> operator[](usize y) { return {&pixels[y * size.x], size.x}; }
    [[nodiscard]] constexpr Slice<const T> operator[](usize y) const { return {&pixels[y * size.x], size.x}; }
};

using MeshHandle = Pool<MeshData>::Handle;

template <typename T> struct ImageHandle {
    using Type = T;
    Pool<ImageData>::Handle handle;

    constexpr operator ImageHandle<void>() const { return {handle}; }
};

struct AssetManager {
    Pool<MeshData> meshes{};
    Pool<ImageData> images{};
    Arena stack{};

    MeshData& operator[](const MeshHandle mesh) { return meshes[mesh]; }
    ImageData& operator[](const ImageHandle<void> image) { return images[image.handle]; }
};

struct AssetMangagerConfig {
    usize max_meshes = 64;
    usize max_images = 64;
    usize stack_size = 4 * 1024 * 1024;
};
AssetManager create_asset_manager(const AssetMangagerConfig& config);
void destroy_asset_manager(AssetManager& assets);

[[nodiscard]] MeshHandle create_mesh(AssetManager& assets);
void destroy_mesh(AssetManager& assets, const MeshHandle mesh);

[[nodiscard]] MeshHandle generate_square(AssetManager& assets);
[[nodiscard]] MeshHandle generate_cube(AssetManager& assets);
[[nodiscard]] MeshHandle generate_sphere(AssetManager& assets, const glm::uvec2 fidelity);

struct GltfData {
    MeshHandle mesh{};
    float roughness = 0.0f;
    float metalness = 0.0f;
};
Result<GltfData> load_gltf(AssetManager& assets, std::filesystem::path path);
void unload_gltf(AssetManager& assets, const GltfData gltf);

[[nodiscard]] ImageHandle<void> create_image(AssetManager& assets);
template <typename T> [[nodiscard]] ImageHandle<T> create_image(AssetManager& assets) {
    return {create_image(assets).handle};
}
[[nodiscard]] ImageHandle<void> create_image(AssetManager& assets, glm::vec<2, usize> size, u32 alignment);
template <typename T> [[nodiscard]] ImageHandle<T> create_image(AssetManager& assets, glm::vec<2, usize> size) {
    return {create_image(assets, size, sizeof(T)).handle};
}
void destroy_image(AssetManager& assets, const ImageHandle<void> image);
template <typename T> void destroy_image(AssetManager& assets, const ImageHandle<T> image) {
    destroy_image(assets, {image.handle});
}

[[nodiscard]] Result<ImageHandle<u32>> load_image(AssetManager& assets, std::filesystem::path path);

template <typename T, typename F> ImageHandle<T> generate_image(AssetManager& assets, glm::vec<2, usize> size, F pred) {
    ImageHandle<T> image_handle = create_image(assets, size, sizeof(T));
    ImageData& image = assets.images[image_handle];
    T* pixel = reinterpret_cast<T*>(image.pixels);
    for (usize y = 0; y < image.size.y; ++y) {
        for (usize x = 0; x < image.size.x; ++x) {
            *pixel = pred(glm::vec<2, usize>{x, y});
            ++pixel;
        }
    }
    return image_handle;
}

// TODO: tiling versions
[[nodiscard]] f32 get_value_noise(const glm::vec<2, usize> pos, const f32 point_width);
[[nodiscard]] f32 get_perlin_noise(const glm::vec<2, usize> pos, const f32 gradient_width);
template <typename F> [[nodiscard]] f32 get_fractal_noise(
    const glm::vec<2, usize> pos, const f32 min_width, const f32 max_width, F noise
) {
    ASSERT(max_width > min_width);
    ASSERT(min_width >= 1.0);

    f32 value = 0.0f;
    f32 octave_width = max_width;
    f32 octave_amplitude = std::floor(max_width / (min_width * 2.0f)) / (max_width / min_width - 1.0f);

    const usize octaves = static_cast<usize>(std::log2(max_width / min_width));
    for (usize i = 0; i < octaves; ++i, octave_width *= 0.5f, octave_amplitude *= 0.5f) {
        value += noise(pos, octave_width) * octave_amplitude;
    }
    return value;
}

[[nodiscard]] glm::vec4 get_normal_from_heightmap(const glm::vec<2, usize> pos, const Image<f32>& heightmap);

} // namespace hg
