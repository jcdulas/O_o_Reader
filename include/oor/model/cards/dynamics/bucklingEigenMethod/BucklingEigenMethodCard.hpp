#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// METHOD of a buckling analysis: inverse power or enhanced inverse power. The format reference gives no default and
// asks for one of the two words; this card accepts a blank, as "not given".
enum class BucklingMethod : std::uint8_t
{
    NotGiven, // blank
    Inv,      // INV
    Sinv      // SINV
};

// NORM of a buckling analysis: how the eigenvectors are scaled.
enum class BucklingNorm : std::uint8_t
{
    Max,  // MAX: the largest component is one (the default)
    Point // POINT: the component of point G and component C is one
};

// One EIGB card: the data for a buckling analysis (selected by METHOD in Case Control, with EIGRL).
//
// Card layout:
//   EIGB  SID  METHOD  L1  L2  NEP  NDP  NDN
//         NORM G       C
// The eigenvalue range of interest is [L1, L2]. NEP estimates the roots in the positive range (not used by SINV); NDP
// and NDN are the roots wanted in the positive and the negative range, 3 * NEP when blank: stored resolved (0 is a
// blank NEP, NDP or NDN that has nothing to resolve from) and written when not zero. G and C are the point for NORM =
// POINT: the format reference asks for them only then, and this card does not refuse them with MAX.
struct BucklingEigenMethodCard
{
    base::EntityId id = 0;
    BucklingMethod method = BucklingMethod::NotGiven;
    double lowerEigenvalue = 0.0;
    double upperEigenvalue = 0.0;
    std::int32_t estimatedModeCount = 0;
    std::int32_t desiredPositiveModeCount = 0;
    std::int32_t desiredNegativeModeCount = 0;
    BucklingNorm norm = BucklingNorm::Max;
    base::EntityId normNode = 0;    // grid or scalar point of NORM = POINT; 0 is blank
    std::int32_t normComponent = 0; // its component; 0 is blank
};

} // namespace oor::model
