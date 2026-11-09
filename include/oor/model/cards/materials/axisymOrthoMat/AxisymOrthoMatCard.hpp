#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MAT3 card: a linear orthotropic material for the axisymmetric elements (CTRIAX6) and the PLCOMP and PSHLN2
// entries; x, theta (the azimuthal direction) and z are the material axes.
//
// Card layout:
//   MAT3  MID  EX    ETH   EZ    NUXTH  NUTHZ  NUZX  RHO
//              (blank)  GZX   AX    ATH    AZ     TREF  GE
// The second line has its first two fields blank: GZX is field 11 (the format reference grid draws the two blank
// cells).
//
// EX, ETH, EZ and GZX are required; the three Poisson ratios, RHO, the expansion coefficients, TREF and
// GE are 0.0 when blank (the format reference gives no default for the ratios).
struct AxisymOrthoMatCard
{
    base::EntityId id = 0;
    double youngsModulusX = 0.0; // Young's moduli in the x, theta and z directions
    double youngsModulusTheta = 0.0;
    double youngsModulusZ = 0.0;
    double poissonRatioXth = 0.0; // coupled strain ratios
    double poissonRatioThz = 0.0;
    double poissonRatioZx = 0.0;
    double density = 0.0;
    double shearModulusZx = 0.0;    // shear modulus
    double thermalExpansionX = 0.0; // thermal expansion coefficients
    double thermalExpansionTheta = 0.0;
    double thermalExpansionZ = 0.0;
    double referenceTemperature = 0.0;
    double structuralDamping = 0.0; // structural damping coefficient
};

} // namespace oor::model
