#pragma once

#include "oor/model/cards/nodes/axisymConePoint/AxisymConePointCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymConePointFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymConePointCard& card);
    [[nodiscard]] static std::optional<AxisymConePointCard> read(FieldReader& reader);
    static void write(const AxisymConePointCard& card, FieldWriter& writer);
};

// All the POINTAX cards of a model: points of the node ID space (IdSpace::Node, category Point), kept in columns. The
// RID names the RINGAX the point is on (a node reference).
class AxisymConePointTable final : public TypedTable<AxisymConePointTable, AxisymConePointCard>
{
public:
    static constexpr std::string_view NAME = "AxisymConePoint";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Point, base::IdSpace::Node};
    using Format = AxisymConePointFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymConePointCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymConePointCard& card, base::CardKey key);
    [[nodiscard]] AxisymConePointCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymConePointCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& ringIdColumn() noexcept { return m_ringId; }
    [[nodiscard]] Column<double>& phiAngleColumn() noexcept { return m_phiAngle; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_ringId;
    Column<double> m_phiAngle;
};

} // namespace oor::model
