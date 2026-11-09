#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One BCRGSRF card: a list of rigid contact surfaces (BCPATCH, BCBZIER, BCNURB2 or BCNURBS) that a BCBODY1 names as a
// whole in its BSID (SOLs 101 and 400). Without it, a BCBODY1 may name one of those surfaces directly.
//
// Card layout: BCRGSRF  BSID  RBID1  RBID2  RBID3  ...   over as many continuation lines as needed.
struct ContactRigidSurfaceListCard
{
    base::EntityId id = 0;                // BSID: unique among the contact surfaces
    std::vector<base::EntityId> surfaces; // the RBID of each rigid surface
};

} // namespace oor::model
