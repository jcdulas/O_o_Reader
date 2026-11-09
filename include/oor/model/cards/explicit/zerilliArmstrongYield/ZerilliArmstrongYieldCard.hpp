#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// The default of CP: no heating of the material by the plastic work.
inline constexpr double ZERILLI_ARMSTRONG_YIELD_NO_LIMIT = 1.0E20;

// One YLDZA card (SOL 700): the Zerilli-Armstrong yield model. With E = exp(-M T + C T ln(rate / EPS0)) (T the
// temperature, rate the effective strain rate) the yield stress is (A + B p^N) E for a face centred cubic metal (iron
// and steels) and (A + B p^N) + D E for a body centred cubic one (aluminum and its alloys). A MATDEUL names the model
// in its YID field.
//
// Card layout: YLDZA  YID  A  B  N  C  M  EPS0  CP            fields 1 to 8
//                    D                                         field 9
// The grid of the format reference leaves field 8 empty while the description lists CP, the specific heat, between EPS0
// and D (as the other yield models with a temperature do): the card reads CP in field 8, and D, the one field the grid
// puts on the second line, in field 9. A is the static yield parameter (greater than zero, required); B (0.0, not
// negative), N (0.0), C (1.0) and M (1.0) are the constants of the formula; EPS0 is the reference strain rate per unit
// time (greater than zero, 1.0) and CP the specific heat (greater than zero, 1.E20). D is the parameter of the body
// centred form: a blank D (no value, `std::optional`) defines a face centred metal, a D (zero included) a body centred
// one. The grid writes N and M in lower case; the card reads and reports them as N and M.
struct ZerilliArmstrongYieldCard
{
    base::EntityId id = 0;                                  // unique among the yield models
    double coefficientA = 0.0;                              // static yield parameter, greater than zero
    double coefficientB = 0.0;                              // hardening parameter, not negative
    double coefficientN = 0.0;                              // hardening exponent
    double coefficientC = 1.0;                              // strain rate parameter
    double coefficientM = 1.0;                              // temperature exponent
    double referenceStrainRate = 1.0;                       // reference strain rate, greater than zero
    double specificHeat = ZERILLI_ARMSTRONG_YIELD_NO_LIMIT; // specific heat, greater than zero
    std::optional<double> coefficientD;                     // body centred parameter, blank for a face centred metal
};

} // namespace oor::model
