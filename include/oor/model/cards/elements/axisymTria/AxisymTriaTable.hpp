#pragma once

#include "oor/model/cards/elements/axisymTria/AxisymTriaCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymTriaFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymTriaCard& card);
    [[nodiscard]] static std::optional<AxisymTriaCard> read(FieldReader& reader);
    static void write(const AxisymTriaCard& card, FieldWriter& writer);
};

// All the CTRIAX elements: the nodes are one column of six IDs per element, 0 for an absent edge point (an (N, 6)
// NumPy view without copy).
class AxisymTriaTable final : public TypedTable<AxisymTriaTable, AxisymTriaCard>
{
public:
    static constexpr std::string_view NAME = "AxisymTria";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = AxisymTriaFormat;

    AxisymTriaTable() : m_matFrameId(AxisymTriaCard::NO_MAT_FRAME_ID) {}

    [[nodiscard]] static base::EntityId cardId(const AxisymTriaCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymTriaCard& card, base::CardKey key);
    [[nodiscard]] AxisymTriaCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymTriaCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, AxisymTriaCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }
    [[nodiscard]] Column<double>& thetaInDegreesColumn() noexcept { return m_thetaInDegrees; }
    [[nodiscard]] Column<base::EntityId>& matFrameIdColumn() noexcept { return m_matFrameId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, AxisymTriaCard::NODE_COUNT>> m_nodes;
    Column<double> m_thetaInDegrees;
    Column<base::EntityId> m_matFrameId;
};

} // namespace oor::model
