#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <string>
#include <vector>

namespace oor::model
{

// One group of a BCHANGE card: a body and a range of grid points (N1, N2, INC) of it.
struct ContactBodyChangeRange
{
    base::EntityId bodyId = 0; // IDBODi: a BCBODY or BCBODY1
    base::EntityId firstNode =
        1; // N1: the starting grid ID (a minus sign marks the first range of a patch with EXCLUDE)
    base::EntityId lastNode = 0;  // N2: the ending grid ID
    base::EntityId increment = 0; // INC: the grid ID increment; 0 when blank (the range is the two points N1 and N2)

    [[nodiscard]] bool operator==(const ContactBodyChangeRange&) const noexcept = default;
};

// One BCHANGE card: changes the definition of the deformable contact bodies in a load case (SOLs 101 and 400).
//
// Card layout: BCHANGE  ID  TYPE  IDBOD1  N1  N2  INC  IDBOD2  N1  N2  INC  ...  over as many continuation lines as
// needed. NODE gives the grid points of a body that may come into contact; EXCLUDE removes the 2-node segments (2D) or
// the 4-node (or 3-node) patches (3D) whose corners are given, a patch by two ranges in sequence: (IDBOD, -N1, N2, INC)
// then (IDBOD, N3, N4, INC). The two types may not be mixed on one card.
struct ContactBodyChangeCard
{
    base::EntityId id = 0;     // the BCONTACT or BCHANGE ID of the load case; 0 is the load case 0
    std::string kind = "Node"; // NODE or EXCLUDE
    std::vector<ContactBodyChangeRange> ranges;
};

} // namespace oor::model
