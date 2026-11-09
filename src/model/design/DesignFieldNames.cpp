#include "oor/model/design/DesignFieldNames.hpp"

#include "oor/model/core/CardProblem.hpp"

namespace oor::model
{

namespace
{

bool isDigit(char character)
{
    return character >= '0' && character <= '9';
}

// The value of a run of digits, or nullopt when it is empty or too long to be a station or a ply number.
std::optional<int> numberOf(std::string_view digits)
{
    if (digits.empty() || digits.size() > 6)
        return std::nullopt;
    int value = 0;
    for (const char digit : digits)
    {
        if (!isDigit(digit))
            return std::nullopt;
        value = value * 10 + (digit - '0');
    }
    return value;
}

} // namespace

NumberedName splitTrailingNumber(std::string_view name)
{
    NumberedName result;
    result.word = std::string(name);
    const std::size_t open = name.find('(');
    if (open != std::string_view::npos && name.back() == ')')
    {
        const auto number = numberOf(name.substr(open + 1, name.size() - open - 2));
        if (number && *number > 0)
        {
            result.word = std::string(name.substr(0, open));
            result.number = *number;
        }
    }
    return result;
}

std::optional<StationName> splitStation(std::string_view name)
{
    StationName result;
    const std::size_t open = name.rfind('(');
    if (open == std::string_view::npos)
    {
        if (name.find(')') != std::string_view::npos)
            return std::nullopt;
        result.word = std::string(name);
        return result;
    }
    if (open == 0 || name.back() != ')')
        return std::nullopt;
    const std::string_view station = name.substr(open + 1, name.size() - open - 2);
    result.word = std::string(name.substr(0, open));
    result.hasStation = true;
    if (station == "EndA")
    {
        result.atEndA = true;
        return result;
    }
    if (station == "EndB")
    {
        result.atEndB = true;
        return result;
    }
    const std::optional<int> number = numberOf(station);
    if (!number || *number < 1)
        return std::nullopt;
    result.intermediate = *number;
    return result;
}

bool holdsDefault(double stored, double expected) noexcept
{
    if (expected == 0.0)
        return stored == 0.0;
    const double difference = stored > expected ? stored - expected : expected - stored;
    const double scale = expected > 0.0 ? expected : -expected;
    return difference <= 1.0e-12 * scale;
}

std::string refusalOfField(std::string_view cardType, const DesignField& field, std::string_view designable)
{
    std::string text = CardProblem::cardNamed(cardType);
    text += " has no field named ";
    text += describeDesignField(field);
    text += " that a design relation can drive (the ones it can: ";
    text += designable;
    text += ")";
    return text;
}

} // namespace oor::model
