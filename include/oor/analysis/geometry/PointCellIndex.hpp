#pragma once

#include "oor/base/math/Vec3.hpp"

#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace oor::analysis
{

// The pairs of points closer than a distance, found in time proportional to the number of points: the points are put
// in cubes of side `cellSize` (a cube is its three integer coordinates, kept sorted), and a point is compared with the
// points of the 27 cubes around its own only. Holds a copy of the points; immutable once made, so concurrent queries
// are safe.
class PointCellIndex
{
public:
    // `cellSize` must be at least the largest distance asked for later (a non-positive one is taken as 1).
    PointCellIndex(std::span<const base::Vec3> points, double cellSize);

    // Calls fn(i, j) once for every pair of points i < j (indices into the points given) whose distance is at most
    // `distance`, which must not exceed the cell size. The order of the calls is by i, then by j.
    void forEachPairWithin(double distance, const std::function<void(std::uint32_t, std::uint32_t)>& fn) const;

    [[nodiscard]] std::size_t size() const noexcept { return m_points.size(); }

private:
    struct CellEntry
    {
        std::int64_t x = 0;
        std::int64_t y = 0;
        std::int64_t z = 0;
        std::uint32_t point = 0;
    };

    [[nodiscard]] std::int64_t cellCoordinate(double value) const noexcept;

    std::vector<base::Vec3> m_points;
    double m_cellSize = 1.0;
    std::vector<CellEntry> m_cells; // sorted by (x, y, z, point)
};

} // namespace oor::analysis
