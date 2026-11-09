#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/ShapeTerms.hpp"

#include <vector>

namespace oor::model
{

// One DVBSHAP card: a boundary shape vector, as a combination of the vectors that the load sequences of an auxiliary
// model give.
//
// Card layout: DVBSHAP  DVID  AUXMOD  COL1  SF1  COL2  SF2  COL3  SF3        fields 1 to 8
// DVID is a DESVAR; AUXMOD the number of the auxiliary model (the AUXMODEL of a Case Control command and of a
// BEGIN AUXMODEL part); COLi a load sequence of that model. Several cards add their vectors. No ID.
struct DesignBoundaryShapeCard
{
    base::EntityId designVarId = 0;
    base::EntityId auxiliaryModified = 0;
    std::vector<ShapeTerm> terms; // one to three
};

} // namespace oor::model
