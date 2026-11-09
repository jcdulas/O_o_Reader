#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One DVGRID card: how a design variable moves a grid point.
//
// Card layout: DVGRID  DVID  GID  CID  COEFF  N1  N2  N3                  fields 1 to 7
// The grid point moves by COEFF * (X - X0) * N, where X is the design variable, X0 its initial value and N the vector
// (N1, N2, N3) in the coordinate system CID (blank or 0: the basic system); at least one component is not zero. Several
// DVGRID cards may name the same grid point or the same design variable: the movements add. The card has no
// ID. COEFF is 0.0 when blank.
struct DesignNodeMoveCard
{
    base::EntityId designVarId = 0;
    base::EntityId nodeId = 0;
    base::EntityId frameId = 0;
    double coefficient = 0.0;
    std::array<double, 3> direction = {};
};

} // namespace oor::model
