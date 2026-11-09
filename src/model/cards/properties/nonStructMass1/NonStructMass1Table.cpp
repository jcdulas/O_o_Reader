#include "oor/model/cards/properties/nonStructMass1/NonStructMass1Table.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_ID_FIELD = 4;

} // namespace

std::optional<NonStructMass1Card> NonStructMass1Format::read(FieldReader& reader)
{
    NonStructMass1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readNonStructMassType(reader, 2);
    card.nonStructMass = reader.requiredReal(3, "nonStructMass");
    card.ids = readNonStructMassIds(reader, FIRST_ID_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonStructMass1Format::check(const NonStructMass1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the non-structural mass set ID must be a positive integer", "id"};
    if (auto problem = checkNonStructMassType(card.type))
        return problem;
    return checkNonStructMassIds(card.ids);
}

void NonStructMass1Format::write(const NonStructMass1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.type);
    writer.real(3, card.nonStructMass);
    writeNonStructMassIds(writer, FIRST_ID_FIELD, card.ids);
}

std::uint32_t NonStructMass1Table::append(const NonStructMass1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_type.push(card.type);
    m_nonStructMass.push(card.nonStructMass);
    m_all.push(card.ids.all ? 1 : 0);
    m_runs.push(toTriples(card.ids.runs));
    return row;
}

NonStructMass1Card NonStructMass1Table::card(std::uint32_t row) const
{
    NonStructMass1Card card;
    card.id = m_id.get(row);
    card.type = m_type.get(row);
    card.nonStructMass = m_nonStructMass.get(row);
    card.ids.all = m_all.get(row) != 0;
    card.ids.runs = toNonStructMassRuns(m_runs.get(row));
    return card;
}

void NonStructMass1Table::store(std::uint32_t row, const NonStructMass1Card& card)
{
    m_id.set(row, card.id);
    m_type.set(row, card.type);
    m_nonStructMass.set(row, card.nonStructMass);
    m_all.set(row, card.ids.all ? 1 : 0);
    m_runs.set(row, toTriples(card.ids.runs));
}

void NonStructMass1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_type.keepRows(rows);
    m_nonStructMass.keepRows(rows);
    m_all.keepRows(rows);
    m_runs.keepRows(rows);
}

void NonStructMass1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitNonStructMassRuns(m_runs, row, visitor, m_type.get(row));
}

} // namespace oor::model
