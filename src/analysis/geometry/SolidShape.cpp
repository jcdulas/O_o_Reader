#include "SolidShape.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <utility>

namespace oor::analysis::detail
{

using base::Vec3;

namespace
{

// The corners and mid-side points of the hexahedron in its natural coordinates (the order of the CHEXA card).
constexpr double HEXAHEDRON[20][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1},  {-1, 1, -1}, {-1, -1, 1},
                                      {1, -1, 1},   {1, 1, 1},   {-1, 1, 1},  {0, -1, -1}, {1, 0, -1},
                                      {0, 1, -1},   {-1, 0, -1}, {-1, -1, 0}, {1, -1, 0},  {1, 1, 0},
                                      {-1, 1, 0},   {0, -1, 1},  {1, 0, 1},   {0, 1, 1},   {-1, 0, 1}};

// The two corners of the edge of each mid-side slot, in the order of the card (format reference, SolidVolume.hpp).
using Edge = std::pair<std::size_t, std::size_t>;
constexpr Edge TETRAHEDRON_EDGES[6] = {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}};
constexpr Edge PENTAHEDRON_EDGES[9] = {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 4}, {2, 5}, {3, 4}, {4, 5}, {5, 3}};
constexpr Edge HEXAHEDRON_EDGES[12] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 5},
                                       {2, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}, {7, 4}};
constexpr Edge PYRAMID_EDGES[8] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 4}, {2, 4}, {3, 4}};

using Weights = std::array<double, MAX_SOLID_NODES>;

// The 8-node (linear) or 20-node (quadratic) hexahedron functions.
void hexahedron(const Vec3& at, bool quadratic, double (&out)[20])
{
    std::fill(std::begin(out), std::end(out), 0.0);
    for (std::size_t node = 0; node < (quadratic ? 20U : 8U); ++node)
    {
        const double* r = HEXAHEDRON[node];
        const double a = 1.0 + at.x * r[0];
        const double b = 1.0 + at.y * r[1];
        const double c = 1.0 + at.z * r[2];
        if (!quadratic)
            out[node] = a * b * c / 8.0;
        else if (node < 8)
            out[node] = a * b * c * (at.x * r[0] + at.y * r[1] + at.z * r[2] - 2.0) / 8.0;
        else if (r[0] == 0.0)
            out[node] = (1.0 - at.x * at.x) * b * c / 4.0;
        else if (r[1] == 0.0)
            out[node] = a * (1.0 - at.y * at.y) * c / 4.0;
        else
            out[node] = a * b * (1.0 - at.z * at.z) / 4.0;
    }
}

bool anyMidSide(const SolidElement& element)
{
    for (std::size_t slot = element.cornerCount(); slot < element.slotCount(); ++slot)
    {
        if (element.hasNode(slot))
            return true;
    }
    return false;
}

