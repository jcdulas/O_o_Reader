#include "oor/model/cards/nonlinear_contact/contactTable1/ContactTable1Table.hpp"

namespace oor::model
{

namespace
{

constexpr ContactRunRules RULES{false, false};

} // namespace

std::optional<ContactTable1Card> ContactTable1Format::read(FieldReader& reader)
{
    ContactTable1Card card;
    card.id = reader.integer(1, "id", 0);
    card.pairs = readContactRuns(reader, 2, reader.lastUsedField(), "pairIds", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactTable1Format::check(const ContactTable1Card& card)
{
    if (card.id < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "id"};
    if (card.pairs.empty())
        return CardProblem{"card.missing-field", "the table needs at least one contact pair", "pairIds"};
    if (card.pairs.front().isRange())
        return CardProblem{"card.bad-value", "the first entry must be the ID of a contact pair, not a range",
                           "pairIds"};
    return checkContactRuns(card.pairs, "pairIds", RULES);
}

void ContactTable1Format::write(const ContactTable1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeContactRuns(writer, 2, card.pairs);
}

std::uint32_t ContactTable1Table::append(const ContactTable1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_runs.push(toContactTriples(card.pairs));
    return row;
}

ContactTable1Card ContactTable1Table::card(std::uint32_t row) const
{
    ContactTable1Card card;
    card.id = m_id.get(row);
    card.pairs = toContactRuns(m_runs.get(row));
    return card;
}

void ContactTable1Table::store(std::uint32_t row, const ContactTable1Card& card)
{
    m_id.set(row, card.id);
    m_runs.set(row, toContactTriples(card.pairs));
}

void ContactTable1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_runs.keepRows(rows);
}

void ContactTable1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ends of a THRU range must exist like the single IDs: the format reference wants every BCONECT between them.
    visitContactTriples(m_runs, row, visitor, "pairIds", ReferenceTarget::ContactPair, ReferenceTarget::ContactPair);
}

} // namespace oor::model
