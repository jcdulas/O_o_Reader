#include "oor/model/cards/loads/axisymTractionLoad/AxisymTractionLoadTable.hpp"

namespace oor::model
{

std::optional<AxisymTractionLoadCard> AxisymTractionLoadFormat::read(FieldReader& reader)
{
    AxisymTractionLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.elemId = reader.requiredInteger(2, "elemId");
    card.tractionA = reader.requiredReal(3, "tractionA");
    card.tractionB = reader.real(4, "tractionB", card.tractionA); // format reference: PB defaults to PA
    card.nodeA = reader.requiredInteger(5, "nodeA");
    card.nodeB = reader.requiredInteger(6, "nodeB");
    card.tractionAngle = reader.real(7, "tractionAngle", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymTractionLoadFormat::check(const AxisymTractionLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.elemId <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "elemId"};
    if (card.nodeA <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "nodeA"};
    if (card.nodeB <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "nodeB"};
    if (card.nodeA == card.nodeB)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("nodeA") + " and " + std::string(CardProblem::FIELD) +
                               " are the two ends of a side and must differ",
                           "nodeB"};
    return std::nullopt;
}

void AxisymTractionLoadFormat::write(const AxisymTractionLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.elemId);
    writer.real(3, card.tractionA);
    writer.real(4, card.tractionB); // depends on PA: always written
    writer.integer(5, card.nodeA);
    writer.integer(6, card.nodeB);
    writer.realUnlessDefault(7, card.tractionAngle, 0.0);
}

std::uint32_t AxisymTractionLoadTable::append(const AxisymTractionLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_elemId.push(card.elemId);
    m_tractionA.push(card.tractionA);
    m_tractionB.push(card.tractionB);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_tractionAngle.push(card.tractionAngle);
    return row;
}

AxisymTractionLoadCard AxisymTractionLoadTable::card(std::uint32_t row) const
{
    return AxisymTractionLoadCard{m_id.get(row),    m_elemId.get(row), m_tractionA.get(row),    m_tractionB.get(row),
                                  m_nodeA.get(row), m_nodeB.get(row),  m_tractionAngle.get(row)};
}

void AxisymTractionLoadTable::store(std::uint32_t row, const AxisymTractionLoadCard& card)
{
    m_id.set(row, card.id);
    m_elemId.set(row, card.elemId);
    m_tractionA.set(row, card.tractionA);
    m_tractionB.set(row, card.tractionB);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_tractionAngle.set(row, card.tractionAngle);
}

void AxisymTractionLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_elemId.keepRows(rows);
    m_tractionA.keepRows(rows);
    m_tractionB.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_tractionAngle.keepRows(rows);
}

void AxisymTractionLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_elemId.set(row, visitor.visit("elemId", ReferenceTarget::Element, m_elemId.get(row)));
    m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, m_nodeA.get(row)));
    m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, m_nodeB.get(row)));
}

} // namespace oor::model
