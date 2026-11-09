#include "SolidVolume.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace oor::analysis::detail
{

namespace
{

using base::Vec3;

bool present(const Vec3& point) noexcept
{
    return point.x == point.x;
}

bool anyPresent(std::span<const Vec3> points, std::size_t first) noexcept
{
    for (std::size_t index = first; index < points.size(); ++index)
    {
        if (present(points[index]))
            return true;
    }
    return false;
}

Vec3 middle(const Vec3& first, const Vec3& second) noexcept
{
    return 0.5 * (first + second);
}

double determinant(const Vec3& first, const Vec3& second, const Vec3& third) noexcept
{
    return dot(first, cross(second, third));
}

// --- the hexahedron
// --------------------------------------------------------------------------------------------------------

// The reference coordinates of the 20 nodes of the hexahedron.
constexpr double HEXAHEDRON_REFERENCE[20][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1},  {-1, 1, -1}, {-1, -1, 1},
                                                {1, -1, 1},   {1, 1, 1},   {-1, 1, 1},  {0, -1, -1}, {1, 0, -1},
                                                {0, 1, -1},   {-1, 0, -1}, {-1, -1, 0}, {1, -1, 0},  {1, 1, 0},
                                                {-1, 1, 0},   {0, -1, 1},  {1, 0, 1},   {0, 1, 1},   {-1, 0, 1}};

// The two corners each mid-side node of the hexahedron lies between.
constexpr std::size_t HEXAHEDRON_EDGES[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 5},
                                                 {2, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}, {7, 4}};

// The Jacobian determinant of the eight-node hexahedron at a point.
double hexahedron8Determinant(const Vec3 (&points)[8], double xi, double eta, double zeta) noexcept
{
    Vec3 dxi;
    Vec3 deta;
    Vec3 dzeta;
    for (std::size_t node = 0; node < 8; ++node)
    {
        const double* r = HEXAHEDRON_REFERENCE[node];
        const double a = 1.0 + xi * r[0];
        const double b = 1.0 + eta * r[1];
        const double c = 1.0 + zeta * r[2];
        dxi += (r[0] * b * c / 8.0) * points[node];
        deta += (r[1] * a * c / 8.0) * points[node];
        dzeta += (r[2] * a * b / 8.0) * points[node];
    }
    return determinant(dxi, deta, dzeta);
}

// The Jacobian determinant of the twenty-node hexahedron (serendipity shape functions) at a point.
double hexahedron20Determinant(const Vec3 (&points)[20], double xi, double eta, double zeta) noexcept
{
    Vec3 dxi;
    Vec3 deta;
    Vec3 dzeta;
    for (std::size_t node = 0; node < 20; ++node)
    {
        const double* r = HEXAHEDRON_REFERENCE[node];
        double dnDxi = 0.0;
        double dnDeta = 0.0;
        double dnDzeta = 0.0;
        const double a = 1.0 + xi * r[0];
        const double b = 1.0 + eta * r[1];
        const double c = 1.0 + zeta * r[2];
        if (node < 8)
        {
            const double s = xi * r[0] + eta * r[1] + zeta * r[2] - 2.0;
            dnDxi = r[0] * b * c * (s + a) / 8.0;
            dnDeta = r[1] * a * c * (s + b) / 8.0;
            dnDzeta = r[2] * a * b * (s + c) / 8.0;
        }
        else if (r[0] == 0.0)
        {
            dnDxi = -xi * b * c / 2.0;
            dnDeta = (1.0 - xi * xi) * r[1] * c / 4.0;
            dnDzeta = (1.0 - xi * xi) * b * r[2] / 4.0;
        }
        else if (r[1] == 0.0)
        {
            dnDxi = (1.0 - eta * eta) * r[0] * c / 4.0;
            dnDeta = -eta * a * c / 2.0;
            dnDzeta = (1.0 - eta * eta) * a * r[2] / 4.0;
        }
        else
        {
            dnDxi = (1.0 - zeta * zeta) * r[0] * b / 4.0;
            dnDeta = (1.0 - zeta * zeta) * a * r[1] / 4.0;
            dnDzeta = -zeta * a * b / 2.0;
        }
        dxi += dnDxi * points[node];
        deta += dnDeta * points[node];
        dzeta += dnDzeta * points[node];
    }
    return determinant(dxi, deta, dzeta);
}

