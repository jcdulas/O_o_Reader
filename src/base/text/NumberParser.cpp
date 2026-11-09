#include "oor/base/text/NumberParser.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>

namespace oor::base
{

namespace
{

constexpr bool isDigit(char character) noexcept
{
    return character >= '0' && character <= '9';
}

} // namespace

std::optional<std::int64_t> NumberParser::parseInteger(std::string_view text) noexcept
{
    text = trimAscii(text);
    if (text.empty())
        return std::nullopt;
    bool negative = false;
    std::size_t position = 0;
    if (text[0] == '+' || text[0] == '-')
    {
        negative = text[0] == '-';
        position = 1;
    }
    if (position == text.size())
        return std::nullopt;

    // Accumulate as a negative number so that INT64_MIN is representable.
    std::int64_t accumulated = 0;
    for (; position < text.size(); ++position)
    {
        if (!isDigit(text[position]))
            return std::nullopt;
        const int digit = text[position] - '0';
        if (accumulated < (std::numeric_limits<std::int64_t>::min() + digit) / 10)
            return std::nullopt;
        accumulated = accumulated * 10 - digit;
    }
    if (!negative)
    {
        if (accumulated == std::numeric_limits<std::int64_t>::min())
            return std::nullopt;
        return -accumulated;
    }
    return accumulated;
}

std::optional<double> NumberParser::parseReal(std::string_view text) noexcept
{
    text = trimAscii(text);
    std::size_t position = 0;
    const std::size_t size = text.size();
    if (size == 0)
        return std::nullopt;

    // Normalised decimal text handed to std::from_chars. A bounded buffer avoids allocation for malformed input.
    std::array<char, 64> normalised{};
    std::size_t length = 0;
    if (size > 60)
        return std::nullopt;

    if (text[position] == '+' || text[position] == '-')
    {
        if (text[position] == '-')
            normalised[length++] = '-';
        ++position;
    }

    std::size_t mantissaDigits = 0;
    bool sawPoint = false;
    for (; position < size; ++position)
    {
        const char character = text[position];
        if (isDigit(character))
        {
            normalised[length++] = character;
            ++mantissaDigits;
        }
        else if (character == '.' && !sawPoint)
        {
            normalised[length++] = '.';
            sawPoint = true;
        }
        else
        {
            break;
        }
    }
    if (mantissaDigits == 0)
        return std::nullopt;

    if (position < size)
    {
        if (text[position] != 'E' && text[position] != 'e')
            return std::nullopt;
        ++position;

        normalised[length++] = 'e';
        if (position < size && (text[position] == '+' || text[position] == '-'))
        {
            if (text[position] == '-')
                normalised[length++] = '-';
            ++position;
        }
        std::size_t exponentDigits = 0;
        for (; position < size && isDigit(text[position]); ++position)
        {
            normalised[length++] = text[position];
            ++exponentDigits;
        }
        if (exponentDigits == 0 || position != size)
            return std::nullopt;
    }

    double value = 0.0;
    const auto result = std::from_chars(normalised.data(), normalised.data() + length, value);
    if (result.ec != std::errc() || result.ptr != normalised.data() + length || !std::isfinite(value))
        return std::nullopt;
    return value;
}

} // namespace oor::base
