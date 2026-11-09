#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::base
{

// Parses the numbers of a model value. Leading and trailing blanks are ignored; a blank field is not a number
// (the caller decides what a blank means). A malformed field gives std::nullopt, never an exception.
struct NumberParser
{
    // Integer: optional sign then digits only. "1." is a real, not an integer. Overflow gives nullopt.
    [[nodiscard]] static std::optional<std::int64_t> parseInteger(std::string_view text) noexcept;

    // Decimal real with an optional E/e exponent. Nonfinite values and malformed text are refused.
    [[nodiscard]] static std::optional<double> parseReal(std::string_view text) noexcept;
};

} // namespace oor::base
