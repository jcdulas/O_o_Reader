#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One EOSDEF card (SOL 700): the deflagration equation of state of a solid propellant that burns into hot gas, with the
// reaction rate that converts the propellant. The gas follows the Noble-Abel law. A MATDEUL names it in its EID field.
//
// Card layout: EOSDEF  EID  GAMMA  B  R  CV  CP  E  RHOS            fields 1 to 8
//                     RHOF  W  BETA  SAVR  X  Y                     fields 9 to 14
// The format calls the first field MID, a material number, although the card is an equation of state, numbered
// like the other EOSxxx cards and named by the EID of a MATDEUL: the library calls it EID, as every equation of state.
// GAMMA, B, E, RHOS, W, BETA, SAVR, X and Y are required. R, CV and CP are the gas constants of the format reference: a
// blank one is a constant the card does not give (the temperature of the gas is computed when one is given), and the
// card does not check that the given ones agree. RHOF is the ratio of the density of the powder to the reference
// density of the propellant (it lets the air between the grains count); the format does not call it required and gives
// it no default, so a blank RHOF is a quantity the card does not give. Y is meant to be positive, but Y = 0 is a
// legitimate form function: the card accepts Y >= 0.
struct DeflagrationStateEquationCard
{
    base::EntityId id = 0;                      // unique among the equations of state
    double gammaCoefficient = 0.0;              // greater than zero
    double coVolume = 0.0;                      // gas co-volume, not negative
    std::optional<double> gasConstant;          // gas constant, greater than zero
    std::optional<double> specificHeatVolume;   // specific heat at constant volume, greater than zero
    std::optional<double> specificHeatPressure; // specific heat at constant pressure, greater than zero
    double chemicalEnergy = 0.0;                // chemical energy per unit burned mass, greater than zero
    double referenceDensity = 0.0;              // reference density of the propellant, greater than zero
    std::optional<double> powderDensityRatio;   // density of powder over the reference density, greater than zero
    double burnRateCoefficient = 0.0;           // burning rate coefficient, not negative
    double burnRateExponent = 0.0;              // burning rate exponent, not negative
    double surfaceToVolumeRatio = 0.0;          // initial surface area over volume, greater than zero
    double formParameterX = 0.0;                // parameter of the form function, greater than zero
    double formParameterY = 0.0;                // parameter of the form function, not negative
};

} // namespace oor::model
