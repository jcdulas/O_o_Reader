#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"

#include <optional>

namespace oor::model
{

// One PERMGBG card (SOL 700): a permeable area of a COUPLE or GBAG surface that is connected to another GBAG. The
// velocity of the gas through it is a linear or a tabular function of the pressure difference, and the gas goes from
// the higher pressure to the lower. A LEAKAGE of PORTYPE PERMGBG names it, to model the flow between two GBAGs or
// between an Eulerian air bag and a GBAG.
//
// Card layout: PERMGBG  PID  PERMC  PERMT  FLOW  GBID            fields 1 to 5
// PERMC is the coefficient of the linear law and PERMT the number of a table of the permeability against the pressure
// difference (a TABLED1 here); the format says that either one must be given, so the card refuses neither (and, like
// PERMEAB, does not refuse both). FLOW is the allowed direction for the GBAG or the coupling surface that names the
// card (BOTH when blank). GBID is the GBAG on the other side, required. The description of PID calls the card a PERMEAB
// entry, which is a slip of the format: it describes this card.
struct GasBagPermeableLinkCard
{
    base::EntityId id = 0;                  // unique among the PERMGBG cards
    std::optional<double> permeability;     // the linear coefficient, zero or more
    base::EntityId permeabilityTableId = 0; // a TABLED1, 0 when blank
    FlowDirection flow = FlowDirection::Both;
    base::EntityId gasBagId = 0; // the GBAG on the other side
};

} // namespace oor::model
