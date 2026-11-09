#pragma once

#include "oor/model/cards/aero/aeroBeamSpline5/AeroBeamSpline5Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct AeroBeamSpline5Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroBeamSpline5Card& card);
    [[nodiscard]] static std::optional<AeroBeamSpline5Card> read(FieldReader& reader);
    static void write(const AeroBeamSpline5Card& card, FieldWriter& writer);
};

// All the SPLINE5 cards, in the category AeroSpline (the number space of the splines is shared by SPLINE1 to SPLINE7
// and SPLINEX; IdSpace::PerCardType refuses a repeat inside this type and a check across the types reports the others).
// The CAERO is an aerodynamic element (ReferenceTarget::AeroElement), the AELIST an AELIST card
// (ReferenceTarget::AeroList), SETG a SET1 or SET3 (ReferenceTarget::Set) and CID a coordinate system. A blank RCORE is
// a NaN in its column.
class AeroBeamSpline5Table final : public TypedTable<AeroBeamSpline5Table, AeroBeamSpline5Card>
{
public:
    static constexpr std::string_view NAME = "AeroBeamSpline5";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroSpline, base::IdSpace::PerCardType};
    using Format = AeroBeamSpline5Format;
    static constexpr double BLANK = std::numeric_limits<double>::quiet_NaN();

    [[nodiscard]] static base::EntityId cardId(const AeroBeamSpline5Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroBeamSpline5Card& card, base::CardKey key);
    [[nodiscard]] AeroBeamSpline5Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroBeamSpline5Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& aeroPanelIdColumn() noexcept { return m_aeroPanelId; }
    [[nodiscard]] Column<base::EntityId>& aeroBoxListIdColumn() noexcept { return m_aeroBoxListId; }
    [[nodiscard]] Column<base::EntityId>& nodeSetIdColumn() noexcept { return m_nodeSetId; }
    [[nodiscard]] Column<double>& linearFlexibilityColumn() noexcept { return m_linearFlexibility; }
    [[nodiscard]] Column<double>& bendingTorsionRatioColumn() noexcept { return m_bendingTorsionRatio; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<double>& rotationFlexibilityXColumn() noexcept { return m_rotationFlexibilityX; }
    [[nodiscard]] Column<double>& rotationFlexibilityYColumn() noexcept { return m_rotationFlexibilityY; }
    [[nodiscard]] Column<std::uint8_t>& usageColumn() noexcept { return m_usage; }
    [[nodiscard]] Column<std::uint8_t>& methodColumn() noexcept { return m_method; }
    [[nodiscard]] Column<std::uint8_t>& functionKindColumn() noexcept { return m_functionKind; }
    [[nodiscard]] Column<double>& coreRadiusColumn() noexcept { return m_coreRadius; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_aeroPanelId;
    Column<base::EntityId> m_aeroBoxListId;
    Column<base::EntityId> m_nodeSetId;
    Column<double> m_linearFlexibility;
    Column<double> m_bendingTorsionRatio{1.0};
    Column<base::EntityId> m_frameId;
    Column<double> m_rotationFlexibilityX;
    Column<double> m_rotationFlexibilityY;
    Column<std::uint8_t> m_usage{static_cast<std::uint8_t>(SplineUsage::Both)};
    Column<std::uint8_t> m_method{static_cast<std::uint8_t>(AeroBeamSpline5Method::Beam)};
    Column<std::uint8_t> m_functionKind{static_cast<std::uint8_t>(SplineRadialFunction::Wendland2)};
    Column<double> m_coreRadius{BLANK};
};

} // namespace oor::model
