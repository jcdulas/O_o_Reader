#pragma once

#include "ShellElements.hpp"
#include "ShellShape.hpp"

#include "oor/base/math/Vec3.hpp"

namespace oor::analysis::detail
{

// The closest point of the surface of one shell element to a point.
struct SurfaceProjection
{
    double xi = 0.0;
    double eta = 0.0;
    base::Vec3 point;
    double distance = 0.0;
    double outside = 0.0; // ShellShape::outsideDistance at (xi, eta): 0 when the point projects inside the element
    ShapeValues shape;    // the shape functions at (xi, eta)

    // Finds the closest point of the surface X(xi, eta) = sum N_i(xi, eta) x_i, extended past the edges of the
    // element, by Newton's method on F = |X - p|^2 / 2: the plane of a
    // triangle (one step), the bilinear surface of a quadrilateral (the twist term X_xi_eta), the quadratic surface of
    // a CQUAD8 or a CTRIA6. A step whose Hessian is not positive definite falls back to Gauss-Newton.
    [[nodiscard]] static SurfaceProjection onto(const ShellElement& element, const base::Vec3& point) noexcept;

    [[nodiscard]] bool isInside(double tolerance) const noexcept { return outside <= tolerance; }
};

// An element that holds a point, with where the point projects on it.
struct HostedPoint
{
    ShellElement element;
    SurfaceProjection projection;
};

// True when two distances to a surface are the same for the choice of a host (a point on an edge shared by two
// elements is at the same distance from both, to rounding).
[[nodiscard]] inline bool isSameDistance(double first, double second) noexcept
{
    const double larger = first > second ? first : second;
    const double gap = first > second ? first - second : second - first;
    return gap <= 1e-12 + 1e-9 * larger;
}

} // namespace oor::analysis::detail
