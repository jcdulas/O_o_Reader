#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

// What the solution control cards (TSTEP, TSTEPNL, NLPARM, NLPCI, ...) share: a blank field whose default depends on
// the solution sequence, which the card cannot know, is kept as "not given" (the card must be written back as it was
// read, and the solution fills it); in the columns an integer that is not given is NO_INTEGER and a real is NaN. Each
// card still lays out its own fields by hand.
namespace oor::model
{

// A blank integer in a column of a table (the integers of these cards can be negative or zero, so no small value
// stands for blank).
inline constexpr std::int32_t NO_INTEGER = std::numeric_limits<std::int32_t>::min();

[[nodiscard]] inline std::int32_t blankInteger(const std::optional<std::int32_t>& value) noexcept
{
    return value ? *value : NO_INTEGER;
}

[[nodiscard]] inline std::optional<std::int32_t> integerOrNone(std::int32_t column) noexcept
{
    if (column == NO_INTEGER)
        return std::nullopt;
    return column;
}

[[nodiscard]] inline double blankReal(const std::optional<double>& value) noexcept
{
    return value ? *value : std::numeric_limits<double>::quiet_NaN();
}

[[nodiscard]] inline std::optional<double> realOrNone(double column) noexcept
{
    if (std::isnan(column))
        return std::nullopt;
    return column;
}

// True when every character of `text` is one of the letters of `allowed` (convergence flags such as "UPW").
[[nodiscard]] inline bool onlyLettersOf(std::string_view text, std::string_view allowed) noexcept
{
    for (const char character : text)
    {
        if (allowed.find(character) == std::string_view::npos)
            return false;
    }
    return true;
}

} // namespace oor::model
