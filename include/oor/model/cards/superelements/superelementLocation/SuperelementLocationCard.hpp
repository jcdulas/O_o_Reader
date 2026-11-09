#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One SELOC card: where a part superelement is placed, by three of its points and the three points they go to.
//
// Card layout: SELOC  SEID  PA1  PA2  PA3  PB1  PB2  PB3                          fields 1 to 7
// SEID is the partitioned superelement to move. PA1 to PA3 are three noncolinear grid points or POINT cards of that
// superelement (so they are in its own ID space), PB1 to PB3 three points of the main Bulk Data the superelement is
// aligned to: the superelement is rotated and translated so that PAi lands on PBi. The PBi must
// have the same relative locations as the PAi. Three points are needed even if the superelement connects to
// one or two. The card belongs in the main Bulk Data and is a fatal error after a BEGIN SUPER.
struct SuperelementLocationCard
{
    base::EntityId superelementId = 0;
    std::array<base::EntityId, 3> pointsA{}; // three points of superelement SEID
    std::array<base::EntityId, 3> pointsB{}; // three points of the main Bulk Data

    [[nodiscard]] bool operator==(const SuperelementLocationCard&) const noexcept = default;
};

} // namespace oor::model
