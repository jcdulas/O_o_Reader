#include "QualityShapes.hpp"
#include "SolidVolume.hpp"

// clang-format off
// The pentahedron (wedge): six corners and up to nine mid-side nodes.
//
//   aspect ratio   longest edge over shortest edge (format reference GEOMCHECK PEN_AR)
//   interior angles  the angles of the two triangular and the three quadrilateral faces
//   face warp      the smallest cosine between the normals at diagonally opposite corners of a quadrilateral
//                  face (PEN_WARP)
//   Jacobian       the determinant of the mapping of the reference wedge (L1, L2 and zeta in [-1, 1], of volume
//                  1) at the six corners and the centre: the volume for a straight prism, where it is the same
//                  everywhere
//   edge points    PEN_EPLR, PEN_EPIA
// clang-format on

namespace oor::analysis::detail
{

QualityValues pentahedronQuality(std::span<const Vec3> points)
{
    QualityValues values;
    for (std::size_t corner = 0; corner < 6; ++corner)
    {
        if (!isPresent(points[corner]))
            return values;
    }
    solidSkeleton(ElementShape::Pentahedron, points.first(6), values);

    double determinants[7];
    wedgeJacobians(points, determinants);
    jacobianStatistics(determinants, values);

    solidEdgePoints(pentahedronMidSideEdges(), points, values);
    return values;
}

} // namespace oor::analysis::detail
