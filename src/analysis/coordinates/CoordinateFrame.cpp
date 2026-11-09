#include "oor/analysis/coordinates/CoordinateFrame.hpp"

#include <cmath>
#include <numbers>

namespace oor::analysis
{

namespace
{

constexpr double DEGREES_TO_RADIANS = std::numbers::pi / 180.0;
constexpr double RADIANS_TO_DEGREES = 180.0 / std::numbers::pi;

} // namespace

base::Vec3 CoordinateFrame::toLocalRectangular(const base::Vec3& coordinates) const noexcept
{
    switch (kind)
    {
    case CoordinateKind::Rectangular:
        return coordinates;
    case CoordinateKind::Cylindrical:
    {
        const double theta = coordinates.y * DEGREES_TO_RADIANS;
        return base::Vec3{coordinates.x * std::cos(theta), coordinates.x * std::sin(theta), coordinates.z};
    }
    case CoordinateKind::Spherical:
    {
        const double theta = coordinates.y * DEGREES_TO_RADIANS;
        const double phi = coordinates.z * DEGREES_TO_RADIANS;
        const double inPlane = coordinates.x * std::sin(theta);
        return base::Vec3{inPlane * std::cos(phi), inPlane * std::sin(phi), coordinates.x * std::cos(theta)};
    }
    }
    return coordinates;
}

base::Vec3 CoordinateFrame::toBasic(const base::Vec3& coordinates) const noexcept
{
    return origin + axes * toLocalRectangular(coordinates);
}

base::Vec3 CoordinateFrame::fromBasic(const base::Vec3& basicPoint) const noexcept
{
    const base::Vec3 local = axes.transposed() * (basicPoint - origin);
    switch (kind)
    {
    case CoordinateKind::Rectangular:
        return local;
    case CoordinateKind::Cylindrical:
        return base::Vec3{std::hypot(local.x, local.y), std::atan2(local.y, local.x) * RADIANS_TO_DEGREES, local.z};
    case CoordinateKind::Spherical:
    {
        const double radius = norm(local);
        if (radius == 0.0)
            return base::Vec3{};
        // The polar angle by atan2 rather than acos(z / R): exact to the rounding near the poles too.
        return base::Vec3{radius, std::atan2(std::hypot(local.x, local.y), local.z) * RADIANS_TO_DEGREES,
                          std::atan2(local.y, local.x) * RADIANS_TO_DEGREES};
    }
    }
    return local;
}

base::Mat3 CoordinateFrame::directionsAt(const base::Vec3& basicPoint) const noexcept
{
    if (kind == CoordinateKind::Rectangular)
        return axes;
    const base::Vec3 local = axes.transposed() * (basicPoint - origin);
    // The three directions in the axes of the system; they are turned into the basic system by `axes` at the end.
    base::Vec3 first{1.0, 0.0, 0.0};
    base::Vec3 second{0.0, 1.0, 0.0};
    base::Vec3 third{0.0, 0.0, 1.0};
    if (kind == CoordinateKind::Cylindrical)
    {
        const double theta = std::atan2(local.y, local.x); // 0 on the axis
        const double cosine = std::cos(theta);
        const double sine = std::sin(theta);
        first = base::Vec3{cosine, sine, 0.0};
        second = base::Vec3{-sine, cosine, 0.0};
    }
    else
    {
        const double inPlane = std::hypot(local.x, local.y);
        const double radius = std::hypot(inPlane, local.z);
        if (radius == 0.0)
        {
            // At the origin the triad is the one of the system.
        }
        else if (inPlane == 0.0)
        {
            // On the polar axis the angles are taken at phi = 0.
            const double sign = local.z > 0.0 ? 1.0 : -1.0;
            first = base::Vec3{0.0, 0.0, sign};
            second = base::Vec3{sign, 0.0, 0.0};
            third = base::Vec3{0.0, 1.0, 0.0};
        }
        else
        {
            const double sinTheta = inPlane / radius;
            const double cosTheta = local.z / radius;
            const double cosPhi = local.x / inPlane;
            const double sinPhi = local.y / inPlane;
            first = base::Vec3{sinTheta * cosPhi, sinTheta * sinPhi, cosTheta};
            second = base::Vec3{cosTheta * cosPhi, cosTheta * sinPhi, -sinTheta};
            third = base::Vec3{-sinPhi, cosPhi, 0.0};
        }
    }
    return base::Mat3::fromColumns(axes * first, axes * second, axes * third);
}

bool frameFromThreePoints(CoordinateKind kind, const base::Vec3& origin, const base::Vec3& onZAxis,
                          const base::Vec3& inXzPlane, CoordinateFrame& frame) noexcept
{
    const base::Vec3 zDirection = onZAxis - origin;
    const base::Vec3 inPlane = inXzPlane - origin;
    const double zLength = norm(zDirection);
    if (zLength == 0.0)
        return false;
    const base::Vec3 z = zDirection / zLength;
    const base::Vec3 yDirection = cross(z, inPlane);
    const double yLength = norm(yDirection);
    // The points are on a line when the third is on the z axis: the cross product vanishes (up to rounding).
    if (!(yLength > 1.0e-12 * zLength * norm(inPlane)))
        return false;
    const base::Vec3 y = yDirection / yLength;
    const base::Vec3 x = cross(y, z);
    frame.kind = kind;
    frame.origin = origin;
    frame.axes = base::Mat3::fromColumns(x, y, z);
    return true;
}

} // namespace oor::analysis
