#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <vector>

namespace oor::model
{

// One BCPROP card: a 3D contact region made of all the elements that have one of the listed properties; the BSID of a
// BCBODY or BCBODY1 names it. Use properties of one kind only (all shell, all solid or all beam).
//
// Card layout: BCPROP  ID  IP1  IP2  IP3  ...        or        BCPROP  ID  IP1  THRU  IP2  IP3  ...
// over as many continuation lines as needed. The THRU form is the one whose field 3 is THRU, and THRU may appear in
// fields 3 to 7; the ranges go up and have no BY.
struct ContactPropertyRegionCard
{
    base::EntityId id = 0;              // unique among BSURF, BCBOX, BCPROP and BCMATL
    std::vector<ContactRun> properties; // the property IDs, single or in ranges
};

} // namespace oor::model
