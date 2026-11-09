#include "oor/model/cards/nonlinear_contact/contactAutoParams/ContactAutoParamsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/contactBodyParams/ContactBodyParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPairGeomParams/ContactPairGeomParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPairPhysicalParams/ContactPairPhysicalParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactParams/ContactParamsTable.hpp"

namespace oor::model
{

namespace
{

// The parameters of the card. The SET1 IDs are integers; FTYPE is BLCOUL or BLSHEAR, or 6 or 7; FRIC is
// a real or the ID of a table.
constexpr ContactParameterRule OWN_RULES[] = {
    {"ContactKind", ContactParameterKind::Text},
    {"DetectionDistance", ContactParameterKind::Real},
    {"ElemSet", ContactParameterKind::Integer},
    {"SeedElemSet", ContactParameterKind::Integer},
    {"SeedNodeSet", ContactParameterKind::Integer},
    {"PropertySet", ContactParameterKind::Integer},
    {"RigidBodySet", ContactParameterKind::Integer},
    {"BeamContact", ContactParameterKind::Text},
    {"BeamCapping", ContactParameterKind::Text},
    {"ModelDimension", ContactParameterKind::Text},
    {"EdgeContact", ContactParameterKind::Text},
    {"FrictionKind", ContactParameterKind::Any},
    {"FrictionCoefficient", ContactParameterKind::Number},
    {"IgnoreThickness", ContactParameterKind::Text},
    {"InitialStressFree", ContactParameterKind::Text},
    {"SelfContact", ContactParameterKind::Text},
};

// The card types whose keywords the names take, in the order of rules(): its own, then the borrowed ones.
constexpr std::string_view OWNERS[] = {ContactAutoParamsTable::NAME, ContactParamsTable::NAME,
                                       ContactPairPhysicalParamsTable::NAME, ContactPairGeomParamsTable::NAME,
                                       ContactBodyParamsTable::NAME};

// The ID (PID) is field 1 and field 2 is blank: the first pair is at field 3.
constexpr int FIRST_PAIR_FIELD = 3;

bool namesSet(std::string_view name)
{
    for (const std::string_view set : {"ElemSet", "SeedElemSet", "SeedNodeSet", "PropertySet", "RigidBodySet"})
    {
        if (name == set)
            return true;
    }
    return false;
}

} // namespace

std::span<const ContactParameterRule> ContactAutoParamsFormat::rules()
{
    static const std::vector<ContactParameterRule> ALL = []
    {
        std::vector<ContactParameterRule> rules(std::begin(OWN_RULES), std::end(OWN_RULES));
        for (const auto other : {ContactParamsFormat::rules(), ContactPairPhysicalParamsFormat::rules(),
                                 ContactPairGeomParamsFormat::rules(), ContactBodyParamsFormat::rules()})
            rules.insert(rules.end(), other.begin(), other.end());
        return rules;
    }();
    return ALL;
}

std::optional<ContactAutoParamsCard> ContactAutoParamsFormat::read(FieldReader& reader)
{
    ContactAutoParamsCard card;
    card.id = reader.integer(1, "id", 0);
    if (!reader.isBlank(2))
        reader.error("card.bad-value",
                     "field 2 of a " + std::string(reader.cardName()) + " is blank: the first parameter is in field 3",
                     "parameterName");
    card.params = readContactParameters(reader, FIRST_PAIR_FIELD, rules(), OWNERS);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactAutoParamsFormat::check(const ContactAutoParamsCard& card)
{
    if (card.id < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "id"};
    return checkContactParameters(card.params, rules(), OWNERS);
}

void ContactAutoParamsFormat::write(const ContactAutoParamsCard& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.id, 0);
    writeContactParameters(writer, FIRST_PAIR_FIELD, card.params, OWNERS);
}

std::uint32_t ContactAutoParamsTable::append(const ContactAutoParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactAutoParamsTable::store(std::uint32_t row, const ContactAutoParamsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactAutoParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactAutoParamsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ContactAutoParamsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (ContactParameter& parameter : m_cards.edit(row).params)
    {
        auto* value = std::get_if<std::int64_t>(&parameter.value);
        if (value == nullptr || !namesSet(parameter.name) || *value <= 0 || *value > base::MAX_ENTITY_ID)
            continue;
        const auto id = static_cast<base::EntityId>(*value);
        const base::EntityId updated = visitor.visit("setId", ReferenceTarget::Set, id);
        if (updated != id)
            *value = updated;
    }
}

} // namespace oor::model
