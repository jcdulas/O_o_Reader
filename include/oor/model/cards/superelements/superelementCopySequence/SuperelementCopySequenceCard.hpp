#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One SEQSEP card: the order of the exterior points of a primary superelement that an identical or mirror image copy of
// it follows.
//
// Card layout: SEQSEP  SSID  PSID  GP1  GP2  GP3  GP4  GP5  GP6
//                     GP7  GP8  -etc.-
// It goes with the CSUPER card of the secondary superelement SSID, which lists the exterior points of the copy in the
// order of the exterior points of the primary superelement PSID; the SEQSEP says another correspondence when the points
// of the CSUPER are not in that order. The GPi are the exterior points of the primary superelement, in the
// order the copy's points follow. SSID is the number as the CSUPER writes it (it may carry the code of the mirror), so
// it is not a reference to a superelement; PSID is one, and the points are those of that superelement. A card that
// lists no point is refused.
struct SuperelementCopySequenceCard
{
    base::EntityId superelementId = 0;
    base::EntityId primarySuperelementId = 0;
    std::vector<base::EntityId> points;

    [[nodiscard]] bool operator==(const SuperelementCopySequenceCard&) const noexcept = default;
};

} // namespace oor::model
