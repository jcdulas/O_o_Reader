#pragma once

#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// What the queries of the aeroelastic model share: the IDs a list of runs stands for (the boxes of an AELIST, the
// points of a SET1 or SET3), the cards of one type, and the subcases that select a card by a Case Control command.

namespace oor::query::aero
{

// A set of IDs kept as sorted runs that neither overlap nor touch, so that the count, the first and the last ID of a
// "1001 THRU 2000000" cost nothing and the IDs are listed only when a table needs them.
class IdRuns
{
public:
    IdRuns() = default;
    // Sorts the runs and joins the ones that overlap or follow each other; a run that goes down is dropped.
    explicit IdRuns(std::vector<model::IdRun> runs);

    [[nodiscard]] bool empty() const noexcept { return m_runs.empty(); }
    // The number of distinct IDs.
    [[nodiscard]] std::int64_t count() const noexcept;
    // The lowest and the highest ID; the set must not be empty.
    [[nodiscard]] base::EntityId first() const noexcept { return m_runs.front().first; }
    [[nodiscard]] base::EntityId last() const noexcept { return m_runs.back().last; }
    [[nodiscard]] const std::vector<model::IdRun>& runs() const noexcept { return m_runs; }

    // This set and `other` together.
    [[nodiscard]] IdRuns united(const IdRuns& other) const;
    // The distinct IDs in increasing order. A list that holds more than `MAX_LISTED_IDS` of them is refused (a
    // QueryError that names `what`): no table row is meant to hold a million boxes, and a damaged THRU must not exhaust
    // the memory.
    [[nodiscard]] std::vector<base::EntityId> expand(const std::string& what) const;

    static constexpr std::int64_t MAX_LISTED_IDS = 4'000'000;

private:
    std::vector<model::IdRun> m_runs;
};

// The items of the AELIST with this SID (aerodynamic boxes, or the points of an aerodynamic mesh); nothing when no
// AELIST has it.
[[nodiscard]] std::optional<IdRuns> aelistItems(const model::Model& model, base::EntityId sid);

// The IDs of the SET1 cards and the SET3 cards of grid points that have this set ID (several cards with one ID are one
// set); nothing when no such card exists.
[[nodiscard]] std::optional<IdRuns> setMembers(const model::Model& model, base::EntityId sid);

// The live cards of one type, in the order of the deck.
template <typename TableT>
[[nodiscard]] std::vector<typename TableT::CardType> cardsOf(const model::Model& model)
{
    std::vector<typename TableT::CardType> cards;
    analysis::forEachCard<TableT>(
        model, [&](const typename TableT::CardType& card, model::CardLocation) { cards.push_back(card); });
    return cards;
}

// The subcases of the Case Control whose command `command` (TRIM, FMETHOD, ...) selects each set ID, in the order of
// the subcases. The command may be inherited from the global section.
[[nodiscard]] std::map<base::EntityId, std::vector<std::int32_t>> subcasesSelecting(const model::Model& model,
                                                                                     std::string_view command);

} // namespace oor::query::aero
