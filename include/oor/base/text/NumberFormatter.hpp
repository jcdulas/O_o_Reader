#pragma once

#include <cstdint>
#include <string>

namespace oor::base
{

// A formatted real and whether the text parses back to exactly the same double.
struct FormattedReal
{
    std::string text;
    bool isExact = true;
};

// Decimal model numbers with an optional standard exponent.
struct NumberFormatter
{
    [[nodiscard]] static std::string formatInteger(std::int64_t value);

    // Round to at most this many significant digits (0 means exact); report whether the result is exact.
    [[nodiscard]] static FormattedReal formatRealWithDigits(double value, int significantDigits);

    // The shortest text that parses back to exactly the same double and remains distinguishable from an integer.
    [[nodiscard]] static std::string formatRealFree(double value);
};

} // namespace oor::base
