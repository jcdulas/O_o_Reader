#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <vector>

namespace oor::model
{

// One BCTABL1 card: the contact table of a load case, a list of BCONECT contact pairs (SOLs 101, 103 to 112, 144 to
// 146, 200, 400 and 700). The BCONTACT command names it.
//
// Card layout: BCTABL1  BCID  ID1  ID2  ID3 ...   or, with ranges,   BCTABL1  BCID  ID1  ID2  ID3  THRU  ID4 ...
// over as many continuation lines as needed. A THRU range needs every BCONECT between its ends to exist.
struct ContactTable1Card
{
    base::EntityId id = 0;         // BCID: 0 is the load case 0 (blank counts as 0)
    std::vector<ContactRun> pairs; // the BCONECT IDs, single or in ranges (ascending, no BY)
};

} // namespace oor::model
