#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One FREQ2 card: excitation frequencies at equal logarithmic steps.
//
// Card layout: FREQ2  SID  F1  F2  NF
// The frequencies are F1 * exp(i * d) for i = 0 to NF, with d = ln(F2 / F1) / NF, in cycles per unit time.
struct FreqLogSteps2Card
{
    base::EntityId id = 0;
    double frequency1 = 0.0;        // the first frequency
    double frequency2 = 0.0;        // the last frequency
    std::int32_t intervalCount = 1; // the number of logarithmic intervals
};

} // namespace oor::model
