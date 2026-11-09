#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One MDFSET card: the fluid grid points or elements of a module that an ACMODL with FSET = -1 couples to the
// structure.
//
// Card layout: MDFSET  MODID  ID1  ID2  ID3  ID4  ID5  ID6  ID7
//                     ID8    -etc.-
// or:         MDFSET  MODID  ID1  "THRU"  ID2  ...
// MODID is the module (zero or more; the format reference gives no default and a blank field is read as 0, the main
// Bulk Data, as everywhere in the multi-module cards). The IDs are kept as written: single IDs and THRU ranges, in
// order, over as many lines as it takes. Several MDFSET cards may name the same or other modules. The card
// can only be used when the deck has modules and belongs in the main Bulk Data; the reader keeps it wherever
// it stands.
//
// The IDs are fluid grid points or fluid elements of module MODID, and the card does not say which, so they are plain
// IDs: nothing is looked up, whatever the module. MODID is a module number, a plain integer.
struct ModuleFluidSetCard
{
    base::EntityId moduleId = 0; // the module whose fluid points or elements are listed
    std::vector<IdRun> ids;      // ID1, ID2, ...: IDs and THRU ranges

    [[nodiscard]] bool operator==(const ModuleFluidSetCard&) const noexcept = default;
};

} // namespace oor::model
