#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/AirbagGas.hpp"

#include <optional>

namespace oor::model
{

// One INFLCG card (SOL 700): a cold gas inflator, a tank of compressed gas that empties into a gas bag (GBAG) or an
// Euler coupling surface (COUPLE) through the ABINFL that names the card. The tank volume stays constant, so the mass
// in it falls as the gas leaves. With an Euler domain it works only with the single material hydrodynamic solver and an
// EOSGAM equation of state.
//
// Card layout, two lines:
//   INFLCG  INFLID  TANKVOL  INITPRES  INITTEMP  INITMAS  GASNAM  CV  R            fields 1 to 8
//   CP                                                                              field 9
// INFLID numbers the card (unique among the inflators of every kind, which the ABINFL INFTYPID names).
//
// The initial state of the gas: INITPRES and INITMAS are each marked required, while the format also says that one of
// them is enough (the other follows from m = p V / (R T)) and that both should not be given. The card takes the
// lenient side of both rules: it needs at least one of the two and keeps both when both are given.
//
// The gas: GASNAM is the ratio of the specific heats when it is a real, or the number of an INFLGAS that defines the
// gas when it is an integer (see InflatorGasName); then the constants CV, R and CP stay blank. Otherwise the card gives
// the gas constants: the format reference asks for two of the four (the ratio in GASNAM, CV, R, CP; they are tied by
// gamma = CP / CV and R = CP - CV, and the solver derives the other two). The card does not count them, as the EOSGAM
// does not.
struct ColdGasInflatorCard
{
    base::EntityId id = 0;                      // unique number of the inflator
    double tankVolume = 0.0;                    // the volume of the tank, greater than zero
    std::optional<double> initialPressure;      // the initial pressure in the tank, greater than zero
    double initialTemperature = 0.0;            // the initial temperature in the tank, greater than zero
    std::optional<double> initialMass;          // the initial mass of gas, greater than zero
    InflatorGasName gasName;                    // the ratio of the specific heats, or an INFLGAS
    std::optional<double> specificHeatVolume;   // specific heat at constant volume
    std::optional<double> gasConstant;          // gas constant
    std::optional<double> specificHeatPressure; // specific heat at constant pressure
};

} // namespace oor::model
