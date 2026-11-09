#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/EulerSolverType.hpp"

namespace oor::model
{

// One PEULER1 card (SOL 700): the property of Eulerian elements whose initial conditions are given in geometric regions
// (a TICEUL1) instead of one material, as a PEULER does. Elements (CHEXA and the elements a MESH generates) name its
// PID. It shares the name of no other entry: PEULER is the card with a single material.
//
// Card layout: PEULER1  PID  TYPE  SID  SID2            fields 1 to 4
// TYPE is the same list of solver types as PEULER (HYDRO when blank): the rules of the PEULER apply here too.
// SID is the number of the TICEUL1 that gives the materials and the regions (ReferenceTarget::EulerInitialization: it
// is resolved and renumbered with the TICEUL1 cards). SID2 is the number of
// an EULFOR1 that defines the acceleration field, 0 or blank for none.
struct EulerRegionPropCard
{
    base::EntityId id = 0; // unique among the properties
    EulerSolverType type = EulerSolverType::Hydro;
    base::EntityId initialRegionsId = 0; // a TICEUL1
    base::EntityId bodyForceSetId = 0;   // an EULFOR1, 0 for none
};

} // namespace oor::model
