#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"

namespace oor::model
{

// One PORFCPL card (SOL 700): a hole in a coupling surface through which gas flows to another coupling surface. A
// LEAKAGE of PORTYPE PORFCPL names it (and only a LEAKAGE does).
//
// Card layout: PORFCPL  PID  SIZE  (blank)  FLOW  CSID  MID            fields 1, 2, 4, 5 and 6; field 3 is an empty
// cell SIZE says how the gas that crosses the hole is computed: SMALL (the hole is no larger than the Euler mesh: one
// dimensional flow through an orifice) or LARGE (a hole larger than the mesh: the velocity method of an Eulerian air
// bag); SMALL when blank. FLOW is the allowed direction into the COUPLE that names the card (BOTH when blank). CSID is
// the COUPLE on the other side of the hole, required. MID is the MATDEUL of the gas that is transported, used only when
// the hole joins a GBAG with the multi-material Euler solver and SIZE is LARGE: blank otherwise.
struct CouplingHoleToCouplingCard
{
    base::EntityId id = 0; // unique among the PORFCPL cards
    HoleSize size = HoleSize::Small;
    FlowDirection flow = FlowDirection::Both;
    base::EntityId couplingId = 0; // the COUPLE on the other side
    base::EntityId matId = 0;      // a MATDEUL, 0 when blank
};

} // namespace oor::model
