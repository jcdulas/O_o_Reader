#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PTUBE card: the property of a thin-walled cylindrical tube (CTUBE).
//
// Card layout:
//   PTUBE  PID  MID  OD  T  NSM  OD2
//
// T and OD2 have defaults that depend on OD (half of it, and OD), so they are stored resolved; `thicknessGiven` and
// `outerDiameter2Given` say whether the card gave them: a blank one follows OD and is written blank. A zero T is a
// solid circular rod. OD2, the diameter at the second grid point of the CTUBE, is used by
// heat transfer only (a tapered tube).
struct TubePropCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0;
    double outerDiameter = 0.0;
    double thickness = 0.0;
    double nonStructMass = 0.0; // nonstructural mass per unit length
    double outerDiameter2 = 0.0;
    bool thicknessGiven = true;      // false when T was blank: OD/2, following OD
    bool outerDiameter2Given = true; // false when OD2 was blank: OD, following OD
};

} // namespace oor::model
