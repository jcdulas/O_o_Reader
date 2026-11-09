#pragma once

#include "oor/model/cards/nodes/node/NodeCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/RowChangeLog.hpp"
#include "oor/model/storage/TableAttachment.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

// How a GRID card is read from and written to its fields.
struct NodeFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NodeCard& card);
    [[nodiscard]] static std::optional<NodeCard> read(FieldReader& reader);
    static void write(const NodeCard& card, FieldWriter& writer);
};

// All the GRID cards of a model, by column. The positions are one column of (x, y, z) triples, so that a NumPy view of
// it is an (N, 3) array without copy.
class NodeTable final : public TypedTable<NodeTable, NodeCard>
{
public:
    static constexpr std::string_view NAME = "Node";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Node, base::IdSpace::Node};
    using Format = NodeFormat;

    NodeTable() : m_positionFrameId(0), m_displacementFrameId(0), m_permanentConstraints(0), m_superelementId(0) {}

    [[nodiscard]] static base::EntityId cardId(const NodeCard& card) noexcept { return card.id; }
    std::uint32_t append(const NodeCard& card, base::CardKey key);
    [[nodiscard]] NodeCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NodeCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id)
    {
        m_id.set(row, id);
        m_changes.noteIdChange();
    }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // Applies the GRDSET defaults to the CP, CD, PS and SEID that were blank (nothing when there is no GRDSET).
    void finishRead(Model& model, const ReadContext& context) override;
    // True when the model has a GRDSET: the grids then write CP, CD, PS and SEID explicitly (a zero overrides it).
    [[nodiscard]] bool fieldsGovernedByDefaults(const Model& model) const override;

    // Field access by row.
    [[nodiscard]] base::EntityId positionFrameId(std::uint32_t row) const { return m_positionFrameId.get(row); }
    [[nodiscard]] base::EntityId displacementFrameId(std::uint32_t row) const { return m_displacementFrameId.get(row); }
    [[nodiscard]] base::Vec3 xyz(std::uint32_t row) const;
    [[nodiscard]] std::int32_t permanentConstraints(std::uint32_t row) const { return m_permanentConstraints.get(row); }
    [[nodiscard]] base::EntityId superelementId(std::uint32_t row) const { return m_superelementId.get(row); }
    void setPositionFrameId(std::uint32_t row, base::EntityId value)
    {
        m_positionFrameId.set(row, value);
        m_changes.noteRow(row);
    }
    void setDisplacementFrameId(std::uint32_t row, base::EntityId value)
    {
        m_displacementFrameId.set(row, value);
        m_changes.noteRow(row);
    }
    void setXyz(std::uint32_t row, const base::Vec3& value)
    {
        m_xyz.set(row, {value.x, value.y, value.z});
        m_changes.noteRow(row);
    }
    void setXyzComponent(std::uint32_t row, std::size_t component, double value);
    void setPermanentConstraints(std::uint32_t row, std::int32_t value) { m_permanentConstraints.set(row, value); }
    void setSuperelementId(std::uint32_t row, base::EntityId value) { m_superelementId.set(row, value); }

    // The columns, for views and queries.
    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& positionFrameIdColumn() noexcept { return m_positionFrameId; }
    [[nodiscard]] Column<base::EntityId>& displacementFrameIdColumn() noexcept { return m_displacementFrameId; }
    [[nodiscard]] Column<std::array<double, 3>>& xyzColumn() noexcept { return m_xyz; }
    [[nodiscard]] Column<std::int32_t>& permanentConstraintsColumn() noexcept { return m_permanentConstraints; }
    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

    // What the geometry layer keeps with the grid points. The log says which rows were edited since the cache of basic
    // positions last looked (it records nothing before that cache exists); the slot holds the cache and is copied with
    // the table. Both are mutable: they are caches, filled when the geometry of a const model is read.
    [[nodiscard]] RowChangeLog& changes() const noexcept { return m_changes; }
    [[nodiscard]] AttachmentSlot& attachment() const noexcept { return m_attachment; }

private:
    mutable RowChangeLog m_changes;
    mutable AttachmentSlot m_attachment;
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_positionFrameId;
    Column<std::array<double, 3>> m_xyz;
    Column<base::EntityId> m_displacementFrameId;
    Column<std::int32_t> m_permanentConstraints;
    Column<base::EntityId> m_superelementId;
    Column<std::uint8_t> m_blankFields; // NodeCard::BLANK_* bits; all zero (nothing stored) once the read is finished
};

} // namespace oor::model
