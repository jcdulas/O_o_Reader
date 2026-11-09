#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/dynamics/FrfComponentReference.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// One FRFRELS card: degrees of freedom of the connection grid points of FRF components that an FRF based assembly (FBA)
// does not connect.
//
// Card layout: FRFRELS  SID  C  COMPID1/COMPNAM1  GRIDPNT1  COMPID2/COMPNAM2  GRIDPNT2  COMPID3/COMPNAM3  GRIDPNT3
// C is a combination of the digits 1 to 6 with no digit twice (the released components of every point of the card),
// kept in ascending order; it is required, and a scalar point (0) has nothing to release. The card names up to three
// points, each by a component (the ID of its FRFCOMP or its name) and a grid point of that component's own model, a
// plain number. The first group is required; a group that is left out is blank in both members, and the card allows a
// gap (a third group with no second one), as the DELAY and DAREA cards allow theirs. The format calls SID the
// identification number of the entry: nothing selects it and a deck may well give the same number to several cards, so
// it is a set ID (IdSpace::Group) rather than a number that is unique.
struct FrfReleasedDofsCard
{
    base::EntityId id = 0;
    std::int32_t components = 0;
    std::array<FrfComponentPoint, 3> points;

    [[nodiscard]] bool operator==(const FrfReleasedDofsCard&) const noexcept = default;
};

} // namespace oor::model
