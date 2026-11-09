#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PSHEAR card: the property of a shear panel (CSHEAR).
//
// Card layout:
//   PSHEAR  PID  MID  T  NSM  F1  F2
//
// F1 and F2 are the effectiveness factors for the extensional stiffness along the edges 1-2 and 3-4, and 2-3 and 1-4
// (0.0, the default, gives the panel no extensional stiffness).
struct ShearPanelPropCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0;
    double thickness = 0.0;
    double nonStructMass = 0.0; // nonstructural mass per unit area
    double extensionalFactor1 = 0.0;
    double extensionalFactor2 = 0.0;
};

} // namespace oor::model
