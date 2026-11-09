#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/thermal/ThermalRuns.hpp"

#include <vector>

namespace oor::model
{

// One RADBC card: the radiation to space from surface element faces.
//
// Card layout: RADBC  NODAMB  FAMB  CNTRLND  EID1  EID2  EID3  EID4  EID5
//                    EID6  -etc.-
// or with ranges: ... EID1 "THRU" EID2 "BY" INC. NODAMB is the ambient point (a black body at its own temperature),
// FAMB the view factor between the faces and the ambient point, CNTRLND the control point (0 when blank). The elements
// are surface elements: single IDs and ranges. A RADBC has no ID of its own.
struct RadiationToSpaceCard
{
    base::EntityId ambientNode = 0;
    double ambientViewFactor = 0.0;
    base::EntityId controlNode = 0;
    std::vector<ThermalRun> elements;
};

} // namespace oor::model
