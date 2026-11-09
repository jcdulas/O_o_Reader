#include "ClosestPoint.hpp"

#include <algorithm>

namespace oor::analysis::detail
{

namespace
{

base::Vec3 closestOnSegment(const base::Vec3& start, const base::Vec3& end, const base::Vec3& point) noexcept
{
    const base::Vec3 along = end - start;
    const double squared = base::dot(along, along);
    if (squared <= 0.0)
        return start;
    const double fraction = std::clamp(base::dot(point - start, along) / squared, 0.0, 1.0);
    return start + along * fraction;
}

// The closest point of the triangle (a, b, c) by the regions of its Voronoi diagram (C. Ericson, Real-Time Collision
// Detection, 5.1.5): a vertex, an edge or the inside.
base::Vec3 closestOnTriangle(const base::Vec3& a, const base::Vec3& b, const base::Vec3& c,
                             const base::Vec3& point) noexcept
{
    const base::Vec3 ab = b - a;
    const base::Vec3 ac = c - a;
    const base::Vec3 ap = point - a;
    const double d1 = base::dot(ab, ap);
    const double d2 = base::dot(ac, ap);
    if (d1 <= 0.0 && d2 <= 0.0)
        return a;
    const base::Vec3 bp = point - b;
    const double d3 = base::dot(ab, bp);
    const double d4 = base::dot(ac, bp);
    if (d3 >= 0.0 && d4 <= d3)
        return b;
    const double vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0)
        return a + ab * (d1 / (d1 - d3));
    const base::Vec3 cp = point - c;
    const double d5 = base::dot(ab, cp);
    const double d6 = base::dot(ac, cp);
    if (d6 >= 0.0 && d5 <= d6)
        return c;
    const double vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0)
        return a + ac * (d2 / (d2 - d6));
    const double va = d3 * d6 - d5 * d4;
    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0)
        return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
    const double total = va + vb + vc;
    if (total <= 0.0) // a degenerate triangle: its longest side stands for it
        return closestOnSegment(a, b, point);
    return a + ab * (vb / total) + ac * (vc / total);
}

} // namespace

ClosestPoint closestPointOnFace(const std::array<base::Vec3, 4>& corners, std::size_t count,
                                const base::Vec3& point) noexcept
{
    base::Vec3 best;
    if (count == 2)
        best = closestOnSegment(corners[0], corners[1], point);
    else if (count == 3)
        best = closestOnTriangle(corners[0], corners[1], corners[2], point);
    else
    {
        const base::Vec3 centre = (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25;
        double bestSquared = -1.0;
        for (std::size_t side = 0; side < 4; ++side)
        {
            const base::Vec3 candidate = closestOnTriangle(centre, corners[side], corners[(side + 1) % 4], point);
            const base::Vec3 gap = candidate - point;
            const double squared = base::dot(gap, gap);
            if (bestSquared < 0.0 || squared < bestSquared)
            {
                bestSquared = squared;
                best = candidate;
            }
        }
    }
    return ClosestPoint{best, base::norm(best - point)};
}

} // namespace oor::analysis::detail
