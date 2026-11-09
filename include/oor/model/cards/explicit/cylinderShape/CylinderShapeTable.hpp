#pragma once

#include "oor/model/cards/explicit/cylinderShape/CylinderShapeCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CylinderShapeFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CylinderShapeCard& card);
    [[nodiscard]] static std::optional<CylinderShapeCard> read(FieldReader& reader);
    static void write(const CylinderShapeCard& card, FieldWriter& writer);
};

// All the CYLINDR cards. The VID is unique among the CYLINDR cards (IdSpace::PerCardType) and is in the category
// EulerRegionShape, with the SURFINI and SPHERE shapes. A CYLINDR names no other card.
class CylinderShapeTable final : public TypedTable<CylinderShapeTable, CylinderShapeCard>
{
public:
    static constexpr std::string_view NAME = "CylinderShape";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EulerRegionShape, base::IdSpace::PerCardType};
    using Format = CylinderShapeFormat;

    [[nodiscard]] static base::EntityId cardId(const CylinderShapeCard& card) noexcept { return card.id; }
    std::uint32_t append(const CylinderShapeCard& card, base::CardKey key);
    [[nodiscard]] CylinderShapeCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CylinderShapeCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a CYLINDR refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CylinderShapeCard> m_cards;
};

} // namespace oor::model
