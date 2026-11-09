#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One RTRPLT card: a rigid triangular plate.
//
// Card layout: RTRPLT  EID  GA  GB  GC  CNA  CNB  CNC
//                     CMA  CMB  CMC  ALPHA  TREF
// The first line ends at CNC (field 8 is blank) and CMA is the first field of the continuation line. CNi are the
// independent components at the three grid points, CMi the dependent ones, each the digits 1 to 6 as one number (0 when
// blank: the format reference gives the default 0 of CMi, so the dependent components are taken as written). With CNA,
// CNB and CNC all blank, CNA is 123456 (the format calls it the default), stored resolved. ALPHA is the thermal
// expansion coefficient (0.0 when blank), TREF the reference temperature (0.0 when blank). The rigid elements share the
// ID space of the elements.
struct RigidTriaPlateCard
{
    base::EntityId id = 0;
    std::array<base::EntityId, 3> nodes{0, 0, 0};     // GA, GB, GC
    std::array<std::int32_t, 3> independent{0, 0, 0}; // CNA, CNB, CNC
    std::array<std::int32_t, 3> dependent{0, 0, 0};   // CMA, CMB, CMC
    double alphaCoefficient = 0.0;
    double referenceTemperature = 0.0;
};

} // namespace oor::model
