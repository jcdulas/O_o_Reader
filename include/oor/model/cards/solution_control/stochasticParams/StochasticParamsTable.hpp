#pragma once

#include "oor/model/cards/solution_control/stochasticParams/StochasticParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct StochasticParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const StochasticParamsCard& card);
    [[nodiscard]] static std::optional<StochasticParamsCard> read(FieldReader& reader);
    static void write(const StochasticParamsCard& card, FieldWriter& writer);
};

// All the STOCHAS cards (category Other). The SID is unique (IdSpace::PerCardType); the Case Control STOCHASTICS that
// selects it is kept as text, so no reference to it is resolved. The card names no other card. It is kept whole next to
// a column of its IDs.
class StochasticParamsTable final : public TypedTable<StochasticParamsTable, StochasticParamsCard>
{
public:
    static constexpr std::string_view NAME = "StochasticParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = StochasticParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const StochasticParamsCard& card) noexcept { return card.id; }
    std::uint32_t append(const StochasticParamsCard& card, base::CardKey key);
    [[nodiscard]] StochasticParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const StochasticParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field names another card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<StochasticParamsCard> m_cards;
};

} // namespace oor::model
