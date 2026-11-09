#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One SHREL card (SOL 700): an elastic shear model with a constant shear modulus. A MATDEUL names it in its SID field.
//
// Card layout: SHREL  SID  G            fields 1 and 2
// G is the shear modulus, zero when blank; the format gives no range for it, so the card checks only that it is finite.
// SID must be unique among all the shear model cards (SHREL, SHRPOL, SHRUDS), which the category of the card expresses.
struct ElasticShearModelCard
{
    base::EntityId id = 0;     // unique among the shear models
    double shearModulus = 0.0; // shear modulus
};

} // namespace oor::model
