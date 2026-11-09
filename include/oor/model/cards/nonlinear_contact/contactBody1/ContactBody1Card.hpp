#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>

namespace oor::model
{

// One BCBODY1 card: a flexible or rigid contact body (SOLs 101, 400 and 700), the new form of BCBODY. A BCONECT or a
// BCHANGE names it by its BID.
//
// Card layout: BCBODY1  BID  BPID  DIM  BEHAV  BSID  BCRGID  BCGOUT
// (SOL 700 reads the BID and the BSID only; the other fields are ignored there.)
struct ContactBody1Card
{
    base::EntityId id = 0;                      // BID: unique among BCBODY1, BCSURF and BCGRID
    base::EntityId bodyParamsId = 0;            // the BCBDPRP of the body parameters; 0 when blank (defaults)
    std::string dimension = "ThreeDimensional"; // 2D or 3D
    std::string behavior = "Deformable"; // DEFORM, RIGID, SYMM (a symmetry rigid body) or HEAT (a heat-rigid body)
    base::EntityId surfaceId = 0; // BSURF or BCPROP (deformable), BCRGSRF, BCPATCH, BCBZIER, BCNURB2 or BCNURBS (rigid)
    base::EntityId rigidMotionId = 0; // the BCRIGID of a rigid body; 0 when blank
    std::int32_t forceReferencePoint =
        0; // the reference point of the resultant contact force: -1 origin, 0 centroid of the body
};

} // namespace oor::model
