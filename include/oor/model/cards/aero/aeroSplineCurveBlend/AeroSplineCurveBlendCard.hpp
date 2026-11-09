#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// OPT of a SPBLND2: the blending functions of the distance to the reference curve. The enumerators are numbered in the
// order of AERO_SPLINE_CURVE_BLEND_OPTION_WORDS. The card has no weighted average (that is SPBLND1) and its default is
// the linear one.
enum class AeroSplineCurveBlendOption : std::uint8_t
{
    Linear, // LIN (the default)
    Cubic   // CUB
};

inline constexpr std::array<std::string_view, 2> AERO_SPLINE_CURVE_BLEND_OPTION_WORDS{"Linear", "Cubic"};

// One SPBLND2 card: a curve based blend of two splines. The reference curve is a polygon through the aerodynamic grid
// points of an AELIST (the order is arbitrary and the points may coincide), and the depths say how far from the curve
// each spline counts.
//
// Card layout: SPBLND2  SID  SID1  SID2  OPT  AELIST  D1  D2
// SID is the number of the blended spline, in the category AeroSpline with SPLINE1 to SPLINE7 (unique per card type, a
// check compares the types). SID1 and SID2 name the two splines it blends; either may be a blend itself. OPT is LIN
// when blank. AELIST (ReferenceTarget::AeroList) lists the points of the curve. D1 and D2 are positive and have no
// default, so a blank one is an error.
struct AeroSplineCurveBlendCard
{
    base::EntityId id = 0;
    base::EntityId splineId1 = 0;
    base::EntityId splineId2 = 0;
    AeroSplineCurveBlendOption option = AeroSplineCurveBlendOption::Linear;
    base::EntityId aeroBoxListId = 0;
    double depth1 = 0.0;
    double depth2 = 0.0;
};

} // namespace oor::model
