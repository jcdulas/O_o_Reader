#include "oor/model/cards/nonlinear_contact/contactBodyParams/ContactBodyParamsTable.hpp"

namespace oor::model
{

namespace
{

// The parameters of the card. A coefficient or an exponent is a real or the ID of a table of values (a Number);
// TBODY and TSINK are a temperature, the ID of a table of temperatures against time, or minus a scalar point.
constexpr ContactParameterRule RULES[] = {
    {"NearContactNaturalConvectionExponent", ContactParameterKind::Number},
    {"EnvironmentNaturalConvectionExponent", ContactParameterKind::Number},
    {"NearContactNonlinearConvectionExponent", ContactParameterKind::Number},
    {"EnvironmentNonlinearConvectionExponent", ContactParameterKind::Number},
    {"EnvironmentFilmCoefficient", ContactParameterKind::Number},
    {"AdditionalHeatCoefficient1", ContactParameterKind::Real},
    {"AdditionalHeatCoefficient2", ContactParameterKind::Real},
    {"BodyContactOptions", ContactParameterKind::Integer},
    {"Emissivity", ContactParameterKind::Number},
    {"FrictionCoefficient", ContactParameterKind::Number},
    {"DistanceDependentConvectionCoefficient", ContactParameterKind::Number},
    {"ContactHeatTransferCoefficient", ContactParameterKind::Number},
    {"NearContactConvectionCoefficient", ContactParameterKind::Number},
    {"NearContactNaturalConvectionCoefficient", ContactParameterKind::Number},
    {"EnvironmentNaturalConvectionCoefficient", ContactParameterKind::Number},
    {"NearContactNonlinearConvectionCoefficient", ContactParameterKind::Number},
    {"EnvironmentNonlinearConvectionCoefficient", ContactParameterKind::Number},
    {"DisplacementOption", ContactParameterKind::Integer},
    {"InterfaceSubtype", ContactParameterKind::Integer},
    {"ThermalInterfaceKind", ContactParameterKind::Integer},
    {"MidsideNodes", ContactParameterKind::Integer},
    {"SeparationAngle", ContactParameterKind::Real},
    {"BodyTemperature", ContactParameterKind::Number},
    {"SinkTemperature", ContactParameterKind::Number},
};

// The ID (PID) is field 1 and field 2 is blank: the first pair is at field 3.
constexpr int FIRST_PAIR_FIELD = 3;

} // namespace

std::span<const ContactParameterRule> ContactBodyParamsFormat::rules()
{
    return RULES;
}

std::optional<ContactBodyParamsCard> ContactBodyParamsFormat::read(FieldReader& reader)
{
    ContactBodyParamsCard card;
    card.id = reader.requiredInteger(1, "id");
    if (!reader.isBlank(2))
        reader.error("card.bad-value",
                     "field 2 of a " + std::string(reader.cardName()) + " is blank: the first parameter is in field 3",
                     "parameterName");
    card.params = readContactParameters(reader, FIRST_PAIR_FIELD, RULES, ContactBodyParamsTable::NAME);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactBodyParamsFormat::check(const ContactBodyParamsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    return checkContactParameters(card.params, RULES, ContactBodyParamsTable::NAME);
}

void ContactBodyParamsFormat::write(const ContactBodyParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeContactParameters(writer, FIRST_PAIR_FIELD, card.params, ContactBodyParamsTable::NAME);
}

std::uint32_t ContactBodyParamsTable::append(const ContactBodyParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactBodyParamsTable::store(std::uint32_t row, const ContactBodyParamsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactBodyParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactBodyParamsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
