#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One MATT9 card: the temperature dependent properties of a MAT9. Each field holds the ID of a TABLEMk (0 when blank:
// the property does not depend on the temperature) that modifies the property of the same name on the MAT9 with the
// same MID.
//
// Card layout (the fields are numbered across the lines, eight to a line, as the MAT9 card):
//   MATT9  MID       T(G11)   T(G12)   T(G13)   T(G14)   T(G15)   T(G16)   T(G22)    fields 1 to 8
//          T(G23)    T(G24)   T(G25)   T(G26)   T(G33)   T(G34)   T(G35)   T(G36)    9 to 16
//          T(G44)    T(G45)   T(G46)   T(G55)   T(G56)   T(G66)   T(RHO)   T(A1)     17 to 24
//          T(A2)     T(A3)    T(A4)    T(A5)    T(A6)    (blank)  T(GE)    (blank)   25 to 32
//          T(GE11)   T(GE12)  T(GE13)  T(GE14)  T(GE15)  T(GE16)  T(GE22)  T(GE23)   33 to 40
//          T(GE24)   T(GE25)  T(GE26)  T(GE33)  T(GE34)  T(GE35)  T(GE36)  T(GE44)   41 to 48
//          T(GE45)   T(GE46)  T(GE55)  T(GE56)  T(GE66)                              49 to 53
// The cells of fields 30 (the MAT9 keeps TREF there) and 32 are blank.
//
// The groups are kept as the 21 values the card gives, in the order of the card (the upper triangle by rows: 11 12 13
// 14 15 16 22 23 24 25 26 33 34 35 36 44 45 46 55 56 66).
struct AnisoMat3DTemperatureCard
{
    base::EntityId id = 0;                                 // the MAT9 that the card extends
    std::array<base::EntityId, 21> stiffnessTableIds = {}; // T(G11) ... T(G66)
    base::EntityId densityTableId = 0;
    std::array<base::EntityId, 6> thermalExpansionTableIds = {}; // T(A1) ... T(A6)
    base::EntityId structuralDampingTableId = 0;
    std::array<base::EntityId, 21> structuralDampingTermTableIds = {}; // T(GE11) ... T(GE66)
};

} // namespace oor::model
