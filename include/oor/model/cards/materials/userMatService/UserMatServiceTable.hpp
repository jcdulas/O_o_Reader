#pragma once

#include "oor/model/cards/materials/userMatService/UserMatServiceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct UserMatServiceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserMatServiceCard& card);
    [[nodiscard]] static std::optional<UserMatServiceCard> read(FieldReader& reader);
    static void write(const UserMatServiceCard& card, FieldWriter& writer);
};

// All the MATUDS cards. The ID space is the card type's own: the ID is the one of the material that the card serves, so
// it is also a reference to a material (renumbering the materials follows into it). The cards are kept whole (CardRows:
// the data blocks have a length of their own); the query engine reads the ID only.
class UserMatServiceTable final : public TypedTable<UserMatServiceTable, UserMatServiceCard>
{
public:
    static constexpr std::string_view NAME = "UserMatService";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = UserMatServiceFormat;

    [[nodiscard]] static base::EntityId cardId(const UserMatServiceCard& card) noexcept { return card.id; }
    std::uint32_t append(const UserMatServiceCard& card, base::CardKey key);
    [[nodiscard]] UserMatServiceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const UserMatServiceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<UserMatServiceCard> m_cards;
};

} // namespace oor::model
