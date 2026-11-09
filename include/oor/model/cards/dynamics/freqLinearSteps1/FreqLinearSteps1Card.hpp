#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One FREQ1 card: excitation frequencies at equal steps.
//
// Card layout: FREQ1  SID  F1  DF  NDF
// The frequencies are F1 + i * DF for i = 0 to NDF, in cycles per unit time. A blank F1 is 0.0 (the format reference
// gives no default but asks for a positive value, and a start at zero is common).
struct FreqLinearSteps1Card
{
    base::EntityId id = 0;
    double frequency1 = 0.0;
    double frequencyIncrement = 0.0;
    std::int32_t incrementCount = 1; // the number of increments
};

} // namespace oor::model
