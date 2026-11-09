#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MATSORT card: the nonlinear elastic (NLELAST) properties of a MATORT, the 3D orthotropic material, in SOL 400.
// Each field holds the ID of a TABL3Di (0 when blank: the property is not a table): a table whose entry may be a
// function of up to four variables (strain, temperature, strain rate, ...). The TABL3D0, TABL3D1 and TABL3D2 cards are
// typed, so the IDs are references to them.
//
// Card layout:
//   MATSORT  MID  TE1  TE2  TE3  TNU12  TNU23  TNU31  TRHO
//            TG12  TG23  TG31  TA1  TA2  TA3
struct OrthoMat3DNonlinearStressDependentCard
{
    base::EntityId id = 0; // the MATORT that the card extends
    base::EntityId youngsModulus1TableId = 0;
    base::EntityId youngsModulus2TableId = 0;
    base::EntityId youngsModulus3TableId = 0;
    base::EntityId poissonRatio12TableId = 0;
    base::EntityId poissonRatio23TableId = 0;
    base::EntityId poissonRatio31TableId = 0;
    base::EntityId densityTableId = 0;
    base::EntityId shearModulus12TableId = 0;
    base::EntityId shearModulus23TableId = 0;
    base::EntityId shearModulus31TableId = 0;
    base::EntityId thermalExpansion1TableId = 0;
    base::EntityId thermalExpansion2TableId = 0;
    base::EntityId thermalExpansion3TableId = 0;
};

} // namespace oor::model
