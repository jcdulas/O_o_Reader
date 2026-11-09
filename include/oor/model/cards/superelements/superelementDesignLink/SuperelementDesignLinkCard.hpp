#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// A term of an SEDLINK: an independent design variable, the superelement it belongs to and its coefficient.
struct SuperelementDesignLinkTerm
{
    base::EntityId superelementId = 0; // the part superelement of the design variable
    base::EntityId designVarId = 0;    // the design variable (a DESVAR of that part)
    double coefficient = 0.0;

    [[nodiscard]] bool operator==(const SuperelementDesignLinkTerm&) const noexcept = default;
};

// One SEDLINK card: a design variable of one part superelement that depends linearly on design variables of other part
// superelements: DDVID = C0 + CMULT * (C1 * IDV1 + C2 * IDV2 + ...), the DLINK across the boundaries of the parts.
//
// Card layout: SEDLINK  ID  DSEID  DDVID  C0  CMULT
//                      ISEID1  IDV1  C1  ISEID2  IDV2  C2
//                      ISEID3  IDV3  C3  ISEID4  IDV4  C4  -etc.-
// ID is the number of the card, unique among the SEDLINK cards (IdSpace::PerCardType); DSEID and DDVID are the part and
// the DESVAR it holds that depends on the others; C0 defaults to 0.0 and CMULT to 1.0. The terms are three fields each
// and two to a continuation line (fields 9 to 14, 17 to 22, ...). Every design variable is a DESVAR of its own part,
// which has its own ID space. The card belongs in the main Bulk Data and is ignored after a BEGIN SUPER=n with n above
// 0.
struct SuperelementDesignLinkCard
{
    base::EntityId id = 0;
    base::EntityId dependentSuperelementId = 0;
    base::EntityId dependentVarId = 0;
    double constant0 = 0.0;
    double multiplier = 1.0;
    std::vector<SuperelementDesignLinkTerm> terms;

    [[nodiscard]] bool operator==(const SuperelementDesignLinkCard&) const noexcept = default;
};

} // namespace oor::model
