#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>
#include <string>

namespace oor::model
{

// One MFLUID card: the properties of an incompressible fluid volume, for the virtual mass matrix.
//
// Card layout:
//   MFLUID  SID  CID  ZFS  RHO  ELIST1  ELIST2  PLANE1  PLANE2
//           RMAX FMEXACT
// SID is the set the MFLUID Case Control command selects, CID the rectangular system of the free surface (0 when
// blank), ZFS the intercept of the free surface on its X3 axis (absent when blank: the surface is infinitely high), RHO
// the density of the fluid, ELIST1 and ELIST2 the ELIST cards of the two-dimensional elements wetted on one side and on
// both sides (0 when blank, not both 0; an ELIST card), PLANE1 and PLANE2 S, A or N (symmetric,
// antisymmetric, neither). RMAX (1.0E10) is the characteristic length and FMEXACT (1.0E-15) the factor of the exact
// integration distance.
struct FluidVirtualMassCard
{
    base::EntityId id = 0;
    base::EntityId frameId = 0;
    std::optional<double> freeSurfaceZ;
    double density = 0.0;
    base::EntityId oneSideElemListId = 0;
    base::EntityId bothSidesElemListId = 0;
    std::string planeSymmetry1 = "NoSymmetry";
    std::string planeSymmetry2 = "NoSymmetry";
    double characteristicLength = 1.0e10;
    double exactIntegrationRatio = 1.0e-15;
};

} // namespace oor::model
