#include "oor/model/cards/elements/connectorSearchControl/ConnectorSearchControlTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/elements/connectorSearchParams/ConnectorSearchParamsCatalogue.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cctype>
#include <cmath>
#include <string>
#include <utility>

namespace oor::model
{

namespace
{

// The first pair is in field 3 (field 2 is blank) and each pair takes two fields, so the name of pair `index` is in
// field 3 + 2 * index: the continuation lines start at field 9, an odd field, and no pair straddles a line.
constexpr int FIRST_PAIR_FIELD = 3;
constexpr int FIELDS_PER_PAIR = 2;

} // namespace

std::optional<ConnectorSearchControlCard> ConnectorSearchControlFormat::read(FieldReader& reader)
{
    ConnectorSearchControlCard card;
    card.elementSetId = reader.requiredInteger(1, "elementSetId");
    if (!reader.isBlank(2))
        reader.error("card.bad-value",
                     "field 2 of a " + std::string(reader.cardName()) + " is not used and must be blank", "");
    int pairNumber = 0;
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
        ++pairNumber;
        const std::string valueName = "parameterValue" + std::to_string(pairNumber);
        ConnectorSearchParamsParameter parameter;
        const std::string word = std::string(reader.text(nameField));
        parameter.name = std::string(word);
        if (reader.isBlank(valueField))
        {
            reader.error("card.missing-field", "the parameter " + word + " needs its value", valueName.c_str());
            break;
        }
        if (const auto number = reader.integerOrReal(valueField, valueName.c_str()))
        {
            parameter.value.number = *number;
            card.parameters.push_back(std::move(parameter));
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ConnectorSearchControlFormat::check(const ConnectorSearchControlCard& card)
{
    if (const auto problem = checkRequiredId(card.elementSetId, "elementSetId"))
        return problem;
    if (card.parameters.empty())
        return CardProblem{"card.missing-field", "the card needs at least one parameter and its value",
                           "parameterName1"};
    for (const ConnectorSearchParamsParameter& parameter : card.parameters)
    {
        // A name is checked as the deck writes it: the keyword of a neutral name, or the word as it was read.
        const std::string label = std::string(parameter.name);
        if (const auto problem = checkNameWord(label, "parameterName"))
            return problem;
        // A name that starts with anything but a letter is a value that lost its name (a field out of place).
        if (std::isalpha(static_cast<unsigned char>(label.front())) == 0)
            return CardProblem{"card.bad-value", "the parameter name " + label + " must start with a letter",
                               "parameterName"};
        if (!parameter.value.isInteger() && !std::isfinite(std::get<double>(parameter.value.number)))
            return CardProblem{"card.bad-value", "the value of " + label + " must be a finite number",
                               "parameterValue"};
        const ConnectorSearchParamsInfo* info = findConnectorSearchParam(parameter.name);
        if (info == nullptr)
            continue; // a name the table does not list is kept as it is
        if (const std::string problem = connectorSearchParamsValueProblem(*info, parameter.value); !problem.empty())
            return CardProblem{"card.bad-value", problem, "parameterValue"};
    }
    return std::nullopt;
}

void ConnectorSearchControlFormat::write(const ConnectorSearchControlCard& card, FieldWriter& writer)
{
    writer.integer(1, card.elementSetId);
    int field = FIRST_PAIR_FIELD;
    for (const ConnectorSearchParamsParameter& parameter : card.parameters)
    {
        writer.text(field, std::string(parameter.name));
        if (parameter.value.isInteger())
            writer.integer(field + 1, std::get<std::int32_t>(parameter.value.number));
        else
            writer.real(field + 1, std::get<double>(parameter.value.number));
        field += FIELDS_PER_PAIR;
    }
}

std::uint32_t ConnectorSearchControlTable::append(const ConnectorSearchControlCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void ConnectorSearchControlTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "elementSetId", ReferenceTarget::Set, m_cards.edit(row).elementSetId);
}

} // namespace oor::model
