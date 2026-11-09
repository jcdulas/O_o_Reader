#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One term of a LOAD card: a load set and the scale factor applied to it.
struct LoadCombinationTerm
{
    double scale = 0.0;                   // Si
    base::EntityId loadCombinationId = 0; // Li: the set ID of cards of the Load category

    [[nodiscard]] bool operator==(const LoadCombinationTerm&) const noexcept = default;
};

// One LOAD card: a static load made of other load sets, combined linearly.
//
// Card layout: LOAD  SID  S  S1  L1  S2  L2  S3  L3
//                   S4  L4  -etc.-
// The load is S times the sum of Si times the load set Li. The pairs follow each other from field 3 on, over as many
// continuation lines as needed.
struct LoadCombinationCard
{
    base::EntityId id = 0;
    double scale = 0.0; // S, the overall scale factor
    std::vector<LoadCombinationTerm> terms;
};

} // namespace oor::model
