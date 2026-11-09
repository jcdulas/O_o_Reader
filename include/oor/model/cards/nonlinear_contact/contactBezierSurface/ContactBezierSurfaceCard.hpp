#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One BCBZIER card: a rigid contact body made of a Bezier surface (SOLs 101 and 400). A BCRGSRF or a BCBODY1 names it
// by its RBID.
//
// Card layout: BCBZIER  RBID  NP1  NP2  NSUB1  NSUB2  G1  G2  G3  G4 ...  over as many continuation lines as needed.
// The NP1 x NP2 control points are given NP1 for the first value of the second direction, then NP1 for the second, and
// so on. For a mesh of 2 x 2 the normal order is 1,2,3,4 and the reversed order 2,1,4,3 (it turns the interior side).
struct ContactBezierSurfaceCard
{
    base::EntityId id = 0;             // RBID: unique among the contact surfaces
    std::int32_t pointCount1 = 0;      // the points in the first direction
    std::int32_t pointCount2 = 0;      // the points in the second direction
    std::int32_t subdivisions1 = 0;    // the subdivisions in the first direction
    std::int32_t subdivisions2 = 0;    // the subdivisions in the second direction
    std::vector<base::EntityId> nodes; // NP1 x NP2 grid points
};

} // namespace oor::model
