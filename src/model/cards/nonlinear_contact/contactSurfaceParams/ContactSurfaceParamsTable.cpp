#include "oor/model/cards/nonlinear_contact/contactSurfaceParams/ContactSurfaceParamsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

// The names checked in BCTPARA, with the range of their values. A blank value gives the default of the parameter; the
// card keeps it blank.
constexpr LegacyParameterRule RULES[] = {
    {"ContactKind", ContactParameterKind::Integer, true, 0.0, 2.0},
    {"SideCount", ContactParameterKind::Integer, true, 1.0, 2.0},
    {"BirthTime", ContactParameterKind::Real, true, LEGACY_NO_LOW_LIMIT, LEGACY_NO_HIGH_LIMIT},
    {"DeathTime", ContactParameterKind::Real, true, LEGACY_NO_LOW_LIMIT, LEGACY_NO_HIGH_LIMIT},
    {"InitialPenetration", ContactParameterKind::Integer, true, 0.0, 3.0},
    {"PenetrationDepth", ContactParameterKind::Real, true, LEGACY_NO_LOW_LIMIT, LEGACY_NO_HIGH_LIMIT},
    {"SegmentNormal", ContactParameterKind::Integer, true, -1.0, 1.0},
    {"OffsetKind", ContactParameterKind::Integer, true, 0.0, 2.0},
    {"OffsetDistance", ContactParameterKind::Real, true, LEGACY_NO_LOW_LIMIT, LEGACY_NO_HIGH_LIMIT},
    {"ZeroPenetrationTime", ContactParameterKind::Real, true, LEGACY_NO_LOW_LIMIT, LEGACY_NO_HIGH_LIMIT},
    {"ContactStiffness", ContactParameterKind::Integer, true, 0.0, 1.0},
    {"TiedContact", ContactParameterKind::Integer, true, 0.0, 1.0},
    {"TiedTolerance", ContactParameterKind::Real, true, LEGACY_NO_LOW_LIMIT, LEGACY_NO_HIGH_LIMIT},
    {"ExtensionFactor", ContactParameterKind::Real, true, 1.0E-6, 0.1},
};

// The first pair is in field 2 (CSID is field 1): three pairs on the first line, then from field 9.
constexpr int FIRST_PAIR_FIELD = 2;

} // namespace

std::span<const LegacyParameterRule> ContactSurfaceParamsFormat::rules()
{
    return RULES;
}

std::optional<ContactSurfaceParamsCard> ContactSurfaceParamsFormat::read(FieldReader& reader)
{
    ContactSurfaceParamsCard card;
    card.contactSetId = reader.requiredInteger(1, "contactSetId");
    card.params = readLegacyContactParameters(reader, FIRST_PAIR_FIELD, RULES, ContactSurfaceParamsTable::NAME);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactSurfaceParamsFormat::check(const ContactSurfaceParamsCard& card)
{
    if (const auto problem = checkRequiredId(card.contactSetId, "contactSetId"))
        return problem;
    return checkLegacyContactParameters(card.params, RULES, ContactSurfaceParamsTable::NAME);
}

void ContactSurfaceParamsFormat::write(const ContactSurfaceParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.contactSetId);
    writeContactParameters(writer, FIRST_PAIR_FIELD, card.params, ContactSurfaceParamsTable::NAME);
}

std::uint32_t ContactSurfaceParamsTable::append(const ContactSurfaceParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_contactSetId.push(card.contactSetId);
    m_cards.push(card);
    return row;
}

void ContactSurfaceParamsTable::store(std::uint32_t row, const ContactSurfaceParamsCard& card)
{
    m_contactSetId.set(row, card.contactSetId);
    m_cards.set(row, card);
}

void ContactSurfaceParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_contactSetId.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactSurfaceParamsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_contactSetId.set(row, id);
    m_cards.edit(row).contactSetId = id;
}

void ContactSurfaceParamsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The CSID is the card's own ID and also the reference to the contact set, so a renumbering of the contact tables
    // changes both places: the card and the ID column.
    ContactSurfaceParamsCard& card = m_cards.edit(row);
    visitId(visitor, "contactSetId", ReferenceTarget::ContactTable, card.contactSetId);
    m_contactSetId.set(row, card.contactSetId);
}

} // namespace oor::model
