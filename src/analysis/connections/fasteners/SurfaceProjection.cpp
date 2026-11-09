#include "SurfaceProjection.hpp"

#include <algorithm>
#include <cmath>

namespace oor::analysis::detail
{

namespace
{

constexpr int MAX_ITERATIONS = 60;
constexpr double CONVERGED_STEP = 1e-14;
constexpr double LARGEST_STEP = 1.0; // natural units: keeps a poor start from jumping far off the element
constexpr double NATURAL_BOUND = 4.0;

struct Derivatives
{
    base::Vec3 xi;
    base::Vec3 eta;
    base::Vec3 xiXi;
    base::Vec3 xiEta;
    base::Vec3 etaEta;
};

Derivatives derivativesOf(const ShellElement& element, const ShapeValues& values) noexcept
{
    Derivatives result;
    for (std::size_t slot = 0; slot < element.slotCount(); ++slot)
    {
        const base::Vec3& x = element.points[slot];
        result.xi += values.dXi[slot] * x;
        result.eta += values.dEta[slot] * x;
        result.xiXi += values.dXiXi[slot] * x;
        result.xiEta += values.dXiEta[slot] * x;
        result.etaEta += values.dEtaEta[slot] * x;
    }
    return result;
}

} // namespace

SurfaceProjection SurfaceProjection::onto(const ShellElement& element, const base::Vec3& point) noexcept
{
    const std::array<double, 2> start = ShellShape::centre(element.kind);
    double xi = start[0];
    double eta = start[1];
    for (int iteration = 0; iteration < MAX_ITERATIONS; ++iteration)
    {
        const ShapeValues values = ShellShape::evaluate(element.kind, element.presentNodes, xi, eta);
        const Derivatives d = derivativesOf(element, values);
        const base::Vec3 residual = element.pointAt(values) - point;
        const double gradientXi = base::dot(d.xi, residual);
        const double gradientEta = base::dot(d.eta, residual);
        // Gauss-Newton part and the curvature terms of the surface.
        const double firstXiXi = base::dot(d.xi, d.xi);
        const double firstXiEta = base::dot(d.xi, d.eta);
        const double firstEtaEta = base::dot(d.eta, d.eta);
        double hXiXi = firstXiXi + base::dot(d.xiXi, residual);
        double hXiEta = firstXiEta + base::dot(d.xiEta, residual);
        double hEtaEta = firstEtaEta + base::dot(d.etaEta, residual);
        double determinant = hXiXi * hEtaEta - hXiEta * hXiEta;
        const double scale = firstXiXi * firstEtaEta;
        if (hXiXi <= 0.0 || determinant <= 1e-14 * scale)
        {
            hXiXi = firstXiXi;
            hXiEta = firstXiEta;
            hEtaEta = firstEtaEta;
            determinant = hXiXi * hEtaEta - hXiEta * hXiEta;
            if (determinant <= 0.0)
                break; // a degenerate element: stays at the last point
        }
        double stepXi = -(hEtaEta * gradientXi - hXiEta * gradientEta) / determinant;
        double stepEta = -(hXiXi * gradientEta - hXiEta * gradientXi) / determinant;
        const double length = std::hypot(stepXi, stepEta);
        if (length > LARGEST_STEP)
        {
            stepXi *= LARGEST_STEP / length;
            stepEta *= LARGEST_STEP / length;
        }
        xi = std::clamp(xi + stepXi, -NATURAL_BOUND, NATURAL_BOUND);
        eta = std::clamp(eta + stepEta, -NATURAL_BOUND, NATURAL_BOUND);
        if (length < CONVERGED_STEP)
            break;
    }
    SurfaceProjection result;
    result.xi = xi;
    result.eta = eta;
    result.shape = ShellShape::evaluate(element.kind, element.presentNodes, xi, eta);
    result.point = element.pointAt(result.shape);
    result.distance = base::norm(result.point - point);
    result.outside = ShellShape::outsideDistance(element.kind, xi, eta);
    return result;
}

} // namespace oor::analysis::detail
