#include "QualityShapes.hpp"

// clang-format off
// The triangle: three corners and up to three mid-side nodes.
//
//   aspect ratio   longest side over shortest side
//   interior angles  the three angles at the corners
//   skew           90 degrees less the smallest angle between a median and the side it falls on, so that an
//                  equilateral triangle has 0 (the quadrilateral's rule on the middles of the sides: the line
//                  from a vertex to the middle of the opposite side against the line that joins the middles of
//                  the other two sides). The format reference calls the smallest vertex angle the skew of a triangle
//                  (GEOMCHECK T3_SKEW): that is `minAngle`.
//   Jacobian       the mapping of the reference triangle (L2, L3) by the quadratic shape functions, at the three
//                  corners and the centroid; its determinant is the cross product of the two tangents along the
//                  normal of the corners (twice the area for a straight-edged triangle, which has the same value
//                  at all four points)
//   warp, taper    do not apply to a triangle (NaN)
// clang-format on

namespace oor::analysis::detail
{

namespace
{

// The Jacobian determinants at the three corners and the centroid. A mid-side node that is not there is put at the
// middle of its side, which is the straight-edged triangle.
void triangleJacobians(const Vec3 (&corner)[3], const Vec3 (&middle)[3], const Vec3& normal, double (&out)[4])
{
    Vec3 full[6];
    for (std::size_t index = 0; index < 3; ++index)
    {
        full[index] = corner[index];
        full[3 + index] = isPresent(middle[index]) ? middle[index] : 0.5 * (corner[index] + corner[(index + 1) % 3]);
    }
    const double samples[4][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0}};
    for (std::size_t sample = 0; sample < 4; ++sample)
    {
        const double* l = samples[sample];
        // dx/dL_k with the three coordinates independent; L1 = 1 - L2 - L3 afterwards.
        Vec3 d[3];
        for (std::size_t node = 0; node < 3; ++node)
            d[node] += (4.0 * l[node] - 1.0) * full[node];
        for (std::size_t side = 0; side < 3; ++side)
        {
            const std::size_t i = side;
            const std::size_t j = (side + 1) % 3;
            d[i] += (4.0 * l[j]) * full[3 + side];
            d[j] += (4.0 * l[i]) * full[3 + side];
        }
        out[sample] = dot(normal, cross(d[1] - d[0], d[2] - d[0]));
    }
}

} // namespace

QualityValues triangleQuality(const Vec3 (&corner)[3], const Vec3 (&middle)[3])
{
    QualityValues values;
    if (!isPresent(corner[0]) || !isPresent(corner[1]) || !isPresent(corner[2]))
        return values;
    const double sides[3] = {distanceBetween(corner[0], corner[1]), distanceBetween(corner[1], corner[2]),
                             distanceBetween(corner[2], corner[0])};
    Range lengths;
    Range angles;
    for (std::size_t index = 0; index < 3; ++index)
    {
        lengths.add(sides[index]);
        angles.add(angleBetween(corner[(index + 1) % 3] - corner[index], corner[(index + 2) % 3] - corner[index]));
    }
    values.minEdge = lengths.min();
    values.maxEdge = lengths.max();
    values.aspectRatio = ratioOf(lengths.max(), lengths.min());
    values.minAngle = angles.min();
    values.maxAngle = angles.max();

    // The skew: the median from each vertex against the opposite side.
    Range skews;
    for (std::size_t index = 0; index < 3; ++index)
    {
        const Vec3& vertex = corner[index];
        const Vec3& next = corner[(index + 1) % 3];
        const Vec3& last = corner[(index + 2) % 3];
        const double angle = angleBetween(0.5 * (next + last) - vertex, last - next);
        skews.add(90.0 - std::min(angle, 180.0 - angle));
    }
    values.skew = skews.max();

    const Vec3 areaVector = cross(corner[1] - corner[0], corner[2] - corner[0]);
    const double areaScale = base::norm(areaVector);
    if (areaScale > 0.0)
    {
        double determinants[4];
        triangleJacobians(corner, middle, areaVector / areaScale, determinants);
        jacobianStatistics(determinants, values);
    }

    EdgePointStatistics edgePoints;
    for (std::size_t side = 0; side < 3; ++side)
        edgePoints.add(corner[side], corner[(side + 1) % 3], middle[side]);
    values.edgePointLengthRatio = edgePoints.lengthRatio();
    values.edgePointAngle = edgePoints.angle();
    return values;
}

} // namespace oor::analysis::detail
