#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One CYSUP card: the fictitious supports of a cyclic symmetry model. They keep the model from moving as a rigid body
// in a static analysis, or define its rigid body modes in an eigenvalue analysis, and act on the cyclic components of
// order 0 and 1 only.
//
// Card layout: CYSUP  GID  C
// GID is the supported grid point (never a scalar point) and C the digits of the supported components as
// one number, 1 to 6 each at most once and at least one (the digits are kept in ascending order). The format reference
// allows one supported point in the model and says where it may sit and how its displacement system must be oriented
// for each symmetry option; those are rules between this card, the CYSYM and the grid point, which the card does not
// check.
struct CyclicSupportCard
{
    base::EntityId nodeId = 0;
    std::int32_t components = 0;
};

} // namespace oor::model
