#include "oor/model/cards/optimisation/DesignField.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <limits>

namespace oor::model
{

namespace
{

bool isLetter(char character) noexcept
{
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
}

} // namespace

DesignField readDesignField(FieldReader& reader, int field, const char* fieldName)
{
    DesignField value;
    const std::string_view text = reader.text(field);
    if (text.empty())
    {
        reader.error("card.missing-field", std::string(fieldName) + " is required", fieldName);
        return value;
    }
    value.name = std::string(text);
    reader.report(checkDesignField(value, fieldName));
    return value;
}

void writeDesignField(FieldWriter& writer, int field, const DesignField& value)
{
    writer.text(field, value.name);
}

std::optional<CardProblem> checkDesignField(const DesignField& value, const char* fieldName)
{
    if (value.name.empty())
    {
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a neutral field name",
                           fieldName};
    }
    return checkDesignFieldName(value.name, fieldName);
}

std::optional<CardProblem> checkDesignFieldName(const std::string& name, const char* fieldName)
{
    const auto problem = [&]() -> std::optional<CardProblem>
    {
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) +
                " must be a lowerCamelCase field name, optionally followed by a positive index or EndA/EndB selector",
            fieldName};
    };
    if (name.empty() || name.front() < 'a' || name.front() > 'z')
        return problem();
    std::size_t next = 1;
    while (next < name.size() && (isLetter(name[next]) || (name[next] >= '0' && name[next] <= '9')))
        ++next;
    int selectors = 0;
    while (next < name.size())
    {
        if (name[next] != '(' || ++selectors > 2)
            return problem();
        const std::size_t close = name.find(')', next + 1);
        if (close == std::string::npos)
            return problem();
        const std::string_view selector(name.data() + next + 1, close - next - 1);
        if (selector != "EndA" && selector != "EndB")
        {
            const auto index = base::NumberParser::parseInteger(selector);
            if (!index || *index < 1 || selector.find_first_not_of("0123456789") != std::string_view::npos)
                return problem();
        }
        next = close + 1;
    }
    return std::nullopt;
}

std::string describeDesignField(const DesignField& value)
{
    return value.name;
}

} // namespace oor::model
