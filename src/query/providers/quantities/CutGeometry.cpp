#include "oor/query/providers/quantities/CutGeometry.hpp"

#include "oor/analysis/components/ElementRoles.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/providers/quantities/Resultants.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <set>
#include <type_traits>

namespace oor::query
{

namespace
{

using Point = std::array<double, 3>;

TopologyList topologyOf(const ResultQueryContext& context, const std::optional<std::vector<std::int64_t>>& only)
{
    const model::Model& model = context.requireModel("find the elements on each side of the cut");
    return TopologyList::build(model, context.exec(), only);
}

// The elements whose grid point forces carry load from one grid point to another decide the sides and the grid points
// of the cut. A mass or a plot element carries none (the inertial load of a mass is the appliedLoad row of its grid
// point); a rigid element has no row of its own (its load is the multiPointConstrForce rows of its grid points: a cut
// connection, see CutConnections).
bool carriesLoad(std::uint16_t type)
{
    const analysis::ElementRole role = analysis::roleOfType(type);
    return role != analysis::ElementRole::Mass && role != analysis::ElementRole::None &&
           role != analysis::ElementRole::Rigid;
}

// The normal of a plane made a unit vector.
Point unitNormal(const Point& normal)
{
    const double length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    if (!(length > 0.0))
        throw QueryError("the normal of the cutting plane is the zero vector");
    return {normal[0] / length, normal[1] / length, normal[2] / length};
}

// The distance of the centre of an element from a plane along its unit normal: negative on the side.
double signedDistance(const Point& normal, const Point& centre, const Point& point) noexcept
{
    return normal[0] * (centre[0] - point[0]) + normal[1] * (centre[1] - point[1]) + normal[2] * (centre[2] - point[2]);
}

// A damper carries no load in a static subcase (no velocity): format reference CDAMP1 to CDAMP5, CVISC. `card` is a
// card type: DamperDofProperty to DamperScalarInline and their explicit forms, HeatCapacity2 (CDAMP5), ViscousDamper2.
bool isDamper(const std::string& card)
{
    return card.starts_with("DamperDof") || card.starts_with("DamperScalar") || card == "HeatCapacity2" ||
           card == "ViscousDamper2";
}

} // namespace

CutGeometry::CutGeometry(const ResultQueryContext& context, const std::optional<std::vector<std::int64_t>>& restrict)
    : m_model(context.requireModel("find the elements on each side of the cut")),
      m_positions(m_model, context.exec().pool()),
      m_all(topologyOf(context, std::nullopt))
{
    if (restrict)
        m_restricted = topologyOf(context, restrict);
    const TopologyList& planar = m_restricted ? *m_restricted : m_all;
    m_centres.resize(planar.list.size());
    m_hasCentre.assign(planar.list.size(), 0);
    for (std::size_t index = 0; index < planar.list.size(); ++index)
    {
        if (!carriesLoad(planar.list.types[index]))
            continue;
        Point sum = {0.0, 0.0, 0.0};
        std::size_t count = 0;
        for (const std::int32_t node : planar.nodes.of(index))
            if (node > 0 && m_positions.has(node))
            {
                const base::Vec3 at = m_positions.position(node);
                sum[0] += at.x;
                sum[1] += at.y;
                sum[2] += at.z;
                ++count;
            }
        if (count == 0)
            continue;
        for (double& value : sum)
            value /= static_cast<double>(count);
        m_centres[index] = sum;
        m_hasCentre[index] = 1;
    }
    auto used = std::make_shared<std::vector<std::int32_t>>();
    for (std::size_t index = 0; index < m_all.list.size(); ++index)
        if (carriesLoad(m_all.list.types[index]))
            for (const std::int32_t node : m_all.nodes.of(index))
                if (node > 0)
                    used->push_back(node);
    std::sort(used->begin(), used->end());
    used->erase(std::unique(used->begin(), used->end()), used->end());
    m_elementNodes = std::move(used);
}

CutDefinition CutGeometry::byPlane(const std::array<double, 3>& point, const std::array<double, 3>& normal) const
{
    CutDefinition cut;
    cut.isPlane = true;
    cut.planePoint = point;
    cut.planeNormal = unitNormal(normal);
    const TopologyList& planar = m_restricted ? *m_restricted : m_all;
    std::vector<std::int32_t> sideNodes;
    std::vector<std::int32_t> otherNodes;
    for (std::size_t index = 0; index < planar.list.size(); ++index)
    {
        if (m_hasCentre[index] == 0)
            continue;
        const bool negative = signedDistance(cut.planeNormal, m_centres[index], point) < 0.0;
        if (negative)
            cut.sideElements.push_back(planar.list.ids[index]);
        std::vector<std::int32_t>& target = negative ? sideNodes : otherNodes;
        for (const std::int32_t node : planar.nodes.of(index))
            if (node > 0)
                target.push_back(node);
    }
    std::sort(sideNodes.begin(), sideNodes.end());
    sideNodes.erase(std::unique(sideNodes.begin(), sideNodes.end()), sideNodes.end());
    std::sort(otherNodes.begin(), otherNodes.end());
    otherNodes.erase(std::unique(otherNodes.begin(), otherNodes.end()), otherNodes.end());
    std::set_intersection(sideNodes.begin(), sideNodes.end(), otherNodes.begin(), otherNodes.end(),
                          std::back_inserter(cut.nodes));
    return finish(std::move(cut));
}

std::vector<CutDefinition> CutGeometry::byParallelPlanes(const std::vector<std::array<double, 3>>& points,
                                                         const std::array<double, 3>& normal) const
{
    const Point unit = unitNormal(normal);
    const TopologyList& planar = m_restricted ? *m_restricted : m_all;
    const std::size_t count = planar.list.size();
    // the elements the planes sort (those with a centre), and what `finish` reads of each in the list of every element:
    // the cut by each plane is that of byPlane, kept up to date from one plane to the next
    std::vector<std::size_t> placed;
    std::vector<std::uint8_t> carries(count, 0); // a side element at the cut carries load there
    std::vector<std::string> cards(count);
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::size_t index = 0; index < count; ++index)
    {
        if (m_hasCentre[index] == 0)
            continue;
        const std::int64_t whole = m_all.indexOf(planar.list.ids[index]);
        if (whole < 0)
        {
            // an element that `finish` would not find: each plane on its own
            std::vector<CutDefinition> cuts;
            for (const Point& point : points)
                cuts.push_back(byPlane(point, normal));
            return cuts;
        }
        placed.push_back(index);
        const std::uint16_t type = m_all.list.types[static_cast<std::size_t>(whole)];
        carries[index] = carriesLoad(type) ? 1 : 0;
        cards[index] = std::string(registry.entry(type).info.name);
    }
    // the elements numbered by increasing ID (an ID the list has twice is on the side when either is)
    std::vector<std::int32_t> elementIds;
    for (const std::size_t index : placed)
        elementIds.push_back(planar.list.ids[index]);
    std::sort(elementIds.begin(), elementIds.end());
    elementIds.erase(std::unique(elementIds.begin(), elementIds.end()), elementIds.end());
    std::vector<std::uint32_t> idNumber(count, 0);
    for (const std::size_t index : placed)
        idNumber[index] = static_cast<std::uint32_t>(
            std::lower_bound(elementIds.begin(), elementIds.end(), planar.list.ids[index]) - elementIds.begin());
    // the grid points of the elements, numbered by increasing ID; how many elements use each (an element as often as
    // it names it), how many of them are on the side, and the elements at each
    std::vector<std::int32_t> nodeIds;
    for (const std::size_t index : placed)
        for (const std::int32_t node : planar.nodes.of(index))
            if (node > 0)
                nodeIds.push_back(node);
    std::sort(nodeIds.begin(), nodeIds.end());
    nodeIds.erase(std::unique(nodeIds.begin(), nodeIds.end()), nodeIds.end());
    std::vector<std::vector<std::uint32_t>> numbers(count);
    std::vector<std::uint32_t> total(nodeIds.size(), 0);
    std::vector<std::uint32_t> side(nodeIds.size(), 0);
    std::vector<std::vector<std::uint32_t>> elementsAt(nodeIds.size());
    for (const std::size_t index : placed)
        for (const std::int32_t node : planar.nodes.of(index))
            if (node > 0)
            {
                const auto number = static_cast<std::uint32_t>(std::lower_bound(nodeIds.begin(), nodeIds.end(), node) -
                                                               nodeIds.begin());
                numbers[index].push_back(number);
                ++total[number];
                elementsAt[number].push_back(static_cast<std::uint32_t>(index));
            }