// The functions of every slot, the missing mid-side points included (as if they stood at the middle of their edge).
Weights rawWeights(const SolidElement& element, const Vec3& at, bool quadratic)
{
    Weights weights{};
    switch (element.kind)
    {
    case SolidShapeKind::Tetrahedron:
    {
        const double l[4] = {1.0 - at.x - at.y - at.z, at.x, at.y, at.z};
        for (std::size_t corner = 0; corner < 4; ++corner)
            weights[corner] = quadratic ? l[corner] * (2.0 * l[corner] - 1.0) : l[corner];
        for (std::size_t edge = 0; quadratic && edge < 6; ++edge)
            weights[4 + edge] = 4.0 * l[TETRAHEDRON_EDGES[edge].first] * l[TETRAHEDRON_EDGES[edge].second];
        break;
    }
    case SolidShapeKind::Pentahedron:
    {
        const double l[3] = {1.0 - at.x - at.y, at.x, at.y};
        const double below = 1.0 - at.z;
        const double above = 1.0 + at.z;
        for (std::size_t corner = 0; corner < 3; ++corner)
        {
            if (!quadratic)
            {
                weights[corner] = 0.5 * l[corner] * below;
                weights[corner + 3] = 0.5 * l[corner] * above;
                continue;
            }
            const double bubble = 0.5 * l[corner] * (1.0 - at.z * at.z);
            weights[corner] = 0.5 * l[corner] * (2.0 * l[corner] - 1.0) * below - bubble;
            weights[corner + 3] = 0.5 * l[corner] * (2.0 * l[corner] - 1.0) * above - bubble;
        }
        if (quadratic)
        {
            // G7 to G9 on the first triangle, G10 to G12 between the triangles, G13 to G15 on the second one
            for (std::size_t edge = 0; edge < 3; ++edge)
            {
                const std::size_t first = edge;
                const std::size_t second = (edge + 1) % 3;
                weights[6 + edge] = 2.0 * l[first] * l[second] * below;
                weights[9 + edge] = l[edge] * (1.0 - at.z * at.z);
                weights[12 + edge] = 2.0 * l[first] * l[second] * above;
            }
        }
        break;
    }
    case SolidShapeKind::Hexahedron:
    {
        double n[20];
        hexahedron(at, quadratic, n);
        std::copy(std::begin(n), std::end(n), weights.begin());
        break;
    }
    case SolidShapeKind::Pyramid:
    {
        // The hexahedron with its top face collapsed to the apex: the four top corners and the four top edges are the
        // apex; the base edges are G6 to G9, the edges to the apex G10 to G13.
        double n[20];
        hexahedron(at, quadratic, n);
        for (std::size_t corner = 0; corner < 4; ++corner)
            weights[corner] = n[corner];
        weights[4] = n[4] + n[5] + n[6] + n[7] + n[16] + n[17] + n[18] + n[19];
        for (std::size_t edge = 0; quadratic && edge < 8; ++edge)
            weights[5 + edge] = n[8 + edge];
        break;
    }
    }
    return weights;
}

std::span<const Edge> edgesOf(SolidShapeKind kind) noexcept
{
    switch (kind)
    {
    case SolidShapeKind::Tetrahedron:
        return TETRAHEDRON_EDGES;
    case SolidShapeKind::Pentahedron:
        return PENTAHEDRON_EDGES;
    case SolidShapeKind::Hexahedron:
        return HEXAHEDRON_EDGES;
    case SolidShapeKind::Pyramid:
        return PYRAMID_EDGES;
    }
    return {};
}

Vec3 mapped(const SolidElement& element, const Vec3& natural)
{
    const Weights weights = solidWeights(element, natural);
    Vec3 point;
    for (std::size_t slot = 0; slot < element.slotCount(); ++slot)
        point += weights[slot] * element.points[slot];
    return point;
}

// How far natural coordinates are outside the element (0 inside).
double outsideOf(SolidShapeKind kind, const Vec3& at) noexcept
{
    switch (kind)
    {
    case SolidShapeKind::Tetrahedron:
        return std::max({0.0, -at.x, -at.y, -at.z, at.x + at.y + at.z - 1.0});
    case SolidShapeKind::Pentahedron:
        return std::max({0.0, -at.x, -at.y, at.x + at.y - 1.0, std::fabs(at.z) - 1.0});
    case SolidShapeKind::Hexahedron:
    case SolidShapeKind::Pyramid:
        break;
    }
    return std::max({0.0, std::fabs(at.x) - 1.0, std::fabs(at.y) - 1.0, std::fabs(at.z) - 1.0});
}

// Natural coordinates brought back inside the element.
Vec3 clampedInside(SolidShapeKind kind, Vec3 at) noexcept
{
    const auto clampUnit = [](double value) { return std::clamp(value, -1.0, 1.0); };
    switch (kind)
    {
    case SolidShapeKind::Tetrahedron:
    {
        at = {std::max(at.x, 0.0), std::max(at.y, 0.0), std::max(at.z, 0.0)};
        const double sum = at.x + at.y + at.z;
        return sum > 1.0 ? at * (1.0 / sum) : at;
    }
    case SolidShapeKind::Pentahedron:
    {
        Vec3 inside{std::max(at.x, 0.0), std::max(at.y, 0.0), clampUnit(at.z)};
        const double sum = inside.x + inside.y;
        if (sum > 1.0)
        {
            inside.x /= sum;
            inside.y /= sum;
        }
        return inside;
    }
    case SolidShapeKind::Hexahedron:
    case SolidShapeKind::Pyramid:
        break;
    }
    return {clampUnit(at.x), clampUnit(at.y), clampUnit(at.z)};
}

