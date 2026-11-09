#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The failure criterion of a BJOIN (the TYPE field). The format reference lists the spotweld only. The enumerator is
// numbered in the order of SPOTWELD_FAILURE_WORDS.
enum class SpotweldJoinPairsFailure : std::uint8_t
{
    Spotweld
};
inline constexpr std::array<std::string_view, 1> SPOTWELD_FAILURE_WORDS{"SpotWeld"};

// The MULTI field of a BJOIN: whether the grid points of the BCGRID are already given as a sequence of pairs (YES), or
// the solver makes a pair of every two points that lie within the tolerance (NO). The enumerators are numbered in the
// order of SPOTWELD_MULTIPLE_PAIRS_WORDS.
enum class SpotweldJoinPairsPairs : std::uint8_t
{
    Given,
    Matched
};
inline constexpr std::array<std::string_view, 2> SPOTWELD_MULTIPLE_PAIRS_WORDS{"Yes", "No"};

// One BJOIN card (SOL 700): pairs of grid points of one-dimensional or shell elements that are joined (a spotweld)
// until the failure criterion of the pair is met. No other entry of the format reference has this name.
//
// Card layout: BJOIN  BID  SID  TOL  TYPE  (blank)  SN  (blank)  SS     fields 1 to 8
//             +      (blank 9 to 12)  MULTI                           field 13
//             +      (blank 14 to 17)  TF                             field 18
// The layout has these gaps, with SN and SS in fields 6 and 8, so the card reads and writes them there and refuses a
// value in a blank cell. BID numbers the join and SID is the BCGRID that lists the grid
// points. TOL (greater than zero, 1.E-4 when blank) is the distance within which two points make a pair; the solver
// uses it only with MULTI = NO. TYPE is SPOTWELD when blank. SN and SS are the failure forces in tension and in shear
// (greater than zero): a blank one means that the pair never fails that way. MULTI is YES when blank. TF is the failure
// time of the constraint, 1.0E20 when blank (it never fails by time).
struct SpotweldJoinPairsCard
{
    static constexpr double DEFAULT_TOLERANCE = 1.0E-4;
    static constexpr double DEFAULT_FAILURE_TIME = 1.0E20;

    base::EntityId id = 0;        // unique among the BJOIN cards
    base::EntityId nodeSetId = 0; // the BCGRID of the grid points
    double tolerance = DEFAULT_TOLERANCE;
    SpotweldJoinPairsFailure type = SpotweldJoinPairsFailure::Spotweld;
    std::optional<double> tensileFailureForce; // failure force in tension, none when blank
    std::optional<double> shearFailureForce;   // failure force in shear, none when blank
    SpotweldJoinPairsPairs multiplePairs = SpotweldJoinPairsPairs::Given;
    double failureTime = DEFAULT_FAILURE_TIME;
};

} // namespace oor::model
