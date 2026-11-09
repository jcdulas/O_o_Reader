#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// One BOLT1 card: a pre-tension bolt model of SOL 400 with advanced nonlinear elements, loaded at a layer of elements
// along a cross-section of the bolt, with no need to duplicate grid points: forces or displacements applied at the
// control grid point GRIDC (its first degree of freedom, along the bolt axis) pretension the bolt.
//
// Card layout: BOLT1  ID  GRIDC  FORM  N1  N2  N3  OFFSET  IDTYPE
//             (continuation)  ELEM  E1  E2  E3 ...
//             (continuation)  GRID  G1  G2  G3 ...
// FORM 0 (the default) gives the elements of the bolt and finds the axis and the cross-section itself (OFFSET moves the
// cross-section along the axis, as a fraction of the bolt length); FORM 1 gives the axis (N1 to N3), the grid points of
// the cross-section (GRID) and the elements that share them and lie on the opposite side of the axis (ELEM). With
// IDTYPE SET the lists are the ID of one SET3 each, of type ELEM and GRID.
struct BoltPretensionCard
{
    base::EntityId id = 0;                // the ID of the bolt
    base::EntityId controlNode = 0;       // the control grid point
    std::int32_t form = 0;                // 0 or 1
    std::array<double, 3> axis{};         // N1 to N3: the direction of the axis (FORM 1)
    double offset = 0.0;                  // the position of the cross-section for FORM 0
    std::string idType = "List";          // LIST or SET
    std::vector<base::EntityId> elements; // ELEM: element IDs, or one SET3 ID
    std::vector<base::EntityId> nodes;    // GRID: grid point IDs, or one SET3 ID
};

} // namespace oor::model
