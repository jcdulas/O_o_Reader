#include "oor/model/cards/nonlinear_contact/contactControlParams/ContactControlParamsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

// The names checked in BCTPARM: PENN, PENT and CTOL are reals, SHLTHK an integer. Every value is required.
constexpr LegacyParameterRule RULES[] = {
    {"NormalPenalty", ContactParameterKind::Real, false, LEGACY_NO_LOW_LIMIT, LEGACY_NO_HIGH_LIMIT},
    {"TangentPenalty", ContactParameterKind::Real, false, LEGACY_NO_LOW_LIMIT, LEGACY_NO_HIGH_LIMIT},
    {"ContactTolerance", ContactParameterKind::Real, false, LEGACY_NO_LOW_LIMIT, LEGACY_NO_HIGH_LIMIT},
    {"ShellThicknessOption", ContactParameterKind::Integer, false, LEGACY_NO_LOW_LIMIT, LEGACY_NO_HIGH_LIMIT},
};

// The first pair is in field 2 (CSID is field 1): three pairs on the first line, then from field 9.
constexpr int FIRST_PAIR_FIELD = 2;

} // namespace

std::span<const LegacyParameterRule> ContactControlParamsFormat::rules()
{
    return RULES;
}

std::optional<ContactControlParamsCard> ContactControlParamsFormat::read(FieldReader& reader)
{
    ContactControlParamsCard card;
    card.contactSetId = reader.requiredInteger(1, "contactSetId");
    card.params = readLegacyContactParameters(reader, FIRST_PAIR_FIELD, RULES, ContactControlParamsTable::NAME);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactControlParamsFormat::check(const ContactControlParamsCard& card)
{
    if (const auto problem = checkRequiredId(card.contactSetId, "contactSetId"))
        return problem;
    return checkLegacyContactParameters(card.params, RULES, ContactControlParamsTable::NAME);
}

void ContactControlParamsFormat::write(const ContactControlParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.contactSetId);
    writeContactParameters(writer, FIRST_PAIR_FIELD, card.params, ContactControlParamsTable::NAME);
}

std::uint32_t ContactControlParamsTable::append(const ContactControlParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_contactSetId.push(card.contactSetId);
    m_cards.push(card);
    return row;
}

void ContactControlParamsTable::store(std::uint32_t row, const ContactControlParamsCard& card)
{
    m_contactSetId.set(row, card.contactSetId);
    m_cards.set(row, card);
}

void ContactControlParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_contactSetId.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactControlParamsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_contactSetId.set(row, id);
    m_cards.edit(row).contactSetId = id;
}

void ContactControlParamsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The CSID is the card's own ID and also the reference to the contact set, so a renumbering of the contact tables
    // changes both places: the card and the ID column.
    ContactControlParamsCard& card = m_cards.edit(row);
    visitId(visitor, "contactSetId", ReferenceTarget::ContactTable, card.contactSetId);
    m_contactSetId.set(row, card.contactSetId);
}

} // namespace oor::model
