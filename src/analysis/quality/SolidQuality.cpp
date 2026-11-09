#include "QualityDriver.hpp"
#include "QualityShapes.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"

namespace oor::analysis
{

namespace
{

QualityValues solidValues(ElementShape shape, std::span<const base::Vec3> points)
{
    switch (shape)
    {
    case ElementShape::Tetrahedron:
        return detail::tetrahedronQuality(points);
    case ElementShape::Pentahedron:
        return detail::pentahedronQuality(points);
    case ElementShape::Hexahedron:
        return detail::hexahedronQuality(points);
    case ElementShape::Pyramid:
        return detail::pyramidQuality(points);
    default:
        return {};
    }
}

} // namespace

bool SolidQuality::supports(std::uint16_t typeIndex) noexcept
{
    return dimensionOf(ElementTopology::shapeOf(typeIndex)) == 3;
}

QualityArrays SolidQuality::compute(const model::Model& model, std::uint16_t typeIndex,
                                    std::span<const std::uint32_t> rows, base::ThreadPool* pool)
{
    if (!supports(typeIndex))
    {
        QualityArrays arrays;
        arrays.assign(rows.size());
        return arrays;
    }
    const ElementShape shape = ElementTopology::shapeOf(typeIndex);
    return detail::computeQuality(model, typeIndex, rows, pool,
                                  [&](std::span<const base::Vec3> points) { return solidValues(shape, points); });
}

QualityValues SolidQuality::of(const model::Model& model, model::CardLocation location)
{
    if (!supports(location.table))
        return {};
    const ElementShape shape = ElementTopology::shapeOf(location.table);
    return detail::qualityOfOne(model, location,
                                [&](std::span<const base::Vec3> points) { return solidValues(shape, points); });
}

} // namespace oor::analysis
