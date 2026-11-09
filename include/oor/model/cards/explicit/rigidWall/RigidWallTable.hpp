#pragma once

#include "oor/model/cards/explicit/rigidWall/RigidWallCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RigidWallFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidWallCard& card);
    [[nodiscard]] static std::optional<RigidWallCard> read(FieldReader& reader);
    static void write(const RigidWallCard& card, FieldWriter& writer);
};

// All the WALL cards. The ID is unique among them (IdSpace::PerCardType) and no other card names it, so the category is
// Other. The BCGRID is a card of the contact body category (a SOL 700 BCGRID, ContactBody): the reference is visited
// with that target, so that renumbering the BCGRID cards follows into the wall. The cards are kept whole (CardRows).
class RigidWallTable final : public TypedTable<RigidWallTable, RigidWallCard>
{
public:
    static constexpr std::string_view NAME = "RigidWall";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = RigidWallFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidWallCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidWallCard& card, base::CardKey key);
    [[nodiscard]] RigidWallCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RigidWallCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<RigidWallCard> m_cards;
};

} // namespace oor::model
