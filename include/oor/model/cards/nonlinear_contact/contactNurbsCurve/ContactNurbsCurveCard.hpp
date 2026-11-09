#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactNurbs.hpp"

#include <cstdint>

namespace oor::model
{

// One BCNURB2 card: a 2D rigid contact body made of a NURBS curve (SOLs 101 and 400). A BCRGSRF or a BCBODY1 names it
// by its RBID.
//
// Card layout: BCNURB2  RBID  NPTU  NORU  NSUB
//             GRID G1 G2 ... (or COORD X1 Y1 X2 Y2 ...)   HOMO Homo1 ...   KNOT Knot1 ...
// each list on lines of its own. NPTU is negative when the control points are given by coordinates; there are |NPTU|
// control points (two coordinates each), one homogeneous coordinate each, and |NPTU| + NORU knots.
struct ContactNurbsCurveCard
{
    base::EntityId id = 0;         // RBID: unique among the contact surfaces
    std::int32_t pointCountU = 0;  // the control points (negative: given by coordinates)
    std::int32_t orderU = 0;       // the order
    std::int32_t subdivisions = 0; // the subdivisions
    ContactNurbsData data;         // GRID or COORD, HOMO, KNOT
};

} // namespace oor::model
