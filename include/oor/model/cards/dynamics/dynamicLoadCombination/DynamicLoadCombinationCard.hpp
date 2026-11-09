#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One term of a DLOAD card: a dynamic load set and the scaleFactor factor applied to it.
struct DynamicLoadCombinationTerm
{
    double scaleFactor = 0.0;     // Si
    base::EntityId loadSetId = 0; // Li: the set ID of an RLOAD1, RLOAD2, TLOAD1 or TLOAD2 card

    [[nodiscard]] bool operator==(const DynamicLoadCombinationTerm&) const noexcept = default;
};

// One DLOAD card: a dynamic load made of other dynamic load sets, combined linearly.
//
// Card layout: DLOAD  SID  S  S1  L1  S2  L2  S3  L3
//                    S4   L4  -etc.-
// The load is S times the sum of Si times the dynamic load set Li. The pairs follow each other from field 3 on, over as
// many continuation lines as needed. A DLOAD cannot refer to its own set ID, and its Li are all different.
struct DynamicLoadCombinationCard
{
    base::EntityId id = 0;
    double scaleFactor = 0.0; // S, the overall scale factor
    std::vector<DynamicLoadCombinationTerm> terms;
};

} // namespace oor::model
