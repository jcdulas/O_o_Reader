#include "AeroQuerySupport.hpp"

#include "oor/model/cards/aero/aeroBoxList/AeroBoxListTable.hpp"
#include "oor/model/cards/sets/idListSet1/IdListSet1Table.hpp"
#include "oor/model/cards/sets/typedListSet3/TypedListSet3Table.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/query/QueryErrors.hpp"

#include <algorithm>

namespace oor::query::aero
{

IdRuns::IdRuns(std::vector<model::IdRun> runs)
{
    std::erase_if(runs, [](const model::IdRun& run) { return run.last < run.first; });
    std::ranges::sort(runs, {}, &model::IdRun::first);
    for (const model::IdRun& run : runs)
    {
        // Runs that touch (the next starts right after the previous ends) are one run: 1 THRU 5 and 6 THRU 9.
        if (!m_runs.empty() &&
            static_cast<std::int64_t>(run.first) <= static_cast<std::int64_t>(m_runs.back().last) + 1)
            m_runs.back().last = std::max(m_runs.back().last, run.last);
        else
            m_runs.push_back(run);
    }
}

std::int64_t IdRuns::count() const noexcept
{
    std::int64_t total = 0;
    for (const model::IdRun& run : m_runs)
        total += static_cast<std::int64_t>(run.last) - run.first + 1;
    return total;
}

IdRuns IdRuns::united(const IdRuns& other) const
{
    std::vector<model::IdRun> all = m_runs;
    all.insert(all.end(), other.m_runs.begin(), other.m_runs.end());
    return IdRuns(std::move(all));
}

std::vector<base::EntityId> IdRuns::expand(const std::string& what) const
{
    const std::int64_t total = count();
    if (total > MAX_LISTED_IDS)
        throw QueryError(what + " lists " + std::to_string(total) + " IDs, more than a table row holds (" +
                         std::to_string(MAX_LISTED_IDS) + ")");
    std::vector<base::EntityId> ids;
    ids.reserve(static_cast<std::size_t>(total));
    for (const model::IdRun& run : m_runs)
    {
        for (std::int64_t id = run.first; id <= run.last; ++id)
            ids.push_back(static_cast<base::EntityId>(id));
    }
    return ids;
}

std::optional<IdRuns> aelistItems(const model::Model& model, base::EntityId sid)
{
    const auto* table = model.findTable<model::AeroBoxListTable>();
    if (table == nullptr)
        return std::nullopt;
    const std::optional<model::CardLocation> found =
        model.locateInType(model::typeIndexOf<model::AeroBoxListTable>(), sid);
    if (!found)
        return std::nullopt;
    return IdRuns(table->card(found->row).ids);
}

std::optional<IdRuns> setMembers(const model::Model& model, base::EntityId sid)
{
    std::vector<model::IdRun> runs;
    bool found = false;
    for (const model::CardLocation location : model::cardsOfSet(model, model::CardCategory::Set, sid))
    {
        if (analysis::isType<model::IdListSet1Table>(location))
        {
            found = true;
            const model::IdListSet1Card card = analysis::cardAt<model::IdListSet1Table>(model, location);
            runs.insert(runs.end(), card.ids.begin(), card.ids.end());
        }
        else if (analysis::isType<model::TypedListSet3Table>(location))
        {
            const model::TypedListSet3Card card = analysis::cardAt<model::TypedListSet3Table>(model, location);
            // A SET3 may list elements, properties or rigid elements: only a list of grid points is a set of points.
            if (card.kind != model::TypedListSet3Kind::Grid)
                continue;
            found = true;
            runs.insert(runs.end(), card.ids.begin(), card.ids.end());
        }
    }
    if (!found)
        return std::nullopt;
    return IdRuns(std::move(runs));
}

std::map<base::EntityId, std::vector<std::int32_t>> subcasesSelecting(const model::Model& model,
                                                                      std::string_view command)
{
    std::map<base::EntityId, std::vector<std::int32_t>> selecting;
    const model::CaseControl& caseControl = model.control().caseControl;
    const model::CaseLineages lineages(caseControl);
    for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
    {
        const std::optional<std::int64_t> selected = analysis::effectiveInteger(lineages, *subcase, command);
        if (!selected)
            continue;
        if (const std::optional<base::EntityId> sid = base::toEntityId(*selected))
            selecting[*sid].push_back(static_cast<std::int32_t>(subcase->id));
    }
    return selecting;
}

} // namespace oor::query::aero
