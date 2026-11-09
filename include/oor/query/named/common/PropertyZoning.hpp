#pragma once

#include "oor/query/named/common/CardList.hpp"

#include <cstdint>
#include <vector>

namespace oor::query
{

// The zone of each element of a list: elements of one property that share a node are in one zone. Returns, for each
// element, the position in the list of the first element of its zone (the root), so that two elements are in one zone
// exactly when they have the same root. `properties` has the property of each element (0: the element is in no zone and
// is its own root).
[[nodiscard]] std::vector<std::uint32_t> propertyZoneRoots(const NodeLists& nodes,
                                                           const std::vector<std::int32_t>& properties);

} // namespace oor::query
