#pragma once

#include "oor/model/cards/aero/aeroBeamSpline2/AeroBeamSpline2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct AeroBeamSpline2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroBeamSpline2Card& card);
    [[nodiscard]] static std::optional<AeroBeamSpline2Card> read(FieldReader& reader);
    static void write(const AeroBeamSpline2Card& card, FieldWriter& writer);
};

// All the SPLINE2 cards, in the category AeroSpline (the number space of the splines is shared by SPLINE1 to SPLINE7
// and SPLINEX; IdSpace::PerCardType refuses a repeat inside this type and a check across the types reports the others).
// The CAERO is an aerodynamic element (ReferenceTarget::AeroElement), ID1 and ID2 aerodynamic boxes
// (ReferenceTarget::AeroBox), SETG a SET1 or SET3 (ReferenceTarget::Set) and CID a coordinate system.
class AeroBeamSpline2Table final : public TypedTable<AeroBeamSpline2Table, AeroBeamSpline2Card>
{
public:
    static constexpr std::string_view NAME = "AeroBeamSpline2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroSpline, base::IdSpace::PerCardType};
    using Format = AeroBeamSpline2Format;

    [[nodiscard]] static base::EntityId cardId(const AeroBeamSpline2Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroBeamSpline2Card& card, base::CardKey key);
    [[nodiscard]] AeroBeamSpline2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroBeamSpline2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& aeroPanelIdColumn() noexcept { return m_aeroPanelId; }
    [[nodiscard]] Column<base::EntityId>& firstBoxColumn() noexcept { return m_firstBox; }
    [[nodiscard]] Column<base::EntityId>& lastBoxColumn() noexcept { return m_lastBox; }
    [[nodiscard]] Column<base::EntityId>& nodeSetIdColumn() noexcept { return m_nodeSetId; }
    [[nodiscard]] Column<double>& linearFlexibilityColumn() noexcept { return m_linearFlexibility; }
    [[nodiscard]] Column<double>& bendingTorsionRatioColumn() noexcept { return m_bendingTorsionRatio; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<double>& rotationFlexibilityXColumn() noexcept { return m_rotationFlexibilityX; }
    [[nodiscard]] Column<double>& rotationFlexibilityYColumn() noexcept { return m_rotationFlexibilityY; }
    [[nodiscard]] Column<std::uint8_t>& usageColumn() noexcept { return m_usage; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_aeroPanelId;
    Column<base::EntityId> m_firstBox;
    Column<base::EntityId> m_lastBox;
    Column<base::EntityId> m_nodeSetId;
    Column<double> m_linearFlexibility;
    Column<double> m_bendingTorsionRatio{1.0};
    Column<base::EntityId> m_frameId;
    Column<double> m_rotationFlexibilityX;
    Column<double> m_rotationFlexibilityY;
    Column<std::uint8_t> m_usage{static_cast<std::uint8_t>(SplineUsage::Both)};
};

} // namespace oor::model
