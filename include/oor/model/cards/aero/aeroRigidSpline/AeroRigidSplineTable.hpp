#pragma once

#include "oor/model/cards/aero/aeroRigidSpline/AeroRigidSplineCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroRigidSplineFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroRigidSplineCard& card);
    [[nodiscard]] static std::optional<AeroRigidSplineCard> read(FieldReader& reader);
    static void write(const AeroRigidSplineCard& card, FieldWriter& writer);
};

// All the SPLINRB cards, in the category AeroSpline (the number space of the splines is shared by SPLINE1 to SPLINE7,
// SPLINEX, SPBLND1, SPBLND2 and SPLINRB; IdSpace::PerCardType refuses a repeat inside this type and a check across the
// types reports the others). The CAERO is an aerodynamic element (ReferenceTarget::AeroElement) and may be blank, the
// AELIST an AELIST card (ReferenceTarget::AeroList) and the grid points of the pairs are nodes (ReferenceTarget::Node).
// The cards are kept whole (CardRows) next to the column of their SID.
class AeroRigidSplineTable final : public TypedTable<AeroRigidSplineTable, AeroRigidSplineCard>
{
public:
    static constexpr std::string_view NAME = "AeroRigidSpline";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroSpline, base::IdSpace::PerCardType};
    using Format = AeroRigidSplineFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroRigidSplineCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroRigidSplineCard& card, base::CardKey key);
    [[nodiscard]] AeroRigidSplineCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroRigidSplineCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroRigidSplineCard> m_cards;
};

} // namespace oor::model
