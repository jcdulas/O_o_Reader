#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One MDRROD card: a pin-ended element that is rigid in translation and connects two grid points that lie in two
// modules (the multi-module counterpart of the RROD, whose rules apply). A module is a BEGIN MODULE section of the
// deck, a part of the deck; module 0 (a blank field) is the main Bulk Data.
//
// Card layout: MDRROD  EID  MODA  GA  MODB  GB  CMA  CMB  ALPHA
//                     TREF
// CMA and CMB name the one dependent translational component (1, 2 or 3), at GA or at GB: one of them holds it and the
// other is blank (0). With the Lagrange method both may be blank (the format reference says so for the MDRROD, as for
// the RROD). ALPHA is the thermal expansion coefficient (0.0 when blank), TREF the reference temperature (0.0
// when blank, the format reference default); TREF is the first field of the continuation line. a common deck puts ALPHA
// in field 8, after a blank CMB. The card belongs in the main Bulk Data (a rule of the deck, not of the
// card).
struct ModuleRigidRodCard
{
    base::EntityId id = 0;
    base::EntityId moduleIdA = 0; // the module of GA, 0 for the main Bulk Data
    base::EntityId nodeA = 0;
    base::EntityId moduleIdB = 0; // the module of GB
    base::EntityId nodeB = 0;
    std::int32_t componentsAtA = 0;
    std::int32_t componentsAtB = 0;
    double alphaCoefficient = 0.0;
    double referenceTemperature = 0.0;
};

} // namespace oor::model
