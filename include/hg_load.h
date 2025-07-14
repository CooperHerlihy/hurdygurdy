#pragma once

#include "hg_utils.h"

#include <filesystem>

namespace hg {

struct ImageData {
    void* pixels = nullptr;
    u32 alignment = 0;
    glm::uvec3 size{};
};

class ImageLoader {
public:
    struct Handle { Pool<ImageData>::Handle handle; };

    struct Config {
        usize max_images = 0;
    };
    ImageLoader() = default;
    ImageLoader(const Config& config) : m_pool{Pool<ImageData>::create(config.max_images)} {}
    void destroy() { m_pool.destroy(); }

    ImageData& get(const Handle image) { return m_pool[image.handle]; }

    Result<Handle> load(std::filesystem::path path);
    void unload(Handle image);

private:
    Pool<ImageData> m_pool{};
};

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec4 tangent{};
    glm::vec2 tex_coord{};
};

void create_tangents(std::span<Vertex> primitives);

struct Mesh {
    std::vector<u32> indices{};
    std::vector<Vertex> vertices{};
};

[[nodiscard]] Mesh create_mesh(std::span<const Vertex> primitives);

struct GltfData {
    Mesh mesh{};
    float roughness = 0.0f;
    float metalness = 0.0f;
};

[[nodiscard]] Result<GltfData> load_gltf(std::filesystem::path path);

} // namespace hg
