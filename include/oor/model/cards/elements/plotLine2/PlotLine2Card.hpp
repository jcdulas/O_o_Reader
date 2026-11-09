#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PLOTEL card: a dummy line element that is only drawn on plots.
//
// Card layout:
//   PLOTEL  EID  G1  G2
//
// G1 and G2 are two different grid points. The format reference puts one element on a card; a deck may put a second one
// in fields 5 to 7 (EID G1 G2 again): each one is a card of its own here.
struct PlotLine2Card
{
    base::EntityId id = 0;
    base::EntityId node1 = 0;
    base::EntityId node2 = 0;
};

} // namespace oor::model
