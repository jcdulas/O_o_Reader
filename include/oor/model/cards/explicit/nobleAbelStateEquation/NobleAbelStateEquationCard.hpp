#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One EOSNA card (SOL 700): the Noble-Abel equation of state of a gas, p = (gamma - 1) rho e / (1 - b rho), the ideal
// gas law corrected by the co-volume of the molecules. A MATDEUL names it in its EID field.
//
// Card layout: EOSNA  EID  GAMMA  B  R  CV  CP            fields 1 to 6
// GAMMA is required and not negative. B is the gas co-volume, not negative, zero when blank (it models the interaction
// of the gas particles). R, CV and CP are the gas constants of the format reference: a blank one is a constant the card
// does not give (the temperature of the gas is computed when one is given). The solver ties them together (gamma = cp /
// cv and R = cp - cv); the card does not check that the given ones agree, as EOSGAM does not.
struct NobleAbelStateEquationCard
{
    base::EntityId id = 0;                      // unique among the equations of state
    double gammaCoefficient = 0.0;              // not negative
    double coVolume = 0.0;                      // gas co-volume, not negative
    std::optional<double> gasConstant;          // gas constant, greater than zero
    std::optional<double> specificHeatVolume;   // specific heat at constant volume, greater than zero
    std::optional<double> specificHeatPressure; // specific heat at constant pressure, greater than zero
};

} // namespace oor::model
