#pragma once

#include "ShellElements.hpp"
#include "PropertyShellIndex.hpp"
#include "SurfaceProjection.hpp"

#include "oor/analysis/adjacency/ElementIndexes.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/model/core/Model.hpp"

#include <map>
#include <optional>
#include <vector>

namespace oor::analysis::detail
{

// The elements a piercing point is searched on: an element and the shell elements that share a grid point with it, or
// the elements of a property, and the rule that picks the host among them.
class ShellCandidates
{
public:
    ShellCandidates(const model::Model& model, const ShellElements& elements, const NodeToElementIndex& nodeIndex);

    // The shell elements that share a grid point with `element` (itself left out), by element ID.
    [[nodiscard]] std::vector<ShellElement> ringOf(const ShellElement& element) const;
    // The index of the elements of a property, built by prepare(); null when the property has no shell element.
    [[nodiscard]] const PropertyShellIndex* property(base::EntityId pid) const noexcept;
    // Builds the indexes of these properties, once each (before the work spreads over threads).
    void prepare(const std::vector<base::EntityId>& pids);

    // The nearest element of `candidates` on which the point projects inside; on a tie of distances (a point on an edge
    // shared by two elements) the lowest element ID.
    [[nodiscard]] static std::optional<HostedPoint> nearestInside(const std::vector<ShellElement>& candidates,
                                                                  const base::Vec3& point, double tolerance);

private:
    const model::Model& m_model;
    const ShellElements& m_elements;
    const NodeToElementIndex& m_nodeIndex;
    std::map<base::EntityId, PropertyShellIndex> m_properties;
};

} // namespace oor::analysis::detail
