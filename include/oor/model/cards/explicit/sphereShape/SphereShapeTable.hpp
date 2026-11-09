#pragma once

#include "oor/model/cards/explicit/sphereShape/SphereShapeCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SphereShapeFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SphereShapeCard& card);
    [[nodiscard]] static std::optional<SphereShapeCard> read(FieldReader& reader);
    static void write(const SphereShapeCard& card, FieldWriter& writer);
};

// All the SPHERE cards. The VID is unique among the SPHERE cards (IdSpace::PerCardType) and is in the category
// EulerRegionShape, with the SURFINI and CYLINDR shapes. A SPHERE names no other card.
class SphereShapeTable final : public TypedTable<SphereShapeTable, SphereShapeCard>
{
public:
    static constexpr std::string_view NAME = "SphereShape";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EulerRegionShape, base::IdSpace::PerCardType};
    using Format = SphereShapeFormat;

    [[nodiscard]] static base::EntityId cardId(const SphereShapeCard& card) noexcept { return card.id; }
    std::uint32_t append(const SphereShapeCard& card, base::CardKey key);
    [[nodiscard]] SphereShapeCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SphereShapeCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a SPHERE refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<SphereShapeCard> m_cards;
};

} // namespace oor::model
