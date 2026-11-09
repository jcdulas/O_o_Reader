#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <optional>

namespace oor::model
{

// One MAT9 card: an anisotropic material for solid elements.
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   MAT9  MID    G11    G12    G13    G14    G15    G16    G22          fields 1 to 8
//         G23    G24    G25    G26    G33    G34    G35    G36          9 to 16
//         G44    G45    G46    G55    G56    G66    RHO    A1           17 to 24
//         A2     A3     A4     A5     A6     TREF   GE     (blank)      25 to 32
//         GE11   GE12   GE13   GE14   GE15   GE16   GE22   GE23         33 to 40   (optional)
//         GE24   GE25   GE26   GE33   GE34   GE35   GE36   GE44         41 to 48
//         GE45   GE46   GE55   GE56   GE66                              49 to 53
//
// The symmetric matrices are kept as the 21 values the card gives, in the order of the card (the upper triangle by
// rows: 11 12 13 14 15 16 22 23 24 25 26 33 34 35 36 44 45 46 55 56 66). Every blank is 0.0. The structural damping
// matrix is optional (when it is present the program ignores GE and uses it): a card without it
// has no `structuralDampingTerms`, a card with it keeps all 21 values, a blank one among them being 0.0.
struct AnisoMat3DCard
{
    base::EntityId id = 0;
    std::array<double, 21> stiffness = {}; // G11 ... G66: the material property matrix
    double density = 0.0;
    std::array<double, 6> thermalExpansion = {}; // A1 ... A6: thermal expansion coefficients
    double referenceTemperature = 0.0;
    double structuralDamping = 0.0;                               // structural damping coefficient
    std::optional<std::array<double, 21>> structuralDampingTerms; // GE11 ... GE66: the structural damping matrix
};

} // namespace oor::model
