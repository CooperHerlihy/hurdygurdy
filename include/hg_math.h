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

static constexpr f64 Pi = 3.14159265358979323;
static constexpr f64 Tau = 2.0 * Pi;
static constexpr f64 E = 2.71828182845904523;

static constexpr f64 Infinity = std::numeric_limits<f64>::infinity();
static constexpr f64 NaN = std::numeric_limits<f64>::quiet_NaN();
static constexpr f64 Epsilon = 1e-10;

template <typename T, usize N> struct Vec {
    static_assert(N > 0);

    T data[N];

    [[nodiscard]] constexpr usize size() const { return N; }

    constexpr Vec() = default;
    constexpr Vec(const Vec& other) = default;
    constexpr Vec(Vec&& other) noexcept = default;
    constexpr Vec& operator=(const Vec& other) = default;
    constexpr Vec& operator=(Vec&& other) noexcept = default;

    template <typename... Args> explicit constexpr Vec(Args&&... args) {
        usize i = 0;
        ((memcpy(reinterpret_cast<byte*>(&data) + i, &args, sizeof(args)), i += sizeof(args)), ...);
        memset(reinterpret_cast<byte*>(&data) + i, 0, (N * sizeof(T) - i));
    }

    [[nodiscard]] constexpr T& operator[](const usize index) {
        ASSERT(index < N);
        return data[index];
    }
    [[nodiscard]] constexpr const T& operator[](const usize index) const {
        ASSERT(index < N);
        return data[index];
    }

    [[nodiscard]] constexpr T* begin() { return data; }
    [[nodiscard]] constexpr const T* begin() const { return data; }
    [[nodiscard]] constexpr T* end() { return data + N; }
    [[nodiscard]] constexpr const T* end() const { return data + N; }

    constexpr operator Slice<T>() const { return {data, N}; }
    constexpr operator Slice<const T>() const { return {data, N}; }
    constexpr operator std::span<T>() const { return {data, N}; }
    constexpr operator std::span<const T>() const { return {data, N}; }

    constexpr Vec& operator+=(const Vec& other) {
        for (usize i = 0; i < N; ++i)
            data[i] += other[i];
        return *this;
    }
    constexpr Vec& operator-=(const Vec& other) {
        for (usize i = 0; i < N; ++i)
            data[i] -= other[i];
        return *this;
    }
    constexpr Vec& operator*=(const Vec& other) {
        for (usize i = 0; i < N; ++i)
            data[i] *= other[i];
        return *this;
    }
    constexpr Vec& operator*=(T scalar) {
        for (usize i = 0; i < N; ++i)
            data[i] *= scalar;
        return *this;
    }
    constexpr Vec& operator/=(const Vec& other) {
        for (usize i = 0; i < N; ++i) {
            ASSERT(other[i] != 0);
            data[i] /= other[i];
        }
        return *this;
    }
    constexpr Vec& operator/=(T scalar) {
        for (usize i = 0; i < N; ++i)
            data[i] /= scalar;
        return *this;
    }
};

template <typename T, usize N> constexpr Vec<T, N> operator+(const Vec<T, N>& lhs, const Vec<T, N>& rhs) {
    Vec<T, N> out{lhs};
    out += rhs;
    return out;
}
template <typename T, usize N> constexpr Vec<T, N> operator-(const Vec<T, N>& lhs, const Vec<T, N>& rhs) {
    Vec<T, N> out{lhs};
    out -= rhs;
    return out;
}
template <typename T, usize N> constexpr Vec<T, N> operator*(const Vec<T, N>& lhs, const Vec<T, N>& rhs) {
    Vec<T, N> out{lhs};
    out *= rhs;
    return out;
}
template <typename T, usize N> constexpr Vec<T, N> operator/(const Vec<T, N>& lhs, const Vec<T, N>& rhs) {
    Vec<T, N> out{lhs};
    out /= rhs;
    return out;
}

template <typename T, usize N> constexpr Vec<T, N> operator*(const Vec<T, N>& lhs, T rhs) {
    Vec<T, N> out{lhs};
    out *= rhs;
    return out;
}
template <typename T, usize N> constexpr Vec<T, N> operator*(T lhs, const Vec<T, N>& rhs) {
    Vec<T, N> out{rhs};
    out *= lhs;
    return out;
}
template <typename T, usize N> constexpr Vec<T, N> operator/(const Vec<T, N>& lhs, T rhs) {
    Vec<T, N> out{lhs};
    out /= rhs;
    return out;
}

template <typename T, usize N> constexpr T dot(const Vec<T, N>& lhs, const Vec<T, N>& rhs) {
    T out = 0;
    for (usize i = 0; i < N; ++i)
        out += lhs[i] * rhs[i];
    return out;
}

