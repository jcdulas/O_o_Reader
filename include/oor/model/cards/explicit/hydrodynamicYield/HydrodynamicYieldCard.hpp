#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One YLDHY card (SOL 700): the hydrodynamic yield model, a yield model whose yield stress is zero. It is meant for
// fluids, which have no shear strength. A MATDEUL names it in its YID field (a MATDEUL with a blank YID is hydrodynamic
// as well).
//
// Card layout: YLDHY  YID            field 1
// The card has nothing but its number.
struct HydrodynamicYieldCard
{
    base::EntityId id = 0; // unique among the yield models (every YLDxxx card shares the category)
};

} // namespace oor::model
