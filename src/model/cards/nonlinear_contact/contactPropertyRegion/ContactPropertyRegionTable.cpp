#include "oor/model/cards/nonlinear_contact/contactPropertyRegion/ContactPropertyRegionTable.hpp"

namespace oor::model
{

namespace
{

constexpr ContactRunRules RULES{false, false};

} // namespace

std::optional<ContactPropertyRegionCard> ContactPropertyRegionFormat::read(FieldReader& reader)
{
    ContactPropertyRegionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.properties = readContactRuns(reader, 2, reader.lastUsedField(), "propIds", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactPropertyRegionFormat::check(const ContactPropertyRegionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.properties.empty())
        return CardProblem{"card.missing-field", "the region needs at least one property", "propIds"};
    return checkContactRuns(card.properties, "propIds", RULES);
}

void ContactPropertyRegionFormat::write(const ContactPropertyRegionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeContactRuns(writer, 2, card.properties);
}

std::uint32_t ContactPropertyRegionTable::append(const ContactPropertyRegionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_runs.push(toContactTriples(card.properties));
    return row;
}

ContactPropertyRegionCard ContactPropertyRegionTable::card(std::uint32_t row) const
{
    ContactPropertyRegionCard card;
    card.id = m_id.get(row);
    card.properties = toContactRuns(m_runs.get(row));
    return card;
}

void ContactPropertyRegionTable::store(std::uint32_t row, const ContactPropertyRegionCard& card)
{
    m_id.set(row, card.id);
    m_runs.set(row, toContactTriples(card.properties));
}

void ContactPropertyRegionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_runs.keepRows(rows);
}

void ContactPropertyRegionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactTriples(m_runs, row, visitor, "propIds", ReferenceTarget::Property, ReferenceTarget::PropertyInRange);
}

} // namespace oor::model
