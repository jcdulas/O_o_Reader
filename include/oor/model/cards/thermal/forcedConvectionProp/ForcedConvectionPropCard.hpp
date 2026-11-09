#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PCONVM card: the forced convection property of the surface elements a CONVM names.
//
// Card layout: PCONVM  PCONID  MID  FORM  FLAG  COEF  EXPR  EXPPI  EXPPO
// MID is the MAT4 of the working fluid (its conductivity, heat capacity and viscosity). FORM is 0 (blank), 1, 10, 11,
// 20 or 21, FLAG is 0 (blank) or 1, COEF is required, EXPR, EXPPI and EXPPO are 0.0 when blank.
struct ForcedConvectionPropCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0;
    int matrixForm = 0;
    int massFlowConvection = 0;
    double coefficient = 0.0;
    double reynoldsExponent = 0.0;
    double prandtlExponentInner = 0.0;
    double prandtlExponentOuter = 0.0;
};

} // namespace oor::model
