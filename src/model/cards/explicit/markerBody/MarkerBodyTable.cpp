#include "oor/model/cards/explicit/markerBody/MarkerBodyTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<MarkerBodyCard> MarkerBodyFormat::read(FieldReader& reader)
{
    MarkerBodyCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.node1 = reader.requiredInteger(3, "node1");
    card.node2 = reader.requiredInteger(4, "node2");
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 4 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MarkerBodyFormat::check(const MarkerBodyCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkRequiredId(card.node1, "node1"))
        return problem;
    if (const auto problem = checkRequiredId(card.node2, "node2"))
        return problem;
    // A line between a point and itself records nothing.
    if (card.node1 == card.node2)
        return CardProblem{"card.duplicate-node", "the two grid points must be different", "node2"};
    return std::nullopt;
}

void MarkerBodyFormat::write(const MarkerBodyCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.node1);
    writer.integer(4, card.node2);
}

std::uint32_t MarkerBodyTable::append(const MarkerBodyCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void MarkerBodyTable::store(std::uint32_t row, const MarkerBodyCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void MarkerBodyTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void MarkerBodyTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void MarkerBodyTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    MarkerBodyCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId);
    visitId(visitor, "node1", ReferenceTarget::Node, card.node1);
    visitId(visitor, "node2", ReferenceTarget::Node, card.node2);
}

} // namespace oor::model
