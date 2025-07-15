#pragma once

#include "hg_utils.h"

#include <filesystem>

namespace hg {

struct ImageData {
    void* pixels = nullptr;
    u32 alignment = 0;
    glm::uvec3 size{};
};

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec4 tangent{};
    glm::vec2 tex_coord{};
};

void generate_tangents(Slice<Vertex> primitives);
int weld_mesh(Slice<Vertex> out_vertices, Slice<u32> out_indices, Slice<const Vertex> primitives);

struct GltfData {
    Slice<u32> indices{};
    Slice<Vertex> vertices{};
    float roughness = 0.0f;
    float metalness = 0.0f;
};

class AssetLoader {
public:
    struct ImageHandle { Pool<ImageData>::Handle handle; };
    struct GltfHandle { Pool<GltfData>::Handle handle; };

    struct Config {
        usize max_images = 0;
        usize max_gltfs = 0;
    };
    AssetLoader() = default;
    AssetLoader(const Config& config)
        : m_images{malloc_slice<Pool<ImageData>::Block>(config.max_images)}
        , m_gltfs{malloc_slice<Pool<GltfData>::Block>(config.max_gltfs)}
    {}
    void destroy() {
        free_slice(m_images.release());
        free_slice(m_gltfs.release());
    }

    ImageData& get(const ImageHandle image) { return m_images[image.handle]; }
    GltfData& get(const GltfHandle gltf) { return m_gltfs[gltf.handle]; }

    Result<ImageHandle> load_image(std::filesystem::path path);
    void unload_image(const ImageHandle image) {
        std::free(get(image).pixels);
        m_images.dealloc(image.handle);
    }

    Result<GltfHandle> load_gltf(std::filesystem::path path);
    void unload_gltf(const GltfHandle gltf) {
        free_slice(m_gltfs[gltf.handle].vertices);
        free_slice(m_gltfs[gltf.handle].indices);
        m_gltfs.dealloc(gltf.handle);
    }

private:
    Pool<ImageData> m_images{};
    Pool<GltfData> m_gltfs{};
};

} // namespace hg
