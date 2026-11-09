#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::analysis
{

// Where a point is in the model: the element that holds it, its natural coordinates there, the distance from the point
// to the element, and the shape-function weights of the element's grid points at the point (the building blocks for the
// plugin packages). No element: `element` is empty.
struct LocatedPoint
{
    std::optional<model::CardLocation> element;
    base::EntityId eid = 0;
    // A shell: xi and eta of its face (-1 to 1 on a quadrilateral from corner 1, the area coordinates L2 and L3 on a
    // triangle), zeta NaN. A solid: its natural coordinates (SolidShape.hpp).
    base::Vec3 natural;
    // A shell: from the point to its projection on the surface through the grid points (no thickness, no offset). A
    // solid: 0 inside, else to the element.
    double distance = 0.0;
    std::vector<base::EntityId> nodes;
    std::vector<double> weights; // parallel to nodes, summing to 1
};

// Finds the element that holds each point, among the shells and the solids of the model (or the elements given), within
// a distance:
//   * a shell holds a point that projects inside its face (the closest-point projection of the fastener piercing
//     search, on the bilinear or the quadratic surface through its corners and mid-side points), at its distance from
//     that surface;
//   * a solid holds a point inside it (Newton's method on its isoparametric mapping, the mid-side points included), or
//     within the distance of it;
//   * among the elements that hold a point, the nearest one, then the lowest element ID (a point on a shared edge or
//     face gets one element, the same every time).
// `tolerance`: the largest distance (nothing: 5 % of the median size of the elements). The elements are indexed in
// cells so that each point looks at the elements near it only; the points are spread over the pool.
class PointLocator
{
public:
    [[nodiscard]] static std::vector<LocatedPoint> locate(
        const model::Model& model, std::span<const base::Vec3> points,
        const std::optional<std::vector<model::CardLocation>>& elements, std::optional<double> tolerance,
        base::ThreadPool* pool = nullptr);
};

} // namespace oor::analysis
