#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// DES: what the IDs of a SET3 are. RBEin and RBEex select the rigid elements an MPC command includes or excludes:
// they are elements here, as the rigid elements share the element ID space.
enum class TypedListSet3Kind : std::uint8_t
{
    Grid,          // GRID
    Element,       // ELEM
    Point,         // POINT
    Property,      // PROP
    RigidIncluded, // RBEin
    RigidExcluded, // RBEex
    Module         // MODULE
};

// One SET3 card: a list of grid points, elements, points, properties or modules that other cards refer to.
//
// Card layout: SET3  SID  DES  ID1  ID2  ID3  ID4  ID5  ID6
//                   ID7  ID8  -etc.-
// or:         SET3  SID  DES  ID1  "THRU"  ID2
// The list is kept as written: single IDs and THRU ranges, in order. The IDs are not negative (the format reference
// allows zero). Several cards may have the same SID: they are one set.
struct TypedListSet3Card
{
    base::EntityId id = 0;
    TypedListSet3Kind kind = TypedListSet3Kind::Grid;
    std::vector<IdRun> ids;
};

} // namespace oor::model
