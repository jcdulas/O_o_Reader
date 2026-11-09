#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/EulerSolverType.hpp"

namespace oor::model
{

// One PEULER card (SOL 700): the property of the Eulerian elements (CHEXA and the elements a MESH generates) that name
// its PID. The elements that reference it use the Eulerian formulation.
//
// Card layout: PEULER  PID  MID  TYPE            fields 1 to 3
// MID is the number of a MATDEUL card, the constitutive model of the material. The format reference calls it required,
// and then says that a blank or zero material makes the elements void, which the Riemann solvers refuse: the
// card reads a blank MID as 0, a void property, and refuses it for the types 1STORDER and 2NDORDER. TYPE is HYDRO when
// blank. Which material (one, or several for MMHYDRO and MMSTREN) the elements may use depends on the type;
// that is a rule of the model, not of the card.
struct EulerPropCard
{
    base::EntityId id = 0;    // unique among the properties
    base::EntityId matId = 0; // a MATDEUL, 0 for void elements
    EulerSolverType type = EulerSolverType::Hydro;
};

} // namespace oor::model
