#include "oor/model/cards/nonlinear_contact/contactSegment/ContactSegmentTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

namespace oor::model
{

std::optional<ContactSegmentCard> ContactSegmentFormat::read(FieldReader& reader)
{
    ContactSegmentCard card;
    card.id = reader.requiredInteger(1, "id");
    card.surfaceId = reader.requiredInteger(2, "surfaceId");
    card.nodes[0] = reader.requiredInteger(3, "node1");
    card.nodes[1] = reader.requiredInteger(4, "node2");
    card.nodes[2] = reader.requiredInteger(5, "node3");
    card.nodes[3] = reader.integer(6, "node4", 0);
    if (reader.lastUsedField() > 6)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has no field beyond " + std::string("node4"), "node4");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactSegmentFormat::check(const ContactSegmentCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.surfaceId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "surfaceId"};
    static constexpr const char* NAMES[4] = {"node1", "node2", "node3", "node4"};
    for (std::size_t index = 0; index < 4; ++index)
    {
        if (card.nodes[index] < 0 || (index < 3 && card.nodes[index] == 0))
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", NAMES[index]};
        for (std::size_t other = 0; other < index; ++other)
        {
            if (card.nodes[index] != 0 && card.nodes[index] == card.nodes[other])
                return CardProblem{"card.bad-value", "the grid points of a segment must be different", NAMES[index]};
        }
    }
    return std::nullopt;
}

void ContactSegmentFormat::write(const ContactSegmentCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.surfaceId);
    writer.integer(3, card.nodes[0]);
    writer.integer(4, card.nodes[1]);
    writer.integer(5, card.nodes[2]);
    writer.integerUnlessDefault(6, card.nodes[3], 0);
}

std::uint32_t ContactSegmentTable::append(const ContactSegmentCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_surfaceId.push(card.surfaceId);
    m_nodes.push(card.nodes);
    return row;
}

ContactSegmentCard ContactSegmentTable::card(std::uint32_t row) const
{
    ContactSegmentCard card;
    card.id = m_id.get(row);
    card.surfaceId = m_surfaceId.get(row);
    card.nodes = m_nodes.get(row);
    return card;
}

void ContactSegmentTable::store(std::uint32_t row, const ContactSegmentCard& card)
{
    m_id.set(row, card.id);
    m_surfaceId.set(row, card.surfaceId);
    m_nodes.set(row, card.nodes);
}

void ContactSegmentTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_surfaceId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void ContactSegmentTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, 4> nodes = m_nodes.get(row);
    const std::array<base::EntityId, 4> before = nodes;
    for (base::EntityId& grid : nodes)
        visitContactId(grid, visitor, "nodes", ReferenceTarget::Node);
    if (nodes != before)
        m_nodes.set(row, nodes);
}

} // namespace oor::model
