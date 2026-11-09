#pragma once

#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <optional>

namespace oor::analysis
{

// The smallest box with its faces normal to the basic axes that holds every grid point of a model, positions taken in
// the basic system (through CP, so a grid point in a cylindrical system counts where it is, not at its coordinates).
struct BoundingBox
{
    base::Vec3 minimum;
    base::Vec3 maximum;
};

// The bounding box of the grid points of a model (its parts are not included); nothing when it has none. Throws
// CoordinateSystemError when a position cannot be computed.
[[nodiscard]] std::optional<BoundingBox> boundingBox(const model::Model& model, base::ThreadPool* pool = nullptr);

} // namespace oor::analysis
