#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The contact algorithm of a WALL (the METHOD field): the penalty method, which allows friction and output, or the
// kinematic method kept for the sake of old decks, which allows neither. The enumerators are numbered in the
// order of RIGID_WALL_METHOD_WORDS. The format reference spells the second word KINMATIC; the card reads and writes it
// that way.
enum class RigidWallMethod : std::uint8_t
{
    Penalty,
    Kinematic
};
inline constexpr std::array<std::string_view, 2> RIGID_WALL_METHOD_WORDS{"PenaltyMethod", "Kinematic"};

// One WALL card (SOL 700): a rigid plane of infinite size, fixed in space, that the grid points of a BCGRID cannot
// penetrate. The points slide on it and separate from it. No other entry of the format reference has this name.
//
// Card layout: WALL  ID  XP  YP  ZP  NX  NY  NZ  BCGRID            fields 1 to 8
//             +     METHOD  FS  FK  EXP                           fields 9 to 12
// The plane goes through the point (XP, YP, ZP) and has the normal (NX, NY, NZ), which points towards the model; every
// one of the six numbers is zero when blank. BCGRID is the ID of the BCGRID card that lists the grid points. METHOD is
// PENALTY when blank. FS and FK are the static and the kinetic coefficient of friction and EXP the exponential decay
// coefficient of the friction law (none of them is negative, all are zero when blank). The first paragraph of the
// format speaks of FINITE, ORTHO, MOVING and FORCES options of the wall: the layout has no field for them, so the card
// has none either. The format reference does not say that the normal must not be zero, and the card does not check it.
struct RigidWallCard
{
    base::EntityId id = 0; // unique among the walls
    double pointX = 0.0;
    double pointY = 0.0;
    double pointZ = 0.0;
    double normalX = 0.0;
    double normalY = 0.0;
    double normalZ = 0.0;
    base::EntityId nodeSetId = 0; // the BCGRID of the points that cannot penetrate the wall
    RigidWallMethod method = RigidWallMethod::Penalty;
    double staticFriction = 0.0;  // static coefficient of friction, not negative
    double kineticFriction = 0.0; // kinetic coefficient of friction, not negative
    double frictionDecay = 0.0;   // exponential decay coefficient, not negative
};

} // namespace oor::model
