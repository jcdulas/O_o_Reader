#pragma once

#include "oor/base/math/Vec3.hpp"

#include <array>
#include <cstddef>

namespace oor::analysis::detail
{

// The closest point of a contact face to a point, and its distance.
struct ClosestPoint
{
    base::Vec3 point;
    double distance = 0.0;
};

// The closest point of a face given by its corners: 2 corners a straight segment (a BLSEG line), 3 a flat triangle, 4 a
// quadrilateral taken as the four triangles from its centre to each side (a warped quadrilateral is followed to the
// second order of its warp, and the result does not depend on the corner it starts from). The mid-side grid points of
// a quadratic element are not used: its faces are taken between their corners.
[[nodiscard]] ClosestPoint closestPointOnFace(const std::array<base::Vec3, 4>& corners, std::size_t count,
                                              const base::Vec3& point) noexcept;

} // namespace oor::analysis::detail
