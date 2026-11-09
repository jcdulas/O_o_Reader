#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One EOSGAM card (SOL 700): the gamma law equation of state of a gas, p = (gamma - 1) rho e. A MATDEUL names it in its
// EID field.
//
// Card layout: EOSGAM  EID  GAMMA  R  CV  CP  VISC            fields 1 to 6
// GAMMA is the ratio of the specific heats and R, CV and CP are the gas constants: the format reference ties the four
// together (gamma = cp / cv, R = cp - cv), so a card gives the ratio or two of the constants, and the temperature of
// the gas is computed when a constant is given. A blank field is a quantity the card does not give. VISC is the dynamic
// viscosity (zero or blank: no viscous stresses). The solver derives the missing values, so the card does not check
// that the given ones agree.
struct GammaLawStateEquationCard
{
    base::EntityId id = 0;                      // unique among the equations of state
    std::optional<double> gammaCoefficient;     // ratio of the specific heats, greater than zero
    std::optional<double> gasConstant;          // gas constant, greater than zero
    std::optional<double> specificHeatVolume;   // specific heat at constant volume, greater than zero
    std::optional<double> specificHeatPressure; // specific heat at constant pressure, greater than zero
    std::optional<double> viscosity;            // dynamic viscosity, not negative
};

} // namespace oor::model
