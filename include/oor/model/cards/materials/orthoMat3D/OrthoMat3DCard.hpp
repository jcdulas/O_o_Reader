#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MAT11 card: an orthotropic material for solid elements (the format reference of this library does not describe
// it).
//
// Layout (the fields are numbered across the lines, eight to a line):
//   MAT11  MID   E1    E2    E3    NU12  NU13  NU23  G12      fields 1 to 8
//          G13   G23   RHO   A1    A2    A3    TREF  GE       9 to 16
//
// E1, E2, E3, the three Poisson ratios and the three shear moduli are required; RHO, the
// expansion coefficients, TREF and GE are 0.0 when blank.
struct OrthoMat3DCard
{
    base::EntityId id = 0;
    double youngsModulus1 = 0.0; // Young's moduli along the three material axes
    double youngsModulus2 = 0.0;
    double youngsModulus3 = 0.0;
    double poissonRatio12 = 0.0; // Poisson's ratios
    double poissonRatio13 = 0.0;
    double poissonRatio23 = 0.0;
    double shearModulus12 = 0.0; // shear moduli
    double shearModulus13 = 0.0;
    double shearModulus23 = 0.0;
    double density = 0.0;
    double thermalExpansion1 = 0.0; // thermal expansion coefficients
    double thermalExpansion2 = 0.0;
    double thermalExpansion3 = 0.0;
    double referenceTemperature = 0.0;
    double structuralDamping = 0.0; // structural damping coefficient
};

} // namespace oor::model