double hexahedron8(const Vec3 (&points)[8]) noexcept
{
    // The determinant is a polynomial of degree 2 in each variable: two Gauss points per direction are exact.
    const double g = 1.0 / std::sqrt(3.0);
    double total = 0.0;
    for (const double xi : {-g, g})
    {
        for (const double eta : {-g, g})
        {
            for (const double zeta : {-g, g})
                total += hexahedron8Determinant(points, xi, eta, zeta);
        }
    }
    return std::fabs(total);
}

double hexahedron20(const Vec3 (&points)[20]) noexcept
{
    // Degree 5 in each variable: three Gauss points per direction are exact.
    const double g = std::sqrt(0.6);
    const double points1[3] = {-g, 0.0, g};
    const double weights[3] = {5.0 / 9.0, 8.0 / 9.0, 5.0 / 9.0};
    double total = 0.0;
    for (std::size_t i = 0; i < 3; ++i)
    {
        for (std::size_t j = 0; j < 3; ++j)
        {
            for (std::size_t k = 0; k < 3; ++k)
                total += weights[i] * weights[j] * weights[k] *
                         hexahedron20Determinant(points, points1[i], points1[j], points1[k]);
        }
    }
    return std::fabs(total);
}

// --- the wedge
// -------------------------------------------------------------------------------------------------------------

// The triangle of node k (0..2) of the wedge corner list and the side (-1 bottom, +1 top).
double wedgeSide(std::size_t node) noexcept
{
    return node < 3 ? -1.0 : 1.0;
}

// Integrates the determinant over the triangle (rule with `count` points, barycentric coordinates and weights summing
// to 1/2) and the interval [-1, 1] (Gauss rule).
template <typename Determinant>
double integrateWedge(const Determinant& determinantAt, bool quadratic) noexcept
{
    struct TrianglePoint
    {
        double l1, l2, weight;
    };
    double total = 0.0;
    if (!quadratic)
    {
        // Three points, exact to degree 2: the determinant of the linear wedge has degree 2 in the triangle coordinates
        // at most, and degree 2 in zeta.
        const TrianglePoint triangle[3] = {
            {1.0 / 6.0, 1.0 / 6.0, 1.0 / 6.0}, {1.0 / 6.0, 2.0 / 3.0, 1.0 / 6.0}, {2.0 / 3.0, 1.0 / 6.0, 1.0 / 6.0}};
        const double g = 1.0 / std::sqrt(3.0);
        for (const TrianglePoint& point : triangle)
        {
            for (const double zeta : {-g, g})
                total += point.weight * determinantAt(point.l1, point.l2, zeta);
        }
        return std::fabs(total);
    }
    // Seven points, exact to degree 5 (Radon), times three Gauss points.
    const double root = std::sqrt(15.0);
    const double a1 = (6.0 - root) / 21.0;
    const double b1 = (9.0 + 2.0 * root) / 21.0;
    const double a2 = (6.0 + root) / 21.0;
    const double b2 = (9.0 - 2.0 * root) / 21.0;
    const double w0 = 9.0 / 80.0;
    const double w1 = (155.0 - root) / 2400.0;
    const double w2 = (155.0 + root) / 2400.0;
    const TrianglePoint triangle[7] = {
        {1.0 / 3.0, 1.0 / 3.0, w0}, {a1, a1, w1}, {a1, b1, w1}, {b1, a1, w1}, {a2, a2, w2}, {a2, b2, w2}, {b2, a2, w2}};
    const double g = std::sqrt(0.6);
    const double zetas[3] = {-g, 0.0, g};
    const double weights[3] = {5.0 / 9.0, 8.0 / 9.0, 5.0 / 9.0};
    for (const TrianglePoint& point : triangle)
    {
        for (std::size_t index = 0; index < 3; ++index)
            total += point.weight * weights[index] * determinantAt(point.l1, point.l2, zetas[index]);
    }
    return std::fabs(total);
}

// The Jacobian determinant of the six-node wedge at the point (L1, L2, zeta) of its reference element.
double wedge6Determinant(const Vec3 (&points)[6], double l1, double l2, double zeta) noexcept
{
    const double l[3] = {l1, l2, 1.0 - l1 - l2};
    Vec3 dl1;
    Vec3 dl2;
    Vec3 dzeta;
    for (std::size_t node = 0; node < 6; ++node)
    {
        const std::size_t corner = node % 3;
        const double side = wedgeSide(node);
        const double factor = (1.0 + zeta * side) / 2.0; // dN/dL_corner
        // L3 is not independent: its derivative counts against L1 and L2.
        if (corner == 0)
            dl1 += factor * points[node];
        else if (corner == 1)
            dl2 += factor * points[node];
        else
        {
            dl1 -= factor * points[node];
            dl2 -= factor * points[node];
        }
        dzeta += (l[corner] * side / 2.0) * points[node];
    }
    return determinant(dl1, dl2, dzeta);
}

