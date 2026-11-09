#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One COSMSEL card (SOL 400): the co-simulation services an analysis uses, listed by the number of their COSMSRV.
//
// Card layout: COSMSEL  SID                              field 1, fields 2 to 8 blank
//             +        CSRVID1  CSRVID2  CSRVID3        fields 9 to 11
// The Case Control command COSMSEL selects the card by its SID (the command is kept as text: a number that no card has
// is not reported). The format draws three services and says that only one is supported today; the card
// accepts the three of the layout, at least one, and refuses a service in a later field. A blank field between two
// services is skipped. The SID is unique among the COSMSEL cards (IdSpace::PerCardType, category Other).
struct CosimulationSelectionCard
{
    base::EntityId id = 0;
    std::vector<base::EntityId> services; // CSRVID1 to CSRVID3: the COSMSRV cards
};

} // namespace oor::model
