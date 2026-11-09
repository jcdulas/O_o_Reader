#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

// The shape of a wetted element given by its grid points.
enum class WettedElementType : std::uint8_t
{
    Tria3,
    Tria6,
    Quad4,
    Quad8,
    Line2,
    Line3,
};

// The words of TYPE, in the order of the enumeration.
inline constexpr std::array<std::string_view, 6> WETTED_ELEMENT_WORDS{"Tria3Shape", "Tria6Shape", "Quad4Shape",
                                                                      "Quad8Shape", "Line2Nodes", "Line3Nodes"};

// The number of grid points each shape has, in the order of the enumeration.
inline constexpr std::array<std::size_t, 6> WETTED_ELEMENT_GRID_COUNTS{3, 6, 4, 8, 2, 3};

[[nodiscard]] constexpr std::size_t gridCountOf(WettedElementType type) noexcept
{
    return WETTED_ELEMENT_GRID_COUNTS[static_cast<std::size_t>(type)];
}

// One WETELMG card (SOL 400): a wetted element of an external fluid-structure simulation, given by its shape and its
// grid points.
//
// Card layout: WETELMG  WEID  TYPE                                        fields 1 and 2, fields 3 to 8 blank
//             +        G1  G2  G3  G4  G5  G6  G7  G8                     fields 9 to 16
// TYPE is required (no default) and is one of TRIA3, TRIA6, QUAD4, QUAD8, LINE2 and LINE3; the grid points follow on
// the continuation line. No grid point has a default, so the card
// has exactly as many as its shape (3, 6, 4, 8, 2 and 3 in the order above): the format does not state the count, the
// shape does, and a QUAD8 with its mid-side points left blank is refused. The grid points fill the fields from the
// ninth with no gap.
//
// The WEID is a number that a WETSURF names; it shares its space with the WETELME cards (see WettedElemSideCard), each
// type unique on its own (IdSpace::PerCardType, category WettedElement).
struct WettedElemNodesCard
{
    base::EntityId id = 0;
    WettedElementType type = WettedElementType::Tria3;
    std::vector<base::EntityId> grids; // G1 to Gn, n from the type
};

} // namespace oor::model
