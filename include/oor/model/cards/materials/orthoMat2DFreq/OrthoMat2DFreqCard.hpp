#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MAT8F card: the frequency dependence of the properties of a MAT8 (a shell element orthotropic material). Each
// field holds the ID of a TABLEDi (0 when blank: the property does not depend on the frequency) that replaces the
// property of the MAT8 with the same MID.
//
// Card layout (the fields sit where the properties sit on the MAT8):
//   MAT8F  MID  E1  E2  NU12  G12  G1Z  G2Z                                      fields 1 to 7
//          (blank line)
//          GE                                                                     the third line: field 17
// GE must be blank when the GE of the MAT8 is 0.0.
struct OrthoMat2DFreqCard
{
    base::EntityId id = 0; // the MAT8 that the card extends
    base::EntityId youngsModulus1TableId = 0;
    base::EntityId youngsModulus2TableId = 0;
    base::EntityId poissonRatio12TableId = 0;
    base::EntityId shearModulus12TableId = 0;
    base::EntityId shearModulus1ZTableId = 0;
    base::EntityId shearModulus2ZTableId = 0;
    base::EntityId structuralDampingTableId = 0;
};

} // namespace oor::model
