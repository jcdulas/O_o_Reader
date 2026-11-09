#include "oor/model/cards/elements/massDofProperty/MassDofPropertyTable.hpp"

namespace oor::model
{

std::optional<MassDofPropertyCard> MassDofPropertyFormat::read(FieldReader& reader)
{
    MassDofPropertyCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    card.node1 = reader.integer(3, "node1", 0);         // blank: grounded
    card.components1 = reader.integer(4, "components1", 0);
    card.node2 = reader.integer(5, "node2", 0);
    card.components2 = reader.integer(6, "components2", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MassDofPropertyFormat::check(const MassDofPropertyCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.node1 < 0 || card.node2 < 0)
        return CardProblem{"card.bad-id", "the point IDs must not be negative", "node1"};
    if (card.components1 < 0 || card.components1 > 6)
        return CardProblem{"card.bad-value", "{field} must be a component from 0 to 6", "components1"};
    if (card.components2 < 0 || card.components2 > 6)
        return CardProblem{"card.bad-value", "{field} must be a component from 0 to 6", "components2"};
    if (card.node1 == card.node2 && card.components1 == card.components2)
        return CardProblem{"card.duplicate-node",
                           "the two connection points ({field:node1}, {field:components1}) and ({field}, "
                           "{field:components2}) must be distinct",
                           "node2"};
    return std::nullopt;
}

void MassDofPropertyFormat::write(const MassDofPropertyCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integerUnlessDefault(3, card.node1, 0);
    writer.integerUnlessDefault(4, card.components1, 0);
    writer.integerUnlessDefault(5, card.node2, 0);
    writer.integerUnlessDefault(6, card.components2, 0);
}

std::uint32_t MassDofPropertyTable::append(const MassDofPropertyCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_node1.push(card.node1);
    m_components1.push(card.components1);
    m_node2.push(card.node2);
    m_components2.push(card.components2);
    return row;
}

MassDofPropertyCard MassDofPropertyTable::card(std::uint32_t row) const
{
    return MassDofPropertyCard{m_id.get(row),          m_propId.get(row), m_node1.get(row),
                         m_components1.get(row), m_node2.get(row),  m_components2.get(row)};
}

void MassDofPropertyTable::store(std::uint32_t row, const MassDofPropertyCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_node1.set(row, card.node1);
    m_components1.set(row, card.components1);
    m_node2.set(row, card.node2);
    m_components2.set(row, card.components2);
}

void MassDofPropertyTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_node1.keepRows(rows);
    m_components1.keepRows(rows);
    m_node2.keepRows(rows);
    m_components2.keepRows(rows);
}

void MassDofPropertyTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    // G1 and G2 are grid points or scalar points (0: ground); a scalar point may be one no SPOINT defines.
    if (const base::EntityId node1 = m_node1.get(row); node1 > 0)
        m_node1.set(row, visitor.visit("node1", ReferenceTarget::ScalarOrGridPoint, node1));
    if (const base::EntityId node2 = m_node2.get(row); node2 > 0)
        m_node2.set(row, visitor.visit("node2", ReferenceTarget::ScalarOrGridPoint, node2));
}

} // namespace oor::model
