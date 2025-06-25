#pragma once

#include "hg_utils.h"

namespace hg {

struct Mesh {
    std::vector<u32> indices = {};
    std::vector<glm::vec3> positions = {};
    std::vector<glm::vec3> normals = {};
    std::vector<glm::vec3> tangents = {};
    std::vector<glm::vec2> tex_coords = {};

    void generate_tangents();
};

[[nodiscard]] Mesh generate_square();
[[nodiscard]] Mesh generate_cube();
[[nodiscard]] Mesh generate_sphere(glm::uvec2 fidelity);

template<typename T>
class Image {
public:
    constexpr Image(usize width, usize height) : m_stride{width} {
        m_vals.resize(width * height);
    }

    [[nodiscard]] constexpr usize size() { return m_vals.size(); }

    [[nodiscard]] constexpr std::span<T> operator[](usize y) { return {&m_vals[y * m_stride], m_stride}; }
    [[nodiscard]] constexpr std::span<const T> operator[](usize y) const { return {&m_vals[y * m_stride], m_stride}; }

    [[nodiscard]] constexpr T* data() { return m_vals.data(); }
    [[nodiscard]] constexpr const T* data() const { return m_vals.data(); }

private:
    std::vector<T> m_vals = {};
    usize m_stride = 0;
};

[[nodiscard]] constexpr f64 gen_random_norm(f64 seed) {
    u64 i = static_cast<u64>(seed * UINT64_MAX);
    i = (i * 1082376456701324) + 987315471354970853;
    return static_cast<f64>(i) / UINT64_MAX;
}

[[nodiscard]] constexpr Image<u32> generate_white_noise_texture(usize width, usize height, f64 seed) {
    Image<u32> tex = {width, height};
    for (usize i = 0; i < height; ++i) {
        for (usize j = 0; j < width; ++j) {
            seed = gen_random_norm(seed);
            tex[i][j] = static_cast<u32>(seed * UINT32_MAX) | 0xff000000;
        }
    }
    return tex;
}

[[nodiscard]] constexpr Image<glm::vec4> generate_white_noise_normal_map(usize width, usize height, f64 seed) {
    Image<glm::vec4> map = {width, height};
    for (usize i = 0; i < height; ++i) {
        for (usize j = 0; j < width; ++j) {
            seed = gen_random_norm(seed);
            const f64 x = seed;
            seed = gen_random_norm(seed);
            const f64 y = seed;
            map[i][j] = glm::normalize(glm::vec4(x * 0.1f - 0.05f, y * 0.1f - 0.05f, -1.0f, 0.0f));
        }
    }
    return map;
}

} // namespace hg
