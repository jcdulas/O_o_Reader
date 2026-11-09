#include "oor/model/control/case_control/CaseValueParser.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

// The words of a list value: split on commas and blanks.
std::vector<std::string> splitItems(std::string_view text)
{
    std::vector<std::string> items;
    std::string current;
    for (const char character : text)
    {
        if (character == ',' || base::isAsciiSpace(character))
        {
            if (!current.empty())
                items.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(character);
        }
    }
    if (!current.empty())
        items.push_back(current);
    return items;
}

bool isSingleWord(std::string_view text) noexcept
{
    return !text.empty() && std::none_of(text.begin(), text.end(), [](char character)
                                         { return base::isAsciiSpace(character) || character == ','; });
}

// "AutoContact,PermanentGlue" (blanks allowed around the comma) as one keyword when the command lets the word
// AutoContact take the qualifier PermanentGlue (CommandDefinition::qualifiedWords); nothing otherwise.
std::optional<std::string> qualifiedKeyword(const CommandDefinition& definition, std::string_view text)
{
    const std::size_t comma = text.find(',');
    if (comma == std::string_view::npos)
        return std::nullopt;
    const std::string word(base::trimAscii(text.substr(0, comma)));
    const std::string qualifier(base::trimAscii(text.substr(comma + 1)));
    for (const QualifiedWord& candidate : definition.qualifiedWords)
    {
        if (candidate.word == word && std::find(candidate.qualifiers.begin(), candidate.qualifiers.end(), qualifier) !=
                                          candidate.qualifiers.end())
            return word + "," + qualifier;
    }
    return std::nullopt;
}

ValueProblem wrongKind(std::string message)
{
    return ValueProblem{true, std::move(message)};
}

ValueProblem badValue(std::string message)
{
    return ValueProblem{false, std::move(message)};
}

} // namespace

std::optional<CaseValue> CaseValueParser::parse(const CommandDefinition& definition, std::string_view text,
                                                std::string& problem)
{
    const std::string_view trimmed = base::trimAscii(text);
    switch (definition.shape)
    {
    case CommandShape::Integer:
    {
        const auto value = base::NumberParser::parseInteger(trimmed);
        if (!value)
        {
            problem = trimmed.empty() ? "an integer is missing" : "\"" + std::string(trimmed) + "\" is not an integer";
            return std::nullopt;
        }
        return CaseValue::ofInteger(*value);
    }
    case CommandShape::IntegerOrKeyword:
    {
        if (const auto value = base::NumberParser::parseInteger(trimmed))
            return CaseValue::ofInteger(*value);
        if (definition.takesLoadSequence)
        {
            // format reference SUPER: "SUPER = n, l", the superelement or SET n and the load sequence number l
            const std::vector<std::string> items = splitItems(trimmed);
            const auto number = items.size() == 2 ? base::NumberParser::parseInteger(items[0]) : std::nullopt;
            const auto sequence = items.size() == 2 ? base::NumberParser::parseInteger(items[1]) : std::nullopt;
            if (number && sequence)
            {
                CaseValue value = CaseValue::ofInteger(*number);
                value.integers = {*sequence};
                return value;
            }
        }
        if (const std::optional<std::string> qualified = qualifiedKeyword(definition, trimmed))
            return CaseValue::ofKeyword(*qualified);
        if (!isSingleWord(trimmed))
        {
            problem = trimmed.empty() ? "an integer or a word is missing"
                                      : "\"" + std::string(trimmed) + "\" is not an integer or one word";
            return std::nullopt;
        }
        return CaseValue::ofKeyword(std::string(trimmed));
    }
    case CommandShape::Real:
    {
        const auto value = base::NumberParser::parseReal(trimmed);
        if (!value)
        {
            problem = trimmed.empty() ? "a real number is missing" : "\"" + std::string(trimmed) + "\" is not a real";
            return std::nullopt;
        }
        return CaseValue::ofReal(*value);
    }
    case CommandShape::Keyword:
        if (trimmed.empty())
        {
            problem = "a word is missing";
            return std::nullopt;
        }
        return CaseValue::ofKeyword(std::string(trimmed));
    case CommandShape::Text:
        return CaseValue::ofText(std::string(trimmed));
    case CommandShape::IntegerList:
    {
        std::vector<std::int64_t> values;
        for (const std::string& item : splitItems(trimmed))
        {
            const auto value = base::NumberParser::parseInteger(item);
            if (!value)
            {
                problem = "\"" + item + "\" is not an integer";
                return std::nullopt;
            }
            values.push_back(*value);
        }
        if (values.empty())
        {
            problem = "the list is empty";
            return std::nullopt;
        }
        return CaseValue::ofIntegers(std::move(values));
    }
    case CommandShape::RealList:
    {
        std::vector<double> values;
        for (const std::string& item : splitItems(trimmed))
        {
            const auto value = base::NumberParser::parseReal(item);
            if (!value)
            {
                problem = "\"" + item + "\" is not a real";
                return std::nullopt;
            }
            values.push_back(*value);
        }
        if (values.empty())
        {
            problem = "the list is empty";
            return std::nullopt;
        }
        return CaseValue::ofReals(std::move(values));
    }
    case CommandShape::Output:
        if (trimmed.empty())
            return CaseValue{};
        if (const auto value = base::NumberParser::parseInteger(trimmed))
            return CaseValue::ofInteger(*value);
        return CaseValue::ofKeyword(std::string(trimmed));
    case CommandShape::Flag:
        if (trimmed.empty())
            return CaseValue{};
        return CaseValue::ofText(std::string(trimmed));
    case CommandShape::SubcaseHeader:
    case CommandShape::SetDefinition:
    case CommandShape::Parameter:
    case CommandShape::BlockHeader:
        break;
    }
    problem = "this command has a structure of its own";
    return std::nullopt;
}

