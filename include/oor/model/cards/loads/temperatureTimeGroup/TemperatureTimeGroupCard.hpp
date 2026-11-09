#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <vector>

namespace oor::model
{

// One TMPSET card (SOL 400, nonlinear transient analysis): a group of grid points that share one time function. A TTEMP
// card names the group by its ID in GROUP_ID and gives the TABLED entry of the function; the temperature of each grid
// point of the group comes from the TEMP, TEMPD, TEMPP1 or TEMPRB cards that have the SID of that TTEMP.
//
// Card layout: TMPSET  ID  G1  G2  G3  G4  G5  G6  G7                   fields 1 to 8
//             TMPSET  ID  G1  THRU  G2  BY  INC                         the alternate format
//             (continuation)  G8  G9  G10  G11  -etc.-  or  G8  THRU  G9  BY  INC     from field 9 on
// The two formats and the two continuation formats may be mixed and repeated in any order, so the list is read from
// field 2 to the last used field: grid points, "a THRU b" and "a THRU b BY n" in any sequence, blank fields skipped
// (a common deck leaves blanks at the end of its lines). The list is the one of ContactRuns, a helper for IDs with
// THRU and BY that any card of the library may use; the card keeps a run as a run (a THRU over a million grids stays
// one run). The format does not allow a range to go down, and the increment is above zero, so the card refuses both.
// The list has at least one grid point.
//
// The ID is the group's own number: IdSpace::PerCardType, category TemperatureGroup, unique among the TMPSET cards
// and what a TTEMP names. A single grid point of the list is a Node reference; the two ends of a THRU range
// are NodeInRange (the grid points between them need not exist), as for the other lists with THRU.
struct TemperatureTimeGroupCard
{
    base::EntityId id = 0;
    std::vector<ContactRun> grids;

    [[nodiscard]] bool operator==(const TemperatureTimeGroupCard&) const noexcept = default;
};

} // namespace oor::model
