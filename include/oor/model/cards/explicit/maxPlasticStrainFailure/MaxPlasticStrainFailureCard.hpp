#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One FAILMPS card (SOL 700): the maximum plastic strain failure model. An element fails when its equivalent plastic
// strain exceeds MPS. A MATDEUL names the model in its FID field.
//
// Card layout: FAILMPS  FID  MPS            fields 1 and 2
// MPS is required. The description of the field gives no sign, so the card only asks for a finite number.
struct MaxPlasticStrainFailureCard
{
    base::EntityId id = 0;         // unique among the failure models
    double maxPlasticStrain = 0.0; // the plastic strain that causes the failure
};

} // namespace oor::model
