#pragma once

#include "oor/base/math/Vec3.hpp"

#include <span>

// The volume of the solid elements, by the isoparametric rule: the integral of the Jacobian determinant of the mapping
// that the shape functions of the element define, by Gauss quadrature exact for the polynomial degree of the
// determinant. A mid-side node that is missing (NaN) is placed at the middle of its edge, so that an element given with
// its corners only is the one with straight edges. The element must be positively oriented for the sign of the result
// to be positive: the absolute value is returned.

namespace oor::analysis::detail
{

// Four corners and up to six mid-side nodes, in the order of the CTETRA card: G1..G4, then G5 (G1-G2), G6 (G2-G3), G7
// (G3-G1), G8 (G1-G4), G9 (G2-G4), G10 (G3-G4). `points` has 4 or 10 entries.
[[nodiscard]] double tetrahedronVolume(std::span<const base::Vec3> points);

// Six corners (G1..G3 one triangle, G4..G6 the other, G4 above G1) and up to nine mid-side nodes: G7 (G1-G2), G8
// (G2-G3), G9 (G3-G1), G10 (G1-G4), G11 (G2-G5), G12 (G3-G6), G13 (G4-G5), G14 (G5-G6), G15 (G6-G4). 6 or 15 entries.
[[nodiscard]] double wedgeVolume(std::span<const base::Vec3> points);

// Eight corners (G1..G4 one face, G5..G8 the opposite one, G5 opposite G1) and up to twelve mid-side nodes: G9 (G1-G2),
// G10 (G2-G3), G11 (G3-G4), G12 (G4-G1), G13 (G1-G5), G14 (G2-G6), G15 (G3-G7), G16 (G4-G8), G17 (G5-G6), G18 (G6-G7),
// G19 (G7-G8), G20 (G8-G5). 8 or 20 entries.
[[nodiscard]] double hexahedronVolume(std::span<const base::Vec3> points);

// Five corners (G1..G4 the base, G5 the apex) and up to eight mid-side nodes: G6..G9 on the base edges G1-G2, G2-G3,
// G3-G4, G4-G1 and G10..G13 on the edges G1-G5, G2-G5, G3-G5, G4-G5. 5 or 13 entries. The pyramid is the hexahedron
// whose top face is collapsed to the apex.
[[nodiscard]] double pyramidVolume(std::span<const base::Vec3> points);

// The Jacobian determinant of the isoparametric mapping at the corners and at the centre of the element, in the
// reference coordinates the volume rule integrates in (the cube [-1, 1]^3 for the hexahedron, the pyramid as its
// collapsed form, L2 L3 L4 for the tetrahedron, L1 L2 and zeta in [-1, 1] for the wedge), so that for a straight-edged
// element with a uniform mapping the determinant times the volume of the reference element is the volume. Positive for
// an element connected as the format reference draws it, negative where the mapping is inverted. The corners come in the order of
// the card, then the centre: 4 + 1 samples for the tetrahedron, 6 + 1 for the wedge, 8 + 1 for the hexahedron, only the
// 4 corners of the base for the pyramid (its apex is a singular point of the mapping and its centre is not comparable
// with the corners). The point lists are those of the volume functions above.
void tetrahedronJacobians(std::span<const base::Vec3> points, double (&out)[5]);
void wedgeJacobians(std::span<const base::Vec3> points, double (&out)[7]);
void hexahedronJacobians(std::span<const base::Vec3> points, double (&out)[9]);
void pyramidJacobians(std::span<const base::Vec3> points, double (&out)[4]);

} // namespace oor::analysis::detail
