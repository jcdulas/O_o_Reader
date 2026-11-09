#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// A run of equal time steps of a TSTEP card: N steps of DT, of which every NO-th is output.
struct TransientTimeStepsInterval
{
    std::int32_t stepCount = 1;      // N
    double timeStep = 0.0;           // DT
    std::int32_t outputInterval = 1; // NO
};

// One TSTEP card: the time steps of a linear transient analysis (selected by TSTEP in Case Control).
//
// Card layout:  TSTEP  SID  N1  DT1  NO1
//                          N2  DT2  NO2
//                          -etc.-
// Each interval is one line, from field 2 of the line (the first field of the first line is the SID: the later lines
// leave field 1 blank, so interval k starts at field 2 + 8 k). The solution takes the intervals in the order written.
struct TransientTimeStepsCard
{
    base::EntityId id = 0;
    std::vector<TransientTimeStepsInterval> intervals;
};

} // namespace oor::model
