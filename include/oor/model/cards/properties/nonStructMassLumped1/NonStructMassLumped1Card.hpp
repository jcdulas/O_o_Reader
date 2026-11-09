#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/NonStructMassFields.hpp"

#include <string>

namespace oor::model
{

// One NSML1 card: the alternate form of NSML, one lumped non-structural mass shared by a list of properties or
// elements.
//
// Card layout: NSML1  SID  TYPE  VALUE  ID  ID  ID  ID  ID  -etc.-
//        or:  NSML1  SID  TYPE  VALUE  ID1  "THRU"  ID2  ["BY"  N]  ID3  "THRU"  ID4  ...
//        or:  NSML1  SID  TYPE  VALUE  "ALL"
// TYPE is as for NSM. VALUE is a mass, distributed over all the elements of the list together in proportion to their
// area (shells) or length (bars, beams, rods, tubes): the mass added is VALUE. An element that appears more than once
// in the list counts its area or length each time. The IDs are kept as written, as in NSM1.
struct NonStructMassLumped1Card
{
    base::EntityId id = 0;
    std::string type;
    double nonStructMass = 0.0;
    NonStructMassIdList ids;
};

} // namespace oor::model
