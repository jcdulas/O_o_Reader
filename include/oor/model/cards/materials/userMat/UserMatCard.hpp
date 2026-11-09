#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <optional>

namespace oor::model
{

// One MATUSR card: the generic properties of a user defined material of SOL 400 (hypoelastic or user material model);
// the MATUDS card with the same ID activates the user subroutine.
//
// Card layout:
//   MATUSR  MID    IPREF  IKINEM RHO    A1     A2     A3     TREF                 fields 1 to 8
//           GE     ST     SC     SS                                                9 to 12
// IPREF is 0 (the user stress-strain law is isotropic, the default) or 1 (it is given in a user defined coordinate
// system). IKINEM (0 by default) says which kinematic quantities the subroutine receives besides the strains: 0 the
// incremental strain only, 1, 2 and 3 more quantities of the deformation, 13 and 23 the same with the quantities taken
// at the middle or the end of the increment. RHO is greater than zero. A1, A2 and A3 are the thermal expansion
// coefficients, absent when blank (when A1 is given and the other two are not, the expansion is isotropic). TREF, GE,
// ST, SC and SS are 0.0 when blank. The material ID is in the space every material type shares.
struct UserMatCard
{
    base::EntityId id = 0;
    int userFrameFlag = 0;
    int kinematicQuantities = 0;
    double density = 0.0;
    std::array<std::optional<double>, 3> thermalExpansion{}; // A1, A2, A3
    double referenceTemperature = 0.0;
    double structuralDamping = 0.0;
    double tensionAllowable = 0.0;
    double compressionAllowable = 0.0;
    double shearAllowable = 0.0;
};

} // namespace oor::model
