#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"

#include <optional>

namespace oor::model
{

// One PORHOLE card (SOL 700): a hole in a COUPLE or GBAG surface through which gas flows to the environment (a vent of
// an air bag). A LEAKAGE of PORTYPE PORHOLE names it.
//
// Card layout: PORHOLE  FID  SIZE  (blank)  FLOW  PENV  RHOENV  SIEENV  CP            fields 1, 2, 4 to 8
//             MID                                                                    field 9
// The format names the first field both FID and PID; the card uses FID, the name of the layout. SIZE
// is the flow method (SMALL when blank: one dimensional flow through a small orifice; LARGE: the velocity method of an
// Eulerian air bag), FLOW the allowed direction (BOTH when blank). PENV, RHOENV and SIEENV are the pressure, density
// and specific internal energy of the environment, which must be consistent with an ideal gas; the three are
// required. CP is
// the specific heat of the environment at constant pressure, needed only where the gas constants are updated. MID is
// the MATDEUL of the environment gas, used only for an Eulerian air bag with the multi-material solver and SIZE LARGE:
// blank otherwise.
struct GasBagVentHoleCard
{
    base::EntityId id = 0; // unique among the PORHOLE cards
    HoleSize size = HoleSize::Small;
    FlowDirection flow = FlowDirection::Both;
    double environmentPressure = 0.0;              // the environment pressure
    double environmentDensity = 0.0;               // the environment density
    double environmentEnergy = 0.0;                // the environment specific internal energy
    std::optional<double> environmentSpecificHeat; // the environment specific heat at constant pressure
    base::EntityId matId = 0;                      // a MATDEUL, 0 when blank
};

} // namespace oor::model
