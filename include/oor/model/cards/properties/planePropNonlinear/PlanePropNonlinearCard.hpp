#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PLPLANE card: the property of a fully nonlinear plane strain, plane stress or axisymmetric element.
//
// Card layout:
//   PLPLANE  PID  MID  CID  STR
//
// CID is the coordinate system that gives the plane of deformation (0, the default, is the basic system). A negative
// CID selects an axes combination for axisymmetric elements and refers to no system. STR is the
// location of the stress and strain output, GRID (the default) or GAUS.
struct PlanePropNonlinearCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0;
    base::EntityId frameId = 0;
    bool gaussOutput = false; // STR = GAUS (true) or GRID (false)
};

} // namespace oor::model
