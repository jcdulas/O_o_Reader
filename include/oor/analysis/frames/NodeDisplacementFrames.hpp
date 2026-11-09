#pragma once

#include "oor/base/math/Mat3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace oor::analysis
{

// The displacement coordinate system (CD) of grid points, as a rotation to the basic system, evaluated at each grid
// point: in a cylindrical or spherical system the directions of the coordinates change from point to point, so two grid
// points with the same CD generally have different rotations.
//
// The matrix of a grid point has the directions 1, 2, 3 of its CD system as columns, in the basic system: the
// displacement (u1, u2, u3) in the CD system is `rotation * (u1, u2, u3)` in the basic system, and the displacement u
// in the basic system is `rotation.transposed() * u` in the CD system. A grid point with CD 0 (or -1, a point that is
// not in any system) has the identity.
class NodeDisplacementFrames
{
public:
    // The rotations of the grid points in `rows` (rows of the GRID table, live ones). Throws CoordinateSystemError when
    // a CD names a system that does not exist, or when the positions of the grid points cannot be computed.
    [[nodiscard]] static std::vector<base::Mat3> rotations(const model::Model& model,
                                                           std::span<const std::uint32_t> rows,
                                                           base::ThreadPool* pool = nullptr);
};

} // namespace oor::analysis
