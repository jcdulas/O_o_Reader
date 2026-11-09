#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/AirbagGas.hpp"

#include <optional>

namespace oor::model
{

// One INFLTR card (SOL 700): a standard airbag inflator. A GBAG or an Euler coupling surface (COUPLE) receives the gas
// through the ABINFL that names the card: the mass flow rate of the inflowing gas is a table of time, and so is (or is
// a constant) the static temperature of the gas.
//
// Card layout, two lines:
//   INFLTR  INFLID  MASFLRT  TEMPT  TEMPC  GASNAM  CV  R  CP                        fields 1 to 8
//   MID                                                                             field 9
// (a common deck puts a 0 in fields 10 and 11 of the second line as well; the layout has no such fields, and the card
// refuses a value there.)
// INFLID numbers the card (unique among the inflators of every kind, which the ABINFL INFTYPID names). MASFLRT is the
// number of a TABLED1 (required), the mass flow rate against time. The static temperature of the inflowing gas is
// TEMPT, the number of a TABLED1, or TEMPC, a constant: the format reference says that either one must be given, and
// the card reads them as the two forms of one field (exactly one of them).
//
// The gas: GASNAM is the ratio of the specific heats when it is a real, or the number of an INFLGAS that defines the
// gas when it is an integer (see InflatorGasName); then the constants CV, R and CP stay blank. Otherwise the card gives
// the gas constants: the format reference asks for two of the four (the ratio in GASNAM, CV, R, CP) and the solver
// derives the others. The card does not count them, as the EOSGAM does not.
//
// MID is the MATDEUL of the inflator material, used only by the multi-material solver; the single material solver
// needs an EOSGAM equation of state instead and leaves it blank. The format allows zero, so a zero is a blank.
struct StandardInflatorCard
{
    base::EntityId id = 0;                      // unique number of the inflator
    base::EntityId massFlowTableId = 0;         // a TABLED1: the mass flow rate against time
    base::EntityId temperatureTableId = 0;      // a TABLED1: the static temperature against time, 0 when TEMPC is given
    std::optional<double> temperature;          // the constant static temperature, greater than zero
    InflatorGasName gasName;                    // the ratio of the specific heats, or an INFLGAS
    std::optional<double> specificHeatVolume;   // specific heat at constant volume
    std::optional<double> gasConstant;          // gas constant
    std::optional<double> specificHeatPressure; // specific heat at constant pressure
    base::EntityId matId = 0;                   // the MATDEUL of the inflator material, 0 for none
};

} // namespace oor::model
