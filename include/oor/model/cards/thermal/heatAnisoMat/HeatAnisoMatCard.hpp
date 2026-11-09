#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MAT5 card: the anisotropic heat transfer material of a conduction element.
//
// Card layout:
//   MAT5  MID  KXX  KXY  KXZ  KYY  KYZ  KZZ  CP
//         RHO  HGEN
// The conductivity matrix is [[KXX KXY KXZ] [KXY KYY KYZ] [KXZ KYZ KZZ]]. A blank conductivity is 0.0 and a blank CP is
// 0.0; RHO and HGEN are 1.0 when blank (format reference).
struct HeatAnisoMatCard
{
    base::EntityId id = 0;
    double conductivityXx = 0.0;
    double conductivityXy = 0.0;
    double conductivityXz = 0.0;
    double conductivityYy = 0.0;
    double conductivityYz = 0.0;
    double conductivityZz = 0.0;
    double specificHeat = 0.0;         // specific heat
    double density = 1.0;              // density
    double heatGenerationFactor = 1.0; // heat generation capability (the multiplier of QVOL)
};

} // namespace oor::model
