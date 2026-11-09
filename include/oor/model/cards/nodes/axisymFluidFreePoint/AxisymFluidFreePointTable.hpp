#pragma once

#include "oor/model/cards/nodes/axisymFluidFreePoint/AxisymFluidFreePointCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymFluidFreePointFormat
{
    // The first thing wrong with the values of a free surface point, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymFluidFreePointCard& card);
    // One FREEPT card gives one point per pair IDPi PHIi it lists, all on the same fluid point.
    [[nodiscard]] static std::vector<AxisymFluidFreePointCard> readAll(FieldReader& reader);
    static void write(const AxisymFluidFreePointCard& card, FieldWriter& writer);
};

// All the free surface points defined by FREEPT cards: one row per point, in the node ID space (IdSpace::Node, category
// Point), kept in columns. The IDF names the RINGFL fluid point the point is on (a node reference).
class AxisymFluidFreePointTable final : public TypedTable<AxisymFluidFreePointTable, AxisymFluidFreePointCard>
{
public:
    static constexpr std::string_view NAME = "AxisymFluidFreePoint";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Point, base::IdSpace::Node};
    using Format = AxisymFluidFreePointFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymFluidFreePointCard& card) noexcept { return card.pointId; }
    std::uint32_t append(const AxisymFluidFreePointCard& card, base::CardKey key);
    [[nodiscard]] AxisymFluidFreePointCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymFluidFreePointCard& card);
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
