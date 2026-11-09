#pragma once

#include "oor/model/cards/aero/aeroSplineStripBlend/AeroSplineStripBlendCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroSplineStripBlendFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroSplineStripBlendCard& card);
    [[nodiscard]] static std::optional<AeroSplineStripBlendCard> read(FieldReader& reader);
    static void write(const AeroSplineStripBlendCard& card, FieldWriter& writer);
};

// All the SPBLND1 cards, in the category AeroSpline (the number space of the splines is shared by SPLINE1 to SPLINE7,
// SPLINEX, SPBLND1, SPBLND2 and SPLINRB; IdSpace::PerCardType refuses a repeat inside this type and a check across the
// types reports the others). SID1 and SID2 name splines of any of those types (ReferenceTarget::AeroSpline), GID an
// aerodynamic grid (ReferenceTarget::AeroNode) and CID a coordinate system. The cards are kept whole (CardRows) next to
// the column of their SID: a SPBLND1 has a dozen fields and few of them are ever set.
class AeroSplineStripBlendTable final : public TypedTable<AeroSplineStripBlendTable, AeroSplineStripBlendCard>
{
public:
    static constexpr std::string_view NAME = "AeroSplineStripBlend";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroSpline, base::IdSpace::PerCardType};
    using Format = AeroSplineStripBlendFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroSplineStripBlendCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroSplineStripBlendCard& card, base::CardKey key);
    [[nodiscard]] AeroSplineStripBlendCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroSplineStripBlendCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroSplineStripBlendCard> m_cards;
};

} // namespace oor::model
