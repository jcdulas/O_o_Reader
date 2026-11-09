#pragma once

#include "oor/model/cards/nodes/axisymConeRing/AxisymConeRingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymConeRingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymConeRingCard& card);
    [[nodiscard]] static std::optional<AxisymConeRingCard> read(FieldReader& reader);
    static void write(const AxisymConeRingCard& card, FieldWriter& writer);
};

// All the RINGAX cards of a model: points of the node ID space (IdSpace::Node, so a GRID, a POINTAX or a RINGAX cannot
// share a number), kept in columns. A RINGAX names no other card.
class AxisymConeRingTable final : public TypedTable<AxisymConeRingTable, AxisymConeRingCard>
{
public:
    static constexpr std::string_view NAME = "AxisymConeRing";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Point, base::IdSpace::Node};
    using Format = AxisymConeRingFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymConeRingCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymConeRingCard& card, base::CardKey key);
    [[nodiscard]] AxisymConeRingCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymConeRingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a RINGAX refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& radiusColumn() noexcept { return m_radius; }
    [[nodiscard]] Column<double>& zCoordColumn() noexcept { return m_zCoord; }
    [[nodiscard]] Column<std::int32_t>& permanentConstraintsColumn() noexcept { return m_permanentConstraints; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_radius;
    Column<double> m_zCoord;
    Column<std::int32_t> m_permanentConstraints;
};

} // namespace oor::model
