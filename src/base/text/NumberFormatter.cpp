#include "oor/base/text/NumberFormatter.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/NumberParser.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>

namespace oor::base
{
namespace
{
std::string realText(double value, int digits)
{
    if (!std::isfinite(value))
        throw UsageError("a nonfinite value cannot be written as a model number");
    digits = std::min(digits, std::numeric_limits<double>::max_digits10);
    char buffer[64];
    const auto result = digits > 0
                            ? std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general, digits)
                            : std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general);
    std::string text(buffer, result.ptr);
    if (text.find_first_of(".eE") == std::string::npos)
        text.push_back('.');
    return text;
}
} // namespace
std::string NumberFormatter::formatInteger(std::int64_t value)
{
    return std::to_string(value);
}
FormattedReal NumberFormatter::formatRealWithDigits(double value, int digits)
{
    std::string text = realText(value, digits);
    const auto parsed = NumberParser::parseReal(text);
    return {std::move(text), parsed && *parsed == value};
}
std::string NumberFormatter::formatRealFree(double value)
{
    return realText(value, 0);
}
} // namespace oor::base