template <typename T> constexpr Vec<T, 3> cross(const Vec<T, 3>& lhs, const Vec<T, 3>& rhs) {
    return {
        lhs[1] * rhs[2] - lhs[2] * rhs[1],
        lhs[2] * rhs[0] - lhs[0] * rhs[2],
        lhs[0] * rhs[1] - lhs[1] * rhs[0],
    };
}

template <typename T, usize N> constexpr Vec<T, N> normalize(const Vec<T, N>& vec) {
    T len_sqr = dot(vec, vec);
    if (len_sqr == 0)
        return vec;
    return vec / std::sqrt(len_sqr);
}

template <typename T> using Vec2 = Vec<T, 2>;
template <typename T> using Vec3 = Vec<T, 3>;
template <typename T> using Vec4 = Vec<T, 4>;

using Vec2f = Vec2<f32>;
using Vec3f = Vec3<f32>;
using Vec4f = Vec4<f32>;
using Vec2d = Vec2<f64>;
using Vec3d = Vec3<f64>;
using Vec4d = Vec4<f64>;

using Vec2i = Vec2<i32>;
using Vec3i = Vec3<i32>;
using Vec4i = Vec4<i32>;
using Vec2u = Vec2<u32>;
using Vec3u = Vec3<u32>;
using Vec4u = Vec4<u32>;

using Vec2p = Vec2<usize>;
using Vec3p = Vec3<usize>;
using Vec4p = Vec4<usize>;

template <typename T, usize W, usize H> struct Mat {
    static_assert(W > 0 && H > 0);

    Vec<T, H> data[W];

    [[nodiscard]] constexpr Vec2p size() const { return Vec2p{W, H}; }

    constexpr Mat() = default;
    constexpr Mat(const Mat& other) = default;
    constexpr Mat(Mat&& other) noexcept = default;
    constexpr Mat& operator=(const Mat& other) = default;
    constexpr Mat& operator=(Mat&& other) noexcept = default;

    constexpr Mat(T scalar) {
        static_assert(W == H);
        for (usize i = 0; i < W; ++i)
            data[i][i] = scalar;
    }
    constexpr Mat(std::initializer_list<Vec<T, H>> list) {
        ASSERT(list.size() == W);
        usize i = 0;
        for (const Vec<T, H>& vec : list)
            data[i++] = vec;
    }
    template <usize WO, usize HO> Mat(const Mat<T, WO, HO>& other) {
        static_assert(W == H);
        usize x;
        for (x = 0; x < WO; ++x) {
            usize y;
            for (y = 0; y < HO; ++y) {
                data[x][y] = other[x][y];
            }
            for (; y < H; ++y) {
                data[x][y] = x == y ? 1 : 0;
            }
        }
        for (; x < W; ++x) {
            for (usize y = HO; y < W; ++y) {
                data[x][y] = x == y ? 1 : 0;
            }
        }
    }

    [[nodiscard]] constexpr Vec<T, H>& operator[](const usize index) {
        ASSERT(index < W);
        return data[index];
    }
    [[nodiscard]] constexpr const Vec<T, H>& operator[](const usize index) const {
        ASSERT(index < W);
        return data[index];
    }

    constexpr Mat& operator+=(const Mat& other) {
        for (usize i = 0; i < W; ++i)
            for (usize j = 0; j < H; ++j)
                data[i][j] += other[i][j];
        return *this;
    }
    constexpr Mat& operator-=(const Mat& other) {
        for (usize i = 0; i < W; ++i)
            for (usize j = 0; j < H; ++j)
                data[i][j] -= other[i][j];
        return *this;
    }
    constexpr Mat& operator*=(T scalar) {
        for (usize i = 0; i < W; ++i)
            for (usize j = 0; j < H; ++j)
                data[i][j] *= scalar;
        return *this;
    }
    constexpr Mat& operator/=(T scalar) {
        for (usize i = 0; i < W; ++i)
            for (usize j = 0; j < H; ++j)
                data[i][j] /= scalar;
        return *this;
    }
};

// check for correctness : TODO
template <typename T, usize WA, usize HA, usize WB, usize HB>
Mat<T, WA, HB> operator*(const Mat<T, WA, HA>& lhs, const Mat<T, WB, HB>& rhs) {
    static_assert(HA == WB);
    Mat<T, WA, HB> out{};
    for (usize i = 0; i < WA; ++i)
        for (usize j = 0; j < HB; ++j)
            for (usize k = 0; k < WB; ++k)
                out[i][j] += lhs[i][k] * rhs[k][j];
    return out;
}

// check for correctness : TODO
template <typename T, usize W, usize H> Vec<T, H> operator*(const Mat<T, W, H>& lhs, const Vec<T, H>& rhs) {
    Vec<T, H> out{0};
    for (usize i = 0; i < W; ++i)
        for (usize j = 0; j < H; ++j)
            out[i] += lhs[i][j] * rhs[j];
    return out;
}