double wedge6(const Vec3 (&points)[6]) noexcept
{
    return integrateWedge([&](double l1, double l2, double zeta) { return wedge6Determinant(points, l1, l2, zeta); },
                          false);
}

// The Jacobian determinant of the fifteen-node wedge at the point (L1, L2, zeta) of its reference element.
double wedge15Determinant(const Vec3 (&points)[15], double l1, double l2, double zeta) noexcept
{
    const double l[3] = {l1, l2, 1.0 - l1 - l2};
    Vec3 dl1;
    Vec3 dl2;
    Vec3 dzeta;
    const auto add = [&](std::size_t node, double dn0, double dn1, double dn2, double dnz)
    {
        // dN/dL1 and dN/dL2 when L3 = 1 - L1 - L2.
        dl1 += (dn0 - dn2) * points[node];
        dl2 += (dn1 - dn2) * points[node];
        dzeta += dnz * points[node];
    };
    for (std::size_t node = 0; node < 15; ++node)
    {
        double dn[3] = {0.0, 0.0, 0.0};
        double dnz = 0.0;
        if (node < 6)
        {
            // A corner: N = L (( 2 L - 1)(1 + zeta s) / 2 - (1 - zeta^2) / 2).
            const std::size_t corner = node % 3;
            const double side = wedgeSide(node);
            dn[corner] = (4.0 * l[corner] - 1.0) * (1.0 + zeta * side) / 2.0 - (1.0 - zeta * zeta) / 2.0;
            dnz = l[corner] * ((2.0 * l[corner] - 1.0) * side / 2.0 + zeta);
        }
        else if (node < 9 || node >= 12)
        {
            // A mid-side node of a triangular face: N = 2 L_i L_j (1 + zeta s).
            const std::size_t edge = node < 9 ? node - 6 : node - 12;
            const double side = node < 9 ? -1.0 : 1.0;
            const std::size_t i = edge;
            const std::size_t j = (edge + 1) % 3;
            dn[i] = 2.0 * l[j] * (1.0 + zeta * side);
            dn[j] = 2.0 * l[i] * (1.0 + zeta * side);
            dnz = 2.0 * l[i] * l[j] * side;
        }
        else
        {
            // A mid-side node of an edge between the two triangles: N = L (1 - zeta^2).
            const std::size_t corner = node - 9;
            dn[corner] = 1.0 - zeta * zeta;
            dnz = -2.0 * zeta * l[corner];
        }
        add(node, dn[0], dn[1], dn[2], dnz);
    }
    return determinant(dl1, dl2, dzeta);
}

double wedge15(const Vec3 (&points)[15]) noexcept
{
    return integrateWedge([&](double l1, double l2, double zeta) { return wedge15Determinant(points, l1, l2, zeta); },
                          true);
}

constexpr std::size_t WEDGE_EDGES[9][2] = {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 4}, {2, 5}, {3, 4}, {4, 5}, {5, 3}};

// --- the tetrahedron
// ---------------------------------------------------------------------------------------------------------

constexpr std::size_t TETRAHEDRON_EDGES[6][2] = {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}};

// The Jacobian determinant of the ten-node tetrahedron at the point with barycentric coordinates `l`, for the reference
// coordinates (L2, L3, L4): the determinant of the three edges at G1 for a straight-edged element, positive for the
// orientation of the format reference.
double tetrahedron10Determinant(const Vec3 (&points)[10], const double (&l)[4]) noexcept
{
    Vec3 d[4]; // dx / dL_k with the four coordinates independent
    for (std::size_t node = 0; node < 10; ++node)
    {
        if (node < 4)
        {
            d[node] += (4.0 * l[node] - 1.0) * points[node];
        }
        else
        {
            const std::size_t i = TETRAHEDRON_EDGES[node - 4][0];
            const std::size_t j = TETRAHEDRON_EDGES[node - 4][1];
            d[i] += (4.0 * l[j]) * points[node];
            d[j] += (4.0 * l[i]) * points[node];
        }
    }
    // L1 = 1 - L2 - L3 - L4.
    return determinant(d[1] - d[0], d[2] - d[0], d[3] - d[0]);
}

