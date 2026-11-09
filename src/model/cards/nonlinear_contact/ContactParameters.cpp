#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

const char* kindName(ContactParameterKind kind)
{
    switch (kind)
    {
    case ContactParameterKind::Integer:
        return "an integer";
    case ContactParameterKind::Real:
        return "a real";
    case ContactParameterKind::Number:
        return "an integer or a real";
    case ContactParameterKind::Text:
        return "a word";
    case ContactParameterKind::Any:
        return "an integer, a real or a word";
    }
    return "a value";
}

// One owner as a list of owners.
ParameterOwners single(const std::string_view& owner)
{
    return ParameterOwners(&owner, 1);
}

} // namespace

ContactValue readContactValue(FieldReader& reader, int field, const char* name)
{
    const std::string_view text = reader.text(field);
    if (text.empty())
        return std::monostate{};
    if (const auto integer = base::NumberParser::parseInteger(text))
        return *integer;
    if (const auto real = base::NumberParser::parseReal(text))
        return *real;
    reader.error("card.bad-value",
                 std::string(name) + " must be an integer or a real, found \"" + std::string(text) + "\"", name);
    return std::monostate{};
}

void writeContactValue(FieldWriter& writer, int field, const ContactValue& value)
{
    if (const auto* integer = std::get_if<std::int64_t>(&value))
        writer.integer(field, *integer);
    else if (const auto* real = std::get_if<double>(&value))
        writer.real(field, *real);
    else if (const auto* word = std::get_if<std::string>(&value))
        writer.text(field, *word);
}

bool isContactNumber(const ContactValue& value) noexcept
{
    if (const auto* real = std::get_if<double>(&value))
        return std::isfinite(*real);
    return !std::holds_alternative<std::string>(value);
}

ContactValue readContactParameterValue(FieldReader& reader, int field, const ContactParameterRule& rule,
                                       std::string_view owner)
{
    return readContactParameterValue(reader, field, rule, single(owner));
}

ContactValue readContactParameterValue(FieldReader& reader, int field, const ContactParameterRule& rule,
                                       [[maybe_unused]] ParameterOwners owners)
{
    const std::string name = std::string(rule.name);
    const std::string_view text = reader.text(field);
    if (text.empty())
        return std::monostate{};
    const auto integer = base::NumberParser::parseInteger(text);
    const auto real = integer ? std::nullopt : base::NumberParser::parseReal(text);
    switch (rule.kind)
    {
    case ContactParameterKind::Text:
        if (!integer && !real)
            return std::string(text);
        reader.error("card.bad-value", name + " is a word, found \"" + std::string(text) + "\"", "parameterValue");
        break;
    case ContactParameterKind::Integer:
        if (integer)
            return *integer;
        reader.error("card.bad-value", name + " is an integer, found \"" + std::string(text) + "\"", "parameterValue");
        break;
    case ContactParameterKind::Real:
        if (integer)
            return static_cast<double>(*integer);
        if (real)
            return *real;
        reader.error("card.bad-value", name + " is a real, found \"" + std::string(text) + "\"", "parameterValue");
        break;
    case ContactParameterKind::Number:
        if (integer)
            return *integer;
        if (real)
            return *real;
        reader.error("card.bad-value", name + " is an integer or a real, found \"" + std::string(text) + "\"",
                     "parameterValue");
        break;
    case ContactParameterKind::Any:
        if (integer)
            return *integer;
        if (real)
            return *real;
        return std::string(text);
    }
    return std::monostate{};
}

const ContactParameterRule* findContactParameterRule(std::span<const ContactParameterRule> rules, std::string_view name)
{
    for (const ContactParameterRule& rule : rules)
    {
        if (rule.name == name)
            return &rule;
    }
    return nullptr;
}

std::vector<ContactParameter> readContactParameters(FieldReader& reader, int firstPairField,
                                                    std::span<const ContactParameterRule> rules, std::string_view owner)
{
    return readContactParameters(reader, firstPairField, rules, single(owner));
}

