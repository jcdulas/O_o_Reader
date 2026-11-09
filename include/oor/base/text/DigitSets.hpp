#pragma once

#include <cstdint>

namespace oor::base
{

// Fields that list degrees of freedom or components (PS of a GRID, C of an SPC1, ...) hold a set of the digits 0 to 6
// written as one integer: "123", "321" and "213" are the same set. The library keeps the digits in ascending order, so
// that equal sets compare equal and are written the same way.
[[nodiscard]] constexpr std::int32_t sortedDigits(std::int32_t value) noexcept
{
    if (value <= 0)
        return value;
    bool present[10] = {};
    for (std::int32_t rest = value; rest > 0; rest /= 10)
        present[rest % 10] = true;
    std::int32_t result = 0;
    for (std::int32_t digit = 0; digit <= 9; ++digit)
    {
        if (present[digit])
            result = result * 10 + digit;
    }
    return result;
}

// True when the value is a valid set of components: 0 (a scalar point, or nothing), or the digits 1 to 6, each at most
// once, in any order.
[[nodiscard]] constexpr bool isComponentSet(std::int32_t value) noexcept
{
    if (value == 0)
        return true;
    if (value < 0)
        return false;
    unsigned seen = 0;
    for (std::int32_t rest = value; rest > 0; rest /= 10)
    {
        const unsigned digit = static_cast<unsigned>(rest % 10);
        if (digit < 1 || digit > 6 || (seen & (1U << digit)) != 0)
            return false;
        seen |= 1U << digit;
    }
    return true;
}

} // namespace oor::base
