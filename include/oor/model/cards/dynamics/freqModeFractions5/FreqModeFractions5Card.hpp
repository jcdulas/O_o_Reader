#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One FREQ5 card: fractions of the modal frequencies of a range (modal frequency response).
//
// Card layout: FREQ5  SID  F1  F2  FR1  FR2  FR3  FR4  FR5
//                    FR6  FR7  -etc.-
// The excitation frequencies are FRi * fN for every modal frequency fN in [F1, F2], those outside [F1, F2] dropped.
// The fractions follow each other from field 4 over as many continuation lines as needed, in the order written.
struct FreqModeFractions5Card
{
    base::EntityId id = 0;
    double frequency1 = 0.0;    // the lower bound of the range, in cycles per unit time
    double frequency2 = 1.0E20; // the upper bound
    std::vector<double> fractions;
};

} // namespace oor::model
