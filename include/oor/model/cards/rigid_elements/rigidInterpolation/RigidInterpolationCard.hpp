#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// A weighted group of an RBE3 card: components Ci of the grid points Gi,j count with the weight WTi.
struct RigidInterpolationGroup
{
    double weight = 0.0;               // WTi
    std::int32_t components = 0;       // Ci: the digits of the components, as one number
    std::vector<base::EntityId> nodes; // Gi,1 Gi,2 ...

    [[nodiscard]] bool operator==(const RigidInterpolationGroup&) const noexcept = default;
};

// A dependent degree of freedom of an RBE3 card listed after "UM": components CMi of grid point GMi.
struct RigidInterpolationDependent
{
    base::EntityId node = 0;     // GMi
    std::int32_t components = 0; // CMi

    [[nodiscard]] bool operator==(const RigidInterpolationDependent&) const noexcept = default;
};

// One RBE3 card: the motion of a reference grid point is the weighted average of the motion of other grid points.
//
// Card layout: RBE3  EID  (blank)  REFGRID  REFC  WT1  C1  G1,1  G1,2
//                   G1,3  WT2  C2  G2,1  G2,2  -etc.-
//                   "UM"  GM1  CM1  GM2  CM2  GM3  CM3
//                   "ALPHA"  ALPHA  TREF
// The groups (WTi, Ci, Gi,j...) follow each other from field 5 on with no padding: a group ends where the next real
// (the next WT) or a keyword starts. "UM" and "ALPHA" each begin a line, and both parts are
// optional. Without "UM" only the REFC components of the reference point are dependent. ALPHA and TREF are 0.0 when
// absent. Reading skips blank fields; writing packs the fields, so a card comes back as written by the library.
struct RigidInterpolationCard
{
    base::EntityId id = 0;
    base::EntityId referenceNode = 0; // REFGRID
    std::int32_t refComponents = 0;   // REFC
    std::vector<RigidInterpolationGroup> groups;
    std::vector<RigidInterpolationDependent> dependents; // the UM list; empty when there is no "UM"
    double alphaCoefficient = 0.0;
    double referenceTemperature = 0.0;
};

} // namespace oor::model
