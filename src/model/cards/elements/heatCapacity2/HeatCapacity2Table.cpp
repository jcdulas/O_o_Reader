#include "oor/model/cards/elements/heatCapacity2/HeatCapacity2Table.hpp"

namespace oor::model
{

std::optional<HeatCapacity2Card> HeatCapacity2Format::read(FieldReader& reader)
{
    HeatCapacity2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    card.node1 = reader.integer(3, "node1", 0);         // blank: grounded
    card.node2 = reader.integer(4, "node2", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatCapacity2Format::check(const HeatCapacity2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.node1 < 0 || card.node2 < 0)
        return CardProblem{"card.bad-id", "the point IDs must not be negative", "node1"};
    if (card.node1 == card.node2)
        return CardProblem{"card.duplicate-node", "the two points must be different", "node2"};
    return std::nullopt;
}

void HeatCapacity2Format::write(const HeatCapacity2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integerUnlessDefault(3, card.node1, 0);
    writer.integerUnlessDefault(4, card.node2, 0);
}

std::uint32_t HeatCapacity2Table::append(const HeatCapacity2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_node1.push(card.node1);
    m_node2.push(card.node2);
    return row;
}

HeatCapacity2Card HeatCapacity2Table::card(std::uint32_t row) const
{
    return HeatCapacity2Card{m_id.get(row), m_propId.get(row), m_node1.get(row), m_node2.get(row)};
}

void HeatCapacity2Table::store(std::uint32_t row, const HeatCapacity2Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_node1.set(row, card.node1);
    m_node2.set(row, card.node2);
}

void HeatCapacity2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_node1.keepRows(rows);
    m_node2.keepRows(rows);
}

void HeatCapacity2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    // G1 and G2 are grid points or scalar points (a scalar point may be one no SPOINT defines).
    if (const base::EntityId node1 = m_node1.get(row); node1 > 0)
        m_node1.set(row, visitor.visit("node1", ReferenceTarget::ScalarOrGridPoint, node1));
    if (const base::EntityId node2 = m_node2.get(row); node2 > 0)
        m_node2.set(row, visitor.visit("node2", ReferenceTarget::ScalarOrGridPoint, node2));
}

} // namespace oor::model
