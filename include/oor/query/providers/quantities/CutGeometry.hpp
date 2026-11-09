#pragma once

#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/results/ResultSets.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace oor::query
{

struct CutDefinition;

// The geometry every cut of a call is made from, built once: the elements with their grid points and centres, the
// positions of the grid points, and the grid points that some element uses. A hundred stations of `sectionCuts`
// classify the same elements a hundred times and build these once.
class CutGeometry
{
public:
    // `restrict` limits the plane cuts to a group of elements (model IDs); the cuts along grid points and the free
    // bodies always see every element.
    CutGeometry(const ResultQueryContext& context, const std::optional<std::vector<std::int64_t>>& restrict);

    // The cut by a plane (a point and a normal, in the basic system): the elements whose centre is on the negative side
    // of the plane (the side the normal points away from) are the side, the grid points they share with the elements
    // of the other side are the cut.
    [[nodiscard]] CutDefinition byPlane(const std::array<double, 3>& point, const std::array<double, 3>& normal) const;
    // The cuts by the planes of one normal through each of `points` (the stations along an axis), each what byPlane
    // gives, found in one sweep along the normal: from one plane to the next only the elements that change sides are
    // looked at again.
    [[nodiscard]] std::vector<CutDefinition> byParallelPlanes(const std::vector<std::array<double, 3>>& points,
                                                              const std::array<double, 3>& normal) const;
    // The cut along a line of grid points with the elements of one side.
    [[nodiscard]] CutDefinition byNodes(const std::vector<std::int64_t>& nodes,
                                        const std::vector<std::int64_t>& sideElements) const;
    // A free body: the elements, and the grid points where it is cut from the rest of the model (those it shares with
    // elements that are not in it, or `nodes` when given).
    [[nodiscard]] CutDefinition freebody(const std::vector<std::int64_t>& elements,
                                         const std::optional<std::vector<std::int64_t>>& nodes) const;

    [[nodiscard]] const NodePositions& positions() const noexcept { return m_positions; }

private:
    // Fills what every form of cut gives: the interior grid points, the side elements on the cut, the centroid.
    [[nodiscard]] CutDefinition finish(CutDefinition cut) const;

    const model::Model& m_model;
    NodePositions m_positions;
    TopologyList m_all;                                              // every element with grid points
    std::optional<TopologyList> m_restricted;                        // the group of the plane cuts, when given
    std::vector<std::array<double, 3>> m_centres;                    // of the elements the plane cuts see
    std::vector<std::uint8_t> m_hasCentre;                           // 0 for an element with no placed grid point
    std::shared_ptr<const std::vector<std::int32_t>> m_elementNodes; // sorted
};

} // namespace oor::query
