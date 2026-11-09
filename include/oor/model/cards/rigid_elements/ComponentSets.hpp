#pragma once

#include <cstdint>

namespace oor::model::components
{

// What the rigid elements need to do with the components fields, which are sets of the digits 1 to 6 written as one
// integer (123456 is all six): take a set away from another, and tell whether two share a component. Used by the cards
// of the rigid_elements family; the digits are expected to be valid (base::isComponentSet).

// All six components.
inline constexpr std::int32_t ALL = 123456;

// Bit d is set when digit d (1 to 6) is in the set.
[[nodiscard]] constexpr unsigned maskOf(std::int32_t set) noexcept
{
    unsigned mask = 0;
    for (std::int32_t rest = set; rest > 0; rest /= 10)
        mask |= 1U << static_cast<unsigned>(rest % 10);
    return mask;
}

// The set of the digits of a mask, in ascending order.
[[nodiscard]] constexpr std::int32_t setOf(unsigned mask) noexcept
{
    std::int32_t set = 0;
    for (unsigned digit = 1; digit <= 6; ++digit)
    {
        if ((mask & (1U << digit)) != 0)
            set = set * 10 + static_cast<std::int32_t>(digit);
    }
    return set;
}

// The components of `set` that are not in `taken`.
[[nodiscard]] constexpr std::int32_t without(std::int32_t set, std::int32_t taken) noexcept
{
    return setOf(maskOf(set) & ~maskOf(taken));
}

// True when the two sets have a component in common.
[[nodiscard]] constexpr bool overlap(std::int32_t first, std::int32_t second) noexcept
{
    return (maskOf(first) & maskOf(second)) != 0;
}

} // namespace oor::model::components