template <typename T, usize W, usize H> Mat<T, W, H> operator+(const Mat<T, W, H>& lhs, const Mat<T, W, H>& rhs) {
    Mat<T, W, H> out{lhs};
    out += rhs;
    return out;
}
template <typename T, usize W, usize H> Mat<T, W, H> operator-(const Mat<T, W, H>& lhs, const Mat<T, W, H>& rhs) {
    Mat<T, W, H> out{lhs};
    out -= rhs;
    return out;
}
template <typename T, usize W, usize H> Mat<T, W, H> operator*(const Mat<T, W, H>& lhs, T rhs) {
    Mat<T, W, H> out{lhs};
    out *= rhs;
    return out;
}
template <typename T, usize W, usize H> Mat<T, W, H> operator*(T lhs, const Mat<T, W, H>& rhs) {
    Mat<T, W, H> out{rhs};
    out *= lhs;
    return out;
}
template <typename T, usize W, usize H> Mat<T, W, H> operator/(const Mat<T, W, H>& lhs, T rhs) {
    Mat<T, W, H> out{lhs};
    out /= rhs;
    return out;
}

template <typename T, usize New, usize Old> Mat<T, New, New> grow_mat(const Mat<T, Old, Old>& old) {
    Mat<T, New, New> out{};
    usize x;
    for (x = 0; x < Old; ++x) {
        usize y;
        for (y = 0; y < Old; ++y) {
            out[x][y] = old[x][y];
        }
        for (; y < New; ++y) {
            out[x][y] = x == y ? 1 : 0;
        }
    }
    for (; x < New; ++x) {
        for (usize y = Old; y < New; ++y) {
            out[x][y] = x == y ? 1 : 0;
        }
    }
    return out;
}

template <typename T> using Mat2 = Mat<T, 2, 2>;
template <typename T> using Mat3 = Mat<T, 3, 3>;
template <typename T> using Mat4 = Mat<T, 4, 4>;

using Mat2f = Mat2<f32>;
using Mat3f = Mat3<f32>;
using Mat4f = Mat4<f32>;
using Mat2d = Mat2<f64>;
using Mat3d = Mat3<f64>;
using Mat4d = Mat4<f64>;

using Mat2i = Mat2<i32>;
using Mat3i = Mat3<i32>;
using Mat4i = Mat4<i32>;
using Mat2u = Mat2<u32>;
using Mat3u = Mat3<u32>;
using Mat4u = Mat4<u32>;

using Mat2p = Mat2<usize>;
using Mat3p = Mat3<usize>;
using Mat4p = Mat4<usize>;

template <typename T> struct Quat {
    T r, i, j, k;

    constexpr Quat() = default;
    constexpr Quat(const Quat& other) = default;
    constexpr Quat(Quat&& other) noexcept = default;
    constexpr Quat& operator=(const Quat& other) = default;
    constexpr Quat& operator=(Quat&& other) noexcept = default;

    constexpr Quat(T r, T i, T j, T k) : r{r}, i{i}, j{j}, k{k} {}
    constexpr Quat(T r) : r{r}, i{0}, j{0}, k{0} {}

    [[nodiscard]] constexpr T& operator[](const usize index) {
        ASSERT(index < 4);
        return (&i)[index];
    }
    [[nodiscard]] constexpr const T& operator[](const usize index) const {
        ASSERT(index < 4);
        return (&i)[index];
    }

    constexpr Quat& operator+=(const Quat& other) {
        i += other.i;
        j += other.j;
        k += other.k;
        r += other.r;
        return *this;
    }
    constexpr Quat& operator-=(const Quat& other) {
        i -= other.i;
        j -= other.j;
        k -= other.k;
        r -= other.r;
        return *this;
    }

    constexpr Quat& operator+=(T scalar) {
        r += scalar;
        return *this;
    }
    constexpr Quat& operator-=(T scalar) {
        r -= scalar;
        return *this;
    }
    constexpr Quat& operator*=(T scalar) {
        i *= scalar;
        j *= scalar;
        k *= scalar;
        r *= scalar;
        return *this;
    }
    constexpr Quat& operator/=(T scalar) {
        i /= scalar;
        j /= scalar;
        k /= scalar;
        r /= scalar;
        return *this;
    }
};

template <typename T> constexpr Quat<T> operator*(const Quat<T>& lhs, const Quat<T>& rhs) {
    return {
        lhs.r * rhs.r - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
        lhs.r * rhs.i + lhs.i * rhs.r + lhs.j * rhs.k - lhs.k * rhs.j,
        lhs.r * rhs.j - lhs.i * rhs.k + lhs.j * rhs.r + lhs.k * rhs.i,
        lhs.r * rhs.k + lhs.i * rhs.j - lhs.j * rhs.i + lhs.k * rhs.r,
    };
}

