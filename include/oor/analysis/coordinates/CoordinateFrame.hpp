#pragma once

#include "oor/base/math/Mat3.hpp"
#include "oor/base/math/Vec3.hpp"

#include <cstdint>

namespace oor::analysis
{

// How the three coordinates of a point in a coordinate system are read.
enum class CoordinateKind : std::uint8_t
{
    Rectangular, // (x, y, z)
    Cylindrical, // (R, theta in degrees, Z)
    Spherical    // (R, theta in degrees from the z axis, phi in degrees from the x axis in the x-y plane)
};

// A coordinate system resolved to the basic system: where its origin is and where its x, y and z axes point, whatever
// the kind (CORD1R/2R, CORD1C/2C and CORD1S/2S all have rectangular axes; the kind says how coordinates are read and
// how the directions of the coordinates vary from point to point).
struct CoordinateFrame
{
    CoordinateKind kind = CoordinateKind::Rectangular;
    base::Vec3 origin;
    // The columns are the x, y and z axes of the system, as unit vectors in the basic system.
    base::Mat3 axes = base::Mat3::identity();

    // The point with these coordinates (in the system's kind), in the basic system.
    [[nodiscard]] base::Vec3 toBasic(const base::Vec3& coordinates) const noexcept;

    // The same point in the rectangular axes of the system (before the rotation and the translation to the basic
    // system).
    [[nodiscard]] base::Vec3 toLocalRectangular(const base::Vec3& coordinates) const noexcept;

    // The coordinates (in the system's kind) of a point given in the basic system: the inverse of toBasic. On the axis
    // of a cylindrical or spherical system the angles are 0.
    [[nodiscard]] base::Vec3 fromBasic(const base::Vec3& basicPoint) const noexcept;

    // The directions of the three coordinates at a point given in the basic system, as the columns of a matrix in the
    // basic system: x, y, z for a rectangular system, (R, theta, Z) for a cylindrical one, (R, theta, phi) for a
    // spherical one. These are the directions 1, 2, 3 of the displacements of a grid point whose CD is this system, so
    // that `rotation * (u1, u2, u3)` is the displacement in the basic system.
    //
    // On the axis of a cylindrical system (R = 0) the angle is 0. In a spherical system, at the origin the directions
    // are the axes of the system (R along x, theta along y, phi along z); on the z axis (away from the origin) the
    // angles theta and phi are taken at phi = 0: R is along +z or -z, theta along +x or -x, phi along +y.
    [[nodiscard]] base::Mat3 directionsAt(const base::Vec3& basicPoint) const noexcept;
};

// The coordinate system whose z axis goes from `origin` to `onZAxis` and whose x-z plane contains `inXzPlane`
// (CORD1x and CORD2x): z = unit(onZAxis - origin), y = unit(z x (inXzPlane - origin)), x = y x z. Returns false when
// the points do not define a system (two of them coincide, or the three are on a line); the frame is then not set.
[[nodiscard]] bool frameFromThreePoints(CoordinateKind kind, const base::Vec3& origin, const base::Vec3& onZAxis,
                                        const base::Vec3& inXzPlane, CoordinateFrame& frame) noexcept;

} // namespace oor::analysis