double tetrahedron10(const Vec3 (&points)[10]) noexcept
{
    // Five points, exact to degree 3 (the determinant of the quadratic tetrahedron is cubic); one weight is negative.
    struct Point
    {
        double l[4];
        double weight;
    };
    const Point rule[5] = {{{0.25, 0.25, 0.25, 0.25}, -4.0 / 5.0},
                           {{0.5, 1.0 / 6.0, 1.0 / 6.0, 1.0 / 6.0}, 9.0 / 20.0},
                           {{1.0 / 6.0, 0.5, 1.0 / 6.0, 1.0 / 6.0}, 9.0 / 20.0},
                           {{1.0 / 6.0, 1.0 / 6.0, 0.5, 1.0 / 6.0}, 9.0 / 20.0},
                           {{1.0 / 6.0, 1.0 / 6.0, 1.0 / 6.0, 0.5}, 9.0 / 20.0}};
    double total = 0.0;
    for (const Point& point : rule)
        total += point.weight * tetrahedron10Determinant(points, point.l);
    return std::fabs(total / 6.0);
}

} // namespace

double tetrahedronVolume(std::span<const Vec3> points)
{
    if (points.size() < 10 || !anyPresent(points, 4))
        return std::fabs(determinant(points[1] - points[0], points[2] - points[0], points[3] - points[0])) / 6.0;
    Vec3 full[10];
    for (std::size_t node = 0; node < 10; ++node)
        full[node] = points[node];
    for (std::size_t edge = 0; edge < 6; ++edge)
    {
        if (!present(full[4 + edge]))
            full[4 + edge] = middle(full[TETRAHEDRON_EDGES[edge][0]], full[TETRAHEDRON_EDGES[edge][1]]);
    }
    return tetrahedron10(full);
}

double wedgeVolume(std::span<const Vec3> points)
{
    if (points.size() < 15 || !anyPresent(points, 6))
    {
        Vec3 corners[6];
        for (std::size_t node = 0; node < 6; ++node)
            corners[node] = points[node];
        return wedge6(corners);
    }
    Vec3 full[15];
    for (std::size_t node = 0; node < 15; ++node)
        full[node] = points[node];
    for (std::size_t edge = 0; edge < 9; ++edge)
    {
        if (!present(full[6 + edge]))
            full[6 + edge] = middle(full[WEDGE_EDGES[edge][0]], full[WEDGE_EDGES[edge][1]]);
    }
    return wedge15(full);
}

double hexahedronVolume(std::span<const Vec3> points)
{
    if (points.size() < 20 || !anyPresent(points, 8))
    {
        Vec3 corners[8];
        for (std::size_t node = 0; node < 8; ++node)
            corners[node] = points[node];
        return hexahedron8(corners);
    }
    Vec3 full[20];
    for (std::size_t node = 0; node < 20; ++node)
        full[node] = points[node];
    for (std::size_t edge = 0; edge < 12; ++edge)
    {
        if (!present(full[8 + edge]))
            full[8 + edge] = middle(full[HEXAHEDRON_EDGES[edge][0]], full[HEXAHEDRON_EDGES[edge][1]]);
    }
    return hexahedron20(full);
}

double pyramidVolume(std::span<const Vec3> points)
{
    const Vec3& apex = points[4];
    if (points.size() < 13 || !anyPresent(points, 5))
    {
        const Vec3 corners[8] = {points[0], points[1], points[2], points[3], apex, apex, apex, apex};
        return hexahedron8(corners);
    }
    Vec3 full[20];
    for (std::size_t node = 0; node < 4; ++node)
        full[node] = points[node];
    for (std::size_t node = 4; node < 8; ++node)
        full[node] = apex;
    // G6..G9 on the base edges, G10..G13 on the edges to the apex; the edges of the collapsed face are the apex itself.
    for (std::size_t edge = 0; edge < 4; ++edge)
    {
        const Vec3& base = points[5 + edge];
        full[8 + edge] = present(base) ? base : middle(points[edge], points[(edge + 1) % 4]);
        const Vec3& rise = points[9 + edge];
        full[12 + edge] = present(rise) ? rise : middle(points[edge], apex);
        full[16 + edge] = apex;
    }
    return hexahedron20(full);
}

// --- the Jacobian at the corners and the centre
// ---------------------------------------------------------------------------------------

void tetrahedronJacobians(std::span<const Vec3> points, double (&out)[5])
{
    if (points.size() < 10 || !anyPresent(points, 4))
    {
        const double straight = determinant(points[1] - points[0], points[2] - points[0], points[3] - points[0]);
        std::fill(out, out + 5, straight);
        return;
    }
    Vec3 full[10];
    for (std::size_t node = 0; node < 10; ++node)
        full[node] = points[node];
    for (std::size_t edge = 0; edge < 6; ++edge)
    {
        if (!present(full[4 + edge]))
            full[4 + edge] = middle(full[TETRAHEDRON_EDGES[edge][0]], full[TETRAHEDRON_EDGES[edge][1]]);
    }
    const double samples[5][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}, {0.25, 0.25, 0.25, 0.25}};
    for (std::size_t sample = 0; sample < 5; ++sample)
        out[sample] = tetrahedron10Determinant(full, samples[sample]);
}

