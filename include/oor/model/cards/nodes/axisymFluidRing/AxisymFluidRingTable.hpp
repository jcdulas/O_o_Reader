#pragma once

#include "oor/model/cards/nodes/axisymFluidRing/AxisymFluidRingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymFluidRingFormat
{
    // The first thing wrong with the values of a fluid point, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymFluidRingCard& card);
    // One RINGFL card gives its one or two fluid points.
    [[nodiscard]] static std::vector<AxisymFluidRingCard> readAll(FieldReader& reader);
    static void write(const AxisymFluidRingCard& card, FieldWriter& writer);
};

// All the fluid points defined by RINGFL cards: one row per point, in the node ID space (IdSpace::Node, category
// Point), kept in columns. A fluid point names no other card; the FREEPT, PRESPT, GRIDB and GRIDS cards name it.
class AxisymFluidRingTable final : public TypedTable<AxisymFluidRingTable, AxisymFluidRingCard>
{
public:
    static constexpr std::string_view NAME = "AxisymFluidRing";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Point, base::IdSpace::Node};
    using Format = AxisymFluidRingFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymFluidRingCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymFluidRingCard& card, base::CardKey key);
    [[nodiscard]] AxisymFluidRingCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymFluidRingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a RINGFL refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& position1Column() noexcept { return m_position1; }
    [[nodiscard]] Column<double>& position2Column() noexcept { return m_position2; }
    [[nodiscard]] Column<double>& position3Column() noexcept { return m_position3; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_position1;
    Column<double> m_position2;
    Column<double> m_position3;
};

} // namespace oor::model
