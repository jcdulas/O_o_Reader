#pragma once

#include "QualityMath.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"

// The quality of each element shape: one function per shape family, in a file of its own. The points are those of the
// nodes of the card in the order of the card (corners first, then the mid-side nodes; a mid-side node that is blank is
// NaN). Each function returns NaN metrics for an element whose corners are not all there.

namespace oor::analysis::detail
{

// The three corners and the mid-side nodes of the sides G1-G2, G2-G3, G3-G1.
[[nodiscard]] QualityValues triangleQuality(const Vec3 (&corner)[3], const Vec3 (&middle)[3]);
// The four corners and the mid-side nodes of the sides G1-G2, G2-G3, G3-G4, G4-G1.
[[nodiscard]] QualityValues quadrilateralQuality(const Vec3 (&corner)[4], const Vec3 (&middle)[4]);

// A solid: the points of its card (4 or 10, 6 or 15, 8 or 20, 5 or 13).
[[nodiscard]] QualityValues tetrahedronQuality(std::span<const Vec3> points);
[[nodiscard]] QualityValues pentahedronQuality(std::span<const Vec3> points);
[[nodiscard]] QualityValues hexahedronQuality(std::span<const Vec3> points);
[[nodiscard]] QualityValues pyramidQuality(std::span<const Vec3> points);

// What the four solids share: the edge lengths, the aspect ratio, the angles of the faces and the warp of the
// quadrilateral faces, from the corners of an element of this shape (the templates of ElementTopology give its edges
// and faces).
void solidSkeleton(ElementShape shape, std::span<const Vec3> corners, QualityValues& values);

// The corner pairs of the mid-side nodes of a solid in the order of its card: node `first + i` lies on the edge
// pairs[i].
struct MidSideEdges
{
    std::size_t first;
    std::span<const std::array<std::uint8_t, 2>> pairs;
};
[[nodiscard]] MidSideEdges tetrahedronMidSideEdges() noexcept;
[[nodiscard]] MidSideEdges pentahedronMidSideEdges() noexcept;
[[nodiscard]] MidSideEdges hexahedronMidSideEdges() noexcept;
[[nodiscard]] MidSideEdges pyramidMidSideEdges() noexcept;

// The edge point metrics of the mid-side nodes in `points` (NaN when there are none).
void solidEdgePoints(const MidSideEdges& edges, std::span<const Vec3> points, QualityValues& values);

} // namespace oor::analysis::detail
