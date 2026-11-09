#pragma once

#include "oor/model/cards/explicit/userStateEquation/UserStateEquationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct UserStateEquationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserStateEquationCard& card);
    [[nodiscard]] static std::optional<UserStateEquationCard> read(FieldReader& reader);
    static void write(const UserStateEquationCard& card, FieldWriter& writer);
};

// All the EOSUDS cards. The ID is the number of an equation of state, unique among the cards of its category
// (IdSpace::PerCardType: the other EOSxxx cards share the category, and a check reports an ID two of them have). The
// cards are kept whole (CardRows) next to their ID: they are few, and the query engine reads the ID.
class UserStateEquationTable final : public TypedTable<UserStateEquationTable, UserStateEquationCard>
{
public:
    static constexpr std::string_view NAME = "UserStateEquation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EquationOfState, base::IdSpace::PerCardType};
    using Format = UserStateEquationFormat;

    [[nodiscard]] static base::EntityId cardId(const UserStateEquationCard& card) noexcept { return card.id; }
    std::uint32_t append(const UserStateEquationCard& card, base::CardKey key);
    [[nodiscard]] UserStateEquationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const UserStateEquationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an EOSUDS refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<UserStateEquationCard> m_cards;
};

} // namespace oor::model
