#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One GBAGCOU card (SOL 700): the switch of an air bag from full gas dynamics (a COUPLE, the Euler-Lagrange coupling)
// to the uniform pressure formulation (a GBAG). After the transition every Eulerian and general coupling calculation is
// deactivated. The COUPLE and the GBAG must have the same BSID, a rule across cards that the
// check `explicit.gas-bag-switch-surface` verifies. No other entry of the format reference has this name.
//
// Card layout: GBAGCOU  ID  CID  GID  TSTART  PERCENT                                  fields 1 to 5
// ID is the number of the card, CID the COUPLE and GID the GBAG. GID has no "Required" on the format and a default of
// blank (its table says 0.0 for an integer), so a card without a GBAG is read. TSTART is the time after which the
// algorithm checks whether the switch is valid: the pressures Pmax, Pmin and Paverage exerted on the surface must
// satisfy a condition that is drawn as a formula, below PERCENT / 100. Both TSTART and PERCENT are zero or more and
// default to 0.0; a blank is a static default, so they are written blank when they are 0.0.
struct GasBagSwitchCard
{
    base::EntityId id = 0;
    base::EntityId couplingId = 0; // a COUPLE
    base::EntityId gasBagId = 0;   // a GBAG, 0 when blank
    double startTime = 0.0;        // the time after which the switch is checked
    double validityPercent = 0.0;  // the value of the validity check, in percent
};

} // namespace oor::model
