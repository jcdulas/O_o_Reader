#include "QualityShapes.hpp"
#include "SolidVolume.hpp"

// clang-format off
// The tetrahedron: four corners and up to six mid-side nodes.
//
//   aspect ratio   longest edge over shortest edge (format reference GEOMCHECK TET_AR)
//   interior angles  the angles of the four triangular faces
//   collapse       the smallest height of a vertex over its opposite face, over the height of the regular
//                  tetrahedron whose faces have that area (sqrt(2/3) times the side of the equilateral triangle
//                  of the same area): 1 for a regular tetrahedron, 0 for a flat one (a sliver or a cap)
//   Jacobian       the determinant of the mapping of the reference tetrahedron (L2, L3, L4) at the four corners
//                  and the centre: six times the volume for a straight-edged element, where it is the same
//                  everywhere; negative when the element is connected the other way round
//   edge points    the position of the mid-side nodes along their edges and the angle they make (TET_EPLR,
//                  TET_EPIA)
// clang-format on

namespace oor::analysis::detail
{

QualityValues tetrahedronQuality(std::span<const Vec3> points)
{
    QualityValues values;
    for (std::size_t corner = 0; corner < 4; ++corner)
    {
        if (!isPresent(points[corner]))
            return values;
    }
    solidSkeleton(ElementShape::Tetrahedron, points.first(4), values);

    double determinants[5];
    tetrahedronJacobians(points, determinants);
    jacobianStatistics(determinants, values);

    // The collapse: the height of vertex i over the face opposite to it is 3 V / A_i = |6V| / (2 A_i).
    const double sixVolumes =
        std::fabs(dot(points[1] - points[0], cross(points[2] - points[0], points[3] - points[0])));
    double collapse = std::numeric_limits<double>::infinity();
    for (std::size_t vertex = 0; vertex < 4; ++vertex)
    {
        const Vec3& first = points[(vertex + 1) % 4];
        const Vec3& second = points[(vertex + 2) % 4];
        const Vec3& third = points[(vertex + 3) % 4];
        const double faceArea = 0.5 * base::norm(cross(second - first, third - first));
        if (!(faceArea > 0.0))
        {
            collapse = 0.0;
            break;
        }
        const double height = sixVolumes / (2.0 * faceArea);
        const double regularHeight = std::sqrt(2.0 / 3.0) * std::sqrt(4.0 * faceArea / std::sqrt(3.0));
        collapse = std::min(collapse, height / regularHeight);
    }
    values.collapse = collapse;

    solidEdgePoints(tetrahedronMidSideEdges(), points, values);
    return values;
}

} // namespace oor::analysis::detail
