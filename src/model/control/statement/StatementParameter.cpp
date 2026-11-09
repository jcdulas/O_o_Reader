#include "oor/model/control/statement/StatementParameter.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

// A value that reads back as one value without quotes.
bool needsQuotes(std::string_view value) noexcept
{
    if (value.empty())
        return false;
    if (value.front() == '(' && value.back() == ')')
        return false;
    return value.find_first_of(" \t,='\"") != std::string_view::npos;
}

} // namespace

std::string StatementParameter::format() const
{
    std::string text = name;
    if (hasOptions)
        text += "(" + options + ")";
    if (hasValue)
    {
        if (!name.empty() || hasOptions)
            text.push_back('=');
        if (quote != '\0')
            text.push_back(quote);
        text += value;
        if (quote != '\0')
            text.push_back(quote);
    }
    return text;
}

bool StatementParameter::isNamed(std::string_view word) const noexcept
{
    return !name.empty() && name == word;
}

StatementParameter StatementParameter::flag(std::string name)
{
    StatementParameter parameter;
    parameter.name = std::move(name);
    return parameter;
}

StatementParameter StatementParameter::withValue(std::string name, std::string value)
{
    StatementParameter parameter;
    parameter.name = std::move(name);
    parameter.hasValue = true;
    if (needsQuotes(value))
        parameter.quote = value.find('\'') == std::string::npos ? '\'' : '"';
    parameter.value = std::move(value);
    return parameter;
}

StatementParameter StatementParameter::withQuotedValue(std::string name, std::string value)
{
    StatementParameter parameter;
    parameter.name = std::move(name);
    parameter.hasValue = true;
    parameter.quote = value.find('\'') == std::string::npos ? '\'' : '"';
    parameter.value = std::move(value);
    return parameter;
}

const StatementParameter* findParameter(const std::vector<StatementParameter>& parameters,
                                        std::string_view word) noexcept
{
    for (const StatementParameter& parameter : parameters)
    {
        if (parameter.isNamed(word))
            return &parameter;
    }
    return nullptr;
}

StatementParameter* findParameter(std::vector<StatementParameter>& parameters, std::string_view word) noexcept
{
    for (StatementParameter& parameter : parameters)
    {
        if (parameter.isNamed(word))
            return &parameter;
    }
    return nullptr;
}

} // namespace oor::model
