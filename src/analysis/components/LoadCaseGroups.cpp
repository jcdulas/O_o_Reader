#include "oor/analysis/components/LoadCaseGroups.hpp"

#include "oor/analysis/components/LoadSets.hpp"
#include "oor/analysis/components/RevisionStamp.hpp"
#include "oor/analysis/components/SubcaseCombination.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <optional>
#include <unordered_map>
#include <utility>

namespace oor::analysis
{

namespace
{

constexpr std::int64_t LONG_RANGE = 4096;

// The elements of the model with their IDs, sorted by ID.
std::vector<std::pair<std::int64_t, model::CardLocation>> elementsSortedById(const model::Model& model)
{
    std::vector<std::pair<std::int64_t, model::CardLocation>> elements;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        if (registry.entry(type).info.category != model::CardCategory::Element)
            continue;
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                elements.emplace_back(table->idAt(row), model::CardLocation{type, row});
        }
    }
    std::sort(elements.begin(), elements.end(),
              [](const auto& left, const auto& right) { return left.first < right.first; });
    return elements;
}

// The load sets one subcase applies, each with the scale of its cards (LoadSets::expansionSets: the loads the expansion
// of its LOAD gives, a set at a time; a combination of subcases those of the subcases it combines, each scale times the
// coefficient). A set reached by thousands of subcases (the cabin pressure of 6 000 LOAD combinations) is read once,
// its footprint on the units kept, not once a subcase.
struct SubcaseLoads
{
    std::int64_t number = 0;
    std::int64_t loadSid = 0;
    std::vector<std::pair<base::EntityId, double>> sets;
};

std::vector<SubcaseLoads> loadsOfSubcases(const model::Model& model, const LoadSetIndex& loadSets)
{
    const model::CaseControl& caseControl = model.control().caseControl;
    const model::CaseLineages lineages(caseControl);
    const std::vector<const model::Subcase*> subcases = analysisSubcases(caseControl);
    std::vector<SubcaseLoads> result(subcases.size());
    for (std::size_t index = 0; index < subcases.size(); ++index)
    {
        const model::Subcase& subcase = *subcases[index];
        SubcaseLoads& entry = result[index];
        entry.number = subcase.id;
        const bool combination = subcase.kind == model::SubcaseKind::SubcaseCombination ||
                                 subcase.kind == model::SubcaseKind::SymmetryCombination;
        if (!combination)
        {
            if (const auto sid = effectiveInteger(lineages, subcase, "StaticLoadSelection"))
            {
                entry.loadSid = *sid;
                if (const auto id = base::toEntityId(*sid))
                {
                    for (const ReachedSet& set : LoadSets::expansionSets(model, loadSets, *id))
                        entry.sets.emplace_back(set.sid, set.scale);
                }
            }
            continue;
        }
        // A combination applies what the subcases it combines apply, each with its coefficient (SUBSEQ aligned as
        // the solver aligns it, SUBSEQ1, SYMSEQ: SubcaseCombination).
        for (const CombinationTerm& term : SubcaseCombination::termsOf(subcases, index))
        {
            if (term.coefficient == 0.0)
                continue;
            for (const auto& [set, scale] : result[term.subcaseIndex].sets)
                entry.sets.emplace_back(set, scale * term.coefficient);
        }
    }
    return result;
}

// What the loads of one subcase do to one component.
struct Accumulator
{
    std::vector<base::EntityId> nodes;
    bool distributed = false;
    std::uint64_t sum = 0;
};

// What the cards of one load set do to the units, whatever the scale they come with: for each unit they touch (sorted),
// the revision hash of each card that touches it, whether one of those is not concentrated, and the grid points of the
// concentrated ones in it. A load of a subcase adds mix(card hash ^ mix(scale)) to every unit it touches.
struct SetFootprint
{
    std::vector<std::int32_t> units;
    std::vector<std::vector<std::uint64_t>> cardHashes;
    std::vector<std::uint8_t> distributed;
    std::vector<std::vector<base::EntityId>> nodes;
};

