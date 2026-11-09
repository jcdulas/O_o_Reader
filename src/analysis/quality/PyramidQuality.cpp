#include "QualityShapes.hpp"
#include "SolidVolume.hpp"

// clang-format off
// The pyramid: four corners of the base, the apex and up to eight mid-side nodes.
//
//   aspect ratio   longest edge over shortest edge (format reference GEOMCHECK PYR_AR)
//   interior angles  the angles of the quadrilateral base and the four triangular faces
//   face warp      the cosine between the normals at diagonally opposite corners of the base (PYR_WARP)
//   Jacobian       the pyramid is the hexahedron whose top face is collapsed to the apex, so the mapping is
//                  singular at the apex: the determinant is taken at the four corners of the base (it is the
//                  determinant of the three edges there, the same four numbers for a symmetric pyramid)
//   edge points    PYR_EPLR, PYR_EPIA
// clang-format on

namespace oor::analysis::detail
{

QualityValues pyramidQuality(std::span<const Vec3> points)
{
    QualityValues values;
    for (std::size_t corner = 0; corner < 5; ++corner)
    {
        if (!isPresent(points[corner]))
            return values;
    }
    solidSkeleton(ElementShape::Pyramid, points.first(5), values);

    double determinants[4];
    pyramidJacobians(points, determinants);
    jacobianStatistics(determinants, values);

    solidEdgePoints(pyramidMidSideEdges(), points, values);
    return values;
}

} // namespace oor::analysis::detail
