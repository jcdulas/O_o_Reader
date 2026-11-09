#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CONROD card: a rod element with its own area and torsional constant, no property entry.
//
// Card layout:
//   CONROD  EID  G1  G2  MID  A  J  C  NSM
//
// A, J, C and NSM are blank (0.0) when the card gives no value.
struct Rod2InlineSectionCard
{
    base::EntityId id = 0;
    std::array<base::EntityId, 2> nodes = {};
    base::EntityId matId = 0;              // the material (a MAT1 in structural problems)
    double area = 0.0;                     // area of the rod
    double torsionConstant = 0.0;          // torsional constant
    double torsionStressCoefficient = 0.0; // coefficient for torsional stress
    double nonStructMass = 0.0;            // nonstructural mass per unit length
};

} // namespace oor::model
