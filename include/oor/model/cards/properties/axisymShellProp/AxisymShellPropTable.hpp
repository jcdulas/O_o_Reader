#pragma once

#include "oor/model/cards/properties/axisymShellProp/AxisymShellPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymShellPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymShellPropCard& card);
    [[nodiscard]] static std::optional<AxisymShellPropCard> read(FieldReader& reader);
    static void write(const AxisymShellPropCard& card, FieldWriter& writer);
};

// All the PAXISYM properties, kept in columns. The ID is in the space every property type shares (IdSpace::Property),
// so the PID of a CAXISYM finds it; the MID is a material (a MAT4 or a MAT5 for heat transfer:
// ReferenceTarget::Material falls back to the thermal materials).
class AxisymShellPropTable final : public TypedTable<AxisymShellPropTable, AxisymShellPropCard>
{
public:
    static constexpr std::string_view NAME = "AxisymShellProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = AxisymShellPropFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymShellPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymShellPropCard& card, base::CardKey key);
    [[nodiscard]] AxisymShellPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymShellPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<double>& thickness1Column() noexcept { return m_thickness1; }
    [[nodiscard]] Column<double>& thickness2Column() noexcept { return m_thickness2; }
    [[nodiscard]] Column<std::uint8_t>& analysisTypeColumn() noexcept { return m_analysisType; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<double> m_thickness1;
    Column<double> m_thickness2;
    Column<std::uint8_t> m_analysisType{static_cast<std::uint8_t>(AxisymShellPropAnalysis::StructuralHeat)};
};

} // namespace oor::model
