#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"

namespace oor::model
{

// One PORFGBG card (SOL 700): a hole in a COUPLE or GBAG (sub)surface that is connected to another GBAG, to model the
// flow between two air bags (two GBAGs, a GBAG and an Eulerian air bag, or two Eulerian air bags). A LEAKAGE of PORTYPE
// PORFGBG names it.
//
// Card layout: PORFGBG  FID  SIZE  (blank)  FLOW  GBID  MID            fields 1, 2, 4, 5 and 6; field 3 is an empty
// cell SIZE is the flow method (SMALL when blank: one dimensional flow through an orifice; LARGE: the velocity method
// of an Eulerian air bag), FLOW the allowed direction into the GBAG or coupling surface that names the card (BOTH when
// blank). GBID is the GBAG on the other side of the hole, required. MID is the MATDEUL of the gas of that GBAG, used
// only when it meets an Eulerian air bag of the multi-material solver with SIZE LARGE: blank otherwise.
struct CouplingHoleToGasBagCard
{
    base::EntityId id = 0; // unique among the PORFGBG cards
    HoleSize size = HoleSize::Small;
    FlowDirection flow = FlowDirection::Both;
    base::EntityId gasBagId = 0; // the GBAG on the other side
    base::EntityId matId = 0;    // a MATDEUL, 0 when blank
};

} // namespace oor::model
