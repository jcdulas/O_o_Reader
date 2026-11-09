#pragma once

#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ShellQuad4DrillingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShellQuad4DrillingCard& card);
    [[nodiscard]] static std::optional<ShellQuad4DrillingCard> read(FieldReader& reader);
    static void write(const ShellQuad4DrillingCard& card, FieldWriter& writer);
};

// All the CQUADR elements: the nodes are one column of four IDs per element (an (N, 4) NumPy view without copy).
class ShellQuad4DrillingTable final : public TypedTable<ShellQuad4DrillingTable, ShellQuad4DrillingCard>
{
public:
    static constexpr std::string_view NAME = "ShellQuad4Drilling";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = ShellQuad4DrillingFormat;

    ShellQuad4DrillingTable()
        : m_matFrameId(ShellQuad4DrillingCard::NO_MAT_FRAME_ID),
          m_cornerThickness({ShellQuad4DrillingCard::NO_THICKNESS, ShellQuad4DrillingCard::NO_THICKNESS,
                             ShellQuad4DrillingCard::NO_THICKNESS, ShellQuad4DrillingCard::NO_THICKNESS})
    {
    }

    [[nodiscard]] static base::EntityId cardId(const ShellQuad4DrillingCard& card) noexcept { return card.id; }
    std::uint32_t append(const ShellQuad4DrillingCard& card, base::CardKey key);
    [[nodiscard]] ShellQuad4DrillingCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ShellQuad4DrillingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] base::EntityId propId(std::uint32_t row) const { return m_propId.get(row); }
    [[nodiscard]] std::array<base::EntityId, 4> nodes(std::uint32_t row) const { return m_nodes.get(row); }
    void setPropId(std::uint32_t row, base::EntityId value) { m_propId.set(row, value); }
    void setNodes(std::uint32_t row, const std::array<base::EntityId, 4>& value) { m_nodes.set(row, value); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, 4>>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] Column<double>& thetaInDegreesColumn() noexcept { return m_thetaInDegrees; }
    [[nodiscard]] Column<base::EntityId>& matFrameIdColumn() noexcept { return m_matFrameId; }
    [[nodiscard]] Column<double>& offsetColumn() noexcept { return m_offset; }
    [[nodiscard]] Column<std::int32_t>& thicknessFlagColumn() noexcept { return m_thicknessFlag; }
    [[nodiscard]] Column<std::array<double, 4>>& cornerThicknessColumn() noexcept { return m_cornerThickness; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, 4>> m_nodes;
    Column<double> m_thetaInDegrees;
    Column<base::EntityId> m_matFrameId;
    Column<double> m_offset;
    Column<std::int32_t> m_thicknessFlag;
    Column<std::array<double, 4>> m_cornerThickness;
};

} // namespace oor::model
