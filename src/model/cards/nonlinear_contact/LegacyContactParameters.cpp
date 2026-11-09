#include "oor/model/cards/nonlinear_contact/LegacyContactParameters.hpp"

#include "oor/base/text/NumberFormatter.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

bool isLetter(char character) noexcept
{
    return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
}

bool isNameCharacter(char character) noexcept
{
    return isLetter(character) || (character >= '0' && character <= '9') || character == '_';
}

// The number a value holds, as a real; nothing for a blank or a word.
std::optional<double> numberOf(const ContactValue& value)
{
    if (const auto* integer = std::get_if<std::int64_t>(&value))
        return static_cast<double>(*integer);
    if (const auto* real = std::get_if<double>(&value))
        return *real;
    return std::nullopt;
}

std::string limitsText(const LegacyParameterRule& rule)
{
    const auto format = [](double value) { return base::NumberFormatter::formatRealFree(value); };
    if (std::isfinite(rule.low) && std::isfinite(rule.high))
        return "between " + format(rule.low) + " and " + format(rule.high);
    if (std::isfinite(rule.low))
        return "at least " + format(rule.low);
    return "at most " + format(rule.high);
}

} // namespace

bool isLegacyParameterName(std::string_view name)
{
    if (name.empty() || !isLetter(name.front()))
        return false;
    for (const char character : name)
    {
        if (!isNameCharacter(character))
            return false;
    }
    return true;
}

LegacyParameterRule findLegacyRule(std::span<const LegacyParameterRule> rules, std::string_view name)
{
    for (const LegacyParameterRule& rule : rules)
    {
        if (rule.name == name)
            return rule;
    }
    return LegacyParameterRule{name, ContactParameterKind::Number, false, LEGACY_NO_LOW_LIMIT, LEGACY_NO_HIGH_LIMIT};
}

std::vector<ContactParameter> readLegacyContactParameters(FieldReader& reader, int firstPairField,
                                                          std::span<const LegacyParameterRule> rules,
                                                          std::string_view owner)
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
            continue; // the padding of a line
        if (reader.isBlank(nameField))
        {
            reader.error("card.missing-field", "a value needs the name of its parameter", "parameterName");
            break;
        }
        const std::string word = std::string(reader.text(nameField));
        if (!isLegacyParameterName(word))
        {
            reader.error("card.bad-value", "\"" + word + "\" is not the name of a parameter", "parameterName");
            break;
        }
        ContactParameter parameter;
        parameter.name = std::string(word);
        const LegacyParameterRule rule = findLegacyRule(rules, parameter.name);
        parameter.value =
            readContactParameterValue(reader, valueField, ContactParameterRule{rule.name, rule.kind}, owner);
        if (std::holds_alternative<std::monostate>(parameter.value) && !rule.blankAllowed)
            reader.error("card.missing-field", word + " needs a value", "parameterValue");
        parameters.push_back(std::move(parameter));
    }
    return parameters;
}

std::optional<CardProblem> checkLegacyContactParameters(const std::vector<ContactParameter>& parameters,
                                                        std::span<const LegacyParameterRule> rules, std::string_view)
{
    for (const ContactParameter& parameter : parameters)
    {
        // A name is checked as the deck writes it: the keyword of a neutral name, or the word as it was read.
        const std::string label = std::string(parameter.name);
        if (!isLegacyParameterName(label))
            return CardProblem{"card.bad-value", "\"" + label + "\" is not the name of a parameter", "params"};
        const LegacyParameterRule rule = findLegacyRule(rules, parameter.name);
        const ContactValue& value = parameter.value;
        if (std::holds_alternative<std::monostate>(value))
        {
            if (!rule.blankAllowed)
                return CardProblem{"card.missing-field", label + " needs a value", "params"};
            continue;
        }
        const bool integer = std::holds_alternative<std::int64_t>(value);
        const bool real = std::holds_alternative<double>(value);
        switch (rule.kind)
        {
        case ContactParameterKind::Integer:
            if (!integer)
                return CardProblem{"card.bad-value", label + " must be an integer", "params"};
            break;
        case ContactParameterKind::Real:
            if (!real)
                return CardProblem{"card.bad-value", label + " must be a real", "params"};
            break;
        default:
            if (!integer && !real)
                return CardProblem{"card.bad-value", label + " must be an integer or a real", "params"};
            break;
        }
        const double number = *numberOf(value);
        if (!std::isfinite(number))
            return CardProblem{"card.bad-value", label + " must be a finite number", "params"};
        if (number < rule.low || number > rule.high)
            return CardProblem{"card.bad-value", label + " must be " + limitsText(rule), "params"};
    }
    return std::nullopt;
}

} // namespace oor::model
