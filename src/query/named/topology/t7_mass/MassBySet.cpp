#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/ids/IdRangeExpansion.hpp"
#include "oor/model/cards/sets/idListSet1/IdListSet1Table.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/common/MassGroups.hpp"
#include "oor/query/named/topology/t7_mass/MassQueries.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::query
{

namespace
{

// The element IDs a named set stands for: the IDs given, or the members of the SET1 cards with this ID.
std::vector<std::int32_t> membersOf(const model::Model& model, const NamedIdSet& set)
{
    std::vector<std::int32_t> ids;
    for (const std::int64_t id : set.ids)
        ids.push_back(static_cast<std::int32_t>(id));
    if (set.setId)
    {
        const auto sid = base::toEntityId(*set.setId);
        if (sid)
        {
            bool found = false;
            for (const model::CardLocation location : model::cardsOfSet(model, model::CardCategory::Set, *sid))
            {
                if (location.table != model::typeIndexOf<model::IdListSet1Table>())
                    continue;
                found = true;
                const model::IdListSet1Card card =
                    static_cast<const model::IdListSet1Table&>(*model.findTable(location.table)).card(location.row);
                std::uint64_t count = ids.size();
                for (const model::IdRun& run : card.ids)
                    count += base::idRangeCount(run.first, run.last);
                base::requireExpandable(count, "massBySet: the SET1 " + std::to_string(*set.setId));
                for (const model::IdRun& run : card.ids)
                {
                    for (std::int64_t id = run.first; id <= run.last; ++id)
                        ids.push_back(static_cast<std::int32_t>(id));
                }
            }
            if (!found)
                throw QueryError("massBySet: there is no SET1 with the ID " + std::to_string(*set.setId));
        }
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

// The element cards with these IDs (sorted, distinct), in the order of the IDs, one card an ID: the one `listCards`
// puts first among the elements of that ID (the lowest card key, which is file order). Found through the ID indexes of
// the model, so that the cost is in the IDs asked for, not in the elements of the model (a companion package,
// 2026-10-18: a call cost the masses of the whole model, 0.14 s at 540 000 elements, whatever its sets); an ID that no
// element has is left out. The element types of an ID space that has no index (none today) are scanned.
CardList elementsWithIds(const model::Model& model, const std::vector<std::int32_t>& ids)
{
    const model::CardRegistry& registry = model::CardRegistry::instance();
    std::vector<bool> isElementType(registry.size(), false);
    std::vector<base::IdSpace> sharedSpaces;
    std::vector<std::uint16_t> perTypeIndexed;
    std::vector<std::uint16_t> unindexed;
    for (const std::uint16_t type : typesOfCategory(model::CardCategory::Element))
    {
        isElementType[type] = true;
        const base::IdSpace space = registry.entry(type).info.idSpace;
        if (space == base::IdSpace::PerCardType)
            perTypeIndexed.push_back(type);
        else if (space == base::IdSpace::None || space == base::IdSpace::Group)
            unindexed.push_back(type);
        else if (std::find(sharedSpaces.begin(), sharedSpaces.end(), space) == sharedSpaces.end())
            sharedSpaces.push_back(space);
    }
    CardList scanned;
    if (!unindexed.empty())
        scanned = listCards(model, unindexed, std::vector<std::int64_t>(ids.begin(), ids.end()));

    CardList list;
    for (const std::int32_t id : ids)
    {
        std::optional<model::CardLocation> first;
        std::uint32_t firstKey = 0;
        const auto consider = [&](const std::optional<model::CardLocation>& location)
        {
            if (!location || !isElementType[location->table])
                return;
            const model::CardTable* table = model.findTable(location->table);
            if (table == nullptr || !table->rows().isAlive(location->row))
                return;
            const std::uint32_t key = table->rows().keyAt(location->row).value;
            if (!first || key < firstKey)
            {
                first = location;
                firstKey = key;
            }
        };
        for (const base::IdSpace space : sharedSpaces)
            consider(model.locate(space, id));
        for (const std::uint16_t type : perTypeIndexed)
            consider(model.locateInType(type, id));
        auto found = std::lower_bound(scanned.ids.begin(), scanned.ids.end(), id);
        for (; found != scanned.ids.end() && *found == id; ++found)
            consider(scanned.location(static_cast<std::size_t>(found - scanned.ids.begin())));
        if (!first)
            continue;
        list.types.push_back(first->table);
        list.rows.push_back(first->row);
        list.ids.push_back(id);
        list.keys.push_back(firstKey);
    }
    return list;
}

// massBySet: the mass of named sets of elements.
class MassBySetQuery final : public QueryBase
{
public:
    MassBySetQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::vector<NamedIdSet> sets = run.arguments.idSets("sets");
        if (sets.empty())
            throw QueryError("massBySet: give the sets: a dict {name: element IDs} or a list of SET1 IDs");
        // The members of each set (a SET1 that does not exist is an error whatever the model holds), then the masses of
        // the elements of the sets alone: an element's mass does not depend on the other elements (the NSM of the
        // Case Control is resolved over the model by MassSetting), so each set's sum, over its members in the order
        // of their IDs, is the one the masses of every element gave.
        std::vector<std::vector<std::int32_t>> members;
        members.reserve(sets.size());
        std::vector<std::int32_t> asked;
        for (const NamedIdSet& set : sets)
        {
            members.push_back(membersOf(*run.model, set));
            asked.insert(asked.end(), members.back().begin(), members.back().end());
        }
        std::sort(asked.begin(), asked.end());
        asked.erase(std::unique(asked.begin(), asked.end()), asked.end());
        const CardList elements = elementsWithIds(*run.model, asked);
        std::vector<GroupTotals> totals(sets.size());
        if (elements.size() > 0)
        {
            ElementData data(run.model, elements, run.context);
            const ElementMassData& masses = data.masses();
            for (std::size_t number = 0; number < sets.size(); ++number)
            {
                for (const std::int32_t id : members[number])
                {
                    const auto found = std::lower_bound(elements.ids.begin(), elements.ids.end(), id);
                    if (found == elements.ids.end() || *found != id)
                        continue;
                    addElementMass(totals[number], masses, static_cast<std::size_t>(found - elements.ids.begin()), 1.0,
                                   true, true);
                }
            }
        }
        const std::vector<std::size_t> order = orderByMass(totals);
        std::vector<std::string> names;
        for (const std::size_t index : order)
            names.push_back(sets[index].name);
        TableBuilder builder;
        builder.text("set", names);
        addMassColumns(builder, totals, order);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "massBySet";
        info.family = "T7";
        info.description =
            "The mass of named sets of elements: `sets` is a dict {name: element IDs} or a list of SET1 IDs (the set "
            "is then "
            "named by its ID). One row for each set, also one that holds no element. IDs that are not elements are "
            "ignored.";
        info.parameters = {parameter("sets", ParameterKind::IdSets, "(required)", "{name: element IDs}, or SET1 IDs")};
        info.defaultSort = "-mass";
        ColumnListBuilder columns;
        columns.text("set").int64("elementCount").reals("structuralMass nonstructuralMass mass cx cy cz");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMassBySetQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MassBySetQuery>());
}

} // namespace oor::query
