#include "oor/analysis/components/ConstraintStamps.hpp"

#include "oor/analysis/components/RevisionStamp.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/constraints/enforcedMotionConstr/EnforcedMotionConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstr/SinglePointConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrList/SinglePointConstrListTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrUnion/SinglePointConstrUnionTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"

#include <algorithm>

namespace oor::analysis
{

namespace
{

// What makes an entry: the SPC and SPC1 of the set, the SPCD of the load sets, the permanent constraints.
enum class EntryKind : std::uint8_t
{
    Constraint,
    EnforcedMotion,
    Permanent
};

std::uint64_t entryHash(EntryKind kind, base::EntityId node, std::int32_t components, double value)
{
    std::uint64_t hash = 14695981039346656037ULL;
    const auto kindByte = static_cast<std::uint8_t>(kind);
    hash = RevisionStamp::hashBytes(hash, &kindByte, sizeof(kindByte));
    hash = RevisionStamp::hashBytes(hash, &node, sizeof(node));
    hash = RevisionStamp::hashBytes(hash, &components, sizeof(components));
    hash = RevisionStamp::hashReal(hash, value);
    return RevisionStamp::mix(hash);
}

// The sum of the entries of one unit.
struct Accumulator
{
    std::uint64_t sum = 0;
    std::uint64_t count = 0;
    std::vector<base::EntityId> nodes;
};

} // namespace

std::vector<ConstraintStamp> ConstraintStamps::compute(const model::Model& model, const UnitMembership& units,
                                                       std::vector<base::EntityId> spcSets,
                                                       const std::vector<std::int32_t>& unitsWanted)
{
    if (spcSets.empty())
    {
        forEachCard<model::SinglePointConstrTable>(
            model, [&](const model::SinglePointConstrCard& card, model::CardLocation) { spcSets.push_back(card.id); });
        forEachCard<model::SinglePointConstrListTable>(model, [&](const model::SinglePointConstrListCard& card,
                                                                  model::CardLocation) { spcSets.push_back(card.id); });
        forEachCard<model::SinglePointConstrUnionTable>(
            model,
            [&](const model::SinglePointConstrUnionCard& card, model::CardLocation) { spcSets.push_back(card.id); });
        const model::CaseControl& caseControl = model.control().caseControl;
        const model::CaseLineages lineages(caseControl);
        for (const model::Subcase* subcase : analysisSubcases(caseControl))
        {
            const auto sid = effectiveInteger(lineages, *subcase, model::SINGLE_POINT_CONSTR_SELECTION);
            if (sid && *sid > 0)
                spcSets.push_back(static_cast<base::EntityId>(*sid));
        }
    }
    std::sort(spcSets.begin(), spcSets.end());
    spcSets.erase(std::unique(spcSets.begin(), spcSets.end()), spcSets.end());

    // The grid points of the model by ID, for the THRU ranges of SPC1: listed the first time one is met (a call again
    // on a deck without THRU listed every grid point and sorted them each time).
    const model::NodeTable* grids = model.findTable<model::NodeTable>();
    std::vector<base::EntityId> gridIds;
    bool gridsListed = false;
    const auto sortedGridIds = [&]() -> const std::vector<base::EntityId>&
    {
        if (gridsListed)
            return gridIds;
        gridsListed = true;
        if (grids != nullptr)
        {
            gridIds.reserve(grids->rows().aliveCount());
            for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
            {
                if (grids->rows().isAlive(row))
                    gridIds.push_back(grids->idAtRow(row));
            }
            if (!std::is_sorted(gridIds.begin(), gridIds.end()))
                std::sort(gridIds.begin(), gridIds.end());
        }
        return gridIds;
    };

    const std::size_t unitCount = units.unitCount();
    std::vector<std::int32_t> here;
    const auto add = [&](std::vector<Accumulator>& accumulators, EntryKind kind, base::EntityId node,
                         std::int32_t components, double value)
    {
        if (node == 0)
            return;
        here.clear();
        units.unitsOfNode(node, here);
        if (here.empty())
            return;
        const std::uint64_t hash = entryHash(kind, node, components, value);
        for (const std::int32_t unit : here)
        {
            Accumulator& accumulator = accumulators[static_cast<std::size_t>(unit - 1)];
            accumulator.sum += hash;
            ++accumulator.count;
            accumulator.nodes.push_back(node);
        }
    };

    // The entries every set shares: the permanent constraints and the enforced motions.
    std::vector<Accumulator> shared(unitCount);
    if (grids != nullptr)
    {
        for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
        {
            if (grids->rows().isAlive(row) && grids->permanentConstraints(row) != 0)
                add(shared, EntryKind::Permanent, grids->idAtRow(row), grids->permanentConstraints(row), 0.0);
        }
    }
    forEachCard<model::EnforcedMotionConstrTable>(model,
                                                  [&](const model::EnforcedMotionConstrCard& card, model::CardLocation)
                                                  {
                                                      for (std::size_t group = 0; group < 2; ++group)
                                                      {
                                                          if (group == 1 && !card.hasSecondGroup())
                                                              break;
                                                          add(shared, EntryKind::EnforcedMotion, card.nodes[group],
                                                              card.components[group], card.enforced[group]);
                                                      }
                                                  });

    std::vector<std::optional<base::EntityId>> sets(spcSets.begin(), spcSets.end());
    if (sets.empty())
        sets.emplace_back(std::nullopt);
    std::vector<ConstraintStamp> stamps;
    for (const std::optional<base::EntityId>& set : sets)
    {
        std::vector<Accumulator> accumulators = shared;
        if (set)
        {
            const std::vector<base::EntityId> members =
                expandedConstraintSet(model, model::SINGLE_POINT_CONSTR_SELECTION, *set);
            forEachCard<model::SinglePointConstrTable>(
                model,
                [&](const model::SinglePointConstrCard& card, model::CardLocation)
                {
                    if (!std::binary_search(members.begin(), members.end(), card.id))
                        return;
                    for (std::size_t group = 0; group < 2; ++group)
                    {
                        if (group == 1 && !card.hasSecondGroup())
                            break;
                        add(accumulators, EntryKind::Constraint, card.nodes[group], card.components[group],
                            card.enforced[group]);
                    }
                });
            forEachCard<model::SinglePointConstrListTable>(
                model,
                [&](const model::SinglePointConstrListCard& card, model::CardLocation)
                {
                    if (!std::binary_search(members.begin(), members.end(), card.id))
                        return;
                    for (const model::NodeRun& run : card.nodes)
                    {
                        if (run.first == run.last)
                        {
                            add(accumulators, EntryKind::Constraint, run.first, card.components, 0.0);
                            continue;
                        }
                        const std::vector<base::EntityId>& ids = sortedGridIds();
                        for (auto at = std::lower_bound(ids.begin(), ids.end(), run.first);
                             at != ids.end() && *at <= run.last; ++at)
                            add(accumulators, EntryKind::Constraint, *at, card.components, 0.0);
                    }
                });
        }
        for (std::size_t index = 0; index < unitCount; ++index)
        {
            const auto unit = static_cast<std::int32_t>(index + 1);
            if (!unitsWanted.empty() && std::find(unitsWanted.begin(), unitsWanted.end(), unit) == unitsWanted.end())
                continue;
            Accumulator& accumulator = accumulators[index];
            std::sort(accumulator.nodes.begin(), accumulator.nodes.end());
            accumulator.nodes.erase(std::unique(accumulator.nodes.begin(), accumulator.nodes.end()),
                                    accumulator.nodes.end());
            ConstraintStamp stamp;
            stamp.unit = unit;
            stamp.spcSet = set;
            stamp.constrainedNodeCount = accumulator.nodes.size();
            stamp.revision = RevisionStamp::mix(accumulator.sum ^ RevisionStamp::mix(accumulator.count));
            stamps.push_back(stamp);
        }
    }
    std::stable_sort(stamps.begin(), stamps.end(),
                     [](const ConstraintStamp& left, const ConstraintStamp& right) { return left.unit < right.unit; });
    return stamps;
}

} // namespace oor::analysis
