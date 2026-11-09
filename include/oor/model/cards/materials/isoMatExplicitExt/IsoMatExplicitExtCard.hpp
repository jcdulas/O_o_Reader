#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MAT1A card: the additional properties of a MAT1 for solid elements (SOL 700): the equation of state.
//
// Card layout:
//   MAT1A  MID  EID                                                              fields 1 and 2
// MID is the MAT1 that the card extends and EID the ID of an equation of state card (EOSxxx: the category
// EquationOfState). A MAT1A is always needed for solid elements when a MATEP is used, except when
// the RYIELD option of the MATEP is VMISES, ORTHOCR or SOIL.
struct IsoMatExplicitExtCard
{
    base::EntityId id = 0; // the MAT1 that the card extends
    base::EntityId equationOfStateId = 0;
};

} // namespace oor::model
