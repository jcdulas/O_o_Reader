#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <vector>

namespace oor::model
{

// One ACCEL1 card: a static acceleration load on individual grid points. The acceleration is the vector (N1, N2, N3) of
// coordinate system CID, scaled so that its magnitude is A times the magnitude of that vector. The load does not act on
// the mass of scalar points, and the format reference says it is not supported with superelements.
//
// Card layout: ACCEL1  SID  CID  A  N1  N2  N3  (blank)  (blank)         fields 1 to 6
//             (continuation)  GRIDID1  GRIDID2 ...                       from field 9 on
// The list takes "a THRU b" and "a THRU b BY n". SID is a load set ID shared by the cards of the set (it may not be the
// SID of any other load entry). CID defaults to the basic system (0). A is required; the N fields default to
// 0.0 and not all of them may be zero. A run goes up and a BY is a positive increment (a common deck lists 1 2 3 and
// then 4 THRU 10 BY 2). The list is the one of ContactRuns, a helper for IDs with THRU and BY that any card of the
// library may use: the card keeps a run as a run.
struct AccelNodeLoadCard
{
    base::EntityId id = 0;
    base::EntityId frameId = 0; // 0 is the basic coordinate system
    double magnitude = 0.0;     // the scale factor of the vector
    double directionX = 0.0;    // the acceleration vector, at least one component is not zero
    double directionY = 0.0;
    double directionZ = 0.0;
    std::vector<ContactRun> grids; // the grid points, at least one
};

} // namespace oor::model
