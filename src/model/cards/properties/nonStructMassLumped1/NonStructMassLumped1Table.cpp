#include "oor/model/cards/properties/nonStructMassLumped1/NonStructMassLumped1Table.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_ID_FIELD = 4;

} // namespace

std::optional<NonStructMassLumped1Card> NonStructMassLumped1Format::read(FieldReader& reader)
{
    NonStructMassLumped1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readNonStructMassType(reader, 2);
    card.nonStructMass = reader.requiredReal(3, "nonStructMass");
    card.ids = readNonStructMassIds(reader, FIRST_ID_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonStructMassLumped1Format::check(const NonStructMassLumped1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the non-structural mass set ID must be a positive integer", "id"};
    if (auto problem = checkNonStructMassType(card.type))
        return problem;
    return checkNonStructMassIds(card.ids);
}

void NonStructMassLumped1Format::write(const NonStructMassLumped1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.type);
    writer.real(3, card.nonStructMass);
    writeNonStructMassIds(writer, FIRST_ID_FIELD, card.ids);
}

std::uint32_t NonStructMassLumped1Table::append(const NonStructMassLumped1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_type.push(card.type);
    m_nonStructMass.push(card.nonStructMass);
    m_all.push(card.ids.all ? 1 : 0);
    m_runs.push(toTriples(card.ids.runs));
    return row;
}

NonStructMassLumped1Card NonStructMassLumped1Table::card(std::uint32_t row) const
{
    NonStructMassLumped1Card card;
    card.id = m_id.get(row);
    card.type = m_type.get(row);
    card.nonStructMass = m_nonStructMass.get(row);
    card.ids.all = m_all.get(row) != 0;
    card.ids.runs = toNonStructMassRuns(m_runs.get(row));
    return card;
}

void NonStructMassLumped1Table::store(std::uint32_t row, const NonStructMassLumped1Card& card)
{
    m_id.set(row, card.id);
    m_type.set(row, card.type);
    m_nonStructMass.set(row, card.nonStructMass);
    m_all.set(row, card.ids.all ? 1 : 0);
    m_runs.set(row, toTriples(card.ids.runs));
}

void NonStructMassLumped1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_type.keepRows(rows);
    m_nonStructMass.keepRows(rows);
    m_all.keepRows(rows);
    m_runs.keepRows(rows);
}

void NonStructMassLumped1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitNonStructMassRuns(m_runs, row, visitor, m_type.get(row));
}

} // namespace oor::model
