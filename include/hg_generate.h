#pragma once

#include "hg_utils.h"
#include "hg_math.h"
#include "hg_load.h"

#include <random>

namespace hg {

struct Mesh {
    std::vector<u32> indices{};
    std::vector<Vertex> vertices{};
};

[[nodiscard]] Mesh generate_square();
[[nodiscard]] Mesh generate_cube();
[[nodiscard]] Mesh generate_sphere(glm::uvec2 fidelity);

template<typename T>
class Image {
public:
    constexpr Image() = default;
    constexpr Image(glm::vec<2, usize> size) : m_stride{size.x} {
        m_vals.resize(size.x * size.y);
    }

    [[nodiscard]] constexpr usize size() const { return m_vals.size(); }
    [[nodiscard]] constexpr usize width() const { return m_stride; }
    [[nodiscard]] constexpr usize height() const { return m_vals.size() / m_stride; }

    [[nodiscard]] constexpr Slice<T> operator[](usize y) { return {&m_vals[y * m_stride], m_stride}; }
    [[nodiscard]] constexpr Slice<const T> operator[](usize y) const { return {&m_vals[y * m_stride], m_stride}; }

    [[nodiscard]] constexpr T* data() { return m_vals.data(); }
    [[nodiscard]] constexpr const T* data() const { return m_vals.data(); }

    constexpr Image& operator+=(const Image& other) {
        ASSERT(m_vals.size() == other.m_vals.size());
        ASSERT(m_stride == other.m_stride);
        for (usize i = 0; i < m_vals.size(); ++i) {
            m_vals[i] += other.m_vals[i];
        }
        return *this;
    }

private:
    std::vector<T> m_vals{};
    usize m_stride = 0;
};

template<typename T, typename U, typename F> Image<T> map_image(const Image<U>& image, F pred) {
    Image<T> mapped{{image.width(), image.height()}};
    for (usize y = 0; y < image.height(); ++y) {
        for (usize x = 0; x < image.width(); ++x) {
            mapped[y][x] = pred(image[y][x]);
        }
    }
    return mapped;
}

template<typename T, typename F> void transform_image(Image<T>& image, F pred) {
    for (usize y = 0; y < image.height(); ++y) {
        for (usize x = 0; x < image.width(); ++x) {
            image[y][x] = pred(image[y][x]);
        }
    }
}

template<typename T, typename F> [[nodiscard]] Image<T> transform_image(Image<T>&& image, F pred) {
    for (usize y = 0; y < image.height(); ++y) {
        for (usize x = 0; x < image.width(); ++x) {
            image[y][x] = pred(image[y][x]);
        }
    }
    return image;
}

[[nodiscard]] Image<glm::vec4> create_normals_from_heightmap(const Image<f32>& heightmap);

inline std::random_device g_random_device{};
inline std::mt19937_64 g_twister{g_random_device()};
using Twister = std::mt19937_64;

template <typename T> T rng();

template <> [[nodiscard]] inline f64 rng<f64>() { return std::uniform_real_distribution<f64>{0.0f, 1.0f}(g_twister); }
template <> [[nodiscard]] inline f32 rng<f32>() { return std::uniform_real_distribution<f32>{0.0f, 1.0f}(g_twister); }

template <> [[nodiscard]] inline u64 rng<u64>() { return g_twister(); }
template <> [[nodiscard]] inline u32 rng<u32>() { return static_cast<u32>(rng<u64>()); }
template <> [[nodiscard]] inline u16 rng<u16>() { return static_cast<u16>(rng<u64>()); }
template <> [[nodiscard]] inline u8 rng<u8>() { return static_cast<u8>(rng<u64>()); }

template <> [[nodiscard]] inline i64 rng<i64>() { return static_cast<i64>(rng<u64>()); }
template <> [[nodiscard]] inline i32 rng<i32>() { return static_cast<i32>(rng<u64>()); }
template <> [[nodiscard]] inline i16 rng<i16>() { return static_cast<i16>(rng<u64>()); }
template <> [[nodiscard]] inline i8 rng<i8>() { return static_cast<i8>(rng<u64>()); }

template <> [[nodiscard]] inline glm::vec2 rng<glm::vec2>() {
    const f32 angle = rng<f32>() * glm::two_pi<f32>();
    return {std::cos(angle), std::sin(angle)};
}

template <typename T, typename RandGen = decltype(rng<T>)> Image<T> generate_white_noise(const glm::vec<2, usize> size, RandGen rand_gen = rng<T>) {
    Image<T> image = size;
    for (T* pixel = image.data(); pixel < image.data() + image.size(); ++pixel) {
        *pixel = rand_gen();
    }
    return image;
}

[[nodiscard]] Image<f32> generate_value_noise(glm::vec<2, usize> size, const Image<f32>& fixed_points);
[[nodiscard]] Image<f32> generate_perlin_noise(glm::vec<2, usize> size, const Image<glm::vec2>& gradients);

[[nodiscard]] Image<f32> generate_fractal_value_noise(
    glm::vec<2, usize> size, glm::vec<2, usize> initial_size, usize max_octaves = SIZE_MAX
);
[[nodiscard]] Image<f32> generate_fractal_perlin_noise(
    glm::vec<2, usize> size, glm::vec<2, usize> initial_size, usize max_octaves = SIZE_MAX
);

} // namespace hg
