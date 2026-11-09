#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MATT5 card: the temperature dependent properties of a MAT5 (anisotropic thermal conductivity). Each field holds
// the ID of a TABLEMi (0 when blank: the property does not depend on the temperature) whose function multiplies the
// property of the same name on the MAT5 with the same MID.
//
// Card layout:
//   MATT5  MID      T(KXX)   T(KXY)   T(KXZ)   T(KYY)   T(KYZ)   T(KZZ)   T(CP)
//          (blank)  T(HGEN)
// T(HGEN) is field 10 (the format reference grid draws the blank cell before it). This card follows the format
// reference, whose fields are those of the MAT5 one for one.
//
// The MAT5 card is not typed yet: until it is, the ID is a reference to a material
// that the reference check cannot resolve.
struct HeatAnisoMatTemperatureCard
{
    base::EntityId id = 0; // the MAT5 that the card extends
    base::EntityId conductivityXxTableId = 0;
    base::EntityId conductivityXyTableId = 0;
    base::EntityId conductivityXzTableId = 0;
    base::EntityId conductivityYyTableId = 0;
    base::EntityId conductivityYzTableId = 0;
    base::EntityId conductivityZzTableId = 0;
    base::EntityId specificHeatTableId = 0;
    base::EntityId heatGenerationFactorTableId = 0;
};

} // namespace oor::model
