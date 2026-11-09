#pragma once

namespace oor::query
{

// The interaction of a fastener under tension T and shear S against its allowables Ta and Sa
// (margins): (T / Ta)^a + (S / Sa)^b, the exponents 2 and 3 by default. T is the tension only (a compression along the
// axis is given as 0 by the caller), S the resultant of the two shear forces.
[[nodiscard]] double fastenerInteraction(double tension, double shear, double tensionAllowable, double shearAllowable,
                                         double tensionExponent, double shearExponent) noexcept;

// The factor u on the loads that brings the interaction to 1 (it grows with u): the reserve factor of the fastener
// before the safety factor. NaN when both loads are zero (nothing to scale).
[[nodiscard]] double fastenerLoadMultiplier(double tension, double shear, double tensionAllowable,
                                            double shearAllowable, double tensionExponent,
                                            double shearExponent) noexcept;

} // namespace oor::query
