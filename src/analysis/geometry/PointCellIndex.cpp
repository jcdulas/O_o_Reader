#include "oor/analysis/geometry/PointCellIndex.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace oor::analysis
{

PointCellIndex::PointCellIndex(std::span<const base::Vec3> points, double cellSize)
    : m_points(points.begin(), points.end()), m_cellSize(cellSize > 0.0 ? cellSize : 1.0)
{
    m_cells.reserve(m_points.size());
    for (std::uint32_t index = 0; index < m_points.size(); ++index)
    {
        const base::Vec3& point = m_points[index];
        m_cells.push_back(CellEntry{cellCoordinate(point.x), cellCoordinate(point.y), cellCoordinate(point.z), index});
    }
    std::sort(
        m_cells.begin(), m_cells.end(), [](const CellEntry& left, const CellEntry& right)
        { return std::tie(left.x, left.y, left.z, left.point) < std::tie(right.x, right.y, right.z, right.point); });
}

std::int64_t PointCellIndex::cellCoordinate(double value) const noexcept
{
    return static_cast<std::int64_t>(std::floor(value / m_cellSize));
}

void PointCellIndex::forEachPairWithin(double distance,
                                       const std::function<void(std::uint32_t, std::uint32_t)>& fn) const
{
    const double squared = distance * distance;
    std::vector<std::uint32_t> neighbours;
    for (std::uint32_t index = 0; index < m_points.size(); ++index)
    {
        const base::Vec3& point = m_points[index];
        const std::int64_t x = cellCoordinate(point.x);
        const std::int64_t y = cellCoordinate(point.y);
        const std::int64_t z = cellCoordinate(point.z);
        neighbours.clear();
        for (std::int64_t dx = -1; dx <= 1; ++dx)
        {
            for (std::int64_t dy = -1; dy <= 1; ++dy)
            {
                for (std::int64_t dz = -1; dz <= 1; ++dz)
                {
                    const CellEntry key{x + dx, y + dy, z + dz, 0};
                    auto it = std::lower_bound(
                        m_cells.begin(), m_cells.end(), key, [](const CellEntry& left, const CellEntry& right)
                        { return std::tie(left.x, left.y, left.z) < std::tie(right.x, right.y, right.z); });
                    for (; it != m_cells.end() && it->x == key.x && it->y == key.y && it->z == key.z; ++it)
                    {
                        if (it->point <= index)
                            continue;
                        const base::Vec3 offset = m_points[it->point] - point;
                        if (base::dot(offset, offset) <= squared)
                            neighbours.push_back(it->point);
                    }
                }
            }
        }
        std::sort(neighbours.begin(), neighbours.end());
        for (const std::uint32_t other : neighbours)
            fn(index, other);
    }
}

} // namespace oor::analysis
