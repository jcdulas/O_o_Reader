#include "ShellShape.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace oor::analysis::detail
{

namespace
{

// The natural coordinates of the corners of a quadrilateral, in the order G1 G2 G3 G4.
constexpr std::array<double, 4> QUAD_XI = {-1.0, 1.0, 1.0, -1.0};
constexpr std::array<double, 4> QUAD_ETA = {-1.0, -1.0, 1.0, 1.0};

// The two corners of the edge of each mid-side node (slot 3 + k of a triangle, 4 + k of a quadrilateral).
constexpr std::array<std::array<std::size_t, 2>, 3> TRIA_EDGES = {{{0, 1}, {1, 2}, {2, 0}}};
constexpr std::array<std::array<std::size_t, 2>, 4> QUAD_EDGES = {{{0, 1}, {1, 2}, {2, 3}, {3, 0}}};

void evaluateTria3(double xi, double eta, ShapeValues& values) noexcept
{
    values.n[0] = 1.0 - xi - eta;
    values.n[1] = xi;
    values.n[2] = eta;
    values.dXi[0] = -1.0;
    values.dXi[1] = 1.0;
    values.dEta[0] = -1.0;
    values.dEta[2] = 1.0;
}

void evaluateQuad4(double xi, double eta, ShapeValues& values) noexcept
{
    for (std::size_t corner = 0; corner < 4; ++corner)
    {
        const double a = 1.0 + xi * QUAD_XI[corner];
        const double b = 1.0 + eta * QUAD_ETA[corner];
        values.n[corner] = 0.25 * a * b;
        values.dXi[corner] = 0.25 * QUAD_XI[corner] * b;
        values.dEta[corner] = 0.25 * QUAD_ETA[corner] * a;
        values.dXiEta[corner] = 0.25 * QUAD_XI[corner] * QUAD_ETA[corner];
    }
}

void evaluateTria6(double xi, double eta, ShapeValues& values) noexcept
{
    const std::array<double, 3> area = {1.0 - xi - eta, xi, eta};
    // The derivatives of the area coordinates with respect to xi and eta.
    constexpr std::array<double, 3> AREA_XI = {-1.0, 1.0, 0.0};
    constexpr std::array<double, 3> AREA_ETA = {-1.0, 0.0, 1.0};
    for (std::size_t corner = 0; corner < 3; ++corner)
    {
        const double l = area[corner];
        values.n[corner] = l * (2.0 * l - 1.0);
        values.dXi[corner] = (4.0 * l - 1.0) * AREA_XI[corner];
        values.dEta[corner] = (4.0 * l - 1.0) * AREA_ETA[corner];
        values.dXiXi[corner] = 4.0 * AREA_XI[corner] * AREA_XI[corner];
        values.dXiEta[corner] = 4.0 * AREA_XI[corner] * AREA_ETA[corner];
        values.dEtaEta[corner] = 4.0 * AREA_ETA[corner] * AREA_ETA[corner];
    }
    for (std::size_t edge = 0; edge < 3; ++edge)
    {
        const std::size_t first = TRIA_EDGES[edge][0];
        const std::size_t second = TRIA_EDGES[edge][1];
        const std::size_t slot = 3 + edge;
        values.n[slot] = 4.0 * area[first] * area[second];
        values.dXi[slot] = 4.0 * (AREA_XI[first] * area[second] + area[first] * AREA_XI[second]);
        values.dEta[slot] = 4.0 * (AREA_ETA[first] * area[second] + area[first] * AREA_ETA[second]);
        values.dXiXi[slot] = 8.0 * AREA_XI[first] * AREA_XI[second];
        values.dXiEta[slot] = 4.0 * (AREA_XI[first] * AREA_ETA[second] + AREA_ETA[first] * AREA_XI[second]);
        values.dEtaEta[slot] = 8.0 * AREA_ETA[first] * AREA_ETA[second];
    }
}

void evaluateQuad8(double xi, double eta, ShapeValues& values) noexcept
{
    // Serendipity corners: (1 + xi xi_i)(1 + eta eta_i)(xi xi_i + eta eta_i - 1) / 4.
    for (std::size_t corner = 0; corner < 4; ++corner)
    {
        const double xiI = QUAD_XI[corner];
        const double etaI = QUAD_ETA[corner];
        const double a = 1.0 + xi * xiI;
        const double b = 1.0 + eta * etaI;
        values.n[corner] = 0.25 * a * b * (xi * xiI + eta * etaI - 1.0);
        values.dXi[corner] = 0.25 * xiI * b * (2.0 * xi * xiI + eta * etaI);
        values.dEta[corner] = 0.25 * etaI * a * (xi * xiI + 2.0 * eta * etaI);
        values.dXiXi[corner] = 0.5 * b;
        values.dEtaEta[corner] = 0.5 * a;
        values.dXiEta[corner] = 0.25 * xiI * etaI * (2.0 * xi * xiI + 2.0 * eta * etaI + 1.0);
    }
    // Mid-sides 5 and 7 (xi_i = 0, eta_i = -1 and +1): (1 - xi^2)(1 + eta eta_i) / 2.
    for (const auto& [slot, etaI] : {std::pair<std::size_t, double>{4, -1.0}, {6, 1.0}})
    {
        values.n[slot] = 0.5 * (1.0 - xi * xi) * (1.0 + eta * etaI);
        values.dXi[slot] = -xi * (1.0 + eta * etaI);
        values.dEta[slot] = 0.5 * (1.0 - xi * xi) * etaI;
        values.dXiXi[slot] = -(1.0 + eta * etaI);
        values.dXiEta[slot] = -xi * etaI;
    }
    // Mid-sides 6 and 8 (eta_i = 0, xi_i = +1 and -1): (1 + xi xi_i)(1 - eta^2) / 2.
    for (const auto& [slot, xiI] : {std::pair<std::size_t, double>{5, 1.0}, {7, -1.0}})
    {
        values.n[slot] = 0.5 * (1.0 + xi * xiI) * (1.0 - eta * eta);
        values.dXi[slot] = 0.5 * xiI * (1.0 - eta * eta);
        values.dEta[slot] = -eta * (1.0 + xi * xiI);
        values.dEtaEta[slot] = -(1.0 + xi * xiI);
        values.dXiEta[slot] = -eta * xiI;
    }
}

// A mid-side node that is not there: its function is shared by the two corners of its edge (the edge stays straight).
void dropMissingMidSides(ShellShapeKind kind, std::uint8_t presentNodes, ShapeValues& values) noexcept
{
    const bool triangle = ShellShape::isTriangle(kind);
    const std::size_t corners = triangle ? 3 : 4;
    for (std::size_t edge = 0; edge < corners; ++edge)
    {
        const std::size_t slot = corners + edge;
        if ((presentNodes & (1U << slot)) != 0)
            continue;
        const auto& ends = triangle ? TRIA_EDGES[edge] : QUAD_EDGES[edge];
        const auto share = [&](std::array<double, MAX_SHELL_NODES>& entry)
        {
            entry[ends[0]] += 0.5 * entry[slot];
            entry[ends[1]] += 0.5 * entry[slot];
            entry[slot] = 0.0;
        };
        share(values.n);
        share(values.dXi);
        share(values.dEta);
        share(values.dXiXi);
        share(values.dXiEta);
        share(values.dEtaEta);
    }
}

} // namespace

std::size_t ShellShape::nodeCount(ShellShapeKind kind) noexcept
{
    switch (kind)
    {
    case ShellShapeKind::Tria3:
        return 3;
    case ShellShapeKind::Quad4:
        return 4;
    case ShellShapeKind::Tria6:
        return 6;
    case ShellShapeKind::Quad8:
        return 8;
    }
    return 0;
}

std::size_t ShellShape::cornerCount(ShellShapeKind kind) noexcept
{
    return isTriangle(kind) ? 3 : 4;
}

std::array<double, 2> ShellShape::centre(ShellShapeKind kind) noexcept
{
    if (isTriangle(kind))
        return {1.0 / 3.0, 1.0 / 3.0};
    return {0.0, 0.0};
}

ShapeValues ShellShape::evaluate(ShellShapeKind kind, std::uint8_t presentNodes, double xi, double eta) noexcept
{
    ShapeValues values;
    switch (kind)
    {
    case ShellShapeKind::Tria3:
        evaluateTria3(xi, eta, values);
        return values;
    case ShellShapeKind::Quad4:
        evaluateQuad4(xi, eta, values);
        return values;
    case ShellShapeKind::Tria6:
        evaluateTria6(xi, eta, values);
        break;
    case ShellShapeKind::Quad8:
        evaluateQuad8(xi, eta, values);
        break;
    }
    dropMissingMidSides(kind, presentNodes, values);
    return values;
}

double ShellShape::outsideDistance(ShellShapeKind kind, double xi, double eta) noexcept
{
    if (isTriangle(kind))
        return std::max({0.0, -(1.0 - xi - eta), -xi, -eta});
    return std::max({0.0, std::abs(xi) - 1.0, std::abs(eta) - 1.0});
}

} // namespace oor::analysis::detail
