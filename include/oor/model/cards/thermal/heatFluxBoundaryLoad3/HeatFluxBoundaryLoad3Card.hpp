#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/thermal/ThermalRuns.hpp"

#include <vector>

namespace oor::model
{

// One QBDY3 card: a uniform heat flux (or its multiplier) into a boundary surface.
//
// Card layout: QBDY3  SID  Q0  CNTRLND  EID1  EID2  EID3  EID4  EID5
//                    EID6  -etc.-
// or with ranges: ... EID1 "THRU" EID2 "BY" INC. Q0 is the heat flux into the surface, CNTRLND the control point whose
// temperature multiplies it (0 when blank). The elements are CHBDYE, CHBDYG and CHBDYP elements: single IDs, which are
// not zero (a negative one is the back face), and ranges.
struct HeatFluxBoundaryLoad3Card
{
    base::EntityId id = 0;
    double heatFlux = 0.0;
    base::EntityId controlNode = 0;
    std::vector<ThermalRun> elements;
};

} // namespace oor::model