template <typename T> constexpr Quat<T> operator+(const Quat<T>& lhs, const Quat<T>& rhs) {
    Quat<T> out{lhs};
    out += rhs;
    return out;
}
template <typename T> constexpr Quat<T> operator-(const Quat<T>& lhs, const Quat<T>& rhs) {
    Quat<T> out{lhs};
    out -= rhs;
    return out;
}

template <typename T> constexpr Quat<T> operator+(const Quat<T>& lhs, T rhs) {
    Quat<T> out{lhs};
    out += rhs;
    return out;
}
template <typename T> constexpr Quat<T> operator+(T lhs, const Quat<T>& rhs) {
    Quat<T> out{rhs};
    out += lhs;
    return out;
}
template <typename T> constexpr Quat<T> operator-(const Quat<T>& lhs, T rhs) {
    Quat<T> out{lhs};
    out -= rhs;
    return out;
}
template <typename T> constexpr Quat<T> operator-(T lhs, const Quat<T>& rhs) {
    Quat<T> out{rhs};
    out -= lhs;
    return out;
}

template <typename T> constexpr Quat<T> operator*(const Quat<T>& lhs, T rhs) {
    Quat<T> out{lhs};
    out *= rhs;
    return out;
}
template <typename T> constexpr Quat<T> operator*(T lhs, const Quat<T>& rhs) {
    Quat<T> out{rhs};
    out *= lhs;
    return out;
}
template <typename T> constexpr Quat<T> operator/(const Quat<T>& lhs, T rhs) {
    Quat<T> out{lhs};
    out /= rhs;
    return out;
}

template <typename T> constexpr Quat<T> conjugate(const Quat<T>& q) {
    return {q.r, -q.i, -q.j, -q.k};
}

template <typename T> constexpr Quat<T> angle_axis(const T angle, const Vec<T, 3>& axis) {
    const T half_angle = angle / 2;
    return {
        std::cos(half_angle),
        std::sin(half_angle) * axis[0],
        std::sin(half_angle) * axis[1],
        std::sin(half_angle) * axis[2],
    };
}

template <typename T> constexpr Vec<T, 3> operator*(const Quat<T>& lhs, const Vec<T, 3>& rhs) {
    Quat<T> q{0, rhs[0], rhs[1], rhs[2]};
    Quat<T> result = lhs * q * conjugate(lhs);
    return {result.i, result.j, result.k};
}

template <typename T> constexpr Mat<T, 3, 3> operator*(Quat<T> lhs, Mat<T, 3, 3> rhs) noexcept {
    return Mat<T, 3, 3>{
        lhs * rhs[0],
        lhs * rhs[1],
        lhs * rhs[2],
    };
}

using Quatf = Quat<f32>;
using Quatd = Quat<f64>;
using Quati = Quat<i32>;
using Quatu = Quat<u32>;
using Quatp = Quat<usize>;

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
template <> [[nodiscard]] constexpr u32 rng(u64 pos, u64 seed) {
    return static_cast<u32>(rng<u64>(pos, seed));
}

template <> [[nodiscard]] constexpr f64 rng(u64 pos, u64 seed) {
    return static_cast<f64>(rng<u64>(pos, seed)) / static_cast<f64>(UINT64_MAX);
}
template <> [[nodiscard]] constexpr f32 rng(u64 pos, u64 seed) {
    return static_cast<f32>(rng<u64>(pos, seed)) / static_cast<f32>(UINT64_MAX);
}

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

// template <typename T> struct Transform3D {
//     Vec3<T> position{0, 0, 0};
//     Vec3<T> scale{1, 1, 1};
//     Quat<T> rotation{1, 0, 0, 0};
//
//     [[nodiscard]] constexpr glm::mat<4, 4, T> matrix() const noexcept {
//         Mat3<T> m3{1};
//         m3[0][0] = scale[0];
//         m3[1][1] = scale[1];
//         m3[2][2] = scale[2];
//         m3 = rotation * m3;
//         Mat3<T> m4{m3};
//         m4[3][0] = position[0];
//         m4[3][1] = position[1];
//         m4[3][2] = position[2];
//         return *reinterpret_cast<glm::mat<4, 4, T>*>(&m4);
//     }
//
//     constexpr Transform3D& translate(const glm::vec<3, T> delta) noexcept {
//         position += *reinterpret_cast<const Vec3<T>*>(&delta);
//         return *this;
//     }
//     constexpr Transform3D& rotate_external(const glm::qua<T> delta) noexcept {
//         rotation = *reinterpret_cast<const Quat<T>*>(&delta) * rotation;
//         return *this;
//     }
//     constexpr Transform3D& rotate_internal(const glm::qua<T> delta) noexcept {
//         rotation = rotation * *reinterpret_cast<const Quat<T>*>(&delta);
//         return *this;
//     }
// };

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
