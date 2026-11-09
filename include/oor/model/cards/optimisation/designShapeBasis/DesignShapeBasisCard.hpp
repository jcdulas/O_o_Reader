#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/ShapeTerms.hpp"

#include <vector>

namespace oor::model
{

// One DVSHAP card: a shape basis vector, as a combination of the columns of a displacement matrix that a previous
// static run stored (attached with the DBLOCATE statement of the File Management Section: the card only names the
// columns).
//
// Card layout: DVSHAP  DVID  COL1  SF1  COL2  SF2  COL3  SF3                fields 1 to 7
// The variable DVID is a DESVAR; several cards, or several columns, add their vectors. A card has no ID.
struct DesignShapeBasisCard
{
    base::EntityId designVarId = 0;
    std::vector<ShapeTerm> terms; // one to three
};

} // namespace oor::model
