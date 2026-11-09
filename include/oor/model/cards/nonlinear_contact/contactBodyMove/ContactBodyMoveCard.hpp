#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One BCMOVE card: the movement of bodies in contact in a load case (SOLs 101 and 400): the rigid bodies are moved
// until they touch the deformable ones, or the contact of some bodies is released.
//
// Card layout: BCMOVE  ID  MTYPE  IDRBOD1  IDRBOD2  IDRBOD3  ...   over as many continuation lines as needed.
struct ContactBodyMoveCard
{
    base::EntityId id = 0;              // the Case Control BCONTACT or BCMOVE ID; 0 is the load case 0
    std::string moveKind = "Approach";  // APPROACH, RELEASE or SYNCHRON
    std::vector<base::EntityId> bodies; // the bodies to release (RELEASE only; blank for the other two types)
};

} // namespace oor::model
