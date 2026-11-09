#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// The default of the hydrodynamic volume limit of an EOSPOL.
inline constexpr double POLYNOMIAL_STATE_EQUATION_VOLUME_LIMIT_DEFAULT = 1.1;

// One EOSPOL card (SOL 700): the polynomial equation of state of a solid. In compression (mu > 0, mu = rho / rho0 - 1)
// the pressure is a1 mu + a2 mu^2 + a3 mu^3 + (b0 + b1 mu + b2 mu^2 + b3 mu^3) rho0 e; in tension it is
// a1 mu + (b0 + b1 mu) rho0 e. A MATDEUL names it in its EID field.
//
// Card layout: EOSPOL  EID  A1  A2  A3  B0  B1  B2  B3            fields 1 to 8
//                     HVL  VISC                                   fields 9 and 10
// Every coefficient is a real, zero when blank (A1 is the bulk modulus of the linear law). HVL is the hydrodynamic
// volume limit: when the relative volume rho0 / rho goes beyond it the pressure is cut off (and with PARAM,HVLFAIL the
// element fails); it must exceed 1 and is 1.1 when blank. VISC is the viscosity coefficient, zero when blank. The
// format gives no range for the coefficients or for VISC, so the card checks only that they are finite.
struct PolynomialStateEquationCard
{
    base::EntityId id = 0; // unique among the equations of state
    double coefficientA1 = 0.0;
    double coefficientA2 = 0.0;
    double coefficientA3 = 0.0;
    double coefficientB0 = 0.0;
    double coefficientB1 = 0.0;
    double coefficientB2 = 0.0;
    double coefficientB3 = 0.0;
    double volumeLimit = POLYNOMIAL_STATE_EQUATION_VOLUME_LIMIT_DEFAULT; // hydrodynamic volume limit, greater than 1
    double viscosity = 0.0;                                              // viscosity coefficient
};

} // namespace oor::model
