#pragma once

#include "oor/model/cards/materials/userCompositeFailureMat/UserCompositeFailureMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct UserCompositeFailureMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserCompositeFailureMatCard& card);
    [[nodiscard]] static std::optional<UserCompositeFailureMatCard> read(FieldReader& reader);
    static void write(const UserCompositeFailureMatCard& card, FieldWriter& writer);
};

// All the COMPUDS cards: materials of their own (IdSpace::Material, shared with every other material), kept whole
// (CardRows) next to the column of their IDs; the query engine reads the MID. A COMPUDS names no other card.
class UserCompositeFailureMatTable final : public TypedTable<UserCompositeFailureMatTable, UserCompositeFailureMatCard>
{
public:
    static constexpr std::string_view NAME = "UserCompositeFailureMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = UserCompositeFailureMatFormat;

    [[nodiscard]] static base::EntityId cardId(const UserCompositeFailureMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const UserCompositeFailureMatCard& card, base::CardKey key);
    [[nodiscard]] UserCompositeFailureMatCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const UserCompositeFailureMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a COMPUDS refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<UserCompositeFailureMatCard> m_cards;
};

} // namespace oor::model
