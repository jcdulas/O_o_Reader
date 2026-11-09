#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One (ROTORIDi, HYBDAMPi) pair of a ROTHYBD: the hybrid damping a rotor takes.
struct RotorHybridPair
{
    base::EntityId rotorId = 0;
    base::EntityId hybridDampingId = 0;

    [[nodiscard]] bool operator==(const RotorHybridPair&) const noexcept = default;
};

// One ROTHYBD card (hybrid damping for rotors): any number of pairs of a rotor and the HYBDAMP that gives its hybrid
// modal damping.
//
// Card layout: ROTHYBD  ROTORID1  HYBDAMP1  ROTORID2  HYBDAMP2  ROTORID3  HYBDAMP3  ROTORID4  HYBDAMP4
//                      ROTORID5  HYBDAMP5  -etc.-
// The pairs are written one after the other, four to a line; both numbers of a pair are required. The ROTORID names a
// rotor of the model (a ROTORG, ROTOR, ROTORAX or ROTORSE: ReferenceTarget::Rotor); the solver
// ignores a rotor that no deck card defines, and the reference is reported as unresolved all the same. The
// HYBDAMP must exist. The card has no ID of its own (IdSpace::None) and a deck may have several.
struct RotorHybridDampingCard
{
    std::vector<RotorHybridPair> pairs;

    [[nodiscard]] bool operator==(const RotorHybridDampingCard&) const noexcept = default;
};

} // namespace oor::model
