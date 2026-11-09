#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <vector>

namespace oor::model
{

// One RSPLINE card: multipoint constraints that interpolate the displacements of grid points along an elastic beam.
//
// Card layout: RSPLINE  EID  D/L  G1  G2  C2  G3  C3  G4
//                      C4  G5  C5  G6  -etc.-
// The points and the components follow each other from field 3 over the continuation lines: G1, then a point and its
// components for each of the others. A blank Ci means that all six components of Gi are independent; a Ci that is given
// names the components that depend on the independent points. G1 and the last point are independent, so they have no
// components field (the last field of the card is a point): their components are 0. D/L is the ratio of
// the diameter of the elastic tube to the sum of the lengths of the segments (greater than 0, 0.1 when blank). The
// rigid elements share the ID space of the elements.
struct RigidSplineCard
{
    base::EntityId id = 0;
    double ratio = 0.1; // D/L
    std::vector<DofPair> points;
};

} // namespace oor::model
