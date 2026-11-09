#include "oor/model/cards/solution_control/nonlinearHarmonicSolverParams/NonlinearHarmonicSolverParamsTable.hpp"

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/nonlinearHarmonicSolverParams/NonlinearHarmonicSolverParamsCatalogue.hpp"

#include <cctype>
#include <cmath>
#include <utility>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_PAIR = 2;

// The value of a pair as the parameter takes it: a listed parameter has the kind of its entry (a real parameter takes
// an integer as a real that is whole); a name the table does not list is an integer, a real or else a word. A value
// that is not of the kind is reported.
std::optional<NonlinearHarmonicSolverParamsValue> parseValue(FieldReader& reader,
                                                             const NonlinearHarmonicSolverParamsInfo* info, int field)
{
    const std::string_view text = reader.text(field);
    const auto integer = base::NumberParser::parseInteger(text);
    const auto number = base::NumberParser::parseReal(text);
    const std::optional<std::int32_t> id = integer ? base::toEntityId(*integer) : std::nullopt;
    if (info == nullptr)
    {
        if (id)
            return NonlinearHarmonicSolverParamsValue(*id);
        if (number)
            return NonlinearHarmonicSolverParamsValue(*number);
        return NonlinearHarmonicSolverParamsValue(std::string(text));
    }
    switch (info->kind)
    {
    case NonlinearHarmonicSolverParamsKind::Word:
        return NonlinearHarmonicSolverParamsValue(std::string(text));
    case NonlinearHarmonicSolverParamsKind::Real:
        if (number)
            return NonlinearHarmonicSolverParamsValue(*number);
        break;
    case NonlinearHarmonicSolverParamsKind::Integer:
        if (id)
            return NonlinearHarmonicSolverParamsValue(*id);
        break;
    }
    reader.error("card.bad-value",
                 std::string(info->name) + " must be " + nonlinearHarmonicSolverParamsValuesText(*info) + ", not \"" +
                     std::string(text) + "\"",
                 "value");
    return std::nullopt;
}

} // namespace

std::optional<NonlinearHarmonicSolverParamsCard> NonlinearHarmonicSolverParamsFormat::read(FieldReader& reader)
{
    NonlinearHarmonicSolverParamsCard card;
    const int last = reader.lastUsedField();
    // The pairs sit side by side from field 1. A pair whose two fields are blank is padding.
    for (int field = 1; field <= last && !reader.failed(); field += FIELDS_PER_PAIR)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        if (reader.isBlank(field))
        {
            reader.error("card.missing-field", "a value needs the name of its parameter in the field before it",
                         "parameterName");
            break;
        }
        NonlinearHarmonicSolverParamsParameter parameter;
        const std::string word = std::string(reader.text(field));
        parameter.name = std::string(word);
        if (reader.isBlank(field + 1))
        {
            reader.error("card.missing-field", "the parameter " + word + " needs its value in the next field", "value");
            break;
        }
        const std::optional<NonlinearHarmonicSolverParamsValue> value =
            parseValue(reader, findNonlinearHarmonicSolverParam(parameter.name), field + 1);
        if (!value)
            break;
        parameter.value = *value;
        card.parameters.push_back(std::move(parameter));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearHarmonicSolverParamsFormat::check(const NonlinearHarmonicSolverParamsCard& card)
{
    for (const NonlinearHarmonicSolverParamsParameter& parameter : card.parameters)
    {
        // A name is checked as the deck writes it: the keyword of a neutral name, or the word as it was read.
        const std::string label = std::string(parameter.name);
        if (const auto problem = checkNameWord(label, "parameterName"))
            return problem;
        // A name that starts with anything but a letter is a value that lost its name (a field out of place).
        if (std::isalpha(static_cast<unsigned char>(label.front())) == 0)
            return CardProblem{"card.bad-value", "the parameter name " + label + " must start with a letter",
                               "parameterName"};
        if (const NonlinearHarmonicSolverParamsInfo* info = findNonlinearHarmonicSolverParam(parameter.name))
        {
            if (const std::string problem = nonlinearHarmonicSolverParamsValueProblem(*info, parameter.value);
                !problem.empty())
                return CardProblem{"card.bad-value", problem, "value"};
            continue;
        }
        // A name the table does not list is kept: its value must still read back.
        if (const auto* number = std::get_if<double>(&parameter.value); number != nullptr && !std::isfinite(*number))
            return CardProblem{"card.bad-value", "the value of " + label + " must be a finite number", "value"};
        if (const auto* word = std::get_if<std::string>(&parameter.value))
        {
            if (const auto problem = checkNameWord(*word, "value"))
                return problem;
        }
    }
    return std::nullopt;
}

void NonlinearHarmonicSolverParamsFormat::write(const NonlinearHarmonicSolverParamsCard& card, FieldWriter& writer)
{
    int field = 1;
    for (const NonlinearHarmonicSolverParamsParameter& parameter : card.parameters)
    {
        writer.text(field, std::string(parameter.name));
        if (const auto* integer = std::get_if<std::int32_t>(&parameter.value))
            writer.integer(field + 1, *integer);
        else if (const auto* number = std::get_if<double>(&parameter.value))
            writer.real(field + 1, *number);
        else
            writer.text(field + 1, std::get<std::string>(parameter.value));
        field += FIELDS_PER_PAIR;
    }
}

std::uint32_t NonlinearHarmonicSolverParamsTable::append(const NonlinearHarmonicSolverParamsCard& card,
                                                         base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

} // namespace oor::model
