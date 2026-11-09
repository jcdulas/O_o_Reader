#pragma once

#include "oor/model/cards/aero/aeroUserSpline/AeroUserSplineCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct AeroUserSplineFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroUserSplineCard& card);
    [[nodiscard]] static std::optional<AeroUserSplineCard> read(FieldReader& reader);
    static void write(const AeroUserSplineCard& card, FieldWriter& writer);
};

// All the SPLINEX cards, in the category AeroSpline (the number space of the splines is shared by SPLINE1 to SPLINE7
// and SPLINEX; IdSpace::PerCardType refuses a repeat inside this type and a check across the types reports the others).
// The AELIST is an AELIST card (ReferenceTarget::AeroList) and the AEFACT an AEFACT card (ReferenceTarget::AeroFactor);
// the AELISTC is a plain ID and the four component fields are names, so none of the three is visited. The cards are
// kept whole (CardRows) next to their EID, which is the column the query engine reads.
class AeroUserSplineTable final : public TypedTable<AeroUserSplineTable, AeroUserSplineCard>
{
public:
    static constexpr std::string_view NAME = "AeroUserSpline";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroSpline, base::IdSpace::PerCardType};
    using Format = AeroUserSplineFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroUserSplineCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroUserSplineCard& card, base::CardKey key);
    [[nodiscard]] AeroUserSplineCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroUserSplineCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroUserSplineCard> m_cards;
};

} // namespace oor::model
