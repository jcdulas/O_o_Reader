#pragma once

#include "oor/model/cards/materials/userMatTable/UserMatTableCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct UserMatTableFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserMatTableCard& card);
    [[nodiscard]] static std::optional<UserMatTableCard> read(FieldReader& reader);
    static void write(const UserMatTableCard& card, FieldWriter& writer);
};

// All the MATTUSR cards. The ID space is the card type's own: the ID is the one of the MATUSR that the card extends, so
// it is also a reference to a material (renumbering the materials follows into it). T(GE), T(ST), T(SC) and T(SS) are
// references to TABLEMi cards (ReferenceTarget::MaterialTable); T(RHO) and T(Ai) may name a TABL3Di, which no card
// types yet, so they are plain IDs that the reference checks leave alone. The cards are kept whole (CardRows).
class UserMatTableTable final : public TypedTable<UserMatTableTable, UserMatTableCard>
{
public:
    static constexpr std::string_view NAME = "UserMatTable";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = UserMatTableFormat;

    [[nodiscard]] static base::EntityId cardId(const UserMatTableCard& card) noexcept { return card.id; }
    std::uint32_t append(const UserMatTableCard& card, base::CardKey key);
    [[nodiscard]] UserMatTableCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const UserMatTableCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<UserMatTableCard> m_cards;
};

} // namespace oor::model
