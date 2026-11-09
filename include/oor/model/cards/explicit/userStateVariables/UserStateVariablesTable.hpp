#pragma once

#include "oor/model/cards/explicit/userStateVariables/UserStateVariablesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct UserStateVariablesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserStateVariablesCard& card);
    [[nodiscard]] static std::optional<UserStateVariablesCard> read(FieldReader& reader);
    static void write(const UserStateVariablesCard& card, FieldWriter& writer);
};

// All the UDSESV cards (a global entry: a deck has one in practice). The card has no ID (IdSpace::None, category Other)
// and refers to no other card. The cards are kept whole (CardRows): the list of names has a length of its own. A column
// of the number of state variables is there for the query engine.
class UserStateVariablesTable final : public TypedTable<UserStateVariablesTable, UserStateVariablesCard>
{
public:
    static constexpr std::string_view NAME = "UserStateVariables";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = UserStateVariablesFormat;

    [[nodiscard]] static base::EntityId cardId(const UserStateVariablesCard&) noexcept { return 0; }
    std::uint32_t append(const UserStateVariablesCard& card, base::CardKey key);
    [[nodiscard]] UserStateVariablesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const UserStateVariablesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a UDSESV refers to no other card

    [[nodiscard]] Column<std::int32_t>& variableCountColumn() noexcept { return m_variableCount; }

private:
    Column<std::int32_t> m_variableCount;
    CardRows<UserStateVariablesCard> m_cards;
};

} // namespace oor::model
