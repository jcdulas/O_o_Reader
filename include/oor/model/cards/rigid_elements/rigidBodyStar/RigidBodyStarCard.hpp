#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One RBE2 card: a rigid body whose independent degrees of freedom are the six of one grid point and whose dependent
// degrees of freedom are some components of any number of other grid points.
//
// Card layout: RBE2  EID  GN  CM  GM1  GM2  GM3  GM4  GM5
//                   GM6  GM7  GM8  -etc.-  ALPHA  TREF
// ALPHA and TREF come right after the last GMi, in the next two fields, and each of them is optional (a blank ALPHA
// with a TREF is allowed). They are told from the grid points by being reals. Both are 0.0 when absent.
struct RigidBodyStarCard
{
    base::EntityId id = 0;
    base::EntityId independent = 0;         // GN
    std::int32_t components = 0;            // CM: the digits of the dependent components, as one number
    std::vector<base::EntityId> dependents; // GMi
    double alphaCoefficient = 0.0;          // thermal expansion coefficient
    double referenceTemperature = 0.0;      // reference temperature
};

} // namespace oor::model
