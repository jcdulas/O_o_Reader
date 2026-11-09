#pragma once

#include "oor/model/cards/aero/aeroSplineEquation3/AeroSplineEquation3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct AeroSplineEquation3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroSplineEquation3Card& card);
    [[nodiscard]] static std::optional<AeroSplineEquation3Card> read(FieldReader& reader);
    static void write(const AeroSplineEquation3Card& card, FieldWriter& writer);
};

// All the SPLINE3 cards, in the category AeroSpline (the number space of the splines is shared by SPLINE1 to SPLINE7
// and SPLINEX; IdSpace::PerCardType refuses a repeat inside this type and a check across the types reports the others).
// The CAERO is an aerodynamic element (ReferenceTarget::AeroElement), the BOXID an aerodynamic box
// (ReferenceTarget::AeroBox) and each G a grid point or scalar point (ReferenceTarget::ScalarOrGridPoint). The cards
// are kept whole (CardRows) next to their EID: a SPLINE3 has a handful of points.
class AeroSplineEquation3Table final : public TypedTable<AeroSplineEquation3Table, AeroSplineEquation3Card>
{
public:
    static constexpr std::string_view NAME = "AeroSplineEquation3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroSpline, base::IdSpace::PerCardType};
    using Format = AeroSplineEquation3Format;

    [[nodiscard]] static base::EntityId cardId(const AeroSplineEquation3Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroSplineEquation3Card& card, base::CardKey key);
    [[nodiscard]] AeroSplineEquation3Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroSplineEquation3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroSplineEquation3Card> m_cards;
};

} // namespace oor::model
