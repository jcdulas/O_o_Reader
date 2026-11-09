#include "oor/model/cards/nodes/nodeSequence/NodeSequenceTable.hpp"

#include "oor/model/cards/sets/DofLists.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <variant>

namespace oor::model
{

namespace
{

// A pair is two fields, from field 1: ID1 SEQID1 ID2 SEQID2 ...
int firstFieldOf(std::size_t index)
{
    return 1 + 2 * static_cast<int>(index);
}

} // namespace

std::optional<NodeSequenceCard> NodeSequenceFormat::read(FieldReader& reader)
{
    NodeSequenceCard card;
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; firstFieldOf(index) <= last && !reader.failed(); ++index)
    {
        const int field = firstFieldOf(index);
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue; // padding
        const std::string number = std::to_string(index + 1);
        const std::string idName = "node" + number;
        const std::string sequenceName = "sequenceId" + number;
        SequencedPoint point;
        point.node = reader.requiredInteger(field, idName.c_str());
        const auto sequence = reader.integerOrReal(field + 1, sequenceName.c_str());
        if (!sequence)
        {
            if (!reader.failed())
                reader.error("card.missing-field", "the sequence number is required", sequenceName.c_str());
            break;
        }
        point.sequence = std::holds_alternative<std::int32_t>(*sequence)
                             ? static_cast<double>(std::get<std::int32_t>(*sequence))
                             : std::get<double>(*sequence);
        card.points.push_back(point);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NodeSequenceFormat::check(const NodeSequenceCard& card)
{
    if (card.points.empty())
        return CardProblem{"card.missing-field", "the card needs at least one point", "node1"};
    if (card.points.size() > MAX_POINTS)
        return CardProblem{"card.bad-value", "the card has at most four points", "node1"};
    for (const SequencedPoint& point : card.points)
    {
        if (point.node <= 0)
            return CardProblem{"card.bad-id", "a grid or scalar point ID must be a positive integer", "node1"};
        if (!(point.sequence > 0.0) || !std::isfinite(point.sequence))
            return CardProblem{"card.bad-value", "a sequence number must be greater than zero", "sequenceId1"};
    }
    return std::nullopt;
}

void NodeSequenceFormat::write(const NodeSequenceCard& card, FieldWriter& writer)
{
    std::size_t index = 0;
    for (const SequencedPoint& point : card.points)
    {
        const int field = firstFieldOf(index++);
        writer.integer(field, point.node);
        // A whole number is written as an integer, anything else as a real.
        const double whole = std::floor(point.sequence);
        if (whole == point.sequence && point.sequence <= static_cast<double>(std::numeric_limits<std::int32_t>::max()))
            writer.integer(field + 1, static_cast<std::int64_t>(whole));
        else
            writer.real(field + 1, point.sequence);
    }
}

std::uint32_t NodeSequenceTable::append(const NodeSequenceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    std::vector<base::EntityId> nodes;
    std::vector<double> sequences;
    for (const SequencedPoint& point : card.points)
    {
        nodes.push_back(point.node);
        sequences.push_back(point.sequence);
    }
    m_node.push(nodes);
    m_sequence.push(sequences);
    return row;
}

NodeSequenceCard NodeSequenceTable::card(std::uint32_t row) const
{
    NodeSequenceCard card;
    const auto nodes = m_node.get(row);
    const auto sequences = m_sequence.get(row);
    for (std::size_t index = 0; index < nodes.size(); ++index)
        card.points.push_back(SequencedPoint{nodes[index], sequences[index]});
    return card;
}

void NodeSequenceTable::store(std::uint32_t row, const NodeSequenceCard& card)
{
    std::vector<base::EntityId> nodes;
    std::vector<double> sequences;
    for (const SequencedPoint& point : card.points)
    {
        nodes.push_back(point.node);
        sequences.push_back(point.sequence);
    }
    m_node.set(row, nodes);
    m_sequence.set(row, sequences);
}

void NodeSequenceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_node.keepRows(rows);
    m_sequence.keepRows(rows);
}

void NodeSequenceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitIdList(m_node, row, visitor, "node1", ReferenceTarget::Node);
}

} // namespace oor::model
