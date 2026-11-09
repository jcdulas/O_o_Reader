#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nodes/ModuleIds.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One MDRBE2 card: a rigid body whose six independent degrees of freedom are those of one grid point and whose
// dependent degrees of freedom are components of any number of other grid points, all of which may lie in different
// modules (the multi-module counterpart of the RBE2, whose rules apply). A module is a BEGIN MODULE section of the
// deck, a part of the deck; module 0 (a blank field) is the main Bulk Data.
//
// Card layout: MDRBE2  EID  MODN  GN  CM  MOD1  GM1  MOD2  GM2
//                     MOD3  GM3  MOD4  GM4  MOD5  GM5  MOD6  GM6
//                     MOD7  GM7  MOD8  GM8  -etc.-  ALPHA  TREF
// The dependent points are pairs of a module and a grid point, one pair after the other from field 5 whatever the
// lines. ALPHA and TREF come right after the last pair, in the next two fields, and each is optional (a blank ALPHA
// with a TREF is allowed): the RBE2 tells them from the points by being reals, and so does this card, the same way.
// Both are 0.0 when absent (the format reference gives TREF that default and leaves ALPHA blank, as for the RBE2).
// The card belongs in the main Bulk Data (a rule of the deck, not of the card).
struct ModuleRigidBodyStarCard
{
    base::EntityId id = 0;
    base::EntityId independentModuleId = 0; // the module of GN, 0 for the main Bulk Data
    base::EntityId independentNode = 0;     // the grid point whose six degrees of freedom are the independent ones
    std::int32_t components = 0;       // CM: the digits of the dependent components, as one number in ascending order
    std::vector<ModuleId> dependents;  // (MODi, GMi), at least one
    double alphaCoefficient = 0.0;     // thermal expansion coefficient
    double referenceTemperature = 0.0; // reference temperature
};

} // namespace oor::model
