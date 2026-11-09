#pragma once

#include "oor/model/cards/materials/userMat/UserMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct UserMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserMatCard& card);
    [[nodiscard]] static std::optional<UserMatCard> read(FieldReader& reader);
    static void write(const UserMatCard& card, FieldWriter& writer);
};

// All the MATUSR cards. The ID is a material ID, in the space every material type shares (IdSpace::Material): the
// properties of the elements name it. The cards are kept whole (CardRows); the query engine reads the ID only.
class UserMatTable final : public TypedTable<UserMatTable, UserMatCard>
{
public:
    static constexpr std::string_view NAME = "UserMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = UserMatFormat;

    [[nodiscard]] static base::EntityId cardId(const UserMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const UserMatCard& card, base::CardKey key);
    [[nodiscard]] UserMatCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const UserMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a MATUSR refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<UserMatCard> m_cards;
};

} // namespace oor::model
