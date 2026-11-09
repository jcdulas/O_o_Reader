#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// The default of FC, the factor that keeps the yield surface away from its singular point.
inline constexpr double SNOW_MULTI_SURFACE_DEFAULT_COEFFICIENT_FC = 0.99;

// One YLDMSS card (SOL 700): the multi-surface yield model of snow. It is used with a MATDEUL, an EOSPOL and a SHREL,
// and only by the Euler solvers with strength. Besides the deviatoric plastic strain the model has a volumetric one.
//
// Card layout: YLDMSS  YID   KC    T     CC    AC    BC    FC    FTU        fields 1 to 8
//                     ALP0  DS                                              fields 9 and 10
// KC (the friction angle parameter), T (the equivalent cohesion), CC (the shape of the yield surface), AC and BC (the
// hardening parameters in compression) are required and greater than zero (but see CC below). FC lies between 0 and 1
// (0.99), FTU is the hydrostatic tensile strength (greater than zero), ALP0 is the initial compressive volumetric
// plastic strain (required, less than zero) and DS the softening modulus (not negative, 0.0).
//
// Two decisions the format leaves open:
// - A CC of zero is accepted although the field description says greater than zero: the format also says the material
//   is then a Drucker-Prager one and that AC, BC, FC and ALP0 are ignored. In that case the card does not require AC,
//   BC and ALP0 (a blank one is stored as 0.0 and written back as 0.) and does not check their sign.
// - A blank FTU is T / 3, which depends on T, so the card stores it resolved and always writes it. The solver lowers
//   an FTU above T / 3 to T / 3 (a common deck gives one); the card keeps what the deck says.
struct SnowMultiSurfaceYieldCard
{
    base::EntityId id = 0;                                            // unique among the yield models
    double coefficientKc = 0.0;                                       // greater than zero
    double coefficientT = 0.0;                                        // greater than zero
    double coefficientCc = 0.0;                                       // not negative: zero is the Drucker-Prager case
    double ac = 0.0;                                                  // greater than zero unless CC is zero
    double bc = 0.0;                                                  // greater than zero unless CC is zero
    double coefficientFc = SNOW_MULTI_SURFACE_DEFAULT_COEFFICIENT_FC; // greater than 0 and less than 1
    double coefficientFtu = 0.0; // greater than zero, stored resolved (T / 3 when blank)
    double initialAlpha = 0.0;   // less than zero unless CC is zero
    double coefficientDs = 0.0;  // not negative
};

} // namespace oor::model
