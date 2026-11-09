#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// FTYPE of a SPLINE4 or a SPLINE5: the radial function of the Radial Interpolation Spline (METH = RIS), a Wendland
// function of the distance to a point divided by the radius of support RCORE. The two cards share the words and the
// default (WF2); the other methods do not use the field.
enum class SplineRadialFunction : std::uint8_t
{
    Wendland0, // WF0: continuous, with no continuous derivative
    Wendland2  // WF2: continuous up to the second derivative (the default)
};

// The words of the format reference, in the order of the enumerators.
inline constexpr std::array<std::string_view, 2> SPLINE_RADIAL_FUNCTION_WORDS{"WendlandFunction0", "WendlandFunction2"};

} // namespace oor::model
