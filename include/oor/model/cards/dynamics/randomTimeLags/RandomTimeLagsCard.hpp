#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One RANDT1 card: the time lags of the autocorrelation function of a random analysis (selected by RANDOM in Case
// Control, with the RANDPS cards of the same set ID).
//
// Card layout: RANDT1  SID  N  T0  TMAX
// There are N intervals between the lags T0 and TMAX: T(i) = T0 + (TMAX - T0) (i - 1) / N, for i = 1 to N + 2.
struct RandomTimeLagsCard
{
    base::EntityId id = 0;
    std::int32_t count = 1;   // the number of time lag intervals
    double startTime = 0.0;   // the starting time lag
    double maximumTime = 0.0; // the maximum time lag
};

} // namespace oor::model