std::vector<ContactParameter> readContactParameters(FieldReader& reader, int firstPairField,
                                                    std::span<const ContactParameterRule> rules,
                                                    [[maybe_unused]] ParameterOwners owners)
{
    std::vector<ContactParameter> parameters;
    const int last = reader.lastUsedField();
    for (std::size_t index = 0;; ++index)
    {
        const int nameField = contactPairField(firstPairField, index);
        if (nameField > last)
            break;
        const int valueField = nameField + 1;
        if (reader.isBlank(nameField) && reader.isBlank(valueField))
            continue;
        if (reader.isBlank(nameField))
        {
            reader.error("card.missing-field", "a value needs the name of its parameter", "parameterName");
            break;
        }
        ContactParameter parameter;
        parameter.name = std::string(reader.text(nameField));
        const ContactParameterRule* rule = findContactParameterRule(rules, parameter.name);
        if (rule == nullptr)
        {
            reader.error("card.bad-value",
                         "\"" + std::string(reader.text(nameField)) + "\" is not a parameter of this card",
                         "parameterName");
            break;
        }
        parameter.value = readContactParameterValue(reader, valueField, *rule, owners);
        parameters.push_back(std::move(parameter));
    }
    return parameters;
}

void writeContactParameters(FieldWriter& writer, int firstPairField, const std::vector<ContactParameter>& parameters,
                            std::string_view owner)
{
    writeContactParameters(writer, firstPairField, parameters, single(owner));
}

void writeContactParameters(FieldWriter& writer, int firstPairField, const std::vector<ContactParameter>& parameters,
                            [[maybe_unused]] ParameterOwners owners)
{
    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        const int nameField = contactPairField(firstPairField, index);
        const ContactParameter& parameter = parameters[index];
        writer.text(nameField, parameter.name);
        if (const auto* integer = std::get_if<std::int64_t>(&parameter.value))
            writer.integer(nameField + 1, *integer);
        else if (const auto* real = std::get_if<double>(&parameter.value))
            writer.real(nameField + 1, *real);
        else if (const auto* word = std::get_if<std::string>(&parameter.value))
            writer.text(nameField + 1, *word);
    }
}

std::optional<CardProblem> checkContactParameters(const std::vector<ContactParameter>& parameters,
                                                  std::span<const ContactParameterRule> rules, std::string_view owner)
{
    return checkContactParameters(parameters, rules, single(owner));
}

std::optional<CardProblem> checkContactParameters(const std::vector<ContactParameter>& parameters,
                                                  std::span<const ContactParameterRule> rules,
                                                  [[maybe_unused]] ParameterOwners owners)
{
    for (const ContactParameter& parameter : parameters)
    {
        const std::string label = std::string(parameter.name);
        const ContactParameterRule* rule = findContactParameterRule(rules, parameter.name);
        if (rule == nullptr)
            return CardProblem{"card.bad-value", "\"" + label + "\" is not a parameter of this card", "params"};
        const ContactValue& value = parameter.value;
        const bool blank = std::holds_alternative<std::monostate>(value);
        const bool integer = std::holds_alternative<std::int64_t>(value);
        const bool real = std::holds_alternative<double>(value);
        const bool word = std::holds_alternative<std::string>(value);
        bool fits = blank;
        switch (rule->kind)
        {
        case ContactParameterKind::Integer:
            fits = fits || integer;
            break;
        case ContactParameterKind::Real:
            fits = fits || real;
            break;
        case ContactParameterKind::Number:
            fits = fits || integer || real;
            break;
        case ContactParameterKind::Text:
            fits = fits || word;
            break;
        case ContactParameterKind::Any:
            fits = true;
            break;
        }
        if (!fits)
            return CardProblem{"card.bad-value", label + " must be " + kindName(rule->kind), "params"};
        if (real && !std::isfinite(std::get<double>(value)))
            return CardProblem{"card.bad-value", label + " must be a finite number", "params"};
    }
    return std::nullopt;
}

} // namespace oor::model
