#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace oor::base
{

// A point or a vector in three dimensions. A plain value type: three doubles, no padding, so that arrays of Vec3
// can be viewed as (N, 3) NumPy arrays without copy.
struct Vec3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    [[nodiscard]] constexpr double operator[](std::size_t index) const noexcept
    {
        return index == 0 ? x : (index == 1 ? y : z);
    }
    constexpr double& operator[](std::size_t index) noexcept { return index == 0 ? x : (index == 1 ? y : z); }

    constexpr Vec3& operator+=(const Vec3& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    constexpr Vec3& operator-=(const Vec3& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    constexpr Vec3& operator*=(double factor) noexcept
    {
        x *= factor;
        y *= factor;
        z *= factor;
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Vec3&) const noexcept = default;
};

[[nodiscard]] constexpr Vec3 operator+(Vec3 left, const Vec3& right) noexcept
{
    return left += right;
}
[[nodiscard]] constexpr Vec3 operator-(Vec3 left, const Vec3& right) noexcept
{
    return left -= right;
}
[[nodiscard]] constexpr Vec3 operator-(const Vec3& value) noexcept
{
    return Vec3{-value.x, -value.y, -value.z};
}
[[nodiscard]] constexpr Vec3 operator*(Vec3 vector, double factor) noexcept
{
    return vector *= factor;
}
[[nodiscard]] constexpr Vec3 operator*(double factor, Vec3 vector) noexcept
{
    return vector *= factor;
}
[[nodiscard]] constexpr Vec3 operator/(Vec3 vector, double divisor) noexcept
{
    return vector *= (1.0 / divisor);
}

[[nodiscard]] constexpr double dot(const Vec3& left, const Vec3& right) noexcept
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

[[nodiscard]] constexpr Vec3 cross(const Vec3& left, const Vec3& right) noexcept
{
    return Vec3{left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
                left.x * right.y - left.y * right.x};
}

[[nodiscard]] inline double norm(const Vec3& value) noexcept
{
    return std::sqrt(dot(value, value));
}

// The unit vector; a zero vector is returned unchanged (callers check degenerate input themselves).
[[nodiscard]] inline Vec3 normalized(const Vec3& value) noexcept
{
    const double length = norm(value);
    return length > 0.0 ? value / length : value;
}

} // namespace oor::base
