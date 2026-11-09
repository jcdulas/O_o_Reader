#pragma once

#include "oor/model/cards/nodes/nodeSequence/NodeSequenceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct NodeSequenceFormat
{
    // The most points a card lists.
    static constexpr std::size_t MAX_POINTS = 4;

    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NodeSequenceCard& card);
    [[nodiscard]] static std::optional<NodeSequenceCard> read(FieldReader& reader);
    static void write(const NodeSequenceCard& card, FieldWriter& writer);
};

// All the SEQGP cards. A card has no ID (IdSpace::None, category Other: it is not a node). The points and the sequence
// numbers of every card are two lists in CSR form. The points are ReferenceTarget::Node references: renumbering the
// nodes renumbers them (the sequence numbers are not IDs of anything and stay as they are).
class NodeSequenceTable final : public TypedTable<NodeSequenceTable, NodeSequenceCard>
{
public:
    static constexpr std::string_view NAME = "NodeSequence";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = NodeSequenceFormat;

    [[nodiscard]] static base::EntityId cardId(const NodeSequenceCard&) noexcept { return 0; }
    std::uint32_t append(const NodeSequenceCard& card, base::CardKey key);
    [[nodiscard]] NodeSequenceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NodeSequenceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<double>& sequenceColumn() noexcept { return m_sequence; }

private:
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<double> m_sequence;
};

} // namespace oor::model
