#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One DEACTEL card (SOL 400, NLSTATIC, NLTRAN and perturbation analyses): the elements that do not take part in the
// analysis, or in one physics pass of a coupled analysis.
//
// Card layout: DEACTEL  ID  (blank)  (blank)  (blank)  ISET                       fields 1 and 5 (2 to 4 are blank)
// ID is the number that the Case Control command DEACTEL=ID selects (it acts before the first subcase, in the
// first subcase, in its first step or in each substep of that step, and is ignored elsewhere). ISET is the SID of a
// SET3 that lists the elements; the DES of that SET3 must be ELEM (which names a rule between two cards that
// the reader does not check), and a listed element that does not exist is ignored. Both numbers are required
// and have no default.
struct DeactivatedElemsCard
{
    base::EntityId id = 0;
    base::EntityId elemSetId = 0;

    [[nodiscard]] bool operator==(const DeactivatedElemsCard&) const noexcept = default;
};

} // namespace oor::model
