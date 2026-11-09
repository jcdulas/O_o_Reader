#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/thermal/ThermalRuns.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

// One RADLST card: the surface elements of a radiation enclosure.
//
// Card layout: RADLST  ICAVITY  MTXTYP  EID1  EID2  EID3  EID4  EID5  EID6
//                     EID7  -etc.-
// or, with a range: ... EID1 "THRU" EID2. An element ID is not zero and may be negative: -EID is the back face of the
// surface element. MTXTYP is 1 to 4, or -n (the first n elements may lose energy to space); a blank
// MTXTYP is the program's choice (1 without an ambient element, 4 with one), kept as `std::nullopt` because it depends
// on the RADCAV.
struct RadiationCavityListCard
{
    base::EntityId cavityId = 0;
    std::optional<int> matrixKind;
    std::vector<ThermalRun> elements;
};

} // namespace oor::model
