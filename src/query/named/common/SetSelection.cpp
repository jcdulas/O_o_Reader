#include "oor/query/named/common/SetSelection.hpp"

#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/ids/IdRangeExpansion.hpp"
#include "oor/model/cards/sets/idListSet1/IdListSet1Table.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/query/QueryErrors.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <utility>

namespace oor::query
{

std::optional<std::vector<base::EntityId>> askedSets(const model::Model& model, const QueryArguments& arguments,
                                                     std::string_view command)
{
    const bool bySids = arguments.has("setIds");
    const bool bySubcases = arguments.has("subcases");
    if (!bySids && !bySubcases)
        return std::nullopt;
    std::set<base::EntityId> sets;
    if (const auto ids = arguments.ids("setIds"))
    {
        for (const std::int64_t id : *ids)
        {
            if (const auto converted = base::toEntityId(id))
                sets.insert(*converted);
        }
    }
    if (const auto numbers = arguments.ids("subcases"))
    {
        const model::CaseControl& caseControl = model.control().caseControl;
        const model::CaseLineages lineages(caseControl);
        for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
        {
            if (std::find(numbers->begin(), numbers->end(), subcase->id) == numbers->end())
                continue;
            if (const auto sid = analysis::effectiveInteger(lineages, *subcase, command))
            {
                if (const auto converted = base::toEntityId(*sid))
                    sets.insert(*converted);
            }
        }
    }
    return std::vector<base::EntityId>(sets.begin(), sets.end());
}

std::vector<base::EntityId> allLoadSets(const model::Model& model)
{
    const std::unordered_set<base::EntityId> ids = model::setIdsOfCategory(model, model::CardCategory::Load);
    std::vector<base::EntityId> sets(ids.begin(), ids.end());
    std::sort(sets.begin(), sets.end());
    return sets;
}

ParameterInfo expandCombinationsParameter()
{
    return parameter(std::string(EXPAND_COMBINATIONS), ParameterKind::Bool, "True",
                     "False: each load set once with its own cards (setId the set of the card, scale 1), a LOAD "
                     "combination replaced by the sets it reaches; loadCombinationsTable gives its factors");
}

LoadSetListing::LoadSetListing(const model::Model& model, const analysis::LoadSetIndex& index,
                               const QueryArguments& arguments, std::vector<base::EntityId> asked)
    : LoadSetListing(model, index, std::move(asked), arguments.boolOr(EXPAND_COMBINATIONS, true))
{
}

LoadSetListing::LoadSetListing(const model::Model& model, const analysis::LoadSetIndex& index,
                               std::vector<base::EntityId> asked, bool expands)
    : m_model(model), m_index(index), m_expands(expands)
{
    if (m_expands)
    {
        m_sets = std::move(asked);
        return;
    }
    for (const base::EntityId sid : asked)
    {
        for (const analysis::ReachedSet& reached : analysis::LoadSets::reachedSets(model, index, sid))
            m_sets.push_back(reached.sid);
        // the cards of its own of a set that also holds a LOAD entry (the expansion loads them, with the scale 1)
        if (index.hasOwnCards(sid))
            m_sets.push_back(sid);
    }
    std::sort(m_sets.begin(), m_sets.end());
    m_sets.erase(std::unique(m_sets.begin(), m_sets.end()), m_sets.end());
}

std::size_t LoadSetListing::cardCount(std::span<const std::uint16_t> types) const
{
    if (m_expands)
        return 0;
    std::size_t count = 0;
    for (const base::EntityId sid : m_sets)
    {
        const std::vector<model::CardLocation>& cards = m_index.cardsOf(sid);
        if (types.empty())
            count += cards.size();
        else
            count += static_cast<std::size_t>(
                std::count_if(cards.begin(), cards.end(), [&](const model::CardLocation& card)
                              { return std::binary_search(types.begin(), types.end(), card.table); }));
    }
    return count;
}

std::vector<analysis::ElementaryLoad> LoadSetListing::loadsOf(base::EntityId sid,
                                                              std::span<const std::uint16_t> types) const
{
    return m_expands ? analysis::LoadSets::expand(m_model, m_index, sid)
                     : analysis::LoadSets::ownLoads(m_model, m_index, sid, types);
}

std::vector<std::int32_t> idSetMembers(const model::Model& model, const NamedIdSet& set, std::string_view query)
{
    std::vector<std::int32_t> ids;
    for (const std::int64_t id : set.ids)
        ids.push_back(static_cast<std::int32_t>(id));
    if (set.setId)
    {
        bool found = false;
        if (const auto sid = base::toEntityId(*set.setId))
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
                base::requireExpandable(count, std::string(query) + ": the SET1 " + std::to_string(*set.setId));
                for (const model::IdRun& run : card.ids)
                    for (std::int64_t id = run.first; id <= run.last; ++id)
                        ids.push_back(static_cast<std::int32_t>(id));
            }
        if (!found)
            throw QueryError(std::string(query) + ": there is no SET1 with the ID " + std::to_string(*set.setId));
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

std::span<const std::int32_t> idsInRun(std::span<const std::int32_t> sortedIds, std::int64_t first, std::int64_t last)
{
    const auto begin = std::lower_bound(sortedIds.begin(), sortedIds.end(), first,
                                        [](std::int32_t id, std::int64_t value) { return id < value; });
    if (first == last)
    {
        // One ID (a PLOAD2 or PLOAD4 of one element: millions of them): the cards of that ID follow `begin`.
        auto end = begin;
        while (end != sortedIds.end() && *end == first)
            ++end;
        return std::span<const std::int32_t>(sortedIds).subspan(static_cast<std::size_t>(begin - sortedIds.begin()),
                                                                static_cast<std::size_t>(end - begin));
    }
    const auto end =
        std::upper_bound(begin, sortedIds.end(), last, [](std::int64_t value, std::int32_t id) { return value < id; });
    return std::span<const std::int32_t>(sortedIds).subspan(static_cast<std::size_t>(begin - sortedIds.begin()),
                                                            static_cast<std::size_t>(end - begin));
}

} // namespace oor::query
