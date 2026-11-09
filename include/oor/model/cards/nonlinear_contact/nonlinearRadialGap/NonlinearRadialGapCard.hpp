#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One NLRGAP card: a nonlinear radial (circular) gap between two coincident grid points, a nonlinear load of the
// transient and nonlinear harmonic response (selected with NONLINEAR = SID, like the NOLIN cards).
//
// Card layout: NLRGAP  SID  GA  GB  PLANE  TABK  TABG  TABU  RADIUS
//                     GAMMA  SMVAL
// GA is the inner grid point (the shaft), GB the outer one (the housing). TABK is a table of gap stiffness against time
// or frequency when positive, and (minus) a table of gap force against penetration when negative. The second line is
// there only with the keyword GAMMA, which asks for the smoothing function (SOL 128).
struct NonlinearRadialGapCard
{
    base::EntityId setId = 0;      // the nonlinear load set (several NLRGAP cards may share it)
    base::EntityId nodeA = 0;      // the inner grid point
    base::EntityId nodeB = 0;      // the outer grid point
    std::string plane = "PlaneXy"; // XY, YZ or ZX, in the displacement systems of GA and GB
    base::EntityId stiffnessTableId =
        0; // the table of the gap stiffness (TABLED1, TABLED5); negative for force against penetration
    base::EntityId clearanceTableId = 0; // the table of the gap clearance
    base::EntityId frictionTableId = 0;  // the table of the friction coefficient
    double radius = 0.0;                 // the shaft radius (used for the friction torque)
    bool smoothing = false;              // the GAMMA keyword: use the smoothing function
    double smoothingValue = 1.0E6;       // the value of the smoothing function
};

} // namespace oor::model
