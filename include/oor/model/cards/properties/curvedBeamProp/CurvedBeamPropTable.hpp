#pragma once

#include "oor/model/cards/properties/curvedBeamProp/CurvedBeamPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct CurvedBeamPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CurvedBeamPropCard& card);
    [[nodiscard]] static std::optional<CurvedBeamPropCard> read(FieldReader& reader);
    static void write(const CurvedBeamPropCard& card, FieldWriter& writer);
};

// All the PBEND cards, both forms in one table: the section values, the pipe values and the common ones are columns of
// their own (the values of the form that a card does not have are zero, or NaN for a blank field).
class CurvedBeamPropTable final : public TypedTable<CurvedBeamPropTable, CurvedBeamPropCard>
{
public:
    static constexpr std::string_view NAME = "CurvedBeamProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = CurvedBeamPropFormat;

    CurvedBeamPropTable()
        : m_neutralAxisOffset(CurvedBeamPropCard::NOT_GIVEN),
          m_internalPressure(CurvedBeamPropCard::NOT_GIVEN),
          m_bendRadius(CurvedBeamPropCard::NOT_GIVEN),
          m_arcAngle(CurvedBeamPropCard::NOT_GIVEN)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const CurvedBeamPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const CurvedBeamPropCard& card, base::CardKey key);
    [[nodiscard]] CurvedBeamPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CurvedBeamPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<std::uint8_t>& formColumn() noexcept { return m_form; }
    [[nodiscard]] Column<std::array<double, 4>>& sectionColumn() noexcept { return m_section; }
    [[nodiscard]] Column<std::array<double, 8>>& stressPointsColumn() noexcept { return m_stressPoints; }
    [[nodiscard]] Column<std::array<double, 2>>& shearFactorColumn() noexcept { return m_shearFactor; }
    [[nodiscard]] Column<double>& neutralAxisOffsetColumn() noexcept { return m_neutralAxisOffset; }
    [[nodiscard]] Column<std::int32_t>& flexibilityOptionColumn() noexcept { return m_flexibilityOption; }
    [[nodiscard]] Column<double>& meanRadiusColumn() noexcept { return m_meanRadius; }
    [[nodiscard]] Column<double>& thicknessColumn() noexcept { return m_thickness; }
    [[nodiscard]] Column<double>& internalPressureColumn() noexcept { return m_internalPressure; }
    [[nodiscard]] Column<double>& bendRadiusColumn() noexcept { return m_bendRadius; }
    [[nodiscard]] Column<double>& arcAngleColumn() noexcept { return m_arcAngle; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }
    [[nodiscard]] Column<double>& centroidRadialOffsetColumn() noexcept { return m_centroidRadialOffset; }
    [[nodiscard]] Column<double>& centerZColumn() noexcept { return m_centerZ; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<std::uint8_t> m_form;
    Column<std::array<double, 4>> m_section; // A, I1, I2, J
    Column<std::array<double, 8>> m_stressPoints;
    Column<std::array<double, 2>> m_shearFactor; // K1, K2
    Column<double> m_neutralAxisOffset;
    Column<std::int32_t> m_flexibilityOption;
    Column<double> m_meanRadius;
    Column<double> m_thickness;
    Column<double> m_internalPressure;
    Column<double> m_bendRadius;
    Column<double> m_arcAngle;
    Column<double> m_nonStructMass;
    Column<double> m_centroidRadialOffset;
    Column<double> m_centerZ;
};

} // namespace oor::model
