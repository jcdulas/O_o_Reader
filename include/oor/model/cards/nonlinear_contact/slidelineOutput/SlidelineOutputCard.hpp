#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <vector>

namespace oor::model
{

// One BOUTPUT card: the secondary nodes at which a slideline contact analysis writes output.
//
// Card layout: BOUTPUT  ID  G1  G2  G3  ...   or, with ranges,   BOUTPUT  ID  G1  THRU  G2  BY  INC  ...
// over as many continuation lines as needed, or  BOUTPUT  ID  ALL  (no continuation line). ID is the contact region of
// a BCONP (slideline contact) or the contact grid in 3D contact: it is not a reference. A range may go down (BY is 1 or
// -1 when left out, stored resolved).
struct SlidelineOutputCard
{
    base::EntityId id = 0;
    bool all = false;              // ALL: every secondary node of the region
    std::vector<ContactRun> nodes; // empty with ALL
};

} // namespace oor::model
