#pragma once

#include "oor/analysis/components/ModelEntities.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace oor::analysis
{

// The grid points that rigid elements of zero length weld together: two coincident grid points are one point of the
// structure when rigid elements between them tie all six components, 123456, whether one element ties them all or
// several share them (an RBE2 with CM 123 and another with CM 456). Meshes are joined that way along a seam, and
// stacked elements are put on their own grid points that way: the topology (the panels, the chains of 1D elements, the
// components) follows the welds instead of the grid point IDs.
//
// What a rigid element ties: each dependent component of a grid point is tied to the grid point that holds that
// independent component (format reference: the independent components of a rigid element are held once, by one grid point). The
// elements that tie this way: RBE2, RBE1, RBAR, RBAR1, RJOINT, RTRPLT, RTRPLT1; an RBE3 or an RSPLINE interpolates, and
// an RROD ties a translation along an axis that zero length does not define. A tie counts only between grid points no
// farther apart than WELD_TOLERANCE in the basic system (the default distance of the check geometry.coincident).
class WeldedNodes
{
public:
    static constexpr double WELD_TOLERANCE = 1.0e-6;

    // The welds of the model; none when its grid points cannot be placed (a broken coordinate system). The elements
    // of `excluded` (element IDs, sorted: the connections a caller declares, a companion package) weld nothing.
    [[nodiscard]] static WeldedNodes build(const model::Model& model, const ModelEntities& entities,
                                           std::span<const base::EntityId> excluded = {});

    [[nodiscard]] bool empty() const noexcept { return m_nodes.empty(); }
    // The grid point that stands for the welded set of `node`: the smallest ID of the set; `node` itself when it is
    // welded to nothing.
    [[nodiscard]] base::EntityId representative(base::EntityId node) const noexcept;
    // Every welded grid point with the one that stands for its set, by increasing grid point ID (the representative
    // is listed too).
    [[nodiscard]] std::span<const std::pair<base::EntityId, base::EntityId>> nodes() const noexcept
    {
        return m_nodes;
    }
    // True for a rigid element of zero length whose grid points are all welded together: it is part of the weld, not a
    // connection between parts.
    [[nodiscard]] bool isWeld(model::CardLocation element) const noexcept;

private:
    std::vector<std::pair<base::EntityId, base::EntityId>> m_nodes;
    std::vector<std::uint64_t> m_welds; // the welding elements, (table << 32 | row), sorted
};

} // namespace oor::analysis
