#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace oor::model
{

// The orientation planes of a CBUSH2D (the PLANE field): the plane of CID in which the directions 1 and 2 of the
// element lie.
inline constexpr std::array<std::string_view, 3> BUSH_ELEM_2D_PLANE_WORDS{"PlaneXy", "PlaneYz", "PlaneZx"};

// The names of the two grids in a diagnostic: the inner grid and the outer grid.
inline constexpr std::array<const char*, 2> BUSH_ELEM_2D_NODE_NAMES{"nodeA", "nodeB"};
inline constexpr std::string_view BUSH_ELEM_2D_DEFAULT_PLANE = "PlaneXy";

// One CBUSH2D card: a two-dimensional linear and nonlinear connection (a bush of a rotor model) between an inner grid
// GA and an outer grid GB.
//
// Card layout: CBUSH2D  EID  PID  GA  GB  CID  PLANE
// EID is a positive integer below 100,000,000, unique among all the elements. PID names a PBUSH2D and is required (the
// format reference says so). GA and GB are required: the format reference wants two grids that are coincident and
// have parallel displacement systems, so the same grid point twice is refused. CID (0 when blank, the basic system) is
// the system whose PLANE (XY, YZ or ZX, XY when blank) holds the directions 1 and 2; it is kept in upper case.
//
// A seventh field SPTID, which the format reference does not have, is not a field of this card: a value there is
// refused.
struct Bushing2DCard
{
    static constexpr std::size_t NODE_COUNT = 2;

    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::array<base::EntityId, NODE_COUNT> nodes = {}; // GA (inner) and GB (outer)
    base::EntityId frameId = 0;                        // 0: the basic system
    std::string plane{BUSH_ELEM_2D_DEFAULT_PLANE};     // PlaneXy, PlaneYz or PlaneZx
};

} // namespace oor::model
