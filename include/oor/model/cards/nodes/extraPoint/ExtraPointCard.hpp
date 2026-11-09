#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One extra point defined by an EPOINT card.
//
// Card layout (up to eight IDs per line, continued on further lines; a range is written with THRU):
//   EPOINT  ID1  ID2  ID3  ID4  ID5  ID6  ID7  ID8
//   EPOINT  ID1  THRU ID2
//
// An EPOINT card defines many points, so the card is read as one row per point (ExtraPointFormat::readAll): the library
// keeps no list of IDs, every extra point is a member of the node ID space with its own row, found by
// `ReferenceTarget::Node` like a GRID. A written model has one EPOINT card per point; a THRU range is not rebuilt.
struct ExtraPointCard
{
    base::EntityId id = 0;
};

} // namespace oor::model
