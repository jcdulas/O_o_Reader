#pragma once

#include "oor/model/cards/aero/aeroSplineCurveBlend/AeroSplineCurveBlendCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroSplineCurveBlendFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroSplineCurveBlendCard& card);
    [[nodiscard]] static std::optional<AeroSplineCurveBlendCard> read(FieldReader& reader);
    static void write(const AeroSplineCurveBlendCard& card, FieldWriter& writer);
};

// All the SPBLND2 cards, in the category AeroSpline (the number space of the splines is shared by SPLINE1 to SPLINE7,
// SPLINEX, SPBLND1, SPBLND2 and SPLINRB; IdSpace::PerCardType refuses a repeat inside this type and a check across the
// types reports the others). SID1 and SID2 name splines of any of those types (ReferenceTarget::AeroSpline), AELIST an
// AELIST card (ReferenceTarget::AeroList). The cards are kept whole (CardRows) next to the column of their SID.
class AeroSplineCurveBlendTable final : public TypedTable<AeroSplineCurveBlendTable, AeroSplineCurveBlendCard>
{
public:
    static constexpr std::string_view NAME = "AeroSplineCurveBlend";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroSpline, base::IdSpace::PerCardType};
    using Format = AeroSplineCurveBlendFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroSplineCurveBlendCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroSplineCurveBlendCard& card, base::CardKey key);
    [[nodiscard]] AeroSplineCurveBlendCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroSplineCurveBlendCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroSplineCurveBlendCard> m_cards;
};

} // namespace oor::model
