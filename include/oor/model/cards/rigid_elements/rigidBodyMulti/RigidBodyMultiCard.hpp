#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One RBE1 card: a rigid body connected to any number of grid points, with its independent components given point by
// point.
//
// Card layout: RBE1  EID  GN1  CN1  GN2  CN2  GN3  CN3
//                   .    GN4  CN4  GN5  CN5  GN6  CN6
//                   "UM"  GM1  CM1  GM2  CM2  GM3  CM3
//                   .    GM4  CM4  -etc.-  ALPHA  TREF
// The independent pairs (GNi, CNi) are at most six: three on the first line, from the second field, and three on the
// next, from the third field of that line. "UM" starts the dependent pairs (GMj, CMj) and is the first field of a line:
// field 9 after one line of independent pairs, field 17 after two. The dependent pairs follow it, three to a line from
// the third field. ALPHA and TREF are the two fields after the last dependent pair, each told from a point by being a
// real (0.0 when absent). Each CNi and CMj holds the digits 1 to 6 of the components as one number. The rigid elements
// share the ID space of the elements.
struct RigidBodyMultiCard
{
    static constexpr std::size_t MAX_INDEPENDENT = 6;

    base::EntityId id = 0;
    std::vector<DofPair> independent; // GNi, CNi
    std::vector<DofPair> dependent;   // GMj, CMj
    double alphaCoefficient = 0.0;
    double referenceTemperature = 0.0;
};

} // namespace oor::model
