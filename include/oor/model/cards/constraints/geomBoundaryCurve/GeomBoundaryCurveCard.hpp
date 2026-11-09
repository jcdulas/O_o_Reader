#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

enum class GeomBoundaryCurveEntity : std::uint8_t
{
    Grid // GRID: the boundary is a chain of grid points
};

inline constexpr std::array<std::string_view, 1> GEOM_BOUNDARY_CURVE_ENTITY_WORDS{"Node"};

// One GMBNDC card: a geometric boundary made of element edges along a curve, which a CINTC names by its BID to say
// where it puts interface elements. The boundary may be edges of shell, beam or solid elements.
//
// Card layout: GMBNDC  BID  GRIDI  GRIDF
//                     .    ENTITY  ID1  ID2  ID3  ID4  ID5  ID6  ID7
//                     .    ID8  -etc.-
// GRIDI and GRIDF are the two end points of the curve. ENTITY (GRID, required) says what the IDs are, and
// the IDi are the grid points in order from GRIDI to GRIDF; the end points need not be repeated in the list. The list
// starts in field 10 and goes on in the following fields (field 17 holds ID8). The IDi are distinct;
// the format does not require a minimum, so a boundary whose list is empty is kept as it is. Several continuation lines
// may carry the IDs. The BID is unique among the GMBNDC cards and has no space shared with other cards.
struct GeomBoundaryCurveCard
{
    base::EntityId id = 0;
    base::EntityId startNode = 0;
    base::EntityId endNode = 0;
    GeomBoundaryCurveEntity entity = GeomBoundaryCurveEntity::Grid;
    std::vector<base::EntityId> ids;
};

} // namespace oor::model
