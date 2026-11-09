#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PLOTEL4 card: a dummy quadrilateral that is only drawn on plots.
//
// Layout: PLOTEL4  EID  G1  G2  G3  G4
//
// The layout is that of another implementation of the format (PLOTEL4,20,2356,7996,20032,10342); it is not in the
// format reference list and the format reference says no more. Like its two-noded sibling PLOTEL, the element
// takes no part in any solution: it is a quadrilateral drawn through four different grid points, in the order G1 to G4
// around its edge. A card holds one element.
struct PlotQuad4Card
{
    base::EntityId id = 0;
    base::EntityId node1 = 0;
    base::EntityId node2 = 0;
    base::EntityId node3 = 0;
    base::EntityId node4 = 0;
};

} // namespace oor::model
