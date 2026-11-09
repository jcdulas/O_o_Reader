#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/dynamics/FrfComponentReference.hpp"

#include <array>

namespace oor::model
{

// One FRFCONN card: an explicit connection between a point of one FRF component and a point of another, in an FRF based
// assembly (FBA).
//
// Card layout: FRFCONN  CONNID  COMPID1/COMPNAM1  POINT1  COMPID2/COMPNAM2  POINT2
//
// CONNID is the unique number of the card. Each component is given by the ID of its FRFCOMP or by its name (a whole
// number is an ID, anything else a name). A POINTi is a grid or scalar point of that component's own model, not of the
// model being read: it is a plain number, never looked up. All five fields are required, and the format does not ask
// the two components to differ.
struct FrfConnectionCard
{
    base::EntityId id = 0;
    std::array<FrfComponentPoint, 2> points;

    [[nodiscard]] bool operator==(const FrfConnectionCard&) const noexcept = default;
};

} // namespace oor::model
