#pragma once

#include "oor/model/cards/aero/aeroSurfaceSpline1/AeroSurfaceSpline1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct AeroSurfaceSpline1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroSurfaceSpline1Card& card);
    [[nodiscard]] static std::optional<AeroSurfaceSpline1Card> read(FieldReader& reader);
    static void write(const AeroSurfaceSpline1Card& card, FieldWriter& writer);
};

// All the SPLINE1 cards, in the category AeroSpline. The EID is in the number space of the splines, which the cards
// SPLINE1 to SPLINE7 and SPLINEX share; each type refuses a repeat of its own (IdSpace::PerCardType) and a check across
// the types reports the others. Nothing names a SPLINE, so no card points here. The CAERO is an aerodynamic element
// (ReferenceTarget::AeroElement), BOX1 and BOX2 aerodynamic boxes (ReferenceTarget::AeroBox) and SETG a SET1 or SET3
// (ReferenceTarget::Set). The method and the usage are kept as the numbers of their enumerators.
class AeroSurfaceSpline1Table final : public TypedTable<AeroSurfaceSpline1Table, AeroSurfaceSpline1Card>
{
public:
    static constexpr std::string_view NAME = "AeroSurfaceSpline1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroSpline, base::IdSpace::PerCardType};
    using Format = AeroSurfaceSpline1Format;

    [[nodiscard]] static base::EntityId cardId(const AeroSurfaceSpline1Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroSurfaceSpline1Card& card, base::CardKey key);
    [[nodiscard]] AeroSurfaceSpline1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroSurfaceSpline1Card& card);
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
    [[nodiscard]] Column<std::uint8_t>& methodColumn() noexcept { return m_method; }
    [[nodiscard]] Column<std::uint8_t>& usageColumn() noexcept { return m_usage; }
    [[nodiscard]] Column<std::int32_t>& elemCountXColumn() noexcept { return m_elemCountX; }
    [[nodiscard]] Column<std::int32_t>& elemCountYColumn() noexcept { return m_elemCountY; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_aeroPanelId;
    Column<base::EntityId> m_firstBox;
    Column<base::EntityId> m_lastBox;
    Column<base::EntityId> m_nodeSetId;
    Column<double> m_linearFlexibility;
    Column<std::uint8_t> m_method{static_cast<std::uint8_t>(AeroSurfaceSpline1Method::InfinitePlate)};
    Column<std::uint8_t> m_usage{static_cast<std::uint8_t>(SplineUsage::Both)};
    Column<std::int32_t> m_elemCountX{AeroSurfaceSpline1Card::DEFAULT_ELEMENTS};
    Column<std::int32_t> m_elemCountY{AeroSurfaceSpline1Card::DEFAULT_ELEMENTS};
};

} // namespace oor::model