SetFootprint footprintOf(const model::Model& model, const LoadSetIndex& loadSets, const UnitMembership& units,
                         base::EntityId sid,
                         std::optional<std::vector<std::pair<std::int64_t, model::CardLocation>>>& elementsById)
{
    struct Entry
    {
        std::vector<std::uint64_t> cardHashes;
        bool distributed = false;
        std::vector<base::EntityId> nodes;
    };
    std::map<std::int32_t, Entry> entries;
    const std::size_t count = units.unitCount();
    std::vector<std::int32_t> unitsHere;
    std::vector<std::int32_t> touched;
    for (const ElementaryLoad& load : LoadSets::ownLoads(model, loadSets, sid))
    {
        touched.clear();
        if (load.isGlobal())
        {
            for (std::size_t component = 0; component < count; ++component)
                touched.push_back(static_cast<std::int32_t>(component + 1));
        }
        for (const base::EntityId node : load.nodes)
        {
            unitsHere.clear();
            units.unitsOfNode(node, unitsHere);
            for (const std::int32_t component : unitsHere)
            {
                touched.push_back(component);
                if (load.isConcentrated())
                    entries[component].nodes.push_back(node);
            }
        }
        for (const IdRun& run : load.elements)
        {
            // A short range is looked up ID by ID; a long one (THRU over a wide span of IDs) through the sorted IDs of
            // the elements the model has, so that its cost does not depend on the span.
            if (run.last - run.first < LONG_RANGE)
            {
                for (std::int64_t id = run.first; id <= run.last; ++id)
                {
                    const auto location = model.locate(base::IdSpace::Element, static_cast<base::EntityId>(id));
                    if (location)
                        units.unitsOfElement(*location, touched);
                }
                continue;
            }
            if (!elementsById)
                elementsById = elementsSortedById(model);
            for (auto at = std::lower_bound(elementsById->begin(), elementsById->end(), run.first,
                                            [](const auto& entry, std::int64_t id) { return entry.first < id; });
                 at != elementsById->end() && at->first <= run.last; ++at)
                units.unitsOfElement(at->second, touched);
        }
        std::sort(touched.begin(), touched.end());
        touched.erase(std::unique(touched.begin(), touched.end()), touched.end());
        if (touched.empty())
            continue;
        const std::uint64_t cardHash = RevisionStamp::cardHash(model, load.card);
        for (const std::int32_t component : touched)
        {
            Entry& entry = entries[component];
            entry.cardHashes.push_back(cardHash);
            if (!load.isConcentrated())
                entry.distributed = true;
        }
    }
    SetFootprint footprint;
    for (auto& [component, entry] : entries)
    {
        footprint.units.push_back(component);
        footprint.cardHashes.push_back(std::move(entry.cardHashes));
        footprint.distributed.push_back(entry.distributed ? 1 : 0);
        footprint.nodes.push_back(std::move(entry.nodes));
    }
    return footprint;
}

} // namespace

LoadCaseGroups LoadCaseGroups::compute(const model::Model& model, const ComponentModel& components,
                                       const std::vector<std::int64_t>& subcases)
{
    return compute(model, ComponentMembership(components), subcases);
}

