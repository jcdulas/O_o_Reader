#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/thermal/ThermalRuns.hpp"

#include <vector>

namespace oor::model
{

// One QVOL card: the volumetric heat generated in a list of conduction elements.
//
// Card layout: QVOL  SID  QVOL  CNTRLND  EID1  EID2  EID3  EID4  EID5
//                   EID6  -etc.-
// or with ranges: ... EID1 "THRU" EID2 "BY" INC. SID is a load set, QVOL the power per unit volume (the heat generation
// capability HGEN of the MAT4 or MAT5 multiplies it), CNTRLND the control point whose temperature is a multiplier (0
// when blank). The elements are conduction elements, as written: single IDs and ranges.
struct HeatVolumeSourceLoadCard
{
    base::EntityId id = 0;
    double heatGeneration = 0.0;
    base::EntityId controlNode = 0;
    std::vector<ThermalRun> elements;
};

} // namespace oor::model
