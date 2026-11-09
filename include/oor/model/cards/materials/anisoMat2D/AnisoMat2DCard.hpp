#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One MAT2 card: an anisotropic material for two-dimensional elements.
//
// Card layout:
//   MAT2  MID    G11   G12   G13   G22   G23   G33   RHO
//         A1     A2    A3    TREF  GE    ST    SC    SS
//         MCSID  GE11  GE12  GE13  GE22  GE23  GE33
//
// The symmetric matrices are kept as the six values the card gives, in the order of the card (11, 12, 13, 22, 23, 33).
// Every blank is 0.0 (the stress limits and the expansion coefficients included, as in MAT1).
struct AnisoMat2DCard
{
    base::EntityId id = 0;
    std::array<double, 6> stiffness = {}; // G11 G12 G13 G22 G23 G33: the material property matrix
    double density = 0.0;
    std::array<double, 3> thermalExpansion = {}; // A1 A2 A3: thermal expansion coefficients
    double referenceTemperature = 0.0;
    double structuralDamping = 0.0; // structural damping coefficient
    double tensionAllowable = 0.0;  // stress limits (tension, compression, shear)
    double compressionAllowable = 0.0;
    double shearAllowable = 0.0;
    base::EntityId stressFrameId = 0;                  // material coordinate system, 0 for none
    std::array<double, 6> structuralDampingTerms = {}; // GE11 GE12 GE13 GE22 GE23 GE33: the structural damping matrix
};

} // namespace oor::model
