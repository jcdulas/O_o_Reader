#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MATT8 card: the temperature dependent properties of a MAT8. Each field holds the ID of a TABLEMi (0 when blank:
// the property does not depend on the temperature) that modifies the property of the same name on the MAT8 with the
// same MID.
//
// Card layout:
//   MATT8  MID     T(E1)   T(E2)   T(NU12)   T(G12)   T(G1Z)   T(G2Z)   T(RHO)
//          T(A1)   T(A2)   (blank) T(XT)     T(XC)    T(YT)    T(YC)    T(S)
//          T(GE)   T(F12)
// The cell of field 11 is blank (the MAT8 keeps TREF there: the format reference grid draws it).
//
// The T(A1) and T(A2) table IDs may be negative: a negative ID is a table of thermal strains rather than of expansion
// coefficients; it names the TABLEMi that has that negative ID.
struct OrthoMat2DTemperatureCard
{
    base::EntityId id = 0; // the MAT8 that the card extends
    base::EntityId youngsModulus1TableId = 0;
    base::EntityId youngsModulus2TableId = 0;
    base::EntityId poissonRatio12TableId = 0;
    base::EntityId shearModulus12TableId = 0;
    base::EntityId shearModulus1ZTableId = 0;
    base::EntityId shearModulus2ZTableId = 0;
    base::EntityId densityTableId = 0;
    base::EntityId thermalExpansion1TableId = 0;
    base::EntityId thermalExpansion2TableId = 0;
    base::EntityId tensionStrength1TableId = 0;
    base::EntityId compressionStrength1TableId = 0;
    base::EntityId tensionStrength2TableId = 0;
    base::EntityId compressionStrength2TableId = 0;
    base::EntityId shearStrengthTableId = 0;
    base::EntityId structuralDampingTableId = 0;
    base::EntityId interactionTerm12TableId = 0;
};

} // namespace oor::model
