#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One NLFREQ1 card: the forcing frequencies of a nonlinear harmonic response at equal steps.
//
// Card layout: NLFREQ1  ID  F1  DF  NDF
// The first frequency is F1; the others follow it by steps of DF, up (DF > 0) or down (DF < 0), NDF times. The
// frequencies are in cycles per unit time.
struct NonlinearHarmonicFreqStepsCard
{
    base::EntityId id = 0; // the NLFREQ field of an NLHARM names it
    double startFreq = 0.0;
    double freqStep = 0.0;
    std::int32_t stepCount = 1; // the number of increments or decrements
};

} // namespace oor::model
