#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

// NORM of an eigenvalue extraction: how the eigenvectors are scaled. Blank is not the same as a word: the default
// depends on the analysis (mass for normal modes, max for buckling), which the card does not know.
enum class EigenvectorNorm : std::uint8_t
{
    Default, // blank
    Mass,    // MASS
    Max      // MAX
};

// One EIGRL card: real eigenvalue extraction by the Lanczos method.
//
// Card layout:
//   EIGRL  SID  V1  V2  ND  MSGLVL  MAXSET  SHFSCL  NORM
//          option_1 = value_1  option_2 = value_2  etc.
// The options are one field each, written NAME=value. ALPH, NUMS and F1 to F15 can only be given that way; V1, V2, ND,
// MSGLVL, MAXSET, SHFSCL and NORM can be given either way as long as the field on the first line is blank. A card keeps
// them in the fields of the first line whichever way they were given.
struct LanczosEigenMethodCard
{
    static constexpr std::int32_t MAX_SEGMENTS = 15; // F1 to F15

    base::EntityId id = 0;
    std::optional<double> lowerFrequency; // the range of interest; blank is open on that side
    std::optional<double> upperFrequency;
    std::int32_t desiredModeCount = 0; // roots wanted; 0 is blank
    std::int32_t messageLevel = 0;     // diagnostic level
    std::int32_t maximumSet = 0;       // Lanczos block size; 0 is blank (the machine default)
    std::optional<double> shiftScale;
    EigenvectorNorm norm = EigenvectorNorm::Default;
    double alpha = 1.0;                     // ALPH: the frequency spacing of the segments of the parallel method
    std::int32_t frequencySegmentCount = 1; // NUMS: the number of segments
    std::vector<double> segmentFrequencies; // F1, F2, ...: the upper frequency of each segment
};

} // namespace oor::model
