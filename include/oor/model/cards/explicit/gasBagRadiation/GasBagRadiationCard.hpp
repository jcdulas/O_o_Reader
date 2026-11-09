#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One HTRRAD card (SOL 700): the heat transfer through radiation from an air bag to its environment, through the
// surface of a COUPLE or a GBAG. A HEATLOS names it (its HTRTYPID, with the type HTRRAD). The rules of the HTRCONV
// apply (Euler: the single material hydrodynamic solver and an EOSGAM equation of state; energy leaves the bag
// only while it is warmer than the environment): rules of the model, not of the card.
//
// Card layout: HTRRAD  HTRID  GASEMI-C  GASEMI-T  TENV  SBOLTZ                         fields 1 to 5
// (the grid of the format prints the second field as GASBMI-C; its description and the rest of the format say
// GASEMI-C). HTRID is the number of the card, unique among the HTRRAD cards. The gas emissivity is a constant
// (GASEMI-C, a real of zero or more) or a function of time (GASEMI-T, the number of a TABLED1), and the format says
// that one of the two must be given: the card holds exactly one of them, as an HTRCONV does (a constant of 0.0 is a
// value, a blank is none). TENV is the temperature of the environment and SBOLTZ the Stefan-Boltzmann constant (the
// format spells it Stephan-Boltzman), both above zero and required. The members are named gasemic and gasemit, the
// format's GASEMI-C and GASEMI-T without the hyphen.
struct GasBagRadiationCard
{
    base::EntityId id = 0;                // the number of the card
    std::optional<double> emissivity;     // the constant emissivity, none when the table is given
    base::EntityId emissivityTableId = 0; // a TABLED1 of the emissivity against time, 0 when the constant is given
    double environmentTemperature = 0.0;  // the temperature of the environment
    double stefanBoltzmann = 0.0;         // the Stefan-Boltzmann constant
};

} // namespace oor::model
