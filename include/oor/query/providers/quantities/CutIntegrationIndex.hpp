#pragma once

#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/providers/quantities/ElementRows.hpp"
#include "oor/query/results/ResultSets.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace oor::query
{

// The shells the integration of the cuts of a call reads, indexed once for the call: the geometry of
// each shell that has a force row (its corners, its axes, its extent), the rows of its forces, and with an axis the
// shells sorted by the position of their centre along it, so that a plane normal to the axis looks only at the shells
// whose extent can reach it.
struct CutIntegrationIndex
{
    using Point = std::array<double, 3>;

    struct Shell
    {
        std::int32_t modelId = 0;
        bool ok = false; // a shell whose frame and three corners are known
        std::vector<Point> corners;
        std::vector<std::int32_t> cornerNodes;
        std::array<Point, 3> axes{}; // x, y, z of the element system in the basic system
        Point centre{};
        double radius = 0.0; // the largest distance of a corner from the centre
        // the centre rows of the shell in each layout of rows: (layout, row); its force rows are that row of each
        // flat of the layout (`forEachRow`)
        std::vector<std::pair<std::uint32_t, std::uint32_t>> places;
    };

    std::vector<Shell> shells;
    // the flats of each layout of rows, in the order of the force set: the flats of the subcases mostly have the same
    // rows (the same elements and places), which are linked to the shells once
    std::vector<std::vector<std::uint32_t>> layoutFlats;
    std::unordered_map<std::int32_t, std::uint32_t> byModelId;
    // the shells sorted by their centre along `axis`, and the largest radius (none without an axis)
    std::optional<Point> axis;
    std::vector<std::uint32_t> alongAxis;
    std::vector<double> positionAlong;
    double largestRadius = 0.0;

    // The index of the shells of `forces` (the centre rows of shells linked to the model), with their geometry from
    // the model; sorted along `axis` (a unit vector) when one is given.
    [[nodiscard]] static CutIntegrationIndex build(const ResultQueryContext& context, const model::Model& model,
                                                   const ElementFlatSet& forces, const NodePositions& positions,
                                                   const TopologyList& topology, const std::optional<Point>& axis);

    // The shells a plane (a unit normal, a point) can cross or lie against: those whose corners can reach it, a
    // superset of the ones it does; in the order of `shells`.
    [[nodiscard]] std::vector<std::uint32_t> nearPlane(const Point& normal, const Point& point) const;

    // Calls `take(flat, row)` for each force row of a shell (in no particular order).
    template <typename Take>
    void forEachRow(const Shell& shell, const Take& take) const
    {
        for (const auto& [layout, row] : shell.places)
            for (const std::uint32_t flat : layoutFlats[layout])
                take(flat, row);
    }
};

} // namespace oor::query
