#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// The defaults of a YLDJC that are not zero: the specific heat and the melt temperature are 1.E20 (no heating, no
// melting), the room temperature is 293.
inline constexpr double JOHNSON_COOK_YIELD_NO_LIMIT = 1.0E20;
inline constexpr double JOHNSON_COOK_YIELD_ROOM_TEMPERATURE = 293.0;

// One YLDJC card (SOL 700): the Johnson-Cook yield model. The yield stress grows with the effective plastic strain,
// with the strain rate and falls with the temperature, as
//   y = (A + B p^n) (1 + C ln(rate / EPS0)) (1 - T*^m),   T* = (T - TROOM) / (TMELT - TROOM).
// A MATDEUL names the model in its YID field.
//
// Card layout: YLDJC  YID  A     B     N     C     M     EPS0  CP         fields 1 to 8
//                    TMELT TROOM                                          fields 9 and 10
// A is the static yield stress (not negative, required). B (0.0), C (0.0), N (1.0) and M (1.0) are the constants of the
// formula, EPS0 the reference strain rate per unit time (greater than zero, 1.0), CP the specific heat (greater than
// zero, 1.E20), TMELT the melt temperature (1.E20) and TROOM the room temperature (293.0). The lower case n and m of
// the grid are N and M in the description of the fields; the card reads and reports them as N and M.
struct JohnsonCookYieldCard
{
    base::EntityId id = 0;                                        // unique among the yield models
    double coefficientA = 0.0;                                    // static yield stress, not negative
    double coefficientB = 0.0;                                    // hardening parameter
    double coefficientN = 1.0;                                    // hardening exponent
    double coefficientC = 0.0;                                    // strain-rate parameter
    double coefficientM = 1.0;                                    // temperature exponent
    double referenceStrainRate = 1.0;                             // reference strain rate, greater than zero
    double specificHeat = JOHNSON_COOK_YIELD_NO_LIMIT;            // specific heat, greater than zero
    double meltTemperature = JOHNSON_COOK_YIELD_NO_LIMIT;         // melt temperature
    double roomTemperature = JOHNSON_COOK_YIELD_ROOM_TEMPERATURE; // room temperature
};

} // namespace oor::model
