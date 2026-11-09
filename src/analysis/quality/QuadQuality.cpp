#include "QualityShapes.hpp"

// clang-format off
// The quadrilateral: four corners and up to four mid-side nodes.
//
//   aspect ratio   longest side over shortest side
//   interior angles  the four angles at the corners between the two edges that meet there (in space: a warped
//                  element is not flat), measured inside the element: a concave corner has more than 180 degrees
//   skew           90 degrees less the smaller angle between the two lines that join the middles of opposite
//                  sides; 0 for a rectangle
//   taper          the largest |area of the triangle at a corner over half the area of the element - 1| (format reference
//                  GEOMCHECK); the triangle at a corner is that corner and its two neighbours, the area of the
//                  element is half the cross product of the diagonals; 0 for a parallelogram, |a - b| / (a + b)
//                  for a trapezoid of parallel sides a and b
//   warp           the largest distance of a corner from the mean plane (through the centre of the corners,
//                  perpendicular to the cross product of the diagonals) over the mean length of the diagonals
//                  (format reference GEOMCHECK); 0 for a flat element
//   Jacobian       the mapping of the square [-1, 1]^2 by the eight-node shape functions (the four-node ones
//                  when the mid-side nodes are missing), at the four corners and the centre; the determinant is
//                  the cross product of the tangents along the normal of the diagonals: a quarter of the area
//                  for a straight-edged parallelogram
// clang-format on

namespace oor::analysis::detail
{

namespace
{

// The Jacobian determinants at the four corners and the centre.
void quadrilateralJacobians(const Vec3 (&corner)[4], const Vec3 (&middle)[4], const Vec3& normal, double (&out)[5])
{
    constexpr double REFERENCE[8][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}, {0, -1}, {1, 0}, {0, 1}, {-1, 0}};
    Vec3 full[8];
    for (std::size_t index = 0; index < 4; ++index)
    {
        full[index] = corner[index];
        full[4 + index] = isPresent(middle[index]) ? middle[index] : 0.5 * (corner[index] + corner[(index + 1) % 4]);
    }
    for (std::size_t sample = 0; sample < 5; ++sample)
    {
        const double xi = sample < 4 ? REFERENCE[sample][0] : 0.0;
        const double eta = sample < 4 ? REFERENCE[sample][1] : 0.0;
        Vec3 dxi;
        Vec3 deta;
        for (std::size_t node = 0; node < 8; ++node)
        {
            const double a = REFERENCE[node][0];
            const double b = REFERENCE[node][1];
            double dnDxi = 0.0;
            double dnDeta = 0.0;
            if (node < 4)
            {
                dnDxi = 0.25 * a * (1.0 + b * eta) * (2.0 * a * xi + b * eta);
                dnDeta = 0.25 * b * (1.0 + a * xi) * (a * xi + 2.0 * b * eta);
            }
            else if (a == 0.0)
            {
                dnDxi = -xi * (1.0 + b * eta);
                dnDeta = 0.5 * b * (1.0 - xi * xi);
            }
            else
            {
                dnDxi = 0.5 * a * (1.0 - eta * eta);
                dnDeta = -eta * (1.0 + a * xi);
            }
            dxi += dnDxi * full[node];
            deta += dnDeta * full[node];
        }
        out[sample] = dot(normal, cross(dxi, deta));
    }
}

} // namespace

QualityValues quadrilateralQuality(const Vec3 (&corner)[4], const Vec3 (&middle)[4])
{
    QualityValues values;
    for (const Vec3& point : corner)
    {
        if (!isPresent(point))
            return values;
    }
    Range lengths;
    for (std::size_t index = 0; index < 4; ++index)
        lengths.add(distanceBetween(corner[index], corner[(index + 1) % 4]));
    values.minEdge = lengths.min();
    values.maxEdge = lengths.max();
    values.aspectRatio = ratioOf(lengths.max(), lengths.min());

    const Vec3 diagonal13 = corner[2] - corner[0];
    const Vec3 diagonal24 = corner[3] - corner[1];
    const Vec3 areaVector = cross(diagonal13, diagonal24); // twice the area, along the normal
    const double areaScale = base::norm(areaVector);
    const Vec3 normal = areaScale > 0.0 ? areaVector / areaScale : undefinedVector();

    // The interior angles, signed by the side of the normal the corner turns to (a reflex corner is more than 180
    // degrees).
    if (areaScale > 0.0)
    {
        Range angles;
        for (std::size_t index = 0; index < 4; ++index)
        {
            const Vec3 toNext = corner[(index + 1) % 4] - corner[index];
            const Vec3 toLast = corner[(index + 3) % 4] - corner[index];
            if (!(base::norm(toNext) > 0.0) || !(base::norm(toLast) > 0.0))
                continue;
            // The angle between the two edges in space (a warped element is not flat), turned past 180 degrees when the
            // corner is reflex: the side of the normal the edges turn to gives the sign.
            const Vec3 turn = cross(toNext, toLast);
            const double sine = dot(normal, turn) >= 0.0 ? base::norm(turn) : -base::norm(turn);
            double angle = std::atan2(sine, dot(toNext, toLast)) * RADIANS_TO_DEGREES;
            if (angle < 0.0)
                angle += 360.0;
            angles.add(angle);
        }
        values.minAngle = angles.min();
        values.maxAngle = angles.max();
    }

    // The skew: the angle between the lines through the middles of opposite sides, taken as the smaller of the two it
    // makes.
    const Vec3 acrossFirst = 0.5 * (corner[2] + corner[3]) - 0.5 * (corner[0] + corner[1]);
    const Vec3 acrossSecond = 0.5 * (corner[1] + corner[2]) - 0.5 * (corner[3] + corner[0]);
    const double between = angleBetween(acrossFirst, acrossSecond);
    values.skew = 90.0 - std::min(between, 180.0 - between);

    if (areaScale > 0.0)
    {
        // The taper.
        double taper = 0.0;
        for (std::size_t index = 0; index < 4; ++index)
        {
            const Vec3 toNext = corner[(index + 1) % 4] - corner[index];
            const Vec3 toLast = corner[(index + 3) % 4] - corner[index];
            const double triangle = base::norm(cross(toNext, toLast));
            taper = std::max(taper, std::fabs(triangle / (0.5 * areaScale) - 1.0));
        }
        values.taper = taper;

        // The warp: the distance of the corners from the mean plane over the mean length of the diagonals.
        const Vec3 center = 0.25 * (corner[0] + corner[1] + corner[2] + corner[3]);
        double height = 0.0;
        for (const Vec3& point : corner)
            height = std::max(height, std::fabs(dot(normal, point - center)));
        values.warp = height / (0.5 * (base::norm(diagonal13) + base::norm(diagonal24)));

        double determinants[5];
        quadrilateralJacobians(corner, middle, normal, determinants);
        jacobianStatistics(determinants, values);
    }

    EdgePointStatistics edgePoints;
    for (std::size_t side = 0; side < 4; ++side)
        edgePoints.add(corner[side], corner[(side + 1) % 4], middle[side]);
    values.edgePointLengthRatio = edgePoints.lengthRatio();
    values.edgePointAngle = edgePoints.angle();
    return values;
}

} // namespace oor::analysis::detail
