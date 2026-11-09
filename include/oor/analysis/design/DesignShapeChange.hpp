#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/model/design/DesignUpdate.hpp"

#include <vector>

namespace oor::analysis
{

// The grid point moves the DVGRID cards give at a design point.
//
// a grid point moves by the sum, over the DVGRID cards that name it, of COEFF * (X - X0) * N, X
// the value of the design variable at the point, X0 its initial value (XINIT) and N the vector (N1, N2, N3) given in
// the system CID. N is taken as written (the format reference does not make it a unit vector) and, in a cylindrical or
// spherical CID, along the directions of that system at the grid point. The move is computed in the basic system and
// written to the grid point in its CP system (the positions that depend on the grid point, those of a CORD1x it
// defines, follow).
//
// A DVGRID is not applicable when its design variable or its grid point does not exist, when the point is not a GRID
// (the format reference also allows a POINT), when CID does not exist, or when the grid point moves
// and its CP system is defined by grid points that move too (its new coordinates would depend on the order of the
// moves).
class DesignShapeChange
{
public:
    // `variables` are the values of the design variables at the point, in the order of `design.variables()` (the
    // dependent ones from their DLINK: DesignModel::evaluate). The cards that are not applicable are in `skipped`, with
    // their reason in `problems`.
    [[nodiscard]] static model::DesignShapeUpdate compute(const model::Model& model, const model::DesignModel& design,
                                                          const std::vector<double>& variables);
};

} // namespace oor::analysis
