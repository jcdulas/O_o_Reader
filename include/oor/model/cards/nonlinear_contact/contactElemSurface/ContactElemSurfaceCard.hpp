#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <vector>

namespace oor::model
{

// One BSURF card: a contact body or surface made of elements, named by the BSID of a BCBODY or BCBODY1.
//
// Card layout: BSURF  ID  ELID1  ELID2  ELID3  ...   or, with ranges,   BSURF  ID  ELID1  THRU  ELID2  BY  INC  ...
// over as many continuation lines as needed. A range may go down; BY is 1 for a rising range and -1 for a falling one
// when it is left out (stored resolved in each run).
struct ContactElemSurfaceCard
{
    base::EntityId id = 0;            // unique among BSURF, BCBOX, BCPROP and BCMATL
    std::vector<ContactRun> elements; // the element IDs, single or in ranges
};

} // namespace oor::model
