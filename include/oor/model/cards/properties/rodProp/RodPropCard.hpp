#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PROD card: the property of a rod element.
//
// Card layout:
//   PROD  PID  MID  A  J  C  NSM
struct RodPropCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0;
    double area = 0.0;              // A
    double torsion = 0.0;           // J
    double stressCoefficient = 0.0; // C
    double nonStructMass = 0.0;
};

} // namespace oor::model