    // The position of each element (its centre) and of each plane along the normal. An element further than `margin`
    // from a plane along it is on the side its position gives (the rounding of the positions and of the distance
    // byPlane computes is a few units of 1e-16 of the coordinates); a nearer one has its side computed as byPlane
    // computes it. The planes are taken in their order along the normal: from one to the next only the elements
    // between them, give or take the margin, can change sides.
    std::vector<double> position(count, 0.0);
    double largest = 0.0;
    for (const std::size_t index : placed)
    {
        const Point& centre = m_centres[index];
        position[index] = unit[0] * centre[0] + unit[1] * centre[1] + unit[2] * centre[2];
        largest = std::max(
            largest, std::fabs(unit[0] * centre[0]) + std::fabs(unit[1] * centre[1]) + std::fabs(unit[2] * centre[2]));
    }
    std::vector<double> along(points.size());
    for (std::size_t plane = 0; plane < points.size(); ++plane)
    {
        const Point& point = points[plane];
        along[plane] = unit[0] * point[0] + unit[1] * point[1] + unit[2] * point[2];
        largest = std::max(
            largest, std::fabs(unit[0] * point[0]) + std::fabs(unit[1] * point[1]) + std::fabs(unit[2] * point[2]));
    }
    const double margin = 1.0e-12 * largest;
    std::vector<std::size_t> byPosition = placed;
    std::stable_sort(byPosition.begin(), byPosition.end(),
                     [&](std::size_t left, std::size_t right) { return position[left] < position[right]; });
    std::vector<double> sortedPosition;
    for (const std::size_t index : byPosition)
        sortedPosition.push_back(position[index]);
    std::vector<std::size_t> order(points.size());
    for (std::size_t plane = 0; plane < points.size(); ++plane)
        order[plane] = plane;
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t left, std::size_t right) { return along[left] < along[right]; });

    // the side of each element, how many elements of each ID are on it, and the sorted numbers of the IDs on the side,
    // of the grid points on the cut and of those inside the side
    std::vector<std::uint8_t> negative(count, 0);
    std::vector<std::uint32_t> negativeOfId(elementIds.size(), 0);
    std::vector<std::int32_t> sideIds;       // element IDs
    std::vector<std::uint32_t> cutNodes;     // grid point numbers
    std::vector<std::int32_t> interiorNodes; // grid point IDs
    // 0 off the side, 1 on the cut (of the side and of the rest), 2 inside the side
    const auto stateOf = [&](std::uint32_t number)
    { return side[number] == 0 ? 0 : (side[number] == total[number] ? 2 : 1); };
    // PERFORMANCE: the elements that change sides from one plane to the next only mark their IDs and grid points; the
    // sorted lists are then merged once for the plane (a companion package: inserting them one at a time into the lists
    // cost the square of the elements, 0.19 s for one station of 64 000 shells)
    std::vector<std::uint8_t> idTouched(elementIds.size(), 0);
    std::vector<std::uint8_t> nodeTouched(nodeIds.size(), 0);
    std::vector<std::uint8_t> nodeBefore(nodeIds.size(), 0);
    std::vector<std::uint32_t> touchedIds;
    std::vector<std::uint32_t> touchedNodes;
    const auto setSide = [&](std::size_t index, std::uint8_t now)
    {
        if (now == negative[index])
            return;
        negative[index] = now;
        const std::uint32_t id = idNumber[index];
        if (idTouched[id] == 0)
        {
            // the membership before the plane: on the side when a copy of the ID is (the count before this change)
            idTouched[id] = negativeOfId[id] > 0 ? 2 : 1;
            touchedIds.push_back(id);
        }
        negativeOfId[id] = now != 0 ? negativeOfId[id] + 1 : negativeOfId[id] - 1;
        for (const std::uint32_t number : numbers[index])
        {
            if (nodeTouched[number] == 0)
            {
                nodeTouched[number] = 1;
                nodeBefore[number] = static_cast<std::uint8_t>(stateOf(number));
                touchedNodes.push_back(number);
            }
            side[number] = now != 0 ? side[number] + 1 : side[number] - 1;
        }
    };
    // the lists after the changes of a plane: what left them taken out, what joined them merged in
    const auto update = [](auto& list, auto& gone, auto& joined)
    {
        using Value = typename std::decay_t<decltype(list)>::value_type;
        std::sort(gone.begin(), gone.end());
        std::sort(joined.begin(), joined.end());
        std::vector<Value> kept;
        kept.reserve(list.size() - std::min(list.size(), gone.size()));
        std::set_difference(list.begin(), list.end(), gone.begin(), gone.end(), std::back_inserter(kept));
        list.clear();
        std::merge(kept.begin(), kept.end(), joined.begin(), joined.end(), std::back_inserter(list));
        gone.clear();
        joined.clear();
    };
    std::vector<std::int32_t> idsGone;
    std::vector<std::int32_t> idsJoined;
    std::vector<std::uint32_t> cutGone;
    std::vector<std::uint32_t> cutJoined;
    std::vector<std::int32_t> interiorGone;
    std::vector<std::int32_t> interiorJoined;
    const auto settle = [&]()
    {
        for (const std::uint32_t id : touchedIds)
        {
            const bool before = idTouched[id] == 2;
            const bool after = negativeOfId[id] > 0;
            idTouched[id] = 0;
            if (before && !after)
                idsGone.push_back(elementIds[id]);
            if (!before && after)
                idsJoined.push_back(elementIds[id]);
        }
        touchedIds.clear();
        for (const std::uint32_t number : touchedNodes)
        {
            const int before = nodeBefore[number];
            const int after = stateOf(number);
            nodeTouched[number] = 0;
            if (before == after)
                continue;
            if (before == 1)
                cutGone.push_back(number);
            if (before == 2)
                interiorGone.push_back(nodeIds[number]);
            if (after == 1)
                cutJoined.push_back(number);
            if (after == 2)
                interiorJoined.push_back(nodeIds[number]);
        }
        touchedNodes.clear();
        update(sideIds, idsGone, idsJoined);
        update(cutNodes, cutGone, cutJoined);
        update(interiorNodes, interiorGone, interiorJoined);
    };

    std::vector<CutDefinition> cuts(points.size());
    std::vector<std::size_t> touching;
    bool first = true;
    double previous = 0.0;
    for (const std::size_t plane : order)
    {
        const Point& point = points[plane];
        const double here = along[plane];
        const std::size_t from =
            first ? 0
                  : static_cast<std::size_t>(
                        std::lower_bound(sortedPosition.begin(), sortedPosition.end(), previous - margin) -
                        sortedPosition.begin());
        const std::size_t to = static_cast<std::size_t>(
            std::upper_bound(sortedPosition.begin(), sortedPosition.end(), here + margin) - sortedPosition.begin());
        for (std::size_t at = from; at < to; ++at)
        {
            const std::size_t index = byPosition[at];
            const double distance = sortedPosition[at] - here;
            const bool onSide = distance < -margin  ? true
                                : distance > margin ? false
                                                    : signedDistance(unit, m_centres[index], point) < 0.0;
            setSide(index, onSide ? 1 : 0);
        }
        settle();
        first = false;
        previous = here;

        CutDefinition& cut = cuts[plane];
        cut.isPlane = true;
        cut.planePoint = point;
        cut.planeNormal = unit;
        cut.sideElements = sideIds;
        cut.interiorNodes = interiorNodes;
        // the grid points on the cut, and the side elements there, by ID, each once
        touching.clear();
        for (const std::uint32_t number : cutNodes)
        {
            cut.nodes.push_back(nodeIds[number]);
            for (const std::uint32_t index : elementsAt[number])
                if (negative[index] != 0)
                    touching.push_back(index);
        }
        std::stable_sort(touching.begin(), touching.end(),
                         [&](std::size_t left, std::size_t right) { return idNumber[left] < idNumber[right]; });
        for (std::size_t at = 0; at < touching.size(); ++at)
        {
            const std::size_t index = touching[at];
            if (at > 0 && idNumber[touching[at - 1]] == idNumber[index])
                continue;
            if (carries[index] != 0)
                cut.sideElementsAtCut.push_back(
                    CutSideElement{planar.list.ids[index], cards[index], isDamper(cards[index])});
        }
        cut.elementNodes = m_elementNodes;
        std::array<double, 3> sum = {0.0, 0.0, 0.0};
        std::size_t placedNodes = 0;
        for (const std::int32_t node : cut.nodes)
            if (m_positions.has(node))
            {
                const base::Vec3 at = m_positions.position(node);
                sum[0] += at.x;
                sum[1] += at.y;
                sum[2] += at.z;
                ++placedNodes;
            }
        if (placedNodes > 0)
            for (double& value : sum)
                value /= static_cast<double>(placedNodes);
        cut.centroid = sum;
    }
    return cuts;
}

