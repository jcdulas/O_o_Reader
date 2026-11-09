#pragma once

#include "oor/model/cards/elements/shellTria6/ShellTria6Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ShellTria6Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShellTria6Card& card);
    [[nodiscard]] static std::optional<ShellTria6Card> read(FieldReader& reader);
    static void write(const ShellTria6Card& card, FieldWriter& writer);
};

// All the CTRIA6 elements: the nodes are one column of six IDs per element, 0 for an absent edge point (an (N, 6)
// NumPy view without copy).
class ShellTria6Table final : public TypedTable<ShellTria6Table, ShellTria6Card>
{
public:
    static constexpr std::string_view NAME = "ShellTria6";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = ShellTria6Format;

    ShellTria6Table()
        : m_matFrameId(ShellTria6Card::NO_MAT_FRAME_ID),
          m_cornerThickness({ShellTria6Card::NO_THICKNESS, ShellTria6Card::NO_THICKNESS, ShellTria6Card::NO_THICKNESS})
    {
    }

    [[nodiscard]] static base::EntityId cardId(const ShellTria6Card& card) noexcept { return card.id; }
    std::uint32_t append(const ShellTria6Card& card, base::CardKey key);
    [[nodiscard]] ShellTria6Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const ShellTria6Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, ShellTria6Card::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }
    [[nodiscard]] Column<double>& thetaInDegreesColumn() noexcept { return m_thetaInDegrees; }
    [[nodiscard]] Column<base::EntityId>& matFrameIdColumn() noexcept { return m_matFrameId; }
    [[nodiscard]] Column<double>& offsetColumn() noexcept { return m_offset; }
    [[nodiscard]] Column<std::int32_t>& thicknessFlagColumn() noexcept { return m_thicknessFlag; }
    [[nodiscard]] Column<std::array<double, ShellTria6Card::CORNER_COUNT>>& cornerThicknessColumn() noexcept
    {
        return m_cornerThickness;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, ShellTria6Card::NODE_COUNT>> m_nodes;
    Column<double> m_thetaInDegrees;
    Column<base::EntityId> m_matFrameId;
    Column<double> m_offset;
    Column<std::int32_t> m_thicknessFlag;
    Column<std::array<double, ShellTria6Card::CORNER_COUNT>> m_cornerThickness;
};

} // namespace oor::model
