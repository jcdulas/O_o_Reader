#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactMotion.hpp"

#include <cstdint>
#include <optional>

namespace oor::model
{

// One BCRIGID card: the motion of a rigid contact body (SOLs 101 and 400). A BCBODY1 or a BCSURF names it by its
// BCRGID.
//
// Card layout: BCRIGID  BCRGID  CGID  CONTROL
//             (continuation)  NLOAD  ANGVEL  DCOS1  DCOS2  DCOS3  VELRB1  VELRB2  VELRB3
//             (continuation)  APPROV  A  N1  N2  N3  V1  V2  V3
//             (continuation)  GROW  GF1  GF2  GF3  TAB-GF1  TAB-GF2  TAB-GF3
// All the continuation lines may be left out.
struct ContactRigidMotionCard
{
    base::EntityId id = 0;            // BCRGID: unique among the BCRIGID cards
    base::EntityId referenceNode = 0; // the grid point of the reference point of the body (0: the basic system)
    std::int32_t control = 0;         // -1 position control, 0 velocity control, a grid point number for load control
    ContactMotion motion;             // the NLOAD ... VELRB3 line
    std::optional<ContactApproach> approach; // the APPROV line
    std::optional<ContactGrowth> growth;     // the GROW line
};

} // namespace oor::model
