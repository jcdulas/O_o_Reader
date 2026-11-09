#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One PANEL card: panels of the structure for the panel participation of a coupled fluid-structure analysis.
//
// Card layout: PANEL  NAME1  SETID1  NAME2  SETID2  NAME3  SETID3  NAME4  SETID4
// (more pairs may follow on continuation lines). NAMEi labels the panel in the output; SETIDi is the SET1 or SET3 that
// lists the grid points, elements or properties of the panel. `names` and `sets` have one entry for each panel.
struct FluidStructurePanelsCard
{
    std::vector<std::string> names;
    std::vector<base::EntityId> sets;
};

} // namespace oor::model
