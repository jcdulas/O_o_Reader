#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <string>

namespace oor::model
{

// One ATBJNT card (SOL 700): the interface to one joint of the dummy of an external occupant model. A joint connects
// two segments and has a local coordinate system on each. The card gives the grid points that span the two systems,
// which SOL 700 uses (with the ATBSEG cards of the two segments) to overwrite the position of the joint that the input
// file of the occupant model has, and the numbers of the elements it may generate to draw the joint in a preprocessor.
//
// Card layout: ATBJNT  ID  NAME                                       fields 1 and 2
//             (line 2)  G0  G1  G2  G3  EID1  EID2  EID3             fields 9 to 15
//             (line 3)  G4  G5  G6  G7  EID4  EID5  EID6             fields 17 to 23
// ID numbers the card (the format reference head line calls it LID in the field table). NAME is the name of the joint
// in the first field of a B.3 entry of the occupant input file. G0 is the origin of the joint system of the segment the
// joint leaves, G1, G2 and G3 are on its x, y and z axes; G4 to G7 do the same for the next segment. The format
// reference marks the grid points as required. The EIDi are numbers for elements that SOL 700 creates, not elements of
// the deck: they are not references, and the format does not say that they are required, so a blank one is accepted.
struct OccupantJointCard
{
    base::EntityId id = 0;                      // unique among the ATBJNT cards
    std::string name;                           // the joint in the occupant input file, upper case
    std::array<base::EntityId, 8> grids = {};   // G0 to G7: grid points of the model
    std::array<base::EntityId, 6> elemIds = {}; // EID1 to EID6: numbers of generated elements, 0 when blank
};

} // namespace oor::model
