#pragma once

#include "oor/model/cards/elements/shellQuad8/ShellQuad8Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ShellQuad8Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShellQuad8Card& card);
    [[nodiscard]] static std::optional<ShellQuad8Card> read(FieldReader& reader);
    static void write(const ShellQuad8Card& card, FieldWriter& writer);
};

// All the CQUAD8 elements: the nodes are one column of six IDs per element, 0 for an absent edge point (an (N, 6)
// NumPy view without copy).
class ShellQuad8Table final : public TypedTable<ShellQuad8Table, ShellQuad8Card>
{
public:
    static constexpr std::string_view NAME = "ShellQuad8";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = ShellQuad8Format;

    ShellQuad8Table()
        : m_matFrameId(ShellQuad8Card::NO_MAT_FRAME_ID),
          m_cornerThickness({ShellQuad8Card::NO_THICKNESS, ShellQuad8Card::NO_THICKNESS, ShellQuad8Card::NO_THICKNESS,
                             ShellQuad8Card::NO_THICKNESS})
    {
    }

    [[nodiscard]] static base::EntityId cardId(const ShellQuad8Card& card) noexcept { return card.id; }
    std::uint32_t append(const ShellQuad8Card& card, base::CardKey key);
    [[nodiscard]] ShellQuad8Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const ShellQuad8Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, ShellQuad8Card::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }
    [[nodiscard]] Column<double>& thetaInDegreesColumn() noexcept { return m_thetaInDegrees; }
    [[nodiscard]] Column<base::EntityId>& matFrameIdColumn() noexcept { return m_matFrameId; }
    [[nodiscard]] Column<double>& offsetColumn() noexcept { return m_offset; }
    [[nodiscard]] Column<std::int32_t>& thicknessFlagColumn() noexcept { return m_thicknessFlag; }
    [[nodiscard]] Column<std::array<double, ShellQuad8Card::CORNER_COUNT>>& cornerThicknessColumn() noexcept
    {
        return m_cornerThickness;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, ShellQuad8Card::NODE_COUNT>> m_nodes;
    Column<double> m_thetaInDegrees;
    Column<base::EntityId> m_matFrameId;
    Column<double> m_offset;
    Column<std::int32_t> m_thicknessFlag;
    Column<std::array<double, ShellQuad8Card::CORNER_COUNT>> m_cornerThickness;
};

} // namespace oor::model
