#pragma once

#include "oor/model/cards/explicit/polynomialStateEquation/PolynomialStateEquationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PolynomialStateEquationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PolynomialStateEquationCard& card);
    [[nodiscard]] static std::optional<PolynomialStateEquationCard> read(FieldReader& reader);
    static void write(const PolynomialStateEquationCard& card, FieldWriter& writer);
};

// All the EOSPOL cards. The ID is the number of an equation of state, unique among the cards of its category
// (IdSpace::PerCardType: the other EOSxxx cards share the category, and a check reports an ID two of them have). The
// cards are kept whole (CardRows) next to their ID: they are few, and the query engine reads the ID.
class PolynomialStateEquationTable final : public TypedTable<PolynomialStateEquationTable, PolynomialStateEquationCard>
{
public:
    static constexpr std::string_view NAME = "PolynomialStateEquation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EquationOfState, base::IdSpace::PerCardType};
    using Format = PolynomialStateEquationFormat;

    [[nodiscard]] static base::EntityId cardId(const PolynomialStateEquationCard& card) noexcept { return card.id; }
    std::uint32_t append(const PolynomialStateEquationCard& card, base::CardKey key);
    [[nodiscard]] PolynomialStateEquationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PolynomialStateEquationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an EOSPOL refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PolynomialStateEquationCard> m_cards;
};

} // namespace oor::model
