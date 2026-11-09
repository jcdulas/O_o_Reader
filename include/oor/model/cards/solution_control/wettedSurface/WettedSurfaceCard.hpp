#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One WETSURF card (SOL 400): a wetted surface of a fluid-structure co-simulation, a list of wetted elements.
//
// Card layout: WETSURF  WSID  WTAG                                        fields 1 and 2, fields 3 to 8 blank
//             +        WEID1  WEID2  WEID3  ...                           fields 9 on, any number
//        or:  +        WEID1  THRU  WEID2  BY  INC                        (the alternate format: a run)
// The list goes on over as many continuation lines as it needs, with no end marker, and mixes single IDs and runs, with
// or without BY, in any order. A run is kept as written ("a THRU b" with a blank INC is 1) and goes upwards; its end
// points are IDs that must exist, because the kind of reference has no "range" form (a WETELME or WETELMG number
// between them need not). A blank field is the padding of a line. The first line holds nothing but WSID and WTAG, as
// the layout has it, so a value in fields 3 to 8 is refused.
//
// WTAG is the name the surface has for the external fluid code through the service interface: required, kept as
// written.
// The WSID is unique among the WETSURF cards (IdSpace::PerCardType, category WettedSurface) and a WETLOAD names it.
struct WettedSurfaceCard
{
    base::EntityId id = 0;
    std::string wettedTag;
    std::vector<ContactRun> runs; // WEID1, WEID2, ...: wetted elements, singly and in THRU/BY runs
};

} // namespace oor::model
