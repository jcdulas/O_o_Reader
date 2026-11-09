#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

// One INFLHB card (SOL 700): a hybrid airbag inflator, which blows a mixture of gases into a GBAG or an Euler coupling
// surface (COUPLE) through the ABINFL that names the card. The mass flow rate and the static temperature of the gas are
// tables of time, or the temperature a constant; the share of each gas in the mixture is a table of time too, the
// INFLFRC that FRAC names. With an Euler domain it works only with the single material hydrodynamic solver and an
// EOSGAM equation of state.
//
// Card layout, two lines or more (fields 6 to 8 are blank cells):
//   INFLHB  INFLID  MASFLRT  TEMPT  TEMPC  FRAC                                    fields 1 to 5
//   GASNAM1  GASNAM2  GASNAM3  etc.                                                 fields 9 and on, eight to a line
// (the grid of the format reference prints the first field as INFLHID and TEMPT as TBMPT; the field list says INFLID
// and TEMPT.) INFLID numbers the card (unique among the inflators of every kind, which the ABINFL INFTYPID names).
// MASFLRT and FRAC are required: the number of a TABLED1 and the number of an INFLFRC. The static temperature is TEMPT,
// the number of a TABLED1, or TEMPC, a constant: the format reference says that either one must be given, and the card
// reads them as the two forms of one field (exactly one of them). The gases are the numbers of INFLGAS cards, at least
// one and as many as needed; the order is the order of the fractions on the INFLFRC.
struct HybridInflatorCard
{
    base::EntityId id = 0;                 // unique number of the inflator
    base::EntityId massFlowTableId = 0;    // a TABLED1: the mass flow rate against time
    base::EntityId temperatureTableId = 0; // a TABLED1: the static temperature against time, 0 when TEMPC is given
    std::optional<double> temperature;     // the constant static temperature, greater than zero
    base::EntityId fractionId = 0;         // an INFLFRC: the fractions of the gases against time
    std::vector<base::EntityId> gasId;     // the INFLGAS of each gas, in order
};

} // namespace oor::model
