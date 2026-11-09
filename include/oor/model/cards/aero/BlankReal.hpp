#pragma once

#include <cmath>
#include <limits>
#include <optional>

namespace oor::model
{

// A real that the format reference lets stay blank, with no default (the VELOCITY of an AERO, the OMAX of a FLUTTER, the FMID of an
// FLFACT). The card holds it as an optional value; the column of its table holds a NaN for a blank field, which is how
// NumPy and Arrow see a missing double, so that the buffer is exported as it is. A NaN never reaches a card: the
// readers and the Python layer refuse it as a value.
inline constexpr double BLANK_REAL = std::numeric_limits<double>::quiet_NaN();

[[nodiscard]] inline double realToColumn(const std::optional<double>& value) noexcept
{
    return value ? *value : BLANK_REAL;
}

[[nodiscard]] inline std::optional<double> realFromColumn(double stored) noexcept
{
    if (std::isnan(stored))
        return std::nullopt;
    return stored;
}

} // namespace oor::model
