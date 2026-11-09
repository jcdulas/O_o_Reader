#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MATT1 card: the temperature dependent properties of a MAT1. Each field holds the ID of a TABLEMi (0 when blank:
// the property does not depend on the temperature) that modifies the property of the same name on the MAT1 with the
// same MID.
//
// Card layout:
//   MATT1  MID   T(E)   T(G)   T(NU)   T(RHO)   T(A)   (blank)   T(GE)
//          T(ST) T(SC)  T(SS)
// The cell of field 7 is blank (the format reference grid draws it).
//
// The T(A) table ID may be negative: a negative ID is a table of thermal strains rather than of expansion coefficients;
// it names the TABLEMi that has that negative ID.
struct IsoMatTemperatureCard
{
    base::EntityId id = 0; // the MAT1 that the card extends
    base::EntityId youngsModulusTableId = 0;
    base::EntityId shearModulusTableId = 0;
    base::EntityId poissonRatioTableId = 0;
    base::EntityId densityTableId = 0;
    base::EntityId thermalExpansionTableId = 0;
    base::EntityId structuralDampingTableId = 0;
    base::EntityId tensionAllowableTableId = 0;
    base::EntityId compressionAllowableTableId = 0;
    base::EntityId shearAllowableTableId = 0;
};

} // namespace oor::model
