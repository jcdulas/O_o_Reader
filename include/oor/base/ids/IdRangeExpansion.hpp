#pragma once

#include "oor/base/errors/Errors.hpp"

#include <cstdint>
#include <string>

namespace oor::base
{

// The bound on what one "a THRU b [BY c]" may be expanded to: a card keeps its ranges
// as written, and whatever lists the IDs of a range one by one (scalar points, the members of a set, the IDs of a
// non-structural mass) refuses a range longer than this before it allocates anything. A hostile "1 THRU 2000000000"
// would otherwise ask for 8 GB from 20 bytes of text. Ten million IDs is far more than any deck the project has seen.
inline constexpr std::int64_t MAX_EXPANDED_IDS = 10'000'000;

// The number of IDs of the range from `first` to `last` by `by` (either direction; `by` of the sign that goes from
// `first` to `last`, 0 read as 1), computed without overflow for any 64-bit ends.
[[nodiscard]] constexpr std::uint64_t idRangeCount(std::int64_t first, std::int64_t last, std::int64_t by = 1) noexcept
{
    const std::uint64_t span = first <= last ? static_cast<std::uint64_t>(last) - static_cast<std::uint64_t>(first)
                                             : static_cast<std::uint64_t>(first) - static_cast<std::uint64_t>(last);
    const std::uint64_t step =
        by == 0 ? 1 : (by > 0 ? static_cast<std::uint64_t>(by) : 0 - static_cast<std::uint64_t>(by));
    return span / step + 1;
}

// Throws LimitExceededError when `count` IDs (of all the ranges of `what`) are more than MAX_EXPANDED_IDS.
inline void requireExpandable(std::uint64_t count, const std::string& what)
{
    if (count > static_cast<std::uint64_t>(MAX_EXPANDED_IDS))
        throw LimitExceededError(what + " stands for " + std::to_string(count) + " IDs, more than the " +
                                 std::to_string(MAX_EXPANDED_IDS) + " a THRU range may be expanded to");
}

} // namespace oor::base
