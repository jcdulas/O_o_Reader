#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// OPT of a SPBLND1: how the two splines are blended. The enumerators are numbered in the order of
// AERO_SPLINE_STRIP_BLEND_OPTION_WORDS.
enum class AeroSplineStripBlendOption : std::uint8_t
{
    WeightedAverage, // WAVG (the default): the weights W1 and 1 - W1 apply everywhere
    Linear,          // LIN: linear blending functions of the distance to the reference grid
    Cubic            // CUB: cubic blending functions
};

inline constexpr std::array<std::string_view, 3> AERO_SPLINE_STRIP_BLEND_OPTION_WORDS{"WeightedAverage", "Linear",
                                                                                      "Cubic"};

// One SPBLND1 card: a strip based blend of two splines, which the other splines name as their own (the blend has a
// spline ID of its own and may be blended again).
//
// Card layout: SPBLND1  SID  SID1  SID2  OPT  W1  GID  D1  D2
//                      X1   X2    X3    CID
// SID is the number of the blended spline, in the category AeroSpline with SPLINE1 to SPLINE7 (the ID is unique per
// card type and a check compares the types). SID1 and SID2 name the two splines it blends; either may be a blend
// itself. OPT is WAVG when blank. W1 is the weight of the first spline, 0.5 when blank, greater than zero and at
// most 1.0 (the format reference gives the weight of the second spline as 1 - W1); only WAVG uses it, the range is
// checked for every OPT.
//
// With WAVG the reference grid GID, the depths D1 and D2 and the direction vector (X1, X2, X3) are optional, so they
// stay blank (GID 0, std::nullopt); with LIN or CUB the blending functions need them and the card refuses a blank one.
// GID is an aerodynamic grid (ReferenceTarget::AeroNode), D1 and D2 are positive, and the direction vector is given in
// the rectangular system CID (0, the basic system, when blank): at least one of its components must be different from
// zero. A blank component is std::nullopt and is 0.0 where the vector is used.
struct AeroSplineStripBlendCard
{
    static constexpr double DEFAULT_WEIGHT = 0.5;

    base::EntityId id = 0;
    base::EntityId splineId1 = 0;
    base::EntityId splineId2 = 0;
    AeroSplineStripBlendOption option = AeroSplineStripBlendOption::WeightedAverage;
    double weight1 = DEFAULT_WEIGHT;
    base::EntityId nodeId = 0;
    std::optional<double> depth1;
    std::optional<double> depth2;
    std::optional<double> directionX;
    std::optional<double> directionY;
    std::optional<double> directionZ;
    base::EntityId frameId = 0;
};

} // namespace oor::model