void wedgeJacobians(std::span<const Vec3> points, double (&out)[7])
{
    // The six corners (G1, G2, G3 at zeta = -1, G4, G5, G6 at zeta = 1) and the centre.
    const double corners[3][2] = {{1.0, 0.0}, {0.0, 1.0}, {0.0, 0.0}};
    const bool quadratic = points.size() >= 15 && anyPresent(points, 6);
    Vec3 full[15];
    if (quadratic)
    {
        for (std::size_t node = 0; node < 15; ++node)
            full[node] = points[node];
        for (std::size_t edge = 0; edge < 9; ++edge)
        {
            if (!present(full[6 + edge]))
                full[6 + edge] = middle(full[WEDGE_EDGES[edge][0]], full[WEDGE_EDGES[edge][1]]);
        }
    }
    Vec3 linear[6];
    for (std::size_t node = 0; node < 6; ++node)
        linear[node] = points[node];
    const auto at = [&](double l1, double l2, double zeta)
    { return quadratic ? wedge15Determinant(full, l1, l2, zeta) : wedge6Determinant(linear, l1, l2, zeta); };
    for (std::size_t corner = 0; corner < 6; ++corner)
        out[corner] = at(corners[corner % 3][0], corners[corner % 3][1], corner < 3 ? -1.0 : 1.0);
    out[6] = at(1.0 / 3.0, 1.0 / 3.0, 0.0);
}

namespace
{

// The determinants of the hexahedron whose twenty nodes are `full` at its eight corners and its centre.
void hexahedronSamples(const Vec3 (&full)[20], bool quadratic, double (&out)[9])
{
    Vec3 corners[8];
    for (std::size_t node = 0; node < 8; ++node)
        corners[node] = full[node];
    for (std::size_t corner = 0; corner < 8; ++corner)
    {
        const double* r = HEXAHEDRON_REFERENCE[corner];
        out[corner] = quadratic ? hexahedron20Determinant(full, r[0], r[1], r[2])
                                : hexahedron8Determinant(corners, r[0], r[1], r[2]);
    }
    out[8] = quadratic ? hexahedron20Determinant(full, 0.0, 0.0, 0.0) : hexahedron8Determinant(corners, 0.0, 0.0, 0.0);
}

} // namespace

void hexahedronJacobians(std::span<const Vec3> points, double (&out)[9])
{
    Vec3 full[20];
    const bool quadratic = points.size() >= 20 && anyPresent(points, 8);
    for (std::size_t node = 0; node < (quadratic ? 20U : 8U); ++node)
        full[node] = points[node];
    if (quadratic)
    {
        for (std::size_t edge = 0; edge < 12; ++edge)
        {
            if (!present(full[8 + edge]))
                full[8 + edge] = middle(full[HEXAHEDRON_EDGES[edge][0]], full[HEXAHEDRON_EDGES[edge][1]]);
        }
    }
    hexahedronSamples(full, quadratic, out);
}

void pyramidJacobians(std::span<const Vec3> points, double (&out)[4])
{
    // The pyramid is the hexahedron whose top face is collapsed to the apex, so the mapping is singular at the apex and
    // not uniform even for a regular pyramid (its centre has a quarter of the determinant of the corners of the base):
    // only the four corners of the base are sampled, where the determinant is that of the three edges there.
    const Vec3& apex = points[4];
    const bool quadratic = points.size() >= 13 && anyPresent(points, 5);
    Vec3 full[20];
    for (std::size_t node = 0; node < 4; ++node)
        full[node] = points[node];
    for (std::size_t node = 4; node < 8; ++node)
        full[node] = apex;
    if (quadratic)
    {
        for (std::size_t edge = 0; edge < 4; ++edge)
        {
            const Vec3& base = points[5 + edge];
            full[8 + edge] = present(base) ? base : middle(points[edge], points[(edge + 1) % 4]);
            const Vec3& rise = points[9 + edge];
            full[12 + edge] = present(rise) ? rise : middle(points[edge], apex);
            full[16 + edge] = apex;
        }
    }
    double samples[9];
    hexahedronSamples(full, quadratic, samples);
    for (std::size_t corner = 0; corner < 4; ++corner)
        out[corner] = samples[corner];
}

} // namespace oor::analysis::detail
