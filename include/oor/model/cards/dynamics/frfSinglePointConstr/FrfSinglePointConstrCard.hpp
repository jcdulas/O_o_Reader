#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/dynamics/FrfComponentReference.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One FRFSPC1 card: single-point constraints on connection points of FRF components in an FRF based assembly (FBA).
//
// Card layout: FRFSPC1  SID  C  COMPID1/COMPNAM1  POINT1  COMPID2/COMPNAM2  POINT2  COMPID3/COMPNAM3  POINT3
// SID is the constraint set that the Case Control SPC command selects, shared by as many cards as the deck
// writes, as for SPC1. C is a combination of the digits 1 to 6 with no digit twice (kept in ascending order) for grid
// points and 0 or blank for scalar points; it applies to every point of the card. The card names up to three points,
// each by a component (the ID of its FRFCOMP or its name) and a point of that component's own model, a plain number.
// The first group is required; a group that is left out is blank in both members and the card allows a gap (a third
// group with no second one), as the DELAY and DAREA cards allow theirs.
struct FrfSinglePointConstrCard
{
    base::EntityId id = 0;
    std::int32_t components = 0; // 0 for scalar points
    std::array<FrfComponentPoint, 3> points;

    [[nodiscard]] bool operator==(const FrfSinglePointConstrCard&) const noexcept = default;
};

} // namespace oor::model
