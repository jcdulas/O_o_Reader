#include "oor/model/cards/nonlinear_contact/contactUnglueNodes/ContactUnglueNodesTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

// "a THRU b BY n" with n positive: the format wants a positive increment, so a range goes up.
constexpr ContactRunRules RULES{true, false};

// The grid points start in field 3, after the ID and the body.
constexpr int FIRST_GRID_FIELD = 3;

} // namespace

std::optional<ContactUnglueNodesCard> ContactUnglueNodesFormat::read(FieldReader& reader)
{
    ContactUnglueNodesCard card;
    card.id = reader.requiredInteger(1, "id");
    card.bodyId = reader.requiredInteger(2, "bodyId");
    card.grids = readContactRuns(reader, FIRST_GRID_FIELD, reader.lastUsedField(), "nodes", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactUnglueNodesFormat::check(const ContactUnglueNodesCard& card)
{
    // The ID of 0 is the load case 0: the only number of the card that may be zero.
    if (card.id < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be zero or a positive integer",
                           "id"};
    if (const auto problem = checkRequiredId(card.bodyId, "bodyId"))
        return problem;
    if (card.grids.empty())
        return CardProblem{"card.missing-field", "the card needs the grid points that leave the glued contact",
                           "nodes"};
    return checkContactRuns(card.grids, "nodes", RULES);
}

void ContactUnglueNodesFormat::write(const ContactUnglueNodesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.bodyId);
    writeContactRuns(writer, FIRST_GRID_FIELD, card.grids);
}

std::uint32_t ContactUnglueNodesTable::append(const ContactUnglueNodesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactUnglueNodesTable::store(std::uint32_t row, const ContactUnglueNodesCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactUnglueNodesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactUnglueNodesTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ContactUnglueNodesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ContactUnglueNodesCard& card = m_cards.edit(row);
    visitId(visitor, "bodyId", ReferenceTarget::ContactBody, card.bodyId);
    visitContactRuns(card.grids, visitor, "nodes", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
