#pragma once

#include <cstdint>
#include <string_view>

namespace oor::model
{

// The number of significant digits of a real number as it is written in a field: the digits of the
// mantissa (before an exponent written E, D or as the compact sign of "1.5+3"), without the sign, the decimal point and
// the zeros before the first non-zero digit. The zeros after the last non-zero digit count, since they were written:
// "1.50" has 3, "0.0012" has 2, "1.5+3" has 2, "-7." has 1, "100" has 3. A zero ("0.", "0.000") has 1. The count stops
// at 255.
[[nodiscard]] std::uint8_t significantDigitsOf(std::string_view text) noexcept;

} // namespace oor::model
