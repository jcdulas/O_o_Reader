#pragma once

#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ShellTria3DrillingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShellTria3DrillingCard& card);
    [[nodiscard]] static std::optional<ShellTria3DrillingCard> read(FieldReader& reader);
    static void write(const ShellTria3DrillingCard& card, FieldWriter& writer);
};

// All the CTRIAR elements: the nodes are one column of three IDs per element (an (N, 3) NumPy view without copy).
class ShellTria3DrillingTable final : public TypedTable<ShellTria3DrillingTable, ShellTria3DrillingCard>
{
public:
    static constexpr std::string_view NAME = "ShellTria3Drilling";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = ShellTria3DrillingFormat;

    ShellTria3DrillingTable()
        : m_matFrameId(ShellTria3DrillingCard::NO_MAT_FRAME_ID),
          m_cornerThickness({ShellTria3DrillingCard::NO_THICKNESS, ShellTria3DrillingCard::NO_THICKNESS,
                             ShellTria3DrillingCard::NO_THICKNESS})
    {
    }

    [[nodiscard]] static base::EntityId cardId(const ShellTria3DrillingCard& card) noexcept { return card.id; }
    std::uint32_t append(const ShellTria3DrillingCard& card, base::CardKey key);
    [[nodiscard]] ShellTria3DrillingCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ShellTria3DrillingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] Column<double>& thetaInDegreesColumn() noexcept { return m_thetaInDegrees; }
    [[nodiscard]] Column<base::EntityId>& matFrameIdColumn() noexcept { return m_matFrameId; }
    [[nodiscard]] Column<double>& offsetColumn() noexcept { return m_offset; }
    [[nodiscard]] Column<std::int32_t>& thicknessFlagColumn() noexcept { return m_thicknessFlag; }
    [[nodiscard]] Column<std::array<double, 3>>& cornerThicknessColumn() noexcept { return m_cornerThickness; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, 3>> m_nodes;
    Column<double> m_thetaInDegrees;
    Column<base::EntityId> m_matFrameId;
    Column<double> m_offset;
    Column<std::int32_t> m_thicknessFlag;
    Column<std::array<double, 3>> m_cornerThickness;
};

} // namespace oor::model
