#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

// The C field of a CYJOIN: the degree of freedom of a boundary point that is normal to the boundary, for dihedral and
// axisymmetric problems. R (rectangular) and C (cylindrical) mean T2 and S (spherical) means T3; the card keeps the
// word as written. The enumerators are numbered in the order of CYCLIC_BOUNDARY_SYSTEM_WORDS.
enum class CyclicBoundaryJoinSystem : std::uint8_t
{
    T1,
    T2,
    T3,
    Rectangular,
    Cylindrical,
    Spherical
};

inline constexpr std::array<std::string_view, 6> CYCLIC_BOUNDARY_SYSTEM_WORDS{
    "Translation1", "Translation2", "Translation3", "Rectangular", "Cylindrical", "Spherical"};

// One CYJOIN card: the boundary points of one side of a segment in a cyclic symmetry problem.
//
// Card layout: CYJOIN  SIDE  C  G1  G2  G3  G4  G5  G6
//                     G7  G8  G9  -etc.-
// SIDE is 1 or 2: a rotational or axisymmetric problem has one logical card for each side and the two lists, which join
// the points of the first side to those of the second one by position, have the same length (a rule between the cards,
// not checked here). C may be blank (rotational problems and scalar points that keep the sign). The points are grid
// points or scalar points, kept as written in `grids`: single points and "first THRU last" ranges, in order; a range
// may not begin or end a line of the card (the writer sees to it; the reader does not mind where the words are). The
// card has no ID of its own (IdSpace::None); a deck has one for each side.
struct CyclicBoundaryJoinCard
{
    base::EntityId sideCode = 0; // 1 or 2
    std::optional<CyclicBoundaryJoinSystem> system;
    std::vector<IdRun> grids;

    [[nodiscard]] bool operator==(const CyclicBoundaryJoinCard&) const noexcept = default;
};

} // namespace oor::model
