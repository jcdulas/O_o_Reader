#include "oor/model/cards/explicit/markerNode/MarkerNodeTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<MarkerNodeCard> MarkerNodeFormat::read(FieldReader& reader)
{
    MarkerNodeCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.node1 = reader.requiredInteger(3, "node1");
    if (reader.lastUsedField() > 3)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 3 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MarkerNodeFormat::check(const MarkerNodeCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    return checkRequiredId(card.node1, "node1");
}

void MarkerNodeFormat::write(const MarkerNodeCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.node1);
}

std::uint32_t MarkerNodeTable::append(const MarkerNodeCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void MarkerNodeTable::store(std::uint32_t row, const MarkerNodeCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void MarkerNodeTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void MarkerNodeTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void MarkerNodeTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    MarkerNodeCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId);
    visitId(visitor, "node1", ReferenceTarget::Node, card.node1);
}

} // namespace oor::model
