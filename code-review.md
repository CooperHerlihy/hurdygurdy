# Code Review: Hurdy Gurdy

## Cross-Cutting Concerns

| Issue | Detail |
|-------|--------|
| **No `constexpr` on many eligible functions** | `Vec2::operator+`, `Vec3::operator*`, `lerp`, `smooth`, `clamp` (if it exists), etc., are defined but not `constexpr`. C++20 allows `constexpr` allocation for containers, but the vector math functions could all be `constexpr`. |

