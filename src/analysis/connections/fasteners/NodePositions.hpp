#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/model/core/Model.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace oor::analysis::detail
{

// The positions of the grid points in the basic system, found by ID (a scalar point has none). A view of the positions
// of BasicPositionCache: valid while the model is not edited.
class NodePositions
{
public:
    NodePositions(const model::Model& model, std::span<const std::array<double, 3>> positions);

    [[nodiscard]] std::optional<base::Vec3> of(base::EntityId node) const noexcept;

private:
    const model::Model& m_model;
    std::span<const std::array<double, 3>> m_positions;
    std::uint16_t m_nodeType = 0;
};

} // namespace oor::analysis::detail
