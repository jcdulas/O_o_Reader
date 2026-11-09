#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One TICEUL1 card (SOL 700): the initial conditions of the Eulerian regions. A PEULER1 names it in its SID, and it
// names the group of TICREG cards that define the regions and the values each one starts with. A TICEL initializes a
// set of elements and a TICEUL1 initializes regions (spheres, cylinders, ...); an Euler element that both name takes
// the TICEL. No other entry of the format reference has this name.
//
// Card layout: TICEUL1  SID  TSID                                                  fields 1 and 2
// SID is the number of the card, the one a PEULER1 names: the ID of the table, unique among the TICEUL1 cards. TSID is
// the number of a group of TICREG cards, which share it in their first TSID field: a reference to the group, so Python
// sees the list of the cards of the group.
struct EulerInitialRegionsCard
{
    base::EntityId id = 0;            // the number of the card
    base::EntityId regionGroupId = 0; // the group of TICREG cards
};

} // namespace oor::model
