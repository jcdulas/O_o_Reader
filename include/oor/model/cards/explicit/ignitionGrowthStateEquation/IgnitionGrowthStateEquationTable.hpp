#pragma once

#include "oor/model/cards/explicit/ignitionGrowthStateEquation/IgnitionGrowthStateEquationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct IgnitionGrowthStateEquationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const IgnitionGrowthStateEquationCard& card);
    [[nodiscard]] static std::optional<IgnitionGrowthStateEquationCard> read(FieldReader& reader);
    static void write(const IgnitionGrowthStateEquationCard& card, FieldWriter& writer);
};

// All the EOSIG cards. The ID is the number of an equation of state, unique among the cards of its category
// (IdSpace::PerCardType: the other EOSxxx cards share the category, and a check reports an ID two of them have). The
// cards are kept whole (CardRows) next to their ID: they are few, and the query engine reads the ID.
class IgnitionGrowthStateEquationTable final
    : public TypedTable<IgnitionGrowthStateEquationTable, IgnitionGrowthStateEquationCard>
{
public:
    static constexpr std::string_view NAME = "IgnitionGrowthStateEquation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EquationOfState, base::IdSpace::PerCardType};
    using Format = IgnitionGrowthStateEquationFormat;

    [[nodiscard]] static base::EntityId cardId(const IgnitionGrowthStateEquationCard& card) noexcept { return card.id; }
    std::uint32_t append(const IgnitionGrowthStateEquationCard& card, base::CardKey key);
    [[nodiscard]] IgnitionGrowthStateEquationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const IgnitionGrowthStateEquationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an EOSIG refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<IgnitionGrowthStateEquationCard> m_cards;
};

} // namespace oor::model
