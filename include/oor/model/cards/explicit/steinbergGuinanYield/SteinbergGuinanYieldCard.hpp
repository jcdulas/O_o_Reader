#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// The defaults of a YLDSG that are not zero: the specific heat and the melt temperature are 1.E20 (no heating, no
// melting), the room temperature is 293.
inline constexpr double STEINBERG_GUINAN_YIELD_NO_LIMIT = 1.0E20;
inline constexpr double STEINBERG_GUINAN_YIELD_ROOM_TEMPERATURE = 293.0;

// One YLDSG card (SOL 700): the Steinberg-Guinan yield model, for metals over a wide range of strain rates. The yield
// stress is a function of the effective plastic strain p, of the pressure (through the density ratio) and of the
// temperature T: with AT = A1 (1 + A3 p)^A4 it is min(A2, AT) (1 - H (T - TROOM) + B p (rho / rho_ref)^(1/3)) below the
// melt temperature and zero above it. A MATDEUL names the model in its YID field.
//
// Card layout: YLDSG  YID    A1    A2    A3    A4    H     B     CP         fields 1 to 8
//                    TMELT  TROOM                                          fields 9 and 10
// A1 to A4 are the yield parameters (greater than zero, required); H (0.0) and B (0.0) are the other two constants of
// the formula; CP is the specific heat (greater than zero, 1.E20), TMELT the melt temperature (1.E20) and TROOM the
// room temperature (293.0).
struct SteinbergGuinanYieldCard
{
    base::EntityId id = 0;                                            // unique among the yield models
    double coefficientA1 = 0.0;                                       // greater than zero
    double coefficientA2 = 0.0;                                       // greater than zero
    double coefficientA3 = 0.0;                                       // greater than zero
    double coefficientA4 = 0.0;                                       // greater than zero
    double coefficientH = 0.0;                                        // yield parameter
    double coefficientB = 0.0;                                        // yield parameter
    double specificHeat = STEINBERG_GUINAN_YIELD_NO_LIMIT;            // specific heat, greater than zero
    double meltTemperature = STEINBERG_GUINAN_YIELD_NO_LIMIT;         // melt temperature
    double roomTemperature = STEINBERG_GUINAN_YIELD_ROOM_TEMPERATURE; // room temperature
};

} // namespace oor::model
