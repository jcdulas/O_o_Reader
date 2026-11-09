#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One entry of the list of properties of a PCOMPF: a single PID (first == last) or "first THRU last [BY by]". `by` is
// stored resolved (1 for a range that goes up, -1 for one that goes down, when BY is left out) and written only when it
// is not that default.
struct CompPropIntegrationRun
{
    base::EntityId first = 0;
    base::EntityId last = 0;
    base::EntityId increment = 1;

    [[nodiscard]] bool isRange() const noexcept { return first != last; }
    [[nodiscard]] bool operator==(const CompPropIntegrationRun&) const noexcept = default;
};

// One PCOMPF card (SOL 400): the through-the-thickness integration procedure of the composite shells whose PCOMP or
// PCOMPG is in the list.
//
// Card layout (the list runs over fields and continuation lines, with blank fields allowed for readability):
//   PCOMPF  INT  PID1  THRU  PID2  BY  N
//   PCOMPF  INT  PID1  PID2  PID3  THRU  PID4  PID5  THRU  PID6  PID7  TO  PID8 ...   any mix of IDs and ranges
//   PCOMPF  INT  ALL
//
// INT is 1 (every material behaviour through the thickness, the default), 2 (linear elastic material, fast integration,
// no thermal strains and no temperature dependent properties) or 3 (linear elastic, fast integration). The card has no
// ID of its own (IdSpace::None): a deck may have many, and the format reference makes it an error when two of them give
// one PID different INT values (a rule between cards).
//
// A PID is 1 to 9 999 999. The ends of a THRU range name properties, but the list inside the range need not be
// continuous: the ends are references that may name nothing, and the properties between them are the
// ones that exist. TO is read as THRU (the format reference writes it in two of its forms) and is written THRU. A range
// may go down; BY then is negative. ALL, which stands for every PCOMP and PCOMPG, is the whole list.
struct CompPropIntegrationCard
{
    int integration = 1; // INT
    bool all = false;    // the ALL form: no runs
    std::vector<CompPropIntegrationRun> runs;

    [[nodiscard]] bool operator==(const CompPropIntegrationCard&) const noexcept = default;
};

} // namespace oor::model
