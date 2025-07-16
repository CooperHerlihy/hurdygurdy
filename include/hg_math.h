#pragma once

#include "hg_pch.h"
#include "hg_utils.h"

namespace glm {

template <typename T> constexpr mat<3, 3, T> operator*(qua<T> lhs, mat<3, 3, T> rhs) noexcept {
    return mat<3, 3, T>{
        lhs * rhs[0],
        lhs * rhs[1],
        lhs * rhs[2],
    };
}

} // namespace glm

namespace hg {

template <typename Ret, typename Pos, typename Seed> constexpr Ret rng(Pos pos, Seed seed);

template <> [[nodiscard]] constexpr u64 rng(u64 pos, u64 seed) {
    pos++; seed++;
    seed *= u64{0xf1eafdfd};
    seed ^= seed >> 12;
    pos *= seed;
    pos ^= pos >> 41;
    pos *= usize{0x1b037387};
    seed *= pos;
    seed ^= seed >> 21;
    return pos ^ seed;
}

template <> [[nodiscard]] constexpr u32 rng(u64 pos, u64 seed) { return static_cast<u32>(rng<u64>(pos, seed)); }

template <> [[nodiscard]] constexpr f64 rng(u64 pos, u64 seed) { return static_cast<f64>(rng<u64>(pos, seed)) / static_cast<f64>(UINT64_MAX); }
template <> [[nodiscard]] constexpr f32 rng(u64 pos, u64 seed) { return static_cast<f32>(rng<u64>(pos, seed)) / static_cast<f32>(UINT64_MAX); }

template <> [[nodiscard]] constexpr glm::vec2 rng(u64 pos, u64 seed) {
    const f32 angle = rng<f32>(pos, seed) * glm::two_pi<f32>();
    return {std::cos(angle), std::sin(angle)};
}

template <std::floating_point T> constexpr T smoothstep(const T t) {
    ASSERT(t >= 0 && t <= 1);
    return t * t * (3 - 2 * t);
}

template <std::floating_point T> constexpr T smoothstep_quintic(const T t) {
    ASSERT(t >= 0 && t <= 1);
    return t * t * t * (t * (t * 6 - 15) + 10);
}

template <typename T> struct Transform2D {
    glm::vec<3, T> position{0, 0, 0};
    glm::vec<2, T> scale{1, 1};
    T radians = 0;

    [[nodiscard]] constexpr glm::mat<4, 4, T> matrix() const noexcept {
        glm::mat<2, 2, T> m2{1};
        m2[0].x = scale.x;
        m2[1].y = scale.y;
        glm::mat<2, 2, T> rot{std::cos(radians), std::sin(radians), -std::sin(radians), std::cos(radians)};
        m2 = rot * m2;
        glm::mat<4, 4, T> m4{m2};
        m4[3].x = position.x;
        m4[3].y = position.y;
        m4[3].z = position.z;
        return m4;
    }

    constexpr Transform2D& translate(const glm::vec<2, T> delta) noexcept {
        position += delta;
        return *this;
    }
    constexpr Transform2D& rotate(const T angle_radians) noexcept {
        radians += angle_radians;
        return *this;
    }
};

using Transform2Df = Transform2D<f32>;

template <typename T> struct Transform3D {
    glm::vec<3, T> position{0, 0, 0};
    glm::vec<3, T> scale{1, 1, 1};
    glm::qua<T> rotation{1, 0, 0, 0};

    [[nodiscard]] constexpr glm::mat<4, 4, T> matrix() const noexcept {
        glm::mat<3, 3, T> m3{1};
        m3[0].x = scale.x;
        m3[1].y = scale.y;
        m3[2].z = scale.z;
        m3 = rotation * m3;
        glm::mat<4, 4, T> m4{m3};
        m4[3].x = position.x;
        m4[3].y = position.y;
        m4[3].z = position.z;
        return m4;
    }

    constexpr Transform3D& translate(const glm::vec<3, T> delta) noexcept {
        position += delta;
        return *this;
    }
    constexpr Transform3D& rotate_external(const glm::qua<T> delta) noexcept {
        rotation = delta * rotation;
        return *this;
    }
    constexpr Transform3D& rotate_internal(const glm::qua<T> delta) noexcept {
        rotation = rotation * delta;
        return *this;
    }
};

using Transform3Df = Transform3D<f32>;

template <typename T> struct Camera {
    glm::vec<3, T> position{0, 0, 0};
    glm::qua<T> rotation{1, 0, 0, 0};

    [[nodiscard]] constexpr glm::mat<4, 4, T> view() const noexcept {
        glm::mat<4, 4, T> rot{glm::conjugate(rotation) * glm::mat<3, 3, T>{1}};
        glm::mat<4, 4, T> pos{1};
        pos[3].x = -position.x;
        pos[3].y = -position.y;
        pos[3].z = -position.z;
        return rot * pos;
    }

    constexpr Camera& translate(const glm::vec<3, T> delta) noexcept {
        position += delta;
        return *this;
    }
    constexpr Camera& move(const glm::vec<3, T> dir, T distance) noexcept {
        glm::vec<3, T> d{rotation * glm::vec<3, T>{dir.x, 0, dir.z}};
        d.y = dir.y;
        position += glm::normalize(d) * distance;
        return *this;
    }
    constexpr Camera& rotate_external(const glm::qua<T> delta) noexcept {
        rotation = delta * rotation;
        return *this;
    }
    constexpr Camera& rotate_internal(const glm::qua<T> delta) noexcept {
        rotation = rotation * delta;
        return *this;
    }
};

using Cameraf = Camera<f32>;

} // namespace hg
