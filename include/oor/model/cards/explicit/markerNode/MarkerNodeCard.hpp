#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One CMARKN1 card (SOL 700): a marker element with one grid point, a point at which the Euler domain records its
// variables (time histories). Not a structural element: it has no stiffness and no mass.
//
// Card layout: CMARKN1  ID  PID  G1            fields 1 to 3
// PID names a PMARKER. The grid point is a structural one (the marker follows the structure) or a free point in space
// that carries no mass and moves as the PMARKER says. The format reference says the ID must be unique in the model and
// must not be the ID of a structural element, so the card lives in the element ID space (IdSpace::Element). The rules
// of the format are those of CMARKB2 (they even call the element a CMARKB2): the entry shares them, it is not another
// form of it.
struct MarkerNodeCard
{
    base::EntityId id = 0;     // the element number
    base::EntityId propId = 0; // a PMARKER
    base::EntityId node1 = 0;  // the grid point

    [[nodiscard]] std::array<base::EntityId, 1> nodes() const noexcept { return {node1}; }
};

} // namespace oor::model
