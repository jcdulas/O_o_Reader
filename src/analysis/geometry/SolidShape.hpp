#pragma once

#include "oor/base/math/Vec3.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

// The isoparametric shape functions of the solid elements, and the natural coordinates of a point in an element
// (`locate_points`). The grid points come in the order of the card (corners, then the mid-side points in the
// format reference order of SolidVolume.hpp); a mid-side point that is not there (bit off in `present`) gives its function half to
// each corner of its edge, so the edge stays straight and the weights are those of the grid points that exist.
//
// Natural coordinates: a hexahedron (xi, eta, zeta) in [-1, 1]^3, corner 1 at (-1, -1, -1) as HEXAHEDRON_REFERENCE; a
// tetrahedron the volume coordinates (L2, L3, L4), L1 = 1 - L2 - L3 - L4; a pentahedron (L2, L3) of its triangles and
// zeta from -1 (G1 G2 G3) to 1 (G4 G5 G6); a pyramid is the hexahedron whose top face is collapsed to the apex.

namespace oor::analysis::detail
{

enum class SolidShapeKind : std::uint8_t
{
    Tetrahedron, // 4 corners, up to 6 mid-side points
    Pentahedron, // 6 corners, up to 9
    Hexahedron,  // 8 corners, up to 12
    Pyramid      // 5 corners, up to 8
};

constexpr std::size_t MAX_SOLID_NODES = 20;

// One solid with the positions of its grid points (slots past the corners are the mid-side points; `present` bit k:
// slot k holds a grid point).
struct SolidElement
{
    SolidShapeKind kind = SolidShapeKind::Hexahedron;
    std::uint32_t present = 0;
    std::array<base::Vec3, MAX_SOLID_NODES> points{};

    [[nodiscard]] std::size_t cornerCount() const noexcept;
    [[nodiscard]] std::size_t slotCount() const noexcept;
    [[nodiscard]] bool hasNode(std::size_t slot) const noexcept { return (present & (1U << slot)) != 0; }
};

// The weight of each slot at a natural point (0 for a slot with no grid point).
[[nodiscard]] std::array<double, MAX_SOLID_NODES> solidWeights(const SolidElement& element, const base::Vec3& natural);

// Where a point is in an element: its natural coordinates (found by Newton's method on the mapping), how far outside
// the element they are (0 inside, in natural units), and the distance from the point to the element: 0 inside, else to
// the point of the element at the natural coordinates brought back inside.
struct SolidLocation
{
    base::Vec3 natural;
    double outside = 0.0;
    double distance = 0.0;
};

[[nodiscard]] SolidLocation locateInSolid(const SolidElement& element, const base::Vec3& point);

} // namespace oor::analysis::detail
