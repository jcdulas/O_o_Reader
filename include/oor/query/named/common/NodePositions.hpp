#pragma once

#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace oor::query
{

// The positions of the grid points of a model in the basic system, found by node ID. The cache is brought up to date
// when the object is made (it can throw CoordinateSystemError); the object is valid until the model is edited.
class NodePositions
{
public:
    NodePositions(const model::Model& model, base::ThreadPool* pool);

    // True when the node is a grid point of the model.
    [[nodiscard]] bool has(base::EntityId node) const noexcept { return row(node) >= 0; }
    // The position of a grid point (the zero vector when `has` is false).
    [[nodiscard]] base::Vec3 position(base::EntityId node) const noexcept;
    // The distance between two grid points; NaN when either is not a grid point.
    [[nodiscard]] double distance(base::EntityId first, base::EntityId second) const noexcept;

private:
    [[nodiscard]] std::int64_t row(base::EntityId node) const noexcept;

    const model::Model& m_model;
    std::uint16_t m_nodeType;
    std::span<const std::array<double, 3>> m_positions;
};

} // namespace oor::query
