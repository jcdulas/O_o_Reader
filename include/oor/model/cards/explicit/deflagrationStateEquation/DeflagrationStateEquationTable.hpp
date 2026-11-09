#pragma once

#include "oor/model/cards/explicit/deflagrationStateEquation/DeflagrationStateEquationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DeflagrationStateEquationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DeflagrationStateEquationCard& card);
    [[nodiscard]] static std::optional<DeflagrationStateEquationCard> read(FieldReader& reader);
    static void write(const DeflagrationStateEquationCard& card, FieldWriter& writer);
};

// All the EOSDEF cards. The ID is the number of an equation of state, unique among the cards of its category
// (IdSpace::PerCardType: the other EOSxxx cards share the category, and a check reports an ID two of them have). The
// cards are kept whole (CardRows) next to their ID: they are few, and the query engine reads the ID.
class DeflagrationStateEquationTable final
    : public TypedTable<DeflagrationStateEquationTable, DeflagrationStateEquationCard>
{
public:
    static constexpr std::string_view NAME = "DeflagrationStateEquation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EquationOfState, base::IdSpace::PerCardType};
    using Format = DeflagrationStateEquationFormat;

    [[nodiscard]] static base::EntityId cardId(const DeflagrationStateEquationCard& card) noexcept { return card.id; }
    std::uint32_t append(const DeflagrationStateEquationCard& card, base::CardKey key);
    [[nodiscard]] DeflagrationStateEquationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DeflagrationStateEquationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an EOSDEF refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DeflagrationStateEquationCard> m_cards;
};

} // namespace oor::model
