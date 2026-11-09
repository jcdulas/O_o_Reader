#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>

namespace oor::model
{

// One PBSH2DT card: the frequency dependent stiffness, damping and mass of the PBUSH2D with the same ID, each term
// given by the TABLED1 that holds its variation against frequency (SOL 400 and the rotor dynamics).
//
// Card layout: three groups of lines, each led by a flag in the second field of its line, the first group on the line
// of the PID (the format draws them K, B, M in this order and does not say that the order is binding: like the PBUSHT,
// they are read in any order, each at most once, and written in the order K, B, M):
//   PBSH2DT  PID  "K"  K11  K22  K12  K21     K11 and K22 in the two directions, K12 and K21 cross coupling
//                 "B"  B11  B22  B12  B21     damping
//                 "M"  M11  M22  M12  M21     mass
// The terms of a group are fields 3 to 6 of its line. A term that is blank (stored as 0) is not frequency dependent:
// the nominal value of the PBUSH2D is used. A group with only blank terms is the same as a group that is
// left out. The ID is the one of the PBUSH2D that the card extends (that PBUSH2D must exist), so the card has
// an ID space of its own, like the PBUSHT, and visits its ID as a property. The tables are TABLED1 entries in the space
// of the dynamic tables. That the card only works in a model with a rotor definition is a rule of the solution.
struct BushProp2DFreqCard
{
    static constexpr std::size_t TERM_COUNT = 4;

    base::EntityId id = 0;
    std::array<base::EntityId, TERM_COUNT> stiffnessTableIds = {}; // K11, K22, K12, K21
    std::array<base::EntityId, TERM_COUNT> dampingTableIds = {};   // B11, B22, B12, B21
    std::array<base::EntityId, TERM_COUNT> massTableIds = {};      // M11, M22, M12, M21
};

} // namespace oor::model
