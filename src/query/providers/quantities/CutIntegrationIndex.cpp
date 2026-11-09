#include "oor/query/providers/quantities/CutIntegrationIndex.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace oor::query
{

namespace
{

using Point = CutIntegrationIndex::Point;

double dot(const Point& a, const Point& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// How far a plane may be from a shell's centre beyond its radius and still be counted: the tolerance of the
// integration's own test (1e-9 of the distances) with room to spare.
double margin(double radius, double distance)
{
    return 1.0e-6 * (radius + std::fabs(distance) + 1.0);
}

} // namespace

CutIntegrationIndex CutIntegrationIndex::build(const ResultQueryContext& context, const model::Model& model,
                                               const ElementFlatSet& forces, const NodePositions& positions,
                                               const TopologyList& topology, const std::optional<Point>& axis)
{
    CutIntegrationIndex index;
    const std::vector<ElementFlat>& flats = forces.flats();
    // PERFORMANCE: the flats of the subcases mostly have the rows of the one before (the same
    // elements and places): they share its layout, whose rows were linked to the shells once (two lookups of a hash
    // table and a row added to a shell for each shell and subcase were most of the cost of the index for a complete
    // base)
    const ElementFlat* layout = nullptr; // the first flat of the current layout
    for (std::uint32_t flatIndex = 0; flatIndex < flats.size(); ++flatIndex)
    {
        const ElementFlat& flat = flats[flatIndex];
        if (flat.isComplex())
            continue;
        if (layout != nullptr && layout->set == flat.set && layout->eid == flat.eid && layout->node == flat.node)
        {
            index.layoutFlats.back().push_back(flatIndex);
            continue;
        }
        layout = &flat;
        const auto layoutIndex = static_cast<std::uint32_t>(index.layoutFlats.size());
        index.layoutFlats.push_back({flatIndex});
        for (std::uint32_t row = 0; row < flat.rows(); ++row)
        {
            if (flat.node[row] != 0)
                continue;
            const ResultQueryContext::ElementCard card = context.elementCard(flat.set, flat.eid[row]);
            if (!card.linked)
                continue;
            auto [found, inserted] =
                index.byModelId.emplace(card.modelId, static_cast<std::uint32_t>(index.shells.size()));
            if (inserted)
            {
                // the geometry of the shell, once for the call
                Shell shell;
                shell.modelId = card.modelId;
                const std::int64_t at = topology.indexOf(card.modelId);
                const auto location = model.locate(base::IdSpace::Element, card.modelId);
                if (at >= 0 && location &&
                    analysis::dimensionOf(analysis::ElementTopology::shapeOf(location->table)) == 2)
                {
                    const analysis::ElementValues values =
                        analysis::ElementGeometry::of(model, *location, analysis::bit(analysis::Quantity::Frame));
                    if (std::isfinite(values.frame.values[0]))
                    {
                        for (std::size_t axisIndex = 0; axisIndex < 3; ++axisIndex)
                            shell.axes[axisIndex] = {values.frame(0, axisIndex), values.frame(1, axisIndex),
                                                     values.frame(2, axisIndex)};
                        std::size_t position = 0;
                        for (const std::int32_t node : topology.nodes.of(static_cast<std::size_t>(at)))
                        {
                            if (isCornerPosition(location->table, position++) && node > 0 && positions.has(node))
                            {
                                const base::Vec3 point = positions.position(node);
                                shell.corners.push_back({point.x, point.y, point.z});
                                shell.cornerNodes.push_back(node);
                            }
                        }
                        shell.ok = shell.corners.size() >= 3;
                    }
                }
                if (shell.ok)
                {
                    for (const Point& corner : shell.corners)
                        for (std::size_t axisIndex = 0; axisIndex < 3; ++axisIndex)
                            shell.centre[axisIndex] += corner[axisIndex];
                    for (double& value : shell.centre)
                        value /= static_cast<double>(shell.corners.size());
                    for (const Point& corner : shell.corners)
                    {
                        const Point offset{corner[0] - shell.centre[0], corner[1] - shell.centre[1],
                                           corner[2] - shell.centre[2]};
                        shell.radius = std::max(shell.radius, std::sqrt(dot(offset, offset)));
                    }
                }
                index.shells.push_back(std::move(shell));
            }
            index.shells[found->second].places.emplace_back(layoutIndex, row);
        }
    }
    if (axis)
    {
        index.axis = axis;
        for (std::uint32_t shell = 0; shell < index.shells.size(); ++shell)
            if (index.shells[shell].ok)
            {
                index.alongAxis.push_back(shell);
                index.largestRadius = std::max(index.largestRadius, index.shells[shell].radius);
            }
        std::sort(index.alongAxis.begin(), index.alongAxis.end(), [&](std::uint32_t left, std::uint32_t right)
                  { return dot(*axis, index.shells[left].centre) < dot(*axis, index.shells[right].centre); });
        for (const std::uint32_t shell : index.alongAxis)
            index.positionAlong.push_back(dot(*axis, index.shells[shell].centre));
    }
    return index;
}

std::vector<std::uint32_t> CutIntegrationIndex::nearPlane(const Point& normal, const Point& point) const
{
    std::vector<std::uint32_t> found;
    const auto reaches = [&](const Shell& shell)
    {
        const double distance =
            dot(normal, {shell.centre[0] - point[0], shell.centre[1] - point[1], shell.centre[2] - point[2]});
        return shell.ok && std::fabs(distance) <= shell.radius + margin(shell.radius, distance);
    };
    // a plane normal to the axis (either way): the shells whose centre is within the largest radius of it
    if (axis && std::fabs(std::fabs(dot(*axis, normal)) - 1.0) < 1.0e-12)
    {
        const double at = dot(*axis, point);
        const double reach = largestRadius + margin(largestRadius, at);
        const auto first = std::lower_bound(positionAlong.begin(), positionAlong.end(), at - reach);
        const auto last = std::upper_bound(positionAlong.begin(), positionAlong.end(), at + reach);
        for (auto it = first; it != last; ++it)
        {
            const std::uint32_t shell = alongAxis[static_cast<std::size_t>(it - positionAlong.begin())];
            if (reaches(shells[shell]))
                found.push_back(shell);
        }
        std::sort(found.begin(), found.end());
        return found;
    }
    for (std::uint32_t shell = 0; shell < shells.size(); ++shell)
        if (reaches(shells[shell]))
            found.push_back(shell);
    return found;
}

} // namespace oor::query
