#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include "oor/base/text/NumberFormatter.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/core/CardRegistry.hpp"

#include <algorithm>
#include <cmath>

namespace oor::model
{

namespace
{

[[nodiscard]] bool accepts(const SourcedValueLayout& layout, ValueSource source) noexcept
{
    return (layout.allowed & sourceBit(source)) != 0;
}

// "CONSTANT or TABLE": the words of the sources a layout accepts, for a message.
std::string acceptedSources(const SourcedValueLayout& layout)
{
    std::string phrase;
    for (const ValueSource source : {ValueSource::Element, ValueSource::Constant, ValueSource::Table})
    {
        if (!accepts(layout, source))
            continue;
        if (!phrase.empty())
            phrase += " or ";
        phrase += keywordText(VALUE_SOURCE_WORDS, source);
    }
    return phrase;
}

} // namespace

std::optional<CardProblem> checkRequiredId(base::EntityId id, const char* field)
{
    if (id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", field};
    return std::nullopt;
}

std::optional<CardProblem> checkOptionalId(base::EntityId id, const char* field)
{
    if (id < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer, or blank",
                           field};
    return std::nullopt;
}

std::optional<CardProblem> checkReal(double value, const char* field, RealRule rule)
{
    if (!std::isfinite(value))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", field};
    if (rule == RealRule::Positive && !(value > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero", field};
    if (rule == RealRule::NonNegative && value < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", field};
    return std::nullopt;
}

std::optional<CardProblem> checkOptionalReal(const std::optional<double>& value, const char* field, RealRule rule)
{
    if (!value)
        return std::nullopt;
    return checkReal(*value, field, rule);
}

std::optional<CardProblem> checkRealRange(double value, const char* field, double low, double high)
{
    if (!std::isfinite(value) || value < low || value > high)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be between " +
                               base::NumberFormatter::formatRealFree(low) + " and " +
                               base::NumberFormatter::formatRealFree(high),
                           field};
    return std::nullopt;
}

std::string readCardTypeWord(FieldReader& reader, int field)
{
    const std::string_view text = reader.text(field);
    if (CardRegistry::instance().typeIndexOf(text))
        return std::string(text);
    return std::string(text);
}

namespace
{

// A neutral name of the format: a letter first, then letters and digits, at least one of them lower case.
[[nodiscard]] bool isNeutralWord(std::string_view word) noexcept
{
    const auto isLower = [](char character) { return character >= 'a' && character <= 'z'; };
    const auto isUpper = [](char character) { return character >= 'A' && character <= 'Z'; };
    const auto isDigit = [](char character) { return character >= '0' && character <= '9'; };
    if (word.empty() || !(isUpper(word.front()) || isLower(word.front())))
        return false;
    bool lower = false;
    for (const char character : word)
    {
        if (!isLower(character) && !isUpper(character) && !isDigit(character))
            return false;
        lower = lower || isLower(character);
    }
    return lower;
}

} // namespace

std::optional<CardProblem> checkWordOrName(std::string_view word, const char* field)
{
    if (isNeutralWord(word))
        return std::nullopt;
    return checkNameWord(word, field);
}

std::string routineNameOf(std::string_view text, std::string_view defaultName)
{
    if (text.empty() || text == defaultName)
        return std::string(defaultName);
    return std::string(text);
}

std::optional<CardProblem> checkRoutineName(std::string_view name, std::string_view defaultName, const char* field)
{
    if (name == defaultName)
        return std::nullopt;
    return checkNameWord(name, field);
}

std::optional<CardProblem> checkCardTypeWord(std::string_view word, const char* field)
{
    const bool neutral =
        std::any_of(word.begin(), word.end(), [](char character) { return character >= 'a' && character <= 'z'; });
    if (neutral)
        return std::nullopt;
    return checkNameWord(word, field);
}

std::optional<CardProblem> checkNameWord(std::string_view word, const char* field)
{
    if (word.empty())
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be blank", field};
    for (const char character : word)
        if (static_cast<unsigned char>(character) < 0x20 || character == 0x7F)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must not contain control characters", field};
    return std::nullopt;
}

SourcedValue readSourcedValue(FieldReader& reader, const SourcedValueLayout& layout)
{
    SourcedValue value;
    value.source = readKeyword(reader, layout.sourceField, layout.sourceName, VALUE_SOURCE_WORDS, layout.fallback);
    if (!accepts(layout, value.source))
    {
        reader.error("card.bad-value",
                     std::string(layout.sourceName) + " must be " + acceptedSources(layout) + ", not \"" +
                         std::string(reader.text(layout.sourceField)) + "\"",
                     layout.sourceName);
        return value;
    }
    switch (value.source)
    {
    case ValueSource::Element:
        if (!reader.isBlank(layout.valueField))
            reader.error("card.bad-value",
                         std::string(layout.valueName) + " must be blank when " + layout.sourceName + " is Element",
                         layout.valueName);
        break;
    case ValueSource::Constant:
        value.constant = reader.real(layout.valueField, layout.valueName, layout.defaultConstant);
        break;
    case ValueSource::Table:
        value.table = reader.requiredInteger(layout.valueField, layout.valueName);
        break;
    }
    return value;
}

void writeSourcedValue(FieldWriter& writer, const SourcedValueLayout& layout, const SourcedValue& value)
{
    writer.textUnlessDefault(layout.sourceField, keywordText(VALUE_SOURCE_WORDS, value.source),
                             keywordText(VALUE_SOURCE_WORDS, layout.fallback));
    switch (value.source)
    {
    case ValueSource::Element:
        break;
    case ValueSource::Constant:
        writer.realUnlessDefault(layout.valueField, value.constant, layout.defaultConstant);
        break;
    case ValueSource::Table:
        writer.integer(layout.valueField, value.table);
        break;
    }
}

std::optional<CardProblem> checkSourcedValue(const SourcedValue& value, const SourcedValueLayout& layout, RealRule rule)
{
    if (!accepts(layout, value.source))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be " + acceptedSources(layout),
                           layout.sourceName};
    if (value.source == ValueSource::Constant)
        return checkReal(value.constant, layout.valueName, rule);
    if (value.source == ValueSource::Table)
        return checkRequiredId(value.table, layout.valueName);
    return std::nullopt;
}

} // namespace oor::model
