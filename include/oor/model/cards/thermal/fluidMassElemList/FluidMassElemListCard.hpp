#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One ELIST card: a list of CQUAD4 and CTRIA3 elements for the virtual fluid mass of an MFLUID.
//
// Card layout: ELIST  LID  E1  E2  E3  E4  E5  E6  E7
//                    E8   E9  -etc.-
// An item is an element ID or "first THRU last" (all the existing elements between them), kept as written in
// `elements`. When an MFLUID names the list in ELIST1 or ELIST2 the sign of an ID says which side of the element the
// fluid is on: a minus sign puts it opposite to the positive normal of the element (the right-hand rule on its corner
// points). The sign is part of the ID in the list, so the elements of a range are the ones the magnitudes name and
// both ends of a range have the same sign: "-62 THRU -70" is the elements 62 to 70, all wetted on the negative side,
// and the end of a range is not below its start in magnitude. A range may not begin or end a line of the card (the
// writer sees to it). The ID of the list is LID (IdSpace::PerCardType: the MFLUID fields ELIST1 and ELIST2 name it).
struct FluidMassElemListCard
{
    base::EntityId id = 0;
    std::vector<IdRun> elements;

    [[nodiscard]] bool operator==(const FluidMassElemListCard&) const noexcept = default;
};

} // namespace oor::model
