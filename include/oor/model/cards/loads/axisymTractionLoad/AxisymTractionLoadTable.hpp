#pragma once

#include "oor/model/cards/loads/axisymTractionLoad/AxisymTractionLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymTractionLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymTractionLoadCard& card);
    [[nodiscard]] static std::optional<AxisymTractionLoadCard> read(FieldReader& reader);
    static void write(const AxisymTractionLoadCard& card, FieldWriter& writer);
};

// All the PLOADX1 cards. The ID is a load set ID shared by many cards (IdSpace::Group).
class AxisymTractionLoadTable final : public TypedTable<AxisymTractionLoadTable, AxisymTractionLoadCard>
{
public:
    static constexpr std::string_view NAME = "AxisymTractionLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = AxisymTractionLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymTractionLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymTractionLoadCard& card, base::CardKey key);
    [[nodiscard]] AxisymTractionLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymTractionLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& elemIdColumn() noexcept { return m_elemId; }
    [[nodiscard]] Column<double>& tractionAColumn() noexcept { return m_tractionA; }
    [[nodiscard]] Column<double>& tractionBColumn() noexcept { return m_tractionB; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<double>& tractionAngleColumn() noexcept { return m_tractionAngle; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_elemId;
    Column<double> m_tractionA;
    Column<double> m_tractionB;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<double> m_tractionAngle;
};

} // namespace oor::model
