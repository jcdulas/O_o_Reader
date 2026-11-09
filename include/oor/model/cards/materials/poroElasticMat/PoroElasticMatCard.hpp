#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One MATPE1 card: an isotropic poroelastic material (the solid skeleton and the fluid of a porous medium), referenced
// by the PSOLID of a poroelastic solid. It goes in the Bulk Data of a TRIM component.
//
// Card layout:
//   MATPE1  MID    MAT1   MAT10    BIOT   POROPT  SRHO
//           VISC   GAMMA  PRANDTL  POR    TOR     AFR    VLE    TLE
// POROPT picks a simplified model: LUMPED (a skeleton assumed extremely soft: its density is SRHO or the one of the
// MAT1), RIGID (a rigid skeleton, MAT1 left blank), MIKI or DELANY (semi-empirical, porosity 1, MAT1 left blank, VISC
// and POR not needed).
struct PoroElasticMatCard
{
    base::EntityId id = 0;
    base::EntityId solidMatId = 0;    // the MAT1 of the skeleton (0 when blank: RIGID, MIKI and DELANY)
    base::EntityId fluidMatId = 0;    // the MAT10 of the fluid
    double biotFactor = 1.0;          // the Biot factor
    std::string simplifiedModel;      // LUMPED, RIGID, MIKI or DELANY; blank for the full model
    double solidDensity = 0.0;        // the solid density of LUMPED
    double viscosity = 0.0;           // the dynamic viscosity of the fluid (0.0 when blank)
    double specificHeatRatio = 1.402; // the ratio of specific heats of the fluid
    double prandtlNumber = 0.71;      // the Prandtl number of the fluid
    double porosity = 0.0;            // the porosity (0.0 when blank)
    double tortuosity = 1.0;          // the tortuosity
    double flowResistivity = 0.0;     // the air flow resistivity
    double viscousLength = 0.0;       // the viscous characteristic length
    double thermalLength = 0.0;       // the thermal characteristic length
};

} // namespace oor::model
