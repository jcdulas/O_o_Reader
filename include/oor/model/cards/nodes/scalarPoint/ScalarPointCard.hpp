#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One scalar point defined by an SPOINT card.
//
// Card layout (up to eight IDs per line, continued on further lines; a range is written with THRU):
//   SPOINT  ID1  ID2  ID3  ID4  ID5  ID6  ID7  ID8
//   SPOINT  ID1  THRU ID2
//
// An SPOINT card defines many points, so the card is read as one row per point (ScalarPointFormat::readAll): the
// library keeps no list of IDs, every scalar point is a member of the node ID space with its own row, found by
// `ReferenceTarget::Node` like a GRID. A written model has one SPOINT card per point; a THRU range is not rebuilt.
struct ScalarPointCard
{
    base::EntityId id = 0;
};

} // namespace oor::model
