#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"

namespace oor::model
{

// One SNORM card: the surface normal vector at a grid point, for the CQUAD4, CQUADR, CTRIA3 and CTRIAR shell elements.
// It overrides any grid point normal that the SNORM parameter generates; for every other element the normal
// is ignored.
//
// Card layout:
//   SNORM  GID  CID  N1  N2  N3
//
// GID is the grid point (unique among the SNORM cards: one normal per grid point), CID the coordinate system of the
// components (0, the default, is the basic system) and N1, N2 and N3 the components, which need not make a unit vector;
// a blank one is 0.0. In a cylindrical or spherical system the components are in the tangent system at the grid point.
struct ShellNormalCard
{
    base::EntityId nodeId = 0;
    base::EntityId frameId = 0; // 0 = the basic coordinate system
    base::Vec3 normal;          // N1 N2 N3 in the system CID
};

} // namespace oor::model
