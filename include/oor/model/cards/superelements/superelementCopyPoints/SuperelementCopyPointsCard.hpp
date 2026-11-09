#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One CSUPER card: the exterior points of a secondary superelement (an identical or mirror image copy of another one,
// or one that comes from an external source).
//
// Card layout: CSUPER  SSID  PSID  GP1  GP2  GP3  GP4  GP5  GP6
//                     GP7  GP8  -etc.-
// SSID is written XXX0000 + n: n, below 10000, is the number of the secondary superelement, and XXX is the
// code of the displacement components that a mirror reverses: 1, 2 or 3 for the reversal of x, y or z, 12, 23 or 31 for
// two of them (a mirror image), 123 for the three (an identical image), and no code for no reversal. PSID is the
// primary superelement, which is blank for one that comes from an external source. A minus sign on PSID asks for a
// z-reversal mirror when SSID has no code. The points are those of the secondary superelement, in the order of the
// exterior points of the primary one unless an SEQSEP says another. The card defines the superelement n
// (IdSpace::Group, category Superelement): the ID of the card is n, so that a renumbering of the superelements changes
// the number and keeps the code.
struct SuperelementCopyPointsCard
{
    static constexpr base::EntityId NUMBER_RANGE = 10000;

    base::EntityId superelementId = 0;        // the number and the code, as written
    base::EntityId primarySuperelementId = 0; // the primary superelement (negative: a z-reversal mirror), 0 when blank
    std::vector<base::EntityId> points;

    // The number of the secondary superelement: the part of SSID below 10000.
    [[nodiscard]] base::EntityId number() const noexcept { return superelementId % NUMBER_RANGE; }
    // The code of the reversed components: the part of SSID from 10000 on (0 for none).
    [[nodiscard]] base::EntityId reversalCode() const noexcept { return superelementId / NUMBER_RANGE; }

    [[nodiscard]] bool operator==(const SuperelementCopyPointsCard&) const noexcept = default;
};

} // namespace oor::model
