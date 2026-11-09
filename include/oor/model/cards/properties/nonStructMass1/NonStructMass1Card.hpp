#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/NonStructMassFields.hpp"

#include <string>

namespace oor::model
{

// One NSM1 card: the alternate form of NSM, one value of non-structural mass for a list of properties or elements.
//
// Card layout: NSM1  SID  TYPE  VALUE  ID  ID  ID  ID  ID  -etc.-
//        or:  NSM1  SID  TYPE  VALUE  ID1  "THRU"  ID2  ["BY"  N]  ID3  "THRU"  ID4  ...
//        or:  NSM1  SID  TYPE  VALUE  "ALL"
// TYPE is as for NSM. VALUE is the mass per unit area (shells, shear panels) or per unit length (bars, beams, rods,
// tubes) added to every property or element of the list. ALL is every property of the family of TYPE (PCOMP and PCOMPG
// are PSHELL, PBARL is PBAR, PBEAML and PBCOMP are PBEAM) or, with TYPE ELEMENT, every element. The list
// is kept as written: single IDs and THRU ranges with their BY increment.
struct NonStructMass1Card
{
    base::EntityId id = 0;
    std::string type;
    double nonStructMass = 0.0;
    NonStructMassIdList ids;
};

} // namespace oor::model
