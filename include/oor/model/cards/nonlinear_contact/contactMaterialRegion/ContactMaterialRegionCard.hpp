#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One BCMATL card: a 3D contact region of SOL 700 made of all the elements that have one of the listed materials; the
// BSID of a BCBODY names it. It cannot define a region of composite elements.
//
// Card layout: BCMATL  ID  IM1  IM2  IM3  ...   over as many continuation lines as needed (at least one material).
struct ContactMaterialRegionCard
{
    base::EntityId id = 0;                 // unique among BSURF, BCBOX, BCPROP and BCMATL
    std::vector<base::EntityId> materials; // the material IDs
};

} // namespace oor::model
