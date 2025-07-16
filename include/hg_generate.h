#pragma once

#include "hg_utils.h"
#include "hg_load.h"

namespace hg {

template <typename T> struct Image {
    T* pixels = nullptr;
    glm::vec<2, usize> size{};

    [[nodiscard]] constexpr usize count() const { return size.x * size.y; }

    [[nodiscard]] constexpr Slice<T> operator[](usize y) { return {&pixels[y * size.x], size.x}; }
    [[nodiscard]] constexpr Slice<const T> operator[](usize y) const { return {&pixels[y * size.x], size.x}; }

    constexpr operator ImageData() { return {pixels, sizeof(T), {size.x, size.y, 1}}; }
};

struct ByteImage {
    byte* pixels = nullptr;
    usize alignment = 0;
    glm::vec<2, usize> size{};

    template <typename T> constexpr ByteImage(const Image<T>& image)
        : pixels{reinterpret_cast<byte*>(image.pixels)}
        , alignment{sizeof(T)}
        , size{image.size}
    {}
    template <typename T> constexpr operator Image<T>() {
        ASSERT(sizeof(T) == alignment);
        return {reinterpret_cast<T*>(pixels), size};
    }
};

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

class Generator {
public:
    struct Config {
        usize max_meshes = 0;
        usize max_images = 0;
        usize internal_stack_size = 1024 * 1024;
    };
    Generator(const Config& config)
        : m_stack{malloc_slice<byte>(config.internal_stack_size)}
        , m_meshes{malloc_slice<Pool<Mesh>::Block>(config.max_meshes)}
        , m_images{malloc_slice<Pool<ByteImage>::Block>(config.max_images)}
    {}
    void destroy() {
        free_slice(m_stack.release());
        free_slice(m_meshes.release());
        free_slice(m_images.release());
    }

    struct MeshHandle { Pool<Mesh>::Handle handle; };
    template <typename T> struct ImageHandle { Pool<ByteImage>::Handle handle; };

    [[nodiscard]] Mesh& get(MeshHandle mesh) { return m_meshes[mesh.handle]; }
    template <typename T> [[nodiscard]] Image<T> get(ImageHandle<T> image) { return m_images[image.handle]; }

    [[nodiscard]] MeshHandle alloc_mesh() {
        auto mesh = m_meshes.alloc();
        m_meshes[mesh] = {};
        return {mesh};
    }
    void dealloc_mesh(MeshHandle mesh) { m_meshes.dealloc(mesh.handle); }

    [[nodiscard]] MeshHandle generate_square(MeshHandle mesh);
    [[nodiscard]] MeshHandle generate_cube(MeshHandle mesh);
    [[nodiscard]] MeshHandle generate_sphere(MeshHandle mesh, const glm::uvec2 fidelity);

    template <typename T, typename F> [[nodiscard]] ImageHandle<T> alloc_image(glm::vec<2, usize> size, F pred) {
        Image<T> image_data{malloc_slice<T>(size.x * size.y).data, size};
        for (usize y = 0; y < image_data.size.y; ++y) {
            for (usize x = 0; x < image_data.size.x; ++x) {
                image_data[y][x] = pred(glm::vec<2, usize>{x, y});
            }
        }
        auto image_handle = m_images.alloc();
        m_images[image_handle] = image_data;
        return {image_handle};
    }
    template <typename T> void dealloc_image(ImageHandle<T> image) { m_images.dealloc(image.handle); }

private:
    Arena m_stack{};
    Pool<Mesh> m_meshes{};
    Pool<ByteImage> m_images{};
};

} // namespace hg
