#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One MATT2 card: the temperature dependent properties of a MAT2. Each field holds the ID of a TABLEMi (0 when blank:
// the property does not depend on the temperature) that modifies the property of the same name on the MAT2 with the
// same MID.
//
// Card layout:
//   MATT2  MID      T(G11)   T(G12)   T(G13)   T(G22)   T(G23)   T(G33)   T(RHO)
//          T(A1)    T(A2)    T(A3)    (blank)  T(GE)    T(ST)    T(SC)    T(SS)
//          (blank)  T(GE11)  T(GE12)  T(GE13)  T(GE22)  T(GE23)  T(GE33)
// The cells of fields 12 and 17 are blank (the format reference grid draws them). The three groups follow the order of
// the MAT2 card: (11, 12, 13, 22, 23, 33) for the matrices.
//
// The T(Ai) table IDs may be negative: a negative ID is a table of thermal strains rather than of expansion
// coefficients; it names the TABLEMi that has that negative ID.
struct AnisoMat2DTemperatureCard
{
    base::EntityId id = 0;                                // the MAT2 that the card extends
    std::array<base::EntityId, 6> stiffnessTableIds = {}; // T(G11) T(G12) T(G13) T(G22) T(G23) T(G33)
    base::EntityId densityTableId = 0;
    std::array<base::EntityId, 3> thermalExpansionTableIds = {}; // T(A1) T(A2) T(A3)
    base::EntityId structuralDampingTableId = 0;
    base::EntityId tensionAllowableTableId = 0;
    base::EntityId compressionAllowableTableId = 0;
    base::EntityId shearAllowableTableId = 0;
    std::array<base::EntityId, 6> structuralDampingTermTableIds = {}; // T(GE11) T(GE12) T(GE13) T(GE22) T(GE23) T(GE33)
};

} // namespace oor::model
