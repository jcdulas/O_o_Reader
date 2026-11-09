#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One RADSET card: the radiation cavities that take part in the enclosure radiation analysis.
//
// Card layout: RADSET  ICAVITY1  ICAVITY2  ICAVITY3  ICAVITY4  ICAVITY5  ICAVITY6  ICAVITY7  ICAVITY8
//                     ICAVITY9  -etc.-
// The cavities continue over as many lines as needed.
struct RadiationCavitySetCard
{
    std::vector<base::EntityId> cavities;
};

} // namespace oor::model
