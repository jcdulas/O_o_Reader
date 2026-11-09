#pragma once

#include "oor/model/cards/matrices/monteCarloParams/MonteCarloParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct MonteCarloParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MonteCarloParamsCard& card);
    [[nodiscard]] static std::optional<MonteCarloParamsCard> read(FieldReader& reader);
    static void write(const MonteCarloParamsCard& card, FieldWriter& writer);
};

// All the MONCARL cards, in the category MonteCarloSet: the Case Control MONCARL names one by its SID, which is unique
// (IdSpace::PerCardType). The card names no other card. It is kept whole next to a column of its IDs.
class MonteCarloParamsTable final : public TypedTable<MonteCarloParamsTable, MonteCarloParamsCard>
{
public:
    static constexpr std::string_view NAME = "MonteCarloParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MonteCarloSet, base::IdSpace::PerCardType};
    using Format = MonteCarloParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const MonteCarloParamsCard& card) noexcept { return card.id; }
    std::uint32_t append(const MonteCarloParamsCard& card, base::CardKey key);
    [[nodiscard]] MonteCarloParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MonteCarloParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field names another card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<MonteCarloParamsCard> m_cards;
};

} // namespace oor::model
