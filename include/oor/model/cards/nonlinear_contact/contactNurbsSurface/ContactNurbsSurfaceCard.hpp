#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactNurbs.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One BCNURBS card: a rigid contact body made of a NURBS surface (SOLs 101 and 400). A BCRGSRF or a BCBODY1 names it by
// its RBID.
//
// Card layout: BCNURBS  RBID  NPTU  NPTV  NORU  NORV  NSUBU  NSUBV
//             GRID G1 G2 ... (or COORD X1 Y1 Z1 X2 ...)   HOMO Homo1 ...   KNOT Knot1 ...   TRIM IDtrim1 IDtrim2 ...
// each list on lines of its own. NPTU is negative when the control points are given by coordinates; there are
// |NPTU| x NPTV control points, one homogeneous coordinate each, and (|NPTU| + NORU) + (NPTV + NORV) knots.
struct ContactNurbsSurfaceCard
{
    base::EntityId id = 0;                    // RBID: unique among the contact surfaces
    std::int32_t pointCountU = 0;             // the control points in the U direction (negative: given by coordinates)
    std::int32_t pointCountV = 0;             // the control points in the V direction
    std::int32_t orderU = 0;                  // the order along U
    std::int32_t orderV = 0;                  // the order along V
    std::int32_t subdivisionsU = 0;           // the subdivisions along U
    std::int32_t subdivisionsV = 0;           // the subdivisions along V
    ContactNurbsData data;                    // GRID or COORD, HOMO, KNOT
    std::vector<base::EntityId> trimCurveIds; // the BCTRIM trimming curves, in the TRIM list
};

} // namespace oor::model
