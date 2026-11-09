#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One NLBSH3D card: a three-dimensional nonlinear load between two coincident grid points, the force being a function
// of their relative displacement and velocity and of the rotor speed (a nonlinear load of the rotor dynamics, selected
// with NONLINEAR = SID like the NOLIN cards and the NLRGAP).
//
// Card layout:  NLBSH3D  SID  GA  GB  (blank)  LID-1  LID-2        fields 1 to 3, 5 and 6
// SID is the nonlinear load set (several cards may share it, each with its own pair of grid points).
// GA is the inner grid point and GB the outer one; both are required, and they are different points (the format says
// they must be coincident, have parallel displacement systems and all six degrees of freedom in the D-set, which are
// rules of the model that the card does not look at). LID-1 is the LBSH3DG that gives the force against the deflection,
// LID-2 the LBSH3DG for the force against the velocity; both are required, and several cards may name the same LBSH3DG.
// Field 4 is a blank cell of the grid.
struct NonlinearBushLoad3DCard
{
    base::EntityId setId = 0;   // the nonlinear load set
    base::EntityId nodeA = 0;   // the inner grid point
    base::EntityId nodeB = 0;   // the outer grid point
    base::EntityId loadId1 = 0; // the LBSH3DG of the force against the deflection (LID-1)
    base::EntityId loadId2 = 0; // the LBSH3DG of the force against the velocity (LID-2)
};

} // namespace oor::model
