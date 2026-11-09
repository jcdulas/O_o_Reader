#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>

namespace oor::model
{

// One GMSPC card: a single point constraint on the finite element grid of a geometric entity (a curve, a surface, an
// FEFACE).
//
// Card layout: GMSPC  SID  C  ENTITY  ID
// The card is not in the format reference list (it is a geometry entry of the p-version pre-processing); the layout is
// the one above (`GMSPC 1 3 FEFACE 1001`, `GMSPC, 1, 3, GMCURV, 102`). SID is selected by the SPC Case Control command.
// C holds the digits 1 to 6 of the constrained components as one number. ENTITY is the name of the kind of geometric
// entity (ElementFace, GeomCurve, GeomSurface, ...), kept as written; ID is the ID of that entity.
struct GeomSinglePointConstrCard
{
    base::EntityId id = 0;
    std::int32_t components = 0;
    std::string entityType;
    base::EntityId entityId = 0;
};

} // namespace oor::model
