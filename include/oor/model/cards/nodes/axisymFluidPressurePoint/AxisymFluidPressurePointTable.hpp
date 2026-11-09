#pragma once

#include "oor/model/cards/nodes/axisymFluidPressurePoint/AxisymFluidPressurePointCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymFluidPressurePointFormat
{
    // The first thing wrong with the values of a pressure point, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymFluidPressurePointCard& card);
    // One PRESPT card gives one point per pair IDPi PHIi it lists, all on the same fluid point.
    [[nodiscard]] static std::vector<AxisymFluidPressurePointCard> readAll(FieldReader& reader);
    static void write(const AxisymFluidPressurePointCard& card, FieldWriter& writer);
};

// All the pressure points defined by PRESPT cards: one row per point, in the node ID space (IdSpace::Node, category
// Point), kept in columns. The IDF names the RINGFL fluid point the point is on (a node reference).
class AxisymFluidPressurePointTable final
    : public TypedTable<AxisymFluidPressurePointTable, AxisymFluidPressurePointCard>
{
public:
    static constexpr std::string_view NAME = "AxisymFluidPressurePoint";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Point, base::IdSpace::Node};
    using Format = AxisymFluidPressurePointFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymFluidPressurePointCard& card) noexcept
    {
        return card.pointId;
    }
    std::uint32_t append(const AxisymFluidPressurePointCard& card, base::CardKey key);
    [[nodiscard]] AxisymFluidPressurePointCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymFluidPressurePointCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_pointId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_pointId.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& fluidPointIdColumn() noexcept { return m_fluidPointId; }
    [[nodiscard]] Column<base::EntityId>& pointIdColumn() noexcept { return m_pointId; }
    [[nodiscard]] Column<double>& phiAngleColumn() noexcept { return m_phiAngle; }

private:
    Column<base::EntityId> m_fluidPointId;
    Column<base::EntityId> m_pointId;
    Column<double> m_phiAngle;
};

} // namespace oor::model
