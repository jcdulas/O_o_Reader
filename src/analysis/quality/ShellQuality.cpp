#include "QualityDriver.hpp"
#include "QualityShapes.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"

namespace oor::analysis
{

namespace
{

// The quality of a triangle or a quadrilateral from the points of its card: corners first and then the mid-side nodes
// of the sides G1-G2, G2-G3, ... (CTRIAX6 alternates corners and mid-side nodes).
QualityValues shellValues(ElementShape shape, bool alternating, std::span<const base::Vec3> points)
{
    const base::Vec3 missing = undefinedVector();
    const auto at = [&](std::size_t slot) { return slot < points.size() ? points[slot] : missing; };
    if (shape == ElementShape::Triangle)
    {
        const base::Vec3 corner[3] = {at(0), at(alternating ? 2 : 1), at(alternating ? 4 : 2)};
        const base::Vec3 middle[3] = {at(alternating ? 1 : 3), at(alternating ? 3 : 4), at(5)};
        return detail::triangleQuality(corner, middle);
    }
    const base::Vec3 corner[4] = {at(0), at(1), at(2), at(3)};
    const base::Vec3 middle[4] = {at(4), at(5), at(6), at(7)};
    return detail::quadrilateralQuality(corner, middle);
}

} // namespace

bool ShellQuality::supports(std::uint16_t typeIndex) noexcept
{
    const ElementShape shape = ElementTopology::shapeOf(typeIndex);
    return shape == ElementShape::Triangle || shape == ElementShape::Quadrilateral;
}

QualityArrays ShellQuality::compute(const model::Model& model, std::uint16_t typeIndex,
                                    std::span<const std::uint32_t> rows, base::ThreadPool* pool)
{
    if (!supports(typeIndex))
    {
        QualityArrays arrays;
        arrays.assign(rows.size());
        return arrays;
    }
    const ElementShape shape = ElementTopology::shapeOf(typeIndex);
    const bool alternating = ElementTopology::alternatingCorners(typeIndex);
    return detail::computeQuality(model, typeIndex, rows, pool, [&](std::span<const base::Vec3> points)
                                  { return shellValues(shape, alternating, points); });
}

QualityValues ShellQuality::of(const model::Model& model, model::CardLocation location)
{
    if (!supports(location.table))
        return {};
    const ElementShape shape = ElementTopology::shapeOf(location.table);
    const bool alternating = ElementTopology::alternatingCorners(location.table);
    return detail::qualityOfOne(
        model, location, [&](std::span<const base::Vec3> points) { return shellValues(shape, alternating, points); });
}

} // namespace oor::analysis
