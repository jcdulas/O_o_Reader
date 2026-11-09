#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"

#include <optional>

namespace oor::model
{

// One PERMEAB card (SOL 700): the permeability of a COUPLE or GBAG (sub)surface, the velocity of the gas that flows
// through it as a function of the pressure difference between the inside and the environment (a linear law, or a
// table). A LEAKAGE of PORTYPE PERMEAB names it.
//
// Card layout: PERMEAB  PID  PERMC  PERMT  FLOW  PENV  RHOENV  SIEENV  CP            fields 1 to 8
// PERMC is the coefficient of the linear law (the permeability is a constant minus PERMC times the absolute pressure
// difference), PERMT the number of a table of the permeability against that difference (the format types it only as
// an integer; the card takes a TABLED1, like the other tables of the explicit cards). The format says that either one
// must be given: the card refuses neither. It does not refuse both (the format does not say which would win), which
// also lets Python switch a card from one form to the other one property at a time. FLOW is the allowed direction (BOTH
// when blank). PENV, RHOENV and SIEENV describe the environment gas, which must agree with an ideal gas; the three are
// required. CP, the specific heat of the environment at constant pressure, is needed only where the gas constants are
// updated and stays blank otherwise.
struct SurfacePermeabilityCard
{
    base::EntityId id = 0;                  // unique among the PERMEAB cards
    std::optional<double> permeability;     // the linear coefficient, zero or more
    base::EntityId permeabilityTableId = 0; // a TABLED1, 0 when blank
    FlowDirection flow = FlowDirection::Both;
    double environmentPressure = 0.0;              // the environment pressure
    double environmentDensity = 0.0;               // the environment density
    double environmentEnergy = 0.0;                // the environment specific internal energy
    std::optional<double> environmentSpecificHeat; // the environment specific heat at constant pressure
};

} // namespace oor::model
