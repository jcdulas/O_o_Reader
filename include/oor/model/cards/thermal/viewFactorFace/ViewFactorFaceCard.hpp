#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One VIEW card: the radiation cavity and the shadowing of a surface element face, for the view factor calculation.
//
// Card layout: VIEW  IVIEW  ICAVITY  SHADE  NB  NG  DISLIN
// SHADE is NONE, KSHD (the face can shade others), KBSHD (it can be shaded) or BOTH (blank). NB and NG, the subelement
// mesh sizes, are 1 when blank, DISLIN is 0.0. ICAVITY is 0 for a face outside every cavity.
struct ViewFactorFaceCard
{
    base::EntityId id = 0;
    base::EntityId cavityId = 0;
    std::string shading = "Both";
    int betaSubdivisions = 1;
    int gammaSubdivisions = 1;
    double surfaceOffset = 0.0;
};

} // namespace oor::model
