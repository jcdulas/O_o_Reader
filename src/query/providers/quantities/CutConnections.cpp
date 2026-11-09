#include "oor/query/providers/quantities/CutConnections.hpp"

#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/providers/quantities/Resultants.hpp"

#include <algorithm>

namespace oor::query
{

std::optional<std::set<base::EntityId>> CutConnections::mpcSetsOf(const model::Model& model, std::int32_t subcase)
{
    const model::CaseControl& caseControl = model.control().caseControl;
    const model::CaseLineages lineages(caseControl);
    for (const model::Subcase* candidate : analysis::analysisSubcases(caseControl))
    {
        if (candidate->id != subcase)
            continue;
        std::set<base::EntityId> sets;
        if (const auto value = analysis::effectiveInteger(lineages, *candidate, "MultiPointConstrSelection"))
            if (const auto id = base::toEntityId(*value))
                for (const base::EntityId leaf :
                     analysis::expandedConstraintSet(model, model::MULTI_POINT_CONSTR_SELECTION, *id))
                    sets.insert(leaf);
        return sets;
    }
    return std::nullopt;
}

CutConnections::CutConnections(const model::Model& model, const std::optional<std::set<base::EntityId>>& mpcSets,
                               base::ThreadPool* pool)
    : m_positions(model, pool)
{
    const std::shared_ptr<const analysis::ModelEntities> entities =
        analysis::ComponentCache::of(model).entities(model, pool);
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::size_t index = 0; index < entities->size(); ++index)
    {
        const analysis::ModelEntity& entity = entities->entity(index);
        const bool active = entity.isEquation ? (!mpcSets || mpcSets->contains(entity.id))
                                              : entity.role == analysis::ElementRole::Rigid;
        if (!active)
            continue;
        Active connection;
        connection.card = std::string(registry.entry(entity.card.table).info.name);
        connection.id = static_cast<std::int32_t>(entity.id);
        connection.isEquation = entity.isEquation;
        for (const base::EntityId node : entities->nodesOf(index))
            if (node > 0)
                connection.nodes.push_back(static_cast<std::int32_t>(node));
        std::sort(connection.nodes.begin(), connection.nodes.end());
        connection.nodes.erase(std::unique(connection.nodes.begin(), connection.nodes.end()), connection.nodes.end());
        for (const std::int32_t node : connection.nodes)
        {
            ++m_actingAt[node];
            if (connection.isEquation)
                ++m_equationsAt[key(node, connection.id)];
        }
        m_active.push_back(std::move(connection));
    }
}

std::vector<CutConnection> CutConnections::crossing(const CutDefinition& cut) const
{
    const auto contains = [](const std::vector<std::int32_t>& sorted, std::int32_t node)
    { return std::binary_search(sorted.begin(), sorted.end(), node); };
    const auto onSide = [&](std::int32_t node)
    {
        if (contains(cut.interiorNodes, node))
            return true;
        if (!cut.isPlane || contains(cut.nodes, node) || (cut.elementNodes && contains(*cut.elementNodes, node)))
            return false;
        // a grid point no element uses (the centre of a spider) is on the side when it is behind the plane
        if (!m_positions.has(node))
            return false;
        const base::Vec3 at = m_positions.position(node);
        const double distance = cut.planeNormal[0] * (at.x - cut.planePoint[0]) +
                                cut.planeNormal[1] * (at.y - cut.planePoint[1]) +
                                cut.planeNormal[2] * (at.z - cut.planePoint[2]);
        return distance < 0.0;
    };
    std::vector<CutConnection> found;
    for (const Active& active : m_active)
    {
        CutConnection connection;
        bool off = false;
        bool allOnCut = !active.nodes.empty();
        for (const std::int32_t node : active.nodes)
        {
            if (onSide(node))
                connection.sideNodes.push_back(node);
            else
                off = true;
            allOnCut = allOnCut && contains(cut.nodes, node);
        }
        if (allOnCut && active.nodes.size() > 1)
        {
            // every grid point of the connection is a grid point of the cut (the reference point of a spider taken
            // into the cut with its legs): its load between them is in no row of the side and cannot be told apart,
            // so it is named as not counted and the cut is not complete
            connection.card = active.card;
            connection.id = active.id;
            connection.isEquation = active.isEquation;
            connection.countable = false;
            connection.withinCut = true;
            found.push_back(std::move(connection));
            continue;
        }
        // The section of a member (a cut by grid points and side elements): the rows of the side at its grid points.
        // A rigid element that is one of the side elements is part of the side: its multiPointConstrForce rows are
        // counted (the spider of a station that a companion package puts among the side elements). Any other connection
        // at the grid points of the side (a spider on the skin a one-element cut takes, the rigid offset of a stringer,
        // an MPC equation) carries load into the side, not across the section: it is not counted, as the elements that
        // share those grid points are not (a companion package's defect of 2026-10-13: it was added, with the moment of
        // its rows).
        const bool sideElement =
            !active.isEquation && std::binary_search(cut.sideElements.begin(), cut.sideElements.end(), active.id);
        if ((cut.memberSection && !sideElement) || connection.sideNodes.empty() || !off)
            continue;
        connection.card = active.card;
        connection.id = active.id;
        connection.isEquation = active.isEquation;
        connection.countable = std::all_of(connection.sideNodes.begin(), connection.sideNodes.end(),
                                           [&](std::int32_t node) { return m_actingAt.at(node) == 1; });
        for (const std::int32_t node : connection.sideNodes)
            connection.aloneAt.push_back(m_actingAt.at(node) == 1 ? 1 : 0);
        connection.ownRowAt.assign(connection.sideNodes.size(), 1);
        found.push_back(std::move(connection));
    }
    // the split row of an MPC set at a grid point is the sum of its equations there: it is the own row of the
    // equations that cross the cut there only when every equation of the set acting there does
    std::unordered_map<std::uint64_t, std::int32_t> crossingAt;
    for (const CutConnection& connection : found)
        if (connection.isEquation)
            for (const std::int32_t node : connection.sideNodes)
                ++crossingAt[key(node, connection.id)];
    for (CutConnection& connection : found)
        if (connection.isEquation)
            for (std::size_t index = 0; index < connection.sideNodes.size(); ++index)
            {
                const std::uint64_t at = key(connection.sideNodes[index], connection.id);
                connection.ownRowAt[index] = crossingAt.at(at) == m_equationsAt.at(at) ? 1 : 0;
            }
    return found;
}

} // namespace oor::query
