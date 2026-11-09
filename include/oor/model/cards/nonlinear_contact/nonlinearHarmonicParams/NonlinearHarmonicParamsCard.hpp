#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One NLHARM card: the parameters of a nonlinear harmonic response analysis (selected by the NLHARM Case Control
// command).
//
// Card layout: NLHARM  ID  SUBFAC  NHARM  NLFREQ
// NLFREQ names the NLFREQ, NLFREQ1 or NLFREQC card that lists the forcing frequencies.
struct NonlinearHarmonicParamsCard
{
    base::EntityId id = 0;
    std::int32_t subharmonicFactor = 1; // the factor that captures sub-harmonic response (1 is none)
    std::int32_t harmonicCount = 0;     // the number of harmonics of the solution
    base::EntityId freqListId = 0;
};

} // namespace oor::model