LoadCaseGroups LoadCaseGroups::compute(const model::Model& model, const UnitMembership& units,
                                       const std::vector<std::int64_t>& subcases)
{
    LoadCaseGroups result;
    const LoadSetIndex loadSets(model); // the cards of every load set, read once for all the subcases
    const std::vector<SubcaseLoads> all = loadsOfSubcases(model, loadSets);
    std::unordered_map<base::EntityId, SetFootprint> footprints; // by load set, made the first time a subcase needs it
    const std::size_t count = units.unitCount();
    // groupOf[component]: the node sets met so far and their group numbers (group 0 is the empty set).
    std::vector<std::map<std::vector<base::EntityId>, std::int32_t>> groupOf(count);
    result.m_groups.reserve(count);
    for (std::size_t component = 0; component < count; ++component)
    {
        groupOf[component].emplace(std::vector<base::EntityId>{}, 0);
        LoadGroupNodes empty;
        empty.component = static_cast<std::int32_t>(component + 1);
        empty.group = 0;
        result.m_groups.push_back(std::move(empty)); // group 0 of every component first
    }
    // The groups of each component are found in subcase order; result.m_groups is arranged afterwards.
    std::vector<LoadGroupNodes> found;
    std::optional<std::vector<std::pair<std::int64_t, model::CardLocation>>>
        elementsById; // made when a long range needs it

    for (const SubcaseLoads& subcase : all)
    {
        std::map<std::int32_t, Accumulator> accumulators;
        for (const auto& [set, scale] : subcase.sets)
        {
            if (scale == 0.0)
                continue; // a load of no size changes nothing
            auto found = footprints.find(set);
            if (found == footprints.end())
                found = footprints.emplace(set, footprintOf(model, loadSets, units, set, elementsById)).first;
            const SetFootprint& footprint = found->second;
            std::uint64_t bits = 0;
            std::memcpy(&bits, &scale, sizeof(bits));
            const std::uint64_t scaleHash = RevisionStamp::mix(bits);
            for (std::size_t at = 0; at < footprint.units.size(); ++at)
            {
                Accumulator& accumulator = accumulators[footprint.units[at]];
                for (const std::uint64_t cardHash : footprint.cardHashes[at])
                    accumulator.sum += RevisionStamp::mix(cardHash ^ scaleHash);
                if (footprint.distributed[at] != 0)
                    accumulator.distributed = true;
                accumulator.nodes.insert(accumulator.nodes.end(), footprint.nodes[at].begin(),
                                         footprint.nodes[at].end());
            }
        }
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto component = static_cast<std::int32_t>(index + 1);
            Accumulator accumulator;
            if (const auto entry = accumulators.find(component); entry != accumulators.end())
                accumulator = std::move(entry->second);
            std::sort(accumulator.nodes.begin(), accumulator.nodes.end());
            accumulator.nodes.erase(std::unique(accumulator.nodes.begin(), accumulator.nodes.end()),
                                    accumulator.nodes.end());
            auto& groups = groupOf[index];
            auto group = groups.find(accumulator.nodes);
            if (group == groups.end())
            {
                group = groups.emplace(accumulator.nodes, static_cast<std::int32_t>(groups.size())).first;
                LoadGroupNodes created;
                created.component = component;
                created.group = group->second;
                created.nodes = accumulator.nodes;
                found.push_back(std::move(created));
            }
            const std::int32_t number = group->second;
            LoadCaseRow row;
            row.component = component;
            row.group = number;
            row.subcase = subcase.number;
            row.loadSid = subcase.loadSid;
            row.loadedNodeCount = accumulator.nodes.size();
            row.isDistributed = accumulator.distributed;
            row.loadRevision = RevisionStamp::mix(accumulator.sum);
            result.m_rows.push_back(row);
            // The load cases of the group, in the order of the Case Control.
            if (number == 0)
                result.m_groups[index].subcases.push_back(subcase.number);
            else
            {
                for (LoadGroupNodes& entry : found)
                {
                    if (entry.component == component && entry.group == number)
                        entry.subcases.push_back(subcase.number);
                }
            }
        }
    }
    // The groups by component, then group.
    for (LoadGroupNodes& entry : found)
        result.m_groups.push_back(std::move(entry));
    std::stable_sort(result.m_groups.begin(), result.m_groups.end(),
                     [](const LoadGroupNodes& left, const LoadGroupNodes& right)
                     {
                         if (left.component != right.component)
                             return left.component < right.component;
                         return left.group < right.group;
                     });
    std::stable_sort(result.m_rows.begin(), result.m_rows.end(),
                     [](const LoadCaseRow& left, const LoadCaseRow& right)
                     {
                         if (left.component != right.component)
                             return left.component < right.component;
                         if (left.group != right.group)
                             return left.group < right.group;
                         return left.subcase < right.subcase;
                     });
    if (!subcases.empty())
    {
        std::vector<LoadCaseRow> kept;
        for (const LoadCaseRow& row : result.m_rows)
        {
            if (std::find(subcases.begin(), subcases.end(), row.subcase) != subcases.end())
                kept.push_back(row);
        }
        result.m_rows = std::move(kept);
    }
    return result;
}

const LoadGroupNodes* LoadCaseGroups::find(std::int32_t component, std::int32_t group) const noexcept
{
    for (const LoadGroupNodes& entry : m_groups)
    {
        if (entry.component == component && entry.group == group)
            return &entry;
    }
    return nullptr;
}

} // namespace oor::analysis
