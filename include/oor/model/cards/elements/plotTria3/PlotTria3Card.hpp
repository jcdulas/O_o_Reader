#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PLOTEL3 card: a dummy triangle that is only drawn on plots.
//
// Layout: PLOTEL3  EID  G1  G2  G3
//
// The layout is that of another implementation of the format (PLOTEL3,10,2356,7996,20032); it is not in the format
// reference list and the format reference says no more. Like its two-noded sibling PLOTEL, the element takes no
// part in any solution: it is a triangle drawn between three different grid points. A card holds one element (no second
// group of fields as the PLOTEL has).
struct PlotTria3Card
{
    base::EntityId id = 0;
    base::EntityId node1 = 0;
    base::EntityId node2 = 0;
    base::EntityId node3 = 0;
};

} // namespace oor::model
