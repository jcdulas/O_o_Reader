#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <vector>

namespace oor::model
{

// One BLSEG card: a curve of line segments of a slideline contact region, given by its grid points in topological order
// (one segment between every two consecutive points). A BCONP names it as its secondary or primary line; a BWIDTH of
// the same ID gives the width of each segment.
//
// Card layout: BLSEG  ID  G1  G2  G3  ...   or, with ranges,   BLSEG  ID  G1  THRU  G2  BY  INC  ...
// over as many continuation lines as needed. A range may go down (BY is 1 or -1 when left out, stored resolved). A
// closed curve repeats its first point at the end.
struct SlidelineSegmentsCard
{
    base::EntityId id = 0;
    std::vector<ContactRun> nodes;
};

} // namespace oor::model
