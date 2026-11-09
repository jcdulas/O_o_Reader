#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One scalar spring property of a PELAS card.
//
// Card layout (a card defines one or two properties, each of four fields):
//   PELAS  PID1  K1  GE1  S1  PID2  K2  GE2  S2
//
// A card with two properties gives two SpringPropCard values, each written as a card of its own, so that every property
// can be renumbered or deleted alone (as a CORD1R with two systems is).
struct SpringPropCard
{
    base::EntityId id = 0;
    double stiffness = 0.0;         // stiffness
    double structuralDamping = 0.0; // damping coefficient
    double stressCoefficient = 0.0; // stress coefficient
};

} // namespace oor::model
