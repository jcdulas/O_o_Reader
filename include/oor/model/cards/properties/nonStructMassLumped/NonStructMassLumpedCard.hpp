#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/NonStructMassFields.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One NSML card: a set of lumped non-structural mass, given as a total on properties or elements.
//
// Card layout: NSML  SID  TYPE  ID1  VALUE1  ID2  VALUE2  ID3  VALUE3
//                        ID4   VALUE4  -etc.-
// TYPE is as for NSM. VALUE is a mass, not a mass per unit: it is distributed over the elements of the property (or the
// element) in proportion to their area (shells) or length (bars, beams, rods, tubes), so that the mass added is VALUE.
// Several cards may share the SID.
struct NonStructMassLumpedCard
{
    base::EntityId id = 0;
    std::string type;
    std::vector<NonStructMassPair> pairs;
};

} // namespace oor::model
