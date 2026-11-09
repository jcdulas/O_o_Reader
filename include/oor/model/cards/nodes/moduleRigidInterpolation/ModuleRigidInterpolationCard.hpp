#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nodes/ModuleIds.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// A weighted group of an MDRBE3 card: the components Ci of the grid points Gi,j (each in the module MODi,j) count with
// the weight WTi.
struct ModuleRigidInterpolationGroup
{
    double weight = 0.0;          // WTi
    std::int32_t components = 0;  // Ci: the digits of the components, as one number in ascending order
    std::vector<ModuleId> points; // (MODi,j, Gi,j), at least one

    [[nodiscard]] bool operator==(const ModuleRigidInterpolationGroup&) const noexcept = default;
};

// A dependent degree of freedom listed after "UM": the components CMk of the grid point GMk, which is in the module
// MODk.
struct ModuleRigidInterpolationDependent
{
    ModuleId point;              // (MODk, GMk)
    std::int32_t components = 0; // CMk

    [[nodiscard]] bool operator==(const ModuleRigidInterpolationDependent&) const noexcept = default;
};

// One MDRBE3 card: the motion of a reference grid point is the weighted average of the motion of other grid points, all
// of which may lie in different modules (the multi-module counterpart of the RBE3, whose rules apply). A module is a
// BEGIN MODULE section of the deck, a part of the deck; module 0 (a blank field) is the main Bulk Data.
//
// Card layout: MDRBE3  EID  REFMOD  REFGRID  REFC  WT1  C1  MOD1,1  G1,1
//                     MOD1,2  G1,2  MOD1,3  G1,3  WT2  C2  MOD2,1  G2,1
//                     MOD2,2  G2,2  -etc.-  WT3  C3  MOD3,1  G3,1  MOD3,2
//                     ...
//                     "UM"  MOD1  GM1  CM1  MOD2  GM2  CM2
//                           MOD3  GM3  CM3  -etc.-
//                     "ALPHA"  ALPHA  TREF
// The groups follow each other from field 5 with no padding: a group is WTi, Ci and the pairs of a module and a grid
// point up to the next real (the next WTi) or a keyword, whatever the lines (the "-etc." cells of the format stand for
// the pairs that follow). "UM" and "ALPHA" each begin a line, as the RBE3 writes them, and both parts are optional. The
// UM part has two triples of a module, a grid point and components on each line: the keyword (on its first line) or a
// blank takes the first field, the triples the fields 2 to 4 and 5 to 7, and the last field is blank. Without "UM" only
// the REFC components of the reference point are dependent. ALPHA and TREF are 0.0 when absent. Reading skips blank
// fields between the groups; writing packs the fields and writes every module (a blank module in the middle of a group
// would read as padding), so a card comes back as the library writes it. The card belongs in the main Bulk
// Data (a rule of the deck, not of the card).
struct ModuleRigidInterpolationCard
{
    base::EntityId id = 0;
    base::EntityId referenceModuleId = 0;                      // the module of REFGRID, 0 for the main Bulk Data
    base::EntityId referenceNode = 0;                          // REFGRID
    std::int32_t referenceComponents = 0;                      // REFC
    std::vector<ModuleRigidInterpolationGroup> groups;         // at least one
    std::vector<ModuleRigidInterpolationDependent> dependents; // the UM list; empty when there is no "UM"
    double alphaCoefficient = 0.0;
    double referenceTemperature = 0.0;
};

} // namespace oor::model
