#include "AxisCrossing.hpp"

#include <algorithm>
#include <cmath>

namespace oor::analysis::detail
{

namespace
{

constexpr int MAX_ITERATIONS = 40;
// Natural units: far below any tolerance, yet above the round-off of a model whose coordinates are 10^5 element sizes
// from the origin (the steps there stagnate near 1e-11 instead of reaching 0).
constexpr double CONVERGED_STEP = 1e-9;
// Tangents (or a line and the tangents) closer to parallel than this, relative to the product of their lengths, are
// taken as parallel: the round-off of a degenerate element.
constexpr double PARALLEL_TOLERANCE = 1e-12;

struct Tangents
{
    base::Vec3 alongXi;
    base::Vec3 alongEta;
    base::Vec3 point;
};

Tangents tangentsAt(const ShellElement& element, double xi, double eta) noexcept
{
    const ShapeValues values = ShellShape::evaluate(element.kind, element.presentNodes, xi, eta);
    Tangents result;
    for (std::size_t slot = 0; slot < element.slotCount(); ++slot)
    {
        result.alongXi += values.dXi[slot] * element.points[slot];
        result.alongEta += values.dEta[slot] * element.points[slot];
        result.point += values.n[slot] * element.points[slot];
    }
    return result;
}

} // namespace

std::optional<LineCrossing> crossElement(const ShellElement& element, const base::Vec3& origin,
                                         const base::Vec3& direction) noexcept
{
    // Each step solves J·(dxi, deta, dt) = -F with J = [dX/dxi, dX/deta, -direction], by Cramer's rule.
    const double directionLength = base::norm(direction);
    if (!(directionLength > 0.0))
        return std::nullopt;
    const std::array<double, 2> start = ShellShape::centre(element.kind);
    LineCrossing crossing{start[0], start[1], 0.0};
    crossing.t = base::dot(tangentsAt(element, crossing.xi, crossing.eta).point - origin, direction) /
                 (directionLength * directionLength);
    const base::Vec3 minusDirection = -direction;
    for (int iteration = 0; iteration < MAX_ITERATIONS; ++iteration)
    {
        const Tangents here = tangentsAt(element, crossing.xi, crossing.eta);
        const base::Vec3 residual = here.point - origin - crossing.t * direction;
        const double determinant = base::dot(here.alongXi, base::cross(here.alongEta, minusDirection));
        const double scale = base::norm(here.alongXi) * base::norm(here.alongEta) * directionLength;
        if (!(std::abs(determinant) > PARALLEL_TOLERANCE * scale))
            return std::nullopt;
        const base::Vec3 rhs = -residual;
        const double stepXi = base::dot(rhs, base::cross(here.alongEta, minusDirection)) / determinant;
        const double stepEta = base::dot(here.alongXi, base::cross(rhs, minusDirection)) / determinant;
        const double stepT = base::dot(here.alongXi, base::cross(here.alongEta, rhs)) / determinant;
        crossing.xi += stepXi;
        crossing.eta += stepEta;
        crossing.t += stepT;
        if (!std::isfinite(crossing.xi) || !std::isfinite(crossing.eta) || !std::isfinite(crossing.t))
            return std::nullopt;
        if (std::abs(stepXi) + std::abs(stepEta) < CONVERGED_STEP)
            return crossing;
    }
    return std::nullopt; // not settled: the line misses a curved surface, or Newton's method cycles near a tangency
}

std::optional<base::Vec3> surfaceNormal(const ShellElement& element, double xi, double eta) noexcept
{
    const Tangents here = tangentsAt(element, xi, eta);
    const base::Vec3 normal = base::cross(here.alongXi, here.alongEta);
    const double length = base::norm(normal);
    if (!(length > PARALLEL_TOLERANCE * base::norm(here.alongXi) * base::norm(here.alongEta)))
        return std::nullopt;
    return normal / length;
}

std::array<double, 2> clampToElement(ShellShapeKind kind, double xi, double eta) noexcept
{
    if (!ShellShape::isTriangle(kind))
        return {std::clamp(xi, -1.0, 1.0), std::clamp(eta, -1.0, 1.0)};
    // Area coordinates (L1, L2, L3) = (1 - xi - eta, xi, eta): the negative ones set to 0, the rest scaled to sum 1.
    const double l1 = std::max(1.0 - xi - eta, 0.0);
    const double l2 = std::max(xi, 0.0);
    const double l3 = std::max(eta, 0.0);
    const double sum = l1 + l2 + l3;
    return {l2 / sum, l3 / sum};
}

double elementSize(const ShellElement& element) noexcept
{
    const std::size_t corners = ShellShape::cornerCount(element.kind);
    double total = 0.0;
    for (std::size_t corner = 0; corner < corners; ++corner)
        total += base::norm(element.points[(corner + 1) % corners] - element.points[corner]);
    return total / static_cast<double>(corners);
}

} // namespace oor::analysis::detail
