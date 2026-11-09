#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MATT11 card: the temperature dependent properties of a MAT11. Each field holds the ID of a TABLEMi (0 when blank:
// the property does not depend on the temperature) that modifies the property of the same name on the MAT11 with the
// same MID. The format reference of this library does not describe it: the layout is the one of the MAT11, field by
// field.
//
// Layout (the fields are numbered across the lines, eight to a line):
//   MATT11  MID    T(E1)   T(E2)   T(E3)   T(NU12)   T(NU13)   T(NU23)   T(G12)     fields 1 to 8
//           T(G13) T(G23)  T(RHO)  T(A1)   T(A2)     T(A3)     (blank)   T(GE)      9 to 16
// The cell of field 15 is blank (the MAT11 keeps TREF there).
struct OrthoMat3DTemperatureCard
{
    base::EntityId id = 0; // the MAT11 that the card extends
    base::EntityId youngsModulus1TableId = 0;
    base::EntityId youngsModulus2TableId = 0;
    base::EntityId youngsModulus3TableId = 0;
    base::EntityId poissonRatio12TableId = 0;
    base::EntityId poissonRatio13TableId = 0;
    base::EntityId poissonRatio23TableId = 0;
    base::EntityId shearModulus12TableId = 0;
    base::EntityId shearModulus13TableId = 0;
    base::EntityId shearModulus23TableId = 0;
    base::EntityId densityTableId = 0;
    base::EntityId thermalExpansion1TableId = 0;
    base::EntityId thermalExpansion2TableId = 0;
    base::EntityId thermalExpansion3TableId = 0;
    base::EntityId structuralDampingTableId = 0;
};

} // namespace oor::model
