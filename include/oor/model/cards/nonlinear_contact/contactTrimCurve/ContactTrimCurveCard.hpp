#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One BCTRIM card: the geometry of a trimming curve of a NURBS surface of a rigid contact body. A BCNURBS names it in
// its TRIM list.
//
// Card layout: BCTRIM  IDtrim  NPTUtrim  NORUtrim  NSUBtrim
//             COORD Xiso1 Yiso1 Xiso2 Yiso2 ...   HOMO Homo1 Homo2 ...   KNOT Knot1 Knot2 ...
// each list on lines of its own: NPTUtrim points of the isoparametric space (two coordinates each), one homogeneous
// coordinate for each, and NPTUtrim + NORUtrim knots.
struct ContactTrimCurveCard
{
    base::EntityId id = 0;           // IDtrim: unique among the trimming curves
    std::int32_t pointCount = 0;     // NPTUtrim: the control points
    std::int32_t order = 0;          // NORUtrim: the order
    std::int32_t subdivisions = 0;   // NSUBtrim: the subdivisions
    std::vector<double> coordinates; // COORD: x and y of each point in the isoparametric space
    std::vector<double> homogeneous; // HOMO
    std::vector<double> knots;       // KNOT
};

} // namespace oor::model
