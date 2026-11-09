#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/DesignTerms.hpp"

#include <vector>

namespace oor::model
{

// One DLINK card: a design variable that is a linear function of other design variables.
//
// Card layout: DLINK  ID  DDVID  C0  CMULT  IDV1  C1  IDV2  C2          fields 1 to 8
//                    IDV3  C3  -etc.-                                  fields 9 to 16, and so on
//   DDVID = C0 + CMULT * (C1*IDV1 + C2*IDV2 + ...)   (the sum of the Ci*IDVi, times CMULT)
// The ID is for reference only. The variables IDVi must be distinct and a variable that is
// dependent here cannot be independent in another DLINK: the second rule is between cards and is the check
// `design.link-cycle`, with the loops.
struct DesignVarLinkCard
{
    base::EntityId id = 0;
    base::EntityId dependent = 0; // DDVID: the DESVAR that depends on the others
    double constant0 = 0.0;
    double multiplier = 1.0;
    std::vector<DesignTerm> terms; // the IDVi and Ci; at least one
};

} // namespace oor::model
