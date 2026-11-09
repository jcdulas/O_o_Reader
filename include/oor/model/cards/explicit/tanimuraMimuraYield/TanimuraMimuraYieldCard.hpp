#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// The defaults of a YLDTM that are not zero: the specific heat and the melt temperature are 1.E20 (no heating, no
// melting), the room temperature is 293, the quasi-static and the reference strain rates, the critical yield stress and
// the strain rate exponent are 1.
inline constexpr double TANIMURA_MIMURA_YIELD_NO_LIMIT = 1.0E20;
inline constexpr double TANIMURA_MIMURA_YIELD_ROOM_TEMPERATURE = 293.0;

// One YLDTM card (SOL 700): the Tanimura-Mimura yield model, for metals over a wide range of strain rates. The yield
// stress depends on the effective plastic strain, on the strain rate (against a quasi-static and a reference rate) and
// on the temperature (between the room and the melt temperature). A MATDEUL names the model in its YID field.
//
// Card layout: YLDTM  YID    A      B      C      D      M      EPSM   CP         fields 1 to 8
//                    TMELT  TROOM  SCR    E      K      EPS0                     fields 9 to 14
// A is the static yield parameter (greater than zero, required). B (0.0), C (0.0), D (0.0), M (0.0, the temperature
// exponent), E (0.0) and K (1.0, the strain rate exponent) are the constants of the formula. EPSM (1.0) is the
// quasi-static strain rate, EPS0 (1.0) the reference strain rate (both per unit time) and SCR (1.0) the critical yield
// stress, all three greater than zero. CP is the specific heat (greater than zero, 1.E20), TMELT the melt temperature
// (1.E20) and TROOM the room temperature (293.0). The format reference gives no sign for the other constants, so the
// card only asks for finite numbers there.
struct TanimuraMimuraYieldCard
{
    base::EntityId id = 0;                                           // unique among the yield models
    double coefficientA = 0.0;                                       // static yield parameter, greater than zero
    double coefficientB = 0.0;                                       // hardening parameter
    double coefficientC = 0.0;                                       // strain rate parameter C
    double coefficientD = 0.0;                                       // strain rate parameter D
    double coefficientM = 0.0;                                       // temperature exponent
    double quasiStaticStrainRate = 1.0;                              // quasi-static strain rate, greater than zero
    double specificHeat = TANIMURA_MIMURA_YIELD_NO_LIMIT;            // specific heat, greater than zero
    double meltTemperature = TANIMURA_MIMURA_YIELD_NO_LIMIT;         // melt temperature
    double roomTemperature = TANIMURA_MIMURA_YIELD_ROOM_TEMPERATURE; // room temperature
    double criticalStress = 1.0;                                     // critical yield stress, greater than zero
    double coefficientE = 0.0;                                       // strain rate parameter E
    double coefficientK = 1.0;                                       // strain rate exponent
    double referenceStrainRate = 1.0;                                // reference strain rate, greater than zero
};

} // namespace oor::model