std::optional<ValueProblem> CaseValueParser::check(const CommandDefinition& definition, const CaseValue& value)
{
    const std::string name = std::string(definition.name);
    switch (definition.shape)
    {
    case CommandShape::Integer:
        if (value.kind != CaseValueKind::Integer)
            return wrongKind(name + " takes an integer");
        if (value.integer < 0)
            return badValue(name + " cannot be negative");
        return std::nullopt;
    case CommandShape::IntegerOrKeyword:
        if (value.kind != CaseValueKind::Integer && value.kind != CaseValueKind::Keyword)
            return wrongKind(name + " takes an integer or a word (All, None, ...)");
        if (value.kind == CaseValueKind::Integer && value.integer < 0)
            return badValue(name + " cannot be negative");
        if (value.kind == CaseValueKind::Integer && !value.integers.empty())
        {
            if (!definition.takesLoadSequence || value.integers.size() != 1)
                return badValue(name + " takes one integer");
            if (value.integers.front() <= 0)
                return badValue(name + ": the load sequence number must be greater than 0");
        }
        return std::nullopt;
    case CommandShape::Real:
        if (value.kind != CaseValueKind::Real)
            return wrongKind(name + " takes a real number");
        return std::nullopt;
    case CommandShape::Keyword:
        if (value.kind != CaseValueKind::Keyword)
            return wrongKind(name + " takes a word");
        if (value.text.empty())
            return badValue(name + " takes a word, not an empty text");
        if (!definition.keywords.empty() &&
            std::find(definition.keywords.begin(), definition.keywords.end(), value.text) == definition.keywords.end())
        {
            std::string listed;
            for (const std::string& word : definition.keywords)
                listed += (listed.empty() ? "" : ", ") + word;
            return badValue(name + " is " + listed + ", not " + value.text);
        }
        return std::nullopt;
    case CommandShape::Text:
        if (value.kind != CaseValueKind::Text)
            return wrongKind(name + " takes text");
        return std::nullopt;
    case CommandShape::IntegerList:
        if (value.kind != CaseValueKind::IntegerList)
            return wrongKind(name + " takes a list of integers");
        if (value.integers.empty())
            return badValue(name + " takes at least one integer");
        return std::nullopt;
    case CommandShape::RealList:
        if (value.kind != CaseValueKind::RealList)
            return wrongKind(name + " takes a list of real numbers");
        if (value.reals.empty())
            return badValue(name + " takes at least one number");
        return std::nullopt;
    case CommandShape::Output:
        if (value.kind != CaseValueKind::None && value.kind != CaseValueKind::Integer &&
            value.kind != CaseValueKind::Keyword)
            return wrongKind(name + " selects All, None or a set ID");
        if (value.kind == CaseValueKind::Integer && value.integer <= 0)
            return badValue(name + " needs a positive set ID");
        return std::nullopt;
    case CommandShape::Flag:
        if (value.kind != CaseValueKind::None && value.kind != CaseValueKind::Text)
            return wrongKind(name + " takes no value");
        return std::nullopt;
    case CommandShape::SubcaseHeader:
    case CommandShape::SetDefinition:
    case CommandShape::Parameter:
    case CommandShape::BlockHeader:
        break;
    }
    return wrongKind(name + " has a structure of its own");
}

} // namespace oor::model
