#pragma once

#include "oor/model/cards/aero/aeroFiniteSurfaceSpline6/AeroFiniteSurfaceSpline6Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct AeroFiniteSurfaceSpline6Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroFiniteSurfaceSpline6Card& card);
    [[nodiscard]] static std::optional<AeroFiniteSurfaceSpline6Card> read(FieldReader& reader);
    static void write(const AeroFiniteSurfaceSpline6Card& card, FieldWriter& writer);
};

// All the SPLINE6 cards, in the category AeroSpline (the number space of the splines is shared by SPLINE1 to SPLINE7
// and SPLINEX; IdSpace::PerCardType refuses a repeat inside this type and a check across the types reports the others).
// The CAERO is an aerodynamic element (ReferenceTarget::AeroElement) and may be blank, the AELIST and the VSLIST AELIST
// cards (ReferenceTarget::AeroList) and SETG a SET1 or SET3 (ReferenceTarget::Set). The cards are kept whole (CardRows)
// next to their EID: a SPLINE6 has nineteen fields of its own and few of them are ever set.
class AeroFiniteSurfaceSpline6Table final
    : public TypedTable<AeroFiniteSurfaceSpline6Table, AeroFiniteSurfaceSpline6Card>
{
public:
    static constexpr std::string_view NAME = "AeroFiniteSurfaceSpline6";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroSpline, base::IdSpace::PerCardType};
    using Format = AeroFiniteSurfaceSpline6Format;

    [[nodiscard]] static base::EntityId cardId(const AeroFiniteSurfaceSpline6Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroFiniteSurfaceSpline6Card& card, base::CardKey key);
    [[nodiscard]] AeroFiniteSurfaceSpline6Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroFiniteSurfaceSpline6Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroFiniteSurfaceSpline6Card> m_cards;
};

} // namespace oor::model
