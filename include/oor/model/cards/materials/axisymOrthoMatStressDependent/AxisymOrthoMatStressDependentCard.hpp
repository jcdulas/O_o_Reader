#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MATS3 card: the nonlinear elastic (NLELAST) properties of a MAT3, the orthotropic material of the axisymmetric
// elements, in SOL 400. Each field holds the ID of a TABL3Di (0 when blank: the property is not a table): a table whose
// entry may be a function of up to four variables (strain, temperature, strain rate, ...). The TABL3D0, TABL3D1 and
// TABL3D2 cards are typed, so the IDs are references to them.
//
// Card layout:
//   MATS3  MID      TEX      TETH     TEZ    TNUXTH   TNUTHZ   TNUZX   TRHO
//          (blank)  (blank)  TGZX     TAX    TATH     TAZ
// The fields follow the MAT3 card: TGZX is field 11 (as the format reference draws it: fields 9 and 10 are blank).
struct AxisymOrthoMatStressDependentCard
{
    base::EntityId id = 0; // the MAT3 that the card extends
    base::EntityId youngsModulusXTableId = 0;
    base::EntityId youngsModulusThetaTableId = 0;
    base::EntityId youngsModulusZTableId = 0;
    base::EntityId poissonRatioXthTableId = 0;
    base::EntityId poissonRatioThzTableId = 0;
    base::EntityId poissonRatioZxTableId = 0;
    base::EntityId densityTableId = 0;
    base::EntityId shearModulusZxTableId = 0;
    base::EntityId thermalExpansionXTableId = 0;
    base::EntityId thermalExpansionThetaTableId = 0;
    base::EntityId thermalExpansionZTableId = 0;
};

} // namespace oor::model
