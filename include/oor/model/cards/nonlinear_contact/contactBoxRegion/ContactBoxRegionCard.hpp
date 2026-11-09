#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One BCBOX card: a 3D contact region of SOL 700 made of the elements inside a box (hexahedron-like region); the BSID
// of a BCBODY names it. The corners are given by eight grid points (form 1) or by eight points of the basic system
// (form 2, COORD in field 2).
//
// Card layout, form 1:  BCBOX  ID  HOW  N1  N2  N3  N4  N5  N6  N7  N8
//             form 2:  BCBOX  ID  COORD  HOW  X1  Y1  Z1  X2  Y2  Z2 ... X8  Y8  Z8
// The corners are numbered like the grid points of a CHEXA.
struct ContactBoxRegionCard
{
    base::EntityId id = 0;          // unique among BSURF, BCBOX, BCPROP and BCMATL
    std::int32_t inclusionRule = 0; // 0: an element with one grid point in the box is in it; 1: all its points must be
    bool byCoordinates = false;     // form 2: the corners are points, not grid points
    std::array<base::EntityId, 8> cornerNodes{}; // form 1: N1 to N8
    std::array<double, 24> cornerCoordinates{};  // form 2: X1, Y1, Z1, X2, ... Z8
};

} // namespace oor::model
