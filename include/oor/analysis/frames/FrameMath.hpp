#pragma once

#include "oor/base/math/Mat3.hpp"
#include "oor/base/math/Vec3.hpp"

#include <cmath>
#include <limits>

namespace oor::analysis
{

// The small pieces of linear algebra the element frames are made of. Frames are Mat3 values whose columns are the x, y
// and z axes in the basic system; an undefined frame is a matrix of NaN.

[[nodiscard]] inline base::Mat3 nanFrame() noexcept
{
    base::Mat3 frame;
    frame.values.fill(std::numeric_limits<double>::quiet_NaN());
    return frame;
}

[[nodiscard]] inline bool isFinite(const base::Vec3& value) noexcept
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

// The unit vector, or NaN when the vector is zero (or not finite).
[[nodiscard]] inline base::Vec3 unitOrNan(const base::Vec3& value) noexcept
{
    const double length = base::norm(value);
    if (!(length > 0.0) || !std::isfinite(length))
        return base::Vec3{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::quiet_NaN()};
    return value / length;
}

// The frame with this x axis whose z axis is perpendicular to the x-y plane that contains `inPlane`: z = unit(x cross
// inPlane), y = z cross x (the bars and beams: the orientation vector gives the plane 1). NaN when `inPlane` is
// parallel to the axis.
[[nodiscard]] inline base::Mat3 frameFromAxisAndPlaneVector(const base::Vec3& xAxis, const base::Vec3& inPlane) noexcept
{
    const base::Vec3 x = unitOrNan(xAxis);
    const base::Vec3 z = unitOrNan(cross(x, inPlane));
    if (!isFinite(x) || !isFinite(z) || base::norm(cross(x, inPlane)) < 1.0e-12 * base::norm(inPlane))
        return nanFrame();
    return base::Mat3::fromColumns(x, cross(z, x), z);
}

// The frame with the x axis and the z axis given (both unit vectors, perpendicular): y = z cross x.
[[nodiscard]] inline base::Mat3 frameFromXAndZ(const base::Vec3& x, const base::Vec3& z) noexcept
{
    if (!isFinite(x) || !isFinite(z))
        return nanFrame();
    return base::Mat3::fromColumns(x, cross(z, x), z);
}

// The inverse of a 3 x 3 matrix (NaN matrix when singular).
[[nodiscard]] inline base::Mat3 inverse(const base::Mat3& matrix) noexcept
{
    const base::Vec3 r0 = matrix.row(0);
    const base::Vec3 r1 = matrix.row(1);
    const base::Vec3 r2 = matrix.row(2);
    const base::Vec3 c0 = cross(r1, r2);
    const base::Vec3 c1 = cross(r2, r0);
    const base::Vec3 c2 = cross(r0, r1);
    const double determinant = dot(r0, c0);
    if (determinant == 0.0 || !std::isfinite(determinant))
        return nanFrame();
    // The rows of the inverse are the cross products of the other rows, over the determinant.
    return base::Mat3::fromColumns(c0 / determinant, c1 / determinant, c2 / determinant);
}

// The rotation Q of the polar decomposition M = Q P (P symmetric positive definite) of a matrix: the rotation closest
// to M. It is how the format reference chooses the element axes of a hexahedron and a tetrahedron "as close as possible" to the
// vectors R, S and T that join the middles of opposite faces or edges. Newton's iteration Q <- (Q + Q^-T) / 2; NaN when
// M is singular or has a negative determinant.
[[nodiscard]] inline base::Mat3 polarRotation(const base::Mat3& matrix) noexcept
{
    if (!(matrix.determinant() > 0.0))
        return nanFrame();
    base::Mat3 q = matrix;
    for (int iteration = 0; iteration < 60; ++iteration)
    {
        const base::Mat3 inverseTransposed = inverse(q).transposed();
        base::Mat3 next;
        double change = 0.0;
        for (std::size_t index = 0; index < 9; ++index)
        {
            next.values[index] = 0.5 * (q.values[index] + inverseTransposed.values[index]);
            change = std::fmax(change, std::fabs(next.values[index] - q.values[index]));
        }
        q = next;
        if (!(change > 1.0e-15))
            break;
    }
    return q;
}

} // namespace oor::analysis