CutDefinition CutGeometry::byNodes(const std::vector<std::int64_t>& nodes,
                                   const std::vector<std::int64_t>& sideElements) const
{
    CutDefinition cut;
    for (const std::int64_t node : nodes)
        cut.nodes.push_back(static_cast<std::int32_t>(node));
    for (const std::int64_t element : sideElements)
        cut.sideElements.push_back(static_cast<std::int32_t>(element));
    cut.memberSection = true;
    return finish(std::move(cut));
}

CutDefinition CutGeometry::freebody(const std::vector<std::int64_t>& elements,
                                    const std::optional<std::vector<std::int64_t>>& nodes) const
{
    CutDefinition cut;
    for (const std::int64_t element : elements)
        cut.sideElements.push_back(static_cast<std::int32_t>(element));
    if (nodes)
    {
        for (const std::int64_t node : *nodes)
            cut.nodes.push_back(static_cast<std::int32_t>(node));
    }
    else
    {
        const std::set<std::int32_t> inside(cut.sideElements.begin(), cut.sideElements.end());
        std::set<std::int32_t> insideNodes;
        std::set<std::int32_t> outsideNodes;
        for (std::size_t index = 0; index < m_all.list.size(); ++index)
        {
            if (!carriesLoad(m_all.list.types[index]))
                continue;
            for (const std::int32_t node : m_all.nodes.of(index))
                if (node > 0)
                    (inside.contains(m_all.list.ids[index]) ? insideNodes : outsideNodes).insert(node);
        }
        for (const std::int32_t node : insideNodes)
            if (outsideNodes.contains(node))
                cut.nodes.push_back(node);
    }
    return finish(std::move(cut));
}

