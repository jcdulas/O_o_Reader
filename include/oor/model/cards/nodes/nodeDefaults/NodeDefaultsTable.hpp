#pragma once

#include "oor/model/cards/nodes/nodeDefaults/NodeDefaultsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NodeDefaultsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NodeDefaultsCard& card);
    [[nodiscard]] static std::optional<NodeDefaultsCard> read(FieldReader& reader);
    static void write(const NodeDefaultsCard& card, FieldWriter& writer);
};

// The GRDSET cards of a model (at most one is allowed). The card has no ID.
class NodeDefaultsTable final : public TypedTable<NodeDefaultsTable, NodeDefaultsCard>
{
public:
    static constexpr std::string_view NAME = "NodeDefaults";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = NodeDefaultsFormat;

    [[nodiscard]] static base::EntityId cardId(const NodeDefaultsCard&) noexcept { return 0; }
    std::uint32_t append(const NodeDefaultsCard& card, base::CardKey key);
    [[nodiscard]] NodeDefaultsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NodeDefaultsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // Reports a second GRDSET: the solver allows one.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<base::EntityId>& positionFrameIdColumn() noexcept { return m_positionFrameId; }
    [[nodiscard]] Column<base::EntityId>& displacementFrameIdColumn() noexcept { return m_displacementFrameId; }
    [[nodiscard]] Column<std::int32_t>& permanentConstraintsColumn() noexcept { return m_permanentConstraints; }
    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }

private:
    Column<base::EntityId> m_positionFrameId;
    Column<base::EntityId> m_displacementFrameId;
    Column<std::int32_t> m_permanentConstraints;
    Column<base::EntityId> m_superelementId;
};

} // namespace oor::model
