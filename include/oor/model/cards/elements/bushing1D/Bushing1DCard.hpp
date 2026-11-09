#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One CBUSH1D card: a rod-type spring and damper between two grid points.
//
// Card layout:
//   CBUSH1D  EID  PID  GA  GB  CID
//
// GB blank (0) is a grounded end. CID is the coordinate system whose x-axis is the element axis; blank, the axis is the
// line from GA to GB. A CID is needed when GA and GB are coincident or GB is blank (the format reference; a later
// validation looks at it, the card does not).
struct Bushing1DCard
{
    static constexpr base::EntityId NO_FRAME_ID = -1; // CID blank: the element axis is the line GA-GB

    base::EntityId id = 0;
    base::EntityId propId = 0; // stored resolved: a blank PID is EID
    base::EntityId nodeA = 0;
    base::EntityId nodeB = 0;
    base::EntityId frameId = NO_FRAME_ID; // NO_FRAME_ID: blank; 0 is the basic system
};

} // namespace oor::model
