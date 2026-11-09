#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MATT4 card: the temperature dependent properties of a MAT4 (thermal conductivity and related properties). Each
// field holds the ID of a TABLEMj (0 when blank: the property does not depend on the temperature) whose function
// multiplies the property of the same name on the MAT4 with the same MID.
//
// Card layout:
//   MATT4  MID   T(K)   T(CP)   (blank)   T(H)   T(MU)   T(HGEN)
// The cell of field 4 is blank (the MAT4 has no table for its density: the format reference grid draws it).
//
// The MAT4 card is not typed yet: until it is, the ID is a reference to a material
// that the reference check cannot resolve.
struct HeatIsoMatTemperatureCard
{
    base::EntityId id = 0; // the MAT4 that the card extends
    base::EntityId conductivityTableId = 0;
    base::EntityId specificHeatTableId = 0;
    base::EntityId convectionCoefficientTableId = 0;
    base::EntityId viscosityTableId = 0;
    base::EntityId heatGenerationFactorTableId = 0;
};

} // namespace oor::model
