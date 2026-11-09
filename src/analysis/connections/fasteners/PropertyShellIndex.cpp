#include "PropertyShellIndex.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace oor::analysis::detail
{

namespace
{

// Below this many elements the point is projected on each of them: the grid would cost more than it saves.
constexpr std::size_t DIRECT_SEARCH = 64;
constexpr std::int64_t KEY_OFFSET = 1 << 20; // cell coordinates are kept in 21 bits each in a key
constexpr std::int64_t KEY_MASK = (1 << 21) - 1;
constexpr std::int64_t FAR_RINGS = 12; // rings of cells searched before the remaining elements are tried one by one

double boxDistance(const base::Vec3& low, const base::Vec3& high, const base::Vec3& point) noexcept
{
    double squared = 0.0;
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        const double below = low[axis] - point[axis];
        const double above = point[axis] - high[axis];
        const double gap = std::max({0.0, below, above});
        squared += gap * gap;
    }
    return std::sqrt(squared);
}

// Keeps `candidate` when it is nearer than `best`, or as near with a lower element ID.
void keepNearer(std::optional<HostedPoint>& best, const ShellElement& element, const SurfaceProjection& projection)
{
    if (best)
    {
        const double held = best->projection.distance;
        if (isSameDistance(projection.distance, held))
        {
            if (element.eid >= best->element.eid)
                return;
        }
        else if (projection.distance > held)
            return;
    }
    best = HostedPoint{element, projection};
}

} // namespace

PropertyShellIndex::PropertyShellIndex(std::vector<ShellElement> elements) : m_elements(std::move(elements))
{
    std::sort(m_elements.begin(), m_elements.end(),
              [](const ShellElement& left, const ShellElement& right) { return left.eid < right.eid; });
    if (m_elements.size() <= DIRECT_SEARCH)
        return;
    constexpr double HUGE_VALUE = std::numeric_limits<double>::max();
    base::Vec3 low{HUGE_VALUE, HUGE_VALUE, HUGE_VALUE};
    base::Vec3 high{-HUGE_VALUE, -HUGE_VALUE, -HUGE_VALUE};
    double sizeSum = 0.0;
    m_boxes.reserve(m_elements.size());
    for (const ShellElement& element : m_elements)
    {
        Box box{element.points[0], element.points[0]};
        for (std::size_t slot = 0; slot < element.slotCount(); ++slot)
        {
            if (!element.hasNode(slot))
                continue;
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                box.low[axis] = std::min(box.low[axis], element.points[slot][axis]);
                box.high[axis] = std::max(box.high[axis], element.points[slot][axis]);
            }
        }
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            low[axis] = std::min(low[axis], box.low[axis]);
            high[axis] = std::max(high[axis], box.high[axis]);
        }
        sizeSum += std::max({box.high.x - box.low.x, box.high.y - box.low.y, box.high.z - box.low.z});
        m_boxes.push_back(box);
    }
    const double span = std::max({high.x - low.x, high.y - low.y, high.z - low.z});
    // About one element per cell, and never more than KEY_OFFSET cells along an axis.
    m_cell = std::max({sizeSum / static_cast<double>(m_elements.size()), span / static_cast<double>(KEY_OFFSET - 1),
                       std::numeric_limits<double>::min()});
    m_origin = low;
    m_extent = static_cast<std::int64_t>(std::ceil(span / m_cell)) + 1;
    for (std::size_t index = 0; index < m_boxes.size(); ++index)
    {
        const Box& box = m_boxes[index];
        for (std::int64_t i = cellOf(box.low.x, 0); i <= cellOf(box.high.x, 0); ++i)
            for (std::int64_t j = cellOf(box.low.y, 1); j <= cellOf(box.high.y, 1); ++j)
                for (std::int64_t k = cellOf(box.low.z, 2); k <= cellOf(box.high.z, 2); ++k)
                    m_cells[keyOf(i, j, k)].push_back(static_cast<std::uint32_t>(index));
    }
}

std::int64_t PropertyShellIndex::cellOf(double value, std::size_t axis) const noexcept
{
    return static_cast<std::int64_t>(std::floor((value - m_origin[axis]) / m_cell));
}

std::uint64_t PropertyShellIndex::keyOf(std::int64_t i, std::int64_t j, std::int64_t k) noexcept
{
    const auto pack = [](std::int64_t value) { return static_cast<std::uint64_t>((value + KEY_OFFSET) & KEY_MASK); };
    return (pack(i) << 42U) | (pack(j) << 21U) | pack(k);
}

std::optional<HostedPoint> PropertyShellIndex::nearestInside(const base::Vec3& point, double tolerance) const
{
    std::optional<HostedPoint> best;
    if (m_boxes.empty())
    {
        for (const ShellElement& element : m_elements)
        {
            const SurfaceProjection projection = SurfaceProjection::onto(element, point);
            if (projection.isInside(tolerance))
                keepNearer(best, element, projection);
        }
        return best;
    }
    const std::int64_t ci = cellOf(point.x, 0);
    const std::int64_t cj = cellOf(point.y, 1);
    const std::int64_t ck = cellOf(point.z, 2);
    // The rings of cells around the cell of the point, from the first that meets the grid to the last that does.
    const auto reach = [&](std::int64_t cell) { return std::max(std::abs(cell), std::abs(cell - m_extent)); };
    const auto before = [&](std::int64_t cell) { return std::max<std::int64_t>({0, -cell, cell - m_extent}); };
    const std::int64_t firstRing = std::max({before(ci), before(cj), before(ck)});
    const std::int64_t lastRing = std::max({reach(ci), reach(cj), reach(ck)});
    std::vector<std::uint8_t> visited(m_elements.size(), 0);
    const auto tryElement = [&](std::size_t index)
    {
        visited[index] = 1;
        const Box& box = m_boxes[index];
        if (best && boxDistance(box.low, box.high, point) > best->projection.distance + m_cell * 1e-9)
            return;
        const SurfaceProjection projection = SurfaceProjection::onto(m_elements[index], point);
        if (projection.isInside(tolerance))
            keepNearer(best, m_elements[index], projection);
    };
    for (std::int64_t ring = firstRing; ring <= lastRing; ++ring)
    {
        // An element not met yet lies outside the cube of cells of the rings before: at least (ring - 1) cells away.
        if (best && static_cast<double>(ring - 1) * m_cell > best->projection.distance)
            break;
        if (ring > firstRing + FAR_RINGS)
        {
            // A point far from the elements it projects on (or on none): the remaining elements one by one, which
            // costs less than the rings of empty cells.
            for (std::size_t index = 0; index < m_elements.size(); ++index)
            {
                if (visited[index] == 0)
                    tryElement(index);
            }
            break;
        }
        for (std::int64_t i = ci - ring; i <= ci + ring; ++i)
            for (std::int64_t j = cj - ring; j <= cj + ring; ++j)
                for (std::int64_t k = ck - ring; k <= ck + ring; ++k)
                {
                    if (std::max({std::abs(i - ci), std::abs(j - cj), std::abs(k - ck)}) != ring)
                        continue;
                    const auto cell = m_cells.find(keyOf(i, j, k));
                    if (cell == m_cells.end())
                        continue;
                    for (const std::uint32_t index : cell->second)
                    {
                        if (visited[index] == 0)
                            tryElement(index);
                    }
                }
    }
    return best;
}

} // namespace oor::analysis::detail
