#include "QualityShapes.hpp"
#include "SolidVolume.hpp"

// clang-format off
// The hexahedron: eight corners and up to twelve mid-side nodes.
//
//   aspect ratio   longest edge over shortest edge (format reference GEOMCHECK HEX_AR)
//   interior angles  the angles of the six quadrilateral faces
//   face warp      the smallest cosine between the normals at diagonally opposite corners of a face (HEX_WARP)
//   Jacobian       the determinant of the mapping of the cube [-1, 1]^3 (trilinear, or the twenty-node
//                  serendipity functions when mid-side nodes are there) at the eight corners and the centre: an
//                  eighth of the volume for a box, where it is the same everywhere; negative at a corner where
//                  the element is inverted
//   edge points    HEX_EPLR, HEX_EPIA
// clang-format on

namespace oor::analysis::detail
{

QualityValues hexahedronQuality(std::span<const Vec3> points)
{
    QualityValues values;
    for (std::size_t corner = 0; corner < 8; ++corner)
    {
        if (!isPresent(points[corner]))
            return values;
    }
    solidSkeleton(ElementShape::Hexahedron, points.first(8), values);

    double determinants[9];
    hexahedronJacobians(points, determinants);
    jacobianStatistics(determinants, values);

    solidEdgePoints(hexahedronMidSideEdges(), points, values);
    return values;
}

} // namespace oor::analysis::detail
