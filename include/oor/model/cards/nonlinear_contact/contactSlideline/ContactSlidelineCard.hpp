#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One BCONP card: a slideline contact region of SOL 106 and 129 (SOL 400 has BCONECT, BCONPRG and BCONPRP instead): its
// secondary and primary lines, its stiffness scale, friction and penetration type.
//
// Card layout: BCONP  ID  SECNDRY  PRIMARY  (blank)  SFAC  FRICID  PTYPE  CID
struct ContactSlidelineCard
{
    base::EntityId id = 0;              // unique among the BCONP cards
    base::EntityId secondaryLineId = 0; // the BLSEG of the secondary line
    base::EntityId primaryLineId = 0;   // the BLSEG of the primary line
    double penaltyScale = 1.0;          // scale of the penalty values the program calculates
    base::EntityId frictionId = 0;      // the BFRIC of the friction; 0 when blank
    std::int32_t penetrationKind = 1;   // 1 unsymmetric (secondary penetration only), 2 symmetric
    base::EntityId frameId =
        0; // the coordinate system whose Z axis is the slideline plane vector; 0 is the basic system
};

} // namespace oor::model
