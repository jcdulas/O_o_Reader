#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One MDBOLT card: a rigid bolt between two modules made of multi-point constraints (SOLs 101, 103, 400): each pair
// (GTi, GBi) of grid points on the two sides of the cut is tied to the control grid point GRIDC in all six degrees of
// freedom, so that the load or enforced motion of GRIDC pretensions the bolt. The rules of the BOLT apply
// to it.
//
// Card layout: MDBOLT  ID  GRIDC  MODC  MODT  MODB                                  fields 1 to 5
//             (line)  TOP     GT1  GT2  GT3 ... GT7 / GT8  GT9 ...                  a keyword and its grid points
//             (line)  BOTTOM  GB1  GB2  GB3 ... GB7 / GB8  GB9 ...
// ID is the element ID of the bolt (required). GRIDC is the control grid point (required). MODC, MODT and MODB are the
// modules that hold GRIDC, the grid points GTi and the grid points GBi (0, the main Bulk Data, when blank); a MODC
// above zero must be MODT or MODB. The lists go on over continuation lines (the format reference draws the grid points
// in the fields after the keyword and numbers them GT1 to GT6 and then GT8: the card reads every field of a line and
// writes seven after the keyword). As in the BOLT the lists have the same number of grid points, one pair after the
// other, and GRIDC is not in the BOTTOM list; with the modules, that is checked only when GRIDC and the
// bottom grid points are in the same module. The card belongs in the main Bulk Data.
//
// The modules are plain integers. The grid points are looked up in this model (`Node`) only for the module that is 0,
// the main Bulk Data; the others are plain IDs.
struct ModuleBoltCard
{
    base::EntityId id = 0;                   // the element ID of the bolt
    base::EntityId controlNode = 0;          // the control grid point
    base::EntityId controlModuleId = 0;      // the module of GRIDC
    base::EntityId topModuleId = 0;          // the module of the TOP grid points
    base::EntityId bottomModuleId = 0;       // the module of the BOTTOM grid points
    std::vector<base::EntityId> topNodes;    // GT1, GT2, ...
    std::vector<base::EntityId> bottomNodes; // GB1, GB2, ...

    [[nodiscard]] bool operator==(const ModuleBoltCard&) const noexcept = default;
};

} // namespace oor::model
