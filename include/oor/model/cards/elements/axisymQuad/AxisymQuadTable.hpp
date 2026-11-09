#pragma once

#include "oor/model/cards/elements/axisymQuad/AxisymQuadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymQuadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymQuadCard& card);
    [[nodiscard]] static std::optional<AxisymQuadCard> read(FieldReader& reader);
    static void write(const AxisymQuadCard& card, FieldWriter& writer);
};

// All the CQUADX elements: the nodes are one column of nine IDs per element, 0 for an absent edge point (an (N, 9)
// NumPy view without copy).
class AxisymQuadTable final : public TypedTable<AxisymQuadTable, AxisymQuadCard>
{
public:
    static constexpr std::string_view NAME = "AxisymQuad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = AxisymQuadFormat;

    AxisymQuadTable() : m_matFrameId(AxisymQuadCard::NO_MAT_FRAME_ID) {}

    [[nodiscard]] static base::EntityId cardId(const AxisymQuadCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymQuadCard& card, base::CardKey key);
    [[nodiscard]] AxisymQuadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymQuadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, AxisymQuadCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }
    [[nodiscard]] Column<double>& thetaInDegreesColumn() noexcept { return m_thetaInDegrees; }
    [[nodiscard]] Column<base::EntityId>& matFrameIdColumn() noexcept { return m_matFrameId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, AxisymQuadCard::NODE_COUNT>> m_nodes;
    Column<double> m_thetaInDegrees;
    Column<base::EntityId> m_matFrameId;
};

} // namespace oor::model
