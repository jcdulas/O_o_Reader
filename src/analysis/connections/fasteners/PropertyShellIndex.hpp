#pragma once

#include "ShellElements.hpp"
#include "SurfaceProjection.hpp"

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace oor::analysis::detail
{

// The shell elements of one property (the surface of a CFAST PROP or a CWELD PARTPAT), with a uniform grid of cells
// over their boxes, so that the element nearest to a point is found by projecting the point on the elements of the
// cells around it rather than on every element of the property. Immutable once built: read by several threads.
class PropertyShellIndex
{
public:
    explicit PropertyShellIndex(std::vector<ShellElement> elements);

    [[nodiscard]] bool isEmpty() const noexcept { return m_elements.empty(); }
    [[nodiscard]] const std::vector<ShellElement>& elements() const noexcept { return m_elements; }
    // The element nearest to `point` among those on which it projects inside (within `tolerance`, natural units); on a
    // tie of distances the lowest element ID. Nothing when it projects inside none.
    [[nodiscard]] std::optional<HostedPoint> nearestInside(const base::Vec3& point, double tolerance) const;

private:
    struct Box
    {
        base::Vec3 low;
        base::Vec3 high;
    };
    [[nodiscard]] std::int64_t cellOf(double value, std::size_t axis) const noexcept;
    [[nodiscard]] static std::uint64_t keyOf(std::int64_t i, std::int64_t j, std::int64_t k) noexcept;

    std::vector<ShellElement> m_elements; // by element ID
    std::vector<Box> m_boxes;
    base::Vec3 m_origin;
    double m_cell = 1.0;
    std::int64_t m_extent = 0; // the number of cells along the longest axis
    std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> m_cells;
};

} // namespace oor::analysis::detail
