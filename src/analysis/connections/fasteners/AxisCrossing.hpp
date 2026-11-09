#pragma once

#include "ShellElements.hpp"
#include "ShellShape.hpp"

#include "oor/base/math/Vec3.hpp"

#include <array>
#include <optional>

namespace oor::analysis::detail
{

// Where a line crosses the surface of a shell element: the natural coordinates of the crossing (on the surface
// extended beyond the edges of the element) and the parameter t of the line, crossing = origin + t·direction.
struct LineCrossing
{
    double xi = 0.0;
    double eta = 0.0;
    double t = 0.0;
};

// The crossing of the line origin + t·direction with the isoparametric surface X(xi, eta) = sum N_i(xi, eta)·x_i of
// the element (the plane of a triangle, the bilinear surface of a quadrilateral, warped or not, the curved surface of a
// CQUAD8 or a CTRIA6), by Newton's method on the three equations X(xi, eta) - origin - t·direction = 0 from the centre
// of the element (exact in one step on a flat element). Nothing when the line runs parallel to the surface where it
// would cross it, or the iterations do not settle (a curved surface the line misses).
[[nodiscard]] std::optional<LineCrossing> crossElement(const ShellElement& element, const base::Vec3& origin,
                                                       const base::Vec3& direction) noexcept;

// The unit normal of the surface of the element at (xi, eta): dX/dxi x dX/deta, normalised (its sense follows the order
// of the corners). Nothing where the two tangents are parallel (a degenerate element).
[[nodiscard]] std::optional<base::Vec3> surfaceNormal(const ShellElement& element, double xi, double eta) noexcept;

// The natural coordinates brought onto the element: xi and eta clamped to [-1, 1] on a quadrilateral; on a triangle a
// negative area coordinate set to 0 and the others scaled to sum to 1. Inside the element they are returned as they
// are.
[[nodiscard]] std::array<double, 2> clampToElement(ShellShapeKind kind, double xi, double eta) noexcept;

// The size of an element: the mean length of its corner-to-corner edges.
[[nodiscard]] double elementSize(const ShellElement& element) noexcept;

} // namespace oor::analysis::detail
