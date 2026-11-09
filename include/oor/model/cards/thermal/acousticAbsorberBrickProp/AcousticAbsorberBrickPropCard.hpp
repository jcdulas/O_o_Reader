#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>
#include <string>

namespace oor::model
{

// One PACABS card: the property of a CHACAB acoustic absorber element.
//
// Card layout:
//   PACABS  PID  SYNTH  TID1  TID2  TID3  (blank)  CUTFR  B
//           K    M
// SYNTH is YES (blank: B, K and M are computed from the tables) or NO (they are given). TID1, TID2 and TID3 are the
// TABLEDi of the resistance, the reactance and the weighting function (0 when blank; TID3 is optional). CUTFR is the
// cutoff frequency of the tables. B, K and M are the equivalent damping, stiffness and mass per unit area. Field 6 is
// a blank cell of the layout, but a deck may have a value there: the value, which the solver does not read, is kept as
// `reservedValue` so that the card is written back as it was read.
struct AcousticAbsorberBrickPropCard
{
    base::EntityId id = 0;
    std::string synthesize = "Yes";
    base::EntityId resistanceTableId = 0;
    base::EntityId reactanceTableId = 0;
    base::EntityId weightingTableId = 0;
    std::optional<double> reservedValue;
    std::optional<double> cutoffFrequency;
    std::optional<double> damping;
    std::optional<double> stiffness;
    std::optional<double> mass;
};

} // namespace oor::model
