#include "oor/query/providers/quantities/CutForceSum.hpp"

#include <algorithm>
#include <set>

namespace oor::query
{

namespace
{

// Adds the force and the moment about `point` of a row of six (a force and a moment at a grid point at `at`), times
// `sign`.
void accumulate(std::array<double, 6>& sum, const std::array<double, 6>& value, const base::Vec3& at,
                const std::array<double, 3>& point, double sign)
{
    const double rx = at.x - point[0];
    const double ry = at.y - point[1];
    const double rz = at.z - point[2];
    sum[0] += sign * value[0];
    sum[1] += sign * value[1];
    sum[2] += sign * value[2];
    sum[3] += sign * (value[3] + ry * value[2] - rz * value[1]);
    sum[4] += sign * (value[4] + rz * value[0] - rx * value[2]);
    sum[5] += sign * (value[5] + rx * value[1] - ry * value[0]);
}

// A cut connection at some of whose grid points on the side the multiPointConstrForce rows are split by relation: at
// those, the rows of its relation (its own when `ownRowAt`); at the others, the lumped row when it is its own
// (`aloneAt`). Not counted, and named, when one of its grid points has no row, no row of its relation, or a row it
// cannot claim.
template <typename IsSplitAt>
void addSplitConnection(const NodePointForceLayout& layout, const CutConnection& connection,
                        const NodePositions& positions, const IsSplitAt& isSplitAt,
                        std::set<std::pair<std::int32_t, std::int32_t>>& splitCounted, CutForceSum::Plan& plan,
                        std::set<std::pair<std::string, std::int32_t>>& uncounted)
{
    const std::int32_t relation = connection.relation();
    std::vector<std::uint8_t> split(connection.sideNodes.size(), 0);
    for (std::size_t index = 0; index < connection.sideNodes.size(); ++index)
    {
        const std::int32_t node = connection.sideNodes[index];
        const std::span<const std::uint32_t> rows = layout.rowsAt(node);
        split[index] = isSplitAt(node) ? 1 : 0;
        const bool claimed =
            split[index] != 0
                ? connection.ownRowAt[index] != 0 &&
                      std::any_of(
                          rows.begin(), rows.end(), [&](std::uint32_t row)
                          { return layout.kind[row] == NodePointForceKind::Mpc && layout.relation[row] == relation; })
                : connection.aloneAt[index] != 0 && !rows.empty();
        if (!claimed)
        {
            uncounted.emplace(connection.card, connection.id);
            return;
        }
    }
    const char* contribution = connection.isEquation ? "mpc" : "rigid";
    for (std::size_t index = 0; index < connection.sideNodes.size(); ++index)
    {
        const std::int32_t node = connection.sideNodes[index];
        if (split[index] != 0 && !splitCounted.emplace(node, relation).second)
            continue; // the row of the set at this grid point is counted already, with another equation of it
        const base::Vec3 at = positions.position(node);
        for (const std::uint32_t row : layout.rowsAt(node))
        {
            if (layout.kind[row] != NodePointForceKind::Mpc || (split[index] != 0 && layout.relation[row] != relation))
                continue;
            plan.terms.push_back(
                CutForceSum::Term{node, at, static_cast<std::uint32_t>(plan.rows.size()), 1, contribution, true});
            plan.rows.push_back(row);
        }
    }
}

} // namespace

CutTableSum CutForceSum::of(const NodePointForceTable& table, const CutDefinition& cut,
                            const std::vector<CutConnection>& connections, const std::array<double, 3>& point,
                            const NodePositions& positions, bool withNodes)
{
    CutTableSum sum;
    apply(plan(table, cut, connections, positions), table, point, withNodes, sum);
    return sum;
}

CutForceSum::Plan CutForceSum::plan(const NodePointForceTable& table, const CutDefinition& cut,
                                    const std::vector<CutConnection>& connections, const NodePositions& positions)
{
    Plan plan;
    const NodePointForceLayout& layout = *table.layout;
    const auto isSide = [&](std::int32_t element)
    { return std::binary_search(cut.sideElements.begin(), cut.sideElements.end(), element); };
    std::set<std::pair<std::string, std::int32_t>> uncounted;

    // the elements of the side at the cut grid points
    std::set<std::int32_t> elements;
    for (const std::int32_t node : cut.nodes)
    {
        const std::span<const std::uint32_t> rows = layout.rowsAt(node);
        if (rows.empty())
        {
            uncounted.emplace("GRID", node);
            continue;
        }
        Term term;
        term.first = static_cast<std::uint32_t>(plan.rows.size());
        for (const std::uint32_t row : rows)
            if (layout.kind[row] == NodePointForceKind::Element && layout.element[row] > 0 &&
                isSide(layout.element[row]))
            {
                plan.rows.push_back(row);
                elements.insert(layout.element[row]);
            }
        term.count = static_cast<std::uint32_t>(plan.rows.size()) - term.first;
        if (term.count == 0)
            continue;
        ++plan.nodeCount;
        term.node = node;
        term.at = positions.position(node);
        plan.terms.push_back(term);
    }
    plan.elementCount = static_cast<std::int64_t>(elements.size());

    // the cut connections: minus the multiPointConstrForce rows of their grid points on the side; at a grid point whose
    // rows are split by relation the rows of the connection's own relation, each (grid point, relation) once
    std::set<std::pair<std::int32_t, std::int32_t>> splitCounted;
    for (const CutConnection& connection : connections)
    {
        const auto isSplitAt = [&](std::int32_t node)
        {
            if (layout.relation.empty())
                return false;
            const std::span<const std::uint32_t> rows = layout.rowsAt(node);
            return std::any_of(rows.begin(), rows.end(), [&](std::uint32_t row)
                               { return layout.kind[row] == NodePointForceKind::Mpc && layout.relation[row] != 0; });
        };
        if (!connection.withinCut && std::any_of(connection.sideNodes.begin(), connection.sideNodes.end(), isSplitAt))
        {
            addSplitConnection(layout, connection, positions, isSplitAt, splitCounted, plan, uncounted);
            continue;
        }
        if (!connection.countable)
        {
            uncounted.emplace(connection.card, connection.id);
            continue;
        }
        for (const std::int32_t node : connection.sideNodes)
        {
            const std::span<const std::uint32_t> rows = layout.rowsAt(node);
            if (rows.empty())
            {
                uncounted.emplace(connection.card, connection.id);
                continue;
            }
            const base::Vec3 at = positions.position(node);
            for (const std::uint32_t row : rows)
            {
                if (layout.kind[row] != NodePointForceKind::Mpc)
                    continue;
                plan.terms.push_back(Term{node, at, static_cast<std::uint32_t>(plan.rows.size()), 1,
                                          connection.isEquation ? "mpc" : "rigid", true});
                plan.rows.push_back(row);
            }
        }
    }

    // the elements of the side on the cut that GPFORCE gives nothing for
    for (const CutSideElement& element : cut.sideElementsAtCut)
        if (!layout.elements.contains(element.id) && !(element.damper && table.isStatic()))
            uncounted.emplace(element.card, element.id);
    for (const auto& [card, id] : uncounted)
    {
        plan.uncountedCards.push_back(card);
        plan.uncountedIds.push_back(id);
    }
    plan.complete = uncounted.empty();
    // a cut that only connections or elements without grid point forces cross gives a row too, if only to name what
    // it cannot count (a grid point of the cut without rows alone says nothing of the side)
    plan.hasRows =
        plan.nodeCount > 0 || !connections.empty() ||
        std::any_of(uncounted.begin(), uncounted.end(), [](const auto& item) { return item.first != "GRID"; });
    return plan;
}

void CutForceSum::apply(const Plan& plan, const NodePointForceTable& table, const std::array<double, 3>& point,
                        bool withNodes, CutTableSum& sum)
{
    sum.hasRows = plan.hasRows;
    sum.nodeCount = plan.nodeCount;
    sum.elementCount = plan.elementCount;
    sum.complete = plan.complete;
    sum.uncountedCards = plan.uncountedCards;
    sum.uncountedIds = plan.uncountedIds;
    const std::size_t steps = table.stepCount();
    sum.real.assign(steps, std::array<double, 6>{});
    sum.imaginary.assign(steps, std::array<double, 6>{});
    if (withNodes)
        sum.nodes.assign(steps, {});
    else
        sum.nodes.clear();
    // the imaginary parts of a real table are zeros, which leave the sums at zero
    const bool imaginaryToo = table.isComplex || withNodes;
    for (std::size_t step = 0; step < steps; ++step)
        for (const Term& term : plan.terms)
        {
            std::array<double, 6> real{};
            std::array<double, 6> imaginary{};
            if (term.negated)
            {
                // a multiPointConstrForce row: the force the side gives the connection
                const std::uint32_t row = plan.rows[term.first];
                for (std::size_t component = 0; component < 6; ++component)
                {
                    real[component] = -table.realAt(step, row, component);
                    if (imaginaryToo)
                        imaginary[component] = -table.imaginaryAt(step, row, component);
                }
            }
            else
            {
                for (std::uint32_t index = term.first; index < term.first + term.count; ++index)
                    for (std::size_t component = 0; component < 6; ++component)
                    {
                        real[component] += table.realAt(step, plan.rows[index], component);
                        if (imaginaryToo)
                            imaginary[component] += table.imaginaryAt(step, plan.rows[index], component);
                    }
            }
            accumulate(sum.real[step], real, term.at, point, 1.0);
            if (imaginaryToo)
                accumulate(sum.imaginary[step], imaginary, term.at, point, 1.0);
            if (withNodes)
                sum.nodes[step].push_back(CutNodeForce{term.node, term.contribution, real, imaginary});
        }
}

} // namespace oor::query
