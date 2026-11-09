#include "oor/model/cards/nonlinear_contact/contactElemSurface/ContactElemSurfaceTable.hpp"

namespace oor::model
{

namespace
{

constexpr ContactRunRules RULES{true, true};

} // namespace

std::optional<ContactElemSurfaceCard> ContactElemSurfaceFormat::read(FieldReader& reader)
{
    ContactElemSurfaceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.elements = readContactRuns(reader, 2, reader.lastUsedField(), "elements", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactElemSurfaceFormat::check(const ContactElemSurfaceCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.elements.empty())
        return CardProblem{"card.missing-field", "the surface needs at least one element", "elements"};
    return checkContactRuns(card.elements, "elements", RULES);
}

void ContactElemSurfaceFormat::write(const ContactElemSurfaceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeContactRuns(writer, 2, card.elements);
}

std::uint32_t ContactElemSurfaceTable::append(const ContactElemSurfaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_runs.push(toContactTriples(card.elements));
    return row;
}

ContactElemSurfaceCard ContactElemSurfaceTable::card(std::uint32_t row) const
{
    ContactElemSurfaceCard card;
    card.id = m_id.get(row);
    card.elements = toContactRuns(m_runs.get(row));
    return card;
}

void ContactElemSurfaceTable::store(std::uint32_t row, const ContactElemSurfaceCard& card)
{
    m_id.set(row, card.id);
    m_runs.set(row, toContactTriples(card.elements));
}

void ContactElemSurfaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_runs.keepRows(rows);
}

void ContactElemSurfaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactTriples(m_runs, row, visitor, "elements", ReferenceTarget::Element, ReferenceTarget::ElementInRange);
}

} // namespace oor::model
