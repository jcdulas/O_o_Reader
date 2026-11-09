#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nodes/ModuleBoundarySupport.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One MDEXCLD card: the grid points of a module that are left out when the module is attached to another one (or to all
// the others): the counterpart of the MDBNDRY, which limits the search to a list.
//
// Card layout: MDEXCLD  MIDA  MIDB  GIDA1  GIDA2  GIDA3  GIDA4  GIDA5  GIDA6
//                      GIDA7 GIDA8 -etc.-
// or:         MDEXCLD  MIDA  MIDB  GIDA1  "THRU"  GIDA2
// MIDA is the module whose points the list names (zero or more, 0 when blank: the main Bulk Data); MIDB is the module
// the points are excluded from connecting to, or the word ALL, which is also what a blank MIDB means (it is kept as
// ALL, no number). The points are kept as written: single points and THRU ranges, in order. The card belongs in the
// main Bulk Data (not in a BEGIN MODULE section) and is meaningful only when the deck has modules;
// the reader keeps it wherever it stands. An MDBULK with METHOD MANUAL makes it a fatal error: a rule
// between cards.
//
// The two modules are plain integers. The points are those of module MIDA: they are grid points of the main Bulk Data,
// looked up in this model, only when MIDA is 0; for another module they are plain IDs.
struct ModuleExcludedNodesCard
{
    base::EntityId moduleIdA = 0;                        // the module whose points are excluded
    ModuleOrAll otherModule = ModuleOrAll::everything(); // the module they are not connected to, or ALL
    std::vector<IdRun> nodes;                            // GIDA1, GIDA2, ...: points and THRU ranges

    [[nodiscard]] bool operator==(const ModuleExcludedNodesCard&) const noexcept = default;
};

} // namespace oor::model
