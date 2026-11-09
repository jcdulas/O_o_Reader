#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/thermal/ThermalRuns.hpp"

#include <vector>

namespace oor::model
{

// One QBDY1 card: a uniform heat flux into surface elements.
//
// Card layout: QBDY1  SID  Q0  EID1  EID2  EID3  EID4  EID5  EID6
//                    EID7  EID8  -etc.-
// or: ... EID1 "THRU" EID2. The elements are CHBDYE, CHBDYG and CHBDYP elements; an ID is not zero, and a negative ID
// is the back face of the surface element.
struct HeatFluxSurfaceLoad1Card
{
    base::EntityId id = 0;
    double heatFlux = 0.0;
    std::vector<ThermalRun> elements;
};

} // namespace oor::model
