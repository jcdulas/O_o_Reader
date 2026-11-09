#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One SHRPOL card (SOL 700): an elastic shear model whose modulus is a cubic polynomial of the effective plastic shear
// strain gamma: G = G0 + G1 gamma + G2 gamma^2 + G3 gamma^3. A MATDEUL names it in its SID field.
//
// Card layout: SHRPOL  SID  G0  G1  G2  G3            fields 1 to 5
// The four coefficients are reals, zero when blank; the format gives no range for them (a hardening law may have
// negative coefficients), so the card checks only that they are finite.
struct PolynomialShearModelCard
{
    base::EntityId id = 0; // unique among the shear models
    double shearModulus0 = 0.0;
    double shearModulus1 = 0.0;
    double shearModulus2 = 0.0;
    double shearModulus3 = 0.0;
};

} // namespace oor::model
