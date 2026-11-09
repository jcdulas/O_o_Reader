#include "oor/model/cards/nonlinear_contact/brakeProp/BrakePropTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

// The first pair is in field 3 and each pair takes two fields, so the name of pair `index` is in field 3 + 2 * index
// (the continuation lines start at field 9, an odd field: the pairs never straddle a line).
constexpr int FIRST_PAIR_FIELD = 3;
constexpr int FIELDS_PER_PAIR = 2;

} // namespace

std::optional<BrakePropCard> BrakePropFormat::read(FieldReader& reader)
{
    BrakePropCard card;
    card.id = reader.requiredInteger(1, "id");
    if (!reader.isBlank(2))
        reader.error("card.bad-value",
                     "field 2 of a " + std::string(reader.cardName()) + " is not used and must be blank", "");
    bool hasAvstif = false;
    bool hasTavstif = false;
    bool hasFrict = false;
    const int last = reader.lastUsedField();
    for (int nameField = FIRST_PAIR_FIELD; nameField <= last && !reader.failed(); nameField += FIELDS_PER_PAIR)
    {
        const int valueField = nameField + 1;
        if (reader.isBlank(nameField) && reader.isBlank(valueField))
            continue; // the padding of a line
        if (reader.isBlank(nameField))
        {
            reader.error("card.missing-field", "a value needs the name of its parameter", "parameterName");
            break;
        }
        const std::string name(reader.text(nameField));
        // A parameter is given once: the card keeps a value, not a list, so a second one would be lost.
        const auto firstTime = [&](bool& given)
        {
            if (given)
                reader.error("card.bad-value", name + " is given twice", "parameterName");
            const bool wasGiven = given;
            given = true;
            return !wasGiven;
        };
        if (name == "AverageStiffnessMethod" && firstTime(hasAvstif))
            card.areaStiffness = reader.optionalReal(valueField, "areaStiffness");
        else if (name == "TemperatureAverageStiffness" && firstTime(hasTavstif))
            card.areaStiffnessTableId = reader.integer(valueField, "areaStiffnessTableId", 0);
        else if (name == "Friction" && firstTime(hasFrict))
            card.friction = reader.real(valueField, "friction", 0.0);
        else if (name != "AverageStiffnessMethod" && name != "TemperatureAverageStiffness" && name != "Friction")
            reader.error("card.bad-value",
                         "\"" + name + "\" is not a parameter of a " + std::string(reader.cardName()) +
                             " (AVSTIF, TAVSTIF or FRICT)",
                         "parameterName");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BrakePropFormat::check(const BrakePropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalReal(card.areaStiffness, "areaStiffness", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalId(card.areaStiffnessTableId, "areaStiffnessTableId"))
        return problem;
    if (const auto problem = checkReal(card.friction, "friction", RealRule::NonNegative))
        return problem;
    // Without a table the stiffness has to be given, or the solver stops with a fatal message.
    if (!card.areaStiffness && card.areaStiffnessTableId == 0)
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " is required when " +
                               CardProblem::fieldNamed("areaStiffnessTableId") + " is 0 (blank)",
                           "areaStiffness"};
    return std::nullopt;
}

void BrakePropFormat::write(const BrakePropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    // The pairs are written one after the other in a fixed order (AVSTIF, TAVSTIF, FRICT); a
    // parameter that is blank or is its default is left out.
    int field = FIRST_PAIR_FIELD;
    if (card.areaStiffness)
    {
        writer.text(field, "AverageStiffnessMethod");
        writer.real(field + 1, *card.areaStiffness);
        field += FIELDS_PER_PAIR;
    }
    if (card.areaStiffnessTableId != 0)
    {
        writer.text(field, "TemperatureAverageStiffness");
        writer.integer(field + 1, card.areaStiffnessTableId);
        field += FIELDS_PER_PAIR;
    }
    if (card.friction != 0.0 || !writer.omitsDefaults())
    {
        writer.text(field, "Friction");
        writer.real(field + 1, card.friction);
    }
}

std::uint32_t BrakePropTable::append(const BrakePropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void BrakePropTable::store(std::uint32_t row, const BrakePropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void BrakePropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void BrakePropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void BrakePropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "areaStiffnessTableId", ReferenceTarget::MultiDimensionalTable,
            m_cards.edit(row).areaStiffnessTableId);
}

} // namespace oor::model
