#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One BCELIPS card: a list of ellipsoid names for the contact of SOL 700 (the ellipsoids of the occupant dummies).
//
// Card layout: BCELIPS  CID  NAME1  NAME2  NAME3  ...   over as many continuation lines as needed.
struct ContactEllipsoidListCard
{
    base::EntityId id = 0;                   // CID: unique among the contact surfaces
    std::vector<std::string> ellipsoidNames; // the ellipsoid names, as given
};

} // namespace oor::model
