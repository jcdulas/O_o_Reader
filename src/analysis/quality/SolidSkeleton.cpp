#include "QualityShapes.hpp"

// What the four solid shapes have in common: the metrics of their corners (edges, angles of the faces, warp of the
// quadrilateral faces) and the position of their mid-side nodes. The shape files add the Jacobian and what is special
// to the shape.

namespace oor::analysis::detail
{

namespace
{

// The mid-side nodes of each solid in the order of the card: G5..G10 of the CTETRA, G7..G15 of the CPENTA, G9..G20 of
// the CHEXA, G6..G13 of the CPYRAM.
constexpr std::array<std::array<std::uint8_t, 2>, 6> TETRAHEDRON_MID = {
    {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}}};
constexpr std::array<std::array<std::uint8_t, 2>, 9> PENTAHEDRON_MID = {
    {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 4}, {2, 5}, {3, 4}, {4, 5}, {5, 3}}};
constexpr std::array<std::array<std::uint8_t, 2>, 12> HEXAHEDRON_MID = {
    {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 5}, {2, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}, {7, 4}}};
constexpr std::array<std::array<std::uint8_t, 2>, 8> PYRAMID_MID = {
    {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 4}, {2, 4}, {3, 4}}};

} // namespace

MidSideEdges tetrahedronMidSideEdges() noexcept
{
    return {4, TETRAHEDRON_MID};
}
MidSideEdges pentahedronMidSideEdges() noexcept
{
    return {6, PENTAHEDRON_MID};
}
MidSideEdges hexahedronMidSideEdges() noexcept
{
    return {8, HEXAHEDRON_MID};
}
MidSideEdges pyramidMidSideEdges() noexcept
{
    return {5, PYRAMID_MID};
}

void solidEdgePoints(const MidSideEdges& edges, std::span<const Vec3> points, QualityValues& values)
{
    EdgePointStatistics statistics;
    for (std::size_t index = 0; index < edges.pairs.size() && edges.first + index < points.size(); ++index)
        statistics.add(points[edges.pairs[index][0]], points[edges.pairs[index][1]], points[edges.first + index]);
    values.edgePointLengthRatio = statistics.lengthRatio();
    values.edgePointAngle = statistics.angle();
}

void solidSkeleton(ElementShape shape, std::span<const Vec3> corners, QualityValues& values)
{
    Range lengths;
    for (const auto& edge : ElementTopology::edgeTemplate(shape))
        lengths.add(distanceBetween(corners[edge[0]], corners[edge[1]]));
    values.minEdge = lengths.min();
    values.maxEdge = lengths.max();
    values.aspectRatio = ratioOf(lengths.max(), lengths.min());

    Range angles;
    Range cosines;
    for (const FaceCorners& face : ElementTopology::faceTemplate(shape))
    {
        Vec3 normals[4];
        for (std::size_t index = 0; index < face.count; ++index)
        {
            const Vec3& here = corners[face.corner[index]];
            const Vec3 toNext = corners[face.corner[(index + 1) % face.count]] - here;
            const Vec3 toLast = corners[face.corner[(index + face.count - 1) % face.count]] - here;
            angles.add(angleBetween(toNext, toLast));
            normals[index] = cross(toNext, toLast);
        }
        if (face.count == 4)
        {
            // The normals at diagonally opposite corners of a face: parallel for a flat face.
            cosines.add(cosineBetween(normals[0], normals[2]));
            cosines.add(cosineBetween(normals[1], normals[3]));
        }
    }
    values.minAngle = angles.min();
    values.maxAngle = angles.max();
    values.faceWarp = cosines.min();
}

} // namespace oor::analysis::detail
