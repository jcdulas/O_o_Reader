#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <vector>

namespace oor::model
{

// One UNGLUE card (SOLs 101 and 400): the grid points of a contact body that are taken out of glued contact and use
// standard contact instead (for a crack, the points along it).
//
// Card layout: UNGLUE  ID  BID  ID1  THRU  ID2  BY  N                                 fields 1 to 7 of the first line
//                     ID3  THRU  ID4  ID5  ID6  -etc.-                               fields 9 and on
// ID is selected by a SUBCASE or a STEP with the Case Control command BCONTACT=ID or UNGLUE=ID (the UNGLUE command
// wins over BCONTACT for an entry with the same ID); an ID of 0 puts the entry in load case 0, which needs no command
// and always acts, so 0 is a valid ID. The entry may be repeated for every subcase and body, so several cards share an
// ID (IdSpace::Group, category Other: no new kind of ID). BID is the body, a BCBODY or a BCBODY1 (`ContactBody`). The
// grid points are a list of IDs, "a THRU b" ranges and "a THRU b BY n" ranges; the increment must be a positive
// integer, so a range goes up (the format wants a positive increment), and a range with no BY has the increment 1. A
// single point is a `Node`, the ends of a range are `NodeInRange` (a range may skip points that no GRID defines).
struct ContactUnglueNodesCard
{
    base::EntityId id = 0;
    base::EntityId bodyId = 0;
    std::vector<ContactRun> grids;

    [[nodiscard]] bool operator==(const ContactUnglueNodesCard&) const noexcept = default;
};

} // namespace oor::model
