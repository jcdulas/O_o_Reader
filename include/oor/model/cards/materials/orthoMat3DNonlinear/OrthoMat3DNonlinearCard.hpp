#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace oor::model
{

// One MATORT card: a 3D orthotropic material of SOL 400 and SOL 700, with the yield condition of a plastic one (also
// read by the solid composite elements of the linear sequences).
//
// Card layout:
//   MATORT  MID       E1      E2        E3         NU12     NU23     NU31     RHO
//           G12       G23     G31       A1         A2       A3       TREF     GE
//           IYLD      IHARD   SY        (blank)    R11/m    R22/C1   R33/C2   (blank)
//           R12/C3    R23/C6  R31
//           OPTION    FILE    X1        Y1         Z1       X2       Y2       Z2
// The constants are in the material axes. NU23 and the shear moduli are meant to be given, but
// a card that gives only E1, E2, E3 and NU12 is common: a blank one is 0.0 here. The yield stress ratios Rij
// (Hill, IYLD 2) and the Barlat exponent m and coefficients Ci (IYLD 3) share their fields: R11 and m, R22 and C1, R33
// and C2, R12 and C3, R23 and C6; R31 is Hill's alone. Their defaults (1.0 for a ratio or a coefficient, 2.0 for m)
// depend on IYLD, so a blank field is kept blank.
struct OrthoMat3DNonlinearCard
{
    base::EntityId id = 0; // the material ID (the material ID space)
    double youngsModulus1 = 0.0;
    double youngsModulus2 = 0.0;
    double youngsModulus3 = 0.0;
    double poissonRatio12 = 0.0;
    double poissonRatio23 = 0.0;
    double poissonRatio31 = 0.0; // a blank NU31 is NU23: stored resolved
    double density = 0.0;
    double shearModulus12 = 0.0;
    double shearModulus23 = 0.0;
    double shearModulus31 = 0.0;
    double thermalExpansion1 = 0.0;
    double thermalExpansion2 = 0.0;
    double thermalExpansion3 = 0.0;
    double referenceTemperature = 0.0;
    double structuralDamping = 0.0;
    std::int32_t yieldCriterion = 1; // -1 elastic, 1 von Mises, 2 Hill, 3 Barlat, 4 viscoplasticity by user subroutine
    std::int32_t hardeningRule = 1;  // 1 isotropic, 2 kinematic, 3 combined work hardening
    std::optional<double> yieldStress; // the equivalent tensile yield stress; blank is the program's default
    // R11 or m, R22 or C1, R33 or C2, R12 or C3, R23 or C6, R31 (blank: the default of the yield condition)
    std::array<std::optional<double>, 6> yieldFields{};
    std::string materialAxes = "Elements"; // OPTION: VECT, ELEM, ELMAT or ELPROP
    std::string propertiesFile;            // the file of the material properties (ELMAT)
    std::array<double, 3> vector1{};       // the first vector of VECT (X1, Y1, Z1)
    std::array<double, 3> vector2{};       // the second vector of VECT (X2, Y2, Z2)
};

} // namespace oor::model
