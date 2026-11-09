#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/EulerFlowBoundaryProperty.hpp"
#include "oor/model/cards/explicit/EulerSolverType.hpp"

#include <vector>

namespace oor::model
{

// One FLOWDEF card (SOL 700): the default flow boundary of an Euler mesh, which the free faces that no FLOW, FLOWT or
// other boundary names take. Without it those faces are walls. No other entry of the format reference has this name.
//
// Card layout: FLOWDEF  FID  (blank)  TYPEM                                   fields 1 to 3
//             +        TYPE1  VALUE1  TYPE2  VALUE2  ...                      fields 9 on, two to a pair, any number
// FID numbers the card. TYPEM is the kind of Eulerian material, HYDRO when blank: HYDRO, STRENGTH, MMHYDRO or MMSTREN
// (the other two words of a PEULER, 1STORDER and 2NDORDER, are refused here; the card uses the enumeration of the
// PEULER). The TYPEi and VALUEi pairs are the same as those of FLOW (a MATDEUL for MATERIAL, numbers for XVEL, YVEL,
// ZVEL, PRESSURE, DENSITY and SIE, IN, OUT or BOTH for FLOW, a HYDSTAT for HYDSTAT), except that the format lists no
// SYM for FLOW. They are kept in the order written, and a pair whose two fields are blank is not a property.
struct EulerDefaultFlowBoundaryCard
{
    base::EntityId id = 0; // unique among the FLOWDEF cards
    EulerSolverType solverType = EulerSolverType::Hydro;
    std::vector<EulerFlowBoundaryProperty> properties;
};

} // namespace oor::model
