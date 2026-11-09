#pragma once

#include "oor/model/cards/nodes/associativeNode/AssociativeNodeCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct AssociativeNodeFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AssociativeNodeCard& card);
    [[nodiscard]] static std::optional<AssociativeNodeCard> read(FieldReader& reader);
    static void write(const AssociativeNodeCard& card, FieldWriter& writer);
};

// All the GRIDA cards of a model: points of the node ID space (IdSpace::Node, category Point), kept in columns. The
// point of the Part is another run's, so a GRIDA names no card of the model.
class AssociativeNodeTable final : public TypedTable<AssociativeNodeTable, AssociativeNodeCard>
{
public:
    static constexpr std::string_view NAME = "AssociativeNode";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Point, base::IdSpace::Node};
    using Format = AssociativeNodeFormat;

    [[nodiscard]] static base::EntityId cardId(const AssociativeNodeCard& card) noexcept { return card.id; }
    std::uint32_t append(const AssociativeNodeCard& card, base::CardKey key);
    [[nodiscard]] AssociativeNodeCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AssociativeNodeCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the point of the Part is not in this model

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& origNodeIdColumn() noexcept { return m_origNodeId; }
    [[nodiscard]] Column<std::string>& partitionNameColumn() noexcept { return m_partitionName; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_origNodeId;
    Column<std::string> m_partitionName;
};

} // namespace oor::model
