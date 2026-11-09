#pragma once

#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/analysis/quality/QualityMetrics.hpp"
#include "oor/base/math/Vec3.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <span>

// The small pieces the metrics of every element shape are made of: distances and angles, the statistics of the edges,
// the position of the mid-side nodes, the Jacobian samples. Each shape family has its own file that puts them together.

namespace oor::analysis::detail
{

using base::Vec3;

inline constexpr double RADIANS_TO_DEGREES = 180.0 / std::numbers::pi;

[[nodiscard]] inline double distanceBetween(const Vec3& first, const Vec3& second) noexcept
{
    return base::norm(first - second);
}

// The angle between two vectors in degrees, from 0 to 180; NaN when one of them is zero.
[[nodiscard]] inline double angleBetween(const Vec3& first, const Vec3& second) noexcept
{
    if (!(base::norm(first) > 0.0) || !(base::norm(second) > 0.0))
        return NOT_DEFINED;
    return std::atan2(base::norm(cross(first, second)), dot(first, second)) * RADIANS_TO_DEGREES;
}

// The cosine of the angle between two vectors; NaN when one of them is zero.
[[nodiscard]] inline double cosineBetween(const Vec3& first, const Vec3& second) noexcept
{
    const double scale = base::norm(first) * base::norm(second);
    if (!(scale > 0.0))
        return NOT_DEFINED;
    return std::clamp(dot(first, second) / scale, -1.0, 1.0);
}

[[nodiscard]] inline bool isPresent(const Vec3& point) noexcept
{
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

// The longest over the shortest; NaN for a zero shortest.
[[nodiscard]] inline double ratioOf(double longest, double shortest) noexcept
{
    return shortest > 0.0 ? longest / shortest : NOT_DEFINED;
}

// The minimum and the maximum of what is added, NaN until something is.
struct Range
{
    double smallest = std::numeric_limits<double>::infinity();
    double largest = -std::numeric_limits<double>::infinity();
    bool any = false;

    void add(double value) noexcept
    {
        if (!(value == value))
            return;
        any = true;
        smallest = std::min(smallest, value);
        largest = std::max(largest, value);
    }
    [[nodiscard]] double min() const noexcept { return any ? smallest : NOT_DEFINED; }
    [[nodiscard]] double max() const noexcept { return any ? largest : NOT_DEFINED; }
};

// The edge point metrics of the mid-side nodes: the position of a node along the line between the corners of its edge,
// and the angle it makes with them. A node that is not there is skipped.
class EdgePointStatistics
{
public:
    void add(const Vec3& first, const Vec3& second, const Vec3& middle) noexcept
    {
        if (!isPresent(middle))
            return;
        const Vec3 along = second - first;
        const double length = dot(along, along);
        if (!(length > 0.0))
            return;
        const double position = dot(middle - first, along) / length;
        m_lengthRatio.add(std::fabs(2.0 * position - 1.0));
        m_angle.add(angleBetween(first - middle, second - middle));
    }
    [[nodiscard]] double lengthRatio() const noexcept { return m_lengthRatio.max(); }
    [[nodiscard]] double angle() const noexcept { return m_angle.min(); }

private:
    Range m_lengthRatio;
    Range m_angle;
};

// The smallest over the largest absolute value of the determinants, negative when one is: the Jacobian ratio; and the
// smallest determinant. NaN when one of the values is not a number or all are zero.
inline void jacobianStatistics(std::span<const double> determinants, QualityValues& values) noexcept
{
    double smallest = std::numeric_limits<double>::infinity();
    double largestAbsolute = 0.0;
    for (const double value : determinants)
    {
        if (!std::isfinite(value))
            return;
        smallest = std::min(smallest, value);
        largestAbsolute = std::max(largestAbsolute, std::fabs(value));
    }
    values.jacobianMin = smallest;
    values.jacobianRatio = largestAbsolute > 0.0 ? smallest / largestAbsolute : NOT_DEFINED;
}

} // namespace oor::analysis::detail
