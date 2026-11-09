#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One FREQ4 card: a spread of frequencies around each modal frequency of a range (modal frequency response).
//
// Card layout: FREQ4  SID  F1  F2  FSPD  NFM
// Every modal frequency fN in [F1, F2] gets NFM evenly spaced excitation frequencies between (1 - FSPD) fN and
// (1 + FSPD) fN.
struct FreqSpreadAroundModes4Card
{
    base::EntityId id = 0;
    double frequency1 = 0.0;       // the lower bound of the range, in cycles per unit time
    double frequency2 = 1.0E20;    // the upper bound
    double frequencySpread = 0.10; // the fractional spread on each side of a modal frequency
    std::int32_t frequencyCount = 3;
};

} // namespace oor::model
