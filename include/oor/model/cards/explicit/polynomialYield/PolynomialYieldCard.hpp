#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// The default of SMAX: no upper bound on the yield stress.
inline constexpr double POLYNOMIAL_YIELD_NO_LIMIT = 1.0E20;

// One YLDPOL card (SOL 700): the polynomial yield model. The yield stress is a polynomial of the effective plastic
// strain p, cut off at SMAX: y = min(SMAX, A + B p + C p^2 + D p^3 + E p^4 + F p^5). A MATDEUL names the model in its
// YID field.
//
// Card layout: YLDPOL  YID  A  B  C  D  E  F  SMAX            fields 1 to 8
// A is the initial yield stress (greater than zero, required); B to F (0.0) are the coefficients of the higher terms;
// SMAX (1.E20) is the maximum yield stress. The format reference gives no sign for the coefficients and SMAX, so the
// card only asks for finite numbers.
struct PolynomialYieldCard
{
    base::EntityId id = 0;                             // unique among the yield models
    double coefficientA = 0.0;                         // initial yield stress, greater than zero
    double coefficientB = 0.0;                         // coefficient of p
    double coefficientC = 0.0;                         // coefficient of p^2
    double coefficientD = 0.0;                         // coefficient of p^3
    double coefficientE = 0.0;                         // coefficient of p^4
    double coefficientF = 0.0;                         // coefficient of p^5
    double maxYieldStress = POLYNOMIAL_YIELD_NO_LIMIT; // maximum yield stress
};

} // namespace oor::model