Vec3 centreOf(SolidShapeKind kind) noexcept
{
    switch (kind)
    {
    case SolidShapeKind::Tetrahedron:
        return {0.25, 0.25, 0.25};
    case SolidShapeKind::Pentahedron:
        return {1.0 / 3.0, 1.0 / 3.0, 0.0};
    case SolidShapeKind::Pyramid:
        return {0.0, 0.0, -0.5}; // the apex is a singular point of the mapping: start below it
    case SolidShapeKind::Hexahedron:
        break;
    }
    return {0.0, 0.0, 0.0};
}

// Solves the 3 x 3 system with columns a, b, c and right-hand side r (Cramer's rule); false for a singular matrix.
bool solve(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& r, Vec3& x) noexcept
{
    const double determinant = base::dot(a, base::cross(b, c));
    const double scale = base::norm(a) * base::norm(b) * base::norm(c);
    if (!(std::fabs(determinant) > 1e-14 * scale))
        return false;
    x = {base::dot(r, base::cross(b, c)) / determinant, base::dot(a, base::cross(r, c)) / determinant,
         base::dot(a, base::cross(b, r)) / determinant};
    return true;
}

} // namespace

std::size_t SolidElement::cornerCount() const noexcept
{
    switch (kind)
    {
    case SolidShapeKind::Tetrahedron:
        return 4;
    case SolidShapeKind::Pentahedron:
        return 6;
    case SolidShapeKind::Pyramid:
        return 5;
    case SolidShapeKind::Hexahedron:
        break;
    }
    return 8;
}

std::size_t SolidElement::slotCount() const noexcept
{
    return cornerCount() + edgesOf(kind).size();
}

Weights solidWeights(const SolidElement& element, const Vec3& natural)
{
    const bool quadratic = anyMidSide(element);
    Weights weights = rawWeights(element, natural, quadratic);
    if (!quadratic)
        return weights;
    // A mid-side point that is not there: its function goes half to each corner of its edge (the edge stays straight).
    const std::span<const Edge> edges = edgesOf(element.kind);
    for (std::size_t edge = 0; edge < edges.size(); ++edge)
    {
        const std::size_t slot = element.cornerCount() + edge;
        if (element.hasNode(slot))
            continue;
        weights[edges[edge].first] += 0.5 * weights[slot];
        weights[edges[edge].second] += 0.5 * weights[slot];
        weights[slot] = 0.0;
    }
    return weights;
}

SolidLocation locateInSolid(const SolidElement& element, const Vec3& point)
{
    constexpr int MAX_ITERATIONS = 50;
    constexpr double STEP = 1e-7;       // of the finite differences of the Jacobian, in natural units
    constexpr double CONVERGED = 1e-12; // natural units
    constexpr double BOUND = 3.0;       // the natural coordinates stay within it while Newton's method runs
    Vec3 at = centreOf(element.kind);
    for (int iteration = 0; iteration < MAX_ITERATIONS; ++iteration)
    {
        const Vec3 residual = point - mapped(element, at);
        // the Jacobian by central differences of the mapping (the shape functions are polynomials of degree 2 at most)
        const auto column = [&](const Vec3& direction)
        { return (mapped(element, at + direction * STEP) - mapped(element, at - direction * STEP)) * (0.5 / STEP); };
        Vec3 step;
        if (!solve(column({1, 0, 0}), column({0, 1, 0}), column({0, 0, 1}), residual, step))
            break;
        at = {std::clamp(at.x + step.x, -BOUND, BOUND), std::clamp(at.y + step.y, -BOUND, BOUND),
              std::clamp(at.z + step.z, -BOUND, BOUND)};
        if (base::norm(step) < CONVERGED)
            break;
    }
    SolidLocation found;
    found.natural = at;
    found.outside = outsideOf(element.kind, at);
    found.distance = found.outside > 0.0 ? base::norm(point - mapped(element, clampedInside(element.kind, at))) : 0.0;
    return found;
}

} // namespace oor::analysis::detail
