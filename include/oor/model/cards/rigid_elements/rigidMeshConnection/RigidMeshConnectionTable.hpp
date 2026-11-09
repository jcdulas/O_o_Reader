#pragma once

#include "oor/model/cards/rigid_elements/rigidMeshConnection/RigidMeshConnectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RigidMeshConnectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidMeshConnectionCard& card);
    [[nodiscard]] static std::optional<RigidMeshConnectionCard> read(FieldReader& reader);
    static void write(const RigidMeshConnectionCard& card, FieldWriter& writer);
};

// All the RCONN cards. The connection number is unique per card type (IdSpace::PerCardType, category Other) and no
// other card names it. SID names a BSURF (ReferenceTarget::ContactSurface) only when STYPE is SURF; with GRID it is the
// ID of a SET1 or of a BCGRID, which the card cannot tell apart, so it is not visited. MID always names a BSURF. The
// cards are kept whole (CardRows) next to the column of their CID: they are few and nobody reads them as columns.
class RigidMeshConnectionTable final : public TypedTable<RigidMeshConnectionTable, RigidMeshConnectionCard>
{
public:
    static constexpr std::string_view NAME = "RigidMeshConnection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = RigidMeshConnectionFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidMeshConnectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidMeshConnectionCard& card, base::CardKey key);
    [[nodiscard]] RigidMeshConnectionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RigidMeshConnectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<RigidMeshConnectionCard> m_cards;
};

} // namespace oor::model
