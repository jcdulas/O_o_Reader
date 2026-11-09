#include "oor/model/cards/nonlinear_contact/contactPairPhysicalParams/ContactPairPhysicalParamsTable.hpp"

namespace oor::model
{

namespace
{

// The parameters of SOLs 101 and 400, then those of SOL 700. A coefficient or an exponent is a real or
// the ID of a table of values (a Number); FRIC is in both lists.
constexpr ContactParameterRule RULES[] = {
    // SOLs 101 and 400.
    {"GlueBreakNormalExponent", ContactParameterKind::Real},
    {"GlueBreakShearExponent", ContactParameterKind::Real},
    {"GlueBreakNormalStress", ContactParameterKind::Real},
    {"GlueBreakShearStress", ContactParameterKind::Real},
    {"NearContactNaturalConvectionExponent", ContactParameterKind::Number},
    {"NearContactNonlinearConvectionExponent", ContactParameterKind::Number},
    {"NearContactDistance", ContactParameterKind::Real},
    {"Emissivity", ContactParameterKind::Number},
    {"SeparationForce", ContactParameterKind::Real},
    {"FrictionCoefficient", ContactParameterKind::Number},
    {"FrictionStressLimit", ContactParameterKind::Real},
    {"DistanceDependentConvectionCoefficient", ContactParameterKind::Number},
    {"ContactHeatTransferCoefficient", ContactParameterKind::Number},
    {"NearContactConvectionCoefficient", ContactParameterKind::Number},
    {"GlueHeatTransfer", ContactParameterKind::Integer},
    {"NearContactNaturalConvectionCoefficient", ContactParameterKind::Number},
    {"NearContactNonlinearConvectionCoefficient", ContactParameterKind::Number},
    // SOL 700.
    {"KineticFrictionCoefficient", ContactParameterKind::Real},
    {"FrictionDecayCoefficient", ContactParameterKind::Real},
    {"PenaltyScaleFactor", ContactParameterKind::Real},
    {"ContactStartTime", ContactParameterKind::Real},
    {"ContactEndTime", ContactParameterKind::Real},
    {"InitialPenetrationAdjustment", ContactParameterKind::Integer},
    {"ContactDamping", ContactParameterKind::Text},
};

// The ID (PID) is field 1 and field 2 is blank: the first pair is at field 3.
constexpr int FIRST_PAIR_FIELD = 3;

} // namespace

std::span<const ContactParameterRule> ContactPairPhysicalParamsFormat::rules()
{
    return RULES;
}

std::optional<ContactPairPhysicalParamsCard> ContactPairPhysicalParamsFormat::read(FieldReader& reader)
{
    ContactPairPhysicalParamsCard card;
    card.id = reader.requiredInteger(1, "id");
    if (!reader.isBlank(2))
        reader.error("card.bad-value",
                     "field 2 of a " + std::string(reader.cardName()) + " is blank: the first parameter is in field 3",
                     "parameterName");
    card.params = readContactParameters(reader, FIRST_PAIR_FIELD, RULES, ContactPairPhysicalParamsTable::NAME);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactPairPhysicalParamsFormat::check(const ContactPairPhysicalParamsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    return checkContactParameters(card.params, RULES, ContactPairPhysicalParamsTable::NAME);
}

void ContactPairPhysicalParamsFormat::write(const ContactPairPhysicalParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeContactParameters(writer, FIRST_PAIR_FIELD, card.params, ContactPairPhysicalParamsTable::NAME);
}

std::uint32_t ContactPairPhysicalParamsTable::append(const ContactPairPhysicalParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactPairPhysicalParamsTable::store(std::uint32_t row, const ContactPairPhysicalParamsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactPairPhysicalParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactPairPhysicalParamsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
