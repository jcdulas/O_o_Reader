#include "oor/model/cards/solution_control/modelParams/ModelParamsTable.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <cmath>
#include <limits>
#include <set>

namespace oor::model
{

namespace
{

// A name of a parameter: a letter, then letters, digits and underscores (at most eight characters).
bool isParameterName(std::string_view text)
{
    if (text.empty() || !((text[0] >= 'A' && text[0] <= 'Z') || (text[0] >= 'a' && text[0] <= 'z')))
        return false;
    for (const char character : text)
    {
        const bool letter = (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
        if (!letter && !(character >= '0' && character <= '9') && character != '_')
            return false;
    }
    return true;
}

} // namespace

std::optional<ModelParamsCard> ModelParamsFormat::read(FieldReader& reader)
{
    ModelParamsCard card;
    const int last = reader.lastUsedField();
    for (int field = 1; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue; // padding: a pair that is not there
        ModelParameter parameter;
        const std::string_view name = reader.text(field);
        if (name.empty())
        {
            reader.error("card.missing-field",
                         "a value of an " + std::string(reader.cardName()) + " has no name before it",
                         "parameterNames");
            break;
        }
        if (!isParameterName(name))
        {
            reader.error("card.bad-value", "\"" + std::string(name) + "\" is not the name of a parameter",
                         "parameterNames");
            break;
        }
        parameter.name = std::string(name);
        const std::string_view value = reader.text(field + 1);
        if (value.empty())
        {
            reader.error("card.missing-field", "the parameter " + std::string(name) + " has no value",
                         "parameterValues");
            break;
        }
        if (const auto integer = base::NumberParser::parseInteger(value))
        {
            if (*integer < std::numeric_limits<std::int32_t>::min() ||
                *integer > std::numeric_limits<std::int32_t>::max())
            {
                reader.error("card.bad-value", "the integer value does not fit in 32 bits", "parameterValues");
                break;
            }
            parameter.kind = ModelParameterKind::Integer;
            parameter.integerValue = static_cast<std::int32_t>(*integer);
        }
        else if (const auto real = base::NumberParser::parseReal(value))
        {
            parameter.kind = ModelParameterKind::Real;
            parameter.realValue = *real;
        }
        else
        {
            parameter.kind = ModelParameterKind::Text;
            parameter.textValue = std::string(value); // a word is a neutral name (LoadRecoveryOffsets)
        }
        card.parameters.push_back(std::move(parameter));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModelParamsFormat::check(const ModelParamsCard& card)
{
    if (card.parameters.empty())
        return CardProblem{"card.missing-field", "the card needs at least one parameter and its value",
                           "parameterNames"};
    std::set<std::string> seen;
    for (const ModelParameter& parameter : card.parameters)
    {
        // A name is checked as the deck writes it: the keyword of a neutral name, or the word as it was read.
        const std::string label = std::string(parameter.name);
        if (!isParameterName(label))
            return CardProblem{"card.bad-value", "\"" + label + "\" is not the name of a parameter", "parameterNames"};
        if (!seen.insert(label).second)
            return CardProblem{"card.bad-value", "the parameter " + label + " is given twice", "parameterNames"};
        if (parameter.kind == ModelParameterKind::Real && !std::isfinite(parameter.realValue))
            return CardProblem{"card.bad-value", "the value of " + label + " must be a finite number",
                               "parameterValues"};
        if (parameter.kind == ModelParameterKind::Text && parameter.textValue.empty())
            return CardProblem{"card.bad-value", "the value of " + label + " is empty", "parameterValues"};
    }
    return std::nullopt;
}

void ModelParamsFormat::write(const ModelParamsCard& card, FieldWriter& writer)
{
    int field = 1;
    for (const ModelParameter& parameter : card.parameters)
    {
        writer.text(field, std::string(parameter.name));
        switch (parameter.kind)
        {
        case ModelParameterKind::Integer:
            writer.integer(field + 1, parameter.integerValue);
            break;
        case ModelParameterKind::Real:
            writer.real(field + 1, parameter.realValue);
            break;
        case ModelParameterKind::Text:
            writer.text(field + 1, parameter.textValue);
            break;
        }
        field += 2;
    }
}

std::uint32_t ModelParamsTable::append(const ModelParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    std::vector<std::string> names;
    std::vector<std::uint8_t> kinds;
    std::vector<std::int32_t> integers;
    std::vector<double> reals;
    std::vector<std::string> texts;
    for (const ModelParameter& parameter : card.parameters)
    {
        names.push_back(parameter.name);
        kinds.push_back(static_cast<std::uint8_t>(parameter.kind));
        integers.push_back(parameter.integerValue);
        reals.push_back(parameter.realValue);
        texts.push_back(parameter.textValue);
    }
    m_name.push(names);
    m_kind.push(kinds);
    m_integer.push(integers);
    m_real.push(reals);
    m_text.push(texts);
    return row;
}

ModelParamsCard ModelParamsTable::card(std::uint32_t row) const
{
    ModelParamsCard card;
    const auto names = m_name.get(row);
    const auto kinds = m_kind.get(row);
    const auto integers = m_integer.get(row);
    const auto reals = m_real.get(row);
    const auto texts = m_text.get(row);
    for (std::size_t index = 0; index < names.size(); ++index)
    {
        ModelParameter parameter;
        parameter.name = names[index];
        parameter.kind = static_cast<ModelParameterKind>(kinds[index]);
        parameter.integerValue = integers[index];
        parameter.realValue = reals[index];
        parameter.textValue = texts[index];
        card.parameters.push_back(std::move(parameter));
    }
    return card;
}

void ModelParamsTable::store(std::uint32_t row, const ModelParamsCard& card)
{
    std::vector<std::string> names;
    std::vector<std::uint8_t> kinds;
    std::vector<std::int32_t> integers;
    std::vector<double> reals;
    std::vector<std::string> texts;
    for (const ModelParameter& parameter : card.parameters)
    {
        names.push_back(parameter.name);
        kinds.push_back(static_cast<std::uint8_t>(parameter.kind));
        integers.push_back(parameter.integerValue);
        reals.push_back(parameter.realValue);
        texts.push_back(parameter.textValue);
    }
    m_name.set(row, names);
    m_kind.set(row, kinds);
    m_integer.set(row, integers);
    m_real.set(row, reals);
    m_text.set(row, texts);
}

void ModelParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_kind.keepRows(rows);
    m_integer.keepRows(rows);
    m_real.keepRows(rows);
    m_text.keepRows(rows);
}

} // namespace oor::model
