#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One HTRCONV card (SOL 700): the heat transfer through convection from an air bag to its environment, through the
// surface of a COUPLE or a GBAG. A HEATLOS names it (its HTRTYPID, with the type HTRCONV). It can be used with Euler
// only with the single material hydrodynamic solver and an EOSGAM equation of state, and energy leaves the
// bag only while it is warmer than the environment: rules of the model, not of the card.
//
// Card layout: HTRCONV  HTRID  HTRCFC  HTRCFT  TENV                                  fields 1 to 4
// HTRID is the number of the card, unique among the HTRCONV cards. The convection coefficient is a constant (HTRCFC, a
// real of zero or more) or a function of time (HTRCFT, the number of a TABLED1), and the format says that one of the
// two must be given: the card holds exactly one of them (a card that gives both does not say which applies, and is
// refused). A constant of 0.0 is a coefficient, not a blank, which is why the
// constant is optional and the table is a plain ID that is 0 when blank. TENV is the temperature of the environment,
// above zero and required.
struct GasBagConvectionCard
{
    base::EntityId id = 0;                 // the number of the card
    std::optional<double> coefficient;     // the constant coefficient, none when the table is given
    base::EntityId coefficientTableId = 0; // a TABLED1 of the coefficient against time, 0 when the constant is given
    double environmentTemperature = 0.0;   // the temperature of the environment
};

} // namespace oor::model