CutDefinition CutGeometry::finish(CutDefinition cut) const
{
    std::sort(cut.nodes.begin(), cut.nodes.end());
    cut.nodes.erase(std::unique(cut.nodes.begin(), cut.nodes.end()), cut.nodes.end());
    std::sort(cut.sideElements.begin(), cut.sideElements.end());
    cut.sideElements.erase(std::unique(cut.sideElements.begin(), cut.sideElements.end()), cut.sideElements.end());
    cut.elementNodes = m_elementNodes;
    // the interior of the side, and the elements of the side that touch the cut (a mass or a plot element carries no
    // load across it)
    const auto onCut = [&](std::int32_t node) { return std::binary_search(cut.nodes.begin(), cut.nodes.end(), node); };
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (const std::int32_t element : cut.sideElements)
    {
        const std::int64_t index = m_all.indexOf(element);
        if (index < 0)
            continue;
        bool touches = false;
        for (const std::int32_t node : m_all.nodes.of(static_cast<std::size_t>(index)))
        {
            if (node <= 0)
                continue;
            if (onCut(node))
                touches = true;
            else
                cut.interiorNodes.push_back(node);
        }
        const std::uint16_t type = m_all.list.types[static_cast<std::size_t>(index)];
        if (touches && carriesLoad(type))
        {
            const std::string card(registry.entry(type).info.name);
            cut.sideElementsAtCut.push_back(CutSideElement{element, card, isDamper(card)});
        }
    }
    std::sort(cut.interiorNodes.begin(), cut.interiorNodes.end());
    cut.interiorNodes.erase(std::unique(cut.interiorNodes.begin(), cut.interiorNodes.end()), cut.interiorNodes.end());
    std::array<double, 3> sum = {0.0, 0.0, 0.0};
    std::size_t count = 0;
    for (const std::int32_t node : cut.nodes)
        if (m_positions.has(node))
        {
            const base::Vec3 at = m_positions.position(node);
            sum[0] += at.x;
            sum[1] += at.y;
            sum[2] += at.z;
            ++count;
        }
    if (count > 0)
        for (double& value : sum)
            value /= static_cast<double>(count);
    cut.centroid = sum;
    return cut;
}

} // namespace oor::query
