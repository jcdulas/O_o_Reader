#pragma once

#include "oor/base/math/Vec3.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace oor::base
{

// A 3x3 matrix, row-major (nine contiguous doubles). Used for coordinate system axes and rotations; the columns
// of a rotation matrix are the axes of the rotated frame expressed in the original one.
struct Mat3
{
    std::array<double, 9> values = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}; // identity

    [[nodiscard]] static constexpr Mat3 identity() noexcept { return Mat3{}; }

    // The matrix whose columns are the three axes.
    [[nodiscard]] static constexpr Mat3 fromColumns(const Vec3& first, const Vec3& second, const Vec3& third) noexcept
    {
        return Mat3{{first.x, second.x, third.x, first.y, second.y, third.y, first.z, second.z, third.z}};
    }

    [[nodiscard]] constexpr double operator()(std::size_t row, std::size_t column) const noexcept
    {
        return values[row * 3 + column];
    }
    constexpr double& operator()(std::size_t row, std::size_t column) noexcept { return values[row * 3 + column]; }

    [[nodiscard]] constexpr Vec3 column(std::size_t index) const noexcept
    {
        return Vec3{(*this)(0, index), (*this)(1, index), (*this)(2, index)};
    }
    [[nodiscard]] constexpr Vec3 row(std::size_t index) const noexcept
    {
        return Vec3{(*this)(index, 0), (*this)(index, 1), (*this)(index, 2)};
    }

    [[nodiscard]] constexpr Mat3 transposed() const noexcept
    {
        return Mat3{
            {values[0], values[3], values[6], values[1], values[4], values[7], values[2], values[5], values[8]}};
    }

    [[nodiscard]] constexpr double determinant() const noexcept { return dot(row(0), cross(row(1), row(2))); }

    [[nodiscard]] constexpr bool operator==(const Mat3&) const noexcept = default;
};

[[nodiscard]] constexpr Vec3 operator*(const Mat3& matrix, const Vec3& vector) noexcept
{
    return Vec3{dot(matrix.row(0), vector), dot(matrix.row(1), vector), dot(matrix.row(2), vector)};
}

[[nodiscard]] constexpr Mat3 operator*(const Mat3& left, const Mat3& right) noexcept
{
    Mat3 product;
    for (std::size_t row = 0; row < 3; ++row)
    {
        for (std::size_t column = 0; column < 3; ++column)
            product(row, column) = dot(left.row(row), right.column(column));
    }
    return product;
}

// Rotation by `angleInRadians` about the unit vector `axis` (Rodrigues formula).
[[nodiscard]] inline Mat3 rotationAboutAxis(const Vec3& axis, double angleInRadians) noexcept
{
    const Vec3 unit = normalized(axis);
    const double cosine = std::cos(angleInRadians);
    const double sine = std::sin(angleInRadians);
    const double oneMinusCosine = 1.0 - cosine;
    return Mat3{{cosine + unit.x * unit.x * oneMinusCosine, unit.x * unit.y * oneMinusCosine - unit.z * sine,
                 unit.x * unit.z * oneMinusCosine + unit.y * sine, unit.y * unit.x * oneMinusCosine + unit.z * sine,
                 cosine + unit.y * unit.y * oneMinusCosine, unit.y * unit.z * oneMinusCosine - unit.x * sine,
                 unit.z * unit.x * oneMinusCosine - unit.y * sine, unit.z * unit.y * oneMinusCosine + unit.x * sine,
                 cosine + unit.z * unit.z * oneMinusCosine}};
}

} // namespace oor::base
