#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One NSMADD card: a set of non-structural mass that is the sum of other sets.
//
// Card layout: NSMADD  SID  S1  S2  S3  S4  S5  S6  S7
//                     S8   S9  -etc.-
// Each Si is the set ID of NSM, NSM1, NSML or NSML1 cards; none may be the ID of another NSMADD. An
// NSMADD takes precedence over the cards of the same set ID.
struct NonStructMassUnionCard
{
    base::EntityId id = 0;
    std::vector<base::EntityId> sets;
};

} // namespace oor::model
