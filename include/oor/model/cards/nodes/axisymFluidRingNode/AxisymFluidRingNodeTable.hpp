#pragma once

#include "oor/model/cards/nodes/axisymFluidRingNode/AxisymFluidRingNodeCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymFluidRingNodeFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymFluidRingNodeCard& card);
    [[nodiscard]] static std::optional<AxisymFluidRingNodeCard> read(FieldReader& reader);
    static void write(const AxisymFluidRingNodeCard& card, FieldWriter& writer);
};

// All the GRIDB cards of a model.
class AxisymFluidRingNodeTable final : public TypedTable<AxisymFluidRingNodeTable, AxisymFluidRingNodeCard>
{
public:
    static constexpr std::string_view NAME = "AxisymFluidRingNode";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Point, base::IdSpace::Node};
    using Format = AxisymFluidRingNodeFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymFluidRingNodeCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymFluidRingNodeCard& card, base::CardKey key);
    [[nodiscard]] AxisymFluidRingNodeCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymFluidRingNodeCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& phiAngleColumn() noexcept { return m_phiAngle; }
    [[nodiscard]] Column<base::EntityId>& displacementFrameIdColumn() noexcept { return m_displacementFrameId; }
    [[nodiscard]] Column<std::int32_t>& permanentConstraintsColumn() noexcept { return m_permanentConstraints; }
    [[nodiscard]] Column<base::EntityId>& fluidPointIdColumn() noexcept { return m_fluidPointId; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_phiAngle;
    Column<base::EntityId> m_displacementFrameId;
    Column<std::int32_t> m_permanentConstraints;
    Column<base::EntityId> m_fluidPointId;
};

} // namespace oor::model
